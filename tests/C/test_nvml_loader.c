#include "nvml_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GPUMETRIC_MOCK_NVML_PATH
#error "GPUMETRIC_MOCK_NVML_PATH is required"
#endif

#ifndef GPUMETRIC_INCOMPLETE_NVML_PATH
#error "GPUMETRIC_INCOMPLETE_NVML_PATH is required"
#endif

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); exit(EXIT_FAILURE); } } while (0)

int main(void) {
    gpumetric_nvml_t nvml = {0};

    /* Invalid arguments */
    CHECK(gpumetric_nvml_load_from_path(NULL, GPUMETRIC_MOCK_NVML_PATH) != 0);
    CHECK(gpumetric_nvml_load_from_path(&nvml, NULL) != 0);
    CHECK(gpumetric_nvml_load_from_path(&nvml, "") != 0);

    /* Library does not exist */
    CHECK(gpumetric_nvml_load_from_path(&nvml, "/gpumetric/nonexistent/libnvidia-ml.so") != 0);

    CHECK(nvml.handle == NULL);
    CHECK(strstr(gpumetric_nvml_error(), "Failed to load NVML") != NULL);

    /* Library is missing required symbols */
    CHECK(gpumetric_nvml_load_from_path(&nvml, GPUMETRIC_INCOMPLETE_NVML_PATH) != 0);

    CHECK(nvml.handle == NULL);
    CHECK(nvml.Init_v2 == NULL);
    CHECK(strstr(gpumetric_nvml_error(), "required symbols") != NULL);

    /* Successful recovery */
    CHECK(gpumetric_nvml_load_from_path(&nvml, GPUMETRIC_MOCK_NVML_PATH) == 0);

    CHECK(nvml.handle != NULL);
    CHECK(nvml.Init_v2 != NULL);
    CHECK(nvml.DeviceGetMemoryInfo != NULL);

    CHECK(gpumetric_nvml_error()[0] == '\0');

    /* Repeated loading must fail without losing the existing handle. */
    void *original_handle = nvml.handle;
    CHECK(gpumetric_nvml_load_from_path(&nvml, GPUMETRIC_MOCK_NVML_PATH) != 0);
    CHECK(nvml.handle == original_handle);
    CHECK(nvml.Init_v2 != NULL);
    CHECK(strstr(gpumetric_nvml_error(), "already loaded") != NULL);

    /* MemoryInfo v2 is optional */
    CHECK(nvml.DeviceGetMemoryInfo_v2 != NULL);

    /* Cleanup */
    gpumetric_nvml_unload(&nvml);

    CHECK(nvml.handle == NULL);
    CHECK(nvml.Init_v2 == NULL);

    /* Repeated cleanup */
    gpumetric_nvml_unload(&nvml);

    /* Reinitialization */
    CHECK(gpumetric_nvml_load_from_path(&nvml, GPUMETRIC_MOCK_NVML_PATH) == 0);
    gpumetric_nvml_unload(&nvml);
    return 0;
}
