import pytest

from gpumetric import GPUMetricDriverMismatchError, GPUMetrics


def test_driver_mismatch_has_a_specific_exception():
    metrics = object.__new__(GPUMetrics)
    metrics._device_index = 0

    with pytest.raises(
        GPUMetricDriverMismatchError,
        match="kernel driver and NVML library versions do not match",
    ):
        metrics._raise_initialization_error(-6)
