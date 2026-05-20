---
title: opentelemetry-collector v0.60 Release Notes
description: opentelemetry-collector v0.60 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.60 Release Notes 是什么
- 如何 opentelemetry-collector v0.60 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.60
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.60 Release Notes

Source: [v0.60.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.60.0)

## v0.60.0 Beta

### 🛑 Breaking changes 🛑

- Replace deprecated `*DataPoint.Flags()` with `*DataPoint.[Set]FlagsImmutable()`. (#6017)
- Remove deprecated `MetricDataPointFlagsStruct` struct and `NewMetricDataPointFlagsStruct` func. (#6017)
- Replace deprecated `MetricDataPointFlags` with `MetricDataPointFlagsImmutable`. (#6017)
- Replace deprecated `LogRecord.[Set]Flags()` with `LogRecord.[Set]FlagsStruct()`. (#6007)
- Remove deprecated components helpers funcs (#6006)
  - `exporterhelper.New[Traces|Metrics|Logs]ExporterWithContext`
  - `processorhelper.New[Traces|Metrics|Logs]ProcessorWithCreateSettings`
  - `component.NewExtensionFactoryWithStabilityLevel`
- Remove deprecated `pcommon.InvalidTraceID` and `pcommon.InvalidSpanID` funcs (#6008)
- Remove deprecated `pcommon.Map` methods: `Update`, `Upsert`, `InsertNull` (#6019)

### 🚩 Deprecations 🚩

- Deprecate pmetric.Metric.SetDataType, in favor of empty setters for each type. (#5979)
- Deprecate `p[metric|log|trace].MarshalerSizer` in favor of `p[metric|log|trace].MarshalSizer`. (#6033)
- Deprecate `pcommon.Map.Update+` in favor of `pcommon.Map.Get` + `pcommon.Value.Set+` (#6013)
- Deprecate `pcommon.Empty[Trace|Span]ID` in favor of `pcommon.New[Trace|Span]IDEmpty` (#6008)
- Deprecate `pcommon.[Trace|Span]ID.Bytes` in favor direct conversion. (#6008)
- Deprecate `pcommon.New[Trace|Span]ID` in favor direct conversion. (#6008)
- Deprecate `MetricDataPointFlagsImmutable` type. (#6017)
- Deprecate `*DataPoint.[Set]FlagsImmutable()` funcs in favor of `*DataPoint.[Set]Flags()`. (#6017)
- Deprecate `LogRecord.FlagsStruct()` and `LogRecord.SetFlagsStruct()` in favor of `LogRecord.Flags()` and `LogRecord.SetFlags()`. (#6007)
- Deprecate `config.Unmarshallable` in favor of `confmap.Unmarshaler`. (#6031)
- Primitive slice wrapper are now mutable (#5971):
  - `pcommon.ImmutableByteSlice` is deprecated in favor of `pcommon.ByteSlice`
  - `pcommon.ImmutableFloat64Slice` is deprecated in favor of `pcommon.Float64Slice`
  - `pcommon.ImmutableUInt64Slice` is deprecated in favor of `pcommon.UInt64Slice`
  - Temporarily deprecate `pcommon.NewValueBytes` that will be replaced with `pcommon.NewValueBytesEmpty` in 0.60.0
  - Deprecate `pcommon.Map.UpsertBytes` in favor of `pcommon.Map.PutEmptyBytes` (#6064)
  - Deprecate `pcommon.Value.SetBytesVal` in favor of `pcommon.Value.SetEmptyBytesVal`
  - Deprecate `pcommon.New[Slice|Map]FromRaw` functions in favor of `New[Slice|Map]().FromRaw` (#6045)
- Deprecate `pcommon.Map.Insert*` methods (#6051)
- Deprecate `pcommon.Map.Upsert*` methods in favor of `pcommon.Map.Put*` (#6064)

### 💡 Enhancements 💡

- Add `skip-get-modules` builder flag to support isolated environment executions (#6009)
  - Skip unnecessary Go binary path validation when the builder is used with `skip-compilation` and `skip-get-modules` flags (#6026)
- Make the otlpreceiver support to use jsoniter to unmarshal JSON payloads. (#6040)
- Add mapstructure hook function for confmap.Unmarshaler interface (#6029)
- Add CopyTo and MoveTo methods to primitive slices (#6044)
- Add support to unmarshalls bytes into plogs.Logs with `jsoniter` in jsonUnmarshaler (#6021)
- Instead of exiting, `ocb` now generates a default Collector when no build configuration is supplied (#5752)

### 🧰 Bug fixes 🧰

- otlpjson: Correctly skip unknown JSON value types. (#6038)
- Fix reading resource attributes from trace JSON. (#6023)
