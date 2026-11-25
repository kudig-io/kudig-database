---
title: grafana v5.0 Release Notes
description: grafana v5.0 Release Notes — Kubernetes 生产运维知识库
summary: grafana v5.0 Release Notes — Kubernetes 生产运维知识库
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
- grafana v5.0 Release Notes 是什么
- 如何 grafana v5.0 Release Notes
trigger_keywords:
- grafana
- v5.0
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




# grafana v5.0 Release Notes

Source: [v5.0.4](https://github.com/grafana/grafana/releases/tag/v5.0.4)

[Download Page](https://grafana.com/grafana/download/5.0.4)
[Installation Guide](http://docs.grafana.org/installation/)

[Release Notes](https://community.grafana.com/t/release-notes-v5-0-x/5250)

* **Docker** Can't start Grafana on [[Kubernetes|Kubernetes]] 1.7.14, 1.8.9, or 1.9.4 [#140 in grafana-docker repo](https://github.com/grafana/grafana-docker/issues/140) thx [@suquant](https://github.com/suquant)
* **Dashboard** Fixed bug where collapsed panels could not be directly linked to/renderer [#11114](https://github.com/grafana/grafana/issues/11114) & [#11086](https://github.com/grafana/grafana/issues/11086) & [#11296](https://github.com/grafana/grafana/issues/11296)
* **Dashboard** Provisioning dashboard with alert rules should create alerts [#11247](https://github.com/grafana/grafana/issues/11247)
* **Snapshots** For snapshots, the Graph panel renders the legend incorrectly on right hand side [#11318](https://github.com/grafana/grafana/issues/11318)
* **Alerting** Link back to Grafana returns wrong URL if root_path contains sub-path components [#11403](https://github.com/grafana/grafana/issues/11403)
* **Alerting** Incorrect default value for upload images setting for alert notifiers [#11413](https://github.com/grafana/grafana/pull/11413)

<!-- risk-assessed -->
