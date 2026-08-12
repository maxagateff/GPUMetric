from gpumetric import GPUMetrics, GPUStats

def test_gpu_metrics_initialization():
    with GPUMetrics(device_index=0) as gpu_metrics:
        assert gpu_metrics is not None

def test_gpu_metrics_sample():
    with GPUMetrics(device_index=0) as gpu_metrics:
        stats = gpu_metrics.sample()

        assert isinstance(stats, GPUStats)

def test_gpu_metrics_sample_values():
    with GPUMetrics(device_index=0) as gpu_metrics:

        stats = gpu_metrics.sample()

        assert isinstance(stats.temperature, int)
        assert isinstance(stats.utilization, int)
        assert isinstance(stats.memory_mib, int)
        assert isinstance(stats.memory_delta_mib, int)

        assert stats.temperature >= 0
        assert stats.utilization >= 0
        assert stats.utilization <= 100

        assert stats.memory_mib >= 0

def test_gpu_metrics_multiple_samples():
    with GPUMetrics(device_index=0) as gpu_metrics:
        first = gpu_metrics.sample()
        second = gpu_metrics.sample()
        third = gpu_metrics.sample()

        assert isinstance(first, GPUStats)
        assert isinstance(second, GPUStats)
        assert isinstance(third, GPUStats)

        assert first.memory_mib >= 0
        assert second.memory_mib >= 0
        assert third.memory_mib >= 0

