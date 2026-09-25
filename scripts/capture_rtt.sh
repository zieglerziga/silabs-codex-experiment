#!/bin/sh
set -eu

duration_seconds=${1:-10}
case "$duration_seconds" in
  ''|*[!0-9]*)
    echo "duration must be a positive integer number of seconds" >&2
    exit 2
    ;;
  0)
    echo "duration must be greater than zero" >&2
    exit 2
    ;;
esac

if ! command -v commander >/dev/null 2>&1; then
  echo "Simplicity Commander is required" >&2
  exit 1
fi
if ! command -v timeout >/dev/null 2>&1; then
  echo "GNU timeout is required" >&2
  exit 1
fi

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output="$repo_root/build/rtt.log"
firmware="$repo_root/firmware/ble_scanner/ble_scanner_cmake/build/default_config/ble_scanner.out"

if [ ! -f "$firmware" ]; then
  echo "firmware ELF is missing; run make firmware" >&2
  exit 1
fi
if ! command -v arm-none-eabi-nm >/dev/null 2>&1; then
  echo "arm-none-eabi-nm is required to locate the RTT control block" >&2
  exit 1
fi

rtt_address=$(arm-none-eabi-nm "$firmware" | awk '$3 == "_SEGGER_RTT" { print "0x" $1 }')
if [ -z "$rtt_address" ]; then
  echo "_SEGGER_RTT is missing from the firmware ELF" >&2
  exit 1
fi

cmake -E make_directory "$repo_root/build"
cmake -E rm -f "$output"

set +e
timeout --signal=INT --kill-after=2 "${duration_seconds}s" \
  commander rtt connect \
  --device EFR32MG21A010F1024IM32 \
  --tif SWD \
  --speed 4000 \
  --blockaddress "$rtt_address" \
  --readbuffer 0 >"$output" 2>&1
status=$?
set -e

case "$status" in
  0|124|130) ;;
  *)
    cat "$output" >&2
    exit "$status"
    ;;
esac

if ! grep -Eq 'BLE scanner initialized|BLE scan started|^scan |^summary ' "$output"; then
  cat "$output" >&2
  echo "No application RTT data captured in ${duration_seconds}s" >&2
  exit 1
fi

printf '\nCaptured RTT output:\n'
sed -n '1,240p' "$output"
