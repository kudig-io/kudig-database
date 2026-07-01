---
title: opentelemetry-collector v0.64 Release Notes
description: opentelemetry-collector v0.64 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.64 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.64 Release Notes 是什么
- 如何 opentelemetry-collector v0.64 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.64
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.64 Release Notes

Source: [v0.64.1](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.64.1)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.64.1

### 🧰 Bug fixes 🧰

- `loggingexporter`: Fix logging exporter to not mutate the data (#6420)