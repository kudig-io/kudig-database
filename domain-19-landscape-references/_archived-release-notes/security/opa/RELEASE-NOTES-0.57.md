---
title: opentelemetry-collector v0.57 Release Notes
description: opentelemetry-collector v0.57 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.57 Release Notes 是什么
- 如何 opentelemetry-collector v0.57 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.57
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

# opentelemetry-collector v0.57 Release Notes

Source: [v0.57.2](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.57.2)


### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.57.2

### 🛑 Breaking changes 🛑

- Remove deprecated funcs/types from [[Service|service]] related to `Config` (#5755)
- Change`confighttp.ToClient` to accept a `component.Host` (#5737)
- Remove deprecated funcs from pdata related to mutable slices (#5754)
- Change the following deprecated component functions to ensure a stability level is set:
  - `component.WithTracesExporter`
  - `component.WithMetricsExporter`
  - `component.WithLogsExporter`
  - `component.WithTracesReceiver`
  - `component.WithMetricsReceiver`
  - `component.WithLogsReceiver`
  - `component.WithTracesProcessor`
  - `component.WithMetricsProcessor`
  - `component.WithLogsProcessor`

### 🚩 Deprecations 🚩

- Deprecated the current Flag API.  The new API provides functions to check and set Flags (#5790) (#5602):
  - `NumberDataPoint.Flags` -> `NumberDataPoint.FlagsStruct`
  - `NumberDataPoint.SetFlags` -> `NumberDataPoint.FlagsStruct`
  - `HistogramDataPoint.Flags` -> `HistogramDataPoint.FlagsStruct`
  - `HistogramDataPoint.SetFlags` -> `HistogramDataPoint.FlagsStruct`
  - `ExponentialHistogramDataPoint.Flags` -> `ExponentialHistogramDataPoint.FlagsStruct`
  - `ExponentialHistogramDataPoint.SetFlags` -> `ExponentialHistogramDataPoint.FlagsStruct`
  - `SummaryDataPoint.Flags` -> `SummaryDataPoint.FlagsStruct`
  - `SummaryDataPoint.SetFlags` -> `SummaryDataPoint.FlagsStruct`
  - `MetricDataPointFlags` -> `MetricDataPointFlagsStruct`
  - `NewMetricDataPointFlags` -> `NewMetricDataPointFlagsStruct`
  - `MetricDataPointFlagsNone` -> `MetricDataPointFlagsStruct.NoRecordedValue`
  - `MetricDataPointFlagNoRecordedValue` -> `MetricDataPointFlagsStruct.NoRecordedValue`
  - `MetricDataPointFlag`
- Deprecate the following component functions added to ensure a stability level is set:
  - `component.WithTracesExporterAndStabilityLevel` -> `component.WithTracesExporter`
  - `component.WithMetricsExporterAndStabilityLevel` -> `component.WithMetricsExporter`
  - `component.WithLogsExporterAndStabilityLevel` -> `component.WithLogsExporter`
  - `component.WithTracesReceiverAndStabilityLevel` -> `component.WithTracesReceiver`
  - `component.WithMetricsReceiverAndStabilityLevel` -> `component.WithMetricsReceiver`
  - `component.WithLogsReceiverAndStabilityLevel` -> `component.WithLogsReceiver`
  - `component.WithTracesProcessorAndStabilityLevel` -> `component.WithTracesProcessor`
  - `component.WithMetricsProcessorAndStabilityLevel` -> `component.WithMetricsProcessor`
  - `component.WithLogsProcessorAndStabilityLevel` -> `component.WithLogsProcessor`
  - 

### 💡 Enhancements 💡

- Make the in-memory and persistent queues more consistent (#5764)ś
- `ocb` now exits with an error if it fails to load the build configuration. (#5731)
- Deprecate `HTTPClientSettings.ToClientWithHost` (#5737)

### 🧰 Bug fixes 🧰

- Fix bug in ocb where flags did not take precedence. (#5726)