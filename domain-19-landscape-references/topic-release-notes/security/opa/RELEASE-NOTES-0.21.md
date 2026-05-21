---
title: opa v0.21 Release Notes
description: opa v0.21 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.21 Release Notes 是什么
- 如何 opa v0.21 Release Notes
trigger_keywords:
- opa
- v0.21
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

# opa v0.21 Release Notes

Source: [v0.21.1](https://github.com/open-policy-agent/opa/releases/tag/v0.21.1)

This release fixes [#2497](https://github.com/open-policy-agent/opa/issues/2497) where the comprehension indexing optimization produced incorrect results for nested comprehensions that close over variables in the outer scope. This issue only affects policies containing nested comprehensions that are recognized by the indexer (which is a relatively small percentage).

This release also backports the GitHub Actions migration and a fix to the Wasm library build step.