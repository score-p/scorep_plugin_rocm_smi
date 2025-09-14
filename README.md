# Score-P AMD ROCm-smi Plugin

This repository provides a Score-P plugin to access the sensors provided by the ROCm-smi interface of AMDGPUs.

## Prerequisites

- Score-P
- rocm_smi library 

## Installation


```
git clone --recursive-submodules https://github.com/score-p/scorep_plugin_rocm_smi.git
cd scorep_plugin_rocm_smi
mkdir build & cd build
cmake ../
make

#copy the resulting librocm_smi_plugin.so into your LD_LIBRARY_PATH
```

## Usage

```
export SCOREP_METRIC_PLUGINS=rocm_smi_plugin
export SCOREP_METRIC_ROCM_SMI_PLUGIN="ID0::EDGE_TEMP_CURRENT"
```

Sensor definitions that the form of:

```
ID[id of GPU]::[sensor name]
```

Available sensors for GPUs of the system can be retrieved with the get_supported_sensors command in this repository.
