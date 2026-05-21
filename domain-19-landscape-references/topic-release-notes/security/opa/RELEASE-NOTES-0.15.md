---
title: opa v0.15 Release Notes
description: opa v0.15 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.15 Release Notes 是什么
- 如何 opa v0.15 Release Notes
trigger_keywords:
- opa
- v0.15
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.15 Release Notes

Source: [v0.15.1](https://github.com/open-policy-agent/opa/releases/tag/v0.15.1)

In this release we reached a milestone for Wasm: any Rego policy can
be compiled to Wasm now! In the next few weeks we will focus on
expanding on the set of built-ins supported out-of-the-box and inside
the NodeJS SDK.

### Fixes

- bundle: Make the DirectoryLoader public ([#1840](https://github.com/open-policy-agent/opa/issues/1840))
- topdown: Add raw_body parameter to http.send ([#1903](https://github.com/open-policy-agent/opa/issues/1903))
- wasm: Update planner to support with keyword ([#1116](https://github.com/open-policy-agent/opa/issues/1116))

### Miscellaneous

- ast: Fix NoWith helper on exprs
- ast: Fix module JSON unmarshalling
- build: Fix build-release.sh by removing obsolete make deps command
- docs: Add JWT verification examples to reference
- docs: Fix introduction to refer to rules consistently
- topdown: Add environment variable to dump tests to disk
- topdown: Provide rewritten query vars in traces
- types: Fix constant select on array types
- wasm: Add support for built-in functions
- wasm: Extend wasm library to support shallow copying
- wasm: Fix JSON string lexing and parsing
- wasm: Fix object insertion operation
- wasm: Fix opa_json_dump to terminate keywords properly
- wasm: Fix opa_set_add to set next element correctly
- wasm: Fix planner to check call expression for false return value
- wasm: Fix planning of virtual document extent
- wasm: Improve calling convention of eval() function
- wasm: Improve planner to reuse local variables
- wasm: Fix planner to plan default rule bodies
- wasm: Remove unnecessary condition statements for scans
- wasm: Store parsed numbers as strings
- website: Replace homepage with new version