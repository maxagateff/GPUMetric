# GPUMetric Technical Documentation

This document describes the architecture, runtime requirements, installation process, Python API, native integration, testing strategy, and release validation workflow for GPUMetric.

GPUMetric is a lightweight NVIDIA GPU telemetry library for Linux and machine learning workloads. The native telemetry implementation uses NVIDIA Management Library (NVML), while the Python package exposes the functionality through a thin FFI layer.

---

## Documentation Scope

This document is focused on technical operation and development.

The primary supported environment is:

- Linux x86-64
- Ubuntu / Ubuntu Server
- NVIDIA GPU
- NVIDIA driver with NVML available
- CPython 3.11

The package currently targets a narrow runtime environment deliberately. Platform compatibility should be established by explicit builds and CI validation rather than inferred from Python package metadata alone.

---

# Architecture

GPUMetric is structured as a small telemetry stack:

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
              ┌──────────────────┐
              │ GPUMetric C Core │
              └────────┬─────────┘
                       │
                     C ABI
                       │
                       ▼
              ┌──────────────────┐
              │ Python FFI Layer │
              │     / ctypes     │
              └────────┬─────────┘
                       │
                       ▼
                 Python Application
```

The native layer owns low-level GPU interaction and NVML communication.

The Python layer exposes the application-facing API and converts the result into Python objects.

The application is responsible for deciding where the collected telemetry is stored or exported.

---

## Native Layer

The native implementation is responsible for:

1. initializing NVML;
2. obtaining access to the selected GPU;
3. collecting GPU telemetry;
4. maintaining sampling state;
5. calculating memory deltas;
6. returning the native result to the FFI layer.

The native library is compiled as a shared object and is consumed by the Python layer.

The native component is not coupled to an ML framework.

---

## Python Layer

The Python layer is the public application interface.

The public API is intentionally small:

```python
from gpumetric import GPUMetrics, GPUStats
```

The two principal objects are:

- `GPUMetrics` — telemetry collector and lifecycle owner.
- `GPUStats` — representation of one telemetry sample.

The current package does not require PyTorch, TensorFlow, JAX, or another ML framework.

---

# Requirements

## Operating System

The current supported target is:

```text
Linux x86-64
```

The primary development, test, and deployment environment is:

```text
Ubuntu / Ubuntu Server
```

Other Linux distributions may work if the NVIDIA driver, NVML runtime, Python environment, and system libraries are compatible, but Ubuntu is the reference environment.

---

## Python

The current binary distribution is built for:

```text
CPython 3.11
```

Check the interpreter:

```bash
python3.11 --version
```

Example:

```text
Python 3.11.x
```

The Python version used to run GPUMetric should match the compatibility of the installed wheel.

---

## NVIDIA GPU and Driver

The system must have an NVIDIA GPU with a working NVIDIA driver.

Verify the driver and GPU access:

```bash
nvidia-smi
```

Also verify that NVML is available through the NVIDIA runtime environment.

If `nvidia-smi` cannot communicate with the GPU, GPUMetric should not be expected to initialize successfully.

---

## System Build Requirements

A source build requires, at minimum:

- C compiler
- CMake
- NVIDIA driver / NVML development environment
- Python 3.11 for Python packaging and integration tests

On Ubuntu, use the repository's dependency setup instructions where available, or install the required compiler and CMake toolchain using the system package manager.

---

# Installation

## Install from PyPI

The standard user installation is:

```bash
python3.11 -m pip install gpumetric
```

Verify the installation:

```bash
python3.11 -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics); print(GPUStats)"
```

Verify that the package is importable:

```bash
python3.11 -c "import gpumetric; print(gpumetric.__file__)"
```

The published wheel contains the package components required by the Python API, so normal package installation does not require manually passing a `lib_path` to the application.

---

## Clean Virtual Environment

For reproducible installation testing, create an isolated environment:

```bash
python3.11 -m venv /tmp/gpumetric-test
source /tmp/gpumetric-test/bin/activate
```

Upgrade packaging tools:

```bash
python -m pip install --upgrade pip setuptools wheel
```

Install GPUMetric:

```bash
python -m pip install gpumetric
```

Verify:

```bash
python -c "from gpumetric import GPUMetrics, GPUStats; print('GPUMetric import OK')"
```

---

# Python API

## Package Imports

The public package entry point is:

```python
import gpumetric
```

The main classes are exposed from the package root:

```python
from gpumetric import GPUMetrics, GPUStats
```

Applications should prefer the package-level import rather than importing implementation modules directly.

---

# `GPUMetrics`

`GPUMetrics` is the primary GPU telemetry collector.

It owns the native monitoring lifecycle and exposes GPU sampling through `sample()`.

Basic construction:

```python
gpu = GPUMetrics(device_index=0)
```

Recommended lifecycle:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

The context-manager form is the preferred pattern because it makes resource ownership and teardown explicit.

---

## Constructor

Current constructor usage:

```python
GPUMetrics(device_index=0)
```

### `device_index`

`device_index` selects the NVIDIA GPU used by the collector.

Example:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

Inspect the device order with:

```bash
nvidia-smi -L
```

Example:

```text
GPU 0: NVIDIA ...
GPU 1: NVIDIA ...
```

Use the appropriate device index when creating the collector.

The current package is tested against the available NVIDIA device selected through the constructor. Multi-GPU behavior should be validated explicitly on the target host.

---

# `sample()`

The primary sampling method is:

```python
stats = gpu.sample()
```

The method returns a `GPUStats` object.

Example:

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
    print(stats)
```

