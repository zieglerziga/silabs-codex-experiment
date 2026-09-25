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
may wrap naturally after long operation. Time comparisons use unsigned elapsed
arithmetic and signed deadline comparison so the 32-bit millisecond clock can
wrap safely.

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

This adapts the separation used by `devs-refd-ble-remote` while avoiding its
checked-in SDK copy and generated absolute SDK paths.
