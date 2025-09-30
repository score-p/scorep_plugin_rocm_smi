#include <scorep/plugin/plugin.hpp>
#include <scorep/plugin/util/matcher.hpp>

#include <mutex>
#include <regex>
#include <set>
#include <thread>

#include <rocm_smi/rocm_smi.h>

#include <rocm_sensor.hpp>
using scorep::plugin::logging;

using TVPair = std::pair<scorep::chrono::ticks, double>;

class RocmSmiMeasurementThread
{
    struct SensorReadings
    {
        std::vector<TVPair> readings;
        double offset{0.0};
    };

    public:
        RocmSmiMeasurementThread(std::chrono::milliseconds interval)
            : interval_(interval)
        {
        }

        void add_sensor(RocmSensor sensor)
        {
            data_.emplace(sensor, SensorReadings{});
            if (sensor.isAccumulated())
            {
                data_[sensor].offset = sensor.read();
            }
        }
        void measurement()
        {
            auto interval = interval_;
            while (!stop_)
            {
                auto until = std::chrono::steady_clock::now() + interval;
                {
                    std::lock_guard<std::mutex> lock(read_mutex_);
                    for (auto& sensor : data_)
                    {
                        double value = sensor.first.read() - sensor.second.offset;
                        sensor.second.readings.emplace_back(scorep::chrono::measurement_clock::now(), value);
                    }
                }
                while (until < std::chrono::steady_clock::now())
                {
                    until    += interval_;
                    interval += interval_;
                }
                std::this_thread::sleep_until(until);
            }
        }

        std::vector<TVPair> get_values_for_sensor(RocmSensor sensor)
        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            std::vector<TVPair> ret;
            std::swap(ret, data_.at(sensor).readings);
            return ret;
        }
        void start()
        {
            thread_ = std::thread([this]() { this->measurement(); });
        }
        void stop()
        {
            stop_ = true;

            if(thread_.joinable())
            {
                thread_.join();
            }
        }
    private:
        bool stop_ = false;
        std::mutex read_mutex_;
        std::chrono::milliseconds interval_;
        std::map<RocmSensor, SensorReadings> data_;
        std::thread thread_;
};

using namespace scorep::plugin::policy;

