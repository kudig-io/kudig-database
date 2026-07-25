---
title: opentelemetry-collector v0.83 Release Notes
description: opentelemetry-collector v0.83 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.83 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.83 Release Notes 是什么
- 如何 opentelemetry-collector v0.83 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.83
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




# opentelemetry-collector v0.83 Release Notes

Source: [v0.83.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.83.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.83.0

## User Facing Changes

### 💡 Enhancements 💡

- `extension`: Add optional `ConfigWatcher` interface (#6596)
  Extensions implementing this interface will be notified of the Collector's effective config.
- `otelcol`: Add optional `ConfmapProvider` interface for Config Providers (#6596)
  This allows providing the Collector's configuration as a marshaled confmap.Conf object
  from a ConfigProvider
  
- `[[Service|service]]`: Add `CollectorConf` field to `service.Settings` (#6596)
  This field is intended to be used by the Collector to pass its effective configuration to the service.

## Go API Changes


### 🛑 Breaking changes 🛑

- `all`: Remove go 1.19 support, bump minimum to go 1.20 and add testing for 1.21 (#8207)

### 💡 Enhancements 💡

- `changelog`: Generate separate changelogs for end users and package consumers (#8153)

<!-- risk-assessed -->
