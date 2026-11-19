# Qonnected NOC / Onvi Windows Speedtest — development scaffold

This branch builds on `feature/windows-native`. **The native Windows TR-471 engine is
not yet validated or production ready.** The WPF UI cannot run real tests until
the native client binaries compile, pass protocol compatibility tests and are
placed next to the GUI.

## Architecture
- `Qonnected.Speedtest`: .NET 8 WPF UI, NOC and Onvi modes.
- `udpst-modern.exe`: native modern TR-471 client (not supplied yet).
- `udpst-legacy.exe`: native legacy TR-471 client (not supplied yet).
- NOC can choose modern/legacy, download/upload/both, copy/export full JSON.
- Onvi mode hides configuration and technical export; uses an externally supplied
  `onvi-settings.json` containing a verified publicly reachable server and port.
- Both clients run the original engine with `-f json -p PORT -d/-u HOST`.
- The GUI preserves original engine JSON as nested results, without inventing
  metrics or pretending a local measurement ID is a server-side correlation ID.

## Build
Requires Windows, .NET 8 SDK with Windows desktop targeting support:

```powershell
dotnet build windows/Qonnected.Speedtest/Qonnected.Speedtest.csproj -c Release
dotnet publish windows/Qonnected.Speedtest/Qonnected.Speedtest.csproj -c Release -r win-x64
```

NOC: launch `Qonnected.Speedtest.exe`.

Onvi: deploy the same UI with `SPEEDTEST_EDITION=onvi` set in its launcher or
environment and a locked, deployment-controlled `onvi-settings.json`:
```json
{ "server": "REPLACE_WITH_VERIFIED_PUBLIC_FQDN", "port": 24601 }
```
This is a **developer scaffold**, not a customer distribution. Before shipping,
make the Onvi edition a distinct branded executable with embedded/verified
configuration and no environment-variable mode switch; implement usable customer
summary rather than raw JSON; sign and package the binaries. Do not distribute
the placeholder configuration.

## Outstanding
1. Finish and test the native Windows UDPST modern client.
2. Port/test legacy client and validate protocol 8/9 negotiation.
3. Validate JSON output and argument behavior for both engines.
4. Add end-to-end integration tests against speedtest01 on both ports.
5. Replace customer raw JSON display with understandable speed measurements.
6. Agree correlation with Speedtest Results Manager; server measurements alone
   do not guarantee a stable customer measurement ID.
7. Build separate NOC and Onvi signed installers; validate privacy disclosure.
