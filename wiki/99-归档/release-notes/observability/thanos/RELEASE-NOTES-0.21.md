---
title: thanos v0.21 Release Notes
description: thanos v0.21 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.21 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.21 Release Notes 是什么
- 如何 thanos v0.21 Release Notes
trigger_keywords:
- thanos
- v0.21
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Thanos|thanos]] v0.21 Release Notes

Source: [v0.21.1](https://github.com/thanos-io/thanos/releases/tag/v0.21.1)

### Fixed

- [#4308](https://github.com/thanos-io/thanos/pull/4308) Sidecar: reloader: fix output config file permission

<!-- risk-assessed -->
