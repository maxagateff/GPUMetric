# GPUMetric

Lightweight NVIDIA GPU telemetry library for Linux and machine learning workloads.

GPUMetric is a small native GPU telemetry library built around **NVIDIA NVML**. The core is implemented in C and exposed to Python through a thin `ctypes` FFI layer.

It provides programmatic access to selected GPU metrics without repeatedly spawning `nvidia-smi` processes.

> **Status: Pre-release / Active Development**
>
> The project is currently under active development. The API, ABI, package structure, and build workflow may change before the first stable release.

---

## Overview

GPUMetric is designed for applications that need to collect GPU telemetry directly from inside a long-running process.

The current implementation exposes:

* GPU temperature
* GPU utilization
* GPU memory usage
* GPU memory usage delta between consecutive samples
* explicit C API return codes
* a C ABI suitable for language bindings
* Python bindings through `ctypes`
* direct communication with NVIDIA NVML
* fixed-size internal sampling state without dynamic allocation in the sampling path

The current implementation supports **one NVIDIA GPU using device index `0`**.

Multi-GPU support and explicit device selection are planned.

---

## Why GPUMetric?

Machine learning applications already expose application-level metrics such as:

* loss
* learning rate
* throughput
* batch size
* training step
* optimizer step
* inference latency

At the same time, the underlying GPU has its own runtime state:

* utilization
* temperature
* memory consumption
* memory changes over time

GPUMetric is designed to make these two layers easy to correlate.

For example:

```text
ML / DL Application
│
├── training step
├── loss
├── learning rate
├── throughput
│
└── GPU Telemetry
    ├── utilization
    ├── temperature
    ├── memory usage
    └── memory delta
```

This is useful when investigating:

* GPU under-utilization
* unexpected memory growth
* memory pressure
* long-running training jobs
* inference workloads
* GPU behavior between training steps
* changes in GPU state over time

GPUMetric deliberately does **not** define where the telemetry should be stored. The application decides whether the values should be sent to logs, Prometheus, OpenTelemetry, a message broker, or another telemetry backend.

---

# Architecture

GPUMetric keeps the telemetry path intentionally small:

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
                ML / DL Process
```

The native C layer is responsible for:

1. initializing NVML;
2. accessing the NVIDIA device;
3. collecting GPU telemetry;
4. maintaining sampling state;
5. calculating memory deltas;
6. returning explicit error codes.

The Python layer is responsible for:

1. loading `libgpumetric.so`;
2. mapping C types to `ctypes`;
3. exposing the C ABI to Python;
4. returning the collected telemetry to the application.

This separation keeps the low-level GPU telemetry implementation independent from the application consuming the data.

---

# Requirements

GPUMetric currently targets **Linux systems with NVIDIA GPUs**.

You need:

* NVIDIA GPU
* NVIDIA driver
* NVIDIA NVML
* C compiler
* CMake
* Python 3

The primary development and testing environment is **Ubuntu Server**.

Other Linux distributions may work if they provide a compatible NVIDIA driver, NVML installation, compiler, CMake, and Python runtime.

---

# Verify NVIDIA GPU Access

Before building GPUMetric, verify that the NVIDIA driver can access the GPU:

```bash
nvidia-smi
```

The command should successfully display the installed GPU.

For example:

```text
+-----------------------------------------------------------------------------+
| NVIDIA-SMI ...                                                             |
+-----------------------------------------------------------------------------+
| GPU  Name ...                                                              |
+-----------------------------------------------------------------------------+
```

If `nvidia-smi` cannot access the GPU, GPUMetric will not be able to initialize NVML successfully.

---

# Installation

GPUMetric is currently distributed as a **source-based development project**.

There is no PyPI package yet.

The current workflow is:

```text
Source
  │
  ▼
CMake
  │
  ▼
libgpumetric.so
  │
  ▼
Python ctypes
  │
  ▼
