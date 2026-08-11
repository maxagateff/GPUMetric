class GPUMetricError(Exception):
    """
    Base exception for all GPUMetric errors.
    """


class GPUMetricInitializationError(GPUMetricError):
    """
    Raised when NVIDIA NVML cannot be initialized.
    """


class GPUMetricNoDeviceError(GPUMetricError):
    """
    Raised when no compatible NVIDIA GPU is detected.
    """


class GPUMetricDeviceError(GPUMetricError):
    """
    Raised when GPUMetric cannot communicate with the GPU
    or fetch GPU metrics.
    """


class GPUMetricArgumentError(GPUMetricError, ValueError):
    """
    Raised when an invalid argument is provided.
    """


class GPUMetricNotInitializedError(GPUMetricError):
    """
    Raised when an operation is attempted before initialization
    or after cleanup.
    """


class GPUMetricSamplingError(GPUMetricError):
    """
    Raised when GPU metrics cannot be sampled.
    """