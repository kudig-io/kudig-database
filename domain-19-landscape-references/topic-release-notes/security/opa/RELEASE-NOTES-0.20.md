---
title: opa v0.20 Release Notes
description: opa v0.20 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.20 Release Notes 是什么
- 如何 opa v0.20 Release Notes
trigger_keywords:
- opa
- v0.20
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.20 Release Notes

Source: [v0.20.5](https://github.com/open-policy-agent/opa/releases/tag/v0.20.5)

### Fixes

- compile: Change name of result var for wasm binary ([#2441](https://github.com/open-policy-agent/opa/issues/2441))
- format: Deep copy inputs to avoid mutating the caller's copy ([#2439](https://github.com/open-policy-agent/opa/issues/2439))

### Miscellaneous

- docs: Add `opa_println` to wasm required imports
