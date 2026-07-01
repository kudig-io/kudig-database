---
title: opa v1.8 Release Notes
description: opa v1.8 Release Notes — Kubernetes 生产运维知识库
summary: opa v1.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- containerd
- docker
- opa
- llm
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v1.8 Release Notes 是什么
- 如何 opa v1.8 Release Notes
trigger_keywords:
- opa
- v1.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- policy-basics
---



# opa v1.8 Release Notes

Source: [v1.8.0](https://github.com/open-policy-agent/opa/releases/tag/v1.8.0)

This release contains a mix of new features, performance improvements, and bugfixes. Notably:

- Support for EdDSA signatures in `io.jwt` built-ins, including a new `io.jwt.verify_eddsa` built-in.

### EdDSA Support in built-ins ([#7824](https://github.com/open-policy-agent/opa/pull/7824))

Support for the EdDSA signing algorithm has been added to built-in functions in the `io.jwt` namespace.

This introduces the new [io.jwt.verify_eddsa](https://www.openpolicyagent.org/docs/policy-reference/builtins/tokens#builtin-tokens-iojwtverify_eddsa) built-in function, and adds EdDSA support for the following built-ins:

- [io.jwt.decode_verify](https://www.openpolicyagent.org/docs/policy-reference/builtins/tokens#builtin-tokens-iojwtdecode_verify)
- [io.jwt.encode_sign](https://www.openpolicyagent.org/docs/policy-reference/builtins/tokensign#builtin-tokensign-iojwtencode_sign)
- [io.jwt.encode_sign_raw](https://www.openpolicyagent.org/docs/policy-reference/builtins/tokensign#builtin-tokensign-iojwtencode_sign_raw)

This feature benefited greatly from the groundwork laid by @lestrrat in ([#7638](https://github.com/open-policy-agent/opa/issues/7638)). 👏 🎉 🥳 

Authored by @johanfylling reported by @aromeyer

### Runtime

- cmd: Add back default `cmd.RootCommand` definition. ([#7811](https://github.com/open-policy-agent/opa/pull/7811)) authored by @philipaconrad  
  Fixing a breaking change to the go API introduced in OPA v1.7.0.
- cmd: Fix `opa exec` parameters ([#7850](https://github.com/open-policy-agent/opa/issues/7850), [#7840](https://github.com/open-policy-agent/opa/issues/7840)) authored by @srenatus  
  Fixing regressions introduced in OPA v1.7.0, where the `--fail-non-empty` and `--stdin-input` flags were dropped.
- config: accept env vars set to `""`, discern from unset ([#7831](https://github.com/open-policy-agent/opa/issues/7831)) authored by @srenatus reported by @ManuelNowackConfinale
- handlers: Add thread-safe initialization for gzipPool ([#7828](https://github.com/open-policy-agent/opa/pull/7828)) authored by @charlieegan3
- plugins: Address race in config access ([#7825](https://github.com/open-policy-agent/opa/pull/7825)) authored by @charlieegan3
- plugin/bundle: Correct bundle delay behavior ([#7812](https://github.com/open-policy-agent/opa/pull/7812)) authored by @charlieegan3
- runtime: Update server init check ([#7818](https://github.com/open-policy-agent/opa/pull/7818)) authored by @charlieegan3

### Topdown

- perf: Performance greatly improved for `Object.Insert` on existing key ([#7820](https://github.com/open-policy-agent/opa/pull/7820)) authored by @anderseknert
- topdown,bundle,plugins: Upgrade interned jwx (0.9.x) with `github.com/lestrrat-go/jwx/v3` ([#7638](https://github.com/open-policy-agent/opa/issues/7638)) authored by @lestrrat

### Docs, Website

- Update website to build from tip of main ([#7848](https://github.com/open-policy-agent/opa/pull/7848)) authored by @tsandall
- ast/builtins: Remove space from `count` description ([#7836](https://github.com/open-policy-agent/opa/pull/7836)) authored by @charlieegan3
- docs: Add link to logic-or/and on docs index ([#7826](https://github.com/open-policy-agent/opa/pull/7826)) authored by @charlieegan3
- docs: Add note on using LLM in PR discussions ([#7859](https://github.com/open-policy-agent/opa/pull/7859)) authored by @anderseknert
- docs: Fix broken anchor links in annotations ([#7827](https://github.com/open-policy-agent/opa/pull/7827)) authored by @charlieegan3
- docs: Use set in the Python code example for consistence ([#7860](https://github.com/open-policy-agent/opa/pull/7860)) authored by @durnik-ivo
- docs: Update frontpage ([#7847](https://github.com/open-policy-agent/opa/pull/7847)) authored by @tsandall
- docs/rest-api: Add notes about policy IDs ([#7837](https://github.com/open-policy-agent/opa/pull/7837)) authored by @charlieegan3
- website: Use latest release rather than edge ([#7781](https://github.com/open-policy-agent/opa/pull/7781)) authored by @charlieegan3

### Miscellaneous

- Update organization affiliations ([#7842](https://github.com/open-policy-agent/opa/pull/7842)) authored by @tsandall
- test/e2e: Avoid port exhaustion in concurrent tests ([#7862](https://github.com/open-policy-agent/opa/pull/7862)) authored by @anderseknert
- server: Make `TestCertReloading` less verbose ([#7823](https://github.com/open-policy-agent/opa/pull/7823)) authored by @charlieegan3
- cmd: Exec test wait for bundle server to start ([#7821](https://github.com/open-policy-agent/opa/pull/7821)) authored by @charlieegan3
- cmd: Update tests to run sync when ready ([#7835](https://github.com/open-policy-agent/opa/pull/7835)) authored by @charlieegan3
- cmd: Move accidental pkg var to local var ([#7813](https://github.com/open-policy-agent/opa/pull/7813)) authored by @philipaconrad
- internal/report: Allow overriding GitHub repo ([#7867](https://github.com/open-policy-agent/opa/pull/7867)) authored by @srenatus
- release: Adding Dockerfile for image used in `*-patch` build targets ([#7864](https://github.com/open-policy-agent/opa/pull/7864)) authored by @johanfylling
- Dependency updates; notably:
  - build: Bump go to 1.24.6 ([#7834](https://github.com/open-policy-agent/opa/pull/7834), [#7839](https://github.com/open-policy-agent/opa/pull/7839)) authored by @johanfylling and @thevilledev
  - build(deps): Bump go-viper/mapstructure/v2 from v2.3.0 to v2.4.0 ([#7857](https://github.com/open-policy-agent/opa/pull/7857)) authored by @deeglaze
  - build(deps): Bump github.com/containerd/containerd/v2 from 2.1.3 to 2.1.4
  - build(deps): Bump github.com/prometheus/client_golang from 1.22.0 to 1.23.0

