from src.gpumetric.FFI import GPUMetrics

gpu_metrics = GPUMetrics(lib_path="build/libgpumetric.so", device_index=0)

ret, stats = gpu_metrics.samples()

if ret == 0:
    print(
        f"Temperature: {stats.temp}C | "
        f"Util: {stats.util}% | "
        f"Memory: {stats.mem_mb}MiB | "
        f"Delta Memory: {stats.delta_mb}MiB"
    )
else:
    print(f"GPU metric collection failed: {ret}")