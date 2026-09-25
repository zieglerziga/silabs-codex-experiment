#!/bin/sh
set -eu

usage() {
  echo "usage: $0 image|verify|firmware" >&2
  exit 2
}

mode=${1:-}
case "$mode" in
  image|verify|firmware) ;;
  *) usage ;;
esac

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
image=${DOCKER_IMAGE:-silabs-ble-scanner-dev:ubuntu24.04}

build_image() {
  docker build --file "$repo_root/Dockerfile" --tag "$image" "$repo_root"
}

if [ "$mode" = image ]; then
  build_image
  exit 0
fi

if ! docker image inspect "$image" >/dev/null 2>&1; then
  build_image
fi

set -- docker run --rm \
  --cap-drop=ALL \
  --network=none \
  --read-only \
  --security-opt=no-new-privileges \
  --tmpfs /tmp:rw,nosuid,nodev,size=256m \
  --user "$(id -u):$(id -g)" \
  --env BUILD_ROOT=/workspace/build/docker \
  --env HOME=/tmp \
  --volume "$repo_root:/workspace" \
  --workdir /workspace

if [ "$mode" = firmware ]; then
  if [ -z "${SISDK_ROOT:-}" ] || [ ! -d "$SISDK_ROOT" ]; then
    echo "SISDK_ROOT must point to Simplicity SDK v2025.6.3" >&2
    exit 1
  fi
  set -- "$@" \
    --env SISDK_ROOT=/opt/simplicity_sdk \
    --volume "$SISDK_ROOT:/opt/simplicity_sdk:ro"
fi

exec "$@" "$image" make "$mode"
