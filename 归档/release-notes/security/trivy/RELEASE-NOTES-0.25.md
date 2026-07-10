---
title: trivy v0.25 Release Notes
description: trivy v0.25 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.25 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.25 Release Notes 是什么
- 如何 trivy v0.25 Release Notes
trigger_keywords:
- trivy
- v0.25
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




# [[Trivy|trivy]] v0.25 Release Notes

Source: [v0.25.4](https://github.com/aquasecurity/trivy/releases/tag/v0.25.4)

## Changelog
* b4a7d6a8 docs: move CONTRIBUTING.md to docs (#1971)
* 0127c1d3 refactor(table): use file name instead package path (#1966)
* a92da722 fix(sbom): add --db-repository (#1964)
* b0f3864e feat(table): add PkgPath in table result (#1960)
* 0b1d32c1 fix(pom): merge multiple pom imports in a good manner (#1959)



<!-- risk-assessed -->
