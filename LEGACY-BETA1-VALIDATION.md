# Qonnected UDPST Legacy 7.4.0 beta.1 — validation record

Source branch: `qon/legacy`, tested commit `fb48c70`.

## CI and build
- GitHub Actions GCC and Clang builds and source regression checks: passed (run 44136102572).
- Debian 12 (`speedtest01`) out-of-tree CMake build: passed.
- Production Modern UDP 24601 and Legacy UDP 24602 remained running during tests.

## Runtime socket checks
- Isolated test server control port UDP 24612, `UDPST_DATA_PORT_RANGE=40500-40999`.
- IPv4 single session: server data UDP 40500.
- Four concurrent independent IPv4 client sessions: UDP 40500, 40501, 40502, 40503 simultaneously.
- Native IPv6 loopback (`::1`): server data UDP 40500.
- Occupied UDP 40500: next IPv4 session allocated UDP 40501; concurrent IPv6 session observed on UDP 40502.
- Exhaustion test used isolated control UDP 24613 and range UDP 41500-41501. Two simultaneous sessions allocated both ports; third client timed out; server logged `BIND ERROR: No available UDP data port in configured range (41500-41501): Address already in use`. Server remained listening.

## Exhaustion test client summaries
| Metric | Successful client A | Successful client B |
|---|---:|---:|
| Upstream delivered | 94.52% | 98.54% |
| Upstream average L3 Mbps | 520.54 | 2971.88 |
| Upstream maximum L3 Mbps | 1346.31 | 4138.09 |
| Lost / OoO / duplicate packets | 30523 / 0 / 0 | 24325 / 0 / 0 |
| RTT variation (ms) | 0–13 | 0–12 |

These are loopback measurements under simultaneous load, not validated network performance benchmarks. The third client's timeout is a known usability limitation; no explicit protocol-level refusal was observed.

## Release considerations
- Core fixed-port functionality is validated at socket level for IPv4, IPv6, concurrency, occupied-port skip and exhaustion.
- No production deployment performed.
- Consider explicit client error signaling on exhaustion in a later improvement; do not claim this exists in beta.1.
- External IPv4/IPv6 reachability and controlled performance benchmarking are not yet validated.
