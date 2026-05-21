---
title: opa v1.2 Release Notes
description: opa v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- containerd
- opa
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- opa v1.2 Release Notes 是什么
- 如何 opa v1.2 Release Notes
trigger_keywords:
- opa
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- policy-basics
- observability-basics
---

# opa v1.2 Release Notes

Source: [v1.2.0](https://github.com/open-policy-agent/opa/releases/tag/v1.2.0)

This release contains a mix of features, performance improvements, and bugfixes.

### Parameterized Rego Tests ([#2176](https://github.com/open-policy-agent/opa/issues/2176))

Rego tests now support parameterization, allowing a single test rule to include multiple, hierarchical, named test cases.
This feature is useful for data-driven testing, where a single test rule can be used for multiple test cases with different inputs and expected outputs.

```rego
package example_test

test_concat[note] if {
	some note, tc in {
		"empty + empty": {
			"a": [],
			"b": [],
			"exp": [],
		},
		"empty + filled": {
			"a": [],
			"b": [1, 2],
			"exp": [1, 2],
		},
		"filled + filled": {
			"a": [1, 2],
			"b": [3, 4],
			"exp": [1, 2, 3], # Faulty expectation, this test case will fail
		},
	}

	act := array.concat(tc.a, tc.b)
	act == tc.exp
}
```

```cmd
$ opa test example_test.rego
example_test.rego:
data.example_test.test_concat: FAIL (263.375µs)
  empty + empty: PASS
  empty + filled: PASS
  filled + filled: FAIL
--------------------------------------------------------------------------------
FAIL: 1/1
```

See the [documentation](https://www.openpolicyagent.org/docs/latest/policy-testing/#parameterized-tests-and-data-driven-testing) for more information.

Authored by @johanfylling, reported by @anderseknert

### Performance Improvements

- perf: Add ref.CopyNonGround ([#7350](https://github.com/open-policy-agent/opa/pull/7350)) authored by @anderseknert
- perf: `opa fmt` 3x faster formatting ([#7341](https://github.com/open-policy-agent/opa/pull/7341)) authored by @anderseknert
- perf: Cost of indexing greatly reduced ([#7370](https://github.com/open-policy-agent/opa/pull/7370)) authored by @anderseknert
- perf: Eval optimizations ([#7367](https://github.com/open-policy-agent/opa/pull/7367)) authored by @anderseknert
- perf: Intern annotation terms ([#7365](https://github.com/open-policy-agent/opa/pull/7365)) authored by @anderseknert
- perf: Slightly more efficient policy scanning ([#7368](https://github.com/open-policy-agent/opa/pull/7368)) authored by @anderseknert
- perf: Switch to a faster xxhash package ([7362](https://github.com/open-policy-agent/opa/pull/7362)) authored by @Juneezee
- perf: Use GetByValue to avoid boxing to interface{} ([#7372](https://github.com/open-policy-agent/opa/pull/7372)) authored by @anderseknert
- perf: Various small improvements ([#7357](https://github.com/open-policy-agent/opa/pull/7357)) authored by @anderseknert
- perf: Improve storage lookup performance ([#7336](https://github.com/open-policy-agent/opa/pull/7336)) authored by @anderseknert
- perf: optimize iteration ([#7327](https://github.com/open-policy-agent/opa/pull/7327)) authored by @anderseknert

### Topdown and Rego

- rego+topdown: Allow providing custom base cache ([#7329](https://github.com/open-policy-agent/opa/pull/7329)) authored by @anderseknert

### Runtime, Tooling, SDK

- ast: Add missing `BuildAnnotationSet` to `ast` v0 ([#7347](https://github.com/open-policy-agent/opa/issues/7347)) authored by @anderseknert
- ast: Eliminate allocation in Value.Find, and other improvements ([#7319](https://github.com/open-policy-agent/opa/pull/7319)) authored by @anderseknert
- ast: Use byte for RuleKind and DocKind ([#7332](https://github.com/open-policy-agent/opa/pull/7332)) authored by @anderseknert
- ast.InterfaceToValue: add test case for `[]byte` ([#7379](https://github.com/open-policy-agent/opa/pull/7379)) authored by @dennygursky
- ast: support []string and ast.Value in ast.InterfaceToValue ([#7306](https://github.com/open-policy-agent/opa/pull/7306)) authored by @regeda
- bundle: Fixing issue where `--v0-compatible` isn't respected for custom bundles ([#7338](https://github.com/open-policy-agent/opa/pull/7338)) authored by @johanfylling
- cmd: Handle failing tests in `opa test --bench` ([#7205](https://github.com/open-policy-agent/opa/issues/7205)) authored by @anderseknert
- cmd: Add decision ID to `opa exec` output ([#7373](https://github.com/open-policy-agent/opa/pull/7373)) authored by @anderseknert
- oracle: Make oracle public under v1/ast/oracle ([#7265](https://github.com/open-policy-agent/opa/issues/7265)) authored by @anderseknert
- oracle: Allow passing own compiler to oracle ([#7354](https://github.com/open-policy-agent/opa/pull/7354)) authored by @anderseknert
- plugins/discovery: Enable tracing for discovery plugin ([#7299](https://github.com/open-policy-agent/opa/pull/7299)) authored by @mjungsbluth
- plugins/rest: Do not attach authorization header in bearerAuthPlugin if response is a redirect ([#7308](https://github.com/open-policy-agent/opa/pull/7308)) authored by @carabasdaniel
- server+distributedtracing: Add Additional Resource Attributes for OpenTelemetry ([#7322](https://github.com/open-policy-agent/opa/issues/7322)) authored by @briankahoot reported by @briankahoot
- util: Add util.HasherMap ([#7363](https://github.com/open-policy-agent/opa/pull/7363)) authored by @anderseknert

### Docs, Website, Ecosystem

- docs: Add support link to README ([#7359](https://github.com/open-policy-agent/opa/pull/7359)) (authored by @anderseknert)
- docs: Update example bundle to be v1 compatible ([#7342](https://github.com/open-policy-agent/opa/pull/7342)) authored by @ashutosh-narkar
- docs: Add note about v1.0 addr behaviour ([#7360](https://github.com/open-policy-agent/opa/issues/7360)) authored by @charlieegan3 reported by @ali-jalaal
- docs: Update homepage examples to drop `v1 import` ([#7391](https://github.com/open-policy-agent/opa/pull/7391)) authored by @charlieegan3
- docs: Updating `--v1-compatible` mentions outside the v1 upgrade guide and v0 compatibility docs ([#7337](https://github.com/open-policy-agent/opa/pull/7337)) authored by @johanfylling
- docs: Fixed invalid links to examples ([#7326](https://github.com/open-policy-agent/opa/pull/7326)) authored by @JonathanDeLaCruzEncora
- MAINTAINERS: Add Anders and Charlie as maintainers ([#7318](https://github.com/open-policy-agent/opa/pull/7318)) authored by @charlieegan3

### Miscellaneous

- build+test: Add `make test-short` task (#7364) (authored by @anderseknert)
- build: Add gocritic linter ([#7377](https://github.com/open-policy-agent/opa/pull/7377)) authored by @anderseknert
- build: Add nilness linter from govet ([#7335](https://github.com/open-policy-agent/opa/pull/7335)) authored by @anderseknert
- build: Add perfsprint linter ([#7334](https://github.com/open-policy-agent/opa/pull/7334)) authored by @anderseknert
- ci: Tagging release binaries with build version ([#7395](https://github.com/open-policy-agent/opa/pull/7395), [#7397](https://github.com/open-policy-agent/opa/pull/7397), [#7400](https://github.com/open-policy-agent/opa/pull/7400)) authored by @johanfylling
- test: fix race in `TestIntraQueryCache_ClientError` and `TestInterQueryCache_ClientError` ([#7280](https://github.com/open-policy-agent/opa/pull/7280)) authored by @Juneezee
- misc: Use Go 1.22+ int ranges ([#7328](https://github.com/open-policy-agent/opa/pull/7328)) authored by @anderseknert
- Dependency updates; notably:
  - build: bump go from 1.23.5 to 1.24.0
  - build(deps): bump github.com/agnivade/levenshtein from 1.2.0 to 1.2.1
  - build(deps): bump github.com/containerd/containerd from 1.7.25 to 1.7.26
  - build(deps): bump github.com/google/go-cmp from 0.6.0 to 0.7.0
  - build(deps): bump github.com/prometheus/client_golang
  - build(deps): bump github.com/spf13/cobra from 1.8.1 to 1.9.1
  - build(deps): bump github.com/spf13/pflag from 1.0.5 to 1.0.6
  - build(deps): bump golang.org/x/net from 0.34.0 to 0.35.0
  - build(deps): bump golang.org/x/time from 0.9.0 to 0.10.0
  - build(deps): bump ossf/scorecard-action from 2.4.0 to 2.4.1
  - Bump golangci-lint from v1.60.1 to 1.64.5

