#ifndef GPUMETRIC_MOCK_NVML_CONTROL_H
#define GPUMETRIC_MOCK_NVML_CONTROL_H

#include "nvml_compat.h"

void mock_nvml_set_init_failure(nvmlReturn_t value);
void mock_nvml_set_temperature_failure(nvmlReturn_t value);
void mock_nvml_set_utilization_failure(nvmlReturn_t value);
void mock_nvml_set_memory_failure(nvmlReturn_t value);

void mock_nvml_set_memory_used(unsigned int index, unsigned long long bytes);

void mock_nvml_reset_failures(void);

unsigned int mock_nvml_init_calls(void);
unsigned int mock_nvml_shutdown_calls(void);

#endif