Typical representation:

```text
GPUStats(temperature=42, utilization=0, memory_mib=397, memory_delta_mib=0)
```

The exact values depend on the current state of the GPU.

---

# `GPUStats`

`GPUStats` is the data representation of a single successful sample.

It does not initialize NVML and it does not itself perform GPU sampling.

Create one directly when unit testing the data model:

```python
from gpumetric import GPUStats

stats = GPUStats(
    temperature=42,
    utilization=0,
    memory_mib=397,
    memory_delta_mib=0,
)
```

The object exposes the following fields.

| Field | Meaning | Unit |
|---|---|---|
| `temperature` | GPU temperature | °C |
| `utilization` | GPU utilization | % |
| `memory_mib` | Current GPU memory usage | MiB |
| `memory_delta_mib` | Memory change relative to the previous sample | MiB |

---

## `temperature`

GPU temperature reported by NVML.

Example:

```python
temperature = stats.temperature
```

Typical values are expressed as integer degrees Celsius.

---

## `utilization`

GPU utilization reported by NVML.

Example:

```python
utilization = stats.utilization
```

For normal GPU utilization semantics, the value is expressed as a percentage from `0` to `100`.

---

## `memory_mib`

Current GPU memory usage represented in mebibytes.

Example:

```python
memory = stats.memory_mib
```

---

## `memory_delta_mib`

Observed change in GPU memory usage relative to the previous sample maintained by the collector.

Conceptually:

```text
memory_delta_mib = current_memory_mib - previous_memory_mib
```

Example sequence:

```text
Sample 1: 4096 MiB
Sample 2: 4128 MiB
Sample 3: 4160 MiB
Sample 4: 4300 MiB
```

Corresponding changes:

```text
Sample 2: +32 MiB
Sample 3: +32 MiB
Sample 4: +140 MiB
```

This is an observation between samples, not a CUDA memory profiler.

It does not identify:

- the CUDA allocation responsible for a change;
- the tensor responsible for a change;
- the process responsible for a change;
- the model component responsible for a change.

---

# Recommended Lifecycle

Initialize the telemetry collector once and sample it repeatedly:

```python
import time

from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    while True:
        stats = gpu.sample()

        print(
            f"Temperature: {stats.temperature} °C | "
            f"Utilization: {stats.utilization}% | "
            f"Memory: {stats.memory_mib} MiB | "
            f"Memory Delta: {stats.memory_delta_mib} MiB"
        )

        time.sleep(1)
```

The intended lifecycle is:

```text
GPUMetrics initialization
        │
        ├── sample()
        ├── sample()
        ├── sample()
        └── ...
        │
        ▼
context exit / resource release
```

Repeated construction and teardown for every single sample is generally a less appropriate usage pattern for a long-running monitor.

---

# Context Manager

`GPUMetrics` implements the context-manager interface.

Preferred usage:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

The purpose of this pattern is to establish an explicit resource boundary around native GPU monitoring.

Avoid treating `GPUStats` as the resource owner. `GPUStats` is only the data returned by sampling.

---

# Error Handling

Initialization and sampling can fail because the runtime environment is not valid or the requested GPU cannot be accessed.

Typical causes include:

