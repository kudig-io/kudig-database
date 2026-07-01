---
title: opentelemetry-collector v0.134 Release Notes
description: opentelemetry-collector v0.134 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.134 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.134 Release Notes 是什么
- 如何 opentelemetry-collector v0.134 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.134
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.134 Release Notes

Source: [v0.134.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.134.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.134.0

## End User Changelog

### 💡 Enhancements 💡

- `pdata`: Add custom [[gRPC|grpc]]/encoding that replaces proto and calls into the custom marshal/unmarshal logic in pdata. (#13631)
  This change should not affect other gRPC calls since it fallbacks to the default grpc/proto encoding if requests are not pdata/otlp requests.
- `pdata`: Avoid copying the pcommon.Map when same origin (#13731)
  This is a very large improvement if using OTTL with map functions since it will avoid a map copy.
- `exporterhelper`: Respect `num_consumers` when batching and partitioning are enabled. (#13607)

### 🧰 Bug fixes 🧰

- `pdata`: Correctly parse OTLP payloads containing non-packed repeated primitive fields (#13727, #13730)
  This bug prevented the Collector from ingesting most Histogram, ExponentialHistogram,
  and Profile payloads.
  

<!-- previous-version -->

## API Changelog

### 💡 Enhancements 💡

- `exporterhelper`: Split exporterhelper into a separate module (#12985)

<!-- previous-version -->
