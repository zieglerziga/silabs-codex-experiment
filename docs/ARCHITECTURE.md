# Architecture

## Design constraints

The firmware runs without an RTOS on the attached EFR32MG21 target. BLE stack
events must be handled quickly, RTT output must never control radio timing, and
the scanner must not allocate memory dynamically.

## Data flow

1. The Silicon Labs Bluetooth stack reports a legacy advertisement.
2. The board-specific adapter converts it to a `scan_observation_t`.
3. `scan_tracker_observe()` safely parses the local-name AD structure and
   updates a fixed 32-entry device cache.
4. The tracker requests a log only for a first sighting, a rate-limited name or
   RSSI change, or a ten-second refresh.
5. The adapter formats that result to the non-blocking RTT stream.
6. The main loop emits cumulative statistics every 30 seconds.

The tracking module has no Silicon Labs dependencies. It is built and tested on
the host in pull-request CI, while the thin adapter is compiled with the
firmware against Simplicity SDK.

## Bounded resources

| Resource | Bound |
| --- | --- |
| Device cache | 32 entries, oldest-seen entry evicted when full |
| Local name | 24 printable characters plus terminator |
| Per-device change log rate | At most once per second |
| Unchanged-device refresh | Once per ten seconds |
| Summary | Once per 30 seconds |

Counters saturate where they are per-device; global counters are diagnostic and
may wrap naturally after long operation. Summary snapshots remain cumulative.
Time comparisons use unsigned arithmetic with intervals below half the 32-bit
range so the millisecond clock can wrap safely without implementation-defined
integer conversions.

## Silicon Labs integration

`ble_scanner.slcp` selects the attached EFR32MG21 target, legacy scanner,
Bluetooth system, bare-metal main loop, sleeptimer, RTT iostream, and the SDK's
tiny `printf` implementation. SLC-generated source, configuration, and CMake
metadata are committed so ordinary firmware builds do not require SLC CLI.
Generated CMake reads the external SDK from `SISDK_ROOT`; a normalization script
rejects host-specific absolute paths.

The scanner uses passive 1M PHY scanning with a 100 ms interval and 50 ms
window. RTT channel 0 is configured in non-blocking mode. The application keeps
an EM1 power-manager requirement for the lifetime of the firmware because the
debug probe cannot read the RTT RAM control block in EM2 on this board. Power
optimization is intentionally outside this observability-first PoC.

The SDK's tiny `printf` component is required even though the public API is
`sl_iostream_printf()`: it streams formatted characters immediately. Falling
back to the C library left output buffered and invisible to a live RTT reader.

## Advertisement parsing

Advertising data is a sequence of length-prefixed fields. The parser checks the
remaining buffer before reading the type or data, stops on a zero-length field,
prefers the complete local name over a shortened name, replaces non-printable
name bytes with `.`, and records malformed packets without dropping the device
observation.

## Repository layout

- `firmware/ble_scanner/`: Silicon Labs project metadata, adapter, and portable
  tracking code.
- `tests/`: SDK-independent tests for parsing, rate limiting, eviction, and
  timer wraparound.
- `docs/`: architecture, progress, and operational documentation.
- `.github/workflows/`: pull-request quality and security gates.
- `scripts/`: repeatable host checks, generation, build, flash, and RTT capture.

This adapts the separation used by `devs-refd-ble-remote` while avoiding its
checked-in SDK copy and generated absolute SDK paths.