- NVIDIA driver failure
- NVML initialization failure
- missing or inaccessible GPU
- invalid device index
- native library loading failure
- incompatible binary/runtime environment

Application code should treat collector initialization and sampling as operations that may fail.

For example:

```python
from gpumetric import GPUMetrics

try:
    with GPUMetrics(device_index=0) as gpu:
        stats = gpu.sample()
        print(stats)
except Exception as exc:
    print(f"GPU monitoring failed: {exc}")
```

The exact exception types exposed by the package should be treated as part of the API contract only after they are explicitly stabilized in the implementation.

---

# Integration with Machine Learning Workloads

GPUMetric is framework-agnostic.

It can be sampled from a PyTorch, TensorFlow, JAX, or custom inference/training process without requiring the telemetry library to depend on the framework.

Example with a training loop:

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
            f"gpu_mem={stats.memory_mib}MiB "
            f"gpu_mem_delta={stats.memory_delta_mib}MiB"
        )
```

This supports correlation between software-level workload metrics and hardware-level GPU state.

---

# Telemetry Export

GPUMetric is a collection primitive, not a complete observability system.

The application can transform `GPUStats` into its own metrics model:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()

    telemetry = {
        "gpu_temperature_celsius": stats.temperature,
        "gpu_utilization_percent": stats.utilization,
        "gpu_memory_mib": stats.memory_mib,
        "gpu_memory_delta_mib": stats.memory_delta_mib,
    }
```

That data can then be exported to:

- logs
- Prometheus
- OpenTelemetry
- Kafka
- time-series databases
- internal telemetry services
- custom monitoring systems

GPUMetric does not require any particular backend.

---

# Why Not `nvidia-smi`?

`nvidia-smi` is intended primarily for NVIDIA administration and diagnostics.

GPUMetric is designed for programmatic sampling inside a long-running process.

The distinction is architectural:

```text
CLI-oriented approach

Application
   │
   ├── spawn nvidia-smi
   ├── wait
   ├── read stdout
   ├── parse text
   └── repeat
```

versus:

```text
Library-oriented approach

Application
   │
   ▼
GPUMetrics
   │
   ▼
FFI
   │
   ▼
Native C
   │
   ▼
NVML
   │
   ▼
GPU
```

GPUMetric does not replace `nvidia-smi`; it addresses a different integration point.

---

# Native ABI and FFI

GPUMetric separates the native implementation from the Python interface through a native ABI.

Conceptually:

```text
C implementation
       │
       ▼
     C ABI
       │
       ▼
   Python FFI
       │
       ▼
 Python objects
```

The FFI layer is responsible for mapping C-compatible types into Python representations.

This boundary is important for correctness because native/Python interoperability depends on:

- integer width;
- signedness;
- structure layout;
- pointer types;
- function signatures;
- library loading;
- ABI compatibility.

When changing the native header or exported function signatures, corresponding FFI changes and integration tests should be updated together.

---

# Native Library

The native implementation is compiled into a shared library used by the Python runtime.

The native layer is responsible for:

- NVML initialization;
- GPU handle management;
- telemetry queries;
- sample state;
- memory delta calculation;
- native error propagation.

The Python package should remain independent of the implementation details of the native sampling mechanism.

---

# Development and Build

## Clone the Repository

```bash
git clone <repository-url>
cd GPUMetric
```

Use the repository URL corresponding to the canonical GPUMetric source repository.

---

## Python Development Environment

Create an isolated Python 3.11 environment:

```bash
python3.11 -m venv .venv
source .venv/bin/activate
```

Upgrade packaging tools:

```bash
python -m pip install --upgrade pip setuptools wheel
```

Install the development package and test tooling according to the repository's current `pyproject.toml` configuration.

---

## Native Build Requirements

The native component requires a C compiler and CMake.

Verify:

```bash
cmake --version
gcc --version
nvidia-smi
```

On Ubuntu, install the system development toolchain as required by the repository build configuration.

---

## CMake Build

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Inspect the resulting build artifacts:

```bash
find build -maxdepth 2 -type f -name '*gpumetric*' -o -name '*.so'
```

The exact build output path is controlled by the current CMake configuration.

---

# Testing

The test suite should be run from the repository root.

Install `pytest`:

```bash
python3.11 -m pip install pytest
```

Run the tests:

```bash
python3.11 -m pytest
```

Run in verbose mode:

```bash
python3.11 -m pytest -v
```

