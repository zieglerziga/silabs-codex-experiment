#!/bin/sh
set -eu

usage() {
  echo "usage: $0 check|test|sanitize" >&2
  exit 2
}

mode=${1:-}
case "$mode" in
  check|test|sanitize) ;;
  *) usage ;;
esac

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$repo_root/build/host"
sanitizer_options=

if [ "$mode" = sanitize ]; then
  build_dir="$repo_root/build/host-sanitize"
  sanitizer_options="-fsanitize=address,undefined -fno-omit-frame-pointer"
fi

git -C "$repo_root" diff --check
cmake -S "$repo_root" -B "$build_dir" \
  -DBUILD_TESTING=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="$sanitizer_options" \
  -DCMAKE_EXE_LINKER_FLAGS="$sanitizer_options"
cmake --build "$build_dir" --parallel

if [ "$mode" = test ] || [ "$mode" = sanitize ]; then
  ctest --test-dir "$build_dir" --output-on-failure
fi
