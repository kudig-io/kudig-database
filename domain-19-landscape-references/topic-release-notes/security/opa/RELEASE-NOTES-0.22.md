---
title: opa v0.22 Release Notes
description: opa v0.22 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- wasm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.22 Release Notes 是什么
- 如何 opa v0.22 Release Notes
trigger_keywords:
- opa
- v0.22
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.22 Release Notes

Source: [v0.22.0](https://github.com/open-policy-agent/opa/releases/tag/v0.22.0)

### Bundle Signing

OPA now supports digital signatures for policy bundles. Specifically, a signed bundle is a normal OPA bundle that includes a file named ".signatures.json" that dictates which files should be included in the bundle, what their SHA hashes are, and of course is cryptographically secure. When OPA receives a new bundle, it checks that it has been properly signed using a key that OPA has been configured with out-of-band. Only if that verification succeeds does OPA activate the new bundle; otherwise, OPA continues using its existing bundle and reports an activation failure via the status API and error logging. For more information see https://openpolicyagent.org/docs/latest/management/#signing. Many thanks to @[ashish246](https://github.com/ashish246) who co-designed the feature and provided valuable input to the development process with his proof-of-concept [#1757](https://github.com/open-policy-agent/opa/issues/1757).

### Optimization Levels

`opa build` now supports multiple optimization levels. The first level (`--optimize=1`) enables constant folding (based on partial evaluation) that only inlines values that can be computed entirely at build time. The second level (`--optimize=2`) enables the existing (more aggressive) version of partial evaluation that eagerly inlines as much of the policy as possible. For more information on the optimization levels see the [Optimization Levels](https://www.openpolicyagent.org/docs/latest/policy-performance/#optimization-levels) section in the documentation.

### Built-in Functions

- `numbers.range` ([#2479](https://github.com/open-policy-agent/opa/issues/2479)) was added to support policies that need to generate a range of integers (e.g., a network port range).
- `semver.is_valid` and `semver.compare` ([#2538](https://github.com/open-policy-agent/opa/pull/2538/)) was added to support policies that need to validate semantic version numbers (authored by @[charlieegan3](https://github.com/charlieegan3)).

### WebAssembly

- All [String](https://www.openpolicyagent.org/docs/latest/policy-reference/#strings) built-in functions (except `sprintf`) are now implemented natively inside of Wasm-compiled policies.

### Fixes

- A few small issues in the Go integration and `rego` package examples have been resolved ([#2294](https://github.com/open-policy-agent/opa/issues/2294)) and [#2367](https://github.com/open-policy-agent/opa/issues/2367)) authored by @[gaga5lala](https://github.com/gaga5lala).
- The Kubernetes Admission Controller tutorial as been updated to work with recent versions of Kubernetes ([#2467](https://github.com/open-policy-agent/opa/issues/2467) authored by @[gaga5lala](https://github.com/gaga5lala)).
- A few issues in partial evaluation around negation inlining and partial rules have been resolved (e.g., [#2492](https://github.com/open-policy-agent/opa/issues/2492), [#2491](https://github.com/open-policy-agent/opa/issues/2491)).

### Miscellaneous

- OPA now supports IMDSv2 for the AWS metadata service. This improves the security posture of OPA deployments in AWS ([#2482](https://github.com/open-policy-agent/opa/issues/2482)) authored by @[nhw76](https://github.com/nhw76).
- Several improvements to the project documentation including a policy style discussion, an integration option comparison, and discussion of bootstrapping and fail-open versus fail-closed modes.
- The project's CI/CD infrastructure has been migrated to GitHub Actions. The new CI/CD infrastructure is designed and implemented to be portable and includes a number of quality-of-life improvements.
- End-to-end query latency with decision logging enabled has been improved by 10%-15% in real-world cases.

### Backwards Compatibility

* The `rego.Tracer` and `rego.EvalTracer` API's have been deprecated in favor of
  the newer `rego.QueryTracer` and `rego.EvalQueryTracer` API.
* The `tester.Runner#SetCoverageTracer` API has been deprecated in favor of the
  newer `test.Runner#SetCoverageQueryTracer` API.