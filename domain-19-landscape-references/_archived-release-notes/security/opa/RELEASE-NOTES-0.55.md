---
title: opentelemetry-collector v0.55 Release Notes
description: opentelemetry-collector v0.55 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.55 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.55 Release Notes 是什么
- 如何 opentelemetry-collector v0.55 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.55
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




# opentelemetry-collector v0.55 Release Notes

Source: [v0.55.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.55.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.55.0

### 🛑 Breaking changes 🛑

- Remove deprecated `config.ServiceTelemetry` (#5565)
- Remove deprecated `config.ServiceTelemetryLogs` (#5565)
- Remove deprecated `config.ServiceTelemetryMetrics` (#5565)

### 🚩 Deprecations 🚩

- Deprecate `[[Service|service]].ConfigServiceTelemetry`, `service.ConfigServiceTelemetryLogs`, and `service.ConfigServiceTelemetryMetrics` (#5565)
- Deprecate the following component functions to ensure a stability level is set (#5580):
  - `component.WithTracesExporter` -> `component.WithTracesExporterAndStabilityLevel`
  - `component.WithMetricsExporter` -> `component.WithMetricsExporterAndStabilityLevel`
  - `component.WithLogsExporter` -> `component.WithLogsExporterAndStabilityLevel`
  - `component.WithTracesReceiver` -> `component.WithTracesReceiverAndStabilityLevel`
  - `component.WithMetricsReceiver` -> `component.WithMetricsReceiverAndStabilityLevel`
  - `component.WithLogsReceiver` -> `component.WithLogsReceiverAndStabilityLevel`
  - `component.WithTracesProcessor` -> `component.WithTracesProcessorAndStabilityLevel`
  - `component.WithMetricsProcessor` -> `component.WithMetricsProcessorAndStabilityLevel`
  - `component.WithLogsProcessor` -> `component.WithLogsProcessorAndStabilityLevel`

### 💡 Enhancements 💡

- Components stability levels are now logged. By default components which haven't defined their stability levels, or which are
  unmaintained, deprecated or in development will log a message. (#5580)

### 💡 Enhancements 💡

- `exporter/logging`: Skip "bad file descriptor" sync errors (#5585)

### 🧰 Bug fixes 🧰

- Fix initialization of the [[OpenTelemetry|OpenTelemetry]] MetricProvider. (#5571)
- Set log level for `undefined` stability level to debug. (#5635)


<!-- risk-assessed -->
