cmake_minimum_required(VERSION 3.10)
project(GPUMetric C)

# Force C11 standard for compilation
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure compiler flags for production optimization (-O3)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

# Include the current directory for project header lookups
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Try to find the directory containing nvml.h header file
find_path(NVML_INCLUDE_DIR nvml.h
        PATHS /usr/include /usr/local/cuda/include
)
if(NVML_INCLUDE_DIR)
    include_directories(${NVML_INCLUDE_DIR})
endif()

# Find the NVIDIA NVML library (searches for libnvidia-ml.so or libnvml.so)
find_library(NVML_LIB
        NAMES nvidia-ml nvml
        PATHS /usr/lib/x86_64-linux-gnu /usr/local/cuda/lib64 /usr/local/cuda/lib64/stubs
)

if(NOT NVML_LIB)
    message(FATAL_ERROR "NVIDIA NVML library not found. Please install headers: sudo apt install libnvidia-ml-dev")
endif()

# Build the project as a shared library (libgpumetric.so) instead of an executable
add_library(gpumetric SHARED
        src/gpumetric_core/gpu_metric.c
        src/gpumetric_core/gpu_metric.h
        src/gpumetric_core/logger.h
)

# Link the system NVML library to your shared library
target_link_libraries(gpumetric PRIVATE ${NVML_LIB})
