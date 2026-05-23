---
title: opa v1.9 Release Notes
description: opa v1.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- envoy
- opa
- postgresql
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
- opa v1.9 Release Notes 是什么
- 如何 opa v1.9 Release Notes
trigger_keywords:
- opa
- v1.9
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
created: "2026-05-23"
---

# opa v1.9 Release Notes

Source: [v1.9.0](https://github.com/open-policy-agent/opa/releases/tag/v1.9.0)

This release contains a mix of new features, performance improvements, and bugfixes. Notably:

- Compile API extensions ported from EOPA
- Improved rule indexing

### Compile Rego Queries Into SQL Filters ([#7887](https://github.com/open-policy-agent/opa/pull/7887))

Compile API extensions with support for SQL filter generation previously exclusive to EOPA has been ported into OPA.

#### Example

With OPA running with this policy, we'll compile the query `data.filters.include` into SQL filters:

```rego
package filters

# METADATA
# scope: document
# compile:
#   unknowns: [input.fruits]
include if input.fruits.name == input.favorite
```

##### Example Request

```
POST /v1/compile/filters/include HTTP/1.1
Content-Type: application/json
Accept: application/vnd.opa.sql.postgresql+json
```
```json
{
  "input": {
    "favorite": "pineapple"
  }
}
```

##### Example Response

```
HTTP/1.1 200 OK
Content-Type: application/vnd.opa.sql.postgresql+json
```
```json
{
  "result": {
    "query": "WHERE fruits.name = E'pineapple'"
  }
}
```

See the [documentation](https://www.openpolicyagent.org/docs/rest-api#compling-a-rego-policy-and-query-into-data-filters) for more details.

Authored by @srenatus and @philipaconrad

### Improved Rule Indexing For "Naked" Refs ([#7897](https://github.com/open-policy-agent/opa/pull/7897))

OPA's [rule indexer](https://blog.openpolicyagent.org/optimizing-opa-rule-indexing-59f03f17caf3) is a means by which OPA can optimize evaluation performance.
Briefly, the indexer can in some cases determine that a rule won't successfully evaluate _before_ it's evaluated based on the query input.
The indexer previously only considered terms in certain compound expressions, ignoring single terms; e.g. an expression containing a sole "naked" ref. This has now changed!

#### Example

Given a policy with an `allow` rule containing two "naked" refs: `input.foo` and `input.bar`:

```rego
package example

allow if {
    input.foo
    input.bar
}
```

and the input document:

```json
{
    "foo": 1
}
```

before this improvement, when evaluating the query `data.example.allow`, we get the trace log:

```
query:1           Enter data.example.allow = _
query:1           | Eval data.example.allow = _
query:1           | Index data.example.allow (matched 1 rule, early exit)
policy.rego:3     | Enter data.example.allow
policy.rego:5     | | Eval input.foo
policy.rego:6     | | Eval input.bar
policy.rego:6     | | Fail input.bar
policy.rego:5     | | Redo input.foo
query:1           | Fail data.example.allow = _
```

Here, we can see that the `allow` rule is evaluated, but fails on the `input.bar` expression, as it's referencing an `undefined` value.

With the improvement to the indexer, we instead get:

```
query:1     Enter data.example.allow = _
query:1     | Eval data.example.allow = _
query:1     | Index data.example.allow (matched 0 rules, early exit)
query:1     | Fail data.example.allow = _
```

Where we can see that the `allow` rule was never evaluated, since the input doesn't meet the conditions established by the indexer; i.e. both `input.foo` and `input.bar` must have `defined` values.

Authored by @srenatus

### Runtime, Tooling

- cmd: Print eval errors to stderr ([#6749](https://github.com/open-policy-agent/opa/issues/6749)) authored by @sspaink reported by @janorn
- plugin/decision: Encoder immediately returns when event same size as limit ([#7928](https://github.com/open-policy-agent/opa/pull/7928)) authored by @sspaink
- plugin/decision: Refactor size buffer into its own type ([#7884](https://github.com/open-policy-agent/opa/pull/7884)) authored by @sspaink
- plugins/bundle: Return callback error for manually triggered bundle downloads through the SDK ([#7869](https://github.com/open-policy-agent/opa/issues/7869)) authored by @sspaink reported by @victoraugustolls
- runtime: Fix possible panic in `opa run` when loading bundles in watch-mode (`--watch`) ([#7870](https://github.com/open-policy-agent/opa/issues/7870)) authored by @sspaink reported by @johanfylling

### Compiler, Topdown and Rego

- perf: Don't invoke future parser for Rego v1 ([#7909](https://github.com/open-policy-agent/opa/pull/7909)) authored by @anderseknert
- topdown: Add counter metric for http.send network requests ([#7851](https://github.com/open-policy-agent/opa/pull/7851)) authored by @anivar
- topdown: Update `numbers.range_step` built-in error message ([#7882](https://github.com/open-policy-agent/opa/pull/7882)) authored by @charlieegan3

### Docs, Website

- docs: Add `every` and `not` examples ([#7901](https://github.com/open-policy-agent/opa/pull/7901)) authored by @charlieegan3
- docs: Add examples for `io.jwt` and `time` built-ins ([#7892](https://github.com/open-policy-agent/opa/pull/7892)) authored by @charlieegan3
- docs: Add examples for `regex` and `string` built-ins ([#7890](https://github.com/open-policy-agent/opa/pull/7890)) authored by @charlieegan3
- docs: Add guide for common Rego errors ([#7896](https://github.com/open-policy-agent/opa/pull/7896)) authored by @charlieegan3
- docs: Add missing anchors and example data ([#6205](https://github.com/open-policy-agent/opa/issues/6205)) authored by @mmzzuu reported by @johanfylling
- docs: Add Rego keyword examples ([#7889](https://github.com/open-policy-agent/opa/pull/7889)) authored by @charlieegan3
- docs: Add Rego language comparison pages ([#7893](https://github.com/open-policy-agent/opa/pull/7893)) authored by @charlieegan3
- docs: Add Style Guide to policy authoring docs ([#7932](https://github.com/open-policy-agent/opa/pull/7932)) authored by @charlieegan3
- docs: Generative AI policy example fix ([#7885](https://github.com/open-policy-agent/opa/pull/7885)) authored by @msorens
- docs: Remove integration from build-security ([#7899](https://github.com/open-policy-agent/opa/pull/7899)) authored by @ieugen
- docs: Update [[Envoy|Envoy]] tutorial for new versions and images ([#7911](https://github.com/open-policy-agent/opa/pull/7911)) authored by @CharlieTLe
- docs: Update references to cheat sheet and awesome-opa ([#7930](https://github.com/open-policy-agent/opa/pull/7930)) authored by @charlieegan3
- docs: Add OCP docs ([#7875](https://github.com/open-policy-agent/opa/pull/7875)) authored by @charlieegan3
  - docs/ocp: Update docs on Azure object storage ([#7921](https://github.com/open-policy-agent/opa/pull/7921)) authored by @minajevs
  - docs/ocp: Fix inline-transform example ([#7913](https://github.com/open-policy-agent/opa/pull/7913)) authored by @srenatus
  - docs/ocp: Fix wrong example on concepts page ([#7907](https://github.com/open-policy-agent/opa/pull/7907)) authored by @srenatus
  - docs/ocp: Update API reference ([#7906](https://github.com/open-policy-agent/opa/pull/7906)) authored by @srenatus
  - docs/ocp: Update OCP api-key ([#7904](https://github.com/open-policy-agent/opa/pull/7904)) authored by @charlieegan3
  - docs/ocp: Update OCP install instructions ([#7910](https://github.com/open-policy-agent/opa/pull/7910)) authored by @ashutosh-narkar
- docs: Add Regal docs to OPA site ([#7874](https://github.com/open-policy-agent/opa/pull/7874)) authored by @charlieegan3
  - docs/regal: Update docs following 0.36.0 ([#7891](https://github.com/open-policy-agent/opa/pull/7891)) authored by @charlieegan3
- docs/deploy: Add OPA deployment docs ([#7898](https://github.com/open-policy-agent/opa/pull/7898)) authored by @charlieegan3
- docs/website: Update references to Styra ([#7877](https://github.com/open-policy-agent/opa/pull/7877)) authored by @charlieegan3

### Miscellaneous

- Bump golangci-lint to v2.4.0 ([#7878](https://github.com/open-policy-agent/opa/pull/7878)) authored by @sspaink
- Community Guidelines: update email list ([#7900](https://github.com/open-policy-agent/opa/pull/7900)) authored by @srenatus
- ci: port binary tests to testscript ([#7865](https://github.com/open-policy-agent/opa/pull/7865)) authored by @srenatus
- dependabot: Updating e2e go deps together with core OPA deps ([#7923](https://github.com/open-policy-agent/opa/pull/7923)) authored by @johanfylling
- github_actions: Add working directory in arguments for Link Checker ([#7883](https://github.com/open-policy-agent/opa/pull/7883)) authored by @sspaink
- rego: Add comprehensive WASM performance benchmarks ([#7841](https://github.com/open-policy-agent/opa/pull/7841)) authored by @anivar
- Dependency updates; notably:
  - build: Bump go to 1.25.1
  - build(deps): Add github.com/huandu/go-sqlbuilder 1.37.0
  - build(deps): Bump github.com/lestrrat-go/jwx/v3 from 3.0.10 to 3.0.11
  - build(deps): Bump github.com/prometheus/client_golang from 1.23.0 to 1.23.2
  - build(deps): Bump golang.org/x/net from 0.43.0 to 0.44.0
  - build(deps): Bump golang.org/x/time from 0.12.0 to 0.13.0
  - build(deps): Bump google.golang.org/grpc from 1.75.0 to 1.75.1
  - build(deps): Bump google.golang.org/protobuf from 1.36.8 to 1.36.9
  - build(deps): bump go.opentelemetry.io deps from 1.37.0/0.62.0 to 1.38.0/0.63.0

