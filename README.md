# GPUMetric

Fast, lightweight NVIDIA GPU telemetry for Linux applications and machine learning workloads.

GPUMetric reads GPU temperature, utilization, and memory information directly through NVIDIA NVML. Its native core is written in C, loaded by a small Python `ctypes` layer, and packaged with the Python wheel. It is designed for long-running applications that need in-process telemetry without repeatedly launching and parsing `nvidia-smi`.

> GPUMetric is alpha software. The Python API and native ABI may change before version 1.0.

## Highlights

- In-process NVIDIA GPU telemetry
- Temperature and GPU utilization
- Allocated, reserved, and delta memory values
- Explicit GPU index selection
- Immutable Python result objects
- Context-manager and explicit cleanup support
- Runtime NVML loading through `dlopen` and `dlsym`
- No compile-time dependency on NVIDIA headers
- manylinux 2.28 wheels for CPython 3.10 through 3.14
- No CUDA context creation or CUDA memory allocation

## Requirements

- Linux x86-64
- glibc 2.28 or newer for published wheels
- CPython 3.10 through 3.14
- An NVIDIA GPU
- An NVIDIA driver that provides `libnvidia-ml.so.1`

Verify the driver:

```bash
nvidia-smi
```

GPUMetric does not require the CUDA toolkit.

## Installation

```bash
python -m pip install gpumetric
```

Verify the package:

```bash
python -c "import gpumetric; print(gpumetric.__version__)"
```

## Quick Start

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()

    print(f"Temperature: {stats.temperature} °C")
    print(f"Utilization: {stats.utilization}%")
    print(f"Allocated memory: {stats.memory_mib} MiB")
    print(f"Memory delta: {stats.memory_delta_mib:+d} MiB")
    print(f"Reserved memory: {stats.memory_reserved_mib} MiB")
```

Example output on an idle GPU:

```text
Temperature: 42 °C
Utilization: 0%
Allocated memory: 0 MiB
Memory delta: +0 MiB
Reserved memory: 396 MiB
```

Actual values depend on the GPU, driver, operating system, and active workloads.

## Repeated Sampling

Create one instance and reuse it:

```python
import time

from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    while True:
        stats = gpu.sample()
        print(
            f"temperature={stats.temperature}C "
            f"utilization={stats.utilization}% "
            f"memory={stats.memory_mib}MiB "
            f"delta={stats.memory_delta_mib:+d}MiB "
            f"reserved={stats.memory_reserved_mib}MiB"
        )
        time.sleep(1)
```

The first successful sample reports a memory delta of zero. Later samples compare the current device-wide allocated memory with the previous successful sample.

## Telemetry

`GPUMetrics.sample()` returns an immutable `GPUStats` object.

| Field | Type | Unit | Meaning |
| --- | ---: | ---: | --- |
| `temperature` | `int` | °C | Current GPU temperature |
| `utilization` | `int` | % | GPU execution utilization |
| `memory_mib` | `int` | MiB | Allocated device memory reported by NVML |
| `memory_delta_mib` | `int` | MiB | Change since the previous successful sample |
| `memory_reserved_mib` | `int` | MiB | Memory reserved for driver or firmware use |

### Memory Semantics

GPUMetric prefers `nvmlDeviceGetMemoryInfo_v2`, which separates allocated memory from memory reserved by the driver or firmware.

If the installed runtime does not provide v2, GPUMetric falls back to `nvmlDeviceGetMemoryInfo`. The legacy API can include system-reserved memory in its used-memory value. In fallback mode, `memory_reserved_mib` is zero because the legacy structure does not expose the reservation separately.

Memory values are device-wide. They are not per-process totals, framework allocator statistics, CUDA tensor sizes, proof of a memory leak, or attribution to a specific application. Use a CUDA-aware profiler or framework allocator API when allocation-level attribution is required.

## Application Integration

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    for step, batch in enumerate(dataloader):
        loss = train_step(batch)
        stats = gpu.sample()

        emit_metrics({
            "step": step,
            "loss": float(loss.item()),
            "gpu_temperature_celsius": stats.temperature,
            "gpu_utilization_percent": stats.utilization,
            "gpu_memory_mib": stats.memory_mib,
            "gpu_memory_delta_mib": stats.memory_delta_mib,
            "gpu_memory_reserved_mib": stats.memory_reserved_mib,
        })
```

