---
title: opa v0.28 Release Notes
description: opa v0.28 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- wasm
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- opa v0.28 Release Notes 是什么
- 如何 opa v0.28 Release Notes
trigger_keywords:
- opa
- v0.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.28 Release Notes

Source: [v0.28.0](https://github.com/open-policy-agent/opa/releases/tag/v0.28.0)

This release includes a number of features, enhancements, and fixes. The default
branch for the Git repository has also been updated to `main`.

#### Schema Annotations

This release adds support for _annotations_. Annotations allow users to declare metadata on rules and packages. Currently, OPA supports one form of metadata: schema declarations. For example:

```rego
package example

# METADATA
# schemas:
# - input: schema.service
deny["service is missing required 'owner' label"] {
  input.kind == "Service"
  not input.metadata.labels.owner
}

# METADATA
# schemas:
# - input: schema.deployment
deny["deployment replica count too low for 'production' namespace"] {
  input.kind == "Deployment"
  input.metadata.namespace == "production"
  object.get(input.spec, "replicas", 1) < 3
}
```

Users can include schema annotations in their policies to tell OPA about the structure of external data loaded under `input` or `data`. By learning the schema of base documents, OPA can surface mistakes in the policy at authoring time (e.g., referring to a non-existent field in a JSON object or calling a built-in function with an invalid value.) For more information on the annotations and schema support see the [Type Checking](https://www.openpolicyagent.org/docs/latest/schemas/) page in the documentation. In the future, annotations will be expanded to support other kinds of metadata and additional tooling will be added to leverage them.

### Server

- The server now automatically sets GOMAXPROCS when running inside of a container that has cgroups applied. This helps the Go runtime avoid consuming too many CPU resources and being throttled by the kernel. ([#3328](https://github.com/open-policy-agent/opa/issues/3328))
- The server now logs an error if users enable the `token` authentication mode without a corresponding authorization policy. ([#3380](https://github.com/open-policy-agent/opa/issues/3380)) authored by @[kale-amruta](https://github.com/kale-amruta)
- The server now supports a `GET /v1/config` endpoint that returns OPA's active configuration. This API is useful if you need to debug the running configuration in an OPA configured via Discovery. ([#2020](https://github.com/open-policy-agent/opa/issues/2020))
- The server now respects the `?pretty` option in the v0 API ([#3332](https://github.com/open-policy-agent/opa/issues/3332)) authored by @[clarshad](https://github.com/clarshad)
- The Bundle plugin is more forgiving when it comes to Etag processing on HTTP 304 responses ([#3361](https://github.com/open-policy-agent/opa/issues/3361))
- The Decision Log plugin now supports a "Decision Per Second" rate limit configuration setting.
- The Status plugin can now be configured to use a custom reporter similar to the Decision Log plugin (e.g., so that Status messages can be sent to AWS Kinesis, etc.)
- The Status plugin now reports the number of decision logs that are dropped due to buffer limits.
- The service clients can authenticate with the Azure Identity OAuth2 implementation the client credentials JWT flow is used ([#3372](https://github.com/open-policy-agent/opa/issues/3372))
- Library users can now customize the logger used by the plugins by providing the `plugins.Logger` option when creating the plugin manager.

### Tooling

- The various OPA subcommands that accept schema files now accept a directory tree of schemas instead of only a single schema.
- The `opa refactor move` subcommand was added to support package renaming use cases ([#3290](https://github.com/open-policy-agent/opa/issues/3290))
- The `opa check` subcommand now supports a `-s`/`--schema` flag like the `opa eval` subcommand.

### Documentation

- The [Management API](https://www.openpolicyagent.org/docs/latest/management-introduction/) docs have been restructured so that each API has a dedicated page. In addition, the [Bundle API](https://www.openpolicyagent.org/docs/latest/management-bundles/#implementations) docs now include getting started steps for cloud-provider specific services (e.g., AWS, GCP, Azure, etc.)

### Security

- OPA now supports PKCS8 encoded EC private keys for JWT verification (which includes service authentication, bundle verification, and verification built-in functions) ([#3283](https://github.com/open-policy-agent/opa/issues/3283)). Authored by @[andrehaland](https://github.com/andrehaland).
- The bundle signing and verification APIs have been updated to support custom signers/verififers ([#3336](https://github.com/open-policy-agent/opa/pull/3336)). Authored by @[gshively11](https://github.com/gshively11).

### Evaluation

- The `time.diff` function was added to support calculating differences between date/time values ([#3348](https://github.com/open-policy-agent/opa/issues/3348)) authored by @[andrehaland](https://github.com/andrehaland)
- The `units.parse_bytes` function now supports floating-point values ([#3297](https://github.com/open-policy-agent/opa/issues/3297)) authored by @[andy-paine](https://github.com/andy-paine)
- The evaluator was fixed to use correct bindings when evaluating the full-extent of a partial rule set. This issue was causing unexpected undefined results and evaluation errors in some rare cases. ([#3369](https://github.com/open-policy-agent/opa/issues/3369) [#3376](https://github.com/open-policy-agent/opa/issues/3376))
- The evaluator was fixed to correctly generate package paths when namespacing is disabled partial evaluation. ([#3302](https://github.com/open-policy-agent/opa/issues/3302)).
- The `http.send` function no longer errors out on invalid Expires headers. ([#3284](https://github.com/open-policy-agent/opa/issues/3284))
- The inter-query cache now serializes elements on insertion thereby reducing memory usage significantly (because deserialized elements carry a ~20x cost.) ([#3042](https://github.com/open-policy-agent/opa/issues/3042))
- The rule indexer was fixed to correctly handle mapped and non-mapped values which could occur with `glob.match` usage ([#3293](https://github.com/open-policy-agent/opa/issues/3293))

### WebAssembly

- The `opa eval` subcommand now correctly returns the set of all variable bindings and expression values when the `wasm` target is enabled. Previously it returned only set of variable bindings. ([#3281](https://github.com/open-policy-agent/opa/issues/3281))
- The `glob.match` function now handles the default delimiter correctly. ([#3294](https://github.com/open-policy-agent/opa/issues/3294))
- The `opa build` subcommand no longer requires a capabilities file when the `wasm` target is enabled. If capabilities are not provided, OPA will use the capabilities for its own version. ([#3270](https://github.com/open-policy-agent/opa/issues/3270))
- The `opa build` subcommand now dumps the IR emitted by the planner when `--debug` is specified.
- The `opa eval` subcommand no longer panics when a policy fails to type check and the `wasm` target is enabled.
- The comparison functions can now return `false` instead of either being `true` or `undefined`.  ([#3271](https://github.com/open-policy-agent/opa/issues/3271))
- The internal wasm runtime will now correctly return `CancelErr` to indicate cancellation errors (instead of `BuiltinErr` which it returned previously.)
- The internal wasm runtime now correctly handles non-halt built-in errors ([#3320](https://github.com/open-policy-agent/opa/issues/3320))
- The planner no longer generates unexpected scan statements when negation used over base documents under `data` ([#3279](https://github.com/open-policy-agent/opa/issues/3279)) and ([#3305](https://github.com/open-policy-agent/opa/issues/3305))
- The planner now correctly discards out-of-scope variables when exiting comprehensions ([#3325](https://github.com/open-policy-agent/opa/issues/3325))
- The `rego` package no longer panics when the `wasm` target is enabled and undefined functions are encountered ([#3251](https://github.com/open-policy-agent/opa/issues/3251))
- 🎈 The remaining exceptions in the e2e test framework for the internal wasm runtime have been resolved.

### Build

- The `make image` target now uses the CI image for building the Go binary. This avoids platform-specific build issues by building the Go binary inside of Docker.