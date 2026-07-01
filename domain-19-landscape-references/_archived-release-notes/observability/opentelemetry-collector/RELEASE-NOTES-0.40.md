---
title: opentelemetry-collector v0.40 Release Notes
description: opentelemetry-collector v0.40 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.40 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.40 Release Notes 是什么
- 如何 opentelemetry-collector v0.40 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.40
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
- observability-basics
---



# opentelemetry-collector v0.40 Release Notes

Source: [v0.40.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.40.0)

## 🛑 Breaking changes 🛑

- Package `client` refactored (#4416) and auth data included in it (#4422). Final PR to be merged in the next release (#4423)
- Remove `pdata.AttributeMap.InitFromMap` (#4429)
- Updated configgrpc `ToDialOptions` to support passing providers to instrumentation library (#4451)
- Make state information propagation non-blocking on the collector (#4460)

## 💡 Enhancements 💡

- Add semconv 1.7.0 and 1.8.0 (#4452)
- Added `feature-gates` CLI flag for controlling feature gate state. (#4368)

Note: the [[OpenTelemetry|OpenTelemetry]] Collector Builder has its own GitHub release: [cmd/builder/v0.40.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/cmd%2Fbuilder%2Fv0.40.0)

Images and binaries: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.40.0