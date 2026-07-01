---
title: opentelemetry-collector v0.127 Release Notes
description: opentelemetry-collector v0.127 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.127 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.127 Release Notes 是什么
- 如何 opentelemetry-collector v0.127 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.127
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---



# opentelemetry-collector v0.127 Release Notes

Source: [v0.127.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.127.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.127.0

## End User Changelog

### 🚩 Deprecations 🚩

- `semconv`: Deprecating the semconv package in favour of go.[[OpenTelemetry|opentelemetry]].io/otel/semconv (#13012)

### 💡 Enhancements 💡

- `exporter/debug`: Display resource and scope in `normal` verbosity (#10515)
- `service`: Add size metrics defined in Pipeline Component Telemetry RFC (#13032)
  See [Pipeline Component Telemetry RFC](https://github.com/open-telemetry/opentelemetry-collector/blob/main/docs/rfcs/component-universal-telemetry.md) for more details:
    - `otelcol.receiver.produced.size`
    - `otelcol.processor.consumed.size`
    - `otelcol.processor.produced.size`
    - `otelcol.connector.consumed.size`
    - `otelcol.connector.produced.size`
    - `otelcol.exporter.consumed.size`
  

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `mdatagen`: Add context parameter for recording event to set traceID and spanID (#12571)
- `otlpreceiver`: Use wrapper type for URL paths (#13046)

### 🚩 Deprecations 🚩

- `pipeline`: Deprecate MustNewID and MustNewIDWithName (#12831)
- `pdata/profile`: Replace AddAttribute with the PutAttribute helper method to modify the content of attributable records. (#12798)

### 💡 Enhancements 💡

- `consumer/consumertest`: Add context to sinks (#13039)
- `cmd/mdatagen`: Add events in generated documentation (#12571)
- `confmap`: Add a `Conf.Delete` method to remove a path from the configuration map. (#13064)
- `confmap`: Support running Unmarshal hooks on nil values. (#12981)

<!-- previous-version -->
