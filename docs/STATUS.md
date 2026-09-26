# Project Status

Last updated: 2026-09-26 (Europe/Budapest)

## Goal

Build a Silicon Labs BLE control-lab firmware for the attached EFR32MG21 board.
The first stage continuously scans for BLE advertisements and exposes bounded,
low-level scanner, TX-power, identity, and logging controls through SEGGER RTT.
The repository must build through CMake/Make/Docker, document its setup, and
protect pull requests with free/open-source GitHub Actions checks.

All repeatable operations are captured as scripts and surfaced through the
root Makefile. Commits use scoped Conventional Commit messages, and each agent
stage leaves a human-readable status entry.

## Confirmed environment

| Item | Value |
| --- | --- |
| GitHub repository | `zieglerziga/silabs-codex-experiment` |
| Integration branch | `development` (created from `main`) |
| Feature branch | `feature/ble-rtt-control` |
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
- [x] Add the first RTT control plane for scanner, TX-power, identity, and log settings.
- [x] Add CMake, Make, and Docker build workflows.
- [x] Add pull-request security and quality workflows.
- [x] Complete junior readability and senior embedded reviews; fix findings.
- [x] Build/test locally and in Docker.
- [x] Open a pull request to `development` and verify all checks.
- [x] Flash the current control image and exercise the RTT command surface on hardware.

## Session handoff

Current stage: the RTT control-plane implementation, independent reviews, and
hardware RTT validation are complete. PR #6 is open against `development`; all
reported GitHub Actions checks pass. Required GitHub review remains.

Next actions:

1. Obtain the required GitHub review and merge PR #6 when approved.
2. Expand the `.slcp` project with advertiser, connection, GATT, security, and
   periodic radio components.

See [DEVELOPMENT_PLAYBOOK.md](DEVELOPMENT_PLAYBOOK.md) for the development
sequence and reusable prompt; see [PROMPT_TEMPLATE.md](PROMPT_TEMPLATE.md) for
the copy-ready template.

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
- `SISDK_ROOT=$SISDK_ROOT SLC_CLI=$SLC_CLI make generate-firmware` regenerated
  the extended-scanner, Filter Accept List, and Resolving List component
  output.
- `SISDK_ROOT=$SISDK_ROOT make prepare-sdk` materialized the one newly required
  SDK LFS archive for the Filter Accept List library.
- `make test`, `make sanitize`, `make format-check`, and `make verify` passed
  after the fix; the final regenerated firmware build completed all 145 steps.
- PR #6 verification run `36228082567` passed Host checks, Firmware build,
  Workflow security, and Secret scan. Bootstrap run `36228082677` passed its
  PR automation check. GitHub reports the PR is blocked only on required review.
- Latest hardware validation on `feature/ble-rtt-control`:
  - `make flash` programmed and verified `139264` bytes on the detected
    BRD4181A/EFR32MG21 target, then reset it successfully.
  - RTT `status` reported `board=BRD4181A`, target
    `EFR32MG21A010F1024IM32`, SDK `2025.6.3`, `stack=ready`, and non-blocking
    RTT with EM1 required.
  - `scan stop`, `scan set active 160 80 1m observation 0 0`, and `scan start`
    succeeded. A live reconfiguration to
    `scan set passive 200 100 1m observation 0 0` also succeeded and status
    reported `scan=on mode=passive interval=200 window=100`.
  - `tx set -30 80` returned status `0x00000000`; the device reported the
    selected range as `-29..80` in `0.1dBm` units. `identity get` returned
    status `0x0000000F`.
  - `log set observations off` and `log set summaries off` were reflected by
    `logs=observations:off,summaries:off`; both streams were restored to `on`.
  - `make rtt RTT_SECONDS=5` passed validation and captured first reports from
    two nearby advertisers without recording their device identifiers.
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

### Silicon Labs firmware integration (review complete)

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
- Senior re-review: no findings; both application-prototype and cache-churn
  issues are resolved without timer-wrap regressions.
- Junior re-review: requested detection of arbitrary absolute generated paths
  instead of common-root enumeration, plus documentation of conditional Git
  LFS and GNU `timeout` prerequisites. The path check now validates quoted and
  unquoted POSIX/Windows paths and has Python regression coverage; the README
  lists both tools.
