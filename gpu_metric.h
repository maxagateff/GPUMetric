#pragma once
#include <stdint.h>

/**
 * @brief Structure containing the sampled GPU hardware metrics.
 */
typedef struct {
    uint32_t temp;      /**< GPU temperature in degrees Celsius */
    uint32_t util;      /**< GPU utilization rate as a percentage (0-100) */
    uint64_t mem_mb;    /**< Current memory usage in Megabytes (MB) */
    int64_t delta_mb;   /**< Memory usage difference compared to the previous sample */
} GPUStats;

/**
 * @brief API return error codes.
 */
enum {
    GPU_METRIC_SUCCESS = 0,         /**< Operation completed successfully */
    GPU_METRIC_ERR_NVML = -1,       /**< NVML library initialization failed */
    GPU_METRIC_ERR_NO_DEVICE = -2,  /**< No compatible NVIDIA GPUs detected on the system */
    GPU_METRIC_ERR_DEVICE = -3,     /**< Failed to communicate with the GPU or fetch metrics */
    GPU_METRIC_ERR_ARGUMENT = -4,   /**< Invalid argument passed to the function (e.g., NULL pointer) */
};

/**
 * @brief Initializes the NVML library and binds to the first available GPU.
 * @return 0 on success, or a negative error code on failure.
 */
int gpu_metric_init(void);

/**
 * @brief Samples current GPU metrics and calculates memory consumption delta.
 * @param[out] out Pointer to the GPUStats structure where metrics will be stored.
 * @return 0 on success, or a negative error code on failure.
 */
int gpu_metric_sample(GPUStats* out);

/**
 * @brief Shuts down NVML and releases allocated system resources.
 */
void gpu_metric_cleanup(void);
