# Project Status

Last updated: 2026-09-25 (Europe/Budapest)

## Goal

Build a Silicon Labs firmware project that continuously scans for BLE
advertisements and reports useful, rate-controlled observations through SEGGER
RTT. The repository must build through CMake/Make/Docker, document its setup,
and protect pull requests with free/open-source GitHub Actions checks.

All repeatable operations are captured as scripts and surfaced through the
root Makefile. Commits use scoped Conventional Commit messages, and each agent
stage leaves a human-readable status entry.

## Confirmed environment

| Item | Value |
| --- | --- |
| GitHub repository | `zieglerziga/silabs-codex-experiment` |
| Integration branch | `development` (created from `main`) |
| Feature branch | `feature/ble-rtt-scanner` |
| Mainboard | BRD4001A Rev. A01 |
| Radio board | BRD4181A Rev. A01 |
| Device | EFR32MG21A010F1024IM32 (1 MiB flash, 96 KiB SRAM) |
| Local SDK | `SISDK_ROOT` environment variable, tag `v2025.6.3` |
| Commander | Simplicity Commander 1v25p0b1995 |
| Debug adapter | SEGGER J-Link Pro OB with host-enumerated VCOM |

The local SDK checkout had a pre-existing untracked Python `__pycache__`; the
project must not modify or clean that external checkout.

For a local session, point the build at the installed SDK before running SDK
commands:

```sh
export SISDK_ROOT=/path/to/simplicity_sdk
```

## Intended scan behavior

- Start passive scanning after the Bluetooth stack reaches system boot.
- Track a fixed-size set of recently observed devices without dynamic memory.
- Log first sightings and meaningful changes (for example name or RSSI bucket),
  with per-device rate limiting.
- Emit a compact periodic summary including total reports, unique devices,
  dropped cache entries, and suppressed log lines.
- Treat malformed advertisement payloads defensively and never read beyond the
  event-provided buffer.

## Stage checklist

- [x] Detect attached hardware and installed development tools.
- [x] Create and push `development`; create the feature branch from it.
- [x] Initialize the local CodeGraph index.
- [x] Capture the reference-project architecture and decide repository layout.
- [x] Add the Silicon Labs project, BLE scanner, RTT logging, and host tests.
- [ ] Add CMake, Make, and Docker build workflows.
- [x] Add pull-request security and quality workflows.
- [ ] Complete junior readability and senior embedded reviews; fix findings.
- [ ] Build/test locally and in Docker.
- [ ] Open a pull request to `development` and verify all checks.

## Session handoff

Current stage: the first Silicon Labs firmware integration review is complete.
All junior and senior findings have been fixed and verified locally and on the
attached hardware; the fixes are ready for their required two-agent re-review.

Next actions:

1. Commit the firmware review fixes and repeat both reviews until clean.
2. Add and validate the container build, then review that stage.
3. Review the existing pull-request workflow before opening the PR.

## Verification log

- `commander adapter probe`: detected WSTK6006A with BRD4001A and BRD4181A.
- `commander device info`: detected `EFR32MG21A010F1024IM32` revision A1.
- `git -C "${SISDK_ROOT}" describe --tags --always`: `v2025.6.3`.
- `codegraph status .`: initialized, zero source files at bootstrap, index is
  up to date.
- `make generate-firmware`: SLC CLI 5.11.0 generated the SDK 2025.6.3 project;
  normalization left no host-specific absolute paths.
- `make prepare-sdk`: materialized only the three Git LFS archives referenced
  by the generated linker inputs.
- `make firmware`: built the EFR32MG21 image with Arm GNU 16.2.0; final size was
  104400 bytes text, 2828 bytes data, and 95476 bytes BSS.
- `make flash`: erased, programmed, and verified 112 KiB on the attached
  BRD4181A, then reset the target.
- `make rtt RTT_SECONDS=5`: captured scanner startup and ten distinct nearby
  advertisers through Commander RTT; longer validation also produced the
  30-second summary (`555` reports, `13` discoveries, `489` suppressed lines).

## Review log

### Bootstrap documentation (`dfe017a`)

- Junior readability review: requested removal of host-specific SDK/VCOM paths
  and correction of a stale handoff action.
- Senior embedded review: confirmed the hardware and SDK facts; independently
  reported the same portability and stale-handoff issues.
