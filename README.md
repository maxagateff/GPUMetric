# GPUMetric

Lightweight NVIDIA GPU telemetry library for Linux and machine learning workloads.

GPUMetric is a small native GPU telemetry library built around **NVIDIA NVML**. The native core is implemented in **C** and exposed to Python through a thin **`ctypes` FFI** layer.

It provides programmatic access to selected NVIDIA GPU metrics from inside a long-running application process without repeatedly spawning `nvidia-smi`.

The project is intentionally focused on **GPU telemetry collection**. It does not implement a monitoring backend, metric storage, dashboards, alerting, or distributed telemetry infrastructure.

The application collects the data through GPUMetric and decides what to do with it.

---

## Architecture

```text
┌─────────────────────────────────────────────────────────────┐
│                         Application                         │
│                                                             │
│     ML Training / Inference / GPU Worker / Agent           │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               │ Python API
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                      Python Package                         │
│                                                             │
│  GPUMetrics     GPUStats     Exceptions                     │
│                         │                                   │
│                         ▼                                   │
│                    ctypes FFI                               │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               │ C ABI
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                     Native C Core                           │
│                                                             │
│                     libgpumetric.so                         │
│                                                             │
│        NVML initialization / device / sampling             │
│        memory-delta state / native lifecycle               │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               │ NVML
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                    NVIDIA Driver                            │
│                         │                                   │
│                         ▼                                   │
│                        GPU                                  │
└─────────────────────────────────────────────────────────────┘
```

The native layer communicates with NVIDIA NVML.

The Python layer provides the application-facing API.

The `ctypes` FFI and C ABI form the boundary between the two.

---

## Features

* NVIDIA GPU temperature
* GPU utilization
* GPU memory usage
* GPU memory usage delta between consecutive samples
* Explicit GPU device selection
* Native C telemetry core
* C ABI between native and Python layers
* NVIDIA NVML integration
* Python bindings through `ctypes`
* Context-manager based resource lifecycle
* Explicit cleanup support
* Typed Python exceptions
* Lightweight runtime interface for ML and infrastructure workloads
* No repeated `nvidia-smi` subprocesses for application-side sampling

---

## Telemetry

The public telemetry object is `GPUStats`:

```python
stats.temperature
stats.utilization
stats.memory_mib
stats.memory_delta_mib
```

A sample contains:

| Field              | Description                                                   |
| ------------------ | ------------------------------------------------------------- |
| `temperature`      | GPU temperature in °C                                         |
| `utilization`      | GPU utilization percentage                                    |
| `memory_mib`       | Current GPU memory usage in MiB                               |
| `memory_delta_mib` | Change in reported GPU memory usage since the previous sample |

`GPUStats` is an immutable Python dataclass.

The memory delta is a telemetry signal. It is **not** a CUDA memory profiler and does not identify individual allocations, tensors, processes, or CUDA subsystems.

---

## Installation

Install GPUMetric from PyPI:

```bash
python3.11 -m pip install gpumetric
```

Verify the package:

```bash
python3.11 -c "import gpumetric; print(gpumetric)"
```

Verify the public API:

```bash
python3.11 -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics, GPUStats)"
```

Before using GPUMetric, verify that the NVIDIA driver can access the GPU:

```bash
nvidia-smi
```

---

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

Actual values depend on the current GPU state and workload.

---

## Repeated Sampling

Initialize `GPUMetrics` once and reuse it:

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
            f"memory_delta={stats.memory_delta_mib}MiB"
        )

        time.sleep(1)
```

The native sampling state is maintained between calls, allowing `memory_delta_mib` to be calculated between consecutive samples.

---

## ML Infrastructure

GPUMetric can be embedded directly into training and inference processes.

```python
from gpumetric import GPUMetrics


with GPUMetrics(device_index=0) as gpu:
    for step, batch in enumerate(dataloader):
        loss = train_step(batch)

        stats = gpu.sample()

        metrics = {
            "step": step,
            "loss": float(loss.item()),
            "gpu_temperature_celsius": stats.temperature,
            "gpu_utilization_percent": stats.utilization,
            "gpu_memory_mib": stats.memory_mib,
            "gpu_memory_delta_mib": stats.memory_delta_mib,
        }

        emit_metrics(metrics)
