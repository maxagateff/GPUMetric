# GPUMetric Documentation

Technical documentation for **GPUMetric**, a lightweight NVIDIA GPU telemetry library for Linux and machine learning workloads.

GPUMetric is a native GPU telemetry component built around **NVIDIA NVML**.

The native implementation is written in **C** and exposed to Python through a thin **`ctypes` FFI** layer.

The library is designed to collect selected GPU telemetry directly from a long-running application process and expose the resulting observations through a small Python API.

---

# 1. Project Overview

GPUMetric solves a narrow infrastructure problem:

> Collect selected NVIDIA GPU telemetry from inside an application process without repeatedly spawning `nvidia-smi`.

The runtime path is:

```text
Python Application
        │
        ▼
GPUMetrics
        │
        ▼
ctypes FFI
        │
        ▼
C ABI
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
NVIDIA GPU
```

The current telemetry model exposes:

* GPU temperature
* GPU utilization
* GPU memory usage
* GPU memory usage delta between consecutive samples

The output is intentionally small.

GPUMetric collects telemetry and returns it to the application. The application determines how that data is logged, exported, transported, aggregated, stored, or visualized.

---

# 2. Design Philosophy

## 2.1 Narrow hardware-facing component

The native layer exists to communicate with NVML and expose GPU state.

It is not intended to become a general-purpose observability system.

---

## 2.2 Small Python API

The primary Python interface is:

```python
from gpumetric import GPUMetrics, GPUStats
```

The application should not need to understand the implementation details of the native layer.

---

## 2.3 Collection and observability are separate

GPUMetric collects telemetry.

It does not own:

* metric storage
* dashboards
* alerting
* distributed aggregation
* message-bus infrastructure
* time-series databases
* telemetry backends

Those concerns belong to the surrounding application or infrastructure.

---

## 2.4 Explicit resource lifecycle

The preferred lifecycle is:

```text
initialize
    │
    ▼
sample
    │
    ├── sample
    ├── sample
    ├── sample
    │
    ▼
cleanup
```

A monitoring object should normally be initialized once and reused for multiple samples.

---

# 3. System Architecture

```text
┌──────────────────────────────────────────────────────────────┐
│                         Application                          │
│                                                              │
│  ML Training / Inference / GPU Worker / Monitoring Agent    │
└──────────────────────────────┬───────────────────────────────┘
                               │
                               │ Python API
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                        Python Layer                          │
│                                                              │
│  GPUMetrics                                                   │
│  GPUStats                                                     │
│  Exception hierarchy                                          │
│                                                              │
│  ctypes FFI                                                   │
└──────────────────────────────┬───────────────────────────────┘
                               │
                               │ C ABI
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                         Native Layer                         │
│                                                              │
│  libgpumetric.so                                              │
│                                                              │
│  - NVML initialization                                        │
│  - device selection                                           │
│  - GPU handle management                                      │
│  - telemetry collection                                       │
│  - sampling state                                             │
│  - memory delta calculation                                   │
│  - native lifecycle                                           │
└──────────────────────────────┬───────────────────────────────┘
                               │
                               │ NVML API
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                       NVIDIA Stack                            │
│                                                              │
│  NVIDIA Driver                                                 │
│       │                                                       │
│       ▼                                                       │
│      NVML                                                     │
└──────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
                         NVIDIA GPU
```

---

# 4. Repository Architecture

The `develop` branch uses a `src/` package layout:

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

The responsibilities are:

```text
src/gpumetric/
    Python application-facing package

src/gpumetric/metrics.py
    GPUMetrics and GPUStats

src/gpumetric/_ffi.py
    ctypes/native ABI boundary

src/gpumetric/exceptions.py
    Public exception hierarchy

src/gpumetric_core/gpu_metric.c
    Native NVML implementation

src/gpumetric_core/gpu_metric.h
    Native C ABI declarations

CMakeLists.txt
    Native build configuration

pyproject.toml
    Python packaging configuration

tests/
    Unit and integration-level validation

docs/DOCS.md
    Technical documentation
```

