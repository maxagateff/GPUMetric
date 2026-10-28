#!/bin/bash
set -e

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON \
    -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror"

cmake --build build --parallel
ctest --test-dir build --output-on-failure --no-tests=error

