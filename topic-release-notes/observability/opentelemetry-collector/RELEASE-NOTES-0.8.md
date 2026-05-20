---
title: opentelemetry-collector v0.8 Release Notes
description: opentelemetry-collector v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.8 Release Notes 是什么
- 如何 opentelemetry-collector v0.8 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.8
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.8 Release Notes

Source: [v0.8.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.8.0)

# v0.8.0 Beta

# 🚀 New components 🚀

- `groupbytrace` processor that waits for a trace to be completed (#1362)

# 💡 Enhancements 💡

- Migrate `zipkin` receiver/exporter to the new interfaces (#1484)
- Migrate `prometheus` receiver/exporter to the new interfaces (#1477, #1515)
- Add new FactoryUnmarshaler support to all components, deprecate old way (#1468)
- Update `fileexporter` to write data in OTLP (#1488)
- Add extension factory helper (#1485)
- Host scrapers: Use same scrape time for all data points coming from same source (#1473)
- Make logs SeverityNumber publicly available (#1496)
- Add recently included conventions for k8s and container resources (#1519)
- Add new config StartTimeMetricRegex to `prometheus` receiver (#1511)
- Convert Zipkin receiver and exporter to use OTLP (#1446)

# 🧰 Bug fixes 🧰

- Infer OpenCensus resource type based on OpenTelemetry's semantic conventions (#1462)
- Fix log adapter in `prometheus` receiver (#1493)
- Avoid frequent errors for process telemetry on Windows (#1487)
