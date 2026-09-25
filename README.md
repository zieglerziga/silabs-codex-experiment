# EFR32MG21 BLE RTT Scanner

This repository contains an observability-first Silicon Labs proof of concept
for the BRD4181A radio board on a BRD4001A mainboard. The firmware passively
scans BLE legacy advertisements, tracks devices in bounded memory, suppresses
duplicate reports, and writes useful observations and summaries through SEGGER
RTT.

The project targets `EFR32MG21A010F1024IM32` with Simplicity SDK `v2025.6.3`.
The SDK and Silicon Labs tools stay outside this repository.

## What gets logged

- the first report from a device;
- a name or RSSI change after a per-device cooldown;
- an unchanged-device refresh after ten seconds; and
- cumulative scan, discovery, eviction, suppression, and malformed-payload
  counters every 30 seconds.

Observation output also has a global four-lines-per-second ceiling, so cache
churn from rotating private addresses cannot turn RTT into a packet trace.

RTT uses non-blocking mode so a disconnected or slow host cannot stall the
Bluetooth event handler. This PoC holds an EM1 power-manager requirement while
running because reliable RTT access is more important than battery behavior.

## Prerequisites

- Simplicity SDK `v2025.6.3` in a local Git checkout;
- Silicon Labs SLC CLI 5.11.0 for regeneration;
- Simplicity Commander for flashing and RTT capture;
- Docker Engine and CLI for the optional container targets;
- CMake, Ninja, Make, Python 3, and an Arm GNU toolchain; and
- ClangFormat for formatting checks;
- Git LFS when the SDK checkout contains pointer-form prebuilt archives; and
- GNU `timeout` (Coreutils) for bounded `make rtt` capture sessions.

Point the build at the external SDK and, when generating, the SLC executable:

```sh
export SISDK_ROOT=/path/to/simplicity_sdk
export SLC_CLI=/path/to/slc
```

If the SDK is a Git LFS checkout, materialize only the archives selected by the
generated project:

```sh
make prepare-sdk
```

## Build and validate

```sh
make test
make sanitize
make format-check
make verify
make firmware
```

The open-source build image pins its Ubuntu base by digest and its apt archive
to a dated Ubuntu snapshot. It contains the host validation tools plus the
checksum-verified Arm GNU 12.2.Rel1 toolchain supported by the selected SDK.
Build and use it with:

```sh
make docker-image
make docker-verify
make docker-firmware
```

`docker-firmware` mounts `SISDK_ROOT` read-only at `/opt/simplicity_sdk` and
runs without network access, Linux capabilities, or a writable container root.
SLC, Commander, and the SDK are deliberately not installed in the image;
generation, flashing, and RTT capture remain explicit host operations. Host
checks use `build/docker/` so native and container CMake caches never collide.

Regenerate the Silicon Labs project after changing `ble_scanner.slcp`:

```sh
make generate-firmware
```

Program the attached supported board and capture a bounded RTT session:

```sh
make flash
make rtt RTT_SECONDS=10
```

The RTT transcript is also saved to `build/rtt.log`. Run `make help` for the
complete command list.

## Pull-request verification

Pull requests targeting `development` run the same `make verify` entry point,
actionlint for workflow correctness, zizmor for workflow security, and
TruffleHog across the changed commits. Every third-party action is pinned to a
full commit SHA and jobs receive read-only permissions unless a narrower
exception is documented in the workflow.

Run the workflow linters locally in digest-pinned, network-isolated containers:

```sh
make workflow-check
```

## Repository guide

- `firmware/ble_scanner/` contains the `.slcp`, generated Silicon Labs project,
  application adapter, and portable tracker.
- `tests/` contains SDK-independent host tests.
- `scripts/` contains repeatable generation, build, SDK preparation, flashing,
  RTT, container, formatting, workflow-audit, and verification flows.
- `docs/ARCHITECTURE.md` explains design decisions.
- `docs/STATUS.md` is the resumable human-readable project log.
- `.github/workflows/pr.yml` defines pull-request quality and security gates.

Generated CMake files are normalized to use `SISDK_ROOT`; host-specific paths,
the SDK itself, build output, and local CodeGraph state must not be committed.
