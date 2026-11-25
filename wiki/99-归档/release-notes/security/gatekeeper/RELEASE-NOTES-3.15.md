---
title: gatekeeper v3.15 Release Notes
description: gatekeeper v3.15 Release Notes — Kubernetes 生产运维知识库
summary: gatekeeper v3.15 Release Notes — Kubernetes 生产运维知识库
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
- gatekeeper v3.15 Release Notes 是什么
- 如何 gatekeeper v3.15 Release Notes
trigger_keywords:
- gatekeeper
- v3.15
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




# gatekeeper v3.15 Release Notes

Source: [v3.15.1](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.15.1)

## Features
- Update audit and controller manager with pod labels (#3240) (cherry pick) (#3294) [#3294](https://github.com/open-policy-agent/gatekeeper/pull/3294) ([James Bruce](https://github.com/open-policy-agent/gatekeeper/commit/3ac0636df12e02beecf23bfdaad2a06b28258a09))

## Chores
- Prepare v3.15.1 release (#3304) [#3304](https://github.com/open-policy-agent/gatekeeper/pull/3304) ([github-actions[bot]](https://github.com/open-policy-agent/gatekeeper/commit/3350319f76d3e2d78df0b972c63258cba7c7915f))

<!-- risk-assessed -->
