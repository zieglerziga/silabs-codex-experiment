#!/usr/bin/env python3
"""Transactionally install normalized SLC output into the project tree."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil


GENERATED_PATHS = ("autogen", "config", "ble_scanner_cmake", "main.c")


def install(staging_dir: Path, project_dir: Path) -> None:
    for name in GENERATED_PATHS:
        if not (staging_dir / name).exists():
            raise SystemExit(f"generated output is missing {name}")

    backup_dir = staging_dir / ".previous"
    backup_dir.mkdir()
    backed_up: list[str] = []
    installed: list[str] = []

    try:
        for name in GENERATED_PATHS:
            destination = project_dir / name
            if destination.exists():
                os.replace(destination, backup_dir / name)
                backed_up.append(name)

        for name in GENERATED_PATHS:
            os.replace(staging_dir / name, project_dir / name)
            installed.append(name)
    except BaseException:
        for name in reversed(installed):
            os.replace(project_dir / name, staging_dir / name)
        for name in reversed(backed_up):
            os.replace(backup_dir / name, project_dir / name)
        raise

    shutil.rmtree(backup_dir)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("staging_dir", type=Path)
    parser.add_argument("project_dir", type=Path)
    args = parser.parse_args()
    install(args.staging_dir, args.project_dir)


if __name__ == "__main__":
    main()
