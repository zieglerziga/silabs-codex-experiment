#!/usr/bin/env python3
"""Rewrite container-only paths in a compilation database for host analysis."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


COMMAND_FIELDS = ("directory", "command", "file")


def _replace_value(value: str, replacements: list[tuple[str, str]]) -> str:
    for source, destination in replacements:
        value = value.replace(source, destination)
    return value


def _resolve_under_root(path: Path, root: Path, *, strict: bool) -> Path:
    resolved = path.resolve(strict=strict)
    try:
        resolved.relative_to(root)
    except ValueError as error:
        raise ValueError(f"{path} is outside trusted root {root}") from error
    return resolved


def _normalize_arguments(
    arguments: Any,
    replacements: list[tuple[str, str]],
) -> tuple[list[str], int]:
    if not isinstance(arguments, list) or not all(
        isinstance(argument, str) for argument in arguments
    ):
        raise ValueError("compilation database arguments must be strings")

    normalized = [_replace_value(argument, replacements) for argument in arguments]
    applied = sum(
        old != new for old, new in zip(arguments, normalized, strict=True)
    )
    return normalized, applied


def _normalize_entry(
    entry: dict[str, Any],
    replacements: list[tuple[str, str]],
) -> int:
    replacements_applied = 0
    for field in COMMAND_FIELDS:
        value = entry.get(field)
        if isinstance(value, str):
            normalized = _replace_value(value, replacements)
            if normalized != value:
                replacements_applied += 1
                entry[field] = normalized

    arguments = entry.get("arguments")
    if arguments is not None:
        normalized_arguments, applied = _normalize_arguments(arguments, replacements)
        replacements_applied += applied
        entry["arguments"] = normalized_arguments

    return replacements_applied


def normalize_compile_commands(
    input_path: Path,
    output_path: Path,
    replacements: list[tuple[str, str]],
    root_path: Path,
) -> int:
    root = root_path.resolve(strict=True)
    if not root.is_dir():
        raise ValueError(f"trusted root {root_path} is not a directory")
    input_path = _resolve_under_root(input_path, root, strict=True)
    output_path = _resolve_under_root(output_path, root, strict=False)
    document: Any = json.loads(input_path.read_text(encoding="utf-8"))
    if not isinstance(document, list):
        raise ValueError("compilation database must contain a JSON array")

    replacements_applied = 0
    for entry in document:
        if not isinstance(entry, dict):
            raise ValueError("each compilation database entry must be an object")
        replacements_applied += _normalize_entry(entry, replacements)

    output_path.write_text(
        json.dumps(document, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    return replacements_applied


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root",
        type=Path,
        required=True,
        help="Trusted root; input and output paths must resolve beneath it.",
    )
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--replace",
        nargs=2,
        action="append",
        metavar=("SOURCE", "DESTINATION"),
        required=True,
    )
    arguments = parser.parse_args()

    applied = normalize_compile_commands(
        arguments.input,
        arguments.output,
        [(source, destination) for source, destination in arguments.replace],
        arguments.root,
    )
    print(f"Normalized {applied} compilation-database field value(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
