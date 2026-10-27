#define _POSIX_C_SOURCE 200809L

#include "gpu_metric.h"
#include "nvml_runtime.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* NVML_ERROR_NOT_SUPPORTED
 * Historical GPU memory state.
 */
typedef struct {
    uint64_t previous_mem_mib;
    int has_previous_sample;
} MemoryCache;

/*
 * NVML memory information v2 ABI.
 */
typedef struct {
    unsigned int version;
    unsigned long long total;
    unsigned long long reserved;
    unsigned long long free;
    unsigned long long used;
} GPUMetricMemoryV2;

#define GPUMETRIC_MEMORY_V2_VERSION ((unsigned int)sizeof(GPUMetricMemoryV2) | (2U << 24U))

/*
 * Independent GPU context.
 *
 * Each context owns:
 * - one NVML runtime reference;
 * - one GPU device handle;
 * - one memory cache;
 * - one sampling mutex.
 */
struct GPUContext {
    const gpumetric_nvml_t *nvml;
    nvmlDevice_t device;
    MemoryCache cache;
    pthread_mutex_t mutex;
};

/*
 * Legacy API state.
 *
 * Retained for backward compatibility.
 */
static pthread_mutex_t legacy_mutex = PTHREAD_MUTEX_INITIALIZER;
static GPUContext *legacy_context = NULL;


/*
 * Convert NVML errors to GPUMetric error codes.
 */
static int gpu_metric_nvml_error(nvmlReturn_t ret) {
    if (ret == NVML_ERROR_LIB_RM_VERSION_MISMATCH) {return GPU_METRIC_ERR_DRIVER_MISMATCH;}
    return GPU_METRIC_ERR_DEVICE;
}


/*
 * Allocate and initialize a GPU context.
 *
 * The caller must already own an NVML runtime reference.
 */
static int gpu_metric_context_allocate(const gpumetric_nvml_t *nvml, nvmlDevice_t device, GPUContext **out) {
    if (!nvml || !device || !out) {return GPU_METRIC_ERR_ARGUMENT;}

    *out = NULL;

    GPUContext *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {return GPU_METRIC_ERR_NVML;}

    ctx->nvml = nvml;
    ctx->device = device;

    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        free(ctx);
        return GPU_METRIC_ERR_NVML;
    }
    *out = ctx;

    return GPU_METRIC_SUCCESS;
}


/*
 * Create a GPU context using its NVML index.
 */
int gpu_metric_create_by_index(unsigned int device_index, GPUContext **out) {
    if (!out) return GPU_METRIC_ERR_ARGUMENT;

    *out = NULL;

    const gpumetric_nvml_t *nvml = NULL;

    int status = gpumetric_runtime_acquire(&nvml);
    if (status != GPU_METRIC_SUCCESS) {return status;}

    unsigned int count = 0;

    nvmlReturn_t ret = nvml->DeviceGetCount_v2(&count);

    if (ret != NVML_SUCCESS) {
        status = gpu_metric_nvml_error(ret);
        goto error;
    }

    if (count == 0) {
        status = GPU_METRIC_ERR_NO_DEVICE;
        goto error;
    }

    if (device_index >= count) {
        status = GPU_METRIC_ERR_ARGUMENT;
        goto error;
    }

    nvmlDevice_t device = NULL;

    ret = nvml->DeviceGetHandleByIndex_v2(device_index, &device);
    if (ret != NVML_SUCCESS) {
        status = gpu_metric_nvml_error(ret);
        goto error;
    }

    status = gpu_metric_context_allocate(nvml, device, out);
    if (status != GPU_METRIC_SUCCESS) goto error;

    return GPU_METRIC_SUCCESS;

error: gpumetric_runtime_release();
    return status;
}


/*
 * Create a GPU context using its UUID.
 */
int gpu_metric_create_by_uuid(const char *uuid, GPUContext **out) {
    if (!out) return GPU_METRIC_ERR_ARGUMENT;

    *out = NULL;
    if (!uuid || !uuid[0]) {return GPU_METRIC_ERR_ARGUMENT;}

    const gpumetric_nvml_t *nvml = NULL;

    int status = gpumetric_runtime_acquire(&nvml);
    if (status != GPU_METRIC_SUCCESS) {return status;}

    nvmlDevice_t device = NULL;
    nvmlReturn_t ret = nvml->DeviceGetHandleByUUID(uuid, &device);

    if (ret != NVML_SUCCESS) {
        status = gpu_metric_nvml_error(ret);
        goto error;
    }
    status = gpu_metric_context_allocate(nvml, device, out);
    if (status != GPU_METRIC_SUCCESS) goto error;
    return GPU_METRIC_SUCCESS;

error: gpumetric_runtime_release();
    return status;
}


/*
 * Collect GPU metrics.
 *
 * This function must be called while holding ctx->mutex.
 */
