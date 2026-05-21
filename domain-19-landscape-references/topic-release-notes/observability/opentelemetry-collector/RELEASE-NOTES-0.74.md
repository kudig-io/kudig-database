---
title: opentelemetry-collector v0.74 Release Notes
description: opentelemetry-collector v0.74 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.74 Release Notes 是什么
- 如何 opentelemetry-collector v0.74 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.74
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.74 Release Notes

Source: [v0.74.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.74.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.74.0

### 🛑 Breaking changes 🛑

- `consumererror`: Remove deprecated funcs in consumererror (#7357)

### 🚩 Deprecations 🚩

- `featuregate`: Deprecate `FlagValue` in favor of `NewFlag`. (#7042)

### 💡 Enhancements 💡

- `service`: Enable connectors by default by moving service.connectors featuregate to beta (#7369)