from ._ffi import GPUState, GPUMetricFFI


class GPUMetrics:
    """
    High-level Python interface for NVIDIA GPU metrics.
    """

    def __init__(
        self,
        device_index: int = 0,
        lib_path: str | None = None,
    ) -> None:
        self._ffi = GPUMetricFFI(lib_path)
        self._initialized = False

        ret = self._ffi.init(device_index)

        if ret != 0:
            raise RuntimeError(
                f"gpu_metric_init failed with error code: {ret}"
            )

        self._initialized = True

    def sample(self) -> GPUState:
        """
        Sample the current GPU metrics.

        Returns:
            GPUState: Current GPU temperature, utilization,
            memory usage, and memory delta.

        Raises:
            RuntimeError: If the native library fails to sample
            GPU metrics.
        """

        if not self._initialized:
            raise RuntimeError(
                "GPUMetrics is not initialized"
            )

        stats = GPUState()

        ret = self._ffi.sample(stats)

        if ret != 0:
            raise RuntimeError(
                f"gpu_metric_sample failed with error code: {ret}"
            )

        return stats

    def cleanup(self) -> None:
        """
        Explicitly release native GPUMetric resources.
        """

        if self._initialized:
            self._ffi.cleanup()
            self._initialized = False

    def __enter__(self) -> "GPUMetrics":
        return self

    def __exit__(
        self,
        exc_type,
        exc_value,
        traceback,
    ) -> None:
        self.cleanup()

    def __del__(self) -> None:
        """
        Best-effort cleanup when the object is garbage-collected.
        """

        try:
            self.cleanup()
        except Exception:
            pass
