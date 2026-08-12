def test_import_gpumetric():
    import gpumetric
    assert gpumetric is not None

def test_public_api():
    from gpumetric import GPUMetrics, GPUStats

    assert GPUMetrics is not None
    assert GPUStats is not None
