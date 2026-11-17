# Modern UDPST 9.1.0 — Beta 1 validation record

Date: 2026-11-17
Candidate branch: `release/modern-v9.1.0-beta.1`
Source baseline: `cfddec5f092c1aafd961d7604faafa9a8da495ed`

## Environment

Debian 12 test server, modern UDPST 9.1.0, control UDP/24601, configured data range UDP/40000-40499. Legacy service remained on UDP/24602. Authenticated tests used local loopback; credentials are not included.

## Observed functional results

- IPv4, 1 flow: server bound UDP/40000.
- IPv4, 4 concurrent flows: server bound UDP/40000, 40001, 40002, 40003.
- Native IPv6 (::1), 4 concurrent flows: server bound UDP/40000, 40001, 40002, 40003.
- With UDP/40000 reserved, native IPv6 4-flow test: server bound UDP/40001 through 40004.
- IPv4 upstream, 4 flows, completed: 98.37% delivered, 25,932 lost, 0 out-of-order, 0 duplicate, average 2618.56 Mbps L3/IP, maximum 5103.39 Mbps L3/IP.
- IPv4 downstream, 4 flows, completed: 98.89% delivered, 15,899 lost, 0 out-of-order, 0 duplicate, average 3016.10 Mbps L3/IP, maximum 7060.37 Mbps L3/IP.

These are loopback measurements, **not** external network throughput benchmarks. Packet loss was observed under high local traffic load. The evidence consists of user-provided terminal output; no automated test logs are attached.

## Remaining gates

- Verify CI on this candidate commit and build downloadable artifacts.
- Test full-range exhaustion and cleanup/reuse.
- Test externally over IPv4 and IPv6, including firewall policy.
- Consider IPv4-specific occupied-port test.
- Review error handling and source-level test coverage.

**Status:** Beta 1 source candidate prepared; not tagged or published as a GitHub Release. No Stable/RC claim.
