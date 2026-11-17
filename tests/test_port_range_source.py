"""Static regression checks for opt-in server data port range wiring.

Runtime bind and interoperability tests are still required before deployment.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class PortRangeSourceTests(unittest.TestCase):
    def test_range_configuration_and_validation(self):
        source = (ROOT / "udpst.c").read_text()
        self.assertIn('getenv("UDPST_DATA_PORT_RANGE")', source)
        self.assertIn("first > last", source)
        self.assertIn("conf.controlPort >= first", source)

    def test_server_bind_range(self):
        source = (ROOT / "udpst_control.c").read_text()
        self.assertIn("repo.isServer && port == 0 && conf.dataPortStart > 0", source)
        self.assertIn("candidate <= conf.dataPortEnd", source)
        self.assertIn("errno != EADDRINUSE", source)
        self.assertIn("sin6_port = htons", source)
        self.assertIn("sin_port = htons", source)

if __name__ == "__main__":
    unittest.main()