- Resolution: use `SISDK_ROOT`, describe VCOM enumeration generically, and make
  the handoff begin with the actual next task.
- Senior re-review: requested an explicit portable setup example for
  `SISDK_ROOT`; the example above resolves it without committing a host path.

### Reference architecture research

- Luna research identified per-project `.slcp` manifests, generated CMake, a
  shared Make wrapper, CMake presets, and root Docker orchestration as the useful
  patterns in `devs-refd-ble-remote`.
- The reference's copied SDK and generated absolute SDK path were explicitly
  rejected. This project will keep SDK `v2025.6.3` external and use
  `SISDK_ROOT`.

### Portable scan tracker

- Added a fixed 32-entry cache with oldest-seen eviction, safe AD-structure
  parsing, name/RSSI change detection, per-device log rate limiting, periodic
  refresh, and cumulative summaries.
- Added SDK-independent CMake/CTest coverage for duplicate suppression, delayed
  name/RSSI changes, malformed payloads, cache eviction, and timer wraparound.
- Added repeatable `make check`, `make test`, `make sanitize`, and formatting
  targets implemented by scripts under `scripts/`.
- `make check`: passed with GCC 16.2.1 and warnings treated as errors.
- `make test`: 1/1 CTest passed.
- `make sanitize`: 1/1 CTest passed with AddressSanitizer and
  UndefinedBehaviorSanitizer.
- `make format-check`: passed with the host clang-format.
- Junior review found that `scan_tracker_take_summary()` sounded destructive
  even though counters are cumulative. It was renamed to
  `scan_tracker_snapshot_summary()` and covered with a regression test.
- Senior review found no defect, but noted an implementation-defined unsigned
  to signed conversion in deadline comparison. It was proactively replaced by
  a fully unsigned half-range comparison.
- Re-review found the half-range comparison excluded the final valid value.
  The boundary is now inclusive and has a dedicated regression test.
- Review-fix verification: `make test` passed 1/1, `make sanitize` passed 1/1,
  and `git diff --check` passed.

### Silicon Labs firmware integration (initial review complete; re-review pending)

- Added an SDK 2025.6.3 `.slcp` project for BRD4181A/EFR32MG21, generated CMake
  metadata, and a portable toolchain file.
- Added a thin Bluetooth event adapter that starts passive legacy scanning,
  feeds the reviewed tracker, and formats observations and summaries to RTT.
- Added repeatable scripts for SLC generation and path normalization, selected
  SDK Git LFS preparation, firmware build, hardware flash, and bounded RTT
  capture. The root Makefile exposes each operation.
- The firmware keeps EM1 as the lowest energy mode so RTT RAM remains visible;
  battery and deep-sleep optimization are explicitly outside this PoC.
- Live hardware validation confirmed scan events, duplicate suppression,
  periodic refreshes, and summary output. The SDK tiny `printf` component was
  added after validation showed that the C library fallback buffered output.

Initial review of `70218dd` found:

- Junior: regeneration removed tracked output before SLC succeeded; host-path
  validation and duplicate assignment checks were too narrow; `make clean`
  omitted the nested firmware build.
- Senior: generated `main.c` called application hooks without their prototype;
  rotating addresses could bypass per-device limiting by continuously evicting
  the 32-entry cache.

Resolutions:

- SLC now generates from staged source copies. Normalization, portable-path
  validation, and prototype injection complete before a transactional installer
  replaces the four generated outputs. A forced SLC failure returned non-zero
  while SHA-256 hashes confirmed every existing generated file was unchanged.
- Normalization rejects duplicate SDK/package assignments, checks a broader set
  of POSIX and Windows host paths, and validates expected project-relative
  source/include references. Path-dependent opaque Studio metadata is removed;
  two consecutive generations produced identical tracked output.
- `make clean` removes both host and nested firmware build directories.
- Generated `main.c` reproducibly includes `app.h`.
- The tracker now applies a 250 ms aggregate observation-log interval in
  addition to the per-device interval. A 100-address churn regression emits
  exactly four lines in one second while still counting all discoveries and
  evictions.
- Fix verification: `make verify` passed; the regenerated firmware built at
  104432 bytes text, 2828 bytes data, and 95476 bytes BSS; hardware flash and a
  five-second RTT capture passed, with live first-sighting lines spaced by at
  least the aggregate limit.
