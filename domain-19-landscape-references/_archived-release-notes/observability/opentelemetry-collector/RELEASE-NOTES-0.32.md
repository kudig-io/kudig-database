---
title: opentelemetry-collector v0.32 Release Notes
description: opentelemetry-collector v0.32 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.32 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.32 Release Notes 是什么
- 如何 opentelemetry-collector v0.32 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.32
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- tracing-basics
- observability-basics
---



# opentelemetry-collector v0.32 Release Notes

Source: [v0.32.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.32.0)

# 🛑 IMPORTANT 🛑

This release is marked as "bad" since the metrics pipelines will produce bad data.

- See https://github.com/open-telemetry/opentelemetry-collector/issues/3824

## v0.32.0 Beta

## 🛑 Breaking changes 🛑

- Rename `CustomUnmarshable` interface to `Unmarshallable` (#3774)

## 💡 Enhancements 💡

- Change default OTLP/HTTP port number from 55681 to 4318 (#3743)
- Update OTLP proto to v0.9.0 (#3740)
  - Remove `SetValue`/`Value` func for `NumberDataPoint`/`Exemplar` (#3730)
  - Remove `IntGauge`/`IntSum`from pdata (#3731)
  - Remove `IntDataPoint` from pdata (#3735)
  - Add support for `Bytes` attribute type (#3756)
  - Add `SchemaUrl` field (#3759)
  - Add `Attributes` to `NumberDataPoint`, `HistogramDataPoint`, `SummaryDataPoint` (#3761)
- `conventions` translator: Replace with conventions generated from spec v1.5.0 (#3494)
- `[[Prometheus|prometheus]]` receiver: Add `ToMetricPdata` method (#3695)
- Make configsource `Watchable` an optional interface (#3792)
- `obsreport` exporter: Change to accept `ExporterCreateSettings` (#3789)

## 🧰 Bug fixes 🧰

- `configgrpc`: Use chained interceptors in the [[gRPC|gRPC]] server (#3744)
- `prometheus` receiver: Use actual interval startTimeMs for cumulative types (#3694)
- `[[Jaeger|jaeger]]` translator: Fix bug that could generate empty proto spans (#3808)
