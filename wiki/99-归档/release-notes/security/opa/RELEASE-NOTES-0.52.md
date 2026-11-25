---
title: opentelemetry-collector v0.52 Release Notes
description: opentelemetry-collector v0.52 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.52 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.52 Release Notes 是什么
- 如何 opentelemetry-collector v0.52 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.52
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




# opentelemetry-collector v0.52 Release Notes

Source: [v0.52.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.52.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.52.0

### 🛑 Breaking changes 🛑

- Remove `configunmarshaler.Unmarshaler` interface, per deprecation comment (#5348)
- Remove deprecated pdata funcs/structs from v0.50.0 (#5345)
- Remove deprecated pdata getters and setters of primitive slice values: `Value.BytesVal`, `Value.SetBytesVal`, 
  `Value.UpdateBytes`, `Value.InsertBytes`, `Value.UpsertBytes`, `<HistogramDataPoint|Buckets>.BucketCounts`, 
  `<HistogramDataPoint|Buckets>.SetBucketCounts`, `HistogramDataPoint.ExplicitBounds`,
  `HistogramDataPoint.SetExplicitBounds` (#5347)
- Remove deprecated featuregate funcs/structs from v0.50.0 (#5346)
- Remove access to deprecated members of the config.Retrieved struct (#5363)
- Replace usage of `config.MapConverterFunc` with `config.MapConverter` (#5382)

### 🚩 Deprecations 🚩

- Deprecate `config.Config` and `config.[[Service|Service]]`, use `service.Config*` (#4608)
- Deprecate `componenterror` package, move everything to `component` (#5383)
- `pcommon.Value.NewValueBytes` is deprecated in favor of `Value.NewValueMBytes` in preparation of migration to 
  immutable slices (#5367)

### 💡 Enhancements 💡

- Update OTLP to v0.17.0 (#5335)
  - Add optional min/max fields to histograms (#5399)

<!-- risk-assessed -->
