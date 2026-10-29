---
title: grafana v4.5 Release Notes
description: grafana v4.5 Release Notes — Kubernetes 生产运维知识库
summary: grafana v4.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v4.5 Release Notes 是什么
- 如何 grafana v4.5 Release Notes
trigger_keywords:
- grafana
- v4.5
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




# grafana v4.5 Release Notes

Source: [v4.5.2](https://github.com/grafana/grafana/releases/tag/v4.5.2)

[Download Page](https://grafana.com/grafana/download/4.5.2)
[Installation Guide](http://docs.grafana.org/installation/)

[Release Notes](https://community.grafana.com/t/release-notes-for-grafana-v4-5-0/2573)

## Fixes 
* **Graphite**: Fix for issues with jsonData & graphiteVersion null errors [#9258](https://github.com/grafana/grafana/issues/9258)
* **Graphite**: Fix for Grafana internal metrics to Graphite sending NaN values [#9279](https://github.com/grafana/grafana/issues/9279)
* **HTTP API**: Fix for HEAD method requests [#9307](https://github.com/grafana/grafana/issues/9307)
* **Templating**: Fix for duplicate template variable queries when refresh is set to time range change [#9185](https://github.com/grafana/grafana/issues/9185)
* **Metrics**: dont write NaN values to graphite [#9279](https://github.com/grafana/grafana/issues/9279)


<!-- risk-assessed -->
