---
title: opa v0.35 Release Notes
description: opa v0.35 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.35 Release Notes 是什么
- 如何 opa v0.35 Release Notes
trigger_keywords:
- opa
- v0.35
- Release
- Notes
- release
- notes
---

# opa v0.35 Release Notes

Source: [v0.35.0](https://github.com/open-policy-agent/opa/releases/tag/v0.35.0)

This release contains a number of fixes and enhancements.

### Early Exit Optimization

This release adds an early exit optimization to the evaluator. With this optimization, the evaluator stops evaluating rules when an answer has been found and subsequent evaluation would not yield any new answers. The optimization is automatically applied to complete rules and functions that meet specific requirements. For more information see the [Early Exit in Rule Evaluation](https://www.openpolicyagent.org/docs/latest/policy-performance/#early-exit-in-rule-evaluation) section in the docs. [#2092](https://github.com/open-policy-agent/opa/issues/2092)

### Built-in Functions

- The `net.lookup_ip_addr` function was added to allow policies to resolve hostnames to IPv4/IPv6 addresses ([#3993](https://github.com/open-policy-agent/opa/issues/3993))
- The `http.send` function has been improved to close TCP connections quickly after receiving the HTTP response and avoid creating HTTP clients unnecessarily when a cached response exists ([#4015](https://github.com/open-policy-agent/opa/issues/4015)). This change reduces the number of open file descriptors required in high-throughput environments and prevents OPA from encountering ulimit errors.

### Rego

- `print()` calls in the head of rules no longer cause runtime errors ([#3967](https://github.com/open-policy-agent/opa/issues/3967))
- Type errors for calls to undefined functions no longer contain rewritten variable names ([#4031](https://github.com/open-policy-agent/opa/issues/4031))
- The `rego.SkipPartialNamespace` option now correctly sets the flag on the partial evaluation queries (previously it would always set the value to `true`) ([#3996](https://github.com/open-policy-agent/opa/issues/3996)) authored by @[thomascoquet](https://github.com/thomascoquet)
- The internal set implementation has been updated to insert elements in sorted order rather than lazily sorting during comparisons.
- Fixed `import` alias parsing bug identified by fuzzer ([#3988](https://github.com/open-policy-agent/opa/issues/3988))

### WebAssembly

- The Golang SDK will now issue a `grow()` call if the `input` document exceeds the available memory space.
- The `malloc()` implementation will now call `opa_abort` if the `grow()` call fails.

### Server

- The decision logger adapts upload chunk sizes based on previous outputs. This allows the decision loggger to encode significantly more decisions into each upload chunk, thereby reducing heap usage for buffered decisions. For more information on the adapative chunking behaviour, see the [Decision Logs](https://www.openpolicyagent.org/docs/latest/management-decision-logs/) page in the docs.
- The decision logger can be configured to send records to a custom plugin as well as an HTTP endpoint at the same time ([#4013](https://github.com/open-policy-agent/opa/issues/4013))
- `print()` calls from the `system.authz` policy are now included in the logs ([#4048](https://github.com/open-policy-agent/opa/issues/4048))
- OPA can use an [Azure Managed Identities Token](https://www.openpolicyagent.org/docs/latest/configuration/#azure-managed-identities-token) to authenticate with control plane services ([#3916](https://github.com/open-policy-agent/opa/issues/3916)) authored by @[Scowluga](https://github.com/Scowluga).
- The logging configuration will be correctly applied to service clients so that DEBUG logs are surfaced ([#4071](https://github.com/open-policy-agent/opa/issues/4071))

### Tooling

- The `opa fmt` command will not generate a line-break when there are generated variables in a function call ([#4018](https://github.com/open-policy-agent/opa/issues/4018)) reported by @[torsrex](https://github.com/torsrex)
- The `opa inspect` command no longer prints a blank namespace when a data.json file is included at the root ([#4022](https://github.com/open-policy-agent/opa/issues/4022))
- The `opa build` command will output debug messages if an optimized entrypoint is discarded.

### Website and Documentation

- The website has been updated to build with Hugo 0.88.1 ([#3787](https://github.com/open-policy-agent/opa/issues/3787))
- The version picker in the documentation is now scrollable ([#3955](https://github.com/open-policy-agent/opa/issues/3955)) authored by @[orweis](https://github.com/orweis)
- The description of the `urlquery` built-in functions have been clarified ([#1592](https://github.com/open-policy-agent/opa/issues/1592)) reported by @[klarose](https://github.com/klarose)
- The decision logger documentation has been improved to cover controls for large-scale environments ([#3976](https://github.com/open-policy-agent/opa/issues/3976))
- The "strict built-in errors" mode is now covered in the docs along with built-in function error behaviour ([#3686](https://github.com/open-policy-agent/opa/issues/3686))
- The OAuth2 and OIDC examples around key rotation and caching have been improved

### CI

- Issues and PRs that have not seen activity in 30 days will be automatically marked as "inactive"
- The `Makefile` can now produce Docker images for other architectures. We do not yet publish binaries or images for non-amd64 architectures however if you want to build OPA yourself, the `Makefile` does not prohibit it.

### Backwards Compatibility

- The diagnostics buffer in the OPA server has been completely removed as part of the deprecation and removal of the diagnostic feature ([#1052](https://github.com/open-policy-agent/opa/issues/1052))