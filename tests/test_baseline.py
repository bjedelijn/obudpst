"""Initial non-invasive source checks; not a substitute for runtime tests."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class BaselineTests(unittest.TestCase):
    def test_source_present(self):
        for filename in ("udpst.c", "udpst_control.c", "udpst_data.c", "CMakeLists.txt"):
            with self.subTest(filename=filename):
                self.assertTrue((ROOT / filename).is_file())

    def test_control_and_data_connection_logic_present(self):
        main = (ROOT / "udpst.c").read_text()
        control = (ROOT / "udpst_control.c").read_text()
        self.assertIn("conf.controlPort", main)
        self.assertIn("new_conn(", control)
        self.assertIn("locPort", control)

if __name__ == "__main__":
    unittest.main()
