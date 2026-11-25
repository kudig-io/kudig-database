---
title: cri-o v0.2 Release Notes
description: cri-o v0.2 Release Notes — Kubernetes 生产运维知识库
summary: cri-o v0.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cri-o
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cri-o v0.2 Release Notes 是什么
- 如何 cri-o v0.2 Release Notes
trigger_keywords:
- cri-o
- v0.2
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




# cri-o v0.2 Release Notes

Source: [v0.2](https://github.com/cri-o/cri-o/releases/tag/v0.2)

With this release, we have made good progress on passing the node conformance tests. 
 
Highlights of the release:

1. Logging support
2. 115/121 (95%) node conformance tests pass  (https://github.com/kubernetes-incubator/cri-o/issues/441)
3. gpg check on image pull
4. Lots of bug fixes
5. Supports latest runc v1.0.0-rc3 and runtime-spec v1.0.0-rc5


Features that don't work yet:
Streaming (exec), attach and port forward.


<!-- risk-assessed -->