---

# 5. Runtime Data Flow

A call to:

```python
stats = gpu.sample()
```

passes through the following layers:

```text
gpu.sample()
     │
     ▼
GPUMetrics
     │
     ▼
GPUMetricFFI
     │
     ▼
ctypes
     │
     ▼
C ABI
     │
     ▼
libgpumetric.so
     │
     ▼
NVML
     │
     ├── temperature
     ├── utilization
     └── memory
     │
     ▼
native sampling state
     │
     ├── current memory
     └── previous memory
     │
     ▼
GPUStats
     │
     ▼
Python Application
```

---

# 6. Native C Layer

The native implementation is located in:

```text
src/gpumetric_core/
```

The primary implementation is:

```text
gpu_metric.c
```

with the C interface declared in:

```text
gpu_metric.h
```

The native layer is responsible for:

* NVML initialization
* NVIDIA GPU enumeration
* GPU device selection
* NVML device handle acquisition
* temperature collection
* utilization collection
* memory collection
* memory delta calculation
* native state management
* native error codes
* NVML shutdown

The native implementation is compiled into:

```text
libgpumetric.so
```

---

# 7. C ABI

The native C interface currently exposes:

```c
int gpu_metric_init(unsigned int device_index);

int gpu_metric_sample(GPUStats* out);

void gpu_metric_cleanup(void);
```

The telemetry structure is:

```c
typedef struct {
    uint32_t temp;
    uint32_t util;
    uint64_t mem_mib;
    int64_t delta_mib;
} GPUStats;
```

The native API uses negative integer return codes for failures.

Current codes are:

| Code | Meaning                                |
| ---: | -------------------------------------- |
|  `0` | Success                                |
| `-1` | NVML initialization failure            |
| `-2` | No NVIDIA GPU detected                 |
| `-3` | GPU communication or telemetry failure |
| `-4` | Invalid argument                       |
| `-5` | Native library not initialized         |

The C ABI is the contract between the native implementation and the Python FFI layer.

---

# 8. Native State Model

The current native implementation maintains process-wide static state:

```c
static nvmlDevice_t device;
static MemoryCache cache;
static int initialized = 0;
```

This means the native library currently operates with:

> **one active GPU context per process**

The selected NVML device and memory sampling cache are not stored per `GPUMetrics` Python object.

Instead, they belong to the process-wide native context.

---

# 9. Consequences of the Process-Wide Context

A normal application should use:

```python
with GPUMetrics(device_index=0) as gpu:
    ...
```

and maintain that object for the lifetime of the monitoring component.

The native initialization function is idempotent while the native context is already initialized.

Therefore:

```python
gpu0 = GPUMetrics(device_index=0)
gpu1 = GPUMetrics(device_index=1)
```

does **not** currently provide two independent native contexts.

The second initialization sees the native library as already initialized.

The current implementation therefore should not be treated as a multi-context GPU telemetry manager.

For multi-GPU applications, the safest current design is to use one native context at a time and explicitly manage the lifecycle between device selections.

This is an implementation limitation of the current native state model, not a limitation imposed by NVML itself.

---

# 10. NVIDIA NVML

GPUMetric uses NVIDIA NVML as its hardware management interface.

The dependency chain is:

```text
GPUMetric
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

NVML provides the hardware-level telemetry queried by the native implementation.

A functional NVIDIA driver and NVML runtime are therefore required.

---

# 11. Why NVML Instead of `nvidia-smi`

`nvidia-smi` is a command-line interface used for NVIDIA GPU administration, inspection, and diagnostics.

A repeated application-side CLI approach looks like:

```text
Application
    │
    ├── spawn nvidia-smi
    │
    ▼
nvidia-smi
    │
    ▼
NVIDIA stack
    │
    ▼
stdout
    │
    ▼
