#define _POSIX_C_SOURCE 200809L

#include "gpu_metric.h"
#include "nvml_loader.h"

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
static gpumetric_nvml_t nvml;
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
     */
    if (initialized) {
        return GPU_METRIC_SUCCESS;
    }

    /*
     * Dynamically load libnvidia-ml.so.1 and resolve
     * the required NVML symbols.
     */
    if (gpumetric_nvml_load(&nvml) != 0) {
        fprintf(
            stderr,
            "[GPUMETRIC] Failed to load NVML: %s\n",
            gpumetric_nvml_error()
        );

        return GPU_METRIC_ERR_NVML;
    }

    /*
     * Initialize NVML.
     */
    nvmlReturn_t ret = nvml.Init_v2();

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlInit_v2 failed: %s\n",
            nvml.ErrorString(ret)
        );

        /*
         * NVML initialization failed, but the shared library
         * was successfully opened by dlopen().
         */
        gpumetric_nvml_unload(&nvml);

        return GPU_METRIC_ERR_NVML;
    }

    /*
     * Query the number of NVIDIA devices.
     */
    unsigned int count = 0;

    ret = nvml.DeviceGetCount_v2(&count);

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetCount_v2 failed: %s\n",
            nvml.ErrorString(ret)
        );

        nvml.Shutdown();
        gpumetric_nvml_unload(&nvml);

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

        nvml.Shutdown();
        gpumetric_nvml_unload(&nvml);

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

        nvml.Shutdown();
        gpumetric_nvml_unload(&nvml);

        return GPU_METRIC_ERR_ARGUMENT;
    }

    /*
     * Resolve the NVML device handle.
     */
    ret = nvml.DeviceGetHandleByIndex_v2(
        device_index,
        &device
    );

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetHandleByIndex_v2 failed: %s\n",
            nvml.ErrorString(ret)
        );

        nvml.Shutdown();
        gpumetric_nvml_unload(&nvml);

        memset(&device, 0, sizeof(device));

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

    /*
     * Do not partially modify the output structure if
     * one of the NVML calls fails.
     */
    unsigned int temp = 0;

    nvmlUtilization_t util;
    nvmlMemory_t mem;

    memset(&util, 0, sizeof(util));
    memset(&mem, 0, sizeof(mem));

    nvmlReturn_t ret;

    /*
     * GPU temperature.
     */
    ret = nvml.DeviceGetTemperature(
        device,
        NVML_TEMPERATURE_GPU,
        &temp
    );

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetTemperature failed: %s\n",
            nvml.ErrorString(ret)
        );

        return GPU_METRIC_ERR_DEVICE;
    }

    /*
     * GPU utilization.
     */
    ret = nvml.DeviceGetUtilizationRates(
        device,
        &util
    );

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetUtilizationRates failed: %s\n",
            nvml.ErrorString(ret)
        );

        return GPU_METRIC_ERR_DEVICE;
    }

    /*
     * GPU memory information.
     */
    ret = nvml.DeviceGetMemoryInfo(
        device,
        &mem
    );

    if (ret != NVML_SUCCESS) {
        fprintf(
            stderr,
            "[GPUMETRIC] nvmlDeviceGetMemoryInfo failed: %s\n",
            nvml.ErrorString(ret)
        );

        return GPU_METRIC_ERR_DEVICE;
    }

    /*
     * NVML reports memory in bytes.
     *
     * Convert bytes -> MiB.
     */
    const uint64_t current_mib =
        (uint64_t) mem.used / (1024ULL * 1024ULL);

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
     * Populate the public C structure only after the complete
     * sample has been collected successfully.
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
     */
    if (initialized) {
        /*
         * IMPORTANT:
         *
         * Do NOT call nvmlShutdown() directly here.
         *
         * GPUMetric uses runtime NVML loading through dlopen/dlsym,
         * therefore the shutdown function must also be invoked
         * through the resolved function pointer.
         */
        if (nvml.Shutdown) {
            nvmlReturn_t ret = nvml.Shutdown();

            if (ret != NVML_SUCCESS && nvml.ErrorString) {
                fprintf(
                    stderr,
                    "[GPUMETRIC] nvmlShutdown failed: %s\n",
                    nvml.ErrorString(ret)
                );
            }
        }

        initialized = 0;
    }

    /*
     * Close libnvidia-ml.so.1 and clear all resolved
     * function pointers.
     */
    gpumetric_nvml_unload(&nvml);

    /*
     * Reset sampling state.
     *
     * This guarantees that a later init() starts a completely
     * new sampling sequence.
     */
    memset(&cache, 0, sizeof(cache));

    /*
     * Clear the opaque device handle.
     */
    memset(&device, 0, sizeof(device));
}