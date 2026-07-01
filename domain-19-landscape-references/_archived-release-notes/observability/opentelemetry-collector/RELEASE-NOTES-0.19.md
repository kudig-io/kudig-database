---
title: opentelemetry-collector v0.19 Release Notes
description: opentelemetry-collector v0.19 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.19 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- jaeger
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.19 Release Notes 是什么
- 如何 opentelemetry-collector v0.19 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.19
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tracing-basics
- observability-basics
---



# opentelemetry-collector v0.19 Release Notes

Source: [v0.19.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.19.0)

# v0.19.0 Beta

## 🛑 Breaking changes 🛑
- Remove deprecated `queued_retry` processor
- Remove deprecated configs from `resource` processor: `type` (set "opencensus.type" key in "attributes.upsert" map instead) and `labels` (use "attributes.upsert" instead).

## 💡 Enhancements 💡

- `hostmetrics` receiver: Refactor load metrics to use generated metrics (#2375)
- Add uptime to the servicez debug page (#2385)
- Add new semantic conventions for AWS (#2365)

## 🧰 Bug fixes 🧰

- `[[Jaeger|jaeger]]` exporter: Improve connection state logging (#2239)
- `pdatagen`: Fix slice of values generated code (#2403)
- `filterset` processor: Avoid returning always nil error in strict filterset (#2399)