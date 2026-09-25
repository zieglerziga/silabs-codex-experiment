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

    def test_detects_unquoted_posix_path_in_linker_script(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.ld"), "SEARCH_DIR(/srv/toolchain/lib)"
        )
        self.assertEqual("/srv/toolchain/lib", detected)

    def test_detects_quoted_windows_drive_path(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), 'set(TOOL_PATH "C:\\Silabs\\toolchain")'
        )
        self.assertEqual("C:\\Silabs\\toolchain", detected)

    def test_detects_quoted_windows_unc_path(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), 'set(TOOL_PATH "\\\\builder\\sdk\\lib")'
        )
        self.assertEqual("\\\\builder\\sdk\\lib", detected)

    def test_detects_posix_include_path_in_flags(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), 'set(CMAKE_C_FLAGS "-I/opt/silabs/sdk")'
        )
        self.assertEqual("/opt/silabs/sdk", detected)

    def test_detects_posix_path_in_semicolon_list(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), 'set(PATHS "relative;/home/builder/sdk")'
        )
        self.assertEqual("/home/builder/sdk", detected)

    def test_detects_posix_path_in_generator_expression(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"),
            'target_include_directories(app PRIVATE "$<BUILD_INTERFACE:/srv/sdk>")',
        )
        self.assertEqual("/srv/sdk", detected)

    def test_detects_posix_path_in_linker_flags(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), 'set(LINK_FLAGS "-Wl,-rpath,/opt/sdk")'
        )
        self.assertEqual("/opt/sdk", detected)

    def test_detects_unquoted_posix_path_in_generator_expression(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"),
            "target_include_directories(app PRIVATE $<BUILD_INTERFACE:/srv/sdk>)",
        )
        self.assertEqual("/srv/sdk", detected)

    def test_detects_forward_unc_in_generator_expression_list(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"),
            'set(INCLUDES "$<BUILD_INTERFACE://builder/sdk;relative>")',
        )
        self.assertEqual("//builder/sdk", detected)

    def test_detects_unquoted_posix_path_in_linker_flags(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), "set(LINK_FLAGS -Wl,-rpath,/opt/sdk)"
        )
        self.assertEqual("/opt/sdk", detected)

    def test_detects_startup_path_in_linker_script(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.ld"), "STARTUP(/srv/toolchain/crt0.o)"
        )
        self.assertEqual("/srv/toolchain/crt0.o", detected)

    def test_detects_output_path_in_linker_script(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.ld"), "OUTPUT(/srv/build/app.out)"
        )
        self.assertEqual("/srv/build/app.out", detected)

    def test_detects_later_input_path_in_linker_script(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.ld"), "INPUT(relative.o /srv/toolchain/libsupport.a)"
        )
        self.assertEqual("/srv/toolchain/libsupport.a", detected)

    def test_detects_later_group_path_in_linker_script(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.ld"), "GROUP(libfoo.a, /srv/toolchain/libbar.a)"
        )
        self.assertEqual("/srv/toolchain/libbar.a", detected)

    def test_detects_forward_slash_unc_in_linker_list(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.ld"), "GROUP(relative.o, //builder/sdk/libbar.a)"
        )
        self.assertEqual("//builder/sdk/libbar.a", detected)

    def test_detects_unquoted_windows_unc_path(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.ld"), "SEARCH_DIR(\\\\builder\\sdk\\lib)"
        )
        self.assertEqual("\\\\builder\\sdk\\lib", detected)

    def test_detects_unquoted_forward_slash_unc_path(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), "set(SDK_MIRROR //builder/sdk/lib)"
        )
        self.assertEqual("//builder/sdk/lib", detected)

    def test_allows_cpp_comments(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.c"),
            "// Generated by SLC\n/* ratio /count */\nint value = total /count;",
        )
        self.assertIsNone(detected)

    def test_allows_https_url(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), 'set(DOCUMENTATION "https://example.com/sdk")'
        )
        self.assertIsNone(detected)

    def test_allows_unquoted_https_url(self) -> None:
        detected = normalizer.find_absolute_path(
            Path("generated.cmake"), "set(DOCUMENTATION https://example.com/sdk)"
        )
        self.assertIsNone(detected)

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
