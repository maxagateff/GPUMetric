//
// Created by Max Agatev on 23-08-2026.
//

#ifndef GPUMETRIC_NVML_LOADER_H
#define GPUMETRIC_NVML_LOADER_H

#include "nvml_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    void *handle;

    nvmlReturn_t (*Init_v2)(void);
    nvmlReturn_t (*Shutdown)(void);

    nvmlReturn_t (*DeviceGetCount_v2)(unsigned int *deviceCount);
    nvmlReturn_t (*DeviceGetHandleByIndex_v2)(unsigned int index, nvmlDevice_t *device);
    nvmlReturn_t (*DeviceGetMemoryInfo)(nvmlDevice_t device, nvmlMemory_t *memory);
    nvmlReturn_t (*DeviceGetMemoryInfo_v2)(nvmlDevice_t device, void *memory);
    nvmlReturn_t (*DeviceGetUtilizationRates)(nvmlDevice_t device, nvmlUtilization_t *utilization);
    nvmlReturn_t (*DeviceGetTemperature)(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, unsigned int *temp);
    nvmlReturn_t (*SystemGetDriverVersion)(char *version, unsigned int length);
    nvmlReturn_t (*SystemGetNVMLVersion)(char *version, unsigned int length);
    const char *(*ErrorString)(nvmlReturn_t result);
} gpumetric_nvml_t;

int gpumetric_nvml_load(gpumetric_nvml_t *nvml);
void gpumetric_nvml_unload(gpumetric_nvml_t *nvml);
const char *gpumetric_nvml_error(void);

#ifdef __cplusplus
}
#endif

#endif
