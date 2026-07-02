---
title: helm v1.2 Release Notes
description: helm v1.2 Release Notes — Kubernetes 生产运维知识库
summary: helm v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
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
- helm v1.2 Release Notes 是什么
- 如何 helm v1.2 Release Notes
trigger_keywords:
- helm
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Helm|helm]] v1.2 Release Notes

Source: [v1.2.1](https://github.com/helm/helm/releases/tag/v1.2.1)

This revision of the 1.2 release fixes a bug that causes make to fail when building and pushing the docker images for the dm server side components. The binaries for this release are pushed to gcr.io/dm-k8s-prod with tag v1.2.1 and to gcr.io/get-dm.


<!-- risk-assessed -->
