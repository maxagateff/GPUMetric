from gpumetric import GPUMetrics
print(GPUMetrics)
stats = GPUMetrics(device_index=0)
print(stats)
print(stats.sample())