Application
```

Python package distribution and wheel builds are planned for a future stable release.

---

## Install Dependencies

The repository provides a dependency installation script for Ubuntu-based systems:

```bash
chmod +x scripts/install_deps.sh
./scripts/install_deps.sh
```

Verify CMake:

```bash
cmake --version
```

Verify the NVIDIA driver:

```bash
nvidia-smi
```

---

# Building

## Build Using the Repository Script

The repository contains a helper script for the initial CMake build:

```bash
chmod +x scripts/build/initial_run_of_the_cmake_build.sh
./scripts/build/initial_run_of_the_cmake_build.sh
```

After a successful build, the native library should be available at:

```text
build/libgpumetric.so
```

Verify it:

```bash
ls -lh build/libgpumetric.so
```

---

## Manual CMake Build

GPUMetric can also be built directly using CMake.

From the repository root:

```bash
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release

cd ..
```

The resulting shared library should be:

```text
build/
└── libgpumetric.so
```

---

# Testing

GPUMetric currently includes a GPU-dependent integration test.

The test validates the complete telemetry path:

```text
Python
  │
  ▼
ctypes
  │
  ▼
C ABI
  │
  ▼
NVML
  │
  ▼
NVIDIA GPU
```

First verify the GPU:

```bash
nvidia-smi
```

Then run:

```bash
python3 tests/test_run.py
```

The test loads:

```text
build/libgpumetric.so
```

and periodically collects GPU metrics.

If NVML initialization or GPU sampling fails, GPUMetric returns the corresponding error code.

> The current integration test requires a Linux system with an accessible NVIDIA GPU.

---

# Python API

The current development API is exposed through the Python FFI module:

```python
from src.python_ffi.FFI import GPUMetrics
```

Create a `GPUMetrics` instance by providing the path to the native shared library:

```python
from src.python_ffi.FFI import GPUMetrics

gpu_metrics = GPUMetrics(
    lib_path="build/libgpumetric.so"
)
```

Collect a sample:

```python
ret, stats = gpu_metrics.samples()
```

Always check the return code before using the metrics:

```python
if ret != 0:
    print(f"GPU metric collection failed: {ret}")
else:
    print(
        f"Temperature: {stats.temp}C | "
        f"Utilization: {stats.util}% | "
        f"Memory: {stats.mem_mb}MiB | "
        f"Memory Delta: {stats.delta_mb}MiB"
    )
```

Example output:

```text
Temperature: 67C | Utilization: 94% | Memory: 6124MiB | Memory Delta: 128MiB
```

> The current import path and `lib_path` interface are part of the development API and may change before the first stable release.

---

# Sampling API

The primary sampling operation is:

```python
ret, stats = gpu_metrics.samples()
```

The return value consists of two objects:

```text
ret
 │
 └── operation result code

stats
 │
 ├── temp
 ├── util
 ├── mem_mb
 └── delta_mb
```

`ret` indicates whether the operation succeeded.

A successful operation returns:

```text
0
```

Example:

```python
ret, stats = gpu_metrics.samples()

if ret != 0:
    print(f"GPUMetric error: {ret}")
    return

print(f"GPU utilization: {stats.util}%")
```

Applications should not consume `stats` as valid telemetry when `ret != 0`.

---

# Available Metrics

The current API exposes four GPU metrics.

| Field      | Description                                          | Unit |
| ---------- | ---------------------------------------------------- | ---- |
| `temp`     | GPU temperature                                      | °C   |
| `util`     | GPU utilization                                      | %    |
| `mem_mb`   | Current GPU memory usage                             | MiB  |
| `delta_mb` | Change in GPU memory usage since the previous sample | MiB  |

Example:

```python
ret, stats = gpu_metrics.samples()

if ret == 0:
    print(stats.temp)
    print(stats.util)
    print(stats.mem_mb)
    print(stats.delta_mb)
