---
title: kops v1.27 Release Notes
description: kops v1.27 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.27 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.27 Release Notes 是什么
- 如何 kops v1.27 Release Notes
trigger_keywords:
- kops
- v1.27
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




# kops v1.27 Release Notes

Source: [v1.27.3](https://github.com/kubernetes/kops/releases/tag/v1.27.3)

## What's Changed
* Update aws-sdk-go to v1.49.24 by @hakman in https://github.com/kubernetes/kops/pull/16304
* Update [[containerd|containerd]] to v1.7.13 and runc to v1.1.12 by @hakman in https://github.com/kubernetes/kops/pull/16305
* Release 1.27.3 by @hakman in https://github.com/kubernetes/kops/pull/16309


**Full Changelog**: https://github.com/kubernetes/kops/compare/v1.27.2...v1.27.3

<!-- risk-assessed -->
