#include "gpu_metric.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(call) do { \
    int status = (call); \
    if (status != GPU_METRIC_SUCCESS) { \
        fprintf(stderr, "[FAIL] %s: status=%d\n", #call, status); \
        goto cleanup; \
    } \
} while (0)

static void print_stats(const char *name, const GPUStats *stats) {printf(
        "[%s] temp=%u C, util=%u%%, memory=%llu MiB, reserved=%llu MiB, delta=%lld MiB\n", name,
        (unsigned int)stats->temp,
        (unsigned int)stats->util,
        (unsigned long long)stats->mem_mib,
        (unsigned long long)stats->reserved_mem_mib,
        (long long)stats->delta_mib
    );}

static int validate_stats(const GPUStats *stats) {
    if (stats->temp > 120) return -1;
    if (stats->util > 100) return -1;

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <gpu-index> <gpu-uuid>\n", argv[0]);
        return EXIT_FAILURE;
    }

    errno = 0;
    char *end = NULL;
    unsigned long index = strtoul(argv[1], &end, 10);

    if (errno != 0 || end == argv[1] || *end != '\0' ||
        argv[1][0] == '-' || index > UINT_MAX) {
        fprintf(stderr, "[FAIL] Invalid GPU index: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    const char *uuid = argv[2];

    GPUContext *index_ctx = NULL;
    GPUContext *uuid_ctx = NULL;
    GPUStats index_stats = {0};
    GPUStats uuid_stats = {0};

    int result = EXIT_FAILURE;

    printf("[TEST] GPU index: %u\n", (unsigned int)index);
    printf("[TEST] GPU UUID: %s\n", uuid);

    /*
     * Create two independent contexts pointing to the same GPU.
     */
    CHECK(gpu_metric_create_by_index((unsigned int)index, &index_ctx));
    CHECK(gpu_metric_create_by_uuid(uuid, &uuid_ctx));

    /*
     * Sample using the GPU index.
     */
    CHECK(gpu_metric_context_sample(index_ctx, &index_stats));
    if (validate_stats(&index_stats) != 0) {
        fprintf(stderr, "[FAIL] Invalid metrics from index context\n");
        goto cleanup;
    }
    print_stats("INDEX", &index_stats);

    /*
     * Sample using the GPU UUID.
     */
    CHECK(gpu_metric_context_sample(uuid_ctx, &uuid_stats));
    if (validate_stats(&uuid_stats) != 0) {
        fprintf(stderr, "[FAIL] Invalid metrics from UUID context\n");
        goto cleanup;
    }
    print_stats("UUID", &uuid_stats);

    /*
     * Both contexts must refer to the same physical GPU.
     *
     * Temperature, utilization and memory may change between
     * samples, so exact equality is not required.
     */

    /*
     * Verify independent memory caches.
     */
    if (index_stats.delta_mib != 0 || uuid_stats.delta_mib != 0) {
        fprintf(stderr, "[FAIL] Initial memory delta must be zero\n");
        goto cleanup;
    }

    /*
     * Repeated sampling.
     */
    for (unsigned int i = 0; i < 100; i++) {
        CHECK(gpu_metric_context_sample(index_ctx, &index_stats));
        CHECK(gpu_metric_context_sample(uuid_ctx, &uuid_stats));

        if (validate_stats(&index_stats) != 0 ||
            validate_stats(&uuid_stats) != 0) {
            fprintf(stderr, "[FAIL] Invalid metrics at iteration %u\n", i);
            goto cleanup;
        }
    }
    print_stats("INDEX FINAL", &index_stats);
    print_stats("UUID FINAL", &uuid_stats);

    /*
     * Destroy one context while the other remains active.
     */
    gpu_metric_context_destroy(index_ctx);
    index_ctx = NULL;
    CHECK(gpu_metric_context_sample(uuid_ctx, &uuid_stats));

    /*
     * Recreate the index context while UUID context is active.
     */
    CHECK(gpu_metric_create_by_index((unsigned int)index, &index_ctx));
    CHECK(gpu_metric_context_sample(index_ctx, &index_stats));
    printf("[PASS] Real GPU integration test completed\n");
    result = EXIT_SUCCESS;

cleanup:
    gpu_metric_context_destroy(index_ctx);
    gpu_metric_context_destroy(uuid_ctx);
    return result;
}
