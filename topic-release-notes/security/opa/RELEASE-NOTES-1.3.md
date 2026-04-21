# opa v1.3 Release Notes

Source: [v1.3.0](https://github.com/open-policy-agent/opa/releases/tag/v1.3.0)

This release contains a mix of features, bugfixes, and dependency updates.

### New Buffer Option for Decision Logs ([#5724](https://github.com/open-policy-agent/opa/issues/5724))

A new, optional, buffering mechanism has been added to decision logging. 
The default buffer is designed around making precise memory footprint guarantees, which can produce lock contention at high loads, negatively impacting query performance.
The new event-based buffer is designed to reduce lock contention and improve performance at high loads, but sacrifices the memory footprint guarantees of the default buffer.

The new event-based buffer is enabled by setting the `decision_logs.reporting.buffer_type` [configuration option](https://www.openpolicyagent.org/docs/latest/configuration/#decision-logs) to `event`.

For more details, see the decision log plugin [README](https://github.com/open-policy-agent/opa/blob/main/v1/plugins/logs/README.md).

Reported by @mjungsbluth, authored by @sspaink

### OpenTelemetry: HTTP Support and Expanded Batch Span Configuration ([#7412](https://github.com/open-policy-agent/opa/issues/7412))

Distributed tracing through OpenTelemetry has been extended to support HTTP collectors (enabled by setting the `distributed_tracing.type` configuration option to `http`).
Additionally, configuration has been expanded with fine-grained batch span processor [options](https://www.openpolicyagent.org/docs/latest/configuration/#distributed-tracing).

Authored and reported by @sqyang94

### Runtime, Tooling, SDK

- compile: Require multi-term entrypoint paths for optimized bundle building ([#7321](https://github.com/open-policy-agent/opa/issues/7321)) authored by @johanfylling reported by @nikpivkin
- fmt: Allow one liner rule grouping ([#6760](https://github.com/open-policy-agent/opa/issues/6760)) authored by @anderseknert
- fmt: Fix v0-compatible fmt with stdin ([#7409](https://github.com/open-policy-agent/opa/issues/7409)) authored and reported by @charlieegan3
- ir: Fix nil pointer deref in Unmarshal() when handling IsSetStmt ([#7415](https://github.com/open-policy-agent/opa/issues/7415)) authored and reported by @KrisKennawayDD
- planner: Fix Wasm vs non-Wasm evaluation difference bug related to the overeager optimization of ref head rules ([#7439](https://github.com/open-policy-agent/opa/pull/7439)) authored by @srenatus
- sdk: Removing repeat args from sub-func call ([#7443](https://github.com/open-policy-agent/opa/pull/7443)) authored by @alingse
- tester: Including parameterized test cases in test report counter ([#7407](https://github.com/open-policy-agent/opa/issues/7407)) authored by @johanfylling
- tester: Only including failed sub-test cases in report summary when non-verbose ([#7426](https://github.com/open-policy-agent/opa/pull/7426)) authored by @johanfylling

### Docs, Website, Ecosystem

- docs: Add some notes about AI assisted patches ([#7436](https://github.com/open-policy-agent/opa/pull/7436)) authored by @charlieegan3
- docs: Add query_parameters_to_set ([#7405](https://github.com/open-policy-agent/opa/pull/7405)) authored by @sedovmik
- docs: Delete reference to license key in Envoy tutorial ([#7466](https://github.com/open-policy-agent/opa/pull/7466)) authored by @joostholslag
- docs: Fix typo in Envoy tutorial ([#7464](https://github.com/open-policy-agent/opa/pull/7464)) authored by @joostholslag
- docs: Update slack inviter link ([#7450](https://github.com/open-policy-agent/opa/pull/7450)) authored by @charlieegan3
- docs: Update terraform examples ([#7429](https://github.com/open-policy-agent/opa/pull/7429)) authored by @charlieegan3
- docs: Simplify `kind` usage instruction in Envoy tutorial ([#7465](https://github.com/open-policy-agent/opa/pull/7465)) authored by @joostholslag

### Miscellaneous

- Enable unused-receiver linter (revive) ([#7448](https://github.com/open-policy-agent/opa/pull/7448)) authored by @anderseknert
- Dependency updates; notably:
  - build(deps): bump github.com/containerd/containerd from 1.7.26 to 1.7.27
  - build(deps): bump github.com/dgraph-io/badger/v4 from 4.5.1 to 4.6.0
  - build(deps): bump github.com/opencontainers/image-spec from 1.1.0 to 1.1.1
  - build(deps): bump github.com/prometheus/client_golang 1.21.0 to 1.21.1
  - build(deps): bump golang.org/x/net from 0.35.0 to 0.37.0
  - build(deps): bump golang.org/x/time from 0.10.0 to 0.11.0
  - build(deps): bump google.golang.org/grpc from 1.70.0 to 1.71.0
  - build(deps): bump go.opentelemetry.io deps to 1.35.0/0.60.0

