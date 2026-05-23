---
title: opentelemetry-collector v0.45 Release Notes
description: opentelemetry-collector v0.45 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.45 Release Notes 是什么
- 如何 opentelemetry-collector v0.45 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.45
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

# opentelemetry-collector v0.45 Release Notes

Source: [v0.45.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.45.0)

## v0.45.0 Beta

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.45.0

### 🛑 Breaking changes 🛑

- Remove deprecated funcs in configtelemetry (#4808)
- Deprecate `[[Service|service]]/defaultcomponents` go package (#4622)
- `otlphttp` and `otlp` exporters enable gzip compression by default (#4632)

### 💡 Enhancements 💡

- Reject invalid queue size exporterhelper (#4799)
- Transform configmapprovider.Retrieved interface to a struct (#4789)
- Added feature gate summary to zpages extension (#4834)
- Add support for reloading TLS certificates (#4737)

### 🚩 Deprecations 🚩

- Deprecate `pdata.NumberDataPoint.Type()` and `pdata.Exemplar.Type()` in favor of `NumberDataPoint.ValueType()` and
  `Exemplar.ValueType()` (#4850)