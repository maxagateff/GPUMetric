#include "nvml_loader.h"

#include <string.h>

#ifndef GPUMETRIC_MOCK_NVML_PATH
#error "GPUMETRIC_MOCK_NVML_PATH is not defined"
#endif

/*
 * Test-only loader.
 *
 * Production gpumetric uses nvml_loader.c.
 * Tests use this implementation instead.
 */
int gpumetric_nvml_load(gpumetric_nvml_t *nvml) {return gpumetric_nvml_load_from_path(nvml, GPUMETRIC_MOCK_NVML_PATH);}