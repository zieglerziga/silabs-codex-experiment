#!/usr/bin/env python3
"""Validate that a bounded RTT capture reached BLE scanning."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


def assess_rtt_capture(output: str) -> tuple[bool, bool]:
    lines = output.splitlines()
    scan_started = any("BLE scan started:" in line for line in lines)
    advertisement_seen = any(line.startswith("scan ") for line in lines)
    return scan_started, advertisement_seen


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("duration_seconds", type=int)
    args = parser.parse_args()

    output = args.capture.read_text(encoding="utf-8", errors="replace")
    scan_started, advertisement_seen = assess_rtt_capture(output)
    if not scan_started:
        print(output, file=sys.stderr, end="" if output.endswith("\n") else "\n")
        print(
            f"BLE scanning did not start within {args.duration_seconds}s",
            file=sys.stderr,
        )
        return 1

    if not advertisement_seen:
        print(
            "BLE scanning started, but no advertisements were observed during "
            f"the {args.duration_seconds}s capture.",
            file=sys.stderr,
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
