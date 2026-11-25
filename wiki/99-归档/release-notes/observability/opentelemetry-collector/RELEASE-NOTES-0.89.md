---
title: opentelemetry-collector v0.89 Release Notes
description: opentelemetry-collector v0.89 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.89 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.89 Release Notes 是什么
- 如何 opentelemetry-collector v0.89 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.89
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




# opentelemetry-collector v0.89 Release Notes

Source: [v0.89.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.89.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.89.0

## User facing changes

### 💡 Enhancements 💡

- `builder`: remove replace statement in builder template (#8763)
- `[[Service|service]]/extensions`: Allow extensions to declare dependencies on other extensions and guarantee start/stop/notification order accordingly. (#8732)
- `exporterhelper`: Log export errors when retry is not used by the component. (#8791)
- `cmd/builder`: Add --verbose flag to log `go` subcommands output that are ran as part of a build (#8715)
- `exporterhelper`: Remove internal goroutine loop for persistent queue (#8868)
- `exporterhelper`: Simplify usage of storage client, avoid unnecessary allocations (#8830)
- `exporterhelper`: Simplify logic in boundedMemoryQueue, use channels len/cap (#8829)

### 🧰 Bug fixes 🧰

- `exporterhelper`: fix bug with queue size and capacity metrics (#8682)
- `obsreporttest`: split handler for otel vs oc test path in TestTelemetry (#8758)
- `builder`: Fix featuregate late initialization (#4967)
- `service`: Fix connector logger zap kind key (#8878)

## API changes

### 🛑 Breaking changes 🛑

- `otelcol`: CollectorSettings.Factories now expects: `func() (Factories, error)` (#8478)
- `exporter/exporterhelper`: The experimental Request API is updated. (#7874)
  - `Request` interface now includes ItemsCount() method.
  - `RequestItemsCounter` is removed.
  - The following interfaces are added:
    - Added an optional interface for handling errors that occur during request processing `RequestErrorHandler`.
    - Added a function to unmarshal bytes into a Request `RequestUnmarshaler`.
    - Added a function to marshal a Request into bytes `RequestMarshaler`
  

### 🚩 Deprecations 🚩

- `featuregate`: Deprecate `featuregate.NewFlag` in favor of `featuregate.Registry`'s `RegisterFlags` method (#8727)

### 💡 Enhancements 💡

- `featuregate`: Add validation for feature gates ID, URL and versions. (#8766)
  Feature gates IDs are now explicitly restricted to ASCII alphanumerics and dots.
  


<!-- risk-assessed -->
