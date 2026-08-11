from pathlib import Path

from ._ffi import GPUStates, GPUMetricFFI


class GPUMetrics:
    """
    High-level Python interface for NVIDIA GPU metrics.
    """

    def __init__(self, lib_path: str, device_index: int = 0):
        self._ffi = GPUMetricFFI(lib_path)

        ret = self._ffi.init(device_index)

        if ret != 0:
            raise RuntimeError(
                f"gpu_metric_init failed with error code: {ret}"
            )

    def samples(self) -> GPUState:
        """
        Sample the current GPU metrics.

        Returns:
            GPUState: Current GPU temperature, utilization,
            memory usage, and memory delta.

        Raises:
            RuntimeError: If the native library fails to sample
            GPU metrics.
        """

        stats = GPUState()

        ret = self._ffi.sample(stats)

        if ret != 0:
            raise RuntimeError(
                f"gpu_metric_sample failed with error code: {ret}"
            )

        return stats

    def cleanup(self) -> None:
        """
        Explicitly release the native GPUMetric resources.
        """

        self._ffi.cleanup()

    def __enter__(self) -> "GPUMetrics":
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.cleanup()
