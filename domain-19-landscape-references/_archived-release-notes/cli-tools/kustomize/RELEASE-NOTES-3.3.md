---
title: kustomize v3.3 Release Notes
description: kustomize v3.3 Release Notes — Kubernetes 生产运维知识库
summary: kustomize v3.3 Release Notes — Kubernetes 生产运维知识库
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
- kustomize v3.3 Release Notes 是什么
- 如何 kustomize v3.3 Release Notes
trigger_keywords:
- kustomize
- v3.3
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




# kustomize v3.3 Release Notes

Source: [v3.3.1](https://github.com/kubernetes-sigs/kustomize/releases/tag/v3.3.1)

Test of new API goreleaser-driven release process.  LGTM.

Ignore the assets, as there's just a binary that prints the API version number.
The important thing with this release is that one may
```
require sigs.k8s.io/kustomize/v3 v3.3.1
```
from your `go.mod` file.

## Changelog

78d14d0d Introduce dummy program to help with API releases.
40ed9e6a fix zh-doc
3cf6b8ec v3.3.0 release notes
281f9328 zh example:chart,secret generator plugin



<!-- risk-assessed -->
