# GPUMetric
## IMPORTANT: the code is currently being updated

Simple and lightweight C wrapper around NVIDIA NVML with Python FFI bindings. 
Built to fetch GPU temperature, utilization, and memory changes (delta) without heavy dependencies. Designed specifically for headless Ubuntu Servers.

---

## Technical Features & Gotchas (Read This First)

* **Zero-allocation sampling:** The C core uses a static ring-buffer (`MemoryCache`) with a fixed size of 16 slots to track historical data and calculate memory diffs. No `malloc` calls in runtime.
* **Single GPU limitation:** Currently hardcoded to track only the first GPU (`index 0`). If you have a multi-GPU rig, it won't see other cards yet.
* **Ctypes sync:** Data types are strictly mapped between C and Python (`uint32_t`, `uint64_t`, `int64_t` -> `ctypes.c_uint32`, etc.). This prevents memory corruption and clipping bugs on Windows/Linux x64 platforms.

---

## Prerequisites

Before building, make sure you actually have an NVIDIA GPU and proprietary drivers installed. 

Install the required build tools and NVML development headers on Ubuntu Server:
```bash
sudo apt update
sudo apt install -y build-essential cmake libnvidia-ml-dev
```
*Note: If `cmake` fails later, double check that `nvidia-smi` works. If the driver is not loaded into the kernel, NVML initialization will crash with error code `-1`.*

---

## Build Instruction

We use CMake for an out-of-source build to keep the root directory clean. 

```bash
mkdir build && cd build
# Generate build files with production optimizations (-O3)
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
This produces `libgpumetric.so` inside the `build/` directory.

---

## Project Structure

* `gpu_metric.c` / `gpu_metric.h` — Core logic, ring-buffer for memory tracking, and NVML communication.
* `logger.h` — Basic macros with ANSI color codes for debugging errors in stdout.
* `FFI.py` — Python wrapper class using `ctypes`. Automatically handles `gpu_metric_cleanup()` via `__del__`.
* `tests/test_run.py` — Live monitoring script.

---

## How to Run & Test

To run the monitoring test script, execute it from the project root directory so Python can resolve the internal package paths correctly:

```bash
# Must be executed from the root directory
python3 tests/test_run.py
```

The script dynamically calculates the path to `build/libgpumetric.so`, hooks into NVML, and prints stats to the terminal every second.

---

## Error Codes Reference

If something breaks, the C library returns negative enums instead of crashing:
* `0`: `GPU_METRIC_SUCCESS` — All good.
* `-1`: `GPU_METRIC_ERR_NVML` — NVML failed to init (usually missing or dead driver).
* `-2`: `GPU_METRIC_ERR_NO_DEVICE` — No NVIDIA cards found.
* `-3`: `GPU_METRIC_ERR_DEVICE` — Failed to communicate or fetch metrics from hardware.
* `-4`: `GPU_METRIC_ERR_ARGUMENT` — Passed a NULL pointer to the sampler.

---

## License

MIT License. Do whatever you want with the code, just don't blame me if your server melts

