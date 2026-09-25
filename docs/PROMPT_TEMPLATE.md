# Embedded Project Agent Prompt Template

Copy the prompt below into a new coding task and fill in the bracketed fields.
Remove requirements that do not apply only after agreeing on the change with
the project owner.

```text
Create or continue the embedded project described here.

## Objective

[Describe the firmware/product behavior and the outcome to deliver.]

## Confirmed environment

- Repository: [owner/repository]
- Integration/base branch: [for example: development]
- Target board and carrier: [exact board identifiers/revisions]
- MCU/SoC: [exact part]
- SDK and supported version: [exact version/tag or commit]
- Connected hardware available on this host: [probe, board, serial/RTT paths]
- Build tools and versions: [SLC/vendor CLI, compiler, CMake, Docker, etc.]
- Architectural reference repositories: [URLs and what may be learned from them]
- Explicitly out of scope: [features, power goals, product behavior, etc.]

## Working rules

- Treat the detected hardware and pinned SDK above as the source of truth.
  Keep vendor SDKs and proprietary tools outside the repository. Do not change
  SDK sources or copy product-specific behavior from an architectural example.
- Inspect the existing repository instructions first. When a local CodeGraph
  index exists, use it before broad searches or multi-file code reading and
  refresh it after material source changes.
- Keep generated/vendor files in the documented generation workflow. Keep
  host-specific absolute paths, credentials, device identifiers, build output,
  and local tool state out of commits.
- Put repeated command sequences in versioned Python or POSIX shell scripts
  under scripts/ and expose common operations in the root Makefile.
- Use scoped Conventional Commits. Commit each meaningful verified stage
  separately and push the working feature branch to origin for backup.
- Keep docs/STATUS.md current at each meaningful stage. Record decisions,
  changes, exact verification commands/results, review findings and fixes, CI
  run IDs, current branch/head, and safe next steps so another person can
  resume without this conversation.
- Only coding agents work directly in the repository. Assign clear file/task
  ownership and avoid overlapping edits.

## Development workflow

1. Inspect repository state and instructions. Confirm the board/SDK/tool
   assumptions from available local evidence. Ask only for decisions that
   materially change the project; continue independent discovery meanwhile.
2. If useful, delegate bounded information-gathering to a Luna agent and
   platform/build/CI research to a Sol agent. Treat their results as evidence
   to verify, not as authority to change scope.
3. Create or continue a feature branch based on [integration branch]. Make
   bootstrap PR automation available from the base branch before relying on PR
   checks. Follow the repository's documented bootstrap procedure.
4. Implement in small stages. Start with portable logic and hardware-free
   tests where possible, then integrate vendor APIs, generated project output,
   and hardware-specific behavior.
5. At each meaningful stage, run the relevant host/static/build/hardware
   checks. Request two independent read-only coding-agent reviews:
   - Junior software reviewer: readability, documentation, usability, and
     maintainability.
   - Senior embedded reviewer: correctness, hardware/SDK integration,
     resource constraints, build, and security defects.
   For test-heavy or safety-critical changes, also request a strict test
   architect review of coverage, test oracles, boundary cases, and false
   confidence. Reviewers must report severity and exact file/line locations;
   they do not edit files. Fix findings in a separate commit, rerun checks, and
   request re-review of the fixes.
6. Keep repetitive verification in scripts/Makefile. At minimum, provide
   hardware-independent tests/static checks and a reproducible target build.
   Run host checks without requiring hardware. Exercise attached hardware when
   the requested feature depends on it and record what was actually observed.
7. Open a PR from the feature branch to [integration branch]. Check the exact
   PR head's required actions, including firmware compilation and security
   scans. Fix failures, push the fix, and re-review. Do not report a prior
   commit's green checks as proof for a newer head.
8. Do not merge unless the project owner explicitly requested merging.

## Repository-specific details

- Architecture or reference project: [name/link and allowed scope]
- Status log: docs/STATUS.md
- Human setup/build guide: [README or path]
- Main local commands: [make verify, make workflow-check, make firmware, etc.]
- Firmware CI: [workflow path and expected job names]
- PR security tools: [actionlint, zizmor, secret scanner, action pin audit]
- Hardware smoke test: [flash/capture command and success criteria]
- Known limitations or follow-up coverage risks: [list]

Begin by reporting the repository state and the first concrete stage. Then
implement, document, verify, commit, push, review, and re-review through the
requested outcome. Do not stop at a plan or partial implementation.
```
