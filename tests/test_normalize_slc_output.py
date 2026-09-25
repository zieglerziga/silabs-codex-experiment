import sys
from pathlib import Path
import unittest


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import normalize_slc_output as normalizer


class NormalizeSlcOutputTests(unittest.TestCase):
    def test_detects_arbitrary_quoted_posix_path(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), 'set(TOOL_PATH "/srv/toolchain/bin")'
        )
        self.assertEqual("/srv/toolchain/bin", detected)

    def test_detects_unquoted_posix_path(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), "set(TOOL_PATH /usr/local/bin)"
        )
        self.assertEqual("/usr/local/bin", detected)

    def test_allows_environment_based_path(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), 'set(SDK_PATH "$ENV{SISDK_ROOT}")'
        )
        self.assertIsNone(detected)

    def test_rejects_duplicate_sdk_assignments(self) -> None:
        source = 'set(SDK_PATH "/one")\nset(SDK_PATH "/two")\n'
        with self.assertRaises(SystemExit):
            normalizer.replace_exactly_once(
                normalizer.SDK_PATTERN,
                normalizer.PORTABLE_SDK_BLOCK,
                source,
                "SDK_PATH",
                Path("generated.cmake"),
            )


if __name__ == "__main__":
    unittest.main()