```

---

# Memory Delta

`delta_mb` represents the difference in reported GPU memory usage between two consecutive successful samples.

For example:

```text
Sample 1: 4096 MiB
Sample 2: 4128 MiB
Sample 3: 4160 MiB
Sample 4: 4300 MiB
```

The corresponding deltas are:

```text
Sample 2: +32 MiB
Sample 3: +32 MiB
Sample 4: +140 MiB
```

Conceptually:

```text
delta_mb = current_memory - previous_memory
```

This makes `delta_mb` useful for observing changes in GPU memory consumption over time.

For example:

```text
Training
│
├── Step 1000 → 4096 MiB
├── Step 1100 → 4128 MiB
├── Step 1200 → 4160 MiB
└── Step 1300 → 4300 MiB
                         │
                         └── increasing memory usage
```

## Important

`delta_mb` describes the **change observed between samples**.

It does not identify:

* which CUDA allocation changed;
* which tensor caused the change;
* which model component allocated memory;
* which process caused the change.

It is a telemetry signal, not a CUDA memory profiler.

---

# Machine Learning Integration

GPUMetric can be sampled directly from a training or inference loop.

For example:

```python
from src.python_ffi.FFI import GPUMetrics

gpu_metrics = GPUMetrics(
    lib_path="build/libgpumetric.so"
)

for step, batch in enumerate(dataloader):

    loss = train_step(batch)

    ret, stats = gpu_metrics.samples()

    if ret == 0:
        print(
            f"step={step} "
            f"loss={loss.item():.4f} "
            f"gpu_util={stats.util}% "
            f"gpu_temp={stats.temp}C "
            f"gpu_mem={stats.mem_mb}MiB "
            f"gpu_mem_delta={stats.delta_mb}MiB"
        )
    else:
        print(f"GPUMetric error: {ret}")
```

This allows application-level metrics and hardware telemetry to be observed together:

```text
Training Step
│
├── loss
├── learning rate
├── throughput
├── batch size
│
└── GPU
    ├── utilization
    ├── temperature
    ├── memory
    └── memory delta
```

This can be especially useful for diagnosing relationships such as:

```text
low GPU utilization
        │
        ├── small batch size
        ├── CPU data-loading bottleneck
        ├── synchronization overhead
        └── model-level bottleneck
```

or:

```text
GPU memory
    │
    ├── stable
    │
    ├── increasing
    │
    └── sudden change
```

---

# Telemetry Backends

GPUMetric intentionally does not contain a monitoring backend.

The application decides how the collected values should be exported.

Possible destinations include:

* application logs
* Prometheus
* OpenTelemetry
* Kafka
* custom telemetry services
* internal ML infrastructure
* time-series databases

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

GPUMetric only owns the first part of this pipeline.

Storage, aggregation, visualization, alerting, and transport remain application-level concerns.

---

# Why Not `nvidia-smi`?

`nvidia-smi` is an excellent NVIDIA diagnostic and administration tool.

GPUMetric targets a different use case.

An application repeatedly using `nvidia-smi` may need to:

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

GPUMetric instead provides:

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

This avoids repeatedly creating an external `nvidia-smi` process and parsing its CLI output.

## GPUMetric does not replace `nvidia-smi`

`nvidia-smi` remains useful for:

* driver diagnostics;
* system administration;
* GPU inspection;
* manual troubleshooting;
* validating NVIDIA GPU access.

GPUMetric is intended for **programmatic, repeated telemetry collection inside an application**.

---

# Error Handling

The native C API uses explicit return codes for expected failures.

| Code | Constant                   | Description                             |
| ---: | -------------------------- | --------------------------------------- |
|  `0` | `GPU_METRIC_SUCCESS`       | Operation completed successfully        |
| `-1` | `GPU_METRIC_ERR_NVML`      | NVML initialization or operation failed |
| `-2` | `GPU_METRIC_ERR_NO_DEVICE` | No NVIDIA GPU was detected              |
| `-3` | `GPU_METRIC_ERR_DEVICE`    | GPU interaction or sampling failed      |
| `-4` | `GPU_METRIC_ERR_ARGUMENT`  | Invalid argument                        |

Example:

```python
ret, stats = gpu_metrics.samples()

if ret != 0:
    print(f"GPUMetric error: {ret}")
    return

