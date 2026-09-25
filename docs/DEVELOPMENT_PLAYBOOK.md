# Development Playbook

This document records how the BLE scanner was developed and provides a
repeatable workflow for future embedded projects. The project-specific facts
and current outcome are a case study; the practices are intended to be reused.

## Project case study

### Start from the machine and the source of truth

The target was established from the hardware attached to the development host:
BRD4181A on BRD4001A with `EFR32MG21A010F1024IM32`. The supported SDK was fixed
at Simplicity SDK `v2025.6.3` and kept outside the repository. The
`devs-refd-ble-remote` repository was used to study project structure and build
patterns, not as a source to copy board choices or product behavior from.

The initial exploration also checked the available SLC/Commander/toolchain
versions, initialized local CodeGraph for code navigation, and mapped the
existing SDK-generated project layout. These checks turned assumptions about
the board, SDK, generated files, and attached debug probe into documented facts
before implementation began. See [STATUS.md](STATUS.md) for the hardware and
tool baseline.

### Build a portable core before wiring in the SDK

The scanner tracker was implemented as SDK-independent C first. It uses a
fixed 32-entry cache, defensive advertising-data parsing, per-device and global
log limits, oldest-seen eviction, periodic refreshes, and cumulative summary
counters. Host tests and sanitizers exercise this logic without a board or
proprietary tools.

The Silicon Labs layer was then kept thin: generated project code initializes
the stack, while a hand-written adapter starts passive legacy scanning,
translates reports into tracker observations, and emits bounded RTT lines.
Project generation runs in a staging directory; normalization and path checks
finish before generated output is installed transactionally. Host-specific
paths and the SDK itself stay out of version control.

### Make each build reproducible

Repeated operations were put in scripts under `scripts/` and exposed through
the root `Makefile`. The repository has separate paths for host verification,
SLC generation, SDK LFS preparation, firmware compilation, container builds,
flashing, and bounded RTT capture.

The container pins its Ubuntu base, package snapshot, and Arm GNU toolchain
checksum. SDK source remains external and is mounted read-only for the firmware
build. The CI SDK helper checks out the exact public SDK commit and materializes
only the LFS archives selected by the generated project. Project-owned
`app.c` and `scan_tracker.c` compile with warnings treated as errors; SDK and
generated files keep Silicon Labs' warning policy.

### Use review findings to strengthen the tests

After each meaningful stage, request two independent read-only coding-agent
reviews:

- A junior software review focused on readability, discoverability, shell and
  Python clarity, and documentation.
- A senior embedded review focused on correctness, SDK integration, generated
  code, build and workflow security, and hardware behavior.

Reviewers inspected without editing. Findings were recorded with file and line
references, fixed by the implementation agent in a follow-up commit, verified,
and sent back for another review. For test-heavy or safety-critical work, add a
strict test-architect review that challenges coverage, test oracles, boundary
cases, and false confidence. Keep the concrete findings, resolutions, and
remaining coverage risks in [STATUS.md](STATUS.md); that historical log is the
source of truth for this project's review details and is intentionally not
duplicated in the reusable playbook.

### Establish pull-request checks before opening the PR

The integration branch is `development`; feature branches start from it and
merge back through a pull request. For this new repository, a small bootstrap
workflow was first committed to `main` and `development`, as requested, so
GitHub had pull-request automation available on the base branch before the
feature PR was opened. The full verification workflow was then added to the
base branch. This bootstrap job only confirms automation is active; it is not a
substitute for the quality gates.

The full verification workflow runs for PRs targeting `development`, pushes to
`development`, and manual dispatch. It defines these jobs; secret scanning is
intentionally limited to pull requests because it needs the PR base/head range:

| Check | What it verifies |
| --- | --- |
| Host checks | `make verify`, range-aware whitespace validation, actionlint, C/Python tests, formatting, and sanitizers |
| Firmware build | Exact SDK checkout, selected LFS materialization, pinned container, and complete EFR32MG21 firmware link |
| Workflow security | zizmor audit |
| Secret scan (pull requests only) | TruffleHog over the exact PR base-to-head commit range |

The separate PR bootstrap workflow runs for pull requests targeting
`development` and manual dispatch. Its small confirmation job verifies that
GitHub has registered the pull-request automation; it is not a quality gate
from the full verification workflow.

Workflows use read-only permissions, pinned action commit SHAs, and ordinary
`pull_request` events. The firmware job uses no secret or PAT. A manual
`make actions-audit` compares external actions with their latest stable release
tags and resolved commits; it also rejects mutable Docker action references.
The stable-action audit is documented in the README and covered by host tests.

### Stage, review, commit, and check

Every stage should leave a short entry in [STATUS.md](STATUS.md): what changed,
why, review findings and resolutions, commands run, relevant CI run IDs, current
branch/commit, and a safe next step. Do this before handing off or stopping.

Keep each meaningful, verified stage in its own scoped Conventional Commit,
for example `ci(firmware): compile target in pull requests`. Add a body when
the motivation or verification is not obvious from the patch. Push feature
commits to `origin` for backup. Do not claim CI passed on a commit until that
specific head's check run has finished successfully. After updating a
documentation-only status commit, wait for the resulting checks too.

The resulting pull request was [#1](https://github.com/zieglerziga/silabs-codex-experiment/pull/1),
from `feature/ble-rtt-scanner` to `development`. It passed host, firmware,
workflow-security, secret-scan, and bootstrap checks, then merged on 2026-09-25
as `f69f79142c8fde207a9c3d7130eeb7d9c13a677c`. The final feature branch backup
was `97d7bf8`; PR #1 contains the complete reviewed history.

## Reusable prompt template

Copy [PROMPT_TEMPLATE.md](PROMPT_TEMPLATE.md) when starting a future embedded
project. Replace the bracketed project facts and objective first. Keep the
source-of-truth, staged-review, reproducibility, status-log, branch, PR, and
verification requirements unless the new project's owner explicitly changes
them.
