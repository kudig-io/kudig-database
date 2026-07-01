---
title: opentelemetry-collector v0.0 Release Notes
description: opentelemetry-collector v0.0 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- jaeger
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.0 Release Notes 是什么
- 如何 opentelemetry-collector v0.0 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- tracing-basics
- observability-basics
---



# opentelemetry-collector v0.0 Release Notes

Source: [v0.0.2](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.0.2)

Alpha release of [[OpenTelemetry|OpenTelemetry]] [[Service|Service]].

Docker image: omnition/opentelemetry-service:v0.0.2 (we are working on getting this under an OpenTelemetry org)

Main changes visible to users since previous release:

```terminal
8fa6afe Translate OC resource labels to Jaeger process tags (#325)
047b0f3 Allow environment variables in config (#334)
96c24a3 Add exclude/include spans option to attributes processor (#311)
4db0414 Allow metric processors to be specified in pipelines (#332)
c277569 Add observability instrumentation for Prometheus receiver (#327)
f47aa79 Add common configuration for receiver tls (#288)
a493765 Refactor extensions to new config format (#310)
41a7afa Add Span Processor logic
97a71b3 Use full name for the metrics and spans created for observability (#316)
fed4ed2 Add support to record metrics for metricsexporter (#315)
5edca32 Add include_filter configuration to prometheus receiver (#298)
0068d0a Passthrough CORS allowed origins (#260)
```