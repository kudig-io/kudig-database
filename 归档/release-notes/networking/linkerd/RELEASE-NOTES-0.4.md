---
title: linkerd v0.4 Release Notes
description: linkerd v0.4 Release Notes — Kubernetes 生产运维知识库
summary: linkerd v0.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- linkerd v0.4 Release Notes 是什么
- 如何 linkerd v0.4 Release Notes
trigger_keywords:
- linkerd
- v0.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Linkerd|linkerd]] v0.4 Release Notes

Source: [v0.4.4](https://github.com/linkerd/linkerd2/releases/tag/v0.4.4)

## v0.4.4

Conduit v0.4.4 continues to improve production suitability and sets up internals for the
upcoming v0.5.0 release.

* Production Readiness
  * The destination [[Service|service]] has been mostly-rewritten to improve safety and correctness,
    especially during controller initialization.
  * Readiness and Liveness checks have been added for some controller components.
  * RBAC settings have been expanded so that [[Prometheus|Prometheus]] can access node-level metrics.
* User Interface
  * Ad blockers like uBlock prevented the Conduit dashboard from fetching API data. This
    has been fixed.
  * The UI now highlights pods that have failed to start a proxy.
* Internals
  * Various dependency upgrades, including Rust 1.26.2.
  * TLS testing continues to bear fruit, precipitating stability improvements to
    dependencies like Rustls.

Special thanks to @alenkacz for improving docker build times!

<!-- risk-assessed -->
