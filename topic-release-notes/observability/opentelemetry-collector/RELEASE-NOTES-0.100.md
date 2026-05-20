---
title: opentelemetry-collector v0.100 Release Notes
description: opentelemetry-collector v0.100 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.100 Release Notes 是什么
- 如何 opentelemetry-collector v0.100 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.100
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.100 Release Notes

Source: [v0.100.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.100.0)

## End User Changelog

### 🛑 Breaking changes 🛑

- `service`: The `validate` sub-command no longer validates that each pipeline's type is the same as its component types (#10031)

### 💡 Enhancements 💡

- `semconv`: Add support for v1.25.0 semantic convention (#10072)
- `builder`: remove the need to go get a module to address ambiguous import paths (#10015)
- `pmetric`: Support parsing metric.metadata from OTLP JSON. (#10026)

### 🧰 Bug fixes 🧰

- `exporterhelper`: Fix enabled config option for batch sender (#10076)

## Go API Changelog

This changelog includes only developer-facing changes.
If you are looking for user-facing changes, check out [CHANGELOG.md](./CHANGELOG.md).

<!-- next version -->

### 💡 Enhancements 💡

- `configgrpc`: Adds `NewDefault*` functions for all the config structs. (#9654)
- `exporterqueue`: Expose ErrQueueIsFull so upstream components can retry or apply backpressure. (#10070)

### 🧰 Bug fixes 🧰

- `mdatagen`: Call connectors with routers to be the same as the service graph (#10079)

## Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.100.0