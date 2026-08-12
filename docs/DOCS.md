# GPUMetric Documentation

Technical documentation for installing, configuring, using, testing, and integrating GPUMetric.

GPUMetric is a lightweight NVIDIA GPU telemetry library for Linux and machine learning workloads. The native core is implemented in C, communicates with NVIDIA NVML, and is exposed to Python through a thin `ctypes` FFI layer.

---

## 1. Supported Environment

The current supported target is:

- **Operating system:** Linux x86-64
- **Primary distribution:** Ubuntu
- **Python:** CPython 3.11
- **GPU:** NVIDIA GPU
- **GPU management interface:** NVIDIA NVML

The current package and test matrix is built around this environment. Other Linux distributions, architectures, Python versions, or GPU vendors are outside the currently validated target and require independent testing.

### 1.1 NVIDIA Driver

GPUMetric requires a functional NVIDIA driver with NVML available.

Verify the host before installing or debugging GPUMetric:

```bash
nvidia-smi
```

The command must be able to communicate with the NVIDIA GPU and display its state.

If `nvidia-smi` fails, GPUMetric should be expected to fail as well because both depend on the NVIDIA driver/NVML stack for GPU management access.

### 1.2 Python

Use CPython 3.11 for the current supported package environment:

```bash
python3.11 --version
```

Expected form:

```text
Python 3.11.x
```

Create an isolated environment before installation:

```bash
python3.11 -m venv .venv
source .venv/bin/activate
```

Upgrade the packaging tools:

```bash
python -m pip install --upgrade pip setuptools wheel
```

---

## 2. Installation

Install GPUMetric from PyPI:

```bash
python3.11 -m pip install gpumetric
```

Verify that the package imports correctly:

```bash
python3.11 -c "import gpumetric; print(gpumetric)"
```

Verify the public classes:

```bash
python3.11 -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics, GPUStats)"
```

A successful import confirms that the Python package is available. It does not by itself prove that NVML can access the GPU; that must be validated separately with `nvidia-smi` and an actual sampling call.

---

## 3. Minimal Usage

The recommended application pattern is:

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

The exact values depend on the current GPU state and workload.

---

## 4. Public API

GPUMetric currently exposes two primary public classes:

```python
from gpumetric import GPUMetrics, GPUStats
```

### 4.1 `GPUMetrics`

`GPUMetrics` is the runtime interface for GPU telemetry collection.

The constructor accepts a GPU device index:

```python
GPUMetrics(device_index=0)
```

The primary operation is:

```python
stats = gpu.sample()
```

The class supports the context-manager protocol:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

The context manager defines a clear lifecycle for the native monitoring resources.

### 4.2 `GPUStats`

`GPUStats` is the data object returned by `GPUMetrics.sample()`.

A sample contains four fields:

```python
stats.temperature
stats.utilization
stats.memory_mib
stats.memory_delta_mib
```

`GPUStats` represents telemetry data. It does not perform GPU initialization or query NVML by itself.

---

## 5. Device Selection

A GPU is selected through `device_index`:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

Device numbering should be validated against the host's NVIDIA configuration:

```bash
nvidia-smi -L
```

Example:

```text
GPU 0: NVIDIA ...
GPU 1: NVIDIA ...
```

The device index passed to `GPUMetrics` is used by the native layer when selecting the GPU through NVML.

Applications running on single-GPU hosts will normally use:

```python
GPUMetrics(device_index=0)
```

---

## 6. Sampling

Call `sample()` to obtain one telemetry snapshot:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

Inspect the returned object:

```python
print(type(stats))
```

The expected public type is:

```text
<class 'gpumetric.metrics.GPUStats'>
```

A sample can be serialized or forwarded to an application-level metrics system using its four fields.

Example:

```python
telemetry = {
    "gpu_temperature_celsius": stats.temperature,
    "gpu_utilization_percent": stats.utilization,
    "gpu_memory_mib": stats.memory_mib,
    "gpu_memory_delta_mib": stats.memory_delta_mib,
}
```

---

## 7. `GPUStats` Fields

### 7.1 `temperature`

GPU temperature in degrees Celsius as reported by NVML.

```python
stats.temperature
```

Example:

```text
42
```

The value should be interpreted as an instantaneous telemetry reading rather than a sustained thermal statistic.

### 7.2 `utilization`

GPU utilization expressed as a percentage.

```python
stats.utilization
```

The expected range is:

```text
0 - 100
```

### 7.3 `memory_mib`

Current GPU memory usage in MiB.

```python
stats.memory_mib
```

Example:

```text
397
```

This value reflects the memory usage reported by the native NVML-backed implementation.

