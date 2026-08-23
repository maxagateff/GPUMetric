#include "nvml_loader.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static char nvml_error[512];

static void set_error(const char *message) {snprintf(nvml_error, sizeof(nvml_error), "%s", message);}

const char *gpumetric_nvml_error(void) {return nvml_error;}

int gpumetric_nvml_load(gpumetric_nvml_t *nvml)
{
    memset(nvml, 0, sizeof(*nvml));
    /*
     * NVIDIA driver provides this library.
     *
     * IMPORTANT:
     * We do NOT ship libnvidia-ml.so with gpumetric.
     */
    nvml->handle = dlopen("libnvidia-ml.so.1", RTLD_NOW | RTLD_LOCAL);

    if (!nvml->handle)
    {
        const char *err = dlerror();
        snprintf(nvml_error, sizeof(nvml_error), "Failed to load libnvidia-ml.so.1: %s", err ? err: "unknown error");
        return -1;
    }
    /*
     * Resolve functions dynamically.
     */
    nvml->Init_v2 = (nvmlReturn_t (*)(void)) dlsym(nvml->handle, "nvmlInit_v2");
    nvml->Shutdown =
        (nvmlReturn_t (*)(void))
        dlsym(nvml->handle, "nvmlShutdown");

    nvml->DeviceGetCount_v2 =
        (nvmlReturn_t (*)(unsigned int *))
        dlsym(
            nvml->handle,
            "nvmlDeviceGetCount_v2"
        );

    nvml->DeviceGetHandleByIndex_v2 =
        (nvmlReturn_t (*)(unsigned int, nvmlDevice_t *))
        dlsym(
            nvml->handle,
            "nvmlDeviceGetHandleByIndex_v2"
        );

    nvml->DeviceGetMemoryInfo =
        (nvmlReturn_t (*)(nvmlDevice_t, nvmlMemory_t *))
        dlsym(
            nvml->handle,
            "nvmlDeviceGetMemoryInfo"
        );

    nvml->DeviceGetUtilizationRates =
        (nvmlReturn_t (*)(nvmlDevice_t, nvmlUtilization_t *))
        dlsym(
            nvml->handle,
            "nvmlDeviceGetUtilizationRates"
        );

    nvml->DeviceGetTemperature =
        (nvmlReturn_t (*)(nvmlDevice_t,
                          nvmlTemperatureSensors_t,
                          unsigned int *))
        dlsym(
            nvml->handle,
            "nvmlDeviceGetTemperature"
        );

    nvml->SystemGetDriverVersion =
        (nvmlReturn_t (*)(char *, unsigned int))
        dlsym(
            nvml->handle,
            "nvmlSystemGetDriverVersion"
        );

    nvml->SystemGetNVMLVersion =
        (nvmlReturn_t (*)(char *, unsigned int))
        dlsym(
            nvml->handle,
            "nvmlSystemGetNVMLVersion"
        );
    nvml->ErrorString =
        (const char *(*)(nvmlReturn_t))
        dlsym(
            nvml->handle,
            "nvmlErrorString"
        );


    /*
     * Required API.
     */

    if (!nvml->Init_v2 ||
        !nvml->Shutdown ||
        !nvml->DeviceGetCount_v2 ||
        !nvml->DeviceGetHandleByIndex_v2 ||
        !nvml->DeviceGetMemoryInfo) {

        set_error(
            "Loaded NVML, but required NVML symbols are missing"
        );

        dlclose(nvml->handle);
        memset(nvml, 0, sizeof(*nvml));

        return -1;
    }

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
}