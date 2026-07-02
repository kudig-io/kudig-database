---
title: trivy v0.21 Release Notes
description: trivy v0.21 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.21 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.21 Release Notes 是什么
- 如何 trivy v0.21 Release Notes
trigger_keywords:
- trivy
- v0.21
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




# [[Trivy|trivy]] v0.21 Release Notes

Source: [v0.21.3](https://github.com/aquasecurity/trivy/releases/tag/v0.21.3)

## Changelog

8e57dee8 fix(docs): typo (#1488)
8bfbc84a feat(plugin): Add option to update plugin (#1462)
1e811de2 fix: fixed skipFiles/skipDirs flags for relative path (#1482)
8b5796f7 feat (plugin): add list and info command for plugin (#1452)
a2199bb4 fix: set up a vulnerability severity (#1458)
279e76f7 chore: add arm64 deb package (#1480)
52625908 Link to trivy tutorial on Semaphore (#1449)
c275a841 refactor([[Helm|helm]]): externalize env vars to configMap (#1345)


## Docker images

- `docker pull aquasec/trivy:0.21.3`
- `docker pull ghcr.io/aquasecurity/trivy:0.21.3`
- `docker pull public.ecr.aws/aquasecurity/trivy:0.21.3`


<!-- risk-assessed -->
