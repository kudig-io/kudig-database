---
title: thanos v0.37 Release Notes
description: thanos v0.37 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.37 Release Notes — Kubernetes 生产运维知识库
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
- thanos v0.37 Release Notes 是什么
- 如何 thanos v0.37 Release Notes
trigger_keywords:
- thanos
- v0.37
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




# [[Thanos|thanos]] v0.37 Release Notes

Source: [v0.37.2](https://github.com/thanos-io/thanos/releases/tag/v0.37.2)

v0.37.2 is out now with a few fixes for Sidecar and Store, before the end of the year!

Please try it out and let us know if you find further issues! 🚀

## Changelog

### Fixed

- [#7970](https://github.com/thanos-io/thanos/pull/7970) Sidecar: Respect min-time setting.
- [#7962](https://github.com/thanos-io/thanos/pull/7962) Store: Fix potential deadlock in hedging request.

<!-- risk-assessed -->
