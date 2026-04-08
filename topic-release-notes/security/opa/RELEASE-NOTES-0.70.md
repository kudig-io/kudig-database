# opa v0.70 Release Notes

Source: [v0.70.0](https://github.com/open-policy-agent/opa/releases/tag/v0.70.0)

This release contains a mix of features, performance improvements, and bugfixes.

### Optimized read mode for OPA's in-memory store ([#7125](https://github.com/open-policy-agent/opa/pull/7125))

A new optimized read mode has been added to the default in-memory store, where data written to the store is eagerly converted
to AST values (the data format used during evaluation). This removes the time spent converting raw data values to AST
during policy evaluation, thereby improving performance.

The memory footprint of the store will increase, as processed AST values generally take up more space in memory than the
corresponding raw data values, but overall memory usage of OPA might remain more stable over time, as pre-converted data
is shared across evaluations and isn't recomputed for each evaluation, which can cause spikes in memory usage.

This mode can be enabled for `opa run`, `opa eval`, and `opa bench` by setting the `--optimize-store-for-read-speed` flag.

More information about this feature can be found [here](https://www.openpolicyagent.org/docs/v0.70.0/policy-performance/#storage-optimization).

Co-authored by @johanfylling and @ashutosh-narkar.

### Topdown and Rego
- topdown: Use new Inter-Query Value Cache for `json.match_schema` built-in function ([#7011](https://github.com/open-policy-agent/opa/issues/7011)) authored by @anderseknert reported by @lcarva
- ast: Fix location text attribute for multi-value rules with generated body  ([#7128](https://github.com/open-policy-agent/opa/issues/7128)) authored by @anderseknert
- ast: Fix regression in `opa check` where a file that referenced non-provided schemas failed validation ([#7124](https://github.com/open-policy-agent/opa/pull/7124)) authored by @tjons
- test/cases/testdata: Fix bug in test by replacing unification by explicit equality check ([#7093](https://github.com/open-policy-agent/opa/pull/7093)) authored by @matajoh
- ast: Replace use of yaml.v2 library with yaml.v3. The earlier version would parse `yes`/`no` values as boolean. The usage of yaml.v2 in the parser was unintentional and now has been updated to yaml.v3 ([#7090](https://github.com/open-policy-agent/opa/issues/7090)) authored by @anderseknert

### Runtime, Tooling, SDK
- cmd: Make `opa check` respect `--ignore` when `--bundle` flag is set ([#7136](https://github.com/open-policy-agent/opa/issues/7136)) authored by @anderseknert
- server/writer: Properly handle result encoding errors which earlier on failure would emit logs such as `superfluous call to WriteHeader()` while still returning `200` HTTP status code. Now, errors encoding the payload properly lead to `500` HTTP status code, without extra logs. Also use Header().Set() not Header().Add() to avoid duplicate content-type headers  ([#7114](https://github.com/open-policy-agent/opa/pull/7114)) authored by @srenatus
- cmd: Support `file://` format for TLS key material file flags in `opa run` ([#7094](https://github.com/open-policy-agent/opa/pull/7094)) authored by @alexrohozneanu
- plugins/rest/azure: Support managed identity for App Service / Container Apps ([#7085](https://github.com/open-policy-agent/opa/issues/7085)) reported and authored by @apc-kamezaki
- debug: Fix step-over behaviour when exiting partial rules ([#7096](https://github.com/open-policy-agent/opa/pull/7096)) authored by @johanfylling
- util+plugins: Fix potential memory leaks with explicit timer cancellation ([#7089](https://github.com/open-policy-agent/opa/pull/7089)) authored by @philipaconrad

### Docs, Website, Ecosystem
- docs: Fix OCI example with updated flag used by the ORAS CLI  ([#7130](https://github.com/open-policy-agent/opa/pull/7130)) authored by @b3n3d17
- docs: Delete Atom editor from supported editor integrations ([#7111](https://github.com/open-policy-agent/opa/pull/7111)) authored by @KaranbirSingh7
- docs/website: Add Styra OPA ASP.NET Core SDK integration ([#7073](https://github.com/open-policy-agent/opa/pull/7073)) authored by @philipaconrad
- docs/website: Update compatibility information on the rego-cpp integration ([#7078](https://github.com/open-policy-agent/opa/pull/7078)) authored by @matajoh

### Miscellaneous
- Dependency updates; notably:
  - build(deps): bump github.com/containerd/containerd from 1.7.22 to 1.7.23
  - build(deps): bump github.com/prometheus/client_golang from 1.20.4 to 1.20.5
  - build(deps): bump golang.org/x/net from 0.29.0 to 0.30.0
  - build(deps): bump golang.org/x/time from 0.6.0 to 0.7.0
  - build(deps): bump google.golang.org/grpc from 1.67.0 to 1.67.1

