#include "nvml_runtime.h"
#include "gpu_metric.h"

#include <limits.h>
#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t runtime_mutex = PTHREAD_MUTEX_INITIALIZER;

static gpumetric_nvml_t runtime_nvml;
static unsigned int runtime_refcount = 0;

int gpumetric_runtime_acquire(const gpumetric_nvml_t **out)
{
    if (!out) return GPU_METRIC_ERR_ARGUMENT;
    *out = NULL;

    if (pthread_mutex_lock(&runtime_mutex) != 0) {return GPU_METRIC_ERR_NVML;}
    if (runtime_refcount == 0) {
        if (gpumetric_nvml_load(&runtime_nvml) != 0) {
            fprintf(stderr, "[GPUMETRIC] NVML load failed: %s\n", gpumetric_nvml_error());
            pthread_mutex_unlock(&runtime_mutex);
            return GPU_METRIC_ERR_NVML;
        }

        nvmlReturn_t ret = runtime_nvml.Init_v2();

        if (ret != NVML_SUCCESS) {
            fprintf(stderr, "[GPUMETRIC] NVML init failed: %s\n", runtime_nvml.ErrorString(ret));
            gpumetric_nvml_unload(&runtime_nvml);
            pthread_mutex_unlock(&runtime_mutex);
            return GPU_METRIC_ERR_NVML;
        }
    }
    if (runtime_refcount == UINT_MAX) {
        pthread_mutex_unlock(&runtime_mutex);
        return GPU_METRIC_ERR_NVML;
    }

    runtime_refcount++;
    *out = &runtime_nvml;
    pthread_mutex_unlock(&runtime_mutex);

    return GPU_METRIC_SUCCESS;
}

void gpumetric_runtime_release(void) {
    if (pthread_mutex_lock(&runtime_mutex) != 0) return;
    if (runtime_refcount == 0) {
        pthread_mutex_unlock(&runtime_mutex);
        return;
    }

    runtime_refcount--;

    if (runtime_refcount == 0) {
        nvmlReturn_t ret = runtime_nvml.Shutdown();
        if (ret != NVML_SUCCESS) {fprintf(stderr, "[GPUMETRIC] NVML shutdown failed: %s\n", runtime_nvml.ErrorString(ret));}
        gpumetric_nvml_unload(&runtime_nvml);
    }
    pthread_mutex_unlock(&runtime_mutex);
}
