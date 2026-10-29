---
title: opentelemetry-collector v0.50 Release Notes
description: opentelemetry-collector v0.50 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.50 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.50 Release Notes 是什么
- 如何 opentelemetry-collector v0.50 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.50
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




# opentelemetry-collector v0.50 Release Notes

Source: [v0.50.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.50.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.50.0

### 🛑 Breaking changes 🛑

- Remove pdata deprecated funcs from 2 versions (v0.48.0) ago. (#5219)
- Remove non pdata deprecated funcs/structs (#5220)
- `pmetric.Exemplar.ValueType()` now returns new type `ExemplarValueType` (#5233)

### 🚩 Deprecations 🚩

- Deprecate `configunmarshaler` package, move it to internal (#5151)
- Deprecate all API in `model/semconv`. The package is moved to a new `semcomv` module (#5196)
- Deprecate access to `config.Retrieved` fields, use the newly added funcs to interact with the internal fields (#5198)
- Deprecate `p<signal>otlp.Request.Set<Logs|Metrics|Traces>` (#5234)
  - `plogotlp.Request.SetLogs` func is deprecated in favor of `plogotlp.NewRequestFromLogs`
  - `pmetricotlp.Request.SetMetrics` func is deprecated in favor of `pmetricotlp.NewRequestFromMetrics`
  - `ptraceotlp.Request.SetTraces` func is deprecated in favor of `ptraceotlp.NewRequestFromTraces`
- `pmetric.NumberDataPoint.ValueType()` now returns new type `NumberDataPointValueType` (#5233)
  - `pmetric.MetricValueType` is deprecated in favor of `NumberDataPointValueType`
  - `pmetric.MetricValueTypeNone` is deprecated in favor of `NumberDataPointValueTypeNone`
  - `pmetric.MetricValueTypeInt` is deprecated in favor of `NumberDataPointValueTypeInt`
  - `pmetric.MetricValueTypeDouble` is deprecated in favor of `NumberDataPointValueTypeDouble`
- Deprecate `plog.LogRecord.SetName()` function (#5230)
- Deprecate global `featuregate` funcs in favor of `GetRegistry` and a public `Registry` type (#5160)

### 💡 Enhancements 💡

- Extend config.Map.Unmarshal hook to check map key string to any TextUnmarshaler not only ComponentID (#5244)
- Collector will no longer print error with stack trace when the collector is shutdown due to a context cancel. (#5258)

### 🧰 Bug fixes 🧰
- Fix translation from otlp.Request to pdata representation, changes to the returned pdata not all reflected to the otlp.Request (#5197)
- `exporterhelper` now properly consumes any remaining items on stop (#5203)
- `pdata`: Fix copying of `Value` with `ValueTypeBytes` type (#5267)
- `pdata`: Fix copying of metric fields of primitive items slice type (#5271)


<!-- risk-assessed -->
