---
title: opentelemetry-collector v0.87 Release Notes
description: opentelemetry-collector v0.87 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.87 Release Notes — Kubernetes 生产运维知识库
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.87 Release Notes

Source: [v0.87.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.87.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.87.0

## User facing changes

### 💡 Enhancements 💡

- `[[Service|service]]/telemetry exporter/exporterhelper`: Enable sampling logging by default and apply it to all components. (#8134)
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


<!-- risk-assessed -->
