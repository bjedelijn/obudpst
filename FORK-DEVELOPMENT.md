# Fork development

This is a development fork of [BroadbandForum/obudpst](https://github.com/BroadbandForum/obudpst).
The original upstream README and license information must remain intact.

## Branch strategy

- `main`: upstream-aligned baseline; do not force-push release modifications here
- `qon/ci-tests`: integration work in progress, currently containing initial CI and an experimental fixed server data-port allocator
- `qon/modern`: planned modern release line, to be created from a verified modern upstream base
- `qon/legacy`: planned legacy release line, to be created from the **verified v7.4.0 source/tag**, not from modern `main`

The modern and legacy lines must be tested and released independently. A
branch name alone does not establish its upstream version.

## Experimental server data port range

```bash
UDPST_DATA_PORT_RANGE=40000-40499 udpst -s
```

The range is optional; when unset, upstream ephemeral-port behavior remains.
It applies to server-created test data sockets, not the control listener.
Avoid overlapping control and data ports. The allocator and parameter
validation are **not yet validated in a real running service**.

## Required tests

- GCC and Clang build; parameter acceptance/rejection
- IPv4/IPv6 bind, address advertisement and client connectivity
- occupied-port retry, range exhaustion, concurrent sessions and cleanup
- interoperability with modern clients and separately with legacy FRITZ!Box clients
- firewall and restart/rollback validation on an isolated test server

## Compatibility

Do not assume modern and legacy protocol versions interoperate. The legacy
release line must be built from its actual historical upstream source.
