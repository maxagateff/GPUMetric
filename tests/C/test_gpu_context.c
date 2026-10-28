#include "gpu_metric.h"
#include "mock_nvml_control.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); exit(EXIT_FAILURE); } } while (0)

static void *sample_thread(void *argument) {
    GPUContext *ctx = argument;
    GPUStats stats = {0};
    for (int i = 0; i < 500; i++) {
        if (gpu_metric_context_sample(ctx, &stats) != GPU_METRIC_SUCCESS) return (void *)1;
    }
    return NULL;
}

int main(void) {
    GPUContext *gpu0 = NULL;
    GPUContext *gpu1 = NULL;

    GPUStats stats0 = {0};
    GPUStats stats1 = {0};

    // Initialize the contexts so that they are not NULL
    CHECK(gpu_metric_create_by_index(0, &gpu0) == GPU_METRIC_SUCCESS);
    CHECK(gpu_metric_create_by_index(1, &gpu1) == GPU_METRIC_SUCCESS);

    // We set an initial value of 1024 MB so that the context remembers it.
    mock_nvml_set_memory_used(0, 1024ULL * 1024 * 1024);
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    CHECK(stats0.mem_mib == 1024);

    // Now, let’s run original test to check for errors.
    mock_nvml_set_memory_used(0, 1536ULL * 1024 * 1024);
    mock_nvml_set_temperature_failure(NVML_ERROR_NOT_SUPPORTED);

    //The yield drops due to the temperature, but the memory capacity should remain the same (1024).
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_ERR_DEVICE);
    CHECK(stats0.mem_mib == 1024);

    // Clear the errors, check the delta (1536 - 1024 = 512)
    mock_nvml_reset_failures();
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    CHECK(stats0.mem_mib == 1536);
    CHECK(stats0.delta_mib == 512);

    // Breaking the memory. The build fails, but the memory remains at the old value (1536).
    mock_nvml_set_memory_used(0, 512ULL * 1024 * 1024);
    mock_nvml_set_memory_v2_failure(NVML_ERROR_NOT_SUPPORTED);
    mock_nvml_set_memory_failure(NVML_ERROR_NOT_SUPPORTED);
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_ERR_DEVICE);

    // Fixing. Memory dropped to 512, delta (512 - 1536 = -1024)
    mock_nvml_reset_failures();
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    CHECK(stats0.mem_mib == 512);
    CHECK(stats0.delta_mib == -1024);

    // Multi-threaded test
    pthread_t threads[4];
    for (int i = 0; i < 4; i++) { CHECK(pthread_create(&threads[i], NULL, sample_thread, gpu0) == 0); }
    for (int i = 0; i < 4; i++) {
        void *result = NULL;
        CHECK(pthread_join(threads[i], &result) == 0);
        CHECK(result == NULL);
    }

    /* MemoryInfo v2: successful collection */
    mock_nvml_set_memory_used(0, 1024ULL * 1024 * 1024);
    mock_nvml_set_memory_reserved(0, 256ULL * 1024 * 1024);

    unsigned int v1_before = mock_nvml_memory_v1_calls();
    unsigned int v2_before = mock_nvml_memory_v2_calls();

    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    CHECK(stats0.mem_mib == 1024);
    CHECK(stats0.reserved_mem_mib == 256);
    CHECK(mock_nvml_memory_v2_calls() == v2_before + 1);
    CHECK(mock_nvml_memory_v1_calls() == v1_before);

    /* MemoryInfo v2: fallback when unsupported */
    mock_nvml_set_memory_v2_failure(NVML_ERROR_NOT_SUPPORTED);

    v1_before = mock_nvml_memory_v1_calls();

    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    CHECK(stats0.mem_mib == 1024);
    CHECK(stats0.reserved_mem_mib == 0);
    CHECK(mock_nvml_memory_v1_calls() == v1_before + 1);

    mock_nvml_reset_failures();

    /* MemoryInfo v2: fallback when function is unavailable */
    mock_nvml_set_memory_v2_failure(NVML_ERROR_FUNCTION_NOT_FOUND);
    v1_before = mock_nvml_memory_v1_calls();
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    CHECK(mock_nvml_memory_v1_calls() == v1_before + 1);
    mock_nvml_reset_failures();

    /* MemoryInfo v2: unexpected error must not trigger fallback */
    mock_nvml_set_memory_v2_failure(NVML_ERROR_LIB_RM_VERSION_MISMATCH);
    v1_before = mock_nvml_memory_v1_calls();
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_ERR_DRIVER_MISMATCH);
    CHECK(mock_nvml_memory_v1_calls() == v1_before);

    mock_nvml_reset_failures();
    mock_nvml_set_memory_reserved(0, 0);

    // Verification of proper closure
    unsigned int shutdown_before = mock_nvml_shutdown_calls();

    gpu_metric_context_destroy(gpu0);

    // gpu1 is no longer NULL, so the build will succeed
    CHECK(gpu_metric_context_sample(gpu1, &stats1) == GPU_METRIC_SUCCESS);
    CHECK(mock_nvml_shutdown_calls() == shutdown_before);
    gpu_metric_context_destroy(gpu1);

    printf("All GPUContext tests passed\n");
    return 0;
}
