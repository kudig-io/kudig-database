---
title: trivy v0.18 Release Notes
description: trivy v0.18 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.18 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.18 Release Notes 是什么
- 如何 trivy v0.18 Release Notes
trigger_keywords:
- trivy
- v0.18
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




# [[Trivy|trivy]] v0.18 Release Notes

Source: [v0.18.3](https://github.com/aquasecurity/trivy/releases/tag/v0.18.3)

## Changelog

85e45ca chore(ci): change to more granular tokens (#1014)
9fa512a chore(ci): add Go scanning and update dependencies (#1001)
349371b docs: Add HIGH severity to Trivy command in GitLab CI example to match comment (#1013)


## Docker images

- `docker pull aquasec/trivy:0.18.3`
- `docker pull ghcr.io/aquasecurity/trivy:0.18.3`
- `docker pull public.ecr.aws/aquasecurity/trivy:0.18.3`
- `docker pull aquasec/trivy:latest`
- `docker pull ghcr.io/aquasecurity/trivy:latest`
- `docker pull public.ecr.aws/aquasecurity/trivy:latest`


<!-- risk-assessed -->
