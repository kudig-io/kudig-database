---
title: opa v0.16 Release Notes
description: opa v0.16 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v0.16 Release Notes 是什么
- 如何 opa v0.16 Release Notes
trigger_keywords:
- opa
- v0.16
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
created: "2026-05-23"
---

# opa v0.16 Release Notes

Source: [v0.16.2](https://github.com/open-policy-agent/opa/releases/tag/v0.16.2)

This release includes an important bugfix for users that enable
tracing and use the "pretty" trace formatter.

- topdown: Fix bug in var rewriting during trace formatting ([#2022](https://github.com/open-policy-agent/opa/issues/2022))