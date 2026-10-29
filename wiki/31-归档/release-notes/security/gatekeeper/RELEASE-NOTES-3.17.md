---
title: gatekeeper v3.17 Release Notes
description: gatekeeper v3.17 Release Notes — Kubernetes 生产运维知识库
summary: gatekeeper v3.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- gatekeeper v3.17 Release Notes 是什么
- 如何 gatekeeper v3.17 Release Notes
trigger_keywords:
- gatekeeper
- v3.17
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




# gatekeeper v3.17 Release Notes

Source: [v3.17.2](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.17.2)

## Bug Fixes
- fixing nil pointer error when converting VAPB from v1beta1 to v1 and updating crypto, net (#3754) (#3766) (#3781) [#3781](https://github.com/open-policy-agent/gatekeeper/pull/3781) ([Jaydip Gabani](https://github.com/open-policy-agent/gatekeeper/commit/9ff69951be914dc897a375cc73928b8921032bca))

## Chores
- Prepare v3.17.2 release (#3799) [#3799](https://github.com/open-policy-agent/gatekeeper/pull/3799) ([github-actions[bot]](https://github.com/open-policy-agent/gatekeeper/commit/c6da4aedc35ad769a6335ca1d56001dc936c5705))

<!-- risk-assessed -->
