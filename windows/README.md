# Qonnected NOC / Onvi Windows Speedtest — development scaffold

This branch builds on `feature/windows-native`. **The native Windows TR-471 engine is
not yet validated or production ready.** The WPF UI cannot run real tests until
the unified native client compiles, pass protocol compatibility tests and are
placed next to the GUI.

## Architecture
- `Qonnected.Speedtest`: .NET 8 WPF UI, NOC and Onvi modes.
- `udpst.exe`: one native TR-471 engine supporting modern and legacy modes (not implemented yet).
- NOC can choose modern/legacy, download/upload/both, copy/export full JSON.
- Onvi mode hides configuration and technical export; uses an externally supplied
  `onvi-settings.json` containing a verified publicly reachable server and port.
- Both GUIs call the same engine with `--protocol modern|legacy -f json -p PORT -d/-u HOST`.
- The `--protocol` interface is a proposed contract for the unified engine; native implementation is still required.
- The GUI preserves original engine JSON as nested results, without inventing
  metrics or pretending a local measurement ID is a server-side correlation ID.

## Build
Requires Windows, .NET 8 SDK with Windows desktop targeting support:

```powershell
dotnet build windows/Qonnected.Speedtest/Qonnected.Speedtest.csproj -c Release
dotnet publish windows/Qonnected.Speedtest/Qonnected.Speedtest.csproj -c Release -r win-x64
```

NOC: launch `Qonnected.Speedtest.exe` from the default build.

Onvi: build with `-p:SpeedtestEdition=Onvi` to produce a separate `Onvi.Speedtest.exe` and deploy it with a deployment-controlled `onvi-settings.json`:
```json
{ "server": "REPLACE_WITH_VERIFIED_PUBLIC_FQDN", "port": 24601 }
```
This is a **developer scaffold**, not a customer distribution. Before shipping,
embed/verify the Onvi public endpoint rather than relying on a replaceable JSON
configuration; validate the speed metric mapping from actual engine JSON;
include original logo PNG assets; sign and package the binaries. Do not distribute
the placeholder configuration.

## Outstanding
1. Implement and test one native Windows `udpst.exe` supporting modern and legacy modes.
2. Validate legacy protocol 8/9 negotiation within that single engine.
3. Validate JSON output and argument behavior for both protocol modes.
4. Add end-to-end integration tests against speedtest01 on both ports.
5. Replace the customer placeholder summary with validated download/upload speed measurements.
6. Agree correlation with Speedtest Results Manager; server measurements alone
   do not guarantee a stable customer measurement ID.
7. Produce signed NOC and Onvi installers; validate privacy disclosure.
8. Current CI builds the GUIs only; a green workflow does not establish native UDPST engine compatibility.
9. Ensure the unified engine implements the proposed `--protocol modern|legacy` CLI contract before end-to-end testing.