### 7.4 `memory_delta_mib`

Change in reported GPU memory usage between samples.

```python
stats.memory_delta_mib
```

Conceptually:

```text
memory_delta_mib = current_memory_mib - previous_memory_mib
```

For example:

```text
Sample 1: 397 MiB
Sample 2: 430 MiB

Delta: +33 MiB
```

The exact initial-sample behavior is determined by the native sampling state. Applications should treat the delta as an observation between samples rather than as a memory profiler result.

`memory_delta_mib` does not identify which CUDA allocation, tensor, process, or subsystem caused the change.

---

## 8. Repeated Sampling

For monitoring loops, initialize `GPUMetrics` once and reuse it for multiple samples.

Recommended:

```python
import time

from gpumetric import GPUMetrics


with GPUMetrics(device_index=0) as gpu:
    while True:
        stats = gpu.sample()

        print(f"Temperature: {stats.temperature} °C")
        print(f"Utilization: {stats.utilization}%")
        print(f"Memory: {stats.memory_mib} MiB")
        print(f"Memory delta: {stats.memory_delta_mib} MiB")

        time.sleep(1)
```

The intended lifecycle is:

```text
GPUMetrics initialization
        │
        ├── sample()
        ├── sample()
        ├── sample()
        ├── sample()
        │
        ▼
GPUMetrics shutdown
```

Repeatedly constructing and destroying `GPUMetrics` for every sample is not the intended monitoring pattern.

---

## 9. Context Manager and Resource Lifecycle

Use the context manager when the monitoring object has a bounded lifetime:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

This provides an explicit boundary for resource initialization and cleanup.

The lifecycle can be viewed as:

```text
enter context
    │
    ▼
initialize GPU monitoring
    │
    ▼
collect samples
    │
    ▼
exit context
    │
    ▼
release native resources
```

For long-running processes, keep the context open for the duration of the monitoring lifecycle rather than opening a new context around every individual sample.

---

## 10. Machine Learning Integration

GPUMetric can be used inside training or inference processes to correlate workload-level metrics with hardware telemetry.

Example:

```python
from gpumetric import GPUMetrics


with GPUMetrics(device_index=0) as gpu:
    for step, batch in enumerate(dataloader):
        loss = train_step(batch)
        stats = gpu.sample()

        print(
            f"step={step} "
            f"loss={loss.item():.4f} "
            f"gpu_util={stats.utilization}% "
            f"gpu_temp={stats.temperature}C "
            f"gpu_memory={stats.memory_mib}MiB "
            f"gpu_memory_delta={stats.memory_delta_mib}MiB"
        )
```

This allows application-level signals such as:

- loss
- learning rate
- throughput
- batch size
- inference latency
- training step

To be observed alongside:

- GPU utilization
- GPU temperature
- GPU memory usage
- GPU memory change

This can be useful when investigating GPU under-utilization, memory growth, thermal behavior, or workload-dependent changes in GPU state.

---

## 11. Telemetry Integration

GPUMetric intentionally does not prescribe a telemetry backend.

The application decides where samples are exported.

Typical integration targets include:

- application logs
- Prometheus exporters
- OpenTelemetry pipelines
- Kafka or another message broker
- time-series databases
- internal ML infrastructure
- custom monitoring services

Example transformation:

```python
stats = gpu.sample()

metric_record = {
    "temperature_celsius": stats.temperature,
    "utilization_percent": stats.utilization,
    "memory_mib": stats.memory_mib,
    "memory_delta_mib": stats.memory_delta_mib,
}
```

The intended architecture is:

```text
                    NVIDIA GPU
                        │
                        ▼
                       NVML
                        │
                        ▼
                    GPUMetric
                        │
                        ▼
                   ML Application
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
        Logs       Prometheus    OpenTelemetry
                                      │
                                      ▼
                                Telemetry Backend
```

GPUMetric owns GPU state collection. Storage, transport, aggregation, alerting, and visualization remain application-level concerns.

---

## 12. Error Handling

The Python API is backed by a native C implementation and NVML. Initialization and sampling can therefore fail when the host GPU environment is unavailable or invalid.

Typical causes include:

- NVIDIA driver failure
- NVML initialization failure
- invalid GPU device index
- inaccessible GPU device
- missing native library
- incompatible runtime environment

At the application level, handle initialization and sampling failures explicitly.

Example:

```python
from gpumetric import GPUMetrics


try:
    with GPUMetrics(device_index=0) as gpu:
        stats = gpu.sample()
        print(stats)
except Exception as exc:
    print(f"GPU monitoring failed: {exc}")
```

