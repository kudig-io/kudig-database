---
title: opentelemetry-collector v0.82 Release Notes
description: opentelemetry-collector v0.82 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.82 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.82 Release Notes 是什么
- 如何 opentelemetry-collector v0.82 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.82
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- observability-basics
---



# opentelemetry-collector v0.82 Release Notes

Source: [v0.82.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.82.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.82.0

### 🛑 Breaking changes 🛑

- `[[Service|service]]`: Enable configuration of collector telemetry through [[Prometheus|prometheus]] reader (#7641)
  These options are still experimental. To enable them, users must enable both
  `telemetry.useOtelForInternalMetrics` and `telemetry.useOtelWithSDKConfigurationForInternalTelemetry`
  feature gates. This change updates `metric_readers` to `readers` to align with the configuration
  working group.
  
- `service`: Remove experimental `metric_readers.args` and `metric_reader.type` config options. (#7641)
  These options were experimental and did not have any effect on the configuration of
  the collector's telemetry. The change aligns the configuration with the latest iteration
  of the configuration json schema, which may still change in the future.

### 💡 Enhancements 💡

- `service`: Add support for exporting internal metrics to the console (#7641)
  Internal collector metrics can now be exported to the console
  using the otel-go stdout exporter.
  
- `service`: Add support for `interval` and `timeout` configuration in periodic reader (#7641)
- `service`: Add support for span processor configuration for internal traces (#8106)
  These options are still experimental. To enable them, users must enable both
  `telemetry.useOtelForInternalMetrics` and `telemetry.useOtelWithSDKConfigurationForInternalTelemetry`
  feature gates.
  
- `service`: Add support for OTLP export for internal metrics (#7641)
  Internal collector metrics can now be exported via OTLP
  using the otel-go otlpgrpc and otlphttp exporters.
  
- `scraperhelper`: Adding optional timeout field to scrapers (#7951)
- `otlpreceiver`: Add http url paths per signal config options to otlpreceiver (#7511)
- `otlphttpexporter`: Add support for trailing slash in endpoint URL (#8084)
  URLs like `http://localhost:4318/` will now be treated as if they were `http://localhost:4318`

### 🧰 Bug fixes 🧰

- `connector`: Fix connector validation (#7892)
  Validation of connectors was too aggressive such that a connector that was used in any combination of unsupported roles would fail.
  Instead, validation should pass as long as each use of the connector has a supported corresponding use.
