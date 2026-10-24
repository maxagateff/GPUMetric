#pragma once
#include <stdint.h>
typedef struct GPUContext GPUContext;

/**
 * @brief Structure containing the sampled GPU hardware metrics.
 */
typedef struct {
    uint32_t temp;      /**< GPU temperature in degrees Celsius */
    uint32_t util;      /**< GPU utilization rate as a percentage (0-100) */
    uint64_t mem_mib;    /**< Current memory usage in Megabytes (MIB) */
    int64_t delta_mib;   /**< Memory usage difference compared to the previous sample */
    uint64_t reserved_mem_mib; /**< Memory reserved by the driver or firmware */
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
    GPU_METRIC_ERR_NOT_INITIALIZED = -5,
    GPU_METRIC_ERR_DRIVER_MISMATCH = -6, /**< NVIDIA kernel driver and NVML library versions differ */
};

/**
 * @brief Initializes the NVML library and binds to the first available GPU.
 * @return 0 on success, or a negative error code on failure.
 */
int gpu_metric_init(unsigned int device_index);

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
/**
 * @brief Creates a GPU context using its NVML device index.
 *
 * @param device_index NVIDIA GPU index.
 * @param out Receives the allocated GPU context.
 * @return GPU_METRIC_SUCCESS on success, negative error code otherwise.
 */
int gpu_metric_create_by_index(unsigned int device_index, GPUContext **out);

/**
 * @brief Creates a GPU context using its UUID.
 *
 * @param uuid NVIDIA GPU UUID.
 * @param out Receives the allocated GPU context.
 * @return GPU_METRIC_SUCCESS on success, negative error code otherwise.
 */
int gpu_metric_create_by_uuid(const char *uuid, GPUContext **out);

/**
 * @brief Samples metrics from a specific GPU context.
 */
int gpu_metric_context_sample(GPUContext *ctx, GPUStats *out);

/**
 * @brief Releases a GPU context.
 */
void gpu_metric_context_destroy(GPUContext *ctx);