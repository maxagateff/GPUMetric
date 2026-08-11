import ctypes
from pathlib import Path

class GPUStates(ctypes.Structure):
    """
    internal ctypes representation of the C GPUStates structure

    this structure must match the C ABI exactly
    """

    _fields_ = [
        ("temp", ctypes.c_uint32),
        ("utils", ctypes.c_uint32),
        ("mem_mib", ctypes.c_uint64),
        ("delta_mib", ctypes.c_uint64),
    ]
def _default_library_path() -> Path:
    """
    return the path to the bundled native GPUMetric library
    """
    package_dir = Path(__file__).resolve().parent
    return package_dir / "lib" / "libgpumetric.so"

class GPUMetricFFI:
    """
    internal los-level ctypes binding for the GPUMetric C library

    this class is an implementation detail & should not be used
    directly by library consumers
    """

    def __init__(self, lib_path: str | Path | None = None) -> None:
        if lib_path is None:
            lib_path = _default_library_path()

        resolved_path = Path(lib_path).resolve()

        if not resolved_path.is_file():
            raise FileNotFoundError(f"GPUMetric native library not found: {resolved_path}")

        self._lib = ctypes.CDLL(str(resolved_path))

        self._configure_api()

    def _configure_api(self) -> None:
        """
        configure ctypes function signatures to match the C ABI
        """
        self._lib.gpu_metric_init.argtypes = [ctypes.c_uint]

        self._lib.gpu_metric_init.restype = ctypes.c_int

        self._lib.gpu_metric_sample.argtypes = [ctypes.POINTER(GPUStates)]
        self._lib.gpu_metric_sample.restype = ctypes.c_int

        self._lib.gpu_metric_cleanup.argtypes = []
        self._lib.gpu_metric_cleanup.restype = None

    def init(self, device_index: int) -> int:
        """
        Initialize the native GPUMetric library for a specific GPU.
        """
        if not isinstance(device_index, int):
            raise TypeError("device_index must be an integer")

        if device_index < 0:
            raise ValueError("device_index must be non-negative")

        return self._lib.gpu_metric_init(ctypes.c_uint(device_index))

    def sample(self, stats: GPUStates) -> int:
        """
        fill a GPUStates structure with the latest GPU metrics
        """

        if not isinstance(stats, GPUStates):
            raise TypeError("stats must be an instance of GPUStates")

        return self._lib.gpu_metric_sample(ctypes.byref(stats))

    def cleanup(self) -> None:
        """
        release resources held by the native library
        """
        self._lib.gpu_metric_cleanup()

