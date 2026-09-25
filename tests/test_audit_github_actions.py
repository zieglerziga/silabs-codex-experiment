import sys
from pathlib import Path
import tempfile
import unittest
from unittest import mock


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import audit_github_actions as audit


class AuditGithubActionsTests(unittest.TestCase):
    def test_collects_unique_external_references(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            workflow_root = Path(temporary_directory)
            (workflow_root / "checks.yml").write_text(
                "steps:\n"
                "  - uses: owner/action@" + "a" * 40 + " # v1.2.3\n"
                "  - uses: owner/action@" + "a" * 40 + " # v1.2.3\n"
                "  - uses: ./local-action\n"
                "  - uses: $/recommended-local-action\n"
                "  - uses: docker://registry.example/action@sha256:"
                + "b" * 64
                + "\n",
                encoding="utf-8",
            )

            with mock.patch.object(audit, "WORKFLOW_ROOT", workflow_root):
                references = audit.workflow_references()

        self.assertEqual({("v1.2.3", "a" * 40)}, references["owner/action"])

    def test_rejects_mutable_action_reference(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            workflow_root = Path(temporary_directory)
            (workflow_root / "checks.yaml").write_text(
                "steps:\n  - uses: owner/action@v1\n",
                encoding="utf-8",
            )

            with mock.patch.object(audit, "WORKFLOW_ROOT", workflow_root):
                with self.assertRaisesRegex(RuntimeError, "unversioned action"):
                    audit.workflow_references()

    def test_rejects_mutable_docker_action_reference(self) -> None:
        with tempfile.TemporaryDirectory(dir=audit.REPOSITORY_ROOT) as directory:
            workflow_root = Path(directory)
            (workflow_root / "checks.yml").write_text(
                "steps:\n"
                "  - uses: owner/action@" + "a" * 40 + " # v1.2.3\n"
                "  - uses: docker://registry.example/action:latest\n",
                encoding="utf-8",
            )

            with mock.patch.object(audit, "WORKFLOW_ROOT", workflow_root):
                with self.assertRaisesRegex(RuntimeError, "mutable Docker action"):
                    audit.workflow_references()

    def test_resolves_annotated_release_tag_to_commit(self) -> None:
        commit = "b" * 40
        responses = {
            "repos/owner/action/releases/latest": {
                "draft": False,
                "prerelease": False,
                "tag_name": "v2.0.0",
            },
            "repos/owner/action/git/ref/tags/v2.0.0": {
                "object": {"type": "tag", "sha": "c" * 40}
            },
            f"repos/owner/action/git/tags/{'c' * 40}": {
                "object": {"type": "commit", "sha": commit}
            },
        }

        with mock.patch.object(audit, "github_api", side_effect=responses.__getitem__):
            self.assertEqual(("v2.0.0", commit), audit.latest_release("owner/action"))

    def test_main_reports_outdated_reference(self) -> None:
        with mock.patch.object(
            audit,
            "workflow_references",
            return_value={"owner/action": {("v1.0.0", "a" * 40)}},
        ), mock.patch.object(
            audit,
            "latest_release",
            return_value=("v2.0.0", "b" * 40),
        ), mock.patch("builtins.print"):
            self.assertEqual(1, audit.main())


if __name__ == "__main__":
    unittest.main()
