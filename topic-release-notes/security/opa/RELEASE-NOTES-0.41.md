---
title: opa v0.41 Release Notes
description: opa v0.41 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
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
- opa v0.41 Release Notes 是什么
- 如何 opa v0.41 Release Notes
trigger_keywords:
- opa
- v0.41
- Release
- Notes
- release
- notes
---

# opa v0.41 Release Notes

Source: [v0.41.0](https://github.com/open-policy-agent/opa/releases/tag/v0.41.0)

This release contains a number of fixes and enhancements.

### GraphQL Built-in Functions

A new set of built-in functions are now available to validate, parse and verify GraphQL query and schema! Following are
the new built-ins:

    graphql.is_valid: Checks that a GraphQL query is valid against a given schema
    graphql.parse: Returns AST objects for a given GraphQL query and schema
    graphql.parse_and_verify: Returns a boolean indicating success or failure alongside the parsed ASTs for a given GraphQL query and schema
    graphql.parse_query: Returns an AST object for a GraphQL query
    graphql.parse_schema: Returns an AST object for a GraphQL schema

### Built-in Function Metadata

Built-in function declarations now support additional metadata to specify name and description for function arguments
and return values. The metadata can be programmatically consumed by external tools such as IDE plugins. The built-in
function documentation is created using the new built-in function metadata.
Check out the new look of the [Built-In Reference](https://www.openpolicyagent.org/docs/latest/policy-reference/#built-in-functions)
page!

Under the hood, a new file called `builtins_metadata.json` is generated via `make generate` which can be consumed by
external tools.

### Tooling, SDK, and Runtime

- OCI Downloader: Add logic to skip bundle reloading based on the digest of the OCI artifact ([#4637](https://github.com/open-policy-agent/opa/issues/4637)) authored by @carabasdaniel
- Bundles: Exclude empty manifest from bundle signature ([#4712](https://github.com/open-policy-agent/opa/issues/4712)) authored by @friedrichsenm reported by @friedrichsenm

### Rego and Topdown

- units.parse: New built-in for parsing standard metric decimal and binary SI units (e.g., K, Ki, M, Mi, G, Gi)
- format: Fix `opa fmt` location for non-key rules  (#4695) (authored by @jaspervdj)
- token: Ignore keys of unknown alg when verifying JWTs with JWKS ([#4699](https://github.com/open-policy-agent/opa/issues/4699)) reported by @lenalebt

### Documentation

- Adding Built-in Functions: Add note about `capabilities.json` while creating a new built-in function
- Policy Reference: Add example for `rego.metadata.rule()` built-in function
- Policy Reference: Fix grammar for `import` keyword ([#4689](https://github.com/open-policy-agent/opa/issues/4689)) authored by @mmzeeman reported by @mmzeeman
- Security: Fix command line flag name for file containing the TLS certificate ([#4678](https://github.com/open-policy-agent/opa/issues/4678)) authored by @pramodak reported by @pramodak

### Website + Ecosystem

- Update Kubernetes policy examples on the website to use latest kubernetes schema (`apiVersion`: `admission.k8s.io/v1`) (authored by @vicmarbev)
- Ecosystem:
  - Add Sansshell (authored by @sfc-gh-jchacon)
  - Add Nginx

### Miscellaneous

- Various dependency bumps, notably:
  - OpenTelemetry-go: 1.6.3 -> 1.7.0
  - go.uber.org/automaxprocs: 1.4.0 -> 1.5.1
  - github.com/containerd/containerd: 1.6.2 -> 1.6.4
  - google.golang.org/grpc: 1.46.0 -> 1.47.0
  - github.com/bytecodealliance/wasmtime-go: 0.35.0 -> 0.36.0
  - github.com/vektah/gqlparser/v2: 2.4.3 -> 2.4.4
- `make test`: Fix "too many open files" issue on Mac OS
- Remove usage of github.com/pkg/errors package (authored by @imjasonh)