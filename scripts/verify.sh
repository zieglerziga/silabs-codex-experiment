#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

for script in scripts/*.sh; do
  sh -n "$script"
done

python3 -c 'import pathlib; [compile(path.read_text(), str(path), "exec") for path in pathlib.Path("scripts").glob("*.py")]'
python3 -m unittest discover -s tests -p 'test_*.py'

make format-check
make check
make test
make sanitize
git diff --check
