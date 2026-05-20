---
title: opentelemetry-collector v0.59 Release Notes
description: opentelemetry-collector v0.59 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.59 Release Notes 是什么
- 如何 opentelemetry-collector v0.59 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.59
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.59 Release Notes

Source: [v0.59.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.59.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.59.0

### 🛑 Breaking changes 🛑

- Remove deprecated fields/funcs from `service` (#5907)
  - Remove `ConfigProviderSettings.Location`
  - Remove `ConfigProviderSettings.MapProviders`
  - Remove `ConfigProviderSettings.MapConverters`
  - Remove `featuregate.Registry.MustAppy`
- Remove deprecated funcs from `pdata` module. (#5911)
  - Remove `pmetric.MetricDataPointFlags.String()`
  - Remove `pmetric.NumberDataPoint.FlagsStruct()`
  - Remove `pmetric.HistogramDataPoint.FlagsStruct()`
  - Remove `pmetric.ExponentialHistogramDataPoint.FlagsStruct()`
  - Remove `pmetric.SummaryDataPoint.FlagsStruct()`
- Remove deprecated settings from `obsreport`, `ProcessorSettings.Level` and `ExporterSettings.Level` (#5918)
- Replace `processorhelper.New[Traces|Metrics|Logs]Exporter` with `processorhelper.New[Traces|Metrics|Logs]ProcessorWithCreateSettings` definition (#5915)
- Replace `exporterhelper.New[Traces|Metrics|Logs]Exporter` with `exporterhelper.New[Traces|Metrics|Logs]ExporterWithContext` definition (#5914)
- Replace ``component.NewExtensionFactory`` with `component.NewExtensionFactoryWithStabilityLevel` definition (#5917)
- Set TLS 1.2 as default for `min_version` for TLS configuration in case this property is not defined (affects servers). (#5956)

### 🚩 Deprecations 🚩

- Deprecate `processorhelper.New[Traces|Metrics|Logs]ProcessorWithCreateSettings` in favor of `processorhelper.New[Traces|Metrics|Logs]Exporter` (#5915)
- Deprecates `LogRecord.Flags()` and `LogRecord.SetFlags()` in favor of `LogRecord.FlagsStruct()` and `LogRecord.SetFlagsStruct()`. (#5972)
- Deprecate `exporterhelper.New[Traces|Metrics|Logs]ExporterWithContext` in favor of `exporterhelper.New[Traces|Metrics|Logs]Exporter` (#5914)
- Deprecate `component.NewExtensionFactoryWithStabilityLevel` in favor of `component.NewExtensionFactory` (#5917)
- Deprecate `plog.SeverityNumber[UPPERCASE]` constants (#5927)
- Deprecate `pcommon.Map.InsertNull` method (#5955)
- Deprecate FlagsStruct types (#5933):
  - `MetricDataPointFlagsStruct` -> `MetricDataPointFlags`
  - `NewMetricDataPointFlagsStruct` -> `NewMetricDataPointFlags`
- Deprecate builder distribution flags, use configuration. (#5946)
- Enforce naming conventions for Invalid[Trace|Span]ID: (#5969)
  - Deprecate funcs `pcommon.InvalidTraceID` and `pcommon.InvalidSpanID` in favor of vars `pcommon.EmptyTraceID` and `pcommon.EmptySpanID` 
- Deprecate `Update` and `Upsert` methods of `pcommon.Map` (#5975)
- Deprecated the current MetricDataPointFlags API.  The new API provides functions to check and set Flags. (#5999)
  - `NumberDataPoint.Flags` -> `NumberDataPoint.FlagsImmutable`
  - `HistogramDataPoint.Flags` -> `HistogramDataPoint.FlagsImmutable`
  - `ExponentialHistogramDataPoint.Flags` -> `ExponentialHistogramDataPoint.FlagsImmutable`
  - `SummaryDataPoint.Flags` -> `SummaryDataPoint.FlagsImmutable`
  - `MetricDataPointFlags` -> `MetricDataPointFlagsImmutable`
  - `NewMetricDataPointFlags` -> `MetricDataPointFlagsImmutable`

### 💡 Enhancements 💡

- Added `MarshalerSizer` interface to `ptrace`, `plog`, and `pmetric` packages. `NewProtoMarshaler` now returns a `MarshalerSizer` (#5929)
- Add support to unmarshalls bytes into pmetric.Metrics with `jsoniter` in jsonUnmarshaler(#5433)
- Add httpprovider to allow loading config files stored in HTTP (#5810)
- Added `service.telemetry.traces.propagators` configuration to set propagators for collector's internal spans. (#5572)
- Remove unnecessary duplicate code and allocations for reading enums in JSON. (#5928)
- Add "dist.build_tags" configuration option to support passing go build flags to builder. (#5659)
- Add an AsRaw func on the flags, lots of places to encode these flags. (#5934)
- Change pdata generated types to use type definition instead of aliases. (#5936)
  - Improves documentation, and makes code easier to read/understand.
- Log `InstrumentationScope` attributes in `loggingexporter` (#5976)
- Add `UpsertEmpty`, `UpsertEmptyMap` and `UpsertEmptySlice` methods to `pcommon.Map` (#5975)
- Add `SetEmptyMapVal` and `SetEmptySliceVal` methods to `pcommon.Value` (#5975)

### 🧰 Bug fixes 🧰

- Fix reading scope attributes for trace JSON, remove duplicate code. (#5930)
- otlpjson/trace: skip unknown fields instead of error. (#5931)
- Fix bug in setting the correct collector state after a configuration change event. (#5830)
- Fix json trace unmarshalling for numbers (#5924):
  - Accept both string and number for float64.
  - Accept both string and number for int32/uint32.
  - Read uint64 numbers without converting from int64.
- Fix persistent storage client not closing when shutting down (#6003)