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
# Generate and normalize a complete replacement before touching tracked output.
staging_dir=$(mktemp -d "$project_dir/.slc-generate.XXXXXX")
cleanup() {
  cmake -E remove_directory "$staging_dir"
}
trap cleanup EXIT HUP INT TERM

cmake -E make_directory "$staging_dir/inc" "$staging_dir/src"
cmake -E copy "$project_file" "$staging_dir/ble_scanner.slcp"
cmake -E copy "$project_dir/app.c" "$project_dir/app.h" "$staging_dir"
cmake -E copy "$project_dir/inc/scan_tracker.h" "$project_dir/inc/rtt_command.h" \
  "$staging_dir/inc"
cmake -E copy "$project_dir/src/scan_tracker.c" "$staging_dir/src"

"$slc_cli" generate \
  --project-file "$staging_dir/ble_scanner.slcp" \
  --sdk "$SISDK_ROOT" \
  --export-destination "$staging_dir" \
  --output-type cmake \
  --toolchain gcc \
  --overwrite-all \
  --require-clean-project

cmake -E copy \
  "$repo_root/cmake/ble-scanner-project.cmake" \
  "$staging_dir/ble_scanner_cmake/ble_scanner_project.cmake"

python3 "$repo_root/scripts/normalize_slc_output.py" \
  "$staging_dir" \
  "$repo_root/cmake/arm-gcc-toolchain.cmake"

python3 "$repo_root/scripts/install_generated_output.py" \
  "$staging_dir" \
  "$project_dir"
