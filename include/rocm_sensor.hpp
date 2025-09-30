
#pragma once

#include <map>
#include <rocm_smi/rocm_smi.h>
enum class RocmSensorType
{
    INVALID,
    SOCKET_POWER,
    AVERAGE_POWER,
    ENERGY_COUNT,
    MEMORY_USAGE_VRAM,
    MEMORY_USAGE_VIS_VRAM,
    MEMORY_USAGE_GTT,
    MEMORY_BUSY,
    FAN_SPEED,
    EDGE_TEMP_CURRENT,
    JUNCTION_TEMP_CURRENT,
    MEMORY_TEMP_CURRENT,
    HBM_0_TEMP_CURRENT,
    HBM_1_TEMP_CURRENT,
    HBM_2_TEMP_CURRENT,
    HBM_3_TEMP_CURRENT,
    VDDGFX_VOLT_CURRENT,
    VDDGFX_VOLT_AVERAGE,
    DEVICE_BUSY,
};

class RocmSensor
{
public:
    RocmSensor(uint32_t device_index, RocmSensorType type)
    : device_index_(device_index), type_(type)
    {
    }

    static std::map<RocmSensorType, std::string> get_sensor_names()
    {
        return {
            { RocmSensorType::SOCKET_POWER, "socket_power" },
            { RocmSensorType::AVERAGE_POWER, "average_power" },
            { RocmSensorType::ENERGY_COUNT, "energy_count" },
            { RocmSensorType::MEMORY_USAGE_VRAM, "memory_usage_vram" },
            { RocmSensorType::MEMORY_USAGE_VIS_VRAM, "memory_usage_vis_vram" },
            { RocmSensorType::MEMORY_USAGE_GTT, "memory_usage_gtt" },
            { RocmSensorType::MEMORY_BUSY, "memory_busy" },
            { RocmSensorType::FAN_SPEED, "fan_speed" },
            { RocmSensorType::EDGE_TEMP_CURRENT, "edge_temp_current" },
            { RocmSensorType::JUNCTION_TEMP_CURRENT, "junction_temp_current" },
            { RocmSensorType::MEMORY_TEMP_CURRENT, "memory_temp_current" },
            { RocmSensorType::HBM_0_TEMP_CURRENT, "hbm0_temp_current" },
            { RocmSensorType::HBM_1_TEMP_CURRENT, "hbm1_temp_current" },
            { RocmSensorType::HBM_2_TEMP_CURRENT, "hbm2_temp_current" },
            { RocmSensorType::HBM_3_TEMP_CURRENT, "hbm3_temp_current" },
            { RocmSensorType::VDDGFX_VOLT_CURRENT, "vddgfx_volt_current" },
            { RocmSensorType::VDDGFX_VOLT_AVERAGE, "vddgfx_volt_average" },
            { RocmSensorType::DEVICE_BUSY, "device_busy" },
        };
    }

    std::string name() const
    {
        std::string sensor = "";

        auto sensors = get_sensor_names();
        if (sensors.count(type_))
        {
            sensor = sensors.at(type_);
        }
        return std::string("ID") + std::to_string(device_index_) + "::" + sensor;
    }

    std::string description() const
    {
        switch (type_)
        {
        case RocmSensorType::SOCKET_POWER:
            return "Socket Power";
        case RocmSensorType::AVERAGE_POWER:
            return "Average Power";
        case RocmSensorType::ENERGY_COUNT:
            return "Energy Count";
        case RocmSensorType::MEMORY_USAGE_VRAM:
            return "Memory Usage (VRAM)";
        case RocmSensorType::MEMORY_USAGE_VIS_VRAM:
            return "Memory Usage (Visible VRAM)";
        case RocmSensorType::MEMORY_USAGE_GTT:
            return "Memory Usage (GTT)";
        case RocmSensorType::MEMORY_BUSY:
            return "Memory Busy Percent";
        case RocmSensorType::FAN_SPEED:
            return "Fan Speed";
        case RocmSensorType::EDGE_TEMP_CURRENT:
            return "Current Edge GPU temperature";
        case RocmSensorType::JUNCTION_TEMP_CURRENT:
            return "Current junction/hotspot temperature";
        case RocmSensorType::MEMORY_TEMP_CURRENT:
            return "Current GPU memory temperature";
        case RocmSensorType::HBM_0_TEMP_CURRENT:
            return "Current HBM0 temperature";
        case RocmSensorType::HBM_1_TEMP_CURRENT:
            return "Current HBM1 temperature";
        case RocmSensorType::HBM_2_TEMP_CURRENT:
            return "Current HBM2 temperature";
        case RocmSensorType::HBM_3_TEMP_CURRENT:
            return "Current HBM3 temperature";
        case RocmSensorType::VDDGFX_VOLT_CURRENT:
            return "Current Vdd_gfx voltage";
        case RocmSensorType::VDDGFX_VOLT_AVERAGE:
            return "Average Vdd_gfx voltage";
        case RocmSensorType::DEVICE_BUSY:
            return "Percentage of time device is busy";
        }
        return "unknown";
    }