static int gpu_metric_collect(GPUContext *ctx, GPUStats *out) {
    const gpumetric_nvml_t *nvml = ctx->nvml;
    nvmlDevice_t device = ctx->device;

    unsigned int temp = 0;

    nvmlUtilization_t util = {0};
    nvmlMemory_t legacy_mem = {0};
    GPUMetricMemoryV2 memory_v2 = {0};

    nvmlReturn_t ret;

    /*
     * GPU temperature.
     */
    ret = nvml->DeviceGetTemperature(device,NVML_TEMPERATURE_GPU, &temp);
    if (ret != NVML_SUCCESS) {return gpu_metric_nvml_error(ret);}

    /*
     * GPU utilization.
     */
    ret = nvml->DeviceGetUtilizationRates(device, &util);
    if (ret != NVML_SUCCESS) {return gpu_metric_nvml_error(ret);}

    /*
     * GPU memory information.
     */
    uint64_t used_bytes = 0;
    uint64_t reserved_bytes = 0;

    if (nvml->DeviceGetMemoryInfo_v2) {
        memory_v2.version = GPUMETRIC_MEMORY_V2_VERSION;
        ret = nvml->DeviceGetMemoryInfo_v2(device, &memory_v2);

        if (ret == NVML_SUCCESS) {
            used_bytes = (uint64_t)memory_v2.used;
            reserved_bytes = (uint64_t)memory_v2.reserved;
        }} else {ret = NVML_ERROR_NOT_SUPPORTED;}

    /*
     * Fallback to legacy NVML memory API.
     */
    if (ret == NVML_ERROR_NOT_SUPPORTED) {
        if (!nvml->DeviceGetMemoryInfo) {return GPU_METRIC_ERR_NVML;}
        ret = nvml->DeviceGetMemoryInfo(device, &legacy_mem);
        if (ret == NVML_SUCCESS) {used_bytes = (uint64_t)legacy_mem.used;}
    }

    if (ret != NVML_SUCCESS) {return gpu_metric_nvml_error(ret);}

    /*
     * Convert bytes to MiB.
     */
    const uint64_t current_mib = used_bytes / (1024ULL * 1024ULL);
    const uint64_t reserved_mib = reserved_bytes / (1024ULL * 1024ULL);

    /*
     * Calculate memory delta.
     */
    int64_t delta_mib = 0;
    if (ctx->cache.has_previous_sample) {delta_mib = (int64_t)current_mib - (int64_t)ctx->cache.previous_mem_mib;}

    /*
     * Prepare a complete sample before committing it.
     */
    GPUStats sample = {
        .temp = temp,
        .util = util.gpu,
        .mem_mib = current_mib,
        .delta_mib = delta_mib,
        .reserved_mem_mib = reserved_mib
    };

    /*
     * Update cache only after successful collection.
     */
    ctx->cache.previous_mem_mib = current_mib;
    ctx->cache.has_previous_sample = 1;

    *out = sample;

    return GPU_METRIC_SUCCESS;
}


/*
 * Sample metrics from a specific GPU context.
 */
int gpu_metric_context_sample(GPUContext *ctx, GPUStats *out) {
    if (!ctx || !out) {return GPU_METRIC_ERR_ARGUMENT;}
    if (pthread_mutex_lock(&ctx->mutex) != 0) {return GPU_METRIC_ERR_NVML;}

    int status = gpu_metric_collect(ctx, out);
    pthread_mutex_unlock(&ctx->mutex);
    return status;
}


/*
 * Destroy a GPU context.
 *
 * The caller must ensure that no other thread
 * is using this context.
 */
void gpu_metric_context_destroy(GPUContext *ctx) {
    if (!ctx) return;
    pthread_mutex_destroy(&ctx->mutex);
    gpumetric_runtime_release();
    free(ctx);
}


/*
 * Legacy API.
 *
 * Preserves the original process-wide interface.
 */
int gpu_metric_init(unsigned int device_index) {
    if (pthread_mutex_lock(&legacy_mutex) != 0) {return GPU_METRIC_ERR_NVML;}

    /*
     * Preserve historical idempotent initialization.
     */
    if (legacy_context) {
        pthread_mutex_unlock(&legacy_mutex);
        return GPU_METRIC_SUCCESS;
    }

    GPUContext *ctx = NULL;

    int status = gpu_metric_create_by_index(device_index, &ctx);
    if (status == GPU_METRIC_SUCCESS) {legacy_context = ctx;}

    pthread_mutex_unlock(&legacy_mutex);

    return status;
}


/*
 * Legacy sampling API.
 */
int gpu_metric_sample(GPUStats *out) {
    if (pthread_mutex_lock(&legacy_mutex) != 0) {return GPU_METRIC_ERR_NVML;}

    if (!legacy_context) {
        pthread_mutex_unlock(&legacy_mutex);
        return GPU_METRIC_ERR_NOT_INITIALIZED;
    }

    int status = gpu_metric_context_sample(legacy_context, out);
    pthread_mutex_unlock(&legacy_mutex);
    return status;
}


/*
 * Legacy cleanup API.
 */
void gpu_metric_cleanup(void)
{
    if (pthread_mutex_lock(&legacy_mutex) != 0) return;
    if (legacy_context) {
        gpu_metric_context_destroy(legacy_context);
        legacy_context = NULL;
    }
    pthread_mutex_unlock(&legacy_mutex);
}
