# Score-P AMD ROCm-smi Plugin

This repository provides a Score-P plugin to access the sensors provided by the ROCm-smi interface of AMDGPUs.

## Prerequisites

- Score-P
- rocm_smi library 

## Installation


```console
$ git clone --recursive-submodules https://github.com/score-p/scorep_plugin_rocm_smi.git
$ cd scorep_plugin_rocm_smi
$ cmake -S . -B build
$ cmake --build build
```

Make the resulting `librocm_smi_plugin.so` available via `LD_LIBRARY_PATH`. For
Score-P its also possible to copy into the library path of the installation.

## Usage

```console
$ export SCOREP_METRIC_PLUGINS=rocm_smi_plugin
$ export SCOREP_METRIC_ROCM_SMI_PLUGIN="edge_temp_current"
```

Sample interval is 50 ms by default and can be adjusted via (here Score-P)

```console
$ export SCOREP_METRIC_ROCM_SMI_PLUGIN_INTERVAL=100
```

Available and supported sensors for GPUs of the system can be retrieved with the
`get_supported_sensors` command in this repository.

## Debug

```console
$ export ROCM_SMI_PLUGIN_VERBOSE=DEBUG
```
