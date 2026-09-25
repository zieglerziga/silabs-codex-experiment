#!/bin/sh
set -eu

usage() {
  echo "usage: $0 check|write" >&2
  exit 2
}

mode=${1:-}
case "$mode" in
  check|write) ;;
  *) usage ;;
esac

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required" >&2
  exit 1
fi

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

git -C "$repo_root" ls-files --cached --others --exclude-standard \
  '*.c' '*.h' |
while IFS= read -r source_file; do
  [ -n "$source_file" ] || continue
  case "$source_file" in
    firmware/ble_scanner/autogen/*|firmware/ble_scanner/config/*|firmware/ble_scanner/main.c)
      continue
      ;;
  esac
  if [ "$mode" = write ]; then
    clang-format -i "$repo_root/$source_file"
  else
    clang-format --dry-run --Werror "$repo_root/$source_file"
  fi
done
