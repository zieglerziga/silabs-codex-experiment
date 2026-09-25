#!/usr/bin/env python3
"""Make SLC output reproducible and independent of the generating host."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil


SDK_PATTERN = re.compile(r'^set\(SDK_PATH\s+"[^"]*"\)\s*$', re.MULTILINE)
PACKAGE_PATTERN = re.compile(r'^set\(PKG_PATH\s+"[^"]*"\)\s*$', re.MULTILINE)
HOST_PATH_PATTERN = re.compile(
    r"(?:/home/[^/]+|/Users/[^/]+|[A-Za-z]:[/\\]Users[/\\][^/\\]+)"
)
PORTABLE_SDK_BLOCK = """if(NOT DEFINED ENV{SISDK_ROOT} OR \"$ENV{SISDK_ROOT}\" STREQUAL \"\")
  message(FATAL_ERROR \"SISDK_ROOT must point to Simplicity SDK v2025.6.3\")
endif()
set(SDK_PATH \"$ENV{SISDK_ROOT}\")"""
PORTABLE_PACKAGE_LINE = 'set(PKG_PATH "$ENV{SILABS_PACKAGE_ROOT}")'
TEXT_SUFFIXES = {
    ".c",
    ".cmake",
    ".h",
    ".json",
    ".ld",
    ".md",
    ".properties",
    ".txt",
}


def replace_exactly_once(
    pattern: re.Pattern[str], replacement: str, source: str, label: str, path: Path
) -> str:
    updated, replacements = pattern.subn(replacement, source, count=1)
    if replacements != 1:
        raise SystemExit(f"expected one {label} assignment in {path}")
    return updated


def generated_text_files(project_dir: Path) -> list[Path]:
    roots = [
        project_dir / "autogen",
        project_dir / "config",
        project_dir / "ble_scanner_cmake",
    ]
    files = [project_dir / "main.c"]
    for root in roots:
        files.extend(path for path in root.rglob("*") if path.is_file())
    return sorted(path for path in files if path.suffix in TEXT_SUFFIXES)


def normalize_whitespace(path: Path) -> None:
    lines = path.read_text(encoding="utf-8").splitlines()
    normalized = "\n".join(line.rstrip() for line in lines).rstrip() + "\n"
    path.write_text(normalized, encoding="utf-8")


def normalize(project_dir: Path, toolchain_template: Path) -> None:
    cmake_dir = project_dir / "ble_scanner_cmake"
    inventory = cmake_dir / "ble_scanner.cmake"
    source = inventory.read_text(encoding="utf-8")
    source = replace_exactly_once(
        SDK_PATTERN, PORTABLE_SDK_BLOCK, source, "SDK_PATH", inventory
    )
    source = replace_exactly_once(
        PACKAGE_PATTERN, PORTABLE_PACKAGE_LINE, source, "PKG_PATH", inventory
    )
    inventory.write_text(source, encoding="utf-8")

    shutil.copyfile(toolchain_template, cmake_dir / "toolchain.cmake")

    for generated_file in generated_text_files(project_dir):
        normalize_whitespace(generated_file)
        content = generated_file.read_text(encoding="utf-8")
        match = HOST_PATH_PATTERN.search(content)
        if match is not None:
            raise SystemExit(
                f"host-specific path remains in {generated_file}: {match.group(0)}"
            )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("project_dir", type=Path)
    parser.add_argument("toolchain_template", type=Path)
    args = parser.parse_args()
    normalize(args.project_dir, args.toolchain_template)


if __name__ == "__main__":
    main()
