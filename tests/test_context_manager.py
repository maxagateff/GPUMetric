from gpumetric import GPUMetrics, GPUStats

def test_context_manager():
    gpumetric = GPUMetrics(device_index=0)

    with gpumetric as metrics:
        stats = metrics.sample()

        assert isinstance(stats, GPUStats)

