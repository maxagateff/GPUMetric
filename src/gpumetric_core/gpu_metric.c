// gpumetric.c
#define _POSIX_C_SOURCE 200809L

#include "gpu_metric.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <nvml.h>


/* Structure to track historical memory usage for delta calculations */
typedef struct {
    uint64_t previous_mem_mib;
    int has_previous_sample;
} MemoryCache;


static nvmlDevice_t device;
static MemoryCache cache;
static int initialized = 0;


/**
 * Initializes the NVML library and binds to the specified GPU.
 *
 * @param device_index NVIDIA GPU index to monitor.
 * @return 0 on success, or a negative error code on failure.
 */
int gpu_metric_init(unsigned int device_index) {
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

    if (device_index >= count)
    {
        fprintf(stderr, "[GPU_METRIC] Invalid GPU index: %u (available GPUs: %u)\n", device_index, count);
        nvmlShutdown();
        return GPU_METRIC_ERR_ARGUMENT;
    }


    // Bind to the requested GPU
    ret = nvmlDeviceGetHandleByIndex(device_index, &device);
    if (ret != NVML_SUCCESS) {
        printf("[GPU_METRIC] Failed to get device handle: %s\n", nvmlErrorString(ret));
        nvmlShutdown();
        return GPU_METRIC_ERR_DEVICE;
    }

    // Reset sampling state and set initialization flag
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
        return GPU_METRIC_ERR_NOT_INITIALIZED;
    }

    if (!out)
    {
        return GPU_METRIC_ERR_ARGUMENT;
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

    // Convert bytes to mebibytes
    uint64_t current_mib = mem.used / (1024ULL * 1024ULL);

    // Calculate memory delta using the previous sample
    int64_t delta_mib = 0;

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
    out->delta_mib = delta_mib;

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

    memset(&cache, 0, sizeof(cache));
}
//TODO Core
//- [ ] Replace `printf()` with `fprintf(stderr, ...)` for library diagnostics.
// - [ ] Preserve and report `nvmlReturn_t` errors from metric sampling.
// - [ ] Use `nvmlErrorString()` for detailed NVML diagnostics.
// - [ ] Fix `GPU_METRIC_ERR_NO_DEVICE` semantics when `nvmlDeviceGetCount()` itself fails.
// - [ ] Clean up NVML state on every initialization failure path.
// - [ ] Review thread safety of the global core state.
// - [ ] Consider replacing global state with an opaque `GPUMetricContext`.
// - [ ] Consider making the logging mechanism configurable. // - [ ] Consider supporting multiple GPU contexts simultaneously.
// - [ ] Add stronger compiler warnings and treat warnings as errors.
// - [ ] Add unit/integration tests for initialization, sampling, cleanup, and error paths.
// - [ ] Test repeated `init -> sample -> cleanup -> init` lifecycle.
// - [ ] Test invalid GPU indices.
// - [ ] Test systems with zero NVIDIA GPUs.
// - [ ] Test negative memory deltas.
//TODO End