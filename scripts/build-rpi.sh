#!/usr/bin/env bash
# build-rpi.sh — Install deps, build, and optionally run on a Raspberry Pi (Debian/Ubuntu).
# Usage:
#   ./scripts/build-rpi.sh              # build only
#   ./scripts/build-rpi.sh --run DEVICE  # build and run (e.g. --run /dev/ttyUSB0)
set -euo pipefail

BUILD_TYPE="${BUILD_TYPE:-Release}"
RUN_DEVICE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --run) RUN_DEVICE="$2"; shift 2 ;;
        --debug) BUILD_TYPE="Debug"; shift ;;
        *) echo "Unknown arg: $1"; exit 1 ;;
    esac
done

echo "==> Build type: ${BUILD_TYPE}"

# --- 1. System dependencies (idempotent) ---
if ! command -v cmake &>/dev/null || ! command -v g++ &>/dev/null; then
    echo "==> Installing system packages..."
    sudo apt-get update -qq
    sudo apt-get install -y -qq cmake g++ python3-pip python3-venv
fi

# --- 2. Python venv with Conan ---
VENV_DIR=".venv-build"
echo "==> Setting up build venv at ${VENV_DIR}..."
python3 -m venv "${VENV_DIR}"
source "${VENV_DIR}/bin/activate"
pip install --quiet --upgrade pip conan

# Ensure a default profile exists
conan profile detect --exist-ok 2>/dev/null || conan profile detect

# --- 3. Conan install ---
echo "==> Running conan install..."
conan install . --build=missing -s build_type="${BUILD_TYPE}"

# --- 4. CMake configure + build ---
PRESET="conan-$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"

echo "==> Configuring with preset: ${PRESET}"
cmake --preset "${PRESET}"

echo "==> Building..."
cmake --build --preset "${PRESET}" -j "$(nproc)"

BINARY="build/${BUILD_TYPE}/bluetooth-at-driver"
echo "==> Build complete: ${BINARY}"

# --- 5. Cleanup venv ---
deactivate
rm -rf "${VENV_DIR}"
echo "==> Cleaned up build venv"

# --- 5. Run (if requested) ---
if [[ -n "${RUN_DEVICE}" ]]; then
    echo "==> Running: ${BINARY} -s --stat ${RUN_DEVICE}"
    exec "${BINARY}" -s --stat "${RUN_DEVICE}"
fi