class rocm_smi_plugin
: public scorep::plugin::base<rocm_smi_plugin, async, per_topology, scorep_clock>
{
    public:
        rocm_smi_plugin()
        : measurement_interval_(
              std::chrono::milliseconds(stoi(scorep::environment_variable::get("INTERVAL", "50")))),
          measurement_thread_(measurement_interval_)
        {
            rsmi_init(0);

            /* Build map from PCI BFD to device index */
            uint32_t num_devices;
            rsmi_num_monitor_devices(&num_devices);
            for (int i = 0; i < num_devices; ++i)
            {
                uint64_t pci_id;
                if (rsmi_status_t ret = rsmi_dev_pci_id_get(i, &pci_id);
                    ret != RSMI_STATUS_SUCCESS)
                {
                    continue;
                }

                /* rocm_smi.h:
                 *      BDFID = ((DOMAIN & 0xFFFFFFFF) << 32) | ((Partition & 0xF) << 28)
                 *              | ((BUS & 0xFF) << 8) | ((DEVICE & 0x1F) <<3 )
                 *              | (FUNCTION & 0x7)
                 *
                 *  \code{.unparsed}
                 *  | Name         | Field   | KFD property       KFD -> PCIe ID (uint64_t)
                 *  -------------- | ------- | ---------------- | ---------------------------- |
                 *  | Domain       | [63:32] | "domain"         | (DOMAIN & 0xFFFFFFFF) << 32  |
                 *  | Partition id | [31:28] | "location id"    | (LOCATION & 0xF0000000)      |
                 *  | Reserved     | [27:16] | "location id"    | N/A                          |
                 *  | Bus          | [15: 8] | "location id"    | (LOCATION & 0xFF00)          |
                 *  | Device       | [ 7: 3] | "location id"    | (LOCATION & 0xF8)            |
                 *  | Function     | [ 2: 0] | "location id"    | (LOCATION & 0x7)             |
                 *  \endcode
                 */
                SCOREP_MetricTopologyNodeIdentifier bdfid;
                bdfid.pci.domain = (pci_id >> 32) & 0xFFFFFFFF;
                bdfid.pci.bus = (pci_id & 0xFF00) >> 8;
                bdfid.pci.device = (pci_id & 0xF8) >> 3;
                bdfid.pci.function = pci_id & 0x07;
                device_by_pci_.emplace(bdfid.id, i);

                logging::debug() << "Found ROCm device: " << i << " with PCI ID "
                    << bdfid.pci.domain << ":" << (unsigned int)bdfid.pci.bus << "." << (unsigned int)bdfid.pci.device << "." << (unsigned int)bdfid.pci.function;
            }
        }

        ~rocm_smi_plugin()
        {
            rsmi_shut_down();
        }

        std::vector<scorep::plugin::metric_property>
        get_metric_properties(const std::string& metricPattern)
        {
            std::vector<scorep::plugin::metric_property> result;
            auto sensors = RocmSensor::get_known_sensors();

            for (const auto& [type, sensor] : sensors)
            {
                if (scorep::plugin::util::matcher(metricPattern)(sensor.name))
                {
                    auto property = scorep::plugin::metric_property(
                        sensor.name,
                        sensor.description,
                        sensor.unit
                    )
                        .value_double();

                    if (sensor.accumulated)
                    {
                        property.accumulated_point();
                    }
                    else
                    {
                        property.absolute_point();
                    }
                    result.emplace_back(property);

                    logging::debug() << "Declared sensor " << sensor.name;
                }
            }
            return result;
        }

        std::vector<scorep::plugin::measurement_point>
        add_topology_metrics(const SCOREP_MetricTopologyNode* topologyRoot,
                             const std::string& metricName)
        {
            /* Find sensor based on metricName */
            const auto sensor_type = [metricName]()
            {
                for (const auto& [type, sensor] : RocmSensor::get_known_sensors())
                {
                    if (metricName == sensor.name)
                    {
                        return type;
                    }
                }
                logging::warn() << "Invalid sensor: " << metricName << ", ignored";
                throw std::exception();
            }();

            /* Collect all responsible GPU nodes */
            std::vector<const SCOREP_MetricTopologyNode*> nodes;
            SCOREP_MetricTopology_ForAllResponsiblePerDomain(
                topologyRoot, SCOREP_METRIC_TOPOLOGY_NODE_DOMAIN_ACCELERATOR_DEVICE,
                [](const SCOREP_MetricTopologyNode* node,
                   void* cbArg)
            {
                auto* nodes = static_cast<std::vector<const SCOREP_MetricTopologyNode*>*>(cbArg);
                nodes->emplace_back(node);
            }, (void*)&nodes);

            std::vector<scorep::plugin::measurement_point> results;
            for (const auto* node : nodes)
            {
                if (const auto match = device_by_pci_.find(node->id);
                    match != device_by_pci_.end())
                {
                    auto sensor = RocmSensor(match->second, sensor_type);
                    if (!sensor.supported())
                    {
                        logging::warn() << "Metric " << metricName << " not supported on device " << match->second;
                        continue;
                    }

                    logging::debug() << "Added sensor " << sensor.name() << " for device " << match->second;

                    const auto metric_id = static_cast<int32_t>(sensors_.size());
                    sensors_.emplace_back(sensor);
                    results.emplace_back(metric_id, node);
                }
            }
            return results;
        }

        template <typename C>
        void get_all_values(int32_t& id, C& cursor)
        {
            std::vector<TVPair> values = measurement_thread_.get_values_for_sensor(sensors_[id]);
            for (auto& value : values)
            {
                cursor.write(value.first, value.second);
            }
        }

        void start()
        {
            for (const auto& sensor : sensors_)
            {
                measurement_thread_.add_sensor(sensor);
            }
            measurement_thread_.start();
        }

        void stop()
        {
            measurement_thread_.stop();
        }

        void synchronize(bool, SCOREP_MetricSynchronizationMode)
        {
        }

    private:
        std::chrono::milliseconds measurement_interval_;
        RocmSmiMeasurementThread measurement_thread_;
        std::vector<RocmSensor> sensors_;
        std::map<uint64_t, uint32_t> device_by_pci_;
};

SCOREP_METRIC_PLUGIN_CLASS(rocm_smi_plugin, "rocm_smi");