```

Typical integrations include:

* ML training workers
* inference services
* GPU workers
* custom monitoring agents
* Prometheus exporters
* OpenTelemetry pipelines
* internal infrastructure services

GPUMetric collects the GPU state. The application owns the telemetry pipeline.

---

## Why GPUMetric?

For application-side telemetry, repeatedly invoking:

```text
nvidia-smi
```

requires repeatedly creating an external process, waiting for it to finish, reading its output, and parsing the result.

GPUMetric provides an in-process path:

```text
Python Application
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

This removes the external CLI process boundary from the application's sampling path.

`nvidia-smi` remains useful for:

* host diagnostics
* driver validation
* manual GPU inspection
* administration
* troubleshooting

GPUMetric is intended for programmatic, application-side GPU telemetry.

---

## Process Model

The current native implementation maintains a **single process-wide GPU context**.

This means an application should normally create one active `GPUMetrics` instance for the native context.

For example:

```python
with GPUMetrics(device_index=0) as gpu:
    ...
```

The native implementation stores the selected NVML device and sampling state globally within the process.

Creating another `GPUMetrics` instance while the native library is already initialized does not create an independent native GPU context.

Applications requiring multiple independent GPU contexts should account for this limitation.

See the complete documentation for the current lifecycle and native-state model.

---

## Supported Environment

The primary validated environment is:

* Linux x86-64
* Ubuntu
* NVIDIA GPU
* NVIDIA driver with NVML support
* CPython 3.11

The package metadata currently declares Python `>=3.10`, but the primary development and validation target is CPython 3.11.

Other Linux distributions, Python versions, architectures, or GPU vendors require separate validation.

---

## Public API

```python
from gpumetric import (
    GPUMetrics,
    GPUStats,
    GPUMetricError,
    GPUMetricArgumentError,
    GPUMetricDeviceError,
    GPUMetricInitializationError,
    GPUMetricNoDeviceError,
    GPUMetricNotInitializedError,
    GPUMetricSamplingError,
)
```

The primary runtime interface is:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

The returned `GPUStats` contains:

```python
stats.temperature
stats.utilization
stats.memory_mib
stats.memory_delta_mib
```

---

## Documentation

The complete technical documentation covers:

* architecture
* runtime model
* public API
* telemetry semantics
* native C layer
* C ABI
* Python FFI
* installation
* development builds
* testing
* lifecycle
* error handling
* ML integration
* observability integration
* troubleshooting

**[Read the complete GPUMetric documentation](docs/DOCS.md)**

---

## Testing

Run the test suite with Python 3.11:

```bash
python3.11 -m pytest -v
```

The integration tests that initialize `GPUMetrics` require an accessible NVIDIA GPU and a working NVML environment.

The repository currently contains tests covering:

* package import
* public API
* `GPUStats`
* `GPUMetrics` initialization
* sampling
* device index validation
* context-manager lifecycle
* cleanup
* reinitialization

---

## Repository Structure

The current `develop` branch uses a `src/` package layout:

```text
GPUMetric/
├── .github/
│   └── workflows/
│
├── docs/
│   └── DOCS.md
│
├── src/
│   ├── gpumetric/
│   │   ├── __init__.py
│   │   ├── _ffi.py
│   │   ├── exceptions.py
│   │   └── metrics.py
│   │
│   └── gpumetric_core/
│       ├── gpu_metric.c
│       └── gpu_metric.h
│
├── tests/
│   ├── test_context_manager.py
│   ├── test_device_index_type.py
│   ├── test_gpu_metrics.py
│   ├── test_gpu_metrics_reinitialization.py
│   ├── test_gpu_stats.py
│   ├── test_import.py
│   └── test_lifecycle.py
│
├── CMakeLists.txt
├── LICENSE
├── pyproject.toml
└── README.md
```

The native implementation is under:

```text
src/gpumetric_core/
```

The Python package is under:

```text
src/gpumetric/
```

The native shared library is packaged under:

```text
gpumetric/lib/libgpumetric.so
```

and loaded internally by the Python FFI layer.

---

## Design Goal

GPUMetric is intentionally a **telemetry collection component**, not a complete observability platform.

It owns:

```text
NVML communication
GPU selection
GPU telemetry
sampling state
memory delta calculation
native lifecycle
Python/native FFI boundary
```

The application owns:

```text
sampling schedule
metric naming
aggregation
serialization
transport
storage
alerting
visualization
distributed processing
```

The central architectural principle is:

> **GPUMetric collects GPU state; the application decides what to do with it.**

---

## License

GPUMetric is licensed under the GNU General Public License v3.0 (GPLv3).

See [LICENSE](LICENSE) for the complete license text and terms.
