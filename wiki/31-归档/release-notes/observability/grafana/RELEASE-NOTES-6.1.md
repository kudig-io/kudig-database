---
title: grafana v6.1 Release Notes
description: grafana v6.1 Release Notes — Kubernetes 生产运维知识库
summary: grafana v6.1 Release Notes — Kubernetes 生产运维知识库
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
- grafana v6.1 Release Notes 是什么
- 如何 grafana v6.1 Release Notes
trigger_keywords:
- grafana
- v6.1
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




# grafana v6.1 Release Notes

Source: [v6.1.6](https://github.com/grafana/grafana/releases/tag/v6.1.6)

# 6.1.6 (2019-04-29)
### Features / Enhancements
* **Security**: Bump jQuery to 3.4.0 . [#16761](https://github.com/grafana/grafana/pull/16761), [@dprokop](https://github.com/dprokop)

### Bug Fixes
* **Playlist**: Fix loading dashboards by tag. [#16727](https://github.com/grafana/grafana/pull/16727), [@marefr](https://github.com/marefr)

# 6.1.5 (2019-04-29)

* **Security**: Urgent security patch release. Please read more in our [blog](https://grafana.com/blog/2019/04/29/grafana-5.4.4-and-6.1.6-released-with-important-security-fix/)

<!-- risk-assessed -->
