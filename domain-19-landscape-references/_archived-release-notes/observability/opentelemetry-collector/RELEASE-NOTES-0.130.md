---
title: opentelemetry-collector v0.130 Release Notes
description: opentelemetry-collector v0.130 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.130 Release Notes 是什么
- 如何 opentelemetry-collector v0.130 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.130
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.130 Release Notes

Source: [v0.130.1](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.130.1)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.130.1

## End User Changelog

### 🧰 Bug fixes 🧰

- `[[Service|service]]`: Fixes bug where internal metrics are emitted with an unexpected suffix in their names when users configure `service::telemetry::metrics::readers` with [[Prometheus|Prometheus]]. (#13449)
  See more details on https://github.com/open-telemetry/opentelemetry-go/issues/7039

<!-- previous-version -->

## API Changelog

<!-- previous-version -->
