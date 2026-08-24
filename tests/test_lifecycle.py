from gpumetric import GPUMetrics


def test_repeated_initialization_and_cleanup():
    for _ in range(100):
        with GPUMetrics(device_index=0) as gpu_metrics:
            gpu_metrics.sample()
