---
title: opentelemetry-collector v0.88 Release Notes
description: opentelemetry-collector v0.88 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.88 Release Notes 是什么
- 如何 opentelemetry-collector v0.88 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.88
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

# opentelemetry-collector v0.88 Release Notes

Source: [v0.88.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.88.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.88.0

## User facing changes

### 💡 Enhancements 💡

- `fanoutconsumer`: Enable runtime assertions to catch incorrect pdata mutations in the components claiming as non-mutating pdata. (#6794)
  This change enables the runtime assertions to catch unintentional pdata mutations in components that are claimed
  as non-mutating pdata. Without these assertions, runtime errors may still occur, but thrown by unrelated components, 
  making it very difficult to troubleshoot.
  

### 🧰 Bug fixes 🧰

- `exporterhelper`: make enqueue failures available for otel metrics (#8673)
- `exporterhelper`: Fix nil pointer dereference when stopping persistent queue after a start encountered an error (#8718)
- `cmd/builder`: Fix ocb ignoring `otelcol_version` when set to v0.86.0 or later (#8692)

## API changes

### 💡 Enhancements 💡

- `pdata`: Add IsReadOnly() method to p[metrics|logs|traces].[Metrics|Logs|Spans] pdata structs allowing to check if the struct is read-only. (#6794)