The `-v` flag asks pytest to print each individual test name and result rather than only the compact dot representation.

Example successful output:

```text
============================= test session starts =============================
platform linux -- Python 3.11.x
collected 10 items

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

============================== 10 passed ==============================
```

The exact number of tests changes as coverage evolves.

---

## Test Categories

The test suite contains two conceptual categories.

### Unit Tests

Unit-level tests cover Python behavior that does not require an active GPU, such as:

- package imports;
- public API exposure;
- `GPUStats` construction;
- data type behavior.

### Integration Tests

GPU integration tests exercise the complete runtime path:

```text
Python API
    │
    ▼
Python FFI
    │
    ▼
Native C library
    │
    ▼
NVML
    │
    ▼
NVIDIA GPU
```

Examples include:

- collector initialization;
- actual sampling;
- metric value validation;
- repeated sampling;
- context-manager behavior;
- collector reinitialization.

Integration tests require a working NVIDIA GPU environment.

---

# Release Validation

A release should validate both the source repository and the packaged artifact.

The minimum release sequence should be:

```text
source changes
      │
      ▼
unit tests
      │
      ▼
integration tests
      │
      ▼
package build
      │
      ▼
distribution validation
      │
      ▼
clean installation
      │
      ▼
clean runtime smoke test
```

---

## Build Python Distributions

Build the distribution using the repository packaging configuration:

```bash
python3.11 -m build
```

Inspect the generated files:

```bash
ls -lh dist/
```

The release artifacts should include the distributions intended for the supported environment.

---

## Validate Distribution Metadata

Install `twine`:

```bash
python3.11 -m pip install twine
```

Validate the artifacts:

```bash
twine check dist/*
```

The command should complete without metadata errors.

---

## Clean Wheel Installation Test

Create a fresh environment:

```bash
python3.11 -m venv /tmp/gpumetric-release-test
source /tmp/gpumetric-release-test/bin/activate
```

Upgrade packaging tools:

```bash
python -m pip install --upgrade pip setuptools wheel
```

Install the wheel produced by the build:

```bash
python -m pip install dist/*.whl
```

Verify the public API:

```bash
python -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics); print(GPUStats)"
```

Run the tests against the installed package when the repository's test configuration supports that workflow.

---

# PyPI Validation

After publishing a package, test the artifact from PyPI rather than relying only on the source checkout.

Create a clean environment:

```bash
python3.11 -m venv /tmp/gpumetric-pypi-test
source /tmp/gpumetric-pypi-test/bin/activate
```

Install directly from PyPI:

```bash
python -m pip install gpumetric
```

Verify the public API:

```bash
python -c "from gpumetric import GPUMetrics, GPUStats; print('GPUMetric import OK')"
```

Verify GPU runtime behavior:

```bash
python - <<'PY'
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
    print(stats)
PY
```

This verifies the actual user path:

```text
PyPI
  │
  ▼
pip
  │
  ▼
Python environment
  │
  ▼
GPUMetrics
  │
  ▼
FFI / native implementation
  │
  ▼
NVML
  │
  ▼
GPU
```

---

# Repository Structure

The exact repository layout may evolve, but the logical structure is:

```text
GPUMetric/
├── gpumetric/
│   ├── __init__.py
│   ├── metrics.py
│   └── _ffi.py
│
├── src/
│   └── native / build sources
│
├── tests/
│   ├── test_context_manager.py
│   ├── test_gpu_metrics.py
│   ├── test_gpu_metrics_reinitialization.py
│   ├── test_gpu_stats.py
│   └── test_import.py
│
├── scripts/
│   └── build / setup helpers
│
├── CMakeLists.txt
├── pyproject.toml
├── README.md
└── docs/
    └── README.md
```

The public Python package is the `gpumetric` namespace.

The implementation details under `src/` and the native build system should not be treated as stable application-facing APIs.

---

# Limitations

## Platform Scope

The currently validated target is Linux x86-64 on Ubuntu with Python 3.11.

Windows, macOS, ARM64 Linux, and other non-reference environments are not current release targets unless separately built and tested.

---

## NVIDIA Dependency

GPUMetric is specifically implemented against NVIDIA NVML.

It is not a vendor-neutral GPU telemetry abstraction.

AMD and Intel GPU telemetry are outside the current scope.

---

## Telemetry Scope

The current public data model is intentionally small:

