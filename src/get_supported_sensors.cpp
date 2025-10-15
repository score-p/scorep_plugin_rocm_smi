#include <iostream>
#include <rocm_sensor.hpp>
#include <rocm_smi/rocm_smi.h>

int main(void)
{
    rsmi_init(0);
    uint32_t num_devices;
    rsmi_num_monitor_devices(&num_devices);
    auto sensors = RocmSensor::get_known_sensors();

    for (int i = 0; i < num_devices; ++i)
    {
        std::cout << "AMDGPU " << i << " supported sensors: " << std::endl;
        for (const auto& [type, sensor] : sensors)
        {
            if (auto rocm_sensor = RocmSensor(i, type);
                rocm_sensor.supported())
            {
                std::cout << "\t" << sensor.name << std::endl;
            }
        }
    }
    rsmi_shut_down();
}
