---
title: opentelemetry-collector v0.52 Release Notes
description: opentelemetry-collector v0.52 Release Notes — Kubernetes 生产运维知识库
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
created: "2026-05-23"
---

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