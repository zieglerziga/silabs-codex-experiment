#!/usr/bin/env python3
"""Combine compilation databases while keeping the first command per source."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def merge_compile_commands(input_paths: list[Path], output_path: Path) -> int:
    input_paths = [input_path.resolve(strict=True) for input_path in input_paths]
    output_path = output_path.resolve()
    merged: list[dict[str, Any]] = []
    seen_files: set[str] = set()

    for input_path in input_paths:
        document: Any = json.loads(input_path.read_text(encoding="utf-8"))
        if not isinstance(document, list):
            raise ValueError(f"{input_path} must contain a JSON array")

        for entry in document:
            if not isinstance(entry, dict) or not isinstance(entry.get("file"), str):
                raise ValueError(f"{input_path} contains an invalid entry")
            source_file = entry["file"]
            if source_file in seen_files:
                continue
            seen_files.add(source_file)
            merged.append(entry)

    output_path.write_text(
        json.dumps(merged, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    return len(merged)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, action="append", required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()

    count = merge_compile_commands(arguments.input, arguments.output)
    print(f"Merged {count} unique compilation command(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