For production applications, catch the specific exception types exposed by the installed package when those types are available and stable in the application dependency policy.

Do not assume that a successful Python import means that GPU telemetry is available. Importability and runtime NVML availability are separate checks.

---

## 13. Why GPUMetric Instead of Repeated `nvidia-smi` Calls?

`nvidia-smi` is an NVIDIA command-line diagnostic and administration tool. GPUMetric addresses a different problem: programmatic telemetry collection from inside an application process.

A CLI-based approach repeatedly creates an external process:

```text
Application
│
├── spawn nvidia-smi
├── wait for process
├── read stdout
├── parse output
│
├── spawn nvidia-smi
├── wait for process
├── read stdout
├── parse output
│
└── ...
```

GPUMetric provides a direct in-process path:

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

This removes the need to repeatedly spawn and parse `nvidia-smi` for application telemetry.

`nvidia-smi` remains valuable for:

- host diagnostics
- driver validation
- manual GPU inspection
- administration
- troubleshooting

GPUMetric is intended for application-side, repeated telemetry collection.

---

## 14. Native and FFI Architecture

The system is divided into a native layer and a Python binding layer.

```text
Python application
        │
        ▼
GPUMetrics / GPUStats
        │
        ▼
Python FFI
        │
        ▼
ctypes
        │
        ▼
C ABI
        │
        ▼
Native GPUMetric library
        │
        ▼
NVIDIA NVML
        │
        ▼
GPU
```

### Native C Layer

The native layer is responsible for:

- NVML initialization
- GPU handle access
- telemetry collection
- sampling state
- memory delta calculation
- native error handling

### Python Layer

The Python layer is responsible for:

- loading the native library
- defining FFI types and function signatures
- exposing the Python API
- representing returned telemetry as Python objects

This separation allows the native telemetry implementation to remain independent from the application consuming the data.

---

## 15. Shared Library

The native implementation is packaged as a shared library named:

```text
libgpumetric.so
```

The shared library contains the native GPU telemetry implementation and its C ABI.

The Python package uses the native library internally through the FFI layer.

Applications should interact with the documented Python API rather than depending on internal Python FFI implementation details unless they explicitly require lower-level integration.

---

## 16. Development and Local Build

The Python package is installed from PyPI for normal use. Native development is performed from the repository.

Install build dependencies on Ubuntu:

```bash
chmod +x scripts/install_deps.sh
./scripts/install_deps.sh
```

Verify the required host tools:

```bash
cmake --version
cc --version
nvidia-smi
python3.11 --version
```

Build the native component using the repository helper script when available:

```bash
chmod +x scripts/build/initial_run_of_the_cmake_build.sh
./scripts/build/initial_run_of_the_cmake_build.sh
```

Or build directly with CMake:

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
cd ..
```

Verify the resulting shared library:

```bash
ls -lh build/libgpumetric.so
```

The resulting binary is the native implementation consumed by the Python FFI layer in repository-level development and testing workflows.

---

## 17. Testing

The project uses `pytest` for Python-level and integration testing.

Install the test dependency:

```bash
python3.11 -m pip install pytest
```

Run the complete suite:

```bash
python3.11 -m pytest
```

Run in verbose mode:

```bash
python3.11 -m pytest -v
```

The `-v` flag enables verbose pytest output and reports each test individually.

Example:

```text
test_context_manager.py::test_context_manager PASSED
test_gpu_metrics.py::test_gpu_metrics_initialization PASSED
test_gpu_metrics.py::test_gpu_metrics_sample PASSED
test_gpu_metrics.py::test_gpu_metrics_sample_values PASSED
test_gpu_metrics.py::test_gpu_metrics_multiple_samples PASSED
test_gpu_metrics_reinitialization.py::test_gpu_metrics_reinitialization PASSED
test_gpu_stats.py::test_gpu_stats_creation PASSED
test_gpu_stats.py::test_gpu_stats_types PASSED
test_import.py::test_import_gpumetric PASSED
test_import.py::test_public_api PASSED
```

A successful test run reports all tests as passed.

### 17.1 Integration Test Requirements

Integration tests that call `GPUMetrics(device_index=0)` require:

- Linux x86-64
- Ubuntu or a compatible Linux environment
- NVIDIA GPU
- working NVIDIA driver
- working NVML
- CPython 3.11

A test environment without an accessible NVIDIA GPU cannot execute the full runtime integration path successfully.

---

## 18. Clean Installation Test

When validating a package build or release candidate, install it into a clean environment rather than relying only on an existing development environment.

Example:

```bash
python3.11 -m venv /tmp/gpumetric-test
source /tmp/gpumetric-test/bin/activate
python -m pip install --upgrade pip
python -m pip install gpumetric
```

Verify the public API:

```bash
python -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics, GPUStats)"
```

Run the integration tests if the environment has access to the GPU:

```bash
python -m pytest -v
```

This is an important distinction:

```text
git checkout
    ↓
