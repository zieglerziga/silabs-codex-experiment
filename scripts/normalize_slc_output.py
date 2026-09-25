#!/usr/bin/env python3
"""Make SLC output reproducible and independent of the generating host."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil


SDK_PATTERN = re.compile(r'^set\(SDK_PATH\s+"[^"]*"\)\s*$', re.MULTILINE)
PACKAGE_PATTERN = re.compile(r'^set\(PKG_PATH\s+"[^"]*"\)\s*$', re.MULTILINE)
STUDIO_METADATA_PATTERN = re.compile(
    r"^# BEGIN_SIMPLICITY_STUDIO_METADATA=.*=END_SIMPLICITY_STUDIO_METADATA\s*$",
    re.MULTILINE,
)
QUOTED_STRING_PATTERN = re.compile(r'"([^"\r\n]*)"')
QUOTED_ABSOLUTE_PATH_PATTERN = re.compile(
    r"(?:^|[\s;,=]|-[IL])((?:"
    r"/(?!/)[^\s;]*"
    r"|[A-Za-z]:[/\\][^\s;]*"
    r"|(?:\\\\|//)[^\\/\s;]+[\\/][^\\/\s;]+[^\s;]*"
    r"))"
)
QUOTED_COLON_PATH_PATTERN = re.compile(
    r":((?:/(?!/)[^\s;]*|[A-Za-z]:[/\\][^\s;]*|"
    r"\\\\[^\\/\s;]+[\\/][^\\/\s;]+[^\s;]*))"
)
CMAKE_GENERATOR_PATH_PATTERN = re.compile(
    r"\$<(?:BUILD_INTERFACE|INSTALL_INTERFACE):((?:"
    r"/(?!/)[^\s;>]*"
    r"|[A-Za-z]:[/\\][^\s;>]*"
    r"|(?:\\\\|//)[^\\/\s;>]+[\\/][^\\/\s;>]+[^\s;>]*"
    r"))>"
)
UNQUOTED_ABSOLUTE_PATH_PATTERN = re.compile(
    r"(?:^|[\s(;,=]|-[IL])((?:"
    r"/(?![/*\s])"
    r"|[A-Za-z]:[/\\]"
    r"|//[^/\s,;]+/[^/\s,;)]*"
    r"|\\\\[^\\/\s;]+[\\/][^\\/\s;)]+"
    r")[^\s;)]*)",
    re.MULTILINE,
)
LINKER_SINGLE_PATH_PATTERN = re.compile(
    r"\b(?:INCLUDE|OUTPUT|SEARCH_DIR|STARTUP)\s*(?:\(\s*)?((?:"
    r"/(?![/*\s])"
    r"|[A-Za-z]:[/\\]"
    r"|//[^/\s,;]+/[^/\s,;)]*"
    r"|\\\\[^\\/\s;]+[\\/][^\\/\s;)]+"
    r")[^\s;)]*)"
)
LINKER_LIST_PATTERN = re.compile(r"\b(?:GROUP|INPUT)\s*\(([^)]*)\)", re.MULTILINE)
LINKER_OPERAND_PATH_PATTERN = re.compile(
    r"(?:^|[\s,(])((?:"
    r"/(?![/*\s])"
    r"|[A-Za-z]:[/\\]"
    r"|//[^/\s,;]+/[^/\s,;)]*"
    r"|\\\\[^\\/\s,;]+[\\/][^\\/\s,;)]+"
    r")[^\s,;)]*)"
)
PORTABLE_SDK_BLOCK = """if(NOT DEFINED ENV{SISDK_ROOT} OR \"$ENV{SISDK_ROOT}\" STREQUAL \"\")
  message(FATAL_ERROR \"SISDK_ROOT must point to Simplicity SDK v2025.6.3\")
endif()
set(SDK_PATH \"$ENV{SISDK_ROOT}\")"""
PORTABLE_PACKAGE_LINE = 'set(PKG_PATH "$ENV{SILABS_PACKAGE_ROOT}")'
PORTABLE_METADATA_LINE = (
    "# Simplicity Studio metadata removed: ble_scanner.slcp is the source of truth."
)
APP_HEADER_INCLUDE = '#include "app.h"'
MAIN_INCLUDE_MARKER = '#include "sl_component_catalog.h"'
EXPECTED_PROJECT_REFERENCES = (
    '"../app.c"',
    '"../src/scan_tracker.c"',
    '"../inc"',
)
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
    replacements = len(pattern.findall(source))
    if replacements != 1:
        raise SystemExit(
            f"expected exactly one {label} assignment in {path}, found {replacements}"
        )
    return pattern.sub(replacement, source)


def add_app_header(project_dir: Path) -> None:
    main_source = project_dir / "main.c"
    source = main_source.read_text(encoding="utf-8")
    include_count = source.count(APP_HEADER_INCLUDE)
    if include_count == 0:
        marker_count = source.count(MAIN_INCLUDE_MARKER)
        if marker_count != 1:
            raise SystemExit(
                f"expected one main include marker in {main_source}, found {marker_count}"
            )
        source = source.replace(
            MAIN_INCLUDE_MARKER,
            f"{MAIN_INCLUDE_MARKER}\n{APP_HEADER_INCLUDE}",
        )
        main_source.write_text(source, encoding="utf-8")
    elif include_count != 1:
        raise SystemExit(
            f"expected at most one app header include in {main_source}, found {include_count}"
        )


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


def find_absolute_path(path: Path, content: str) -> str | None:
    for match in QUOTED_STRING_PATTERN.finditer(content):
        value = match.group(1)
        for pattern in (
            CMAKE_GENERATOR_PATH_PATTERN,
            QUOTED_ABSOLUTE_PATH_PATTERN,
            QUOTED_COLON_PATH_PATTERN,
        ):
            path_match = pattern.search(value)
            if path_match is not None:
                return path_match.group(1)

    unquoted_content = QUOTED_STRING_PATTERN.sub('""', content)
    if path.suffix == ".ld":
        match = LINKER_SINGLE_PATH_PATTERN.search(unquoted_content)
        if match is not None:
            return match.group(1)
        for command_match in LINKER_LIST_PATTERN.finditer(unquoted_content):
            operand_match = LINKER_OPERAND_PATH_PATTERN.search(command_match.group(1))
            if operand_match is not None:
                return operand_match.group(1)
    elif path.suffix in {".cmake", ".json", ".properties", ".txt"}:
        if path.suffix == ".cmake":
            match = CMAKE_GENERATOR_PATH_PATTERN.search(unquoted_content)
            if match is not None:
                return match.group(1)
        match = UNQUOTED_ABSOLUTE_PATH_PATTERN.search(unquoted_content)
        if match is not None:
            return match.group(1)

    return None


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
    source = replace_exactly_once(
        STUDIO_METADATA_PATTERN,
        PORTABLE_METADATA_LINE,
        source,
        "Simplicity Studio metadata block",
        inventory,
    )
    for reference in EXPECTED_PROJECT_REFERENCES:
        if source.count(reference) != 1:
            raise SystemExit(
                f"expected one portable project reference {reference} in {inventory}"
            )
    inventory.write_text(source, encoding="utf-8")

    shutil.copyfile(toolchain_template, cmake_dir / "toolchain.cmake")
    add_app_header(project_dir)

    for generated_file in generated_text_files(project_dir):
        normalize_whitespace(generated_file)
        content = generated_file.read_text(encoding="utf-8")
        absolute_path = find_absolute_path(generated_file, content)
        if absolute_path is not None:
            raise SystemExit(
                f"absolute path remains in {generated_file}: {absolute_path}"
            )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("project_dir", type=Path)
    parser.add_argument("toolchain_template", type=Path)
    args = parser.parse_args()
    normalize(args.project_dir, args.toolchain_template)


if __name__ == "__main__":
    main()
