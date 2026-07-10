---
title: prometheus v0.17 Release Notes
description: prometheus v0.17 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v0.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v0.17 Release Notes 是什么
- 如何 prometheus v0.17 Release Notes
trigger_keywords:
- prometheus
- v0.17
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Prometheus|prometheus]] v0.17 Release Notes

Source: [0.17.0](https://github.com/prometheus/prometheus/releases/tag/0.17.0)

This version no longer works with Alertmanager 0.0.4 and earlier!
The alerting rule syntax has changed as well but the old syntax is supported
up until version 0.18.

All regular expressions in PromQL are anchored now, matching the behavior of
regular expressions in config files.
- [CHANGE] Integrate with Alertmanager 0.1.0 and higher
- [CHANGE] Degraded storage mode renamed to rushed mode
- [CHANGE] New alerting rule syntax
- [CHANGE] Add label validation on ingestion
- [CHANGE] Regular expression matchers in PromQL are anchored
- [FEATURE] Add `without` aggregation modifier
- [FEATURE] Send alert resolved notifications to Alertmanager
- [FEATURE] Allow millisecond precision in configuration file
- [FEATURE] Support AirBnB's Smartstack Nerve for [[Service|service]] discovery
- [ENHANCEMENT] Storage switches less often between regular and rushed mode.
- [ENHANCEMENT] Storage switches into rushed mode if there are too many memory chunks.
- [ENHANCEMENT] Added more storage instrumentation
- [ENHANCEMENT] Improved instrumentation of notification handler
- [BUGFIX] Do not count head chunks as chunks waiting for persistence
- [BUGFIX] Handle OPTIONS HTTP requests to the API correctly
- [BUGFIX] Parsing of ranges in PromQL fixed
- [BUGFIX] Correctly validate URL flag parameters
- [BUGFIX] Log argument parse errors
- [BUGFIX] Properly handle creation of target with bad TLS config
- [BUGFIX] Fix of checkpoint timing issue


<!-- risk-assessed -->