GPUMetric collects telemetry. Your application decides how to log, aggregate, export, store, alert on, or visualize it.

## Public API

```python
from gpumetric import (
    GPUMetrics,
    GPUStats,
    GPUMetricError,
    GPUMetricArgumentError,
    GPUMetricDeviceError,
    GPUMetricDriverMismatchError,
    GPUMetricInitializationError,
    GPUMetricNoDeviceError,
    GPUMetricNotInitializedError,
    GPUMetricSamplingError,
)
```

### `GPUMetrics`

```python
GPUMetrics(device_index: int = 0, lib_path: str | None = None)
```

- `device_index` must be a non-negative integer. Boolean values are rejected.
- `lib_path` overrides the bundled native-library path for development builds.
- `sample()` returns `GPUStats`.
- `cleanup()` is idempotent.
- The class supports the context-manager protocol.

### Exceptions

| Exception | Meaning |
| --- | --- |
| `GPUMetricArgumentError` | Invalid device index or native argument |
| `GPUMetricInitializationError` | NVML could not be initialized |
| `GPUMetricDriverMismatchError` | The kernel driver and loaded NVML library do not match |
| `GPUMetricNoDeviceError` | No compatible NVIDIA GPU was found |
| `GPUMetricDeviceError` | The selected GPU could not be accessed or sampled |
| `GPUMetricNotInitializedError` | Sampling was attempted after cleanup |
| `GPUMetricSamplingError` | An unclassified native sampling error occurred |

All public exceptions inherit from `GPUMetricError`.
`GPUMetricDriverMismatchError` also inherits from
`GPUMetricInitializationError`, so existing initialization handlers continue
to work.

## Current Process Model

The native implementation owns one process-wide NVML state, selected device handle, and memory-delta cache. Create one active `GPUMetrics` instance per process:

```python
with GPUMetrics(device_index=0) as gpu:
    ...
```

A second instance does not create an independent native context. Independent simultaneous contexts and concurrent multi-GPU sampling require a future context-based native API.

## Why Not `nvidia-smi`?

`nvidia-smi` remains the right tool for host administration, driver diagnostics, and manual inspection. GPUMetric is intended for repeated application-side sampling:

```text
Python application
        │
        ▼
      ctypes
        │
        ▼
libgpumetric.so
        │
        ▼
       NVML
        │
        ▼
       GPU
```

This avoids creating an external process and parsing command-line output for every sample.

## Development

```bash
git clone https://github.com/maxagateff/GPUMetric.git
cd GPUMetric
git switch develop

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
python -m pytest -q
```

GPU integration tests require a working NVIDIA driver, NVML runtime, and accessible GPU.

## Native Build

The C target requires CMake 3.18 or newer, a C11 compiler, and `libdl` on Linux.

GPUMetric does not link against NVML. The loader resolves required functions from `libnvidia-ml.so.1` at runtime. Minimal stable ABI declarations live in `src/gpumetric_core/nvml_compat.h`.

## Wheel Builds and Releases

GitHub Actions uses `cibuildwheel` and manylinux 2.28 to build x86-64 wheels for CPython 3.10 through 3.14.

- Pushes to `develop` build and upload wheel artifacts.
- Published GitHub Releases build wheels and publish them to PyPI.
- Development pushes do not publish to PyPI.

## Repository Layout

```text
GPUMetric/
├── .github/workflows/python-publish.yml
├── docs/DOCS.md
├── src/
│   ├── gpumetric/
│   │   ├── __init__.py
│   │   ├── _ffi.py
│   │   ├── exceptions.py
│   │   └── metrics.py
│   └── gpumetric_core/
│       ├── gpu_metric.c
│       ├── gpu_metric.h
│       ├── nvml_compat.h
│       ├── nvml_loader.c
│       └── nvml_loader.h
├── tests/
├── CMakeLists.txt
├── pyproject.toml
└── README.md
```

## Documentation

See [docs/DOCS.md](docs/DOCS.md) for the complete Python API, native ABI, memory semantics, lifecycle, build, release, troubleshooting, and integration documentation.

## License

GPUMetric is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE).
