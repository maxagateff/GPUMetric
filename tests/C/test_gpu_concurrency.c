#include "gpu_metric.h"
#include "mock_nvml_control.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREAD_COUNT 8
#define ITERATIONS 500

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); exit(EXIT_FAILURE); } } while (0)

typedef struct {
    unsigned int gpu_index;
    int status;
} ThreadData;

static void *worker(void *argument) {
    ThreadData *data = argument;
    GPUContext *ctx = NULL;
    GPUStats stats = {0};

    data->status = 1;

    for (int i = 0; i < ITERATIONS; i++) {
        if (gpu_metric_create_by_index(data->gpu_index, &ctx) != GPU_METRIC_SUCCESS) return NULL;
        if (gpu_metric_context_sample(ctx, &stats) != GPU_METRIC_SUCCESS) {
            gpu_metric_context_destroy(ctx);
            return NULL;
        }
        gpu_metric_context_destroy(ctx);
        ctx = NULL;
    }
    data->status = 0;
    return NULL;
}

int main(void) {
    pthread_t threads[THREAD_COUNT];
    ThreadData data[THREAD_COUNT];

    unsigned int init_before = mock_nvml_init_calls();
    unsigned int shutdown_before = mock_nvml_shutdown_calls();

    for (int i = 0; i < THREAD_COUNT; i++) {
        data[i].gpu_index = (unsigned int)(i % 2);
        data[i].status = 1;

        CHECK(pthread_create(&threads[i], NULL, worker, &data[i]) == 0);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        CHECK(pthread_join(threads[i], NULL) == 0);
        CHECK(data[i].status == 0);
    }

    unsigned int init_after = mock_nvml_init_calls();
    unsigned int shutdown_after = mock_nvml_shutdown_calls();

    CHECK(init_after > init_before);
    CHECK(init_after - init_before == shutdown_after - shutdown_before);

    printf("Concurrency test passed: %d operations\n", THREAD_COUNT * ITERATIONS);
    printf("NVML init calls: %u\n", init_after - init_before);
    printf("NVML shutdown calls: %u\n", shutdown_after - shutdown_before);

    return 0;
}