---
title: opentelemetry-collector v0.85 Release Notes
description: opentelemetry-collector v0.85 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.85 Release Notes 是什么
- 如何 opentelemetry-collector v0.85 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.85
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.85 Release Notes

Source: [v0.85.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.85.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.85.0

### 💡 Enhancements 💡

- `components command`: The "components" command now lists the component's stability levels. (#8289)
  Note that the format of this output is NOT stable and can change between versions.
- `confighttp`: Add option to disable HTTP keep-alives (#8260)

### 🧰 Bug fixes 🧰

- `confmap`: fix bugs of unmarshalling slice values (#4001)
- `exporterhelper`: Stop logging error messages suggesting user to enable `retry_on_failure` or `sending_queue` when they are not available. (#8369)