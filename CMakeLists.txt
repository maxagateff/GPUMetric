cmake_minimum_required(VERSION 3.10)
project(GPUMetric C)

# Force C11 standard for compilation
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Configure compiler flags for production optimization (-O3)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

# Include the current directory for header lookups
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Find the NVIDIA NVML library on Ubuntu Server
find_library(NVML_LIB nvml)
if(NOT NVML_LIB)
    message(FATAL_ERROR "NVIDIA NVML library not found. Please install NVIDIA proprietary drivers.")
endif()

# Build the project as a shared library (libgpumetric.so) instead of an executable
add_library(gpumetric SHARED
        gpu_metric.c
        gpu_metric.h
        logger.h
)

# Link the system NVML library to your shared library
target_link_libraries(gpumetric PRIVATE ${NVML_LIB})
