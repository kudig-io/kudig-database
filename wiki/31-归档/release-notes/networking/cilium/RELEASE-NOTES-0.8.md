---
title: cilium v0.8 Release Notes
description: cilium v0.8 Release Notes — Kubernetes 生产运维知识库
summary: cilium v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cilium
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
- cilium v0.8 Release Notes 是什么
- 如何 cilium v0.8 Release Notes
trigger_keywords:
- cilium
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Cilium|cilium]] v0.8 Release Notes

Source: [v0.8.2](https://github.com/cilium/cilium/releases/tag/v0.8.2)

- Separate state directory inside runtime directory (#537)
- Fix all remaining testsuites and have Jenkins fail properly on all failures (#513)
- policy: Support carrying part of the path in the name (#533)
- Temporary fix: Set net.ipv6.conf.all.disable_ipv6=1 as Docker disables it by mistake (#544)

<!-- risk-assessed -->
