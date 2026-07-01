---
title: opa v1.15 Release Notes
description: opa v1.15 Release Notes — Kubernetes 生产运维知识库
summary: opa v1.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- opa
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v1.15 Release Notes 是什么
- 如何 opa v1.15 Release Notes
trigger_keywords:
- opa
- v1.15
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---



# opa v1.15 Release Notes

Source: [v1.15.1](https://github.com/open-policy-agent/opa/releases/tag/v1.15.1)

This patch release fixes a backwards-incompatible change in the `v1/logging.Logger` interface that inadvertently made it into Release v1.15.0.
When using OPA as Go module, and when providing custom `Logger` implementations, this change would break your build.

> [!TIP]
> Users of the binaries or Docker images can ignore this, the code is otherwise the same as v1.15.0.

### Miscellaneous

- logging: make WithContext() optional (authored by @srenatus)

