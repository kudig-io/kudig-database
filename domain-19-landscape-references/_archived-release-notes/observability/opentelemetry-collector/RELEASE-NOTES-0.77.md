---
title: opentelemetry-collector v0.77 Release Notes
description: opentelemetry-collector v0.77 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.77 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.77 Release Notes 是什么
- 如何 opentelemetry-collector v0.77 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.77
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




# opentelemetry-collector v0.77 Release Notes

Source: [v0.77.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.77.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.77.0

### 🛑 Breaking changes 🛑

- `exporterhelper`: Reduce the default queue size to 1000 from 5000 (#7359)
  Affects any exporter which enables the queue by default and doesn't set its own default size.
  For example: otlphttp.
  
- `featuregate`: Remove deprecated `RemovalVersion` and `WithRegisterRemovalVersion` functions. (#7587)

### 💡 Enhancements 💡

- `[[Service|service]]`: Adds ResourceAttributes map to telemetry settings and thus CreateSettings. (#6599)
- `service`: Allows users to disable high cardinality OTLP attributes behind a feature flag. (#7517)
- `featuregate`: Finalize purpose of `toVersion`.  Allow stable gates to be explicitly set to true, but produce a warning log. (#7626)

### 🧰 Bug fixes 🧰

- `config/confighttp`: Ensure Auth RoundTripper follows compression/header changes (#7574)
- `otlpreceiver`: do not reject requests having 'content-type' header with optional parameters (#7452)

<!-- risk-assessed -->
