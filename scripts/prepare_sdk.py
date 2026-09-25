#!/usr/bin/env python3
"""Check or materialize Git LFS archives required by the generated firmware."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess


ARCHIVE_PATTERN = re.compile(r'"\$\{SDK_PATH\}/([^"]+\.a)"')
LFS_HEADER = b"version https://git-lfs.github.com/spec/v1\n"


def selected_archives(inventory: Path) -> list[Path]:
    source = inventory.read_text(encoding="utf-8")
    return sorted({Path(match) for match in ARCHIVE_PATTERN.findall(source)})


def missing_lfs_objects(sdk_root: Path, inventory: Path) -> list[Path]:
    missing: list[Path] = []
    for relative_path in selected_archives(inventory):
        archive = sdk_root / relative_path
        if not archive.is_file():
            raise SystemExit(f"selected SDK archive is missing: {archive}")
        with archive.open("rb") as stream:
            if stream.read(len(LFS_HEADER)) == LFS_HEADER:
                missing.append(relative_path)
    return missing


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("check", "fetch"))
    parser.add_argument("sdk_root", type=Path)
    parser.add_argument("inventory", type=Path)
    args = parser.parse_args()

    missing = missing_lfs_objects(args.sdk_root, args.inventory)
    if not missing:
        print("All selected SDK archives are materialized.")
        return

    if args.mode == "check":
        for relative_path in missing:
            print(f"Git LFS pointer: {relative_path}")
        raise SystemExit("run 'make prepare-sdk' to materialize these archives")

    include = ",".join(path.as_posix() for path in missing)
    subprocess.run(
        [
            "git",
            "-C",
            str(args.sdk_root),
            "lfs",
            "pull",
            f"--include={include}",
        ],
        check=True,
    )

    remaining = missing_lfs_objects(args.sdk_root, args.inventory)
    if remaining:
        raise SystemExit("Git LFS completed but one or more archives remain pointers")
    print(f"Materialized {len(missing)} SDK archive(s).")


if __name__ == "__main__":
    main()
