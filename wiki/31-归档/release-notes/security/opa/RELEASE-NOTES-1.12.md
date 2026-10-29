---
title: opa v1.12 Release Notes
description: opa v1.12 Release Notes — Kubernetes 生产运维知识库
summary: opa v1.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- opa
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opa v1.12 Release Notes 是什么
- 如何 opa v1.12 Release Notes
trigger_keywords:
- opa
- v1.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opa v1.12 Release Notes

Source: [v1.12.3](https://github.com/open-policy-agent/opa/releases/tag/v1.12.3)

v1.12.3


This is a bug fix release addressing two issues:

###  Bundle polling is being misconfigured when discovery bundle is updated ([#8215](https://github.com/open-policy-agent/opa/issues/8215))

This is an issue where the polling interval for discovery (`discovery.polling.min_delay_seconds` and `discovery.polling.max_delay_seconds`) were misinterpreted on reconfiguration, causing extremely long update intervals.

Reported by @loganmiller-chime, authored by @sspaink

### Decision log `size` buffer `buffer_size_limit_bytes` misconfigured during reconfiguration ([#8213](https://github.com/open-policy-agent/opa/pull/8213))

This is a regression in the decision log, where the `decision_logs.reporting.buffer_size_limit_bytes` was mistakenly assigned the value of `decision_logs.reporting.upload_size_limit_bytes` during reconfiguration.
This issue is only present when `decision_logs.reporting.buffer_type` is set to `size`, which is the default value.

Authored by @sspaink



<!-- risk-assessed -->
