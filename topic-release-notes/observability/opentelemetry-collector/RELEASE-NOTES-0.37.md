---
title: opentelemetry-collector v0.37 Release Notes
description: opentelemetry-collector v0.37 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.37 Release Notes 是什么
- 如何 opentelemetry-collector v0.37 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.37
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.37 Release Notes

Source: [v0.37.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.37.0)

## v0.37.0 Beta

## 🛑 Breaking changes 🛑

- Move `configcheck.ValidateConfigFromFactories` as internal function in service package (#3876)
- Rename `configparser.Parser` as `config.Map` (#4075)
- Rename `component.DefaultBuildInfo()` to `component.NewDefaultBuildInfo()` (#4129)
- Rename `consumererror.Permanent` to `consumererror.NewPermanent` (#4118)
- Rename `config.NewID` to `config.NewComponentID` and `config.NewIDFromString` to `config.NewComponentIDFromString` (#4137)
- Rename `config.NewIDWithName` to `config.NewComponentIDWithName` (#4151)
- Move `extension/storage` to `extension/experimental/storage` (#4082)
- Rename `obsreporttest.SetupRecordedMetricsTest()` to `obsreporttest.SetupTelemetry()` and `obsreporttest.TestTelemetrySettings` to `obsreporttest.TestTelemetry` (#4157)

## 💡 Enhancements 💡

- Add Gen dependabot into CI (#4083)
- Update OTLP to v0.10.0 (#4045).
- Add Flags field to NumberDataPoint, HistogramDataPoint, SummaryDataPoint (#4081).
- Add feature gate library (#4108)
- Add version to the internal telemetry metrics (#4140)

## 🧰 Bug fixes 🧰

- Fix panic when not using `service.NewCommand` (#4139)