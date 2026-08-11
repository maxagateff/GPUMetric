from gpumetric import GPUMetrics

gpu = GPUMetrics(device_index=0)

stats = gpu.sample()

print(f"Temperature: {stats.temp} °C")
print(f"Utilization: {stats.util} %")
print(f"Memory:      {stats.mem_mib} MiB")
print(f"Memory Δ:    {stats.delta_mib} MiB")

gpu.cleanup()
