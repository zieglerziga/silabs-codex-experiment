import sys
from pathlib import Path
import unittest


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

import validate_rtt_capture as validator


class ValidateRttCaptureTests(unittest.TestCase):
    def test_initialization_without_scan_start_fails(self) -> None:
        assessment = validator.assess_rtt_capture(
            "BLE scanner initialized; waiting for stack boot\n"
        )
        self.assertEqual((False, False), assessment)

    def test_scan_start_without_advertisement_is_distinct(self) -> None:
        assessment = validator.assess_rtt_capture(
            "BLE scan started: passive, interval=100ms, window=50ms, PHY=1M\n"
            "summary t=30000 reports=0 discoveries=0 active=0 evictions=0 "
            "suppressed=0 malformed=0\n"
        )
        self.assertEqual((True, False), assessment)

    def test_scan_start_with_advertisement_is_complete_smoke_evidence(self) -> None:
        assessment = validator.assess_rtt_capture(
            "BLE scan started: passive, interval=100ms, window=50ms, PHY=1M\n"
            "scan t=250 addr=06:05:04:03:02:01 type=0 rssi=-50 "
            'name="Test" reason=first reports=1\n'
        )
        self.assertEqual((True, True), assessment)


if __name__ == "__main__":
    unittest.main()
