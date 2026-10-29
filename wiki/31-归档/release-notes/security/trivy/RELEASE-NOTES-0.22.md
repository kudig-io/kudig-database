---
title: trivy v0.22 Release Notes
description: trivy v0.22 Release Notes — Kubernetes 生产运维知识库
summary: trivy v0.22 Release Notes — Kubernetes 生产运维知识库
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
- trivy v0.22 Release Notes 是什么
- 如何 trivy v0.22 Release Notes
trigger_keywords:
- trivy
- v0.22
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




# [[Trivy|trivy]] v0.22 Release Notes

Source: [v0.22.0](https://github.com/aquasecurity/trivy/releases/tag/v0.22.0)

## Changelog

42f795fa fix(java/pom): ignore unsupported requirements (#1514)
8f737cc6 feat(cli): warning for root command (#1516)
76249bdc BREAKING: disable JAR detection in fs/repo scanning (#1512)
59957d4c feat(scan): support --offline-scan option (#1511)
da8b72d2 fix: improve memory usage (#1509)
b713ad0f feat(java): support pom.xml (#1501)
56115e9d docs: fixing rust link to security advisory (#1504)
7f859afa Add missing IacMetdata (#1505)
628a7964 feat(jar): add file path (#1498)
82fba771 feat(rpm): support NDB (#1497)
d5269da5 feat: added misconfiguration field for html.tpl (#1444)


## Docker images

- `docker pull aquasec/trivy:0.22.0`
- `docker pull ghcr.io/aquasecurity/trivy:0.22.0`
- `docker pull public.ecr.aws/aquasecurity/trivy:0.22.0`


<!-- risk-assessed -->
