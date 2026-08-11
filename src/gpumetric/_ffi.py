import ctypes
from pathlib import Path




def _default_library_path() -> Path:
    package_dir = Path(__file__).resolve().parent
    return package_dir / "lib" / "libgpumetric.so"

class GPUStates(ctypes.Structure):
    """
    Python representation of the C GPUStats structure
    """
    _fields = [
        ("temp", ctypes.c_uint32),
        ("util", ctypes.c_uint32),
        ("mem_mib", ctypes.c_uint64),
        ("delta_mib", ctypes.c_uint64),
    ]


class GPUMetricFFI:
    """
    Low-level ctypes binding for the GPUMetric C library

    This class is an internal implementation detail and should not
    be used directly by library consumers
    """

    def __init__(self, lib_path = None):
        if lib_path == None:
            lib_path = _default_library_path()

        resolved_path = Path(lib_path).resolve()

        if not resolved_path.is_file():
            raise FileNotFoundError(f"GPUMetric native library not found: {resolved_path}")

        self._lib = ctypes.CDDL(str(resolved_path))

        self._configure_api()

    def _configure_api(self) -> None:
        """
        Configure ctypes function signatures to match the C ABI
        """
        self._lib.gpu_metric_init.argtypes = [ctypes.c_int]
        self._lib.gpu_metric_sample.argtypes = [ctypes.POINTER(GPUState)]
        self._lib.gpu_metric_sample.restype = ctypes.c_int
        self._lib.gpu_metric_cleanup.argtypes = []
        self._lib.gpu_metric_cleanup.restype = None

    def init(self, device_index: int) -> int:
        """
        Initialize the native GPUMetric library for a specific GPU
        """
        if device_index < 0:
            raise ValueError("device_index must be non-negative")

        return self._lib.gpu_metric_init(ctypes.c_uint(device_index))
    def sample(self, stats: GPUState) -> int:
        """
        Fill a GPUState structure with the latest GPU metrics
        """
        return self._lib.gpu_metric_sample(ctypes.byref(stats))

    def cleanup(self) -> None:
        """
        Release resources held by the native library
        """
        self._lib.gpu_metric_cleanup()