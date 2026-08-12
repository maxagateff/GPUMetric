from gpumetric import GPUMetrics, GPUStats

def test_gpu_metrics_reinitialization():
    with GPUMetrics(device_index=0) as gpu_metrics:
        first = gpu_metrics.sample()

    with GPUMetrics(device_index=0) as gpu_metrics:
        second = gpu_metrics.sample()


    assert isinstance(first, GPUStats)
    assert isinstance(second, GPUStats)

