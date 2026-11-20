# Release policy (fork)

No release tags have been created by this policy yet.

## Channels

| Channel | Example modern tag | Purpose |
| --- | --- | --- |
| Beta | `modern-v9.1.0-beta.1` | Experimental testing |
| RC | `modern-v9.1.0-rc.1` | Release candidate after feature freeze |
| Stable | `modern-v9.1.0` | Approved release |

For the separate historical line, use `legacy-v7.4.0-beta.1`,
`legacy-v7.4.0-rc.1`, and `legacy-v7.4.0`.
These are **fork tags**, not claims of official upstream releases.

## Gates

1. Confirm branch ancestry and upstream source version.
2. Run builds and static checks for the target branch.
3. Run real socket tests: IPv4, IPv6, concurrency, collisions, exhaustion.
4. Validate setup-response data ports and client interoperability.
5. Validate staging firewall and rollback behavior.
6. Document configuration, migration, known limitations and release notes.
7. Require explicit approval before creating an annotated tag or publishing a GitHub Release.

Stable releases must be built from an immutable commit with recorded CI
results. Never retag an existing release; increment the prerelease number
or patch version instead.