print(f"GPU utilization: {stats.util}%")
```

The library is designed to report expected operational failures through return codes rather than terminating the host application.

---

# C / Python ABI

GPUMetric uses an explicit C ABI between the native implementation and the Python FFI layer.

The Python layer maps C integer types to the corresponding `ctypes` types.

For example:

```text
C                  Python ctypes
────────────────────────────────────
uint32_t      →    ctypes.c_uint32
uint64_t      →    ctypes.c_uint64
int64_t       →    ctypes.c_int64
```

Explicit ABI mapping is important for avoiding problems related to:

* integer width;
* truncation;
* structure layout;
* ABI compatibility;
* FFI correctness.

The architecture is therefore:

```text
C implementation
       │
       ▼
    C ABI
       │
       ▼
 ctypes FFI
       │
       ▼
 Python API
```

---

# Native Library

The native build produces:

```text
libgpumetric.so
```

This is the primary C shared library consumed by the Python FFI layer.

The native component is intentionally independent from Python.

Conceptually:

```text
libgpumetric.so
│
├── NVML initialization
├── GPU discovery
├── GPU sampling
├── memory delta calculation
└── C error handling
```

Python is only one possible consumer of the C ABI.

The same native library can potentially be consumed by other languages with compatible FFI support.

---

# Project Structure

The current repository is organized as follows:

```text
GPUMetric/
├── src/
│   ├── gpumetric_core/
│   │   ├── gpu_metric.c
│   │   ├── gpu_metric.h
│   │   └── logger.h
│   │
│   └── python_ffi/
│       └── FFI.py
│
├── tests/
│   └── test_run.py
│
├── scripts/
│   ├── install_deps.sh
│   └── build/
│       └── initial_run_of_the_cmake_build.sh
│
├── CMakeLists.txt
├── LICENSE
├── .gitignore
└── README.md
```

---

## `src/gpumetric_core/`

Contains the native C implementation.

Responsibilities include:

* NVML interaction;
* GPU telemetry collection;
* sampling state;
* memory delta calculation;
* C API;
* error reporting.

---

## `src/python_ffi/`

Contains the Python FFI layer implemented with `ctypes`.

Responsibilities include:

* loading `libgpumetric.so`;
* defining C-compatible types;
* mapping native functions;
* exposing the Python API.

---

## `tests/`

Contains GPU-dependent integration tests.

The current test validates:

```text
Python
  ↓
ctypes
  ↓
C ABI
  ↓
NVML
  ↓
NVIDIA GPU
```

---

## `scripts/`

Contains development and build helper scripts.

Currently:

```text
scripts/
├── install_deps.sh
└── build/
    └── initial_run_of_the_cmake_build.sh
```

---

# Technical Design

GPUMetric is intentionally designed as a small native telemetry component.

The sampling path is kept simple:

```text
Python
  │
  ▼
ctypes
  │
  ▼
C ABI
  │
  ▼
NVML
  │
  ▼
GPU
```

The current C implementation maintains sampling history using fixed-size internal state rather than dynamically allocating memory for every sample.

This is intended to make repeated sampling predictable for long-running workloads.

---

# Limitations

## Single GPU

The current implementation uses:

```text
GPU index: 0
```

Explicit GPU selection and multi-GPU support are planned.

---

## Linux Only

The primary target is:

```text
Linux Server
    │
    ├── NVIDIA Driver
    ├── NVML
    ├── C / CMake
    ├── Python 3
    └── GPUMetric
```

Other Linux distributions may work if the required runtime dependencies are available.

---

## NVIDIA Dependency

GPUMetric directly depends on NVIDIA NVML.

Correct operation therefore requires:

* an NVIDIA GPU;
* a compatible NVIDIA driver;
* an accessible NVML library;
* a compatible Linux runtime.

GPUMetric is not a vendor-neutral GPU telemetry abstraction at this stage.

---

## No PyPI Package Yet

The project currently requires building the native library from source.

The intended future workflow is:

```text
Source
   │
   ▼
Wheel Build
   │
   ▼
PyPI
   │
   ▼
