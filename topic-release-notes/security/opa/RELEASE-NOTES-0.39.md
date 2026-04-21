# opa v0.39 Release Notes

Source: [v0.39.0](https://github.com/open-policy-agent/opa/releases/tag/v0.39.0)

This release contains a number of fixes and enhancements.

### Disk Storage

The on-disk storage backend has been fully integrated with the OPA server, and can now be enabled via configuration:

```yaml
storage:
  disk:
    directory: /var/opa # put data here
    auto_create: true   # create directory if it doesn't exist
    partitions:         # partitioning is important for data storage,
    - /users/*          # please see the documentation
```

It is intended to enable the use of OPA in scenarios where the data needed for policy evaluation exceeds the available memory.

The on-disk contents will persist among restarts, but should not be used as a single source of truth: there are no backup mechanisms, and certain data partitioning changes will require a start-over. These are things that may get improved in the future.

For all the details, please refer to the [configuration](https://www.openpolicyagent.org/docs/v0.39.0/configuration/#disk-storage) and [detailled Disk Storage section](https://www.openpolicyagent.org/docs/v0.39.0/misc-disk/) of the documentations.

### Tooling, SDK, and Runtime

- Server: Add warning when `input` attribute is missing in `POST /v1/data` API ([#4386](https://github.com/open-policy-agent/opa/issues/4386)) authored by @aflmp
- SDK: Support partial evaluation ([#4240](https://github.com/open-policy-agent/opa/pull/4240)), authored by @kroekle; with a fix to avoid using different state (authored by @Iceber)
- Runtime: Suppress payloads in debug logs for handlers that compress responses (`/metrics` and `/debug/pprof`) (authored by @christian1607)
- `opa test`: Add file path to failing tests to make debugging failing tests easier ([#4457](https://github.com/open-policy-agent/opa/issues/4457)), authored by @liamg
- `opa fmt`: avoid whitespace mixed with tabs on `with` statements ([#4376](https://github.com/open-policy-agent/opa/issues/4376)) reported by @tiwood
- Coverage reporting: Remove duplicates from coverage report ([#4393](https://github.com/open-policy-agent/opa/issues/4393)) reported by @gianna7wu
- Plugins: Fix broken retry logic in decision logs plugin ([#4486](https://github.com/open-policy-agent/opa/issues/4486)) reported by @iamatwork
- Plugins: Update regular polling fallback mechanism for downloader
- Plugins: Support for adding custom parameters and headers for OAuth2 Client Credentials Token request (authored by @srlk)
- Plugins: Log message on unexpected bundle content type ([#4278](https://github.com/open-policy-agent/opa/issues/4278))
- Plugins: Mask Authorization header value in debug logs ([#4495](https://github.com/open-policy-agent/opa/issues/4495))
- Docker images: Use GID 1000 in `-rootless` images ([#4380](https://github.com/open-policy-agent/opa/issues/4380)); also warn when using UID/GID 0.
- Runtime: change processed file event log level to info

### Rego and Topdown

- Type checker: Skip pattern JSON Schema attribute compilation ([#4426](https://github.com/open-policy-agent/opa/issues/4426)): These are not supported, but could have caused the parsing of a JSON Schema document to fail.
- Topdown: Copy without modifying expr, fixing a bug that could occur when running multiple partial evaluation requests concurrently.
- Compiler strict mode: Raise error on unused imports ([#4354](https://github.com/open-policy-agent/opa/issues/4354)) authored by @damienjburks
- AST: Fix print call rewriting in else rules ([#4489](https://github.com/open-policy-agent/opa/issues/4489))
- Compiler: Improve error message on missing `with` target ([#4431](https://github.com/open-policy-agent/opa/issues/4431)) reported by @gabrielfern
- Parser: hint about 'every' future keyword import

### Documentation and Website

- AWS CloudFormation Hook: New tutorial
- Community: Stretch background so it covers on larger screens ([#4402](https://github.com/open-policy-agent/opa/issues/4402)) authored by @msorens
- Build: Make local dev and PR preview not build everything ([#4379](https://github.com/open-policy-agent/opa/issues/4379))
- Philosophy: Grammar fixes (authored by @ajonesiii)
- README: Add note about Hugo version mismatch errors (authored by @ogazitt)
- Integrations: Add GraphQL-Graphene (authored by @dolevf), Emissary-Ingress (authored by @tayyabjamadar), rekor-sidekick,
- Integrations CI: ensure referenced software is listed, and logo file names match; allow SVG logos
- Envoy: Update policy primer with new control headers
- Envoy: Update bob_token and alice_token in tutorial (authored by @rokkiter)
- Envoy: Include new configurable gRPC msg sizes (authored by @emaincourt)
- Annotations: add missing title to index (authored by @itaysk)

### Miscellaneous

- Various dependency bumps, notably:
  - OpenTelemetry-go: 1.4.1 -> 1.6.1
  - Wasmtime-go: 0.34.0 -> 0.35.0
- Binaries and Docker images are now built using Go 1.18; CI runs build/test for Ubuntu and macos with Go 1.16 and 1.17.
- CI: remove go-fuzz, use native go 1.18 fuzzer