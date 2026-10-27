cmake_minimum_required(VERSION 3.18)

project(
        gpumetric
        VERSION 0.1.3
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
        src/gpumetric_core/nvml_runtime.c
)

# ---------------------------------------------------------
# Shared library
# ---------------------------------------------------------
add_library(gpumetric SHARED ${GPUMETRIC_SOURCES})

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(gpumetric PUBLIC ${PROJECT_SOURCE_DIR}/src/gpumetric_core)

# ---------------------------------------------------------
# Threads
#
# Required for:
#   pthread_mutex_lock()
#   pthread_mutex_unlock()
# ---------------------------------------------------------
find_package(Threads REQUIRED)
target_link_libraries(gpumetric PRIVATE Threads::Threads)

# ---------------------------------------------------------
# libdl
#
# Required for:
#   dlopen()
#   dlsym()
#   dlclose()
# ---------------------------------------------------------

if(UNIX AND NOT APPLE)
    target_link_libraries(gpumetric PRIVATE ${CMAKE_DL_LIBS})
endif()

# ---------------------------------------------------------
# Compiler warnings & Strict Mode (-Werror)
# ---------------------------------------------------------

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gpumetric PRIVATE -Wall -Wextra -Wpedantic)

    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(gpumetric PRIVATE -Wno-newline-eof)
    endif()
endif()

set_target_properties(gpumetric PROPERTIES COMPILE_WARNING_AS_ERROR ON)

# ---------------------------------------------------------
# Position Independent Code
# ---------------------------------------------------------
set_target_properties(gpumetric PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------
# Output name
# ---------------------------------------------------------
set_target_properties(gpumetric PROPERTIES OUTPUT_NAME gpumetric)

# ---------------------------------------------------------
# Install
# ---------------------------------------------------------

include(GNUInstallDirs)
install(TARGETS gpumetric LIBRARY DESTINATION gpumetric/lib RUNTIME DESTINATION gpumetric/lib ARCHIVE DESTINATION gpumetric/lib)
install(FILES src/gpumetric_core/gpu_metric.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gpumetric)

# ---------------------------------------------------------
# Tests
# ---------------------------------------------------------

include(CTest)

if(BUILD_TESTING AND EXISTS "${PROJECT_SOURCE_DIR}/tests/c/CMakeLists.txt")
    add_subdirectory(tests/c)
endif()

# ---------------------------------------------------------
# Build information
# ---------------------------------------------------------

message(STATUS "")
message(STATUS "========== GPUMetric ==========")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "C standard:     C${CMAKE_C_STANDARD}")
message(STATUS "NVML loading:   runtime (dlopen)")
message(STATUS "NVML linking:   disabled")
message(STATUS "NVML headers:   internal runtime ABI declarations")
message(STATUS "Threads:        enabled")
message(STATUS "================================")
message(STATUS "")