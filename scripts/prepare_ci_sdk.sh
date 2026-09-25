#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
sdk_root=${1:-}
sdk_repository=https://github.com/SiliconLabs/simplicity_sdk.git
sdk_commit=b41bec3ff2485199c1a5a9995b3e649e118c1b8d

if [ -z "$sdk_root" ]; then
  echo "usage: $0 /absolute/empty/sdk/path" >&2
  exit 2
fi
case "$sdk_root" in
  /*) ;;
  *)
    echo "SDK destination must be an absolute path: $sdk_root" >&2
    exit 2
    ;;
esac
if [ -e "$sdk_root" ]; then
  echo "SDK destination already exists: $sdk_root" >&2
  exit 2
fi

git lfs version
mkdir -p "$(dirname -- "$sdk_root")"
git init --quiet "$sdk_root"
git -C "$sdk_root" lfs install --local
git -C "$sdk_root" remote add origin "$sdk_repository"
git -C "$sdk_root" -c protocol.version=2 fetch \
  --depth=1 --no-tags origin "$sdk_commit"
GIT_LFS_SKIP_SMUDGE=1 git -C "$sdk_root" checkout \
  --quiet --detach "$sdk_commit"

resolved_commit=$(git -C "$sdk_root" rev-parse HEAD)
if [ "$resolved_commit" != "$sdk_commit" ]; then
  echo "unexpected SDK commit: $resolved_commit" >&2
  exit 1
fi

SISDK_ROOT="$sdk_root" make -C "$repo_root" prepare-sdk