Application parser
```

GPUMetric instead provides:

```text
Application
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

This eliminates the repeated external process boundary and CLI parsing from the application's sampling path.

This is an architectural benefit rather than a claim that GPUMetric is universally faster than every possible use of `nvidia-smi`.

`nvidia-smi` remains the appropriate tool for:

* diagnostics
* driver validation
* administration
* manual inspection
* troubleshooting

GPUMetric is intended for programmatic application-side telemetry.

---

# 12. Python Package

The Python package is located under:

```text
src/gpumetric/
```

The package exposes:

```python
from gpumetric import GPUMetrics, GPUStats
```

It also exports the public exception hierarchy.

The package version currently defined by the source is:

```python
__version__ = "0.1.1"
```

---

# 13. `GPUMetrics`

`GPUMetrics` is the primary runtime interface.

Import it with:

```python
from gpumetric import GPUMetrics
```

Initialize it with:

```python
gpu = GPUMetrics(device_index=0)
```

The constructor currently accepts:

```python
GPUMetrics(
    device_index: int = 0,
    lib_path: str | None = None,
)
```

`device_index` selects the NVIDIA GPU.

`lib_path` allows the native shared library path to be overridden.

The default native library path is resolved relative to the installed Python package:

```text
gpumetric/lib/libgpumetric.so
```

---

# 14. Device Index Validation

The Python layer validates `device_index` before passing it to native code.

It must:

* be an integer
* be non-negative

Invalid values raise:

```python
GPUMetricArgumentError
```

For example:

```python
from gpumetric import GPUMetrics

GPUMetrics(device_index=-1)
```

raises a Python-level argument error.

The native layer additionally validates the device index against the number of GPUs reported by NVML.

---

# 15. `GPUStats`

`GPUStats` is the public telemetry object returned by:

```python
gpu.sample()
```

It is defined as an immutable dataclass:

```python
@dataclass(frozen=True, slots=True)
class GPUStats:
    temperature: int
    utilization: int
    memory_mib: int
    memory_delta_mib: int
```

A sample therefore contains four values:

```python
stats.temperature
stats.utilization
stats.memory_mib
stats.memory_delta_mib
```

`GPUStats` does not initialize NVML and does not communicate with the GPU.

It represents the result of a sampling operation.

---

# 16. Temperature

Temperature is exposed through:

```python
stats.temperature
```

The value represents the GPU temperature in degrees Celsius reported by NVML.

Example:

```text
42
```

This is an instantaneous telemetry observation.

It is not a historical thermal average or thermal profile.

---

# 17. GPU Utilization

Utilization is exposed through:

```python
stats.utilization
```

It represents GPU utilization as a percentage.

The expected range is:

```text
0 - 100
```

The value comes from:

```c
nvmlDeviceGetUtilizationRates()
```

and the native implementation uses the GPU utilization field.

---

# 18. GPU Memory

Current GPU memory usage is exposed through:

```python
stats.memory_mib
```

The native implementation obtains memory information through:

```c
nvmlDeviceGetMemoryInfo()
```

NVML reports memory in bytes.

GPUMetric converts the used memory value to MiB:

```text
memory_mib = used_bytes / (1024 * 1024)
```

Example:

```text
397
```

means approximately 397 MiB of reported used GPU memory after the integer conversion.

---

# 19. Memory Delta

The native implementation tracks the previous memory observation.

Conceptually:

```text
memory_delta_mib =
    current_memory_mib -
    previous_memory_mib
```

Example:

```text
Sample 1:
397 MiB

Sample 2:
430 MiB

Delta:
+33 MiB
```

A decrease is also valid:

```text
Sample 1:
430 MiB

Sample 2:
397 MiB

Delta:
-33 MiB
```

The first sample after initialization has no previous observation and therefore produces a zero delta in the native implementation.

The cache is updated only after all NVML telemetry calls for the sample succeed.

---

# 20. Memory Delta Semantics

