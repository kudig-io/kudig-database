---
title: opa v0.69 Release Notes
description: opa v0.69 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- istio
- containerd
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
- opa v0.69 Release Notes 是什么
- 如何 opa v0.69 Release Notes
trigger_keywords:
- opa
- v0.69
- Release
- Notes
- release
- notes
---

# opa v0.69 Release Notes

Source: [v0.69.0](https://github.com/open-policy-agent/opa/releases/tag/v0.69.0)

This release contains a mix of features, bugfixes and necessary tooling and test changes required to support the upcoming OPA `1.0` release.


### Inter-Query Value Cache ([#6908](https://github.com/open-policy-agent/opa/issues/6908))

OPA now has a new inter-query value cache added to the SDK. It is intended to be used for values that are expensive to compute and can be reused across multiple queries. The cache can be leveraged by built-in functions to store values that otherwise aren't appropriate for the existing inter-query cache; for instance when the entry size isn't an appropriate or primary limiting factor for cache eviction.

The default size of the inter-query value cache is unbounded, but can be configured via the `caching.inter_query_builtin_value_cache.max_num_entries` configuration field. OPA will drop random items from the cache if this limit is exceeded.

The cache is used by the `regex` and `glob` built-in functions, which previously had individual, non-configurable caches with a max entry size of `100` each.

Currently, the cache is only exercised when running OPA in server mode (ie. `opa run -s`). Also this feature is unsupported for WASM.

Authored by @ashutosh-narkar, reported by @amirsalarsafaei

### Topdown and Rego

- Future-proofing tests in the `ast`, `topdown`, `rego` etc. packages to be `1.0` compatible (authored by @johanfylling)
- ast: Attach annotation to static part of rule ref ([#7050](https://github.com/open-policy-agent/opa/issues/7050)) authored by @anderseknert
- ast: Make `Module.String()` include `if`/`contains` for v1 modules ([#6973](https://github.com/open-policy-agent/opa/issues/6973)) authored by @johanfylling reported by @nikpivkin
- topdown/http: Stop `http.send` latency timer when an error is encountered ([#7007](https://github.com/open-policy-agent/opa/pull/7007)) authored by @lukyer
- ast/compile: Refactor local variable replacement and replace declared variables in `with`'s target ([#6979](https://github.com/open-policy-agent/opa/issues/6979)) authored by @srenatus reported by @bluebrown
- ast: Update type checker to cache schema types ([#6970](https://github.com/open-policy-agent/opa/pull/6970)) authored by @nikpivkin
- test: Fix indentation in a YAML test case ([#7039](https://github.com/open-policy-agent/opa/pull/7039)) authored by @matajoh
- format: Bracketing keyword ref elements in formatter output ([#7010](https://github.com/open-policy-agent/opa/pull/7010)) authored by @johanfylling

### Runtime, Tooling, SDK

- Future-proofing tests in the `sdk`, `downlaod`, `server` , `cmd` etc. packages to be `1.0` compatible (authored by @johanfylling)
- cmd: Add `--v0-compatible` flag to make OPA behave as `v0.x` post `v1.0` release ([#7065](https://github.com/open-policy-agent/opa/pull/7065)) authored by @johanfylling
- util: Strip  UTF-8 BOM from input JSON when found ([#6988](https://github.com/open-policy-agent/opa/issues/6988)) authored by @anderseknert reported by @adhilto
- plugins/rest: Support reading AWS token from the filesystem for the AWS container credential provider ([#6997](https://github.com/open-policy-agent/opa/pull/6997)) authored by @cmaddalozzo
- debug: Add `RegoOption` launch option to debugger for setting custom Rego options ([#7045](https://github.com/open-policy-agent/opa/issues/7045)) authored by @johanfylling
- debug: Always include `Input` and `Data` variable scopes to ease discoverability of the scopes ([#7074](https://github.com/open-policy-agent/opa/pull/7074)) authored by @johanfylling
- wasm: Fix arithmetic comparison for large numbers, caused by an integer overflow ([#6991](https://github.com/open-policy-agent/opa/issues/6991)) authored by @Ptroger

### Docs, Website, Ecosystem

- Add Marsh McLennan to adopters ([#7060](https://github.com/open-policy-agent/opa/issues/7060)) authored by @anderseknert reported by @pratimsc
- Add APIwiz to adopters ([#7067](https://github.com/open-policy-agent/opa/pull/7067)) authored by @anderseknert
- docs: Fix misnomer in OPA-Istio tutorial to document Istio's AuthorizationPolicy API ([#6984](https://github.com/open-policy-agent/opa/pull/6984)) authored by @tjons
- docs: Readme updates to highlight more up-to-date information about OPA ([#7066](https://github.com/open-policy-agent/opa/pull/7066)) authored by @charlieegan3
- docs: Update documentation to show Debug API uses ([#7036](https://github.com/open-policy-agent/opa/pull/7036))  authored by @charlieegan3
- docs: Simplify the OPA-Istio tutorial example policy ([#7059](https://github.com/open-policy-agent/opa/pull/7059)) authored by @anderseknert
- website: Update policy examples on the OPA home page to be `1.0` compatible  ([#7033](https://github.com/open-policy-agent/opa/pull/7033))  authored by @charlieegan3

### Miscellaneous

- build: Bump github.com/golang/glob, remove replace directive ([#7024](https://github.com/open-policy-agent/opa/issues/7024)) authored by @srenatus reported by @mmannerm
- Dependency updates; notably:
  - build(deps): bump github.com/containerd/containerd from 1.7.21 to 1.7.22
  - build(deps): bump github.com/prometheus/client_golang from 1.20.2 to 1.20.4
  - build(deps): bump go.uber.org/automaxprocs from 1.5.3 to 1.6.0
  - build(deps): bump golang.org/x/net from 0.28.0 to 0.29.0
  - build(deps): bump google.golang.org/grpc from 1.66.0 to 1.67.0
  - build(go): bump 1.22.5 to 1.23.1 ([#7006](https://github.com/open-policy-agent/opa/pull/7006)) authored by @srenatus

