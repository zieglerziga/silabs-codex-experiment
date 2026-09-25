#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cmake_dir="$repo_root/firmware/ble_scanner/ble_scanner_cmake"

if [ -z "${SISDK_ROOT:-}" ]; then
  echo "SISDK_ROOT must point to Simplicity SDK v2025.6.3" >&2
  exit 1
fi
if [ ! -f "$cmake_dir/ble_scanner.cmake" ]; then
  echo "generated firmware files are missing; run make generate-firmware" >&2
  exit 1
fi

python3 "$repo_root/scripts/prepare_sdk.py" \
  check \
  "$SISDK_ROOT" \
  "$cmake_dir/ble_scanner.cmake"

cd "$cmake_dir"
cmake --workflow --preset project --fresh
