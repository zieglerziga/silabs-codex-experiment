#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
project_dir="$repo_root/firmware/ble_scanner"
project_file="$project_dir/ble_scanner.slcp"
slc_cli=${SLC_CLI:-slc}

if [ -z "${SISDK_ROOT:-}" ]; then
  echo "SISDK_ROOT must point to Simplicity SDK v2025.6.3" >&2
  exit 1
fi
if [ ! -f "$SISDK_ROOT/simplicity_sdk.slcs" ]; then
  echo "SISDK_ROOT does not contain simplicity_sdk.slcs: $SISDK_ROOT" >&2
  exit 1
fi
if ! command -v "$slc_cli" >/dev/null 2>&1 && [ ! -x "$slc_cli" ]; then
  echo "SLC CLI not found; set SLC_CLI to its executable" >&2
  exit 1
fi

# SLC configuration headers contain machine-readable comment annotations.
# Recreate all generated content so unrelated formatters or stale incremental
# state cannot make generation non-deterministic.
cmake -E remove_directory "$project_dir/autogen"
cmake -E remove_directory "$project_dir/config"
cmake -E remove_directory "$project_dir/ble_scanner_cmake"
cmake -E rm -f "$project_dir/main.c"

"$slc_cli" generate \
  --project-file "$project_file" \
  --sdk "$SISDK_ROOT" \
  --export-destination "$project_dir" \
  --output-type cmake \
  --toolchain gcc \
  --overwrite-all \
  --require-clean-project

python3 "$repo_root/scripts/normalize_slc_output.py" \
  "$project_dir" \
  "$repo_root/cmake/arm-gcc-toolchain.cmake"
