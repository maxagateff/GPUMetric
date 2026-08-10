# GPUMetric

> **Status: Active development**
>
> GPUMetric is currently under active development. The API, project structure, and build process may change before the first stable release.

**GPUMetric** is a lightweight GPU telemetry library built around **NVIDIA NVML**.

The core implementation is written in C, with a Python interface exposed through FFI using `ctypes`. The project is designed primarily for **headless Linux servers with NVIDIA GPUs** and long-running machine learning, deep learning, training, and inference workloads.

GPUMetric provides direct access to selected GPU hardware metrics without repeatedly spawning the external `nvidia-smi` process.

---

## Features

The current implementation provides:

* GPU temperature
* GPU utilization
* GPU memory usage
* GPU memory usage delta between samples
* Explicit C API error codes
* Python bindings through `ctypes`
* Static sampling state without dynamic allocation during the sampling path
* Direct communication with NVIDIA NVML
* C ABI suitable for integration with higher-level applications

The current implementation supports **one GPU** and uses GPU index `0`.

Multi-GPU support and explicit GPU selection are planned for future releases.

---

# Architecture

GPUMetric intentionally keeps the telemetry path small:

```text
                    NVIDIA GPU
                         │
                         ▼
                  NVIDIA NVML
                         │
                         ▼
              ┌──────────────────┐
              │   GPUMetric C    │
              │     library      │
              └────────┬─────────┘
                       │
                     C ABI
                       │
                       ▼
              ┌──────────────────┐
              │ Python FFI       │
              │     ctypes       │
              └────────┬─────────┘
                       │
                       ▼
                 ML / DL workload
```

The C layer is responsible for:

* initializing NVML;
* interacting with the NVIDIA GPU;
* collecting hardware metrics;
* maintaining sampling state;
* calculating memory deltas;
* returning explicit error codes.

The Python layer provides access to the C ABI from Python applications.

This separation keeps the low-level telemetry path independent from the application consuming the metrics.

---

# Why GPUMetric?

Machine learning workloads expose model-level metrics such as:

* loss;
* learning rate;
* throughput;
* training step;
* optimizer state.

At the same time, the workload is running on physical GPU hardware with its own state:

* utilization;
* temperature;
* memory consumption;
* memory changes over time.

GPUMetric is intended to make these two categories of information easy to correlate.

For example:

```text
Training Step
│
├── loss
├── learning rate
├── throughput
├── optimizer step
│
└── GPU telemetry
    ├── utilization
    ├── temperature
    ├── memory usage
    └── memory delta
```

This can be useful when investigating:

* unexpected GPU memory growth;
* workload behavior;
* GPU under-utilization;
* memory pressure;
* long-running training jobs;
* inference workloads;
* changes in GPU state between training steps.

GPUMetric does **not** define where telemetry should be stored.

The application can forward the collected metrics to an existing logging, monitoring, or observability system.

---

# Technical Characteristics

## Low-overhead sampling

GPUMetric is designed to keep the sampling path small and predictable.

The current C implementation maintains sampling history using a fixed-size internal structure rather than performing dynamic memory allocation during each sample.

This is intended to make repeated sampling suitable for long-running workloads.

---

## Direct NVML access

GPUMetric communicates with NVIDIA GPUs through **NVIDIA Management Library (NVML)**.

The telemetry path is:

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
 NVIDIA Driver
        │
        ▼
       GPU
