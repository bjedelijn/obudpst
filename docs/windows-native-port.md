# Native Windows client port — implementation plan

Status: initial source inspection completed; no Windows executable built yet.

## Objective
Build a native Windows x64 `udpst.exe` from this fork without WSL, preserving the existing Linux server behavior and TR-471/OB-UDPST measurement logic.

Target test server: `speedtest.nikhef.qonnected.net`
- Modern UDP control port: 24601
- Legacy UDP control port: 24602
- Test-session data ports are assigned dynamically by UDPST (e.g. 40500); do not hard-code them.

## Source observations (main branch)
- `CMakeLists.txt` creates `udpst_core` from `udpst_control.c`, `udpst_data.c`, `udpst_srates.c`, `cJSON.c`, and the `udpst` executable from `udpst.c`.
- CMake probes POSIX/Linux facilities including `unistd.h`, `getifaddrs`, `fork`, `sendmmsg`, `recvmmsg`, `UDP_SEGMENT` and `mkfifo`.
- Existing link line includes Unix `m`; this must be conditional on platform.
- OpenSSL >=3.0 is optional and enables authentication support.
- README documents `-C` for multiple connections and `-f json` for machine-readable output.

## Planned implementation stages
1. Audit source files for socket APIs, `select`/`poll`, file descriptors, timers, signals, platform-specific networking and CLI dependencies.
2. Introduce a small Windows portability layer (Winsock startup/cleanup, socket type/error handling, close, nonblocking mode, high-resolution clock/sleep), preserving Unix code paths.
3. Make CMake platform-aware, link `ws2_32` on Windows and Unix math library only where required. Explicitly disable unsupported Linux-specific optimizations on Windows.
4. Add GitHub Actions Windows x64 build and upload `udpst.exe` as an artifact. Preserve copyright/license notices in distributions.
5. Validate CLI startup, `-?`, IPv4/IPv6, modern 24601, legacy 24602, download/upload, JSON output and `-C 1/2/4/8` where server permits.
6. Compare native Windows results to FRITZ!Box tests on the same 1 Gbit/s line; investigate CPU utilization and packet loss.

## Guardrails
- Do not modify the Linux server configuration.
- Do not merge to `main` until Windows CI builds and basic client tests pass.
- A successful compile alone does not prove measurement accuracy.

## Windows compatibility targets
- Primary: Windows 11 x64; Windows Server 2016, 2019, 2022 and 2025 x64.
- Legacy validation targets: Windows Server 2012 and 2012 R2 x64, subject to testing and compatible toolchain/API selection.
- Clarification required: there is no standalone Microsoft product named "Windows Server 2011". If Windows Small Business Server 2011 is intended, it is based on Windows Server 2008 R2 and requires a separate compatibility assessment.
- Avoid introducing mandatory .NET or WSL dependencies; aim for a standalone native executable. Static linking and actual minimum supported Windows API version must be verified by CI and runtime testing.
- Build on Windows 11/modern Windows Server first, then test legacy server compatibility explicitly; do not claim untested OS support.
