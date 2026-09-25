# Repository Working Agreement

This repository contains firmware for a Silicon Labs EFR32MG21 BLE scanner.

## Source of truth

- The supported target is the hardware detected on the development host:
  BRD4181A on BRD4001A, using `EFR32MG21A010F1024IM32`.
- The supported SDK is Simplicity SDK `v2025.6.3`. Keep the SDK outside this
  repository and do not edit its sources. `make prepare-sdk` is the only
  allowed exception: it may materialize selected Git LFS library objects.
- `devs-refd-ble-remote` is an architectural reference only. Do not copy its
  board target or product-specific behavior.

## Workflow

- Base feature branches on `development`; merge completed work back through a
  pull request.
- Keep [docs/STATUS.md](docs/STATUS.md) current at every meaningful stage so
  work can resume safely after an interrupted Codex session.
- Commit each meaningful, verified stage separately.
- After each stage, request two independent reviews: one focused on readability
  and documentation, and one focused on embedded correctness and defects.
- Record review results and the verification commands in `docs/STATUS.md`.
- Use scoped Conventional Commits (`type(scope): imperative summary`). Add a
  concise body when the motivation or verification is not obvious from the
  diff.
- Only coding agents work directly in this repository. Coordinate ownership of
  files, avoid overlapping edits, and leave every stage understandable to a
  human reader through committed documentation.
- Put any repeated command sequence in a versioned Python or POSIX shell script
  under `scripts/`; expose common operations through the root `Makefile`.

## Code navigation

- This repository uses a local CodeGraph index in `.codegraph/`.
- When `.codegraph/` exists, use `codegraph explore` before `rg`, `find`, or
  opening multiple source files for code-understanding tasks.
- Refresh the index with `codegraph sync` after material source changes.
- `.codegraph/` is local tool state and must not be committed.

## Generated and external content

- Do not edit generated Silicon Labs files unless regeneration is part of the
  documented workflow.
- Do not vendor the Simplicity SDK, ARM toolchain, or proprietary build tools.
- Keep host-independent project configuration in version control and put build
  output under `build/`.

## Quality bar

- Production firmware must compile with warnings treated as errors where the
  SDK permits it.
- Host-side tests and static checks must run without attached hardware.
- Never print every advertisement indefinitely. Logging must be bounded,
  deduplicated, and summarized so RTT remains useful under scan load.
- No secrets, private keys, device identifiers, or host-specific absolute paths
  may be committed.
