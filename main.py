from gpumetric import GPUMetrics

while True:

    with GPUMetrics(device_index=0) as gpu_metrics:
        stats = gpu_metrics.sample()

        print(f"Temperature: {stats.temperature} °C")
        print(f"Utilization: {stats.utilization}%")
        print(f"Memory: {stats.memory_mib} MiB")
        print(f"Memory delta: {stats.memory_delta_mib} MiB")
