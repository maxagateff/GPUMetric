from gpumetric import GPUMetrics

gpu = GPUMetrics(
    lib_path="build/libgpumetric.so",
    device_index=0,
)

stats = gpu.samples()

print(stats.temp)
print(stats.util)
print(stats.mem_mb)
print(stats.delta_mb)

gpu.cleanup()