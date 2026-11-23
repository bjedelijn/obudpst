# Windows UDPST stabilization gate

Goal: stabilize native Windows UDPST without changing official TR-471 metrics or silently altering traffic pacing.

## Baseline / reproducibility

- Keep the current Windows pacing implementation as the baseline.
- Run the same host, machine, payload, 30-second duration and 100 Mbps rate for every A/B comparison.
- Record executable commit SHA, Windows version, CPU/power profile, `UDPST_WIN_PACING_MAX_LAG_MS`, JSON output and stderr diagnostics.
- Perform at least three runs per configuration; compare the distribution and worst intervals, not only the average.
- Do not compare TR-471 interval N directly with diagnostic window N: their boundaries can differ.

## Diagnostic invariants

- `[UDPST-CLIENT-RATE]` is calculated using monotonic elapsed time and Winsock-accepted bytes; it is NOT independently verified wire throughput.
- `[UDPST-LOOP-GAP]` includes all work and waits between main-loop iterations; it does NOT prove OS thread descheduling.
- `[UDPST-CLIENT-HEALTH]` is a suspicion signal, not an authoritative TR-471 result.
- Expected packet rate must use the configured payload and protocol overhead explicitly; do not conflate UDP payload Mbps, IP-layer Mbps and Ethernet Mbps.
- No Winsock failures does not mean packets reached the server.
- Never silently modify official TR-471 JSON, sender cadence, or wire packet sizes while adding diagnostics.

## Short iteration plan

1. Verify native Windows build on each commit; preserve a known-good binary.
2. Measure poll duration, timer/wait duration, send-call duration, and time spent in remaining loop work separately.
3. Add a controlled local loopback or LAN sender test to reproduce pacing without an external network bottleneck.
4. Add independent expected-versus-accepted packet/byte counters, with monotonic timebase.
5. Compare baseline and one experimental pacing change at a time (e.g. reduced busy-polling); keep it opt-in.
6. Promote a candidate only when repeatable tests show no throughput regression, no unexpected loss/reordering increase, and no protocol/JSON regression.

## Suggested Windows manual test

```powershell
$env:UDPST_TIMING_DIAG = "1"
$env:UDPST_WIN_PACING_MAX_LAG_MS = "10"
.\udpst.exe -u -4 -I 100 -t 30 -C 1 -f jsonf -y 1 -a speedtest.nikhef.qonnected.net 2> timing.log
```

Use the same command for the A/B variant, changing only the intended variable.
