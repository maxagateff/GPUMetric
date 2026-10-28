#include <stddef.h>
#include <nvml.h>

#include "../../src/gpumetric_core/nvml_memory_abi.h"

_Static_assert(sizeof(GPUMetricMemoryV2) == sizeof(nvmlMemory_v2_t), "NVML MemoryInfo v2 size mismatch");
_Static_assert(_Alignof(GPUMetricMemoryV2) == _Alignof(nvmlMemory_v2_t), "NVML MemoryInfo v2 alignment mismatch");

#define CHECK_OFFSET(field) _Static_assert(offsetof(GPUMetricMemoryV2, field) == offsetof(nvmlMemory_v2_t, field), "NVML MemoryInfo v2 offset mismatch: " #field)

CHECK_OFFSET(version);
CHECK_OFFSET(total);
CHECK_OFFSET(reserved);
CHECK_OFFSET(free);
CHECK_OFFSET(used);

_Static_assert(GPUMETRIC_MEMORY_V2_VERSION == nvmlMemory_v2, "NVML MemoryInfo v2 version mismatch");

int main(void) {return 0;}
