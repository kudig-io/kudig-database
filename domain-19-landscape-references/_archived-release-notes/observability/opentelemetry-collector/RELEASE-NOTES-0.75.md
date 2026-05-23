---
title: opentelemetry-collector v0.75 Release Notes
description: opentelemetry-collector v0.75 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.75 Release Notes 是什么
- 如何 opentelemetry-collector v0.75 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.75
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.75 Release Notes

Source: [v0.75.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.75.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.75.0

### 🛑 Breaking changes 🛑

- `featuregate`: Remove deprecated featuregate.FlagValue (#7401)

### 💡 Enhancements 💡

- `provider`: Added userfriendly error on incorrect type. (#7399)

### 🧰 Bug fixes 🧰

- `loggingexporter`: Fix display of bucket boundaries of exponential histograms to correctly reflect inclusive/exclusive bounds. (#7445)
- `exporterhelper`: Fix a deadlock in persistent queue initialization (#7400)