```text
GPUStats
├── temperature
├── utilization
├── memory_mib
└── memory_delta_mib
```

The library does not currently provide a full CUDA profiler, kernel profiler, allocation profiler, or process-level memory attribution system.

---

## Observability Scope

GPUMetric does not provide:

- metric persistence;
- time-series storage;
- dashboards;
- alerting;
- message transport;
- Prometheus server functionality;
- OpenTelemetry collector functionality.

Those concerns belong to the consuming application or infrastructure layer.

---

# Engineering Guidelines

## Preserve the API Boundary

Changes to the native ABI should be treated as API changes.

When changing native function signatures, structures, or exported symbols, update together:

1. the native header;
2. the native implementation;
3. the Python FFI declarations;
4. the Python API layer;
5. integration tests;
6. package/release tests.

---

## Add Tests for Public Behavior

Every public behavior should have a corresponding test.

For example, changes to `sample()` should cover:

- successful sampling;
- field types;
- reasonable ranges;
- repeated sampling;
- lifecycle behavior;
- failure behavior where applicable.

---

## Validate the Installed Artifact

A passing source-tree test is not sufficient for a release.

The packaged artifact must also be installed into a clean environment and tested through the same public API used by an external consumer.

This prevents failures such as:

```text
source tree works
      │
      ├── missing package data
      ├── incorrect wheel tag
      ├── missing native library
      ├── broken import path
      └── incompatible Python version
      │
      ▼
installed package fails
```

---

# Example: Minimal Application

```python
from gpumetric import GPUMetrics


def main() -> None:
    with GPUMetrics(device_index=0) as gpu:
        stats = gpu.sample()

        print(f"Temperature: {stats.temperature} °C")
        print(f"Utilization: {stats.utilization}%")
        print(f"Memory: {stats.memory_mib} MiB")
        print(f"Memory delta: {stats.memory_delta_mib} MiB")


if __name__ == "__main__":
    main()
```

---

# Example: Continuous Monitoring

```python
import time

from gpumetric import GPUMetrics


def main() -> None:
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


if __name__ == "__main__":
    main()
```

---

# Example: Telemetry Transformation

Applications can translate the result into their own internal telemetry schema:

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()

    telemetry = {
        "gpu_temperature_celsius": stats.temperature,
        "gpu_utilization_percent": stats.utilization,
        "gpu_memory_mib": stats.memory_mib,
        "gpu_memory_delta_mib": stats.memory_delta_mib,
    }
```

This keeps the GPU collector independent from the rest of the observability stack.

---

# Current Technical Direction

GPUMetric is intended to remain a small, embeddable GPU telemetry component.

The core architectural boundary is:

```text
GPU / NVML
     │
     ▼
Native GPUMetric Core
     │
     ▼
FFI Boundary
     │
     ▼
Python API
     │
     ▼
ML / Infrastructure Application
```

The library should remain focused on collecting GPU state rather than absorbing the responsibilities of a complete monitoring platform.

---

# Troubleshooting

## `pip` cannot find a compatible `gpumetric` distribution

First verify the Python runtime:

```bash
python3 --version
```

For the currently published binary distribution, use Python 3.11:

```bash
python3.11 --version
```

Then inspect the available package artifacts:

```bash
python3.11 -m pip index versions gpumetric
```

If the package exists on PyPI but the resolver reports no compatible version, the installed interpreter or platform may not match the wheel compatibility tags.

---

## Import succeeds but GPU initialization fails

Verify the host GPU environment:

```bash
nvidia-smi
```

If `nvidia-smi` fails, fix the NVIDIA driver/runtime before investigating the Python package.

Then verify the installed package:

```bash
python3.11 -c "from gpumetric import GPUMetrics; print(GPUMetrics)"
```

---

## `sample()` fails

Check:

1. NVIDIA driver availability;
2. NVML availability;
3. selected `device_index`;
4. Python version;
5. architecture compatibility;
6. native library loading;
7. whether the failure reproduces in a clean environment.

Use a minimal reproduction:

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    print(gpu.sample())
```

---

# API Summary

The current public usage can be reduced to:

```python
from gpumetric import GPUMetrics, GPUStats

with GPUMetrics(device_index=0) as gpu:
    stats: GPUStats = gpu.sample()
```

Then access:

```python
stats.temperature
stats.utilization
stats.memory_mib
stats.memory_delta_mib
```

This is the primary application-facing interface.
