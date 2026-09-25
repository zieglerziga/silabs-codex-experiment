#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
image="$repo_root/firmware/ble_scanner/ble_scanner_cmake/build/default_config/ble_scanner.hex"

if [ ! -f "$image" ]; then
  echo "firmware image is missing; run make firmware" >&2
  exit 1
fi
if ! command -v commander >/dev/null 2>&1; then
  echo "Simplicity Commander is required" >&2
  exit 1
fi

commander flash "$image" \
  --device EFR32MG21A010F1024IM32 \
  --tif SWD