```

GPUMetric does not execute `nvidia-smi` for every sample.

---

## Single-GPU support

The current implementation monitors:

```text
GPU index: 0
```

Multi-GPU support and explicit GPU selection are planned.

---

## Explicit C/Python ABI mapping

The Python FFI layer explicitly maps C integer types to corresponding `ctypes` types.

For example:

```text
uint32_t  → ctypes.c_uint32
uint64_t  → ctypes.c_uint64
int64_t   → ctypes.c_int64
```

Explicit ABI mapping helps avoid problems related to:

* integer sizes;
* integer truncation;
* memory layout;
* C/Python FFI compatibility.

---

# Requirements

GPUMetric currently targets **Linux systems with NVIDIA GPUs**.

You need:

1. An NVIDIA GPU
2. A working NVIDIA driver
3. NVIDIA NVML
4. A C compiler
5. CMake
6. Python 3

The primary development and testing environment is Ubuntu Server.

Other Linux distributions may work if the required NVIDIA, compiler, CMake, and Python dependencies are available.

---

## Verify the NVIDIA driver

Before building GPUMetric, verify that the NVIDIA driver can access the GPU:

```bash
nvidia-smi
```

The command should successfully report the installed NVIDIA GPU.

If `nvidia-smi` cannot access the GPU, GPUMetric will not be able to initialize NVML successfully.

---

# Installation

> **Current status:** Python package installation through PyPI is not available yet.

The current development workflow builds the native shared library locally and loads it through the Python FFI layer.

Python package distribution is planned for a future release.

---

# Installing Dependencies

On Ubuntu Server, the repository provides a dependency installation script:

```bash
chmod +x scripts/install_deps.sh
./scripts/install_deps.sh
```

After installing the dependencies, verify CMake:

```bash
cmake --version
```

Then verify NVIDIA GPU access:

```bash
nvidia-smi
```

---

# Building

## Using the Build Script

The repository provides a build script for the initial CMake build:

```bash
chmod +x scripts/build/initial_run_of_the_cmake_build.sh
./scripts/build/initial_run_of_the_cmake_build.sh
```

After a successful build, the shared library should be available at:

```text
build/libgpumetric.so
```

Verify the library:

```bash
ls -lh build/libgpumetric.so
```

---

## Manual CMake Build

GPUMetric can also be built directly with CMake.

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

GPUMetric currently includes a GPU-dependent integration test that verifies the complete telemetry path:

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

First verify that the NVIDIA driver is working:

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

If NVML initialization or GPU sampling fails, the library returns the corresponding error code.

> The current test is intended to run on a Linux machine with an accessible NVIDIA GPU.

---

# Usage

After successfully building the library, it can be loaded from Python using the current FFI interface.

```python
from src.python_ffi.FFI import GPUMetrics

gpu_metrics = GPUMetrics(
    lib_path="build/libgpumetric.so"
)

ret, stats = gpu_metrics.samples()

if ret == 0:
    print(
        f"Temperature: {stats.temp}C | "
        f"Util: {stats.util}% | "
        f"Memory: {stats.mem_mb}MiB | "
        f"Delta Memory: {stats.delta_mb}MiB"
    )
else:
    print(f"GPU metric collection failed: {ret}")
```

> **Note:** The current import path and `lib_path` usage reflect the development version of the project. The Python API is expected to be improved before stable package distribution.

---

# Sampling API

The current sampling interface returns:

```python
ret, stats = gpu_metrics.samples()
```

where:

* `ret` is the operation result code;
* `stats` contains the collected GPU metrics.

Always check `ret` before consuming the contents of `stats`.

Example:

```python
ret, stats = gpu_metrics.samples()

if ret != 0:
    print(f"GPUMetric error: {ret}")
    return

print(f"GPU utilization: {stats.util}%")
```

---

# Available Metrics

The current implementation provides:

| Field      | Description                                          | Unit |
| ---------- | ---------------------------------------------------- | ---- |
| `temp`     | GPU temperature                                      | °C   |
| `util`     | GPU utilization                                      | %    |
| `mem_mb`   | Current GPU memory usage                             | MiB  |
| `delta_mb` | Change in GPU memory usage since the previous sample | MiB  |

Example:

```text
Temperature: 67C
Util: 94%
Memory: 6124MiB
Delta Memory: 128MiB
```

---

# Memory Delta

`delta_mb` represents the change in GPU memory usage between consecutive samples.

For example, if the samples are:

```text
4096 MiB
4128 MiB
4160 MiB
4300 MiB
```

the corresponding deltas are:

```text
+32 MiB
+32 MiB
+140 MiB
```

This can be useful for identifying changes in GPU memory consumption during long-running workloads.

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

The metric represents the change observed between samples. It does not attempt to identify which CUDA allocation, tensor, model component, or process caused the change.

---

# Using GPUMetric with Machine Learning

GPUMetric can be called from a training loop or another long-running GPU workload.

Example:

```python
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
```

This allows application-level metrics and GPU telemetry to be observed together:

```text
Step 1000
│
├── loss: 1.82
├── learning_rate: 2e-5
├── throughput: ...
│
└── GPU
    ├── utilization: 97%
    ├── temperature: 71C
    ├── memory: ...
    └── memory delta: ...
