---
title: opentelemetry-collector v0.124 Release Notes
description: opentelemetry-collector v0.124 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.124 Release Notes 是什么
- 如何 opentelemetry-collector v0.124 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.124
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.124 Release Notes

Source: [v0.124.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.124.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.124.0

## End User Changelog

### 🐛 Known bugs 🐛

- `service/telemetry`: Missing attributes in internal Collector logs (#12870).
  - See issue description for workarounds.

### 💡 Enhancements 💡

- `exporterhelper`: Add support for bytes-based batching for profiles in the exporterhelper package. (#3262)
- `otelcol`: Enhance config validation using <validate> command to capture all validation errors that prevents the collector from starting. (#8721)
- `exporterhelper`: Link batcher context to all batched request's span contexts. (#12212, #8122)

### 🧰 Bug fixes 🧰

- `confighttp`: Ensure http authentication server failures are handled by the provided error handler (#12666)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `exporterbatcher`: Remove deprecated package exporterbatcher (#12780)
- `exporterqueue`: Remove deprecated package exporterqueue (#12779)

### 💡 Enhancements 💡

- `mdatagen`: Add variable for metric name in mdatagen (#12459)
  Access metric name via `metadata.MetricsInfo.<metric-variable>.Name`
- `client`: Add support for iterating over client metadata keys (#12804)
- `service`: Adds the GetFactory interface to the hostcapabilities package (#12789)
- `cmd/mdatagen`: Add the foundational changes necessary for supporting logs data in `mdatagen` (#12571)

<!-- previous-version -->