- Follow-up review identified two remaining path forms: Windows UNC paths and
  unquoted paths in linker scripts. Detection now covers every generated text
  type, distinguishes C++ comments from network paths, and has drive-letter,
  UNC, and linker-script regression cases.
- Senior re-review then exposed a false positive for C division and escapes in
  CMake generator expressions and comma-separated linker flags. Unquoted
  validation is now syntax-scoped, while quoted colon/comma contexts are
  covered without treating HTTPS URLs as filesystem paths.
- Final path-review follow-up adds unquoted comma/colon CMake contexts and GNU
  ld `STARTUP`/`OUTPUT` directives, with a regression case for every reported
  escape.
- Senior review identified that a relative first operand could hide a later
  absolute path in GNU ld `INPUT`/`GROUP` lists. The normalizer now scans every
  operand in those commands, with space- and comma-separated regressions.
- Final senior follow-up adds forward-slash Windows UNC paths to unquoted CMake
  and linker contexts, matching the already-supported quoted UNC spelling.
- Because unquoted URLs share the `//` spelling, CMake colon handling is now
  limited to known generator-expression path contexts; quoted and unquoted
  HTTPS URLs both have explicit allow regressions.
- Generator-expression validation scans the full semicolon-separated interface
  list, preventing a forward-slash UNC entry from hiding beside relative paths.
- Final junior and senior re-reviews reported no findings. `make verify` passes
  24 Python path-normalization regressions plus the host C and sanitizer tests;
  deterministic SLC regeneration also passes.

### Reproducible container build (implementation complete; review pending)

- Added a digest-pinned Ubuntu 24.04 image containing only open-source host
  validation and Arm GNU build dependencies. Apt resolves from the fixed
  `20260925T090000Z` Ubuntu snapshot so package versions cannot drift.
- Added one repeatable container script and Make targets for image creation,
  host verification, and a network-isolated firmware build with the external
  SDK mounted read-only.
- Container host checks use a dedicated `build/docker` tree, preventing CMake
  cache paths from colliding with native host checks.
- SLC generation, Commander flashing, and RTT capture intentionally remain
  host operations; proprietary tooling and hardware access are not baked into
  the image.
- `make docker-image`, `make docker-verify`, and
  `SISDK_ROOT=/path/to/simplicity_sdk make docker-firmware` passed.
  The container firmware compiled and linked all 145 build steps with Arm GNU
  13.2.1 while networking, capabilities, and root-filesystem writes were
  disabled.
- Junior review found the host SDK path in this log, the omitted Docker
  prerequisite, and mutable apt repositories. The path is now portable, Docker
  is listed as an optional prerequisite, and both the certificate-bootstrap
  image and runtime base are pinned by digest before apt uses the fixed Ubuntu
  snapshot.
- After the fixes, the image rebuilt from the snapshot and both container host
  verification and the complete 145-step firmware build passed again.
- Senior review found that Ubuntu's Arm GCC 13.2 package was newer than the
  SDK-supported 12.2.Rel1 release. The image now installs Arm's architecture-
  specific 12.2.Rel1 bundle after verifying its published SHA-256 checksum;
  the runtime also asserts compiler version 12.2.1 during the image build.
- Junior and senior re-reviews reported no findings. `make docker-verify` and
  the complete 145-step container firmware build passed with GCC 12.2.1.

### Pull-request CI (complete; reviews passed)

- Bootstrap commit `ece465c` is pushed to both `main` and `development`, so
  GitHub registers pull-request automation before this feature branch opens a
  PR. Manual run `36119982282` passed on `main`.
- The workflow runs for pull requests targeting `development`, pushes to
  `development`, and manual dispatches, with concurrency cancellation and
  least-privilege default permissions.
- Host quality uses the same `make verify` entry point as local development:
  formatting, Python regressions, strict C compilation, CTest, sanitizers, and
  whitespace validation.
- Open-source checks add actionlint 1.7.12, zizmor 1.30.1, and TruffleHog
  3.97.9. Actions and tool sources are pinned to full commit SHAs or versions;
  the secret scan compares the exact pull-request base and head commits.
