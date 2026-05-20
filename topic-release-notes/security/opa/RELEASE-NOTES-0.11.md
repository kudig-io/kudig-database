---
title: opa v0.11 Release Notes
description: opa v0.11 Release Notes — Kubernetes 生产运维知识库
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
estimated_read_time: 5min
intent_queries:
- opa v0.11 Release Notes 是什么
- 如何 opa v0.11 Release Notes
trigger_keywords:
- opa
- v0.11
- Release
- Notes
- release
- notes
---

# opa v0.11 Release Notes

Source: [v0.11.0](https://github.com/open-policy-agent/opa/releases/tag/v0.11.0)


### Compatibility Notes

This release includes a few small but backward incompatible
changes:

* The compiler will reject functions that redeclare arguments. A
  search of public .rego files on GitHub only returned one result
  which was contained in the OPA documentation. For example:

    ```
    f(x) {
        x := 1  # bad: redeclaration of 'x'
        x == 1  # ok
    }
    ```

* Errors returned by built-in calls are no longer coded as
  `eval_internal_error`. Instead they are returned as
  `eval_builtin_error`. This change is made so callers can
  differentiate between actual internal errors and built-in errors
  that are result of bad inputs from the policy.

* The `ast.QueryCompiler#WithInput` function and
  `ast.QueryContext#Input` field have been removed because they were
  unused and had no affect.

* The `ast.Compiler` and `ast.QueryCompiler` functions to register
  extra changes now require a stage and metric name.

### Major Features

This release includes a few notable features and improvements:

* The `some` keyword allows you to declare local variables to avoid
  namespacing issues. See the [Some
  Keyword](https://www.openpolicyagent.org/docs/edge/how-do-i-write-policies/#some-keyword)
  section in the documentation for more detail.

* The `opa test`, `eval`, REPL, and HTTP API have been extended with a
  new explanation mode for filtering tracing notes. This makes it
  easier to see the output of `trace(msg)` calls from your policy.

* The WebAssembly (Wasm) compiler has been extended to include support for
  compiling rules into Wasm. Previously the compiler relied on partial
  evaluation to inline all rules. In some cases this is not possible
  due to limitations on Rego queries. In coming releases, the Wasm
  support will be extended to cover the entire language.

* The `rego` package has been extended to support prepared
  queries. Prepared queries cache the parsed and compiled query ASTs
  for re-use across multiple `Eval` calls. For small policies the
  speedup can be significant. See the [GoDoc](https://godoc.org/github.com/open-policy-agent/opa/rego#example-Rego-PrepareForEval) for details.

### Fixes

- Add Kubernetes admission control debugging tips ([#1039](https://github.com/open-policy-agent/opa/issues/1039))
- Add docs on health check API endpoint ([#1086](https://github.com/open-policy-agent/opa/issues/1086))
- Add hardened configuration example to security page ([#1172](https://github.com/open-policy-agent/opa/issues/1172))
- Add support for with keyword stacking ([#802](https://github.com/open-policy-agent/opa/issues/802))
- Fix type inferencing on object keys ([#1361](https://github.com/open-policy-agent/opa/issues/1361))
- Fix simple Kubernetes deployment example ([#874](https://github.com/open-policy-agent/opa/issues/874))
- Fix bug in data mocking that resulted in wrong iteration behavior ([#1261](https://github.com/open-policy-agent/opa/issues/1261))
- Fix bug in set deep copy that caused panic ([#1406](https://github.com/open-policy-agent/opa/issues/1406))
- Fix bug in REPL that prevented rules from being declared ([#1104](https://github.com/open-policy-agent/opa/issues/1104))

### Miscellaneous

- docs: Better documentation for providing the `input` document over HTTP ([#1293](https://github.com/open-policy-agent/opa/issues/1293))
- docs: Add note about HTTP_PROXY  friends ([#1410](https://github.com/open-policy-agent/opa/issues/1410))
- Add CLI config overrides and ENV injection
- Add additional compiler metrics for each stage
- Add an “edge” release to the docs
- Add param to include bundle activation in /health response
- Add provenance query output
- Add support for graceful shutdown of OPA server
- Improve discovery feature documentation
- Make `json` logs the default and add `json-pretty`
- Raise error when loading empty module in bundle
- Return eval_builtin_error instead of eval_internal_error
- Rewrite == to = in queries passed to the compile API
- docs: Update bundle docs with caching info
- Update logrus to 1.4.0
- server: Add early exit on PUT /v1/policies
- topdown: Fix set unification partial eval bug
- topdown: Omit rule body from enter/redo events