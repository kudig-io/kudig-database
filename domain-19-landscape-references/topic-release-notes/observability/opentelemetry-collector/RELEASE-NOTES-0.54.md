---
title: opentelemetry-collector v0.54 Release Notes
description: opentelemetry-collector v0.54 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.54 Release Notes 是什么
- 如何 opentelemetry-collector v0.54 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.54
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.54 Release Notes

Source: [v0.54.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.54.0)

## v0.54.0 Beta

### 🛑 Breaking changes 🛑

- Remove deprecated `GetLogger`. (#5504)
- Remove deprecated `configtest.LoadConfigMap` (#5505)
- Remove deprecated `config.Map` (#5505)
- Remove deprecated `config.MapProvider` (#5505)
- Remove deprecated `config.MapConverter` (#5505)
- Remove deprecated `config.Received` (#5505)
- Remove deprecated `config.CloseFunc` (#5505)
- Deprecated `pcommon.Value.NewValueBytes` is brought back taking `pcommon.ImmutableByteSlice` as an argument instead of `[]byte` (#5299)

### 🚩 Deprecations 🚩

- Use immutable slices for primitive types slices to restrict mutations. (#5299)
  - `Value.NewValueMBytes` func is deprecated in favor of `Value.NewValueBytes` func that takes
    `ImmutableByteSlice` instead of `[]byte`
  - `Value.SetMBytesVal` func is deprecated in favor of `Value.SetBytesVal` func that takes
    `pcommon.ImmutableByteSlice` instead of []byte.
  - `Value.BytesVal` func is deprecated in favor of `Value.BytesVal` func that returns `pcommon.ImmutableByteSlice`
    instead of []byte.
  - `<HistogramDataPoint|Buckets>.SetMBucketCounts` funcs are deprecated in favor of
    `<HistogramDataPoint|Buckets>.SetBucketCounts` funcs that take `pcommon.ImmutableUInt64Slice` instead of []uint64.
  - `<HistogramDataPoint|Buckets>.MBucketCounts` funcs are deprecated in favor of
    `<HistogramDataPoint|Buckets>.BucketCounts` funcs that return `pcommon.ImmutableUInt64Slice` instead of []uint64.
  - `HistogramDataPoint.SetMExplicitBounds` func is deprecated in favor of `HistogramDataPoint.SetExplicitBounds` func
    that takes `pcommon.ImmutableFloat64Slice` instead of []float64.
  - `HistogramDataPoint.MExplicitBounds` func func is deprecated in favor of `HistogramDataPoint.ExplicitBounds`
    returns `pcommon.ImmutableFloat64Slice` instead of []float64.

### 💡 Enhancements 💡

- Use OpenCensus `metric` package for process metrics instead of `stats` package (#5486)
- Update OTLP to v0.18.0 (#5530)
- Log histogram min/max fields with `logging` exporter (#5520)

### 🧰 Bug fixes 🧰

- Update sum field of exponential histograms to make it optional (#5530)
- Remove redundant extension shutdown call (#5532)
- Refactor pipelines builder, fix some issues (#5512)
  - Unconfigured receivers are not identified, this was not a real problem in final binaries since the validation of the config catch this.
  - Allow configurations to contain "unused" receivers. Receivers that are configured but not used in any pipeline, this was possible already for exporters and processors.
  - Remove the enforcement/check that Receiver factories create the same instance for the same config.
