# Project Status

Last updated: 2026-09-25 (Europe/Budapest)

## Goal

Build a Silicon Labs firmware project that continuously scans for BLE
advertisements and reports useful, rate-controlled observations through SEGGER
RTT. The repository must build through CMake/Make/Docker, document its setup,
and protect pull requests with free/open-source GitHub Actions checks.

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
- [ ] Capture the reference-project architecture and decide repository layout.
- [ ] Add the Silicon Labs project, BLE scanner, RTT logging, and host tests.
- [ ] Add CMake, Make, and Docker build workflows.
- [ ] Add pull-request security and quality workflows.
- [ ] Complete junior readability and senior embedded reviews; fix findings.
- [ ] Build/test locally and in Docker.
- [ ] Open a pull request to `development` and verify all checks.

## Session handoff

Current stage: repository bootstrap and discovery. The local CodeGraph database
is initialized and intentionally ignored by Git; it currently has no source
files to index.

Next actions:

1. Inventory the relevant `devs-refd-ble-remote` build files without adopting
   its target hardware.
2. Inspect the installed SDK examples and project-generation metadata for
   BRD4181A / EFR32MG21.

## Verification log

- `commander adapter probe`: detected WSTK6006A with BRD4001A and BRD4181A.
- `commander device info`: detected `EFR32MG21A010F1024IM32` revision A1.
- `git -C "${SISDK_ROOT}" describe --tags --always`: `v2025.6.3`.
- `codegraph status .`: initialized, zero source files at bootstrap, index is
  up to date.

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
