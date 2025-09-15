#include <scorep/plugin/plugin.hpp>

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
    public:
        RocmSmiMeasurementThread(std::chrono::milliseconds interval)
            : interval_(interval)
        {
        }

        void add_sensor(RocmSensor sensor)
        {
            data_.emplace(sensor, std::vector<TVPair>());
        }
        void measurement()
        {
            while (!stop_)
            {
                for (auto& sensor : data_)
                {
                    std::lock_guard<std::mutex> lock(read_mutex_);
                    double value = sensor.first.read();
                    sensor.second.emplace_back(scorep::chrono::measurement_clock::now(), value);
                }
                std::this_thread::sleep_for(interval_);
            }
        }

        std::vector<TVPair> get_values_for_sensor(RocmSensor sensor)
        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            auto ret = data_.at(sensor);
            data_.at(sensor).clear();
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
        std::map<RocmSensor, std::vector<TVPair>> data_;
        std::thread thread_;
};

using namespace scorep::plugin::policy;

template <typename T, typename Policies>
using rocm_smi_object_id = object_id<RocmSensor, T, Policies>;

class rocm_smi_plugin
: public scorep::plugin::base<rocm_smi_plugin, async, once, scorep_clock, rocm_smi_object_id>
{
    public:
        rocm_smi_plugin()
        : measurement_interval_(
              std::chrono::milliseconds(stoi(scorep::environment_variable::get("INTERVAL", "50")))),
          measurement_thread_(measurement_interval_)
        {
            rsmi_init(0);
        }

        ~rocm_smi_plugin()
        {
            rsmi_shut_down();
        }

        std::vector<scorep::plugin::metric_property>
        get_metric_properties(const std::string& metric_name)
        {
            std::set<RocmSensor> rocm_sensors;
            uint32_t num_devices;

            std::regex sensor_regex("ID([^:]+)::(.+)");
            std::smatch sensor_match;
            if (std::regex_match(metric_name, sensor_match, sensor_regex))
            {
                logging::warn() << sensor_match[0];
                if (sensor_match[1] == "*")
                {
                    auto type = RocmSensor::Type::INVALID;
                    auto sensors = RocmSensor::get_sensor_names();

                    for (auto sensor : sensors)
                    {
                        if (sensor_match[2] == sensor.second)
                        {
                            type = sensor.first;
                            break;
                        }
                    }

                    if (type == RocmSensor::Type::INVALID)
                    {
                        logging::warn() << "Invalid sensor: " << sensor_match[2] << ", ignored";
                        return {};
                    }

                    rsmi_num_monitor_devices(&num_devices);
                    for (int i = 0; i < num_devices; ++i)
                    {
                        rocm_sensors.emplace(i, type);
                        measurement_thread_.add_sensor(RocmSensor(i, type));
                    }
                }
                else
                {
                    logging::warn() << "Invalid AMDGPU id: " << sensor_match[1] << ", ignored";
                }
            }
            else
            {
                logging::warn() << "Invalid sensor: " << metric_name << ", ignored";
            }

            std::vector<scorep::plugin::metric_property> ret;
            for (auto& sensor : rocm_sensors)
            {
                if (!sensor.supported())
                {
                    logging::warn() << "sensor \'" << sensor.name() << "\' not supported, ignoring";
                    continue;
                }
                ret.push_back(scorep::plugin::metric_property(sensor.name(), sensor.description(),
                                                              sensor.unit())
                                  .absolute_point()
                                  .value_double());
                make_handle(sensor.name(), sensor);
            }
            return ret;
        }

        template <typename C>
        void get_all_values(RocmSensor& id, C& cursor)
        {
            std::vector<TVPair> values = measurement_thread_.get_values_for_sensor(id);
            for (auto& value : values)
            {
                cursor.write(value.first, value.second);
            }
        }

        void add_metric(RocmSensor& id)
        {
        }

        void start()
        {
            measurement_thread_.start();
        }

        void stop()
        {
            measurement_thread_.stop();
        }
    private:
        std::chrono::milliseconds measurement_interval_;
        RocmSmiMeasurementThread measurement_thread_;
};

SCOREP_METRIC_PLUGIN_CLASS(rocm_smi_plugin, "rocm_smi");
