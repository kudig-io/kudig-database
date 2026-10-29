---
title: opentelemetry-collector v0.135 Release Notes
description: opentelemetry-collector v0.135 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.135 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.135 Release Notes 是什么
- 如何 opentelemetry-collector v0.135 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.135
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




# opentelemetry-collector v0.135 Release Notes

Source: [v0.135.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.135.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.135.0

## End User Changelog

### 💡 Enhancements 💡

- `exporterhelper`: Add new `exporter_queue_batch_send_size` and `exporter_queue_batch_send_size_bytes` metrics, showing the size of telemetry batches from the exporter. (#12894)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `pdata/pprofile`: Remove deprecated AddAttribute method (#13764)

### 💡 Enhancements 💡

- `configmiddleware`: Stabilize `configmiddleware` module (#13422)
  This only stabilizes the configuration interface but does not stabilize the middlewares themselves or the way of implementing them.
- `xpdata`: Add experimental MapBuilder struct to optimize pcommon.Map construction (#13617)

<!-- previous-version -->


<!-- risk-assessed -->
