cmake_minimum_required(VERSION 2.8)

project(rocm_smi_plugin)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(PLUGIN_SOURCE src/rocm_smi.cpp)

add_subdirectory(lib/scorep_plugin_cxx_wrapper)


include_directories(rocm_smi_plugin PRIVATE
	include
	"/opt/rocm/include")
link_directories("/opt/rocm/lib")
add_library(rocm_smi_plugin SHARED ${PLUGIN_SOURCE})

add_executable(get_supported_sensors src/get_supported_sensors.cpp)
target_link_libraries(get_supported_sensors PRIVATE rocm_smi64)
target_link_libraries(rocm_smi_plugin PRIVATE rocm_smi64 Scorep::scorep-plugin-cxx)

install(TARGETS rocm_smi_plugin LIBRARY DESTINATION lib)
