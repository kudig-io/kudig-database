---
title: opentelemetry-collector v0.20 Release Notes
description: opentelemetry-collector v0.20 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.20 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- opentelemetry-collector v0.20 Release Notes 是什么
- 如何 opentelemetry-collector v0.20 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.20
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




# opentelemetry-collector v0.20 Release Notes

Source: [v0.20.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.20.0)

## v0.20.0 Beta

## 🛑 Breaking changes 🛑

- Move `samplingprocessor/probabilisticsamplerprocessor` to `probabilisticsamplerprocessor` (#2392), affects only user who import the code.

## 💡 Enhancements 💡

- `hostmetrics` receiver: Refactor to use metrics metadata utilities (#2405, #2406, #2421)
- Add k8s.node semantic conventions (#2425)

## Note
As a precautionary measure against the [codecov incident](https://about.codecov.io/security-update/), we've rebuilt the binaries, packages and docker images for this release. Please update your builds and checksums.


<!-- risk-assessed -->
