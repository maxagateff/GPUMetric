# ---------------------------------------------------------
# Mock NVIDIA NVML library
# ---------------------------------------------------------

add_library(mock_nvml SHARED mock_nvml.c)
target_include_directories(mock_nvml PRIVATE ${PROJECT_SOURCE_DIR}/src/gpumetric_core)

# ---------------------------------------------------------
# Test NVML loader
# ---------------------------------------------------------

add_library(gpumetric_test_loader OBJECT ${PROJECT_SOURCE_DIR}/src/gpumetric_core/nvml_loader.c)
target_include_directories(gpumetric_test_loader PRIVATE ${PROJECT_SOURCE_DIR}/src/gpumetric_core)
target_compile_definitions(gpumetric_test_loader PRIVATE gpumetric_nvml_load=gpumetric_nvml_load_original)

# ---------------------------------------------------------
# Test GPUMetric Core
# ---------------------------------------------------------

add_library(gpumetric_test_core STATIC ${PROJECT_SOURCE_DIR}/src/gpumetric_core/gpu_metric.c ${PROJECT_SOURCE_DIR}/src/gpumetric_core/nvml_runtime.c mock_nvml_loader.c $<TARGET_OBJECTS:gpumetric_test_loader>)

target_include_directories(gpumetric_test_core PRIVATE ${PROJECT_SOURCE_DIR}/src/gpumetric_core)
target_compile_definitions(gpumetric_test_core PRIVATE GPUMETRIC_MOCK_NVML_PATH="$<TARGET_FILE:mock_nvml>")
target_link_libraries(gpumetric_test_core PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
add_dependencies(gpumetric_test_core mock_nvml)

# ---------------------------------------------------------
# GPU Context tests
# ---------------------------------------------------------

add_executable(test_gpu_context test_gpu_context.c)
target_include_directories(test_gpu_context PRIVATE ${PROJECT_SOURCE_DIR}/src/gpumetric_core)
target_link_libraries(test_gpu_context PRIVATE gpumetric_test_core)
add_test(NAME gpu_context COMMAND test_gpu_context)