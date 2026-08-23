#include "nvml_loader.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static char nvml_error[512];


/*
 * Store a human-readable loader error.
 */
static void set_error(const char *message)
{
    snprintf(
        nvml_error,
        sizeof(nvml_error),
        "%s",
        message
    );
}


/*
 * Return the last NVML loader error.
 */
const char *gpumetric_nvml_error(void)
{
    return nvml_error;
}


/*
 * Helper for resolving symbols with dlsym().
 *
 * dlsym() returns void *, while POSIX permits converting the returned
 * address to a function pointer. ISO C itself does not formally define
 * object-pointer -> function-pointer conversions, hence the usual
 * -Wpedantic warning with direct casts.
 *
 * memcpy avoids the direct cast while preserving the resolved address.
 */
#define LOAD_SYMBOL(handle, target, symbol_name)        \
    do {                                                \
        void *symbol_ptr = dlsym((handle),              \
                                 (symbol_name));         \
        memcpy(&(target),                               \
               &symbol_ptr,                             \
               sizeof(target));                         \
    } while (0)


int gpumetric_nvml_load(gpumetric_nvml_t *nvml)
{
    if (!nvml) {
        set_error("Invalid NVML loader context");
        return -1;
    }

    memset(nvml, 0, sizeof(*nvml));
    memset(nvml_error, 0, sizeof(nvml_error));

    /*
     * NVIDIA driver provides this library.
     *
     * IMPORTANT:
     * GPUMetric does not ship libnvidia-ml.so.
     */
    nvml->handle = dlopen(
        "libnvidia-ml.so.1",
        RTLD_NOW | RTLD_LOCAL
    );

    if (!nvml->handle) {
        const char *err = dlerror();

        snprintf(
            nvml_error,
            sizeof(nvml_error),
            "Failed to load libnvidia-ml.so.1: %s",
            err ? err : "unknown error"
        );

        return -1;
    }

    /*
     * Clear possible stale dlerror() state before resolving symbols.
     */
    dlerror();

    /*
     * Resolve required NVML API dynamically.
     */
    LOAD_SYMBOL(
        nvml->handle,
        nvml->Init_v2,
        "nvmlInit_v2"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->Shutdown,
        "nvmlShutdown"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->DeviceGetCount_v2,
        "nvmlDeviceGetCount_v2"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->DeviceGetHandleByIndex_v2,
        "nvmlDeviceGetHandleByIndex_v2"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->DeviceGetMemoryInfo,
        "nvmlDeviceGetMemoryInfo"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->DeviceGetUtilizationRates,
        "nvmlDeviceGetUtilizationRates"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->DeviceGetTemperature,
        "nvmlDeviceGetTemperature"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->SystemGetDriverVersion,
        "nvmlSystemGetDriverVersion"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->SystemGetNVMLVersion,
        "nvmlSystemGetNVMLVersion"
    );

    LOAD_SYMBOL(
        nvml->handle,
        nvml->ErrorString,
        "nvmlErrorString"
    );

    /*
     * Required API.
     *
     * Every symbol used unconditionally by GPUMetric must be checked here.
     */
    if (!nvml->Init_v2 ||
        !nvml->Shutdown ||
        !nvml->DeviceGetCount_v2 ||
        !nvml->DeviceGetHandleByIndex_v2 ||
        !nvml->DeviceGetMemoryInfo ||
        !nvml->DeviceGetUtilizationRates ||
        !nvml->DeviceGetTemperature ||
        !nvml->ErrorString) {

        const char *err = dlerror();

        if (err) {
            snprintf(
                nvml_error,
                sizeof(nvml_error),
                "Loaded NVML, but required symbols are missing: %s",
                err
            );
        } else {
            set_error(
                "Loaded NVML, but required NVML symbols are missing"
            );
        }

        dlclose(nvml->handle);
        memset(nvml, 0, sizeof(*nvml));

        return -1;
    }

    /*
     * SystemGetDriverVersion and SystemGetNVMLVersion are left optional.
     * If GPUMetric depends on them unconditionally, add them to the check
     * above as required symbols.
     */

    return 0;
}


void gpumetric_nvml_unload(gpumetric_nvml_t *nvml)
{
    if (!nvml) {
        return;
    }

    if (nvml->handle) {
        dlclose(nvml->handle);
    }

    memset(nvml, 0, sizeof(*nvml));
}