---
title: opentelemetry-collector v0.17 Release Notes
description: opentelemetry-collector v0.17 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.17 Release Notes 是什么
- 如何 opentelemetry-collector v0.17 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.17
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

# opentelemetry-collector v0.17 Release Notes

Source: [v0.17.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.17.0)

# v0.17.0 Beta

## 💡 Enhancements 💡

- Default config environment variable expansion (#2231)
- `prometheusremotewrite` exporter: Add batched exports (#2249)
- `memorylimiter` processor: Introduce soft and hard limits (#2250)

## 🧰 Bug fixes 🧰

- Fix nits in pdata usage (#2235)
- Convert status to not be a pointer in the Span (#2242)
- Report the error from `pprof.StartCPUProfile` (#2263)
- Rename `service.Application.SignalTestComplete` to `Shutdown` (#2277)