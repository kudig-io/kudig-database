---
title: opa v0.13 Release Notes
description: opa v0.13 Release Notes — Kubernetes 生产运维知识库
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
- opa v0.13 Release Notes 是什么
- 如何 opa v0.13 Release Notes
trigger_keywords:
- opa
- v0.13
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

# opa v0.13 Release Notes

Source: [v0.13.5](https://github.com/open-policy-agent/opa/releases/tag/v0.13.5)

- Fix panic in OPA HTTP server with `/health?bundle=true` when
  using bundles loaded from CLI ([#1703](https://github.com/open-policy-agent/opa/issues/1703)).