`memory_delta_mib` is a change in the **reported GPU memory usage**.

It is not:

* CUDA allocation size
* tensor allocation size
* per-process allocation
* allocator statistics
* CUDA memory leak detection
* kernel memory usage
* individual allocation tracking

For example, a positive delta:

```text
+512 MiB
```

means that the observed total GPU memory usage increased by approximately 512 MiB between the two samples.

It does not identify what caused the increase.

---

# 21. Important FFI ABI Requirement

The native C ABI defines:

```c
int64_t delta_mib;
```

The corresponding `ctypes` representation must therefore be:

```python
("delta_mib", ctypes.c_int64)
```

The current FFI implementation in `develop` uses:

```python
("delta_mib", ctypes.c_int64)
```

This is an ABI mismatch.

A negative native `int64_t` value can therefore be interpreted as a very large positive `uint64_t` value in Python.

The FFI definition should be corrected before relying on negative memory deltas.

The correct mapping is:

```python
class GPUStates(ctypes.Structure):
    _fields_ = [
        ("temp", ctypes.c_uint32),
        ("utils", ctypes.c_uint32),
        ("mem_mib", ctypes.c_uint64),
        ("delta_mib", ctypes.c_int64),
    ]
```

This correction should be included in the next source revision and covered by a test that validates a negative delta.

---

# 22. Sampling

The primary sampling operation is:

```python
stats = gpu.sample()
```

Example:

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()

    print(stats.temperature)
    print(stats.utilization)
    print(stats.memory_mib)
    print(stats.memory_delta_mib)
```

The native layer performs all required NVML queries during the sample.

---

# 23. Repeated Sampling

For long-running workloads, initialize the monitoring object once:

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
GPUMetrics cleanup
```

Do not create and destroy the native context around every sample unless the application explicitly requires that lifecycle.

---

# 24. Context Manager

The preferred resource-management interface is the context manager:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

Entering the context returns the `GPUMetrics` instance.

Exiting the context calls:

```python
cleanup()
```

The lifecycle is:

```text
enter
  │
  ▼
native initialization
  │
  ▼
sampling
  │
  ▼
exit
  │
  ▼
native cleanup
```

---

# 25. Explicit Cleanup

The object also provides:

```python
gpu.cleanup()
```

Cleanup is idempotent.

Calling it multiple times is safe:

```python
gpu.cleanup()
gpu.cleanup()
```

After cleanup, another call to:

```python
gpu.sample()
```

raises:

```python
GPUMetricNotInitializedError
```

The object also performs best-effort cleanup during Python object destruction.

The context manager remains the preferred explicit lifecycle mechanism.

---

# 26. Reinitialization

After a `GPUMetrics` instance has been cleaned up, another instance can initialize the native library:

```python
with GPUMetrics(device_index=0) as first:
    first_stats = first.sample()

with GPUMetrics(device_index=0) as second:
    second_stats = second.sample()
```

The native cleanup resets:

* initialization state
* memory sampling cache
* device handle state

Therefore the second initialization starts a new sampling sequence.

The first sample of the new sequence has no previous sample and therefore starts with a zero memory delta.

---

# 27. Exception Hierarchy

The package exposes:

```python
GPUMetricError
```

as the base exception.

The current hierarchy is:

```text
GPUMetricError
├── GPUMetricInitializationError
├── GPUMetricNoDeviceError
├── GPUMetricDeviceError
├── GPUMetricArgumentError
├── GPUMetricNotInitializedError
└── GPUMetricSamplingError
```

---

# 28. `GPUMetricInitializationError`

Raised when NVML initialization fails.

Example:

```python
from gpumetric import GPUMetricInitializationError
```

Typical causes include:

* NVIDIA driver problems
* NVML initialization failure
* inaccessible NVIDIA runtime

---

# 29. `GPUMetricNoDeviceError`

Raised when NVML reports that no compatible NVIDIA GPU is available.

Example:

