#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
GPU_INDEX="${1:-0}"

if ! command -v nvidia-smi >/dev/null 2>&1; then
    echo "[ERROR] nvidia-smi not found"
    exit 1
fi

if ! [[ "$GPU_INDEX" =~ ^[0-9]+$ ]]; then
    echo "[ERROR] Invalid GPU index: $GPU_INDEX"
    exit 1
fi

GPU_UUID="$(nvidia-smi --id="$GPU_INDEX" --query-gpu=uuid --format=csv,noheader | tr -d '[:space:]')"

if [[ -z "$GPU_UUID" ]]; then
    echo "[ERROR] GPU UUID not found for index $GPU_INDEX"
    exit 1
fi

echo "[GPUMetric] GPU index: $GPU_INDEX"
echo "[GPUMetric] GPU UUID:  $GPU_UUID"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DGPUMETRIC_TEST_REAL_GPU=ON -DGPUMETRIC_REAL_GPU_INDEX="$GPU_INDEX" -DGPUMETRIC_REAL_GPU_UUID="$GPU_UUID"
cmake --build "$BUILD_DIR" --target test_gpu_real --parallel
ctest --test-dir "$BUILD_DIR" -R '^gpu_real$' --output-on-failure --verbose --no-tests=error