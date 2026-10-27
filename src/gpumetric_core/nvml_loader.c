#include "nvml_loader.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

static _Thread_local char nvml_error[512];

static void set_error(const char *message) {snprintf(nvml_error, sizeof(nvml_error), "%s", message);}
const char *gpumetric_nvml_error(void) {return nvml_error;}

/*
 * POSIX dlsym() returns a symbol address.
 * memcpy avoids direct object-to-function pointer casts.
 */
#define LOAD_SYMBOL(handle, target, name) do {              \
    void *symbol_address = dlsym((handle), (name));          \
    memcpy(&(target), &symbol_address, sizeof(target));      \
} while (0)

int gpumetric_nvml_load_from_path(gpumetric_nvml_t *nvml, const char *library_path) {
    if (!nvml) {
        set_error("Invalid NVML loader context");
        return -1;
    }

    /*
     * The caller must unload an existing context before reusing it.
     */
    if (!library_path || !library_path[0]) {
        set_error("Invalid NVML library path");
        return -1;
    }

    memset(nvml, 0, sizeof(*nvml));
    nvml_error[0] = '\0';

    nvml->handle = dlopen(library_path, RTLD_NOW | RTLD_LOCAL);

    if (!nvml->handle) {
        const char *err = dlerror();
        snprintf(nvml_error, sizeof(nvml_error), "Failed to load NVML: %s", err ? err : "unknown error");
        return -1;
    }

    LOAD_SYMBOL(nvml->handle, nvml->Init_v2, "nvmlInit_v2");
    LOAD_SYMBOL(nvml->handle, nvml->Shutdown, "nvmlShutdown");
    LOAD_SYMBOL(nvml->handle, nvml->DeviceGetCount_v2, "nvmlDeviceGetCount_v2");
    LOAD_SYMBOL(nvml->handle, nvml->DeviceGetHandleByIndex_v2, "nvmlDeviceGetHandleByIndex_v2");
    LOAD_SYMBOL(nvml->handle, nvml->DeviceGetHandleByUUID, "nvmlDeviceGetHandleByUUID");
    LOAD_SYMBOL(nvml->handle, nvml->DeviceGetMemoryInfo, "nvmlDeviceGetMemoryInfo");
    LOAD_SYMBOL(nvml->handle, nvml->DeviceGetMemoryInfo_v2, "nvmlDeviceGetMemoryInfo_v2");
    LOAD_SYMBOL(nvml->handle, nvml->DeviceGetUtilizationRates, "nvmlDeviceGetUtilizationRates");
    LOAD_SYMBOL(nvml->handle, nvml->DeviceGetTemperature, "nvmlDeviceGetTemperature");
    LOAD_SYMBOL(nvml->handle, nvml->SystemGetDriverVersion, "nvmlSystemGetDriverVersion");
    LOAD_SYMBOL(nvml->handle, nvml->SystemGetNVMLVersion, "nvmlSystemGetNVMLVersion");
    LOAD_SYMBOL(nvml->handle, nvml->ErrorString, "nvmlErrorString");

    if (!nvml->Init_v2 ||
        !nvml->Shutdown ||
        !nvml->DeviceGetCount_v2 ||
        !nvml->DeviceGetHandleByIndex_v2 ||
        !nvml->DeviceGetHandleByUUID ||
        (!nvml->DeviceGetMemoryInfo && !nvml->DeviceGetMemoryInfo_v2) ||
        !nvml->DeviceGetUtilizationRates ||
        !nvml->DeviceGetTemperature ||
        !nvml->ErrorString) {

        set_error("Loaded NVML, but required symbols are missing");
        dlclose(nvml->handle);
        memset(nvml, 0, sizeof(*nvml));
        return -1;
    }
    return 0;
}

int gpumetric_nvml_load(gpumetric_nvml_t *nvml) {return gpumetric_nvml_load_from_path(nvml,"libnvidia-ml.so.1");}

void gpumetric_nvml_unload(gpumetric_nvml_t *nvml)
{
    if (!nvml) return;
    if (nvml->handle) dlclose(nvml->handle);
    memset(nvml, 0, sizeof(*nvml));
}