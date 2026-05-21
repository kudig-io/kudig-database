---
title: opentelemetry-collector v0.87 Release Notes
description: opentelemetry-collector v0.87 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.87 Release Notes 是什么
- 如何 opentelemetry-collector v0.87 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.87
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.87 Release Notes

Source: [v0.87.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.87.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.87.0

## User facing changes

### 💡 Enhancements 💡

- `service/telemetry exporter/exporterhelper`: Enable sampling logging by default and apply it to all components. (#8134)
  The sampled logger configuration can be disabled easily by setting the `service::telemetry::logs::sampling::enabled` to `false`.
- `core`: Adds the ability for components to report status and for extensions to subscribe to status events by implementing an optional StatusWatcher interface. (#7682)

### 🧰 Bug fixes 🧰

- `telemetry`: remove workaround to ignore errors when an instrument includes a `/` (#8346)

## API changes

### 💡 Enhancements 💡

- `pdata`: Introduce API to control pdata mutability (#6794)
  This change introduces new API pdata methods to control the mutability:
  - p[metric|trace|log].[Metrics|Traces|Logs].MarkReadOnly() - marks the pdata as read-only. Any subsequent
    mutations will result in a panic.
  - p[metric|trace|log].[Metrics|Traces|Logs].IsReadOnly() - returns true if the pdata is marked as read-only.
  Currently, all the data is kept mutable. This API will be used by fanout consumer in the following releases. 

### 🛑 Breaking changes 🛑

- `obsreport`: remove methods/structs deprecated in previous release. (#8492)
- `extension`: remove deprecated Configs and Factories (#8631)