```

The application can correlate GPU telemetry with:

* loss;
* learning rate;
* throughput;
* training step;
* optimizer step;
* inference latency;
* batch size;
* other workload-specific metrics.

GPUMetric does not impose a storage or observability backend.

The application can forward the collected values to systems such as:

* logging infrastructure;
* Prometheus;
* OpenTelemetry;
* custom telemetry pipelines.

---

# Why Not `nvidia-smi`?

`nvidia-smi` is an excellent system administration and diagnostic tool.

GPUMetric is intended for a different use case: **programmatic, repeated GPU telemetry collection from inside an application**.

With `nvidia-smi`, an application can repeatedly spawn an external process:

```text
Application
    │
    ├── spawn nvidia-smi
    │
    ├── parse output
    │
    ├── spawn nvidia-smi
    │
    ├── parse output
    │
    └── ...
```

GPUMetric instead provides:

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

This avoids repeatedly creating an external `nvidia-smi` process and parsing its command-line output.

GPUMetric is **not intended to replace `nvidia-smi`**.

`nvidia-smi` remains useful for:

* manual diagnostics;
* validating the NVIDIA driver;
* inspecting GPU state;
* system administration;
* troubleshooting.

---

# Error Codes

The C API uses explicit return codes for expected failures.

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
```

The library reports expected failures through return codes instead of terminating the host application.

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

### `src/gpumetric_core/`

Contains the native C implementation and C API.

### `src/python_ffi/`

Contains the Python FFI layer built on top of `ctypes`.

The Python layer is responsible for:

* loading `libgpumetric.so`;
* defining C-compatible types;
* mapping C functions into Python;
* exposing the current Python interface.

### `tests/`

Contains GPU-dependent tests used to verify the Python → C → NVML → GPU path.

### `scripts/`

Contains development and build helper scripts.

---

# Technical Limitations

## Single GPU

The current implementation uses GPU index `0`.

```text
index = 0
```

Multi-GPU support and explicit GPU selection are planned.

---

## Linux

The primary target is Linux servers with NVIDIA GPUs.

The main development environment is:

```text
Ubuntu Server
    │
    ├── NVIDIA Driver
    ├── NVML
    ├── C/CMake
    ├── Python 3
    └── GPUMetric
```

Other Linux distributions may work if the required dependencies are available.

---

## NVIDIA NVML Dependency

GPUMetric directly depends on NVIDIA NVML.

Correct operation therefore requires:

* a properly installed NVIDIA driver;
* an accessible NVML library;
* an NVIDIA GPU;
* a compatible Linux runtime environment.

---

## Python Package Distribution

The current repository is a source-based development project.

A proper Python package distribution workflow is planned, including:

```text
Python package
      │
      ▼
  wheel build
      │
      ▼
     PyPI
      │
      ▼
pip install gpumetric
```

The current development API should therefore be considered provisional.

---

# Development Philosophy

GPUMetric is intentionally small.

The project is **not** intended to become:

* a complete monitoring platform;
* a replacement for Prometheus;
* a replacement for OpenTelemetry;
* a Grafana exporter;
* a GPU scheduler;
* a GPU management daemon;
* a general-purpose observability system.

Instead, the goal is to provide a small native telemetry component:

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

This keeps the library focused on collecting GPU state while leaving storage, aggregation, visualization, alerting, and observability decisions to the application.

# Current Status

GPUMetric should currently be considered **pre-release software**.

The project is suitable for:

* experimentation;
* development;
* internal ML infrastructure;
* testing GPU telemetry collection;
* evaluating the architecture and API.

The API and packaging workflow may change before the first stable release.

For production deployments, pin the repository revision or release version you have tested rather than assuming the development branch is stable.

---

# License

GPUMetric is licensed under the **GNU General Public License v3.0 (GPLv3)**.

You are free to use, study, modify, and distribute the software in accordance with the terms of the GPLv3.

See [`LICENSE`](LICENSE) for the complete license text.

The software is provided **"AS IS"**, without warranties of any kind, either express or implied.
