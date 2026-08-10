// gpumetric.c
#define _POSIX_C_SOURCE 200809L

#include "gpu_metric.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <nvml.h>

#define CACHE_SIZE 16

/* Structure to track historical memory usage for delta calculations */
typedef struct {
    uint64_t previous_mem_mib;
    int has_previous_sample;
} MemoryCache;

static nvmlDevice_t device;
static MemoryCache cache;
static int initialized = 0;

/**
 * Initializes the NVML library and binds to the first available GPU.
 * Returns 0 on success, or a negative error code on failure.
 */
int gpu_metric_init(void) {
    // Return early if the library has already been initialized
    if (initialized) {
        return GPU_METRIC_SUCCESS;
    }

    nvmlReturn_t ret = nvmlInit();
    if (ret != NVML_SUCCESS) {
        printf("[GPU_METRIC] nvmlInit failed: %s\n", nvmlErrorString(ret));
        return GPU_METRIC_ERR_NVML;
    }

    unsigned int count = 0;
    ret = nvmlDeviceGetCount(&count);
    if (ret != NVML_SUCCESS) {
        printf("[GPU_METRIC] Failed to get device count: %s\n", nvmlErrorString(ret));
        nvmlShutdown();
        return GPU_METRIC_ERR_NO_DEVICE;
    }

    if (count == 0) {
        printf("[GPU_METRIC] No GPUs found on the system\n");
        nvmlShutdown();
        return GPU_METRIC_ERR_NO_DEVICE;
    }

    // Bind to the first GPU (index 0)
    ret = nvmlDeviceGetHandleByIndex(0, &device);
    if (ret != NVML_SUCCESS) {
        printf("[GPU_METRIC] Failed to get device handle: %s\n", nvmlErrorString(ret));
        nvmlShutdown();
        return GPU_METRIC_ERR_DEVICE;
    }

    // Reset ring buffer cache and set initialization flag
    memset(&cache, 0, sizeof(cache));
    initialized = 1;

    return GPU_METRIC_SUCCESS;
}

/**
 * Samples current GPU metrics and calculates memory consumption delta.
 * Returns 0 on success, or a negative error code on failure.
 */
int gpu_metric_sample(GPUStats* out) {
    if (!initialized) {
        return GPU_METRIC_ERR_ARGUMENT;
    }

    if (!out)
    {
        return GPU_METRIC_ERR_NOT_INITIALIZED;
    }

    unsigned int temp = 0;
    nvmlUtilization_t util;
    nvmlMemory_t mem;

    // Fetch hardware metrics via NVML
    if (nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp) != NVML_SUCCESS)
        return GPU_METRIC_ERR_DEVICE;
    if (nvmlDeviceGetUtilizationRates(device, &util) != NVML_SUCCESS)
        return GPU_METRIC_ERR_DEVICE;
    if (nvmlDeviceGetMemoryInfo(device, &mem) != NVML_SUCCESS)
        return GPU_METRIC_ERR_DEVICE;

    // Convert bytes to megabytes
    uint64_t current_mib = mem.used / (1024ULL * 1024ULL);
    int64_t delta = 0;

    // Calculate memory delta using the previous sample from the ring buffer
    uint64_t delta_mib = 0;

    if (cache.has_previous_sample)
    {
        delta_mib = (int64_t)current_mib - (int64_t)cache.previous_mem_mib;
    }

    cache.previous_mem_mib = current_mib;
    cache.has_previous_sample = 1;


    // Populate the output structure
    out->temp = temp;
    out->util = util.gpu;
    out->mem_mib = current_mib;
    out->delta_mib = delta;

    return GPU_METRIC_SUCCESS;
}

/**
 * Shuts down NVML and releases resources.
 */
void gpu_metric_cleanup(void) {
    if (initialized) {
        nvmlShutdown();
        initialized = 0;
    }

    memset(&cache, 0. sizeof(cache));
}
