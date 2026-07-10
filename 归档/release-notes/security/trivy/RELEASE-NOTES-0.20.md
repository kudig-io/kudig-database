---
title: trivy v0.20 Release Notes
description: trivy v0.20 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.20 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- trivy v0.20 Release Notes 是什么
- 如何 trivy v0.20 Release Notes
trigger_keywords:
- trivy
- v0.20
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




# [[Trivy|trivy]] v0.20 Release Notes

Source: [v0.20.2](https://github.com/aquasecurity/trivy/releases/tag/v0.20.2)

## Changelog

5dc8cfe docs: update builtin.md (#1335)
798b564 chore: fix issues with Homebrew formula (#1329)
21bf5e5 chore: bump GoReleaser to v0.183.0 (#1328)
e0f4ebd docs: update iac.md for a typo (#1326)
23a9a5e docs: typo fix (#1308)
1f5d17f Add new networking API features to [[Ingress|Ingress]] (#1262)


## Docker images

- `docker pull aquasec/trivy:0.20.2`
- `docker pull ghcr.io/aquasecurity/trivy:0.20.2`
- `docker pull public.ecr.aws/aquasecurity/trivy:0.20.2`


<!-- risk-assessed -->
