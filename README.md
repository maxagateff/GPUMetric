# GPUMetric

> **WARNING:** This project is currently under active development. The API and internal implementation may change.

**GPUMetric** is a lightweight GPU telemetry library built around NVIDIA NVML.

The core implementation is written in **C**, with Python bindings provided through **FFI using `ctypes`**.

GPUMetric was originally developed as an internal component for collecting GPU metrics during deep learning workloads. It was later extracted into a standalone repository, simplified, and released as a reusable library.

GPUMetric is primarily designed for **headless Linux servers with NVIDIA GPUs** and provides low-overhead access to GPU hardware metrics without repeatedly spawning the external `nvidia-smi` process.

The current implementation provides:

* GPU temperature
* GPU utilization
* GPU memory usage
* GPU memory usage delta between samples
* Explicit error codes through the C API
* Python FFI through `ctypes`

---

## Architecture

GPUMetric intentionally keeps the telemetry path as short as possible:

```text
             NVIDIA GPU
                  │
                  ▼
             NVIDIA NVML
                  │
                  ▼
        ┌─────────────────┐
        │   C library     │
        │    GPUMetric    │
        └────────┬────────┘
                 │
               C ABI
                 │
                 ▼
        ┌─────────────────┐
        │     Python      │
        │     ctypes      │
        └────────┬────────┘
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
* handling errors.

The Python layer provides a lightweight interface for accessing the C API from Python applications.

This separation keeps the low-level telemetry path independent from the application using it.

---

## Why GPUMetric?

During machine learning and LLM workloads, it is useful to monitor not only model-level metrics but also the hardware running the workload.

For example:

```text
Training step
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

Correlating these metrics makes it possible to analyze the relationship between model behavior and GPU state.

For example, `delta_mb` can help identify unexpected changes in GPU memory consumption during long-running workloads.

GPUMetric intentionally does not implement a full monitoring or observability platform.

Instead, it provides a low-level telemetry component that can be integrated into a larger logging, monitoring, or observability stack.

---

# Technical Features

## Zero-Allocation Sampling

The C core uses a static ring buffer (`MemoryCache`) with a fixed number of slots to maintain memory sampling history and calculate memory deltas.

Dynamic memory allocation through `malloc` is not performed during metric sampling.

This keeps the sampling path lightweight and predictable during long-running GPU workloads.

---

## Single-GPU Support

The current implementation monitors only the first NVIDIA GPU:

```text
GPU index: 0
```

Multi-GPU support and explicit device selection are planned for future releases.

---

## Explicit C/Python ABI Mapping

The Python FFI layer explicitly maps C integer types to their corresponding `ctypes` types:

```text
uint32_t  -> ctypes.c_uint32
uint64_t  -> ctypes.c_uint64
int64_t   -> ctypes.c_int64
```

Keeping the ABI types explicit helps prevent issues related to:

* type sizes;
* memory layout;
* integer truncation;
* C/Python FFI incompatibilities.

---

## Headless Linux

GPUMetric is primarily designed for headless Linux servers.

The typical target environment looks like:

```text
Ubuntu Server
    │
    ├── NVIDIA Driver
    ├── NVIDIA NVML
    ├── GPUMetric
    └── Python application
```

The primary use cases are GPU servers, training workloads, inference workloads, and other long-running compute workloads.

---

# Requirements

Before installing GPUMetric, make sure your system provides:

1. An NVIDIA GPU
2. An installed NVIDIA driver
3. NVIDIA Management Library (NVML)
4. A C compiler
5. CMake
6. Python 3

Verify that the NVIDIA driver is working:

```bash
nvidia-smi
```

If `nvidia-smi` cannot access the GPU, GPUMetric will not be able to initialize NVML successfully either.

---

# Installing Dependencies

On Ubuntu Server, use the project's dependency installation script:

```bash
chmod +x install_deps.sh
./install_deps.sh
```

After installation, verify that CMake is available:

```bash
cmake --version
```

Then verify the NVIDIA driver:

```bash
nvidia-smi
```

---

# Building

## Recommended Method

After installing the dependencies, run:

```bash
chmod +x scripts/build/initial_run_of_the_cmake_build.sh
./scripts/build/initial_run_of_the_cmake_build.sh
```

After a successful build, the shared library should be located at:

```text
build/libgpumetric.so
```

You can verify the result with:

```bash
ls -lh build/libgpumetric.so
```

---

## Manual CMake Build

GPUMetric can also be built directly with CMake:

```bash
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
make

cd ..
```

The resulting shared library should be:

```text
build/
└── libgpumetric.so
```

---

# Testing

Before running the test suite, make sure the NVIDIA driver is working:

```bash
nvidia-smi
```

If `nvidia-smi` successfully reports the GPU, run:

```bash
python3 tests/test_run.py
```

The test loads:

```text
build/libgpumetric.so
```

and periodically collects GPU metrics through the C API.

If NVML initialization or GPU sampling fails, the library returns the corresponding error code.

---

# Usage

Once the library has been successfully built and tested, GPUMetric can be loaded from a Python application through `ctypes`.

A minimal example:

