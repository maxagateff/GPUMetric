cmake_minimum_required(VERSION 3.18)

project(
        gpumetric
        VERSION 0.1.2
        LANGUAGES C
)

# ---------------------------------------------------------
# C standard
# ---------------------------------------------------------

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---------------------------------------------------------
# Sources
# ---------------------------------------------------------

set(GPUMETRIC_SOURCES
        src/gpumetric_core/gpu_metric.c
        src/gpumetric_core/nvml_loader.c
)

# ---------------------------------------------------------
# Shared library
# ---------------------------------------------------------

add_library(
        gpumetric
        SHARED
        ${GPUMETRIC_SOURCES}
)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------

target_include_directories(
        gpumetric
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src/gpumetric_core
)

# ---------------------------------------------------------
# NVIDIA NVML headers
#
# NVML is loaded dynamically at runtime through dlopen().
# We DO NOT link against libnvidia-ml.so.
# ---------------------------------------------------------

find_path(
        NVML_INCLUDE_DIR
        NAMES nvml.h
        PATHS
        /usr/include
        /usr/local/include
        /usr/local/cuda/include
        /usr/local/cuda-*/include
)

if(NOT NVML_INCLUDE_DIR)
    message(
            FATAL_ERROR
            "NVIDIA NVML header (nvml.h) not found."
    )
endif()

message(
        STATUS
        "NVML include directory: ${NVML_INCLUDE_DIR}"
)

target_include_directories(
        gpumetric
        PRIVATE
        ${NVML_INCLUDE_DIR}
)

# ---------------------------------------------------------
# libdl
#
# Required for:
#   dlopen()
#   dlsym()
#   dlclose()
# ---------------------------------------------------------

if(UNIX AND NOT APPLE)
    target_link_libraries(
            gpumetric
            PRIVATE
            dl
    )
endif()

# ---------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
            gpumetric
            PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

# ---------------------------------------------------------
# Position Independent Code
# ---------------------------------------------------------

set_target_properties(
        gpumetric
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

# ---------------------------------------------------------
# Output name
# ---------------------------------------------------------

set_target_properties(
        gpumetric
        PROPERTIES
        OUTPUT_NAME gpumetric
)

# ---------------------------------------------------------
# Install
# ---------------------------------------------------------

include(GNUInstallDirs)

install(
        TARGETS gpumetric
        LIBRARY DESTINATION gpumetric/lib
        RUNTIME DESTINATION gpumetric/lib
        ARCHIVE DESTINATION gpumetric/lib
)

install(
        FILES
        src/gpumetric_core/gpu_metric.h
        DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/gpumetric
)

# ---------------------------------------------------------
# Build information
# ---------------------------------------------------------

message(STATUS "")
message(STATUS "========== GPUMetric ==========")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "C standard:     C${CMAKE_C_STANDARD}")
message(STATUS "NVML loading:   runtime (dlopen)")
message(STATUS "NVML linking:   disabled")
message(STATUS "NVML include:   ${NVML_INCLUDE_DIR}")
message(STATUS "================================")
message(STATUS "")
