# install_deps.sh
# Update package lists and install basic build tools (gcc, cmake)
sudo apt update && sudo apt install -y build-essential libnvidia-ml-dev cmake

# Note: NVIDIA NVML comes bundled with the proprietary NVIDIA drivers.
# To check if it's available, look for libnvml.so in the system.
