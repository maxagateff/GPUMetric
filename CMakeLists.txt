cmake_minimum_required(VERSION 3.15)


project(GPUMetric VERSION 0.1.0 LANGUAGES C)


set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


set(CMAKE_C_FLAGS_RELEASE
        "${CMAKE_C_FLAGS_RELEASE} -O3"
)


find_path(
        NVML_INCLUDE_DIR
        NAMES nvml.h
        PATHS
        /usr/include
        /usr/local/cuda/include
)

if(NOT NVML_INCLUDE_DIR)
    message(FATAL_ERROR
            "NVIDIA NVML header (nvml.h) not found. "
            "Install the NVIDIA NVML development package."
    )
endif()

find_library(
        NVML_LIBRARY
        NAMES nvidia-ml nvml
        PATHS
        /usr/lib/x86_64-linux-gnu
        /usr/lib64
        /usr/local/cuda/lib64
        /usr/local/cuda/lib64/stubs
)

if(NOT NVML_LIBRARY)
    message(FATAL_ERROR
            "NVIDIA NVML library not found. "
            "Please install the NVIDIA driver/NVML development files."
    )
endif()

message(STATUS "NVML include directory: ${NVML_INCLUDE_DIR}")
message(STATUS "NVML library: ${NVML_LIBRARY}")


add_library(gpumetric SHARED
        src/gpumetric_core/gpu_metric.c
        src/gpumetric_core/gpu_metric.h
        src/gpumetric_core/logger.h
)


target_include_directories(gpumetric
        PRIVATE
        ${NVML_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gpumetric_core
)


target_link_libraries(gpumetric
        PRIVATE
        ${NVML_LIBRARY}
)


set_target_properties(gpumetric PROPERTIES
        OUTPUT_NAME "gpumetric"
)


install(
        TARGETS gpumetric
        LIBRARY DESTINATION gpumetric/lib
)


install(FILES src/gpumetric_core/gpu_metric.h DESTINATION gpumetric/include)