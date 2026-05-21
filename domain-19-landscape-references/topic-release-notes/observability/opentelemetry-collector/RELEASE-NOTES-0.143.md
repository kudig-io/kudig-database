---
title: opentelemetry-collector v0.143 Release Notes
description: opentelemetry-collector v0.143 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.143 Release Notes 是什么
- 如何 opentelemetry-collector v0.143 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.143
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.143 Release Notes

Source: [v0.143.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.143.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.143.0

## End User Changelog

### 💡 Enhancements 💡

- `all`: Update semconv import to 1.38.0 (#14305)
- `exporter/nop`: Add profiles support to nop exporter (#14331)
- `pkg/pdata`: Optimize the size and pointer bytes for pdata structs (#14339)
- `pkg/pdata`: Avoid using interfaces/oneof like style for optional fields (#14333)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `pkg/xprocessor`: Use pointer receivers in xprocessor factory methods for consistency with other factories. (#14348)

<!-- previous-version -->
