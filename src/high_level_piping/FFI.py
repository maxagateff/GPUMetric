import ctypes
import os
from pathlib import Path

class GPUState(ctypes.Structure):
    """
    Represents the GPUStats C-structure in Python.
    Matches data types strictly with uint32_t, uint64_t, and int64_t.
    """
    _fields_ = [
        ("temp", ctypes.c_uint32),
        ("util", ctypes.c_uint32),
        ("mem_mb", ctypes.c_ulonglong),
        ("delta_mb", ctypes.c_int64),  # Fixed: explicit 64-bit integer to prevent Windows x64 truncation
    ]

class GPUMetrics:
    """
    Python wrapper for the C GPU metrics library using ctypes FFI.
    """
    def __init__(self, lib_path: str, device_index: int):
        # Resolve absolute path and load the shared library (.so / .dll)
        resolved_path = str(Path(lib_path).resolve())
        self.lib = ctypes.CDLL(resolved_path, device_index)

        # Configure C function prototypes
        self.lib.gpu_metric_init.argtypes = []
        self.lib.gpu_metric_init.restype = ctypes.c_int

        self.lib.gpu_metric_sample.argtypes = [ctypes.POINTER(GPUState)]
        self.lib.gpu_metric_sample.restype = ctypes.c_int

        self.lib.gpu_metric_cleanup.argtypes = []
        self.lib.gpu_metric_cleanup.restype = None

        # Initialize the NVML library context
        ret = self.lib.gpu_metric_init()
        if ret != 0:
            raise RuntimeError(f"gpu_metric_init failed with error code: {ret}")

    def samples(self) -> GPUState:
        """
        Samples the current GPU metrics.
        :return: GPUState object containing hardware metrics.
        :raises RuntimeError: If the C library fails to query the GPU.
        """
        stats = GPUState()
        ret = self.lib.gpu_metric_sample(ctypes.byref(stats))

        if ret != 0:
            raise RuntimeError(f"gpu_metric_sample failed with error code: {ret}")

        # Fixed: return only the stats object since 'ret' is always 0 here
        return stats

    def __del__(self):
        """
        Ensures proper resource cleanup when the Python object is destroyed.
        """
        try:
            self.lib.gpu_metric_cleanup()
        except AttributeError:
            # Handle case where library loading failed in __init__
            pass
#TODO :)