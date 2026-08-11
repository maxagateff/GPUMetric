cmake_minimum_required(VERSION 3.15)

project(
    GPUMetric
    VERSION 0.0.1
    LANGUAGES C
)

# ---------------------------------------------------------
# C standard
# ---------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---------------------------------------------------------
# Release optimization
# ---------------------------------------------------------

set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} -O3"
)

# ---------------------------------------------------------
# NVIDIA NVML
# ---------------------------------------------------------

find_path(
    NVML_INCLUDE_DIR
    NAMES nvml.h
    PATHS
        /usr/include
        /usr/include/nvidia
        /usr/local/cuda/include
)

if(NOT NVML_INCLUDE_DIR)
    message(FATAL_ERROR
        "NVIDIA NVML header (nvml.h) not found. "
        "Please install the NVIDIA NVML development package."
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

# ---------------------------------------------------------
# GPUMetric native library
# ---------------------------------------------------------

add_library(
    gpumetric
    SHARED
    src/gpumetric_core/gpu_metric.c
)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------

target_include_directories(
    gpumetric
    PRIVATE
        ${NVML_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gpumetric_core
)

# ---------------------------------------------------------
# Link NVML
# ---------------------------------------------------------

target_link_libraries(
    gpumetric
    PRIVATE
        ${NVML_LIBRARY}
)

# ---------------------------------------------------------
# Native library properties
# ---------------------------------------------------------

set_target_properties(
    gpumetric
    PROPERTIES
        OUTPUT_NAME "gpumetric"
)

# ---------------------------------------------------------
# Install native library into Python package
#
# Final wheel:
#
# gpumetric/
# ├── __init__.py
# ├── _ffi.py
# ├── metrics.py
# └── lib/
#     └── libgpumetric.so
#
# ---------------------------------------------------------

install(
        TARGETS gpumetric
        LIBRARY DESTINATION lib
)