```python
from gpumetric import GPUMetricNoDeviceError
```

This can occur on systems without an accessible NVIDIA GPU.

---

# 30. `GPUMetricDeviceError`

Raised when GPUMetric cannot communicate with the GPU or fetch telemetry.

Typical causes include:

* GPU communication failure
* NVML telemetry query failure
* driver/runtime failure
* inaccessible device

---

# 31. `GPUMetricArgumentError`

Raised when an invalid argument is supplied.

It also derives from:

```python
ValueError
```

Example:

```python
GPUMetrics(device_index=-1)
```

---

# 32. `GPUMetricNotInitializedError`

Raised when an operation requires an initialized native context but the context is not initialized.

For example:

```python
gpu.cleanup()
gpu.sample()
```

results in:

```python
GPUMetricNotInitializedError
```

---

# 33. `GPUMetricSamplingError`

Raised when a GPU sampling operation fails without mapping to a more specific public error type.

It represents a failure during:

```python
gpu.sample()
```

---

# 34. Native Error Mapping

The Python layer converts native return codes into Python exceptions.

Conceptually:

```text
C return code
      │
      ▼
GPUMetrics
      │
      ▼
Python exception
```

Current mapping includes:

| Native code | Python exception               |
| ----------: | ------------------------------ |
|        `-1` | `GPUMetricInitializationError` |
|        `-2` | `GPUMetricNoDeviceError`       |
|        `-3` | `GPUMetricDeviceError`         |
|        `-4` | `GPUMetricArgumentError`       |
|        `-5` | `GPUMetricNotInitializedError` |

Unknown initialization errors are mapped to:

```python
GPUMetricError
```

Unknown sampling errors are mapped to:

```python
GPUMetricSamplingError
```

---

# 35. Error Handling Example

A simple application-level boundary is:

```python
from gpumetric import GPUMetrics


try:
    with GPUMetrics(device_index=0) as gpu:
        stats = gpu.sample()
        print(stats)

except Exception as exc:
    print(f"GPU monitoring failed: {exc}")
```

Production applications should prefer catching the specific public exception classes when the application can distinguish recovery behavior.

---

# 36. Runtime Requirements

The primary validated environment is:

| Component        | Target            |
| ---------------- | ----------------- |
| Operating system | Linux x86-64      |
| Distribution     | Ubuntu            |
| Python           | CPython 3.11      |
| GPU              | NVIDIA GPU        |
| GPU interface    | NVIDIA NVML       |
| Native library   | `libgpumetric.so` |

The Python package metadata currently declares:

```text
Python >= 3.10
```

and advertises Python 3.10 through 3.13 classifiers.

However, the primary development and validation environment for the current project is CPython 3.11.

Other environments require separate validation.

---

# 37. NVIDIA Driver Validation

Before troubleshooting GPUMetric, validate the NVIDIA stack:

```bash
nvidia-smi
```

The command should successfully communicate with the GPU.

The dependency chain is:

```text
Python
  │
  ▼
GPUMetric
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

If the NVIDIA runtime is broken, GPUMetric cannot provide GPU telemetry.

---

# 38. Python Environment

Verify Python:

```bash
python3.11 --version
```

Expected:

```text
Python 3.11.x
```

Create a virtual environment:

```bash
python3.11 -m venv .venv
source .venv/bin/activate
```

Upgrade packaging tools:

```bash
python -m pip install --upgrade pip setuptools wheel
```

---

# 39. Installation from PyPI

Install:

```bash
python3.11 -m pip install gpumetric
```

Verify:

```bash
python3.11 -c "import gpumetric; print(gpumetric)"
```

Verify the public classes:

```bash
python3.11 -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics, GPUStats)"
```

A successful import confirms that the Python package is installed.

It does not prove that the NVIDIA driver or NVML is functional.

---

# 40. Minimal Runtime Validation

After installation:

```bash
python3.11 - <<'PY'
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
    print(stats)
PY
```

This exercises:

```text
Python
  │
  ▼
GPUMetric
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

---

# 41. Native Build

The native library is built using CMake.

The current `CMakeLists.txt` requires:

```text
CMake >= 3.15
```

and builds the native implementation using:

```text
C11
```

The native library source is:

```text
src/gpumetric_core/gpu_metric.c
```

Build manually from the repository:

```bash
mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release

cd ..
```

---

# 42. Native Build Dependencies

CMake searches for:

```text
nvml.h
```

and:

```text
libnvidia-ml.so
```

The current CMake configuration searches common locations including:

```text
/usr/include
/usr/include/nvidia
/usr/include/nvidia/gdk
/usr/local/cuda/include
```

for the NVML header.

For the NVML library it searches locations including:

```text
/usr/lib
/usr/lib64
/usr/lib/x86_64-linux-gnu
/usr/local/cuda/lib64
```

If either the header or library cannot be found, CMake stops with an error.

---

# 43. CMake Configuration

The project configures:

```text
C standard: C11
C extensions: disabled
Release optimization: -O3
```

The native target is:

```text
gpumetric
```

and its output name is:

```text
libgpumetric.so
```

The target links against:

```text
libnvidia-ml.so
```

through the NVML library discovered by CMake.

---

# 44. Python Packaging

The Python build backend is:

```text
scikit-build-core
```

as configured in:

```text
pyproject.toml
```

The relevant configuration is:

```toml
[build-system]
requires = [
    "scikit-build-core>=0.11"
]
build-backend = "scikit_build_core.build"
```

The Python package uses the `src/` layout:

```toml
[tool.scikit-build]
wheel.packages = ["src/gpumetric"]
```

The CMake build type is configured as:

```toml
[tool.scikit-build.cmake]
build-type = "Release"
```

---

# 45. Native Library Packaging

CMake installs the native library into:

```text
gpumetric/lib
```

The relevant installation rule is:

```cmake
install(
    TARGETS gpumetric
    LIBRARY DESTINATION gpumetric/lib
)
```

The Python FFI layer resolves the default native library path as:

```text
<installed Python package>/lib/libgpumetric.so
```

This allows the native implementation to be packaged together with the Python distribution.

---

# 46. FFI Layer

The Python FFI implementation is:

```text
src/gpumetric/_ffi.py
```

It uses:

```python
import ctypes
```

The native shared library is loaded using:

```python
ctypes.CDLL(...)
```

The FFI layer defines the native structure:

```python
class GPUStates(ctypes.Structure):
    ...
```

and configures the native function signatures:

```python
gpu_metric_init
gpu_metric_sample
gpu_metric_cleanup
```

---

# 47. FFI Responsibility

The FFI layer is intentionally thin.

It is responsible for:

* locating the native library
* loading the shared library
* defining native structure layouts
* defining function argument types
* defining function return types
* calling native functions
* passing native structures between Python and C

The FFI layer should not become a second telemetry implementation.

The hardware logic belongs in the native layer.

The application-facing semantics belong in `GPUMetrics`.

---

# 48. C/Python ABI Contract

The C structure:

```c
typedef struct {
    uint32_t temp;
    uint32_t util;
    uint64_t mem_mib;
    int64_t delta_mib;
} GPUStats;
```

must have an exactly matching `ctypes.Structure`.

The corresponding Python types are:

```python
ctypes.c_uint32
ctypes.c_uint32
ctypes.c_uint64
ctypes.c_int64
```

ABI-compatible field order and field widths are critical.

A mismatch can corrupt values without producing a Python exception.

---

# 49. ML Training Integration

GPUMetric can run directly inside a training process:

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

This allows application-level metrics to be correlated with GPU hardware state.

For example:

```text
Application
───────────
step
loss
throughput
batch size
latency

GPU
───
temperature
utilization
memory
memory delta
```

---

# 50. Inference Integration

The same model can be used in inference workers.

A service can correlate:

```text
request latency
batch size
throughput
```

with:

```text
GPU utilization
GPU memory
GPU temperature
```

without requiring a separate GPU monitoring process.

---

# 51. Observability Integration

GPUMetric intentionally does not implement a telemetry backend.

The application owns the next stage:

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
 ├── Logs
 ├── Metrics
 ├── Events
 └── Traces
```

Possible destinations include:

* Prometheus exporters
* OpenTelemetry
* Kafka
* message brokers
* time-series databases
* structured logging
* internal monitoring services

Example:

```python
stats = gpu.sample()

record = {
    "gpu_temperature_celsius": stats.temperature,
    "gpu_utilization_percent": stats.utilization,
    "gpu_memory_mib": stats.memory_mib,
    "gpu_memory_delta_mib": stats.memory_delta_mib,
}
```

GPUMetric stops at the application boundary.

---

# 52. Separation of Responsibilities

## GPUMetric owns

```text
NVML communication
GPU selection
GPU telemetry
sampling state
memory delta calculation
native lifecycle
Python/native ABI
```

## Application owns

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

This separation keeps GPUMetric independent of any specific observability stack.

---

# 53. Testing

The repository uses `pytest`.

Install it if necessary:

```bash
python3.11 -m pip install pytest
```

Run the test suite:

```bash
python3.11 -m pytest
```

Run in verbose mode:

```bash
python3.11 -m pytest -v
```

---

# 54. Current Test Coverage

The repository currently contains tests for:

```text
test_context_manager.py
test_device_index_type.py
test_gpu_metrics.py
test_gpu_metrics_reinitialization.py
test_gpu_stats.py
test_import.py
test_lifecycle.py
```

The tests cover areas including:

* package import
* public API
* `GPUStats` creation
* `GPUStats` field types
* `GPUMetrics` initialization
* sampling
* telemetry value types
* utilization range
* multiple samples
* context-manager behavior
* reinitialization
* device-index validation

---

# 55. Integration Test Requirements

Tests that instantiate:

```python
GPUMetrics(device_index=0)
```

require:

* Linux x86-64
* Ubuntu or compatible Linux environment
* CPython 3.11
* NVIDIA GPU
* functional NVIDIA driver
* functional NVML
* valid GPU device index

A CPU-only environment cannot execute the complete hardware integration path.

---

# 56. Testing the Published Artifact

A local source-tree test and a clean package installation validate different things.

Source validation:

```text
Repository
    │
    ▼
Local build
    │
    ▼
Local environment
    │
    ▼
Tests
```

Package validation:

```text
PyPI
    │
    ▼
pip install
    │
    ▼
Clean virtual environment
    │
    ▼
Python package
    │
    ▼
Native shared library
    │
    ▼
NVML
    │
    ▼
GPU
```

The second path is important because it validates the artifact users actually install.

Example:

```bash
python3.11 -m venv /tmp/gpumetric-test
source /tmp/gpumetric-test/bin/activate

python -m pip install --upgrade pip
python -m pip install gpumetric
```

Verify:

```bash
python -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics, GPUStats)"
```

Then:

```bash
python - <<'PY'
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    print(gpu.sample())
PY
```

---

# 57. Troubleshooting

## `nvidia-smi` fails

Fix the NVIDIA driver/GPU environment first.

GPUMetric depends on the same underlying NVIDIA management stack.

---

## Python import fails

Check:

```bash
python3.11 --version
python3.11 -m pip --version
```

Then verify the package:

```bash
python3.11 -m pip show gpumetric
```

---

## Import succeeds but `GPUMetrics` initialization fails

Run:

```bash
nvidia-smi
```

Then inspect the exact Python exception:

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    print(gpu.sample())
```

Determine whether the failure is:

```text
NVML initialization
GPU detection
device selection
native library loading
GPU communication
```

---

## Invalid device index

List available GPUs:

```bash
nvidia-smi -L
```

Then select a valid index:

```python
GPUMetrics(device_index=0)
```

---

## Native library not found

The Python FFI expects the default library at:

```text
gpumetric/lib/libgpumetric.so
```

If using a custom development build, provide an explicit path:

```python
from gpumetric import GPUMetrics

gpu = GPUMetrics(
    device_index=0,
    lib_path="/path/to/libgpumetric.so",
)
```

---

# 58. Production Lifecycle

A typical production GPU worker should follow:

```text
1. Start process
       │
       ▼
2. Validate NVIDIA runtime
       │
       ▼
3. Initialize GPUMetrics
       │
       ▼
4. Run workload
       │
       ├── sample()
       ├── sample()
       ├── sample()
       │
       ▼
5. Export telemetry
       │
       ▼
6. Cleanup
       │
       ▼
7. Process shutdown
```

Recommended principles:

1. Validate the NVIDIA runtime.
2. Initialize `GPUMetrics` once.
3. Reuse the monitoring object.
4. Sample at the application's required interval.
5. Treat `GPUStats` as an observation.
6. Export telemetry through existing application infrastructure.
7. Handle monitoring failures explicitly.
8. Keep storage and transport outside GPUMetric.
9. Do not assume multiple `GPUMetrics` objects represent independent native contexts.

---

# 59. Minimal Monitoring Service

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

The application can replace:

```python
print(record)
```

with its own telemetry exporter.

---

# 60. API Summary

## Import

```python
from gpumetric import GPUMetrics, GPUStats
```

## Initialize

```python
gpu = GPUMetrics(device_index=0)
```

## Preferred lifecycle

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

## Sample

```python
stats = gpu.sample()
```

## Read telemetry

```python
stats.temperature
stats.utilization
stats.memory_mib
stats.memory_delta_mib
```

## Explicit cleanup

```python
gpu.cleanup()
```

---

# 61. Architecture Summary

The complete runtime architecture is:

```text
                         Application
                              │
                              ▼
                         GPUMetrics
                              │
                              ▼
                         ctypes FFI
                              │
                              ▼
                           C ABI
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

Each layer has a defined responsibility:

```text
Application
    Owns workload and observability.

GPUMetrics
    Owns the Python-facing telemetry lifecycle.

GPUStats
    Represents one immutable telemetry observation.

ctypes FFI
    Bridges Python and native code.

C ABI
    Defines the Python/native contract.

libgpumetric.so
    Implements native telemetry collection.

NVML
    Provides NVIDIA GPU management access.

NVIDIA Driver
    Provides the underlying GPU management stack.

GPU
    Provides the physical hardware state.
```

The central architectural principle is:

> **GPUMetric collects GPU state; the application decides what to do with it.**

---

# 62. Current Project Scope

GPUMetric is a GPU telemetry collection library.

It is not:

* a GPU scheduler
* a CUDA profiler
* a CUDA memory profiler
* a Prometheus server
* an OpenTelemetry Collector
* a message broker
* a time-series database
* a dashboard
* an alerting platform
* a distributed observability system

Its boundary is:

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
GPUStats
 │
 ▼
Application
```

Everything after the application boundary is intentionally outside the library.

---

# 63. Documentation Policy

This document is the technical reference for GPUMetric.

The repository README is intentionally concise and should contain only:

* project description
* architecture overview
* feature summary
* supported environment
* installation
* minimal usage
* documentation link
* basic testing information
* repository structure
* license

Detailed implementation and operational information belongs in:

```text
docs/DOCS.md
```

Documentation should be updated when changes affect:

* public API
* native ABI
* FFI structure definitions
* runtime lifecycle
* telemetry semantics
* supported environments
* package layout
* build process
* testing
* error handling
* deployment behavior

---

# 64. License

GPUMetric is licensed under the GNU General Public License v3.0 (GPLv3).

See the repository `LICENSE` file for the complete license text and terms.
