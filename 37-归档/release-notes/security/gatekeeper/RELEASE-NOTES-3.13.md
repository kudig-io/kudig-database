---
title: gatekeeper v3.13 Release Notes
description: gatekeeper v3.13 Release Notes — Kubernetes 生产运维知识库
summary: gatekeeper v3.13 Release Notes — Kubernetes 生产运维知识库
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
- gatekeeper v3.13 Release Notes 是什么
- 如何 gatekeeper v3.13 Release Notes
trigger_keywords:
- gatekeeper
- v3.13
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




# gatekeeper v3.13 Release Notes

Source: [v3.13.4](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.13.4)

## Bug Fixes
- CVE-2023-45142 for release 3.13 (#3113) [#3113](https://github.com/open-policy-agent/gatekeeper/pull/3113) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/d7678344c7c91c2783d509ef94cccca35ea9874e))
- ns exclusion audit from cache (#3129) cherry-pick for 3.13 (#3140) [#3140](https://github.com/open-policy-agent/gatekeeper/pull/3140) ([alex](https://github.com/open-policy-agent/gatekeeper/commit/72364ad873be2af2f02549ab9aa027c0c8133b77))

## Chores
- bump kubectl for release 3.13 (#3118) [#3118](https://github.com/open-policy-agent/gatekeeper/pull/3118) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/8710f401a476d963180fe35443d86c3e64a3265c))
- Prepare v3.13.4 release (#3144) [#3144](https://github.com/open-policy-agent/gatekeeper/pull/3144) ([github-actions[bot]](https://github.com/open-policy-agent/gatekeeper/commit/d7228ce938e84118219a6126825944f1cff89311))

<!-- risk-assessed -->
