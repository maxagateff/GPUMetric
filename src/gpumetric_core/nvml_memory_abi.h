#ifndef GPUMETRIC_NVML_MEMORY_ABI_H
#define GPUMETRIC_NVML_MEMORY_ABI_H

typedef struct {
    unsigned int version;
    unsigned long long total;
    unsigned long long reserved;
    unsigned long long free;
    unsigned long long used;
} GPUMetricMemoryV2;

#define GPUMETRIC_MEMORY_V2_VERSION ((unsigned int)sizeof(GPUMetricMemoryV2) | (2U << 24U))

#endif
