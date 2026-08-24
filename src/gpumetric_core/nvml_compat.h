#ifndef GPUMETRIC_NVML_COMPAT_H
#define GPUMETRIC_NVML_COMPAT_H

/*
 * Minimal NVML ABI declarations used by GPUMetric.
 *
 * GPUMetric resolves NVML symbols at runtime and does not link against the
 * NVIDIA library. Keeping only the stable ABI types required by this project
 * avoids making wheel builds depend on a distribution-specific nvml.h.
 */

typedef struct nvmlDevice_st *nvmlDevice_t;

typedef enum {
    NVML_SUCCESS = 0,
    NVML_ERROR_NOT_SUPPORTED = 3,
    NVML_ERROR_FUNCTION_NOT_FOUND = 13,
    NVML_ERROR_LIB_RM_VERSION_MISMATCH = 18
} nvmlReturn_t;

typedef enum {
    NVML_TEMPERATURE_GPU = 0
} nvmlTemperatureSensors_t;

typedef struct {
    unsigned int gpu;
    unsigned int memory;
} nvmlUtilization_t;

typedef struct {
    unsigned long long total;
    unsigned long long free;
    unsigned long long used;
} nvmlMemory_t;

#endif
