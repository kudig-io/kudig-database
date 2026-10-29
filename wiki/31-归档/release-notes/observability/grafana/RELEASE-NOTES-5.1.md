---
title: grafana v5.1 Release Notes
description: grafana v5.1 Release Notes — Kubernetes 生产运维知识库
summary: grafana v5.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
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
- grafana v5.1 Release Notes 是什么
- 如何 grafana v5.1 Release Notes
trigger_keywords:
- grafana
- v5.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# grafana v5.1 Release Notes

Source: [v5.1.5](https://github.com/grafana/grafana/releases/tag/v5.1.5)

[Download Page](https://grafana.com/grafana/download/5.1.5)
[Installation Guide](http://docs.grafana.org/installation/)

[Release Notes](https://community.grafana.com/t/release-notes-v-5-1-x/6958)

* **Docker**: Config keys ending with _FILE are not respected [#170](https://github.com/grafana/grafana-docker/issues/170)

<!-- risk-assessed -->
