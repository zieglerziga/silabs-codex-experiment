#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
actionlint_image='rhysd/actionlint@sha256:b1934ee5f1c509618f2508e6eb47ee0d3520686341fec936f3b79331f9315667'
zizmor_image='ghcr.io/zizmorcore/zizmor@sha256:a2eb396d886c053073405c7a980f2139ba2248ec172243cfa3841e57196e8101'

run_read_only() {
  image=$1
  shift

  docker run --rm \
    --cap-drop=ALL \
    --network=none \
    --read-only \
    --security-opt=no-new-privileges \
    --volume "$repo_root:/repo:ro" \
    --workdir /repo \
    "$image" "$@"
}

run_read_only "$actionlint_image" -color
run_read_only "$zizmor_image" --format plain .github/workflows
