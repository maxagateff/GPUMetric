import pytest

from gpumetric import GPUMetricArgumentError, GPUMetrics


@pytest.mark.parametrize("device_index", [-1, [], "0", None, True])
def test_invalid_device_index_is_rejected(device_index):
    with pytest.raises(GPUMetricArgumentError):
        GPUMetrics(device_index=device_index)
