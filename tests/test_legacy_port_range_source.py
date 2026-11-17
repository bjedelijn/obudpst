"""Regression checks for Legacy UDPST's optional fixed data port allocation."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LegacyPortRangeSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main = (ROOT / "udpst.c").read_text()
        cls.control = (ROOT / "udpst_control.c").read_text()
        cls.header = (ROOT / "udpst.h").read_text()
        cls.protocol = (ROOT / "udpst_protocol.h").read_text()

    def test_legacy_protocol_unchanged(self):
        self.assertRegex(self.protocol, r"#define\s+PROTOCOL_VER\s+9\b")
        self.assertRegex(self.protocol, r"#define\s+PROTOCOL_MIN\s+8\b")

    def test_optional_server_only_range(self):
        self.assertIn('getenv("UDPST_DATA_PORT_RANGE")', self.main)
        self.assertRegex(self.main, r"if\s*\(repo\.isServer\)\s*\{\s*const char \*range")
        self.assertIn("conf.dataPortStart = first;", self.main)
        self.assertIn("conf.dataPortEnd = last;", self.main)

    def test_range_validation(self):
        for term in ("first < 1", "last > 65535", "first > last",
                     "conf.controlPort >= first && conf.controlPort <= last"):
            self.assertIn(term, self.main)

    def test_data_sockets_only_and_ephemeral_fallback(self):
        self.assertIn("repo.isServer && port == 0 && conf.dataPortStart > 0", self.control)
        self.assertIn("else if (bind(fd, ai->ai_addr, ai->ai_addrlen) == -1)", self.control)
        self.assertIn("errno != EADDRINUSE", self.control)
        self.assertIn("candidate <= conf.dataPortEnd", self.control)
        self.assertIn("No available UDP data port", self.control)

    def test_configuration_fields(self):
        self.assertIn("int dataPortStart;", self.header)
        self.assertIn("int dataPortEnd;", self.header)


if __name__ == "__main__":
    unittest.main()