local source
    ↓
works in developer environment
```

is not equivalent to:

```text
PyPI
    ↓
pip install
    ↓
clean virtual environment
    ↓
import
    ↓
NVML
    ↓
GPU
```

The second path validates the artifact that users actually install.

---

## 19. Troubleshooting

### `nvidia-smi` fails

First resolve the NVIDIA driver or host GPU issue.

GPUMetric depends on the same underlying NVIDIA driver/NVML environment.

### `pip install gpumetric` fails

Check:

```bash
python3.11 --version
python3.11 -m pip --version
```

The current package target is CPython 3.11 on Linux x86-64.

Also check that pip is using PyPI rather than a private or stale package index:

```bash
python3.11 -m pip config list
```

### Import succeeds but sampling fails

Verify:

```bash
nvidia-smi
```

Then test the actual API:

```bash
python3.11 - <<'PY'
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    print(gpu.sample())
PY
```

If initialization fails, investigate the NVIDIA driver, NVML availability, device index, and runtime permissions.

### Invalid device index

Validate the available devices:

```bash
nvidia-smi -L
```

Then use a valid device index:

```python
GPUMetrics(device_index=0)
```

---

## 20. Example: Minimal Monitoring Service

A long-running service should normally initialize the monitor once and reuse it:

```python
import time

from gpumetric import GPUMetrics


def main() -> None:
    with GPUMetrics(device_index=0) as gpu:
        while True:
            stats = gpu.sample()

            record = {
                "temperature_celsius": stats.temperature,
                "utilization_percent": stats.utilization,
                "memory_mib": stats.memory_mib,
                "memory_delta_mib": stats.memory_delta_mib,
            }

            print(record)
            time.sleep(1)


if __name__ == "__main__":
    main()
```

The service can then replace the `print(record)` operation with an application-specific exporter.

---

## 21. Example: Correlating GPU Telemetry with Training State

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

This pattern keeps hardware telemetry close to the application state that produced the workload.

---

## 22. Project Boundaries

GPUMetric is a telemetry component, not a complete observability system.

It does not provide, by itself:

- metric storage
- dashboards
- alerting
- distributed aggregation
- message-bus delivery
- time-series querying
- GPU scheduling
- CUDA memory profiling
- model-level performance profiling

Those capabilities belong to the surrounding infrastructure.

The intended boundary is:

```text
GPU
 │
 ▼
NVML
 │
 ▼
GPUMetric
 │
 ▼
Application
 │
 ├── logging
 ├── metrics
 ├── tracing
 ├── message bus
 └── monitoring backend
```

---

## 23. Recommended Production Pattern

For a long-running GPU service:

1. Validate the NVIDIA driver with `nvidia-smi`.
2. Install GPUMetric into an isolated CPython 3.11 environment.
3. Initialize `GPUMetrics` once for the lifetime of the monitoring component.
4. Call `sample()` at the desired telemetry interval.
5. Treat `GPUStats` as an immutable snapshot of the current observation.
6. Export the values into the application's existing observability pipeline.
7. Log or surface initialization/sampling failures instead of silently discarding them.

A typical service topology is:

```text
                 GPU Worker
                     │
          ┌──────────┴──────────┐
          │                     │
      ML Runtime            GPUMetric
          │                     │
          │                  NVML/GPU
          │                     │
          └──────────┬──────────┘
                     ▼
                 Telemetry
                     │
          ┌──────────┼──────────┐
          ▼          ▼          ▼
        Logs      Metrics    Tracing
```

---

## 24. API Summary

### Import

```python
from gpumetric import GPUMetrics, GPUStats
```

### Initialize

```python
gpu = GPUMetrics(device_index=0)
```

### Preferred lifecycle

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

### Sample

```python
stats = gpu.sample()
```

### Read telemetry

```python
stats.temperature
stats.utilization
stats.memory_mib
stats.memory_delta_mib
```

---

## 25. Documentation Scope

This document covers the current operational interface:

- supported environment
- installation
- NVIDIA host validation
- Python API
- device selection
- sampling
- memory delta semantics
- context-manager lifecycle
- ML integration
- telemetry integration
- error handling
- native/FFI architecture
- local development
- testing
- clean installation validation
- troubleshooting

Implementation details that are not part of the public API should be treated as internal and may change independently of the application-facing interface.
