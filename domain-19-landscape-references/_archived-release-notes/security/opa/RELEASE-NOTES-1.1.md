---
title: opa v1.1 Release Notes
description: opa v1.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- containerd
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
- opa v1.1 Release Notes 是什么
- 如何 opa v1.1 Release Notes
trigger_keywords:
- opa
- v1.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
created: "2026-05-23"
---

# opa v1.1 Release Notes

Source: [v1.1.0](https://github.com/open-policy-agent/opa/releases/tag/v1.1.0)

This release contains a mix of features, performance improvements, and bugfixes.

### Performance Improvements

- ast: Remove jsonOptions from AST nodes and terms ([#7281](https://github.com/open-policy-agent/opa/pull/7281)) authored by @anderseknert
- ast+plugins: Optimize activation of bundles with no inter-bundle path overlap ([#7144](https://github.com/open-policy-agent/opa/issues/7144)) authored and reported by @sqyang94
- bundle: Optimizing rego-version management in bundle activation ([#7296](https://github.com/open-policy-agent/opa/pull/7296)) authored by @johanfylling
- cmd: Don't generate JSON from result in `opa bench` ([#7291](https://github.com/open-policy-agent/opa/issues/7291)) authored by @anderseknert
- topdown: Adding configurable token cache to `io.jwt` token verification built-ins ([#7274](https://github.com/open-policy-agent/opa/pull/7274)) authored by @johanfylling
- topdown: Reduce allocations in hot path ([#7288](https://github.com/open-policy-agent/opa/pull/7288)) authored by @anderseknert
- perf: Improvements to terms and built-in functions ([#7284](https://github.com/open-policy-agent/opa/pull/7284)) authored by @anderseknert
- perf: add Regorus ACI benchmark tests ([#7298](https://github.com/open-policy-agent/opa/pull/7298)) authored by @anderseknert
- plugins: Don't use reflect.DeepEqual for errors ([#7238](https://github.com/open-policy-agent/opa/issues/7238)) authored by @anderseknert
- testing: replace reflect.DeepEqual where possible ([#7286](https://github.com/open-policy-agent/opa/pull/7286)) authored by @anderseknert

### Topdown and Rego

- topdown: Fix out of range error in `numbers.range` built-in ([#7269](https://github.com/open-policy-agent/opa/issues/7269)) authored by @anderseknert
- topdown+rego+server: Allow opt-in for evaluating non-det builtins in PE ([#6496](https://github.com/open-policy-agent/opa/issues/6496)) authored by @srenatus

### Runtime, Tooling, SDK

- bundle: Add info about the correct rego version to parse modules on the store ([#7278](https://github.com/open-policy-agent/opa/pull/7278)) co-authored by @ashutosh-narkar and @johanfylling
- bundle+plugins: Fixing issue where bundle plugin could panic on reconfiguration (SDK use) ([#7297](https://github.com/open-policy-agent/opa/issues/7297)) authored by @johanfylling reported by @carabasdaniel
- cmd: Fix printed representation of ref head rules in `opa repl` ([#7301](https://github.com/open-policy-agent/opa/issues/7301)) authored by @anderseknert reported by @tsandall
- cmd: Respect `--v0-compatible` for `opa eval` partial eval support modules ([#7251](https://github.com/open-policy-agent/opa/pull/7251)) authored by @johanfylling
- golangci: fix invalid `linter-settings` configuration name ([#7244](https://github.com/open-policy-agent/opa/pull/7244)) authored by @Juneezee
- plugins/logs: Add support for masking with array keys ([#6883](https://github.com/open-policy-agent/opa/issues/6883)) authored by @charlieegan3
- tester: code nitpicks ([#7252](https://github.com/open-policy-agent/opa/pull/7252)) authored by @srenatus
- util: Add util.Keys and util.KeysSorted ([#7285](https://github.com/open-policy-agent/opa/pull/7285)) authored by @anderseknert

### Docs, Website, Ecosystem

- docs: Update docker compose file in HTTP API tutorial and use addr for binding ([#7264](https://github.com/open-policy-agent/opa/issues/7264)) authored and reported by @zanliffick
- docs: Make 'ancient' warnings closable ([#7253](https://github.com/open-policy-agent/opa/issues/7253)) authored by @srenatus reported by @konradzagozda
- docs: Redirect opa-1 to v0-upgrade ([#7259](https://github.com/open-policy-agent/opa/pull/7259)) authored by @charlieegan3
- docs: Use preformatted strings in fmt help ([#7263](https://github.com/open-policy-agent/opa/pull/7263)) authored by @charlieegan3
- docs: Fix typo in k8s primer ([#7242](https://github.com/open-policy-agent/opa/pull/7242)) authored by @vicentinileonardo
- docs: Formatting and wording fixes ([#7268](https://github.com/open-policy-agent/opa/pull/7268)) authored by @kamilturek
- docs: Update output document of Envoy plugin. ([#7241](https://github.com/open-policy-agent/opa/pull/7241)) authored by @regeda

### Miscellaneous

- ci(nightly): Remove vendor w/o modproxy check ([#7292](https://github.com/open-policy-agent/opa/pull/7292)) authored by @srenatus
- Dependency updates; notably:
  - build(go): bump to 1.23.5 ([7279](https://github.com/open-policy-agent/opa/pull/7279)) authored by @srenatus
  - build(deps): upgrade github.com/dgraph-io/badger to v4 (4.5.1) ([#7239](https://github.com/open-policy-agent/opa/pull/7239)) authored by @Juneezee
  - build(deps): bump github.com/containerd/containerd from 1.7.24 to 1.7.25
  - build(deps): bump github.com/tchap/go-patricia/v2 from 2.3.1 to 2.3.2
  - build(deps): bump golang.org/x/net from 0.33.0 to 0.34.0
  - build(deps): bump golang.org/x/time from 0.8.0 to 0.9.0
  - build(deps): bump google.golang.org/grpc from 1.69.2 to 1.70.0
  - build(deps): bump go.opentelemetry.io deps to 1.34.0/0.59.0 

