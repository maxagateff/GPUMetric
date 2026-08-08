import time
import sys
from pathlib import Path

project_root = str(Path(__file__).resolve().parent.parent)
if project_root not in sys.path:
    sys.path.insert(0, project_root)

from FFI import GPUMetrics


def main():

    lib_path = str(Path(__file__).resolve().parent.parent / "build" / "libgpumetric.so")

    print("Connecting to GPU metrics library...")
    try:
        metrics = GPUMetrics(lib_path)
        print("Connected successfully. Monitoring started (Ctrl+C to exit).\n")
    except RuntimeError as e:
        print(f"Initialization failed: {e}")
        sys.exit(1)
    
    print(f"{'Temp (°C)':<10} | {'Util (%)':<10} | {'Memory (MB)':<12} | {'Delta (MB)':<10}")
    print("-" * 52)
    
    try:
        while True:
            stats = metrics.samples()
            
            print(f"{stats.temp:<10} | {stats.util:<10} | {stats.mem_mb:<12} | {stats.delta_mb:<10}")
            
            time.sleep(1.0)
            
    except KeyboardInterrupt:
        print("\nMonitoring stopped by user.")

if __name__ == "__main__":
    main()

