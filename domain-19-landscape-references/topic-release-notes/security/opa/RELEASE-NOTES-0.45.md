---
title: opa v0.45 Release Notes
description: opa v0.45 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- wasm
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- opa v0.45 Release Notes 是什么
- 如何 opa v0.45 Release Notes
trigger_keywords:
- opa
- v0.45
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.45 Release Notes

Source: [v0.45.0](https://github.com/open-policy-agent/opa/releases/tag/v0.45.0)

This release contains a mix of bugfixes, optimizations, and new features.

### Improved Decision Logging with `nd_builtin_cache`

OPA has several non-deterministic built-ins, such as `rand.intn` and `http.send` that can make debugging policies from decision log results a surprisingly tricky and involved process. To improve the situation around debugging policies that use those built-ins, OPA now provides an opt-in system for caching the inputs and outputs of these built-ins during policy evaluation, and can include this information in decision log entries.

A new top-level config key is used to enable the non-deterministic builtin caching feature, as shown below:

    nd_builtin_cache: true

This data is exposed to OPA's [decision log masking system](https://www.openpolicyagent.org/docs/v0.45.0/management-decision-logs/#masking-sensitive-data) under the `/nd_builtin_cache` path, which allows masking or dropping sensitive values from decision logs selectively. This can be useful in situations where only some information about a non-deterministic built-in was needed, or the arguments to the built-in involved sensitive data.

To prevent unexpected decision log size growth from non-deterministic built-ins like `http.send`, the new cache information is included in decision logs on a best-effort basis. If a decision log event exceeds the `decision_logs.reporting.upload_size_limit_bytes` limit for an OPA instance, OPA will reattempt uploading it, after dropping the non-deterministic builtin cache information from the event. This behavior will trigger a log error when it happens, and will increment the `decision_logs_nd_builtin_cache_dropped` metrics counter, so that it will be possible to debug cases where the cache information is unexpectedly missing from a decision log entry.

#### Decision Logging Example

To observe the change in decision logging we can run OPA in server mode with `nd_builtin_cache` enabled:

```bash
opa run -s --set=decision_logs.console=true,nd_builtin_cache=true
```

After sending it the query `x := rand.intn("a", 15)` we should see something like the following in the decision logs:

```
{..., "msg":"Decision Log", "nd_builtin_cache":{"rand.intn":{"[\"a\",15]":3}}, "query":"assign(x, rand.intn(\"a\", 15))", ..., "result":[{"x":3}], ..., "type":"openpolicyagent.org/decision_logs"}
```

The new information is included under the optional `nd_builtin_cache` JSON key, and shows what arguments were provided for each unique invocation of `rand.intn`, as well as what the output of that builtin call was (in this case, `3`).

If we send the query `x := rand.intn("a", 15); y := rand.intn("b", 150)"` we can see how unique input arguments get recorded in the cache:

```
{..., "msg":"Decision Log", "nd_builtin_cache":{"rand.intn":{"[\"a\",15]":12,"[\"b\",150]":149}}, "query":"assign(x, rand.intn(\"a\", 15)); assign(y, rand.intn(\"b\", 150))", ..., "result":[{"x":12,"y":149}], ..., "type":"openpolicyagent.org/decision_logs"}
```

With this information, it's now easier to debug exactly why a particular rule is used or why a rule fails when non-deterministic builtins are used in a policy.

### New Built-in Function: `regex.replace`

This release introduces a new builtin for regex-based search/replace on strings: `regex.replace`.

See [the built-in functions docs for all the details](https://www.openpolicyagent.org/docs/v0.45.0/policy-reference/#builtin-regex-regexreplace)

This implementation fixes [#5162](https://github.com/open-policy-agent/opa/issues/5162) and was authored by @boranx.

### `object.union_n` Optimization

The `object.union_n` builtin allows easily merging together an array of Objects.

Unfortunately, as noted in [#4985](https://github.com/open-policy-agent/opa/issues/4985) its implementation generated unnecessary intermediate copies from doing pairwise, recursive Object merges. These pairwise merges resulted in poor performance for large inputs; in many cases worse than writing the equivalent operation in pure Rego.

This release changes the `object.union_n` builtin's implementation to use a more efficient merge algorithm that respects the original implementation's sequential, left-to-right merging semantics. The `object.union_n` builtin now provides a 2-3x improvement in speed and memory efficiency over the pure Rego equivalent.

### Tooling, SDK, and Runtime

- cli: Fix doubled CLI hints/errors. ([#5115](https://github.com/open-policy-agent/opa/issues/5115)) authored by @ivanphdz
- cli/test: Add capabilities flag to test command. (authored by @ivanphdz)
- fmt: Fix blank lines after multiline expressions. (authored by @jaspervdj)
- internal/report: Include heap usage in the telemetry report.
- plugins/logs: Improve error message when decision log chunk size is greater than the upload limit. ([#5155](https://github.com/open-policy-agent/opa/issues/5155))
- ir: Make the `internal/ir` package public as `ir`.

### Rego

- ast/parser+formatter: Allow 'if' in rule 'else' statements.
- ast/schema: Add support for recursive json schema elements. ([#5166](https://github.com/open-policy-agent/opa/issues/5166)) authored and reported by @liamg
- ast/schema: Fix race condition in parsing with reused references.(authored by @liamg)
- internal/gojsonschema: Fix race condition in `SetAllowNet`. ([#5187](https://github.com/open-policy-agent/opa/issues/5187)) authored and reported by @liamg
- ast/compiler: Rewrite declared variables in function calls and recursively rewrite local variables in `with` clauses. ([#5148](https://github.com/open-policy-agent/opa/issues/5148)) authored and reported by @liu-du
- ast: Skip rules when parsing a body (or query) to help improve ambiguous parsing cases.

### Topdown

- topdown/object: Rework `object.union_n` to use in-place merge algorithm. (reported by @charlesdaniels)
- topdown/jwt_decode_verify: Ensure `exp` and `nbf` fields are numbers when present. ([#5165](https://github.com/open-policy-agent/opa/issues/5165)) authored and reported by @charlieflowers
- topdown: Fix `InterQueryCache` only dropping one entry when over the size limit. (authored by @vinhph0906)
- topdown+builtins: Block all ND builtins from partial evaluation.
- topdown/builtins: Add Rego Object support for GraphQL builtins to improve composability.
- topdown/json: Fix panic in `json.filter` on empty JSON paths.
- topdown/sets_bench_test: Add `intersection` builtin tests.
- topdown/tokens: Protect against nistec panics. ([#5128](https://github.com/open-policy-agent/opa/issues/5218))

### Documentation

- Add IR to integration docs.
- Added Gloo Edge Tutorial with examples. (authored by @Parsifal-M)
- Updated examples for CLI commands.
- Updated section on performance metrics (authored by @hutchins)
- docs/annotations: Add policy example and a link to the policy reference. ([#4937](https://github.com/open-policy-agent/opa/issues/4937)) authored by @Parsifal-M
- docs/policy-language: Be more explicit about future keywords.
- docs/security: Fix token authz example. (authored by @pigletfly)
- docs: Update generated CLI docs. (authored by @charlieflowers)
- docs: Update mentions of `#development` to `#contributors`. (authored by @charlieflowers)

### Website + Ecosystem

- website/security: Style improvements. (authored by @orweis)

### Miscellaneous

- ci: Add `prealloc` linter check and linter fixes.
- ci: Add govulncheck to Nightly CI.
- build/wasm: Use golang1.16 `go:embed` mechanism.
- util/backoff: Seed from math/rand source.
- version: Use `runtime/debug.BuildInfo`.

- Dependency bumps, notably:
  - build: bump golang 1.19.1 -> 1.19.2
  - build(deps): bump golang.org/x/net
  - build(deps): bump internal/gqlparser to v2.5.1
  - build(deps): bump tj-actions/changed-files from 29.0.3 -> 32.0.0
  - deps(build): bump wasmtime-go 0.36.0 -> 1.0.0 (authored by @Parsifal-M)