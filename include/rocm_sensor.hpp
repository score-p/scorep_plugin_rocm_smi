
#pragma once

#include <map>
#include <rocm_smi/rocm_smi.h>

class RocmSensor
{
public:
    enum class Type
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

    struct Properties
    {
        std::string name;
        std::string description;
        std::string unit;
    };

public:
    RocmSensor(uint32_t device_index, Type type)
    : device_index_(device_index), type_(type)
    {
    }

    static std::map<Type, Properties> get_known_sensors()
    {
        return {
            { Type::SOCKET_POWER,
              { "socket_power", "Socket Power", "W" } },
            { Type::AVERAGE_POWER,
              { "average_power", "Average Power", "W"} },
            { Type::ENERGY_COUNT,
              { "energy_count", "Energy Count", "J"} },
            { Type::MEMORY_USAGE_VRAM,
              { "memory_usage_vram", "Memory Usage (VRAM)", "B"} }, // TODO: Real Unit
            { Type::MEMORY_USAGE_VIS_VRAM,
              { "memory_usage_vis_vram", "Memory Usage (Visible VRAM)", "B"} }, // TODO: Real Unit
            { Type::MEMORY_USAGE_GTT,
              { "memory_usage_gtt", "Memory Usage (GTT)", "B"} }, // TODO: Real Unit
            { Type::MEMORY_BUSY,
              { "memory_busy", "Memory Busy Percent", "%"} },
            { Type::FAN_SPEED,
              { "fan_speed", "Fan Speed", "rpm" } },
            { Type::EDGE_TEMP_CURRENT,
              { "edge_temp_current", "Current Edge GPU temperature", "°C" } },
            { Type::JUNCTION_TEMP_CURRENT,
              { "junction_temp_current", "Current junction/hotspot temperature", "°C" } },
            { Type::MEMORY_TEMP_CURRENT,
              { "memory_temp_current", "Current GPU memory temperature", "°C" } },
            { Type::HBM_0_TEMP_CURRENT,
              { "hbm0_temp_current", "Current HBM0 temperature", "°C" } },
            { Type::HBM_1_TEMP_CURRENT,
              { "hbm1_temp_current", "Current HBM1 temperature", "°C" } },
            { Type::HBM_2_TEMP_CURRENT,
              { "hbm2_temp_current", "Current HBM2 temperature", "°C" } },
            { Type::HBM_3_TEMP_CURRENT,
              { "hbm3_temp_current", "Current HBM3 temperature", "°C" } },
            { Type::VDDGFX_VOLT_CURRENT,
              { "vddgfx_volt_current", "Current Vdd_gfx voltage", "V" } },
            { Type::VDDGFX_VOLT_AVERAGE,
              { "vddgfx_volt_average", "Average Vdd_gfx voltage", "V" } },
            { Type::DEVICE_BUSY,
              { "device_busy", "Percentage of time device is busy", "%"} },
        };
    }

    std::string name() const
    {
        if (auto sensors = get_known_sensors();
            sensors.count(type_) != 0)
        {
            return sensors.at(type_).name;
        }
        return {};
    }

    std::string description() const
    {
        if (auto sensors = get_known_sensors();
            sensors.count(type_) != 0)
        {
            return sensors.at(type_).description;
        }
        return {};
    }

    std::string unit() const
    {
        if (auto sensors = get_known_sensors();
            sensors.count(type_) != 0)
        {
            return sensors.at(type_).unit;
        }
        return {};
    }

    double read() const
    {
        uint64_t u64;
        int64_t i64;
        uint32_t u32;
        float counter_resolution;
        uint64_t timestamp;
        rsmi_status_t ret;
        switch (type_)
        {
        case Type::SOCKET_POWER:
            ret = rsmi_dev_current_socket_power_get(device_index_, &u64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return u64 * 0.000001;
            }
            break;
        case Type::AVERAGE_POWER:
            ret = rsmi_dev_power_ave_get(device_index_, 0, &u64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return u64 * 0.000001;
            }
            break;
        case Type::ENERGY_COUNT:
            ret = rsmi_dev_energy_count_get(device_index_, &u64,
                                            &counter_resolution, &timestamp);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return u64 * counter_resolution * 0.000001;
            }
            break;
        case Type::MEMORY_USAGE_VRAM:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_VRAM, &u64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return u64;
            }
            break;
        case Type::MEMORY_USAGE_VIS_VRAM:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_VIS_VRAM, &u64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return u64;
            }
            break;
        case Type::MEMORY_USAGE_GTT:
            ret = rsmi_dev_memory_usage_get(device_index_, RSMI_MEM_TYPE_GTT, &u64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return u64;
            }
            break;
        case Type::MEMORY_BUSY:
            ret = rsmi_dev_memory_busy_percent_get(device_index_, &u32);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return u32;
            }
            break;
        case Type::FAN_SPEED:
            ret = rsmi_dev_fan_rpms_get(device_index_, 0, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64;
            }
            break;
        case Type::EDGE_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_EDGE,
                                           RSMI_TEMP_CURRENT, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::JUNCTION_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_JUNCTION,
                                           RSMI_TEMP_CURRENT, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::MEMORY_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_MEMORY,
                                           RSMI_TEMP_CURRENT, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::HBM_0_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_0,
                                           RSMI_TEMP_CURRENT, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::HBM_1_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_1,
                                           RSMI_TEMP_CURRENT, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::HBM_2_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_2,
                                           RSMI_TEMP_CURRENT, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::HBM_3_TEMP_CURRENT:
            ret = rsmi_dev_temp_metric_get(device_index_, RSMI_TEMP_TYPE_HBM_3,
                                           RSMI_TEMP_CURRENT, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::VDDGFX_VOLT_CURRENT:
            ret = rsmi_dev_volt_metric_get(device_index_, RSMI_VOLT_TYPE_VDDGFX,
                                           RSMI_VOLT_CURRENT, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::VDDGFX_VOLT_AVERAGE:
            ret = rsmi_dev_volt_metric_get(device_index_, RSMI_VOLT_TYPE_VDDGFX,
                                           RSMI_VOLT_AVERAGE, &i64);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return i64 * 0.001;
            }
            break;
        case Type::DEVICE_BUSY:
            ret = rsmi_dev_busy_percent_get(device_index_, &u32);
            if (ret == RSMI_STATUS_SUCCESS)
            {
                return u32;
            }
            break;
        }
        const char* error_string;
        rsmi_status_string(ret, &error_string);
        throw std::runtime_error(std::string(error_string));
    }

    bool supported() const
    {
        try
        {
            read();
            return true;
        }
        catch (std::runtime_error& e)
        {
            return false;
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
    Type type_;
};
