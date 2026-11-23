# Windows TR-471 CI integration boundary

The Windows UDPST client and its test automation live in this repository.
Firewall admission logic belongs in `bjedelijn/speedtest-firewall-manager`
and must not be copied into this codebase.

## Intended workflow

1. Build the Windows executable on a GitHub-hosted Windows runner.
2. Run offline unit/integration checks before attempting network tests.
3. Obtain a GitHub Actions OIDC token with narrowly scoped claims.
4. Request short-lived runner IPv4 admission from the separately deployed,
   authenticated firewall-manager service.
5. Execute repeated TR-471 tests against the designated test server.
6. Upload raw JSON and stderr logs as GitHub Actions artifacts.
7. Publish a reproducible summary with commit SHA, environment, expected
   rate, measured rate, pacing clamps, send failures, and loop-gap maxima.
8. Fail the job on verified regressions; do not rewrite official TR-471 JSON.

## Security and rollout gates

- Do NOT expose an unauthenticated admission endpoint.
- The firewall service must verify the OIDC token issuer, audience, repository
  identity, workflow identity, ref and expiry before admitting the independently
  observed caller IPv4.
- Limit permission to the dedicated `qon_speed_github_ci_v4` ipset.
- Require a short TTL, rate limiting, request logging and server-side expiry.
- Do not add firewall credentials, root SSH keys, or broad firewall privileges
  to GitHub secrets.
- Do not enable remote tests until the admission service is deployed and
  tested, and the server confirms its iptables/ipset persistence strategy.
- GitHub-hosted runner throughput is a diagnostic signal, not a controlled
  line-rate benchmark. Keep a dedicated reference environment for release gates.
- Never automatically promote an experimental pacing change to stable.
