# opa v0.68 Release Notes

Source: [v0.68.0](https://github.com/open-policy-agent/opa/releases/tag/v0.68.0)

This release contains a mix of features and bugfixes.

### Breaking Changes

#### `entrypoint` annotation implies `document` scope ([#6798](https://github.com/open-policy-agent/opa/issues/6798))

The [entrypoint annotation's](https://www.openpolicyagent.org/docs/latest/policy-language/#entrypoint) scope requirement has changed from `rule` to `document` ([https://github.com/open-policy-agent/opa/issues/6798](#6798)). Furthermore, if no `scope` annotation is declared for a METADATA block preceding a rule, the presence of an `entrypoint` annotation with a `true` value will assign the block a `document` scope, where the `rule` scope is otherwise the default.

In practice, a rule entrypoint always point to the entire document and not a particular rule definition. The previous behavior was a bug, and one we've now addressed.

Authored by @anderseknert

### Topdown and Rego

- ast: Fixing nil-pointer dereference in compiler for partial rule edge case ([#6930](https://github.com/open-policy-agent/opa/issues/6930)) authored by @johanfylling
- ast+parser: Add hint to future-proof imports ([6968](https://github.com/open-policy-agent/opa/pull/6968)) authored by @srenatus
- topdown: Adding unification scope to virtual-cache key. Fixing issue where false positive cache hits can occur when unification "restricts" the scope of ref-head rule evaluation ([#6926](https://github.com/open-policy-agent/opa/issues/6926)) authored by @johanfylling reported by @anderseknert
- topdown: Marshal JWT encode sign inputs as JSON ([#6934](https://github.com/open-policy-agent/opa/pull/6934)) authored by @charlieegan3

### Runtime, Tooling, SDK

- ast: Make type checker `copy` method copy all values ([#6949](https://github.com/open-policy-agent/opa/pull/6949)) authored by @anderseknert
- ast: Include term locations in rule heads when requested ([#6860](https://github.com/open-policy-agent/opa/issues/6860)) authored by @anderseknert
- debug: Adding experimental debugger SDK ([#6876](https://github.com/open-policy-agent/opa/issues/6876)) authored by @johanfylling
- distributedtracing: allow OpenTelemetry resource attributes to be configured under distributed_tracing config ([#6942](https://github.com/open-policy-agent/opa/issues/6942)) authored and reported by @brettmc
- download: Fixing issue when saving OCI bundles on disk ([#6939](https://github.com/open-policy-agent/opa/issues/6939)) authored and reported by @Sergey-Kizimov
- logging: Always include HTTP request context in incoming req context ([#6951](https://github.com/open-policy-agent/opa/issues/6951)) authored by @ashutosh-narkar reported by @alvarogomez93
- plugins/bundle: Avoid race-condition during bundle reconfiguration and activation ([#6849](https://github.com/open-policy-agent/opa/issues/6849)) authored by @ashutosh-narkar reported by @Pushpalanka
- plugins/bundle: Escape reserved chars used in persisted bundle directory name ([#6915](https://github.com/open-policy-agent/opa/issues/6915)) authored by @ashutosh-narkar reported by @alvarogomez93
- plugins/rest: Support AWS_CONTAINER_CREDENTIALS_FULL_URI metadata endpoint ([#6893](https://github.com/open-policy-agent/opa/issues/6893)) authored and reported by @mbamber
- util+server: Fix bug around chunked request handling. ([#6904](https://github.com/open-policy-agent/opa/issues/6904)) authored by @philipaconrad reported by @David-Wobrock
- `opa exec`: This command never supported "pretty" formatting (`--format=pretty` or `-f pretty`), only `json`. Passing `pretty` is now invalid. ([#6923](https://github.com/open-policy-agent/opa/pull/6923)) authored by @srenatus
  Note that the flag is now unnecessary, but it's kept so existing calls like `opa exec -fjson ...` remain valid.

#### Security Fix: CVE-2024-8260 ([#6933](https://github.com/open-policy-agent/opa/pull/6933))

This release includes a fix where OPA would accept UNC locations on Windows. Reading those could leak NTLM hashes.
The attack vector would include an adversary tricking the user in passing an UNC path to OPA, e.g. `opa eval -d $FILE`.
UNC paths are now forbidden. If this is an issue for you, please reach out on Slack or GitHub issues.

Reported by Shelly Raban
Authored by @ashutosh-narkar

### Docs, Website, Ecosystem

- docs: Suggest using `opa-config.yaml` as name for config file (#6966) ([#6959](https://github.com/open-policy-agent/opa/issues/6959)) authored by @anderseknert
- docs: Add documentation for OPA Spring Boot integration ([#6898](https://github.com/open-policy-agent/opa/pull/6898)) authored by @charlieegan3
- docs: Update Istio tutorial ([#6896](https://github.com/open-policy-agent/opa/pull/6896)) authored by @Pindar
- docs: Update contrib docs ([#6974](https://github.com/open-policy-agent/opa/pull/6974)) authored by @charlieegan3
- docs: Add Lula to the OPA ecosystem ([#6902](https://github.com/open-policy-agent/opa/pull/6902)) authored by @brandtkeller
- docs: Add github action policy testing automation ([#6954](https://github.com/open-policy-agent/opa/pull/6954)) authored by @oycyc
- docs: Mention `http.send` in inter-query cache config docs ([#6953](https://github.com/open-policy-agent/opa/pull/6953)) authored by @anderseknert
- docs+topdown: Fixing typos in built-in descriptions ([#6940](https://github.com/open-policy-agent/opa/pull/6940)) authored by @msorens

### Miscellaneous

- build: Make it possible to build only wasm testcases ([#6920](https://github.com/open-policy-agent/opa/pull/6920)) authored by @andreaTP
- Dependency updates; notably:
  - build(deps): bump github.com/containerd/containerd from 1.7.20 to 1.7.21
  - build(deps): bump github.com/prometheus/client_golang from 1.19.1 to 1.20.2
  - build(deps): bump golang.org/x/net from 0.27.0 to 0.28.0
  - build(deps): bump golang.org/x/time from 0.5.0 to 0.6.0
  - build(deps): bump google.golang.org/grpc from 1.65.0 to 1.66.0

