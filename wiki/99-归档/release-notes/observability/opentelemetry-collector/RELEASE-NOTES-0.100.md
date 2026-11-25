---
title: opentelemetry-collector v0.100 Release Notes
description: opentelemetry-collector v0.100 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.100 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.100 Release Notes 是什么
- 如何 opentelemetry-collector v0.100 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.100
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.100 Release Notes

Source: [v0.100.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.100.0)

## End User Changelog

### 🛑 Breaking changes 🛑

- `[[Service|service]]`: The `validate` sub-command no longer validates that each pipeline's type is the same as its component types (#10031)

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

<!-- risk-assessed -->
