#!/usr/bin/env python3
"""Compare immutable GitHub Action pins with latest stable releases."""

from __future__ import annotations

import json
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import quote


REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
WORKFLOW_ROOT = REPOSITORY_ROOT / ".github" / "workflows"
USES_PATTERN = re.compile(
    r"^\s*(?:-\s*)?uses:\s*(?P<target>\S+)"
    r"(?:\s+#\s*(?P<tag>\S+))?\s*$",
    re.MULTILINE,
)


def github_api(endpoint: str) -> dict[str, object]:
    result = subprocess.run(
        ["gh", "api", endpoint],
        check=True,
        capture_output=True,
        text=True,
    )
    value = json.loads(result.stdout)
    if not isinstance(value, dict):
        raise RuntimeError(f"unexpected GitHub response for {endpoint}")
    return value


def workflow_references() -> dict[str, set[tuple[str, str]]]:
    references: dict[str, set[tuple[str, str]]] = {}
    workflow_paths = sorted(WORKFLOW_ROOT.glob("*.yml"))
    workflow_paths.extend(sorted(WORKFLOW_ROOT.glob("*.yaml")))

    for workflow_path in workflow_paths:
        source = workflow_path.read_text(encoding="utf-8")
        for match in USES_PATTERN.finditer(source):
            target = match.group("target")
            if target.startswith(("./", "$/")):
                continue

            if target.startswith("docker://"):
                image = target.removeprefix("docker://")
                if not re.fullmatch(r".+@sha256:[0-9a-f]{64}", image):
                    relative_path = workflow_path.relative_to(REPOSITORY_ROOT)
                    raise RuntimeError(
                        "mutable Docker action reference in "
                        f"{relative_path}: {target}"
                    )
                continue

            action_path, separator, commit = target.rpartition("@")
            path_parts = action_path.split("/")
            tag = match.group("tag")
            if (
                not separator
                or len(path_parts) < 2
                or not re.fullmatch(r"[0-9a-f]{40}", commit)
                or not tag
            ):
                try:
                    display_path = workflow_path.relative_to(REPOSITORY_ROOT)
                except ValueError:
                    display_path = workflow_path
                raise RuntimeError(
                    f"unversioned action reference in {display_path}: {target}"
                )

            repository = "/".join(path_parts[:2])
            reference = (tag, commit)
            references.setdefault(repository, set()).add(reference)

    if not references:
        raise RuntimeError(f"no immutable action references found in {WORKFLOW_ROOT}")
    return references


def latest_release(repository: str) -> tuple[str, str]:
    release = github_api(f"repos/{repository}/releases/latest")
    if release.get("draft") or release.get("prerelease"):
        raise RuntimeError(f"latest release for {repository} is not stable")

    tag = release.get("tag_name")
    if not isinstance(tag, str) or not tag:
        raise RuntimeError(f"latest release for {repository} has no tag")

    reference = github_api(
        f"repos/{repository}/git/ref/tags/{quote(tag, safe='')}"
    )
    target = reference.get("object")
    while isinstance(target, dict) and target.get("type") == "tag":
        tag_object = github_api(
            f"repos/{repository}/git/tags/{target.get('sha')}"
        )
        target = tag_object.get("object")

    if not isinstance(target, dict) or target.get("type") != "commit":
        raise RuntimeError(f"tag {repository}@{tag} does not resolve to a commit")
    commit = target.get("sha")
    if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise RuntimeError(f"tag {repository}@{tag} resolved to an invalid commit")
    return tag, commit


def main() -> int:
    outdated = False
    for repository, references in sorted(workflow_references().items()):
        latest_tag, latest_commit = latest_release(repository)
        expected = {(latest_tag, latest_commit)}
        status = "current" if references == expected else "outdated"
        print(f"{status}: {repository}@{latest_commit} # {latest_tag}")
        if references != expected:
            for tag, commit in sorted(references):
                print(f"  found: {repository}@{commit} # {tag}")
            outdated = True
    return 1 if outdated else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"action audit failed: {error}", file=sys.stderr)
        raise SystemExit(2) from error
