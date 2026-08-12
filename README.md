# GPUMetric

Lightweight NVIDIA GPU telemetry library for Linux and machine learning workloads.

GPUMetric is a small native GPU telemetry library built around **NVIDIA NVML**. The native core is implemented in C and exposed to Python through a thin `ctypes` FFI layer.

The library provides programmatic access to selected GPU metrics from inside a long-running process without repeatedly spawning `nvidia-smi` subprocesses.

## Features

- NVIDIA GPU temperature
- GPU utilization
- GPU memory usage
- GPU memory usage delta between consecutive samples
- Explicit GPU device selection
- Native C core with a C ABI
- Python bindings through `ctypes`
- NVML-backed telemetry collection
- Context-manager based resource management
- Lightweight runtime interface suitable for ML and infrastructure workloads

## Architecture

```text
                    NVIDIA GPU
                        │
                        ▼
                  NVIDIA Driver
                        │
                        ▼
                      NVML
                        │
                        ▼
              ┌─────────────────┐
              │ GPUMetric C Core│
              └────────┬────────┘
                       │
                     C ABI
                       │
                       ▼
              ┌─────────────────┐
              │ Python ctypes   │
              │      FFI        │
              └────────┬────────┘
                       │
                       ▼
                  Python Process
```

The native layer handles NVML communication and GPU telemetry collection. The Python layer provides the application-facing API.

## Supported Environment

The current release target is:

- **Linux x86-64**
- **Ubuntu**
- **NVIDIA GPU**
- **NVIDIA driver with NVML available**
- **CPython 3.11**

Other Linux distributions or Python versions may require separate build and runtime validation.

## Installation

Install directly from PyPI:

```bash
python3.11 -m pip install gpumetric
```

Verify the installation:

```bash
python3.11 -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics, GPUStats)"
```

Before using the library, verify that the NVIDIA driver can access the GPU:

```bash
nvidia-smi
```

## Basic Usage

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()

    print(f"Temperature: {stats.temperature} °C")
    print(f"Utilization: {stats.utilization}%")
    print(f"Memory: {stats.memory_mib} MiB")
    print(f"Memory delta: {stats.memory_delta_mib} MiB")
```

Example output:

```text
Temperature: 42 °C
Utilization: 0%
Memory: 397 MiB
Memory delta: 0 MiB
```

## Public API

The package exposes two primary classes:

```python
from gpumetric import GPUMetrics, GPUStats
```

`GPUMetrics` is the runtime interface used to initialize GPU monitoring and collect samples.

`GPUStats` represents one telemetry sample and contains:

```python
stats.temperature
stats.utilization
stats.memory_mib
stats.memory_delta_mib
```

The primary sampling operation is:

```python
stats = gpu.sample()
```

## Documentation

Complete installation, API, configuration, sampling, error handling, testing, and development documentation is available here:

- [GPUMetric Documentation](docs/DOCS.md)

## Testing

Run the test suite with Python 3.11:

```bash
python3.11 -m pytest -v
```

The integration tests require an accessible NVIDIA GPU and a working NVML installation.

## Repository Structure

```text
GPUMetric/
├── gpumetric/
│   ├── __init__.py
│   ├── metrics.py
│   └── _ffi.py
├── tests/
├── docs/
│   └── README.md
├── CMakeLists.txt
├── pyproject.toml
├── LICENSE
└── README.md
```

## Design Goals

GPUMetric is intentionally focused on GPU telemetry collection rather than being a complete monitoring platform.

It does not prescribe a storage or observability backend. Applications can forward the resulting metrics to their own logging, metrics, tracing, message-bus, or monitoring infrastructure.

Typical integrations include:

- ML training and inference services
- GPU workers
- custom monitoring agents
- Prometheus exporters
- OpenTelemetry pipelines
- internal infrastructure services

## License

GPUMetric is licensed under the **GNU General Public License v3.0 (GPLv3)**.

See the [`LICENSE`](LICENSE) file for the complete license text and terms.
