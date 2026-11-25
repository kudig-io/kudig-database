---
title: loki v3.0 Release Notes
description: loki v3.0 Release Notes — Kubernetes 生产运维知识库
summary: loki v3.0 Release Notes — Kubernetes 生产运维知识库
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
- loki v3.0 Release Notes 是什么
- 如何 loki v3.0 Release Notes
trigger_keywords:
- loki
- v3.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# loki v3.0 Release Notes

Source: [v3.0.1](https://github.com/grafana/loki/releases/tag/v3.0.1)

## [3.0.1](https://github.com/grafana/loki/compare/v3.0.0...v3.0.1) (2024-08-09)


### Bug Fixes

* **deps:** bumped dependencies versions to resolve CVEs ([#13833](https://github.com/grafana/loki/pull/13833)) ([e13011d](https://github.com/grafana/loki/commit/e13011d91a77501ca4f659df9cf33f23085d3a35))
* Fix nil pointer dereference in bloomstore initialisation ([#12869](https://github.com/grafana/loki/issues/12869)) ([167b468](https://github.com/grafana/loki/commit/167b468598bc70bbed6eed44826d3f9b85e1e0b8)), closes [#12270](https://github.com/grafana/loki/issues/12270)

<!-- risk-assessed -->
