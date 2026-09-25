#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

verify_diff_range=
if [ -n "${VERIFY_BASE:-}" ] || [ -n "${VERIFY_HEAD:-}" ]; then
  if [ -z "${VERIFY_BASE:-}" ] || [ -z "${VERIFY_HEAD:-}" ]; then
    echo "VERIFY_BASE and VERIFY_HEAD must be set together" >&2
    exit 2
  fi
  verify_diff_range="${VERIFY_BASE}...${VERIFY_HEAD}"
fi

for script in scripts/*.sh; do
  sh -n "$script"
done

python3 -c 'import pathlib; [compile(path.read_text(), str(path), "exec") for path in pathlib.Path("scripts").glob("*.py")]'
python3 -m unittest discover -s tests -p 'test_*.py'

make format-check
make check
make test
make sanitize

if [ -n "$verify_diff_range" ]; then
  git diff --check "$verify_diff_range"
else
  git diff --check
fi
