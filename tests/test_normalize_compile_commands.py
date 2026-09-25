import sys
from pathlib import Path
import tempfile
import unittest


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import normalize_compile_commands as normalizer


class NormalizeCompileCommandsTests(unittest.TestCase):
    def test_replaces_paths_in_all_supported_fields(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "compile_commands.json"
            path.write_text(
                '[{"directory":"/workspace/build",'
                '"command":"/opt/simplicity_sdk/bin/cc -I/workspace/inc",'
                '"file":"/workspace/src/main.c",'
                '"arguments":["/workspace/src/main.c",'
                '"-I/opt/simplicity_sdk/include"]}]',
                encoding="utf-8",
            )

            applied = normalizer.normalize_compile_commands(
                path,
                path,
                [
                    ("/workspace", "/runner/repository"),
                    ("/opt/simplicity_sdk", "/runner/sdk"),
                ],
                Path(directory),
            )

            self.assertEqual(5, applied)
            self.assertNotIn("/workspace", path.read_text(encoding="utf-8"))
            self.assertNotIn(
                "/opt/simplicity_sdk", path.read_text(encoding="utf-8")
            )

    def test_rejects_non_array_database(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "compile_commands.json"
            path.write_text("{}", encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "JSON array"):
                normalizer.normalize_compile_commands(path, path, [], Path(directory))

    def test_rejects_paths_outside_trusted_root(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "root"
            root.mkdir()
            outside = Path(directory) / "outside.json"
            outside.write_text("[]", encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "outside trusted root"):
                normalizer.normalize_compile_commands(
                    outside,
                    root / "output.json",
                    [],
                    root,
                )


if __name__ == "__main__":
    unittest.main()
