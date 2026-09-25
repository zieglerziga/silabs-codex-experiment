import json
import sys
from pathlib import Path
import tempfile
import unittest


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import merge_compile_commands as merger


class MergeCompileCommandsTests(unittest.TestCase):
    def test_keeps_first_command_for_duplicate_source(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = root / "first.json"
            second = root / "second.json"
            output = root / "merged.json"
            first.write_text(
                '[{"file":"/repo/main.c","command":"first"}]',
                encoding="utf-8",
            )
            second.write_text(
                '[{"file":"/repo/main.c","command":"second"},'
                '{"file":"/repo/test.c","command":"test"}]',
                encoding="utf-8",
            )

            count = merger.merge_compile_commands([first, second], output)

            self.assertEqual(2, count)
            self.assertEqual(
                ["first", "test"],
                [
                    entry["command"]
                    for entry in json.loads(output.read_text(encoding="utf-8"))
                ],
            )

    def test_rejects_invalid_entry(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "invalid.json"
            path.write_text("[{}]", encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "invalid entry"):
                merger.merge_compile_commands([path], path)


if __name__ == "__main__":
    unittest.main()
