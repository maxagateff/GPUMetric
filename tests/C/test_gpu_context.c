#include "gpu_metric.h"
#include "mock_nvml_control.h"

#include <pthread.h>
#include <assert.h>
#include <stdio.h>

#include "../../src/gpumetric_core/gpu_metric.h"


static void *sample_thread(void *argument) {
    GPUContext *ctx = argument;
    GPUStats stats = {0};
    for (int i = 0; i < 500; i++){if (gpu_metric_context_sample(ctx, &stats) != GPU_METRIC_SUCCESS) return (void *)1;}
    return NULL;
}

int main(void) {
    GPUContext *gpu0 = NULL;
    GPUContext *gpu1 = NULL;

    GPUStats stats0 = {0};
    GPUStats stats1 = {0};

    ////////////////////////
    mock_nvml_set_memory_used(0, 1536ULL * 1024 * 1024);
    mock_nvml_set_temperature_failure(NVML_ERROR_NOT_SUPPORTED);

    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_ERR_DEVICE);
    CHECK(stats0.mem_mib == 1024);

    mock_nvml_reset_failures();
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    CHECK(stats0.mem_mib == 1536);
    CHECK(stats0.delta_mib == 512);

    //////////////////////////////
    mock_nvml_set_memory_used(0, 512ULL * 1024 * 1024);
    mock_nvml_set_memory_failure(NVML_ERROR_NOT_SUPPORTED);
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_ERR_DEVICE);
    CHECK(stats0.mem_mib == 1536);

    mock_nvml_reset_failures();
    CHECK(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    CHECK(stats0.mem_mib == 512);
    CHECK(stats0.delta_mib == -1024);

    pthread_t threads[4];

    for (int i = 0; i < 4; i++) {CHECK(pthread_create(&threads[i], NULL, sample_thread, gpu0) == 0);}
    for (int i = 0; i < 4; i++) {
        void *result = NULL;
        CHECK(pthread_join(threads[i], &result) == 0);
        CHECK(result == NULL);
    }

    unsigned int shutdown_before = mock_nvml_shutdown_calls();

    gpu_metric_context_destroy(gpu0);

    assert(gpu_metric_context_sample(gpu1, &stats1) == GPU_METRIC_SUCCESS);
    CHECK(mock_nvml_shutdown_calls() == shutdown_before);
    gpu_metric_context_destroy(gpu1);
    printf("All GPUContext tests passed\n");
    return 0;
}
