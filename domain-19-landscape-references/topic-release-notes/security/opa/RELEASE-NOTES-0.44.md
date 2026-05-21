---
title: opa v0.44 Release Notes
description: opa v0.44 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- containerd
- docker
- opa
- minio
- kafka
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- opa v0.44 Release Notes 是什么
- 如何 opa v0.44 Release Notes
trigger_keywords:
- opa
- v0.44
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- kafka-basics
- policy-basics
---

# opa v0.44 Release Notes

Source: [v0.44.0](https://github.com/open-policy-agent/opa/releases/tag/v0.44.0)

This release contains a number of fixes, two new builtins, a few new features, and several performance improvements.

### Security Fixes

This release includes the security fixes present in the recent v0.43.1 release, which mitigate CVE-2022-36085 in OPA itself, and CVE-2022-27664 and CVE-2022-32190 in our Go build tooling.

See the Release Notes for v0.43.1 for more details.

### Set Element Addition Optimization

Rego Set element addition operations did not scale linearly ([#4999](https://github.com/open-policy-agent/opa/pull/4999)) in the past, and like the Object type before v0.43.0, experienced noticeable reallocation/memory movement overheads once the Set grew past 120k-150k elements in size.

This release introduces different handling of Set internals during element addition operations to avoid pathological reallocation behavior, and allows linear performance scaling up into the 500k key range and beyond.

### Set `union` Built-in Optimization

The Set `union` builtin allows applying the union operation to a set of sets.

However, as discovered in [#4979](https://github.com/open-policy-agent/opa/issues/4979), its implementation generated unnecessary intermediate copies, which resulted in poor performance; in many cases, worse than writing the equivalent operation in pure Rego.

This release improves the `union` builtin's implementation, such that only the final result set is ever modified, reducing memory allocations and GC pressure. The `union` builtin is now about 15-30% faster than the equivalent operation in pure Rego.

### New Built-in Functions: `strings.any_prefix_match` and `strings.any_suffix_match`

This release introduces two new builtins, optimized for bulk matching of string prefixes and suffixes: `strings.any_prefix_match`, and `strings.any_suffix_match`. It works with sets and arrays of strings, allowing efficient matching of collections of prefixes or suffixes against a target string.

See [the built-in functions docs for all the details](https://www.openpolicyagent.org/docs/v0.42.0/policy-reference/#builtin-strings-stringsany_prefix_match)

This implementation fixes [#4994](https://github.com/open-policy-agent/opa/issues/4994) and was authored by @cube2222.

### Tooling, SDK, and Runtime

- Logger: Allow configuration of the timestamp format ([#2413](https://github.com/open-policy-agent/opa/issues/2413))
- loader: Add support for fs.FS (authored by @ear7h)

#### Bundles

This release includes several bugfixes and improvements around bundle building:

- cmd: Add optimize flag to OPA eval command to allow building optimized bundles
- cmd/build+compile: Allow opt-out of dependents gathering to allow compilation of more bundles into WASM ([#5035](https://github.com/open-policy-agent/opa/issues/5035))
- opa build -t wasm|plan: Fail on unmatched entrypoints ([#3957](https://github.com/open-policy-agent/opa/issues/3957))
- opa build: Fix bundle mode to work with ignore flag
- bundle/status: Include bundle size in status information
- bundle: Remove raw bytes check for lazy bundle loading mode

#### Storage Fixes

This release has performance improvements and bugfixes for the disk storage system:

- storage/disk: Improve handling of in-flight transactions during truncate operations ([#4900](https://github.com/open-policy-agent/opa/issues/4900))
- storage/inmem: Allow disabling `util.Roundtrip` on Write for improved performance ([#4708](https://github.com/open-policy-agent/opa/issues/4708))
- storage: Improve multi-bundle data with overlapping roots is handled ([#4998](https://github.com/open-policy-agent/opa/issues/4998)) reported by @sirpi
- storage: Fix issue with policyID in Truncate calls ([#4958](https://github.com/open-policy-agent/opa/issues/4958)) authored by @martinjoha reported by @martinjoha

#### Rego

- eval+rego: Support caching output of non-deterministic builtins. ([#1514](https://github.com/open-policy-agent/opa/issues/1514))

#### AST and Topdown

The AST and Topdown module received a number of important bugfixes in this release:

- ast/term: Fix multiple-reader race condition for Sets/Objects
- ast/compile: Respect unsafeBuiltinMap for 'with' replacements
- ast: Add capacity to array initialization when size is known (authored by @mstrYoda)
- topdown/object: Fix unchecked error case in `object.union_n` builtin ([#5073](https://github.com/open-policy-agent/opa/issues/5073))
- topdown/reachable: Fix missing operand type checks. ([#4951](https://github.com/open-policy-agent/opa/issues/4951))
- topdown/units_parse: Avoid extra decimal places for integers
- topdown/type+wasm: Fix inconsistent `is_type` return values. ([#4943](https://github.com/open-policy-agent/opa/issues/4943))
- builtins: Fix inconsistent error messages in `units.parse*`
- Add query parameter in canonical request of AWS Sigv4 signature to avoid 403 errors from AWS (authored by @sinhaaks)

#### Test Suite

- Add error type to `units.*` builtin test assertions
- test/e2e/certrefresh: Add `file.Sync()` to eliminate test failures due to slow disk writes
- topdown/exported_tests: Remove Golang 1.16 x509 exception
- cmd/bench: Fix port collision in utility function used for E2E testing

### Documentation

- SECURITY: Migrate policy to web site, update content ([#4272](https://github.com/open-policy-agent/opa/issues/4272)) reported by @adoliver
- Add deprecated flag to all deprecated builtins ([#5072](https://github.com/open-policy-agent/opa/issues/5072))
- builtins: Update description of `format_int` to say it rounds down
- docs/policy-reference: Update Rego EBNF grammar (authored by @shaded-enmity)
- docs/builtins: Fix typo in `semver.compare` ([#5012](https://github.com/open-policy-agent/opa/issues/5012)) reported by @tetsuya28
- docs: Fix AWS Signature section in Configuration (authored by @pauly4it)
- docs: Update port and bundle folder for GraphQL tutorial
- docs: Document that function overloading is unsupported
- docs: Fixing related_resources annotations example ([#4982](https://github.com/open-policy-agent/opa/issues/4982)) reported by @humbertoc-silva
- docs: Fixing typo in metadata ([#5018](https://github.com/open-policy-agent/opa/issues/5018)) authored by @cimin0 reported by @cimin0

### Website + Ecosystem

- Update links to opa-kafka-plugin
- Add OCI documentation (authored by @carabasdaniel)
- Add article on using OPA for data filtering in Kafka
- Ecosystem: Add some links to Rönd (authored by @ugho16)
- Add community integration for Fiber (authored by @mstrYoda)
- Add Spacelift Integration (authored by @theseanodell)
- Fix broken link for Minio OPA integration  (authored by @unautre)

- Ecosystem Additions:
  - cosign (#5040) (authored by @Dentrax)

### Miscellaneous

- Dockerfile: Append root "/" to $PATH ([#5003](https://github.com/open-policy-agent/opa/issues/5003)) authored by @matusf reported by @matusf
- Add VNG Cloud to adopters (authored by @vinhph0906)

- Dependency bumps, notably:
  - build: bump golang: 1.19 -> 1.19.1
  - build: use go 1.19, drop go 1.16
  - build(deps): bump aquasecurity/trivy-action from 0.6.1 -> 0.7.1
  - build(deps): bump github.com/agnivade/levenshtein from 1.0.1 -> 1.1.1
  - build(deps): bump github.com/containerd/containerd from 1.6.6 -> 1.6.8
  - build(deps): bump github.com/go-ini/ini from 1.66.6 -> 1.67.0
  - build(deps): bump github.com/prometheus/client_golang
  - build(deps): bump google.golang.org/grpc from 1.48.0 -> 1.49.0
  - build(deps): bump tj-actions/changed-files from 28.0.0 -> 29.0.3

- Dependency removals:
  - internal: Vendor gqlparser library ([#5065](https://github.com/open-policy-agent/opa/issues/5065)) reported by @vikstrous2