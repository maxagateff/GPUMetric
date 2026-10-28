#include "nvml_compat.h"
#include "mock_nvml_control.h"

#include <string.h>

#define MOCK_GPU_COUNT 2

static int initialized = 0;

static nvmlReturn_t init_failure = NVML_SUCCESS;
static nvmlReturn_t temperature_failure = NVML_SUCCESS;
static nvmlReturn_t utilization_failure = NVML_SUCCESS;
static nvmlReturn_t memory_failure = NVML_SUCCESS;

static nvmlReturn_t memory_v2_failure = NVML_SUCCESS;
static unsigned int memory_v1_calls = 0;
static unsigned int memory_v2_calls = 0;

typedef struct {
    unsigned int version;
    unsigned long long total;
    unsigned long long reserved;
    unsigned long long free;
    unsigned long long used;
} MockMemoryV2;

#define MOCK_MEMORY_V2_VERSION ((unsigned int)sizeof(MockMemoryV2) | (2U << 24U))

static unsigned int init_calls = 0;
static unsigned int shutdown_calls = 0;

void mock_nvml_set_init_failure(nvmlReturn_t value) {init_failure = value;}
void mock_nvml_set_temperature_failure(nvmlReturn_t value) {temperature_failure = value;}
void mock_nvml_set_utilization_failure(nvmlReturn_t value) {utilization_failure = value;}
void mock_nvml_set_memory_failure(nvmlReturn_t value) {memory_failure = value;}

void mock_nvml_set_memory_v2_failure(nvmlReturn_t value) {memory_v2_failure = value;}
unsigned int mock_nvml_memory_v1_calls(void) {return memory_v1_calls;}
unsigned int mock_nvml_memory_v2_calls(void) {return memory_v2_calls;}

unsigned int mock_nvml_init_calls(void) {return init_calls;}
unsigned int mock_nvml_shutdown_calls(void) {return shutdown_calls;}

void mock_nvml_reset_failures(void) {
    init_failure = NVML_SUCCESS;
    temperature_failure = NVML_SUCCESS;
    utilization_failure = NVML_SUCCESS;
    memory_failure = NVML_SUCCESS;
    memory_v2_failure = NVML_SUCCESS;
}

typedef struct {
    unsigned int index;
    const char *uuid;
    unsigned int temperature;
    unsigned int utilization;
    unsigned long long memory_used;
    unsigned long long memory_reserved;
} MockDevice;

//
static MockDevice devices[MOCK_GPU_COUNT] = {
    {0, "GPU-MOCK-0000", 65, 80, 1024ULL * 1024 * 1024, 0},
    {1, "GPU-MOCK-0001", 70, 90, 2048ULL * 1024 * 1024, 0}
};

void mock_nvml_set_memory_reserved(unsigned int index, unsigned long long bytes) {if (index < MOCK_GPU_COUNT) devices[index].memory_reserved = bytes;}

nvmlReturn_t nvmlInit_v2(void) {
    init_calls++;
    if (init_failure != NVML_SUCCESS) return init_failure;

    initialized = 1;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlShutdown(void) {
    shutdown_calls++;
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
    memory_v1_calls++;
    if (memory_failure != NVML_SUCCESS) return memory_failure;
    unsigned int index = ((MockDevice *)device)->index;
    memory->total = 8ULL * 1024 * 1024 * 1024;
    memory->used = devices[index].memory_used;
    memory->free = memory->total - memory->used;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetMemoryInfo_v2(nvmlDevice_t device, MockMemoryV2 *memory) {
    if (!initialized || !device || !memory) return NVML_ERROR_NOT_SUPPORTED;
    memory_v2_calls++;
    if (memory_v2_failure != NVML_SUCCESS) return memory_v2_failure;
    if (memory->version != MOCK_MEMORY_V2_VERSION) return NVML_ERROR_NOT_SUPPORTED;
    unsigned int index = ((MockDevice *)device)->index;
    memory->total = 8ULL * 1024 * 1024 * 1024;
    memory->used = devices[index].memory_used;
    memory->reserved = devices[index].memory_reserved;
    memory->free = memory->total - memory->used;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetUtilizationRates(nvmlDevice_t device, nvmlUtilization_t *utilization) {
    if (!initialized || !device || !utilization) return NVML_ERROR_NOT_SUPPORTED;
    if (utilization_failure != NVML_SUCCESS) return utilization_failure;
    unsigned int index = ((MockDevice *)device)->index;
    utilization->gpu = devices[index].utilization;
    utilization->memory = 50;
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetTemperature(nvmlDevice_t device, nvmlTemperatureSensors_t sensor, unsigned int *temperature) {
    (void)sensor;
    if (!initialized || !device || !temperature) return NVML_ERROR_NOT_SUPPORTED;
    if (temperature_failure != NVML_SUCCESS) return temperature_failure;
    unsigned int index = ((MockDevice *)device)->index;
    *temperature = devices[index].temperature;
    return NVML_SUCCESS;
}

const char *nvmlErrorString(nvmlReturn_t result) {return result == NVML_SUCCESS ? "Success" : "Mock NVML error";}

void mock_nvml_set_memory_used(unsigned int index, unsigned long long bytes) {if (index < MOCK_GPU_COUNT) devices[index].memory_used = bytes;}
