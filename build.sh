#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

echo "============================================"
echo "  Simou3Camera SDK build (Linux, Release)"
echo "  out dir: linux_build/"
echo "============================================"

command -v cmake >/dev/null || { echo "[ERROR] not found cmake (sudo apt install cmake)"; exit 1; }
command -v g++   >/dev/null || { echo "[ERROR] not found g++ (sudo apt install g++)"; exit 1; }

JOBS=$(nproc 2>/dev/null || echo 4)

cmake -S . -B linux_build -DCMAKE_BUILD_TYPE=Release
cmake --build linux_build -j"${JOBS}"
