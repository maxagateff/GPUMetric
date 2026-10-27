#include "nvml_compat.h"

#include <string.h>

#define MOCK_GPU_COUNT 2

static int initialized = 0;

static struct {
    unsigned int index;
    const char *uuid;
    unsigned int temperature;
    unsigned int utilization;
    unsigned long long memory_used;
} MockDevice;
// ((typeof(devices[0]) *)device)->index
static MockDevice devices[MOCK_GPU_COUNT] = {
    {0, "GPU-MOCK-0000", 65, 80, 1024ULL * 1024 * 1024},
    {1, "GPU-MOCK-0001", 70, 90, 2048ULL * 1024 * 1024}
};

nvmlReturn_t nvmlInit_v2(void) {
    initialized = 1;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlShutdown(void) {
    initialized = 0;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetCount_v2(unsigned int *count) {
    if (!initialized || !count) return NVML_ERROR_NOT_SUPPORTED;
    *count = MOCK_GPU_COUNT;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetHandleByIndex_v2(unsigned int index, nvmlDevice_t *device) {
    if (!initialized || !device || index >= MOCK_GPU_COUNT) return NVML_ERROR_NOT_SUPPORTED;
    *device = (nvmlDevice_t)&devices[index];
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetHandleByUUID(const char *uuid, nvmlDevice_t *device) {
    if (!initialized || !uuid || !device) return NVML_ERROR_NOT_SUPPORTED;
    for (unsigned int i = 0; i < MOCK_GPU_COUNT; i++) {
        if (strcmp(uuid, devices[i].uuid) == 0) {
            *device = (nvmlDevice_t)&devices[i];
            return NVML_SUCCESS;
        }
    }
    return NVML_ERROR_NOT_SUPPORTED;
}

nvmlReturn_t nvmlDeviceGetMemoryInfo(nvmlDevice_t device, nvmlMemory_t *memory) {
    if (!initialized || !device || !memory) return NVML_ERROR_NOT_SUPPORTED;
    unsigned int index = ((MockDevice *)device)->index
    memory->total = 8ULL * 1024 * 1024 * 1024;
    memory->used = devices[index].memory_used;
    memory->free = memory->total - memory->used;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetUtilizationRates(nvmlDevice_t device, nvmlUtilization_t *utilization) {
    if (!initialized || !device || !utilization) return NVML_ERROR_NOT_SUPPORTED;
    unsigned int index = ((MockDevice *)device)->index
    utilization->gpu = devices[index].utilization;
    utilization->memory = 50;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetTemperature(nvmlDevice_t device, nvmlTemperatureSensors_t sensor, unsigned int *temperature) {
    (void)sensor;
    if (!initialized || !device || !temperature) return NVML_ERROR_NOT_SUPPORTED;
    unsigned int index = ((MockDevice *)device)->index
    *temperature = devices[index].temperature;
    return NVML_SUCCESS;
}

const char *nvmlErrorString(nvmlReturn_t result) {return result == NVML_SUCCESS ? "Success" : "Mock NVML error";}