```python
from FFI import GPUMetrics

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

The `samples()` method returns two values:

```python
ret, stats = gpu_metrics.samples()
```

where:

* `ret` is the operation result code;
* `stats` contains the collected GPU metrics.

The return code should be checked before using the contents of `stats`.

---

# Available Metrics

The current implementation provides the following metrics:

| Field      | Description                                          |
| ---------- | ---------------------------------------------------- |
| `temp`     | GPU temperature                                      |
| `util`     | Current GPU utilization                              |
| `mem_mb`   | GPU memory currently in use, in MiB                  |
| `delta_mb` | Change in GPU memory usage since the previous sample |

Example output:

```text
Temperature: 67C
Util: 94%
Memory: 6124MiB
Delta Memory: 128MiB
```

## Memory Delta

`delta_mb` represents the change in GPU memory usage between samples.

For example:

```text
4096 MiB
4128 MiB
4160 MiB
4300 MiB
```

produces:

```text
+32 MiB
+32 MiB
+140 MiB
```

This can be useful for detecting gradual changes in memory consumption during long-running workloads.

---

# Using GPUMetric with Machine Learning

GPUMetric can be used directly inside a training loop or another long-running GPU workload.

For example:

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

This makes it possible to correlate model-level metrics with GPU telemetry:

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

Hardware telemetry can therefore be correlated with:

* loss;
* learning rate;
* throughput;
* training step;
* optimizer step;
* other ML workload metrics.

GPUMetric does not define where or how these metrics should be stored.

The application can forward them to a logging system, Prometheus, OpenTelemetry, or another telemetry backend.

---

# Why Not `nvidia-smi`?

GPUMetric does not launch `nvidia-smi` as a separate process for every sample.

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

This allows the application to access NVML directly through the C library without repeatedly spawning an external process.

GPUMetric is **not intended to replace `nvidia-smi`**.

`nvidia-smi` remains useful for:

* manual diagnostics;
* validating the NVIDIA driver;
* inspecting GPU state;
* system administration and troubleshooting.

---

# Error Codes

The C API uses explicit return codes for expected failures.

| Code | Constant                   | Description                             |
| ---: | -------------------------- | --------------------------------------- |
|  `0` | `GPU_METRIC_SUCCESS`       | Operation completed successfully        |
| `-1` | `GPU_METRIC_ERR_NVML`      | NVML initialization or operation failed |
| `-2` | `GPU_METRIC_ERR_NO_DEVICE` | No NVIDIA GPU was detected              |
| `-3` | `GPU_METRIC_ERR_DEVICE`    | GPU interaction or sampling failed      |
| `-4` | `GPU_METRIC_ERR_ARGUMENT`  | Invalid argument, such as `NULL`        |

Example:

```python
ret, stats = gpu_metrics.samples()

if ret != 0:
    print(f"GPUMetric error: {ret}")
```

Explicit return codes allow the application to decide how failures should be handled instead of having the library terminate the process.

---

# Project Structure

```text
GPUMetric/
├── gpu_metric.c
├── gpu_metric.h
├── logger.h
├── FFI.py
├── tests/
│   └── test_run.py
├── scripts/
│   └── build/
│       └── initial_run_of_the_cmake_build.sh
├── install_deps.sh
├── CMakeLists.txt
└── README.md
```

## Core Components

### `gpu_metric.c` / `gpu_metric.h`

The core C implementation and public C API.

Responsible for:

* NVML initialization;
* GPU interaction;
* metric collection;
* sampling history;
* memory delta calculation;
* error handling.

### `logger.h`

A small set of logging macros used by the C implementation for diagnostics and error reporting.

### `FFI.py`

The Python FFI layer built on top of `ctypes`.

It:

* loads `libgpumetric.so`;
* defines the C-compatible types;
* maps C functions into Python;
* provides the Python interface for collecting GPU metrics.

### `tests/test_run.py`

A test script used to verify the Python → C → NVML → GPU path.

---

# Technical Limitations

## Single GPU

The current version uses only the first NVIDIA GPU:

```text
index = 0
```

Multi-GPU support and explicit GPU selection are planned for future releases.

## Linux

The primary target is Linux servers with NVIDIA GPUs.

The project is primarily tested in environments such as:

```text
Ubuntu Server
NVIDIA Driver
NVML
Python 3
```

Support for other Linux distributions may depend on the availability of NVML and the required build tools.

## NVIDIA NVML Dependency

GPUMetric directly depends on the NVIDIA Management Library.

Correct operation therefore requires:

* a properly installed NVIDIA driver;
* accessible NVML;
* a working NVIDIA GPU;
* a compatible runtime environment.

---

# Roadmap

The current implementation is intentionally small and focused on low-level GPU telemetry.

Planned areas of development include:

* [ ] Multi-GPU support
* [ ] Explicit GPU selection
* [ ] Additional NVML metrics
* [ ] More comprehensive automated tests
* [ ] Improved Python API
* [ ] Python package distribution and installation
* [ ] Prometheus integration
* [ ] OpenTelemetry integration
* [ ] FFI bindings for additional programming languages

The core architecture is expected to remain:

```text
NVIDIA NVML
     │
     ▼
     C
     │
     ▼
   C ABI
     │
     ▼
  Python FFI
     │
     ▼
 ML / DL application
```

GPUMetric is not intended to become a full observability platform.

The goal is to provide a small, predictable, low-level component for collecting GPU telemetry with minimal overhead.

---

# License

This project is licensed under the **GNU General Public License v3 (GPLv3)**.

You are free to use, study, modify, and distribute the software in accordance with the terms of the GPLv3.

The full license text is available in:

```text
LICENSE
```

The software is provided **"AS IS"**, without warranties of any kind, either express or implied.

The author is not responsible for hardware damage, data loss, system downtime, or any other consequences resulting from the use of this software.

---

# Project Status

**GPUMetric is currently under active development.**

The API, internal implementation, and project structure may change between releases.

If you use GPUMetric in production, pin the library version and review API changes before upgrading.

The core goal of the project is:

> **Provide a simple, low-level way to collect GPU telemetry from Python with minimal overhead.**

```
```