    std::string unit() const
    {
        switch (type_)
        {
        case RocmSensorType::SOCKET_POWER:
            return "W";
        case RocmSensorType::AVERAGE_POWER:
            return "W";
        case RocmSensorType::ENERGY_COUNT:
            return "J";
        case RocmSensorType::MEMORY_USAGE_VRAM:
        case RocmSensorType::MEMORY_USAGE_VIS_VRAM:
        case RocmSensorType::MEMORY_USAGE_GTT:
            // TODO: Real Unit
            return "B";
        case RocmSensorType::MEMORY_BUSY:
        case RocmSensorType::DEVICE_BUSY:
            return "%";
        case RocmSensorType::FAN_SPEED:
            return "rpm";
        case RocmSensorType::EDGE_TEMP_CURRENT:
        case RocmSensorType::JUNCTION_TEMP_CURRENT:
        case RocmSensorType::MEMORY_TEMP_CURRENT:
        case RocmSensorType::HBM_0_TEMP_CURRENT:
        case RocmSensorType::HBM_1_TEMP_CURRENT:
        case RocmSensorType::HBM_2_TEMP_CURRENT:
        case RocmSensorType::HBM_3_TEMP_CURRENT:
            return "C";
        case RocmSensorType::VDDGFX_VOLT_CURRENT:
        case RocmSensorType::VDDGFX_VOLT_AVERAGE:
            return "V";
        }
        return "#";
    }

    bool supported() const
    {
        uint64_t val;
        uint32_t val32;
        float counter_resolution;
        uint64_t timestamp;
        rsmi_status_t ret;
        switch (type_)
        {
        case RocmSensorType::SOCKET_POWER:
            ret = rsmi_dev_current_socket_power_get(device_index_, &val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::AVERAGE_POWER:
            ret = rsmi_dev_power_ave_get(device_index_, 0, &val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::ENERGY_COUNT:
            ret = rsmi_dev_energy_count_get(device_index_, &val, &counter_resolution, &timestamp);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::MEMORY_USAGE_VRAM:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_VRAM, &val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::MEMORY_USAGE_VIS_VRAM:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_VIS_VRAM, &val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::MEMORY_USAGE_GTT:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_GTT, &val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::MEMORY_BUSY:
            ret = rsmi_dev_memory_busy_percent_get(device_index_, &val32);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::FAN_SPEED:
            ret = rsmi_dev_fan_rpms_get(device_index_, 0, (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::EDGE_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_EDGE, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::JUNCTION_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_JUNCTION,
                                           RSMI_TEMP_CURRENT, (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::MEMORY_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_MEMORY, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::HBM_0_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_0, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::HBM_1_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_1, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::HBM_2_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_2, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::HBM_3_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_3, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::VDDGFX_VOLT_CURRENT:
            ret = rsmi_dev_volt_metric_get(device_index_, RSMI_VOLT_TYPE_VDDGFX, RSMI_VOLT_CURRENT,
                                           (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::VDDGFX_VOLT_AVERAGE:
            ret = rsmi_dev_volt_metric_get(device_index_, RSMI_VOLT_TYPE_VDDGFX, RSMI_VOLT_AVERAGE,
                                           (int64_t*)&val);
            return (ret == RSMI_STATUS_SUCCESS);
        case RocmSensorType::DEVICE_BUSY:
            ret = rsmi_dev_busy_percent_get(device_index_, &val32);
            return (ret == RSMI_STATUS_SUCCESS);
        default:
            return false;
        }
        return (ret == RSMI_STATUS_SUCCESS);
    }

    double read() const
    {
        uint64_t val;
        float counter_resolution;
        uint64_t timestamp;
        uint32_t val32;
        rsmi_status_t ret;
        switch (type_)
        {
        case RocmSensorType::SOCKET_POWER:
            ret = rsmi_dev_current_socket_power_get(device_index_, &val);
            return val * 0.000001;
        case RocmSensorType::AVERAGE_POWER:
            ret = rsmi_dev_power_ave_get(device_index_, 0, &val);
            return val * 0.000001;
        case RocmSensorType::ENERGY_COUNT:
            ret = rsmi_dev_energy_count_get(device_index_, &val, &counter_resolution, &timestamp);
            return val * counter_resolution * 0.000001;
        case RocmSensorType::MEMORY_USAGE_VRAM:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_VRAM, &val);
            return val;
        case RocmSensorType::MEMORY_USAGE_VIS_VRAM:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_VIS_VRAM, &val);
            return val;
        case RocmSensorType::MEMORY_USAGE_GTT:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_GTT, &val);
            return val;
        case RocmSensorType::MEMORY_BUSY:
            ret = rsmi_dev_memory_busy_percent_get(device_index_, &val32);
            return val32;
        case RocmSensorType::FAN_SPEED:
            ret = rsmi_dev_fan_rpms_get(device_index_, 0, (int64_t*)&val);
            return val;
        case RocmSensorType::EDGE_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_EDGE, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::JUNCTION_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_JUNCTION,
                                           RSMI_TEMP_CURRENT, (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::MEMORY_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_MEMORY, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::HBM_0_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_0, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::HBM_1_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_1, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::HBM_2_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_2, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::HBM_3_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_3, RSMI_TEMP_CURRENT,
                                           (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::VDDGFX_VOLT_CURRENT:
            ret = rsmi_dev_volt_metric_get(device_index_, RSMI_VOLT_TYPE_VDDGFX, RSMI_VOLT_CURRENT,
                                           (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::VDDGFX_VOLT_AVERAGE:
            ret = rsmi_dev_volt_metric_get(device_index_, RSMI_VOLT_TYPE_VDDGFX, RSMI_VOLT_AVERAGE,
                                           (int64_t*)&val);
            return val * 0.001;
        case RocmSensorType::DEVICE_BUSY:
            ret = rsmi_dev_busy_percent_get(device_index_, &val32);
            return val32;
        default:
            return 0.0;
        }
    }

    friend bool operator<(const RocmSensor& lhs, const RocmSensor& rhs)
    {
        if (lhs.device_index_ == rhs.device_index_)
        {
            return lhs.type_ < rhs.type_;
        }
        return lhs.device_index_ < rhs.device_index_;
    };

private:
    uint32_t device_index_;
    RocmSensorType type_;
};