- `make workflow-check` reproduces the actionlint and zizmor audits with
  digest-pinned, read-only containers that have no network access or Linux
  capabilities.
- Junior review reported no findings. Senior review found that a bare
  `git diff --check` only inspects working-tree changes and is empty in a clean
  Actions checkout. The workflow now fetches history and passes the exact PR
  base/head SHAs to `make verify`, which checks their merge-base diff; local
  calls retain the working-tree check.
- Junior and senior re-reviews reported no findings. `make workflow-check`,
  local and range-aware `make verify` runs, and an explicit whitespace check
  against `origin/development...HEAD` passed. Supplying only one verification
  SHA fails fast with status 2.
- The feature branch is backed up as `origin/feature/ble-rtt-scanner`; it also
  contains the current `development` bootstrap baseline so the PR event can
  execute both the bootstrap and full verification workflows.

### Pull request verification (merged)

- GitHub pull request [#1](https://github.com/zieglerziga/silabs-codex-experiment/pull/1)
  targets `development` from `feature/ble-rtt-scanner`.
- Feature head `03c5b73` passed all four checks: bootstrap run `36146207230`
  and full verification run `36146207345` (`Host checks`, `Workflow security`,
  and `Secret scan`).
- The final status-only head `97d7bf8` passed bootstrap run `36177956545` and
  full run `36177956577`, including the firmware build. PR #1 merged on
  2026-09-25 as `f69f79142c8fde207a9c3d7130eeb7d9c13a677c`.

### Firmware CI (complete)

- Added a dedicated `Firmware build` job for pull requests, development pushes,
  and manual workflow runs on `ubuntu-24.04` with read-only permissions and a
  60-minute timeout for SDK and Git LFS network variance.
- `make prepare-ci-sdk` creates a new external SDK checkout at exact public
  commit `b41bec3ff2485199c1a5a9995b3e649e118c1b8d` (tag `v2025.6.3`) while
  skipping broad LFS smudging, then materializes only the 16 archives selected
  by the generated firmware project.
- The job builds the digest- and snapshot-pinned container, then performs the
  complete EFR32MG21 build with Arm GNU 12.2.Rel1, no container network, and a
  read-only SDK mount. No SDK, toolchain, or build artifact is committed.
- The platform review recommended no cache initially to avoid cache-poisoning
  complexity; the public SDK checkout and selected LFS objects fit the hosted
  runner disk budget. The job uses no secrets, PAT, or `pull_request_target`.
- Local end-to-end validation created a fresh SDK checkout in a temporary
  external directory, resolved the pinned commit, materialized all 16 selected
  archives, and completed the 145-step network-isolated firmware build with
  GCC 12.2.1. `make verify` and `make workflow-check` also passed.
- Junior readability/documentation review of commit `4fa58d3` reported no
  findings after checking Make target discoverability, shell clarity, failure
  behavior, reproduction guidance, and status-log continuity. The reviewer ran
  `make help`, `sh -n scripts/prepare_ci_sdk.sh`, `make verify`,
  `make workflow-check`, a relative-path rejection check, and
  `git diff --check 4fa58d3^ 4fa58d3`.
- Senior embedded/CI review of commit `4fa58d3` reported no findings. It
  independently confirmed the exact SDK tag and commit, 16-archive selective
  LFS fetch, BRD4181A/EFR32MG21 generated target, fork-safe workflow
  permissions, read-only SDK mount, network-disabled build runtime, and hosted
  build log through `[145/145] Linking C executable
  default_config/ble_scanner.out`.
- Pull-request verification run `36151070655` passed at commit `4fa58d3`:
  `Firmware build` (3m09s), `Host checks`, `Workflow security`, and
  `Secret scan`. Bootstrap run `36151070718` also passed. This status-only
  follow-up intentionally triggers the same checks again; the live PR check
  state remains authoritative before merge.

### GitHub Action stable-version refresh (complete)

- Audited all seven `uses:` entries and four unique third-party actions against
  each upstream repository's official latest non-draft, non-prerelease GitHub
  release and resolved tag commit.
- Upgraded all four `actions/checkout` references from v4.2.2 to v7.0.1 at
  immutable commit `3d3c42e5aac5ba805825da76410c181273ba90b1`.
- Upgraded `actions/setup-go` from v6.0.0 to v7.0.0 at immutable commit
  `b7ad1dad31e06c5925ef5d2fc7ad053ef454303e`.
- Confirmed `zizmorcore/zizmor-action` v0.6.4 and
  `trufflesecurity/trufflehog` v3.97.9 were already the latest stable releases
  at their existing immutable commits. Their selected zizmor v1.30.1 and
  TruffleHog v3.97.9 tool versions are also current.
- Added `make actions-audit` to repeat the official release/tag/SHA comparison
  without making normal pull-request checks depend on mutable upstream release
  timing.
- The audit rejects mutable or uncommented external action references, resolves
  lightweight and annotated release tags to commits, and has host tests for
  discovery, rejection, tag resolution, and outdated-result handling.
- Platform review confirmed both v7 migrations use Node 24 and require no input
  changes for this `pull_request` workflow on the hosted Ubuntu 24.04 runner.
  `make actions-audit`, `make workflow-check`, and `make verify` (29 Python
  tests plus the existing strict C, test, and sanitizer gates) passed locally.
- Senior review found two low-severity audit completeness gaps: Docker action
  references were skipped without enforcing a digest, and GitHub's `$/path`
  self-repository syntax was not recognized. The follow-up requires Docker
  actions to use a SHA-256 digest, accepts both supported local-action forms,
  and adds regression coverage for each case.
- Fix commit `ddaf735` resolved both findings. Junior and senior re-reviews
  reported no findings after checking the documentation, all seven current
  `uses:` entries, local and Docker reference handling, external action
  subpaths, annotated tags, latest stable release data, and immutable commits.
- Pull-request verification run `36153115003` passed at commit `ddaf735`:
  `Firmware build` (2m17s), `Host checks`, `Workflow security`, and
  `Secret scan`. Bootstrap run `36153115008` also passed. The preceding action
  migration commit `e0d4e8c` passed the same gates in runs `36152609685` and
  `36152609682` before the audit-only review fixes.
- This status-only follow-up intentionally triggers the same checks again. The
  live PR check state remains authoritative before merge.

### Strict test-architect P2/P3 remediation (complete)

- RTT capture validation now requires the stack-boot handler's `BLE scan
  started` message; the earlier initialization-only message can no longer pass
  the hardware smoke check. A quiet RF interval is reported separately from a
  stack-start failure, with three host regression tests for those states.
- The cache-capacity test now re-observes both the newest and oldest original
  devices after overflow. It proves the newest entry retained its report count
  and the actual oldest entry was evicted, rather than only checking counters.
- A reproducibly installed managed CMake fragment applies `-Werror` to
  hand-written `app.c` and `scan_tracker.c` firmware compilation only. SDK and
  generated sources retain the Silicon Labs warning policy, and firmware builds
  fail if the managed fragment drifts from its source template.
- Local verification passed `make verify` with 32 Python tests, strict C host
  builds, tests, and sanitizers; `make workflow-check`; `make actions-audit`;
  and the complete 145-step containerized firmware build with GCC 12.2.1.
  Generated Ninja rules contained `-Werror` on exactly the two hand-written
  firmware translation units and not on an inspected SDK translation unit.
- The freshly built image was flashed to the attached BRD4181A/BRD4001A. A
  10-second RTT capture passed only after observing `BLE scan started` and then
  recorded multiple real advertisement lines; an earlier capture containing
  probe output but no stack-start evidence correctly failed.
- The strict test-architect re-review reported no unresolved findings on the
  requested P2/P3 items. It confirmed all three RTT states, the eviction-victim
  oracle, the managed-fragment drift guard, and the actual Ninja flag scope.
  Direct testing of the validator CLI's stderr and exit code remains an optional
  small coverage improvement; the tested assessment logic and observed CLI
  behavior are correct.
- Junior readability/documentation and senior embedded/CI reviews reported no
  findings. The senior review independently confirmed the template/generated
  fragments match and `-Werror` applies to `app.c` and `scan_tracker.c`, but not
  an inspected generated translation unit.
- Pull-request verification run `36160957933` passed at commit `c2fe4aa`:
  `Firmware build` (2m47s), `Host checks`, `Workflow security`, and
  `Secret scan`. Bootstrap run `36160957852` also passed. This status-only
  follow-up intentionally triggers the same checks again; the live PR state is
  authoritative before merge.

### Development documentation and reusable prompt

- Added [DEVELOPMENT_PLAYBOOK.md](DEVELOPMENT_PLAYBOOK.md) to explain the
  project discovery, staged implementation, review loops, bootstrap/full PR
  workflow design, verification, commit practices, and final merge.
- Added [PROMPT_TEMPLATE.md](PROMPT_TEMPLATE.md) with fill-in fields and the
  reusable agent instructions for future embedded projects.
- Updated this status log's checklist and handoff to reflect that PR #1 is
  merged and the initial project objective is complete.
- Junior readability re-review reported no findings. Senior review found one
  P3 documentation mismatch: the table mixed the separate bootstrap workflow
  with full-workflow jobs and implied that secret scanning ran on pushes and
  manual dispatch. The playbook now separates the workflows and labels
  TruffleHog as pull-request-only; senior and junior re-reviews reported no
  remaining findings.
- `git diff --check` passed. Reviewers verified the scope against
  `.github/workflows/pr.yml` and `pr-bootstrap.yml`; junior also confirmed
  PR #1's merged state and recorded head/merge/check metadata with GitHub.
- The documentation stage was committed as `9bb67d8` on
  `docs/development-playbook` and opened as PR
  [#2](https://github.com/zieglerziga/silabs-codex-experiment/pull/2) against
  `development`. At that head, PR verification run `36180685356` passed Host
  checks, Firmware build, Workflow security, and Secret scan; bootstrap run
  `36180685539` passed. This status-only follow-up will trigger the same gates
  again; the live PR check state is authoritative before merge.
- In response to the PR review, removed the project-specific RTT, eviction,
  and `-Werror` findings from the reusable playbook. The playbook now states
  the general test-architect review rule and points to this status log for
  historical findings and resolutions. Commit `57484ae` contains the focused
  change; both junior readability and senior embedded/process re-reviews found
  no issues, and `git diff --check` passed. The final PR check run is recorded
  after this status-only update.

### RTT control plane (fix and final review complete)

- Started isolated branch `feature/ble-rtt-control` directly from `development`,
  leaving the existing dirty checkout untouched.
- Added a bounded, SDK-independent RTT command parser with `help`, `status`,
  scanner start/stop/configuration, TX-power get/set, identity inspection, and
  observation/summary logging controls. Parser input is fixed-size, validates
  numeric ranges and named BLE modes, and allocates no memory.
- The firmware applies scanner mode, interval, window, PHY, discovery mode,
  scanner flags, and filter policy through the Silicon Labs API. The generated
  project selects the extended scanner, Filter Accept List, and Resolving List
  components required by those controls. TX-power changes are rejected while
  scanning because the SDK forbids that operation in the active scanner state.
  Existing aggregate/per-device log bounds remain in force.
- Scan reconfiguration restores the last known working configuration when a
  new configuration or restart fails, and reports restoration failures instead
  of claiming that scanning resumed.
- Updated project metadata, generation staging, README usage, and architecture
  documentation. Advertiser, connection, GATT, security, and periodic-radio
  controls are explicitly recorded as the next component-expansion stage.
- Verification passed: `make test` (2/2 CTest tests), `make sanitize`,
  `make format-check`, `make verify` (32 Python tests plus strict C gates),
  `git diff --check`, and the regenerated `SISDK_ROOT=$SISDK_ROOT make firmware`
  build with all 145 target steps completed.

### RTT control review outcome (`e41a85d` -> `1aea0ed` -> `74f549f`)

- Initial readability and embedded reviews identified scan rollback, scanner
  component selection, unsupported option validation, and documentation issues.
- Commit `1aea0ed` enabled the required scanner/list components, regenerated the
  project, rejected unsupported scanner flag bits, and made scan rollback
  preserve/report the last known state.
- Commit `74f549f` corrected the architecture data flow to cover both legacy and
  extended advertisement events. Final readability and embedded re-reviews
  reported no confirmed findings; the embedded reviewer withdrew the earlier
  resolving-list concern after checking the successful 145-step build.