pip install gpumetric
```

Until then, the development API should be considered provisional.

---

# Development Philosophy

GPUMetric is intentionally small.

It is **not** intended to become:

* a complete monitoring platform;
* a replacement for Prometheus;
* a replacement for OpenTelemetry;
* a Grafana exporter;
* a GPU scheduler;
* a GPU management daemon;
* a general-purpose observability platform.

The goal is to provide a focused native telemetry component:

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
 ├── logs
 ├── metrics
 ├── Prometheus
 ├── OpenTelemetry
 └── custom telemetry backend
```

This keeps GPU state collection separate from the systems responsible for:

* storage;
* aggregation;
* visualization;
* alerting;
* transport;
* observability.

---

# Roadmap

The following capabilities are planned for future development:

* [ ] Multi-GPU support
* [ ] Explicit GPU/device selection
* [ ] Stable Python API
* [ ] Python package distribution
* [ ] Wheel builds
* [ ] PyPI distribution
* [ ] Improved API documentation
* [ ] More GPU telemetry
* [ ] Expanded test coverage
* [ ] Stable release and versioning

The exact roadmap may change during development.

---

# Production Considerations

GPUMetric is currently **pre-release software**.

It is suitable for:

* experimentation;
* development;
* internal ML infrastructure;
* GPU telemetry testing;
* evaluating the architecture;
* prototyping observability integrations.

For production systems, pin a tested repository revision or release rather than depending on an unpinned development branch.

Example:

```bash
git checkout <tested-revision>
```

This is particularly important while the Python API and native ABI are still evolving.

---

# Example End-to-End Usage

Build the library:

```bash
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release

cd ..
```

Verify the GPU:

```bash
nvidia-smi
```

Run the integration test:

```bash
python3 tests/test_run.py
```

Use GPUMetric from Python:

```python
from src.python_ffi.FFI import GPUMetrics


gpu_metrics = GPUMetrics(
    lib_path="build/libgpumetric.so"
)


ret, stats = gpu_metrics.samples()


if ret != 0:
    raise RuntimeError(
        f"GPUMetric sampling failed with error code {ret}"
    )


print(
    f"Temperature: {stats.temp}C | "
    f"Utilization: {stats.util}% | "
    f"Memory: {stats.mem_mb}MiB | "
    f"Memory Delta: {stats.delta_mb}MiB"
)
```

Example output:

```text
Temperature: 43C | Utilization: 0% | Memory: 397MiB | Memory Delta: 0MiB
```

---

# License

GPUMetric is licensed under the **GNU General Public License v3.0 (GPLv3)**.

Copyright © 2026 Max Agatev

You are free to:

* use the software;
* study the source code;
* modify the software;
* redistribute the software;

subject to the terms and conditions of the GNU GPLv3.

The complete license text is available in the repository:

```text
LICENSE
```

The software is provided **"AS IS"**, without warranty of any kind, express or implied.

For the complete terms and conditions, see the `LICENSE` file.

---

# Third-Party Dependencies

GPUMetric relies on NVIDIA's **NVIDIA Management Library (NVML)** provided by the NVIDIA driver/runtime environment.

NVML is an NVIDIA component and is not part of GPUMetric itself.

Users are responsible for complying with the applicable NVIDIA software and driver licensing terms when deploying GPUMetric.

---

# Project Status

GPUMetric is currently under active development.

The project currently provides a working native C telemetry core, a Python `ctypes` FFI layer, CMake-based builds, and GPU-dependent integration testing.

The current architecture is intentionally minimal:

```text
                NVIDIA GPU
                    │
                    ▼
                   NVML
                    │
                    ▼
             ┌─────────────┐
             │  GPUMetric  │
             │   C Core    │
             └──────┬──────┘
                    │
                  C ABI
                    │
                    ▼
             Python ctypes
                    │
                    ▼
             ML / DL Workload
                    │
          ┌─────────┼─────────┐
          ▼         ▼         ▼
        Logs    Prometheus  OpenTelemetry
```

The long-term goal is to keep the core lightweight, predictable, and suitable for embedding into machine learning infrastructure and long-running GPU workloads.