# GPUMetric Technical Documentation

Version 0.1.2

GPUMetric is a Linux-native NVIDIA GPU telemetry library. It combines a small C core, runtime NVML symbol loading, and a Python `ctypes` interface. This document describes the current implementation, including its limitations.

## 1. Scope

GPUMetric provides:

- GPU temperature;
- GPU execution utilization;
- allocated device memory;
- driver- and firmware-reserved device memory;
- the device-wide memory change between successful samples;
- explicit device-index selection;
- Python exceptions for native error conditions;
- deterministic cleanup through a context manager or explicit method.

GPUMetric does not provide:

- CUDA allocation tracing;
- per-tensor or per-process attribution;
- monitoring storage or transport;
- dashboards or alerting;
- GPU scheduling or administration;
- vendor-neutral telemetry;
- independent simultaneous native contexts.

## 2. Architecture

```text
Application
    │
    │ Python API
    ▼
GPUMetrics / GPUStats
    │
    │ ctypes
    ▼
libgpumetric.so
    │
    │ dlopen + dlsym
    ▼
libnvidia-ml.so.1
    │
    ▼
NVIDIA driver and GPU
```

The Python layer validates arguments, maps native return codes to exceptions, and exposes immutable results. The C layer owns NVML initialization, the selected device handle, sampling state, and cleanup.

GPUMetric does not call CUDA APIs and does not create a CUDA context.

## 3. Supported Environment

The package metadata supports CPython 3.10 through 3.14.

The wheel pipeline targets:

- Linux x86-64;
- manylinux 2.28;
- CPython 3.10, 3.11, 3.12, 3.13, and 3.14.

Runtime requirements:

- an NVIDIA GPU;
- a working NVIDIA driver;
- `libnvidia-ml.so.1`;
- permission to access the selected GPU.

The CUDA toolkit and NVIDIA development headers are not runtime requirements.

## 4. Installation

Install the latest published package:

```bash
python -m pip install gpumetric
```

Check the installed version:

```bash
python -c "import gpumetric; print(gpumetric.__version__)"
```

Check the public API:

```bash
python -c "from gpumetric import GPUMetrics, GPUStats; print(GPUMetrics, GPUStats)"
```

A successful import verifies the Python package and bundled shared library path. It does not prove that NVML can initialize or that a GPU is accessible.

Verify the host driver separately:

```bash
nvidia-smi
```

## 5. First Sample

```python
from gpumetric import GPUMetrics

with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
    print(stats)
```

A result looks like:

```text
GPUStats(
    temperature=42,
    utilization=0,
    memory_mib=0,
    memory_delta_mib=0,
    memory_reserved_mib=396
)
```

Values vary by system.

## 6. Python API

### 6.1 `GPUMetrics`

```python
GPUMetrics(
    device_index: int = 0,
    lib_path: str | None = None,
)
```

`device_index` selects the NVML device by zero-based index. It must be a non-negative integer. Strings, containers, `None`, negative integers, and Boolean values are rejected.

`lib_path` overrides the bundled library:

```python
gpu = GPUMetrics(
    device_index=0,
    lib_path="/path/to/libgpumetric.so",
)
```

This override is useful for local native builds and ABI testing. Normal package users should omit it.

Initialization occurs in the constructor. A failure raises a public GPUMetric exception.

### 6.2 `sample()`

```python
stats = gpu.sample()
```

The method performs one complete native sample and returns `GPUStats`. The output object is created only after all required NVML queries succeed.

Calling `sample()` after cleanup raises `GPUMetricNotInitializedError`.

### 6.3 `cleanup()`

```python
gpu.cleanup()
```

Cleanup shuts down the native NVML state and unloads the runtime library. Repeated calls on the same Python object are safe.

Prefer a context manager:

```python
with GPUMetrics(device_index=0) as gpu:
    stats = gpu.sample()
```

`__del__` performs best-effort cleanup, but application correctness should not depend on garbage-collection timing.

## 7. `GPUStats`

```python
@dataclass(frozen=True, slots=True)
class GPUStats:
    temperature: int
    utilization: int
    memory_mib: int
    memory_delta_mib: int
    memory_reserved_mib: int = 0
```

The object is immutable and uses slots.

| Field | Unit | Description |
| --- | --- | --- |
| `temperature` | °C | GPU temperature reported by NVML |
| `utilization` | % | Percentage of the sampling period during which GPU work executed |
| `memory_mib` | MiB | Allocated device memory |
| `memory_delta_mib` | MiB | Signed change from the previous successful sample |
| `memory_reserved_mib` | MiB | Device memory reserved for driver or firmware use |

All byte values are converted to MiB with integer division by `1024 * 1024`.

## 8. Memory Reporting

### 8.1 Preferred v2 Query

When available, GPUMetric calls:

```text
nvmlDeviceGetMemoryInfo_v2
```

The versioned memory structure separates:

- total physical device memory;
- allocated memory;
- free memory;
- system-reserved memory.

GPUMetric exposes allocated memory as `memory_mib` and the system reservation as `memory_reserved_mib`.

### 8.2 Legacy Fallback

If the runtime does not export the v2 symbol or reports it as unsupported, GPUMetric calls:

```text
nvmlDeviceGetMemoryInfo
```

The legacy structure does not expose reserved memory separately. Its used-memory result may include driver or firmware reservations. In fallback mode:

```text
memory_reserved_mib = 0
```

Consumers that require strict separation should record the driver and NVML version alongside telemetry.

### 8.3 Memory Delta

The first successful sample produces:

```text
memory_delta_mib = 0
```

Later samples calculate:

```text
memory_delta_mib = current_memory_mib - previous_memory_mib
```

The cache changes only after a complete successful sample. A failed temperature, utilization, or memory query does not advance it.

Negative values are valid when allocated memory decreases.

### 8.4 Device-Wide Meaning

These values describe the selected GPU, not the current Python process. A delta can be caused by any workload using that GPU.

GPUMetric cannot identify:

- a leaking tensor;
- a CUDA allocation call;
- a framework caching allocator;
- the process responsible for a change;
- the allocation lifetime.

Use NVIDIA profiling tools, NVML process queries, or framework-specific memory APIs for those tasks.

## 9. Exceptions

```text
GPUMetricError
├── GPUMetricInitializationError
│   └── GPUMetricDriverMismatchError
├── GPUMetricNoDeviceError
├── GPUMetricDeviceError
├── GPUMetricArgumentError
├── GPUMetricNotInitializedError
└── GPUMetricSamplingError
```

| Exception | Typical cause |
| --- | --- |
| `GPUMetricInitializationError` | NVML library load or initialization failure |
| `GPUMetricDriverMismatchError` | NVIDIA kernel driver and NVML userspace library versions differ |
| `GPUMetricNoDeviceError` | NVML found no compatible GPUs |
| `GPUMetricDeviceError` | Device lookup or telemetry query failed |
| `GPUMetricArgumentError` | Invalid Python or native argument |
| `GPUMetricNotInitializedError` | Sampling after cleanup |
| `GPUMetricSamplingError` | Native sampling returned an unknown error code |

Catch `GPUMetricError` when one handler is appropriate for all library failures.

## 10. Native C API

The installed public header declares:

```c
typedef struct {
    uint32_t temp;
    uint32_t util;
    uint64_t mem_mib;
    int64_t delta_mib;
    uint64_t reserved_mem_mib;
} GPUStats;

int gpu_metric_init(unsigned int device_index);
int gpu_metric_sample(GPUStats *out);
void gpu_metric_cleanup(void);
```

Return codes:

| Value | Constant | Meaning |
| ---: | --- | --- |
| `0` | `GPU_METRIC_SUCCESS` | Success |
| `-1` | `GPU_METRIC_ERR_NVML` | NVML load, initialization, or operation failure |
| `-2` | `GPU_METRIC_ERR_NO_DEVICE` | No compatible device |
| `-3` | `GPU_METRIC_ERR_DEVICE` | Device access or query failure |
| `-4` | `GPU_METRIC_ERR_ARGUMENT` | Invalid argument |
| `-5` | `GPU_METRIC_ERR_NOT_INITIALIZED` | Sampling before initialization or after cleanup |
| `-6` | `GPU_METRIC_ERR_DRIVER_MISMATCH` | NVIDIA kernel driver and NVML library version mismatch |

The `GPUStats` layout is part of the native ABI. Native consumers must rebuild when this structure changes.

## 11. Python FFI

The internal `ctypes.Structure` mirrors the C ABI:

```python
class GPUStates(ctypes.Structure):
    _fields_ = [
        ("temp", ctypes.c_uint32),
        ("utils", ctypes.c_uint32),
        ("mem_mib", ctypes.c_uint64),
        ("delta_mib", ctypes.c_int64),
        ("reserved_mem_mib", ctypes.c_uint64),
    ]
```

The Python field name `utils` differs from the C field name `util`, but field names do not affect structure layout. Order, width, signedness, and alignment must remain identical.

The bundled library path is:

```text
gpumetric/lib/libgpumetric.so
```

## 12. Runtime NVML Loader

GPUMetric does not link against NVML at build time. The loader opens:

```text
libnvidia-ml.so.1
```

with `RTLD_NOW | RTLD_LOCAL`, then resolves required functions with `dlsym`.

If the default library reports a driver/library version mismatch, GPUMetric
unloads it and retries common trusted absolute locations used by Linux
distributions and the NVIDIA Container Toolkit. This protects applications
from a stale or incompatible NVML library selected through `LD_LIBRARY_PATH`.
GPUMetric never bundles an NVIDIA library.

Required symbols include:

- `nvmlInit_v2`;
- `nvmlShutdown`;
- `nvmlDeviceGetCount_v2`;
- `nvmlDeviceGetHandleByIndex_v2`;
- at least one supported memory query;
- `nvmlDeviceGetUtilizationRates`;
- `nvmlDeviceGetTemperature`;
- `nvmlErrorString`.

The v2 memory query is optional because older runtimes may only provide the legacy function.

Minimal ABI declarations are maintained in:

```text
src/gpumetric_core/nvml_compat.h
```

This removes the wheel build's dependency on a distribution-specific `nvml.h`.

## 13. Native Lifecycle

Initialization performs these operations:

1. return success immediately if the process-wide state is already initialized;
2. load `libnvidia-ml.so.1`;
3. resolve required symbols;
4. call `nvmlInit_v2`;
5. query the device count;
6. validate `device_index`;
7. acquire the device handle;
8. reset the memory-delta cache;
9. mark the state initialized.

Cleanup:

1. calls the resolved `nvmlShutdown` function when initialized;
2. clears the initialized flag;
3. closes the shared-library handle;
4. clears resolved function pointers;
5. resets the cache and device handle.

Failure paths shut down and unload resources that were already acquired.

## 14. Process-Wide State Limitation

The current native state is global:

```text
NVML loader
selected device
memory cache
initialized flag
```

Consequences:

- initializing a second object does not create a second native context;
- a different device index is not applied while the global context is active;
- cleanup affects the shared native state;
- the delta cache is shared;
- concurrent sample and cleanup operations are not designed as an independent-context API.

The recommended pattern is one active object per process. A future native context handle is required for fully independent multi-GPU and concurrent use.

## 15. Building from Source

Requirements:

- CMake 3.18 or newer;
- a C11 compiler;
- Linux `libdl`;
- Python 3.10 through 3.14 for the package;
- scikit-build-core for wheel builds.

Build the native library:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The output is normally:

```text
build/libgpumetric.so
```

Test a development library explicitly:

```python
from gpumetric import GPUMetrics

with GPUMetrics(
    device_index=0,
    lib_path="build/libgpumetric.so",
) as gpu:
    print(gpu.sample())
```

## 16. Building Wheels

The package uses `scikit-build-core`. CMake installs the native library into:

```text
gpumetric/lib/libgpumetric.so
```

The GitHub Actions workflow uses `cibuildwheel` with the manylinux 2.28 image. It builds CPython wheels for 3.10 through 3.14 on x86-64.

Compiling inside the target manylinux toolchain is required. A binary compiled against a newer glibc cannot be made manylinux 2.28 compatible by changing its wheel tag afterward.

## 17. Release Workflow

The workflow is triggered by:

- pushes to `develop`;
- published GitHub Releases.

A `develop` push builds wheels and uploads them as workflow artifacts.

A published release builds the same wheel matrix and then publishes the artifacts to PyPI using the configured trusted-publishing environment.

PyPI publication does not run for ordinary development pushes.

## 18. Testing

Run:

```bash
python -m pytest -q
```

The current suite covers:

- package imports;
- public API exports;
- immutable telemetry objects;
- initialization;
- sampling;
- telemetry value types and ranges;
- invalid device-index values;
- context-manager cleanup;
- repeated initialization and cleanup;
- reinitialization.

Tests that initialize GPUMetric require an NVIDIA GPU and working NVML runtime. Import-only tests do not prove native runtime functionality.

A release artifact should also be tested in a clean environment:

```bash
python -m venv /tmp/gpumetric-test
source /tmp/gpumetric-test/bin/activate
python -m pip install --upgrade pip
python -m pip install gpumetric
python -c "from gpumetric import GPUMetrics; print(GPUMetrics().sample())"
```

## 19. Troubleshooting

### Native library not found

Symptom:

```text
FileNotFoundError: GPUMetric native library not found
```

Confirm that the wheel contains:

```text
gpumetric/lib/libgpumetric.so
```

Reinstall the correct platform wheel in a clean virtual environment.

### NVML cannot initialize

Confirm:

```bash
nvidia-smi
ldconfig -p | grep libnvidia-ml.so.1
```

Common causes include a missing driver, driver/library mismatch, inaccessible GPU device files, or container runtime configuration.

For `GPUMetricDriverMismatchError`, first restart the host if the NVIDIA
driver was updated without a reboot. In a container, do not install a separate
kernel-driver stack in the image; expose the host GPU and matching driver
libraries through the NVIDIA Container Toolkit. GPUMetric automatically tries
known system NVML locations when an incompatible library was selected through
`LD_LIBRARY_PATH`, but it cannot repair a genuine host kernel/driver mismatch.

### No compatible GPU

Check the visible devices and container GPU configuration. Device index zero must exist unless another valid index is supplied.

### Invalid device index

Use a zero-based non-negative integer:

```python
GPUMetrics(device_index=0)
```

Do not pass `True`; Python Boolean values are intentionally rejected.

### Memory appears too high

Check `memory_reserved_mib`. Driver and firmware reservations can be hundreds of MiB. Also remember that `memory_mib` is device-wide and can include allocations from other workloads.

On legacy NVML, reserved memory cannot be separated and may be included in the used-memory result.

### Sampling fails after cleanup

Create a new `GPUMetrics` instance or keep sampling inside the context-manager block.

### Container cannot access NVML

Pass the GPU and NVIDIA driver libraries into the container using the NVIDIA Container Toolkit. Installing the Python package alone does not expose the host GPU.

## 20. Operational Guidance

- Initialize once and reuse the object.
- Sample at a rate appropriate for the application; one sample per second is typical for general monitoring.
- Treat memory delta as a signal, not allocation attribution.
- Record device index, driver version, package version, and host identity with telemetry.
- Use explicit cleanup or a context manager.
- Do not assume multiple Python objects provide independent native contexts.
- Validate the exact wheel in the same type of environment used in production.

## 21. Project Layout

```text
GPUMetric/
├── .github/
│   └── workflows/
│       └── python-publish.yml
├── docs/
│   └── DOCS.md
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
├── LICENSE
├── pyproject.toml
└── README.md
```

## 22. Versioning and Compatibility

The project is currently version 0.1.2 and classified as alpha.

Until version 1.0:

- Python signatures may evolve;
- telemetry fields may be added or renamed;
- the C structure layout may change;
- wheel and platform coverage may change;
- lifecycle behavior may be redesigned around explicit native contexts.

Pin a tested version in production:

```text
gpumetric==0.1.2
```

## 23. License

GPUMetric is distributed under the GNU General Public License v3.0. NVIDIA NVML is supplied by the NVIDIA driver and remains subject to NVIDIA's applicable license terms.
