from gpumetric import GPUStats

def test_gpu_stats_creation():
    stats = GPUStats(
            temperature=42,
            utilization=0,
            memory_mib=397,
            memory_delta_mib=0
    )

    assert stats.temperature == 42
    assert stats.utilization == 0
    assert stats.memory_mib == 397
    assert stats.memory_delta_mib == 0

def test_gpu_stats_types():
    stats = GPUStats(
            temperature=42,
            utilization=25,
            memory_mib=1024,
            memory_delta_mib=128
            )

    assert isinstance(stats.temperature, int)
    assert isinstance(stats.utilization, int)
    assert isinstance(stats.memory_mib, int)
    assert isinstance(stats.memory_delta_mib, int)
