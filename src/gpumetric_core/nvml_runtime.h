#ifndef GPUMETRIC_NVML_RUNTIME_H
#define GPUMETRIC_NVML_RUNTIME_H

#include "nvml_loader.h"

int gpumetric_runtime_acquire(const gpumetric_nvml_t **out);
void gpumetric_runtime_release(void);

#endif