#define _POSIX_C_SOURCE 200809L

#include "gpu_metric.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <nvml.h>

/*
 * Historical GPU memory state used to calculate
 * the difference between consecutive samples.
 */
typedef struct {
    uint64_t previous_mem_mib;
    int has_previous_sample;
} MemoryCache;

/*
 * Native GPUMetric state.
 *
 * The current implementation intentionally keeps a single
 * process-wide GPU context.
 */
static nvmlDevice_t device;
static MemoryCache cache;
static int initialized = 0;


/*
 * Initialize NVML and bind to the requested NVIDIA GPU.
 */
int gpu_metric_init(unsigned int device_index)
{
    /*
     * Initialization is idempotent for the current context.
     *
     * If the library is already initialized, do not call
     * nvmlInit() a second time.
     */
    if (initialized) {
        return GPU_METRIC_SUCCESS;
    }

    nvmlReturn_t ret = nvmlInit();

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlInit failed: %s\n",
            nvmlErrorString(ret)
        );

        return GPU_METRIC_ERR_NVML;
    }

    /*
     * Query the number of NVIDIA devices.
     */
    unsigned int count = 0;

    ret = nvmlDeviceGetCount(&count);

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetCount failed: %s\n",
            nvmlErrorString(ret)
        );

        /*
         * NVML was successfully initialized, so it must
         * be shut down before returning.
         */
        nvmlShutdown();

        return GPU_METRIC_ERR_NVML;
    }

    /*
     * NVML is available, but there are no NVIDIA GPUs.
     */
    if (count == 0) {
        fprintf(
            stderr,
            "[GPUMETRIC] No compatible NVIDIA GPUs detected\n"
        );

        nvmlShutdown();

        return GPU_METRIC_ERR_NO_DEVICE;
    }

    /*
     * Validate the requested GPU index.
     */
    if (device_index >= count) {
        fprintf(
            stderr,
            "[GPUMETRIC] Invalid GPU index: %u "
            "(available GPUs: %u)\n",
            device_index,
            count
        );

        nvmlShutdown();

        return GPU_METRIC_ERR_ARGUMENT;
    }

    /*
     * Resolve the NVML device handle.
     */
    ret = nvmlDeviceGetHandleByIndex(
        device_index,
        &device
    );

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetHandleByIndex failed: %s\n",
            nvmlErrorString(ret)
        );

        nvmlShutdown();

        return GPU_METRIC_ERR_DEVICE;
    }

    /*
     * Reset sampling state.
     *
     * The first sample after initialization has no previous
     * sample, therefore its memory delta is zero.
     */
    memset(&cache, 0, sizeof(cache));

    initialized = 1;

    return GPU_METRIC_SUCCESS;
}


/*
 * Sample current GPU metrics.
 */
int gpu_metric_sample(GPUStats *out)
{
    if (!initialized) {
        return GPU_METRIC_ERR_NOT_INITIALIZED;
    }

    if (out == NULL) {
        return GPU_METRIC_ERR_ARGUMENT;
    }

    unsigned int temp = 0;

    nvmlUtilization_t util;
    nvmlMemory_t mem;

    /*
     * GPU temperature.
     */
    nvmlReturn_t ret = nvmlDeviceGetTemperature(
        device,
        NVML_TEMPERATURE_GPU,
        &temp
    );

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetTemperature failed: %s\n",
            nvmlErrorString(ret)
        );

        return GPU_METRIC_ERR_DEVICE;
    }

    /*
     * GPU utilization.
     */
    ret = nvmlDeviceGetUtilizationRates(
        device,
        &util
    );

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetUtilizationRates failed: %s\n",
            nvmlErrorString(ret)
        );

        return GPU_METRIC_ERR_DEVICE;
    }

    /*
     * GPU memory information.
     */
    ret = nvmlDeviceGetMemoryInfo(
        device,
        &mem
    );

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetMemoryInfo failed: %s\n",
            nvmlErrorString(ret)
        );

        return GPU_METRIC_ERR_DEVICE;
    }

    /*
     * NVML reports memory in bytes.
     *
     * Convert bytes -> MiB.
     */
    uint64_t current_mib =
        mem.used / (1024ULL * 1024ULL);

    /*
     * Calculate signed memory delta.
     *
     * A negative value is valid when GPU memory usage decreases.
     */
    int64_t delta_mib = 0;

    if (cache.has_previous_sample) {
        delta_mib =
            (int64_t) current_mib -
            (int64_t) cache.previous_mem_mib;
    }

    /*
     * Update historical state only after all NVML calls
     * have succeeded.
     */
    cache.previous_mem_mib = current_mib;
    cache.has_previous_sample = 1;

    /*
     * Populate the public C structure.
     */
    out->temp = temp;
    out->util = util.gpu;
    out->mem_mib = current_mib;
    out->delta_mib = delta_mib;

    return GPU_METRIC_SUCCESS;
}


/*
 * Shut down NVML and release native state.
 */
void gpu_metric_cleanup(void)
{
    /*
     * cleanup() is intentionally idempotent.
     *
     * Calling cleanup() on an already-cleaned-up context
     * is safe.
     */
    if (initialized) {
        nvmlShutdown();
        initialized = 0;
    }

    /*
     * Always reset sampling state.
     *
     * This guarantees that a later init() starts a completely
     * new sampling sequence.
     */
    memset(&cache, 0, sizeof(cache));

    /*
     * Clear the device handle as well.
     *
     * nvmlDevice_t is an opaque NVML handle, so zeroing it
     * is only used here as local state hygiene.
     */
    memset(&device, 0, sizeof(device));
}