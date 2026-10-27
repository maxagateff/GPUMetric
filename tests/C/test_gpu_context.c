#include "gpu_metric.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    GPUContext *gpu0 = NULL;
    GPUContext *gpu1 = NULL;

    GPUStats stats0 = {0};
    GPUStats stats1 = {0};

    assert(gpu_metric_create_by_index(0, &gpu0) == GPU_METRIC_SUCCESS);
    assert(gpu_metric_create_by_uuid("GPU-MOCK-0001", &gpu1) == GPU_METRIC_SUCCESS);

    assert(gpu0 != NULL);
    assert(gpu1 != NULL);
    assert(gpu0 != gpu1);

    assert(gpu_metric_context_sample(gpu0, &stats0) == GPU_METRIC_SUCCESS);
    assert(gpu_metric_context_sample(gpu1, &stats1) == GPU_METRIC_SUCCESS);

    assert(stats0.temp == 65);
    assert(stats1.temp == 70);

    assert(stats0.util == 80);
    assert(stats1.util == 90);

    assert(stats0.mem_mib == 1024);
    assert(stats1.mem_mib == 2048);

    assert(stats0.delta_mib == 0);
    assert(stats1.delta_mib == 0);

    gpu_metric_context_destroy(gpu0);

    assert(gpu_metric_context_sample(gpu1, &stats1) == GPU_METRIC_SUCCESS);

    gpu_metric_context_destroy(gpu1);
    printf("All GPUContext tests passed\n");
    return 0;
}
