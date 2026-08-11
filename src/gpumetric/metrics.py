from dataclasses import dataclass

from ._ffi import GPUState, GPUMetricFFI
from .exceptions import (
    GPUMetricArgumentError,
    GPUMetricDeviceError,
    GPUMetricError,
    GPUMetricInitializationError,
    GPUMetricNoDeviceError,
    GPUMetricNotInitializedError,
    GPUMetricSamplingError,
)


@dataclass(frozen=True, slots=True)
class GPUStats:
    """
    Immutable snapshot of NVIDIA GPU metrics.

    Attributes:
        temperature:
            GPU temperature in degrees Celsius.

        utilization:
            GPU utilization percentage.

        memory_mib:
            Currently used GPU memory in MiB.

        memory_delta_mib:
            Change in used GPU memory since the previous
            sample in MiB.
    """

    temperature: int
    utilization: int
    memory_mib: int
    memory_delta_mib: int


class GPUMetrics:
    """
    High-level Python interface for NVIDIA GPU metrics.
    """

    def __init__(
            self,
            device_index: int = 0,
            lib_path: str | None = None,
    ) -> None:
        if not isinstance(device_index, int):
            raise GPUMetricArgumentError(
                "device_index must be an integer"
            )

        if device_index < 0:
            raise GPUMetricArgumentError(
                "device_index must be non-negative"
            )

        self._ffi = GPUMetricFFI(lib_path)
        self._initialized = False
        self._device_index = device_index

        ret = self._ffi.init(device_index)

        if ret != 0:
            self._raise_initialization_error(ret)

        self._initialized = True

    def _raise_initialization_error(
            self,
            error_code: int,
    ) -> None:
        """
        Convert a native initialization error code into
        the appropriate Python exception.
        """

        if error_code == -1:
            raise GPUMetricInitializationError(
                "Failed to initialize NVIDIA NVML"
            )

        if error_code == -2:
            raise GPUMetricNoDeviceError(
                "No compatible NVIDIA GPU detected"
            )

        if error_code == -3:
            raise GPUMetricDeviceError(
                f"Failed to access GPU device "
                f"{self._device_index}"
            )

        if error_code == -4:
            raise GPUMetricArgumentError(
                f"Invalid GPU device index: "
                f"{self._device_index}"
            )

        if error_code == -5:
            raise GPUMetricNotInitializedError(
                "GPUMetric native library is not initialized"
            )

        raise GPUMetricError(
            f"GPUMetric initialization failed "
            f"with error code: {error_code}"
        )

    def sample(self) -> GPUStats:
        """
        Sample the current GPU metrics.

        Returns:
            GPUStats:
                Immutable snapshot of the current GPU state.

        Raises:
            GPUMetricNotInitializedError:
                If the instance has already been cleaned up.

            GPUMetricSamplingError:
                If the native library fails to sample metrics.
        """

        if not self._initialized:
            raise GPUMetricNotInitializedError(
                "GPUMetrics is not initialized"
            )

        state = GPUState()

        ret = self._ffi.sample(state)

        if ret != 0:
            if ret == -3:
                raise GPUMetricDeviceError(
                    "Failed to communicate with the GPU "
                    "or fetch GPU metrics"
                )

            if ret == -4:
                raise GPUMetricArgumentError(
                    "Invalid argument passed to GPU metric sampler"
                )

            if ret == -5:
                raise GPUMetricNotInitializedError(
                    "GPUMetric native library is not initialized"
                )

            raise GPUMetricSamplingError(
                f"Failed to sample GPU metrics "
                f"(error code: {ret})"
            )

        return GPUStats(
            temperature=state.temp,
            utilization=state.util,
            memory_mib=state.mem_mib,
            memory_delta_mib=state.delta_mib,
        )

    def cleanup(self) -> None:
        """
        Explicitly release native GPUMetric resources.

        Calling cleanup() multiple times is safe.
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