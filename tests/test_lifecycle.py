from gpumetric import GPUMetrics

for _ in range(5):
    with GPUMetrics(device_index=0) as gpu_metrics:
        stats = gpu_metrics.sample()

        print(f"Temperature: {stats.temperature} °C | Utilization: {stats.utilization}% | Memory: {stats.memory_mib} MiB | Memory delta: {stats.memory_delta_mib} MiB")
