---
title: prometheus v0.14 Release Notes
description: prometheus v0.14 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v0.14 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v0.14 Release Notes 是什么
- 如何 prometheus v0.14 Release Notes
trigger_keywords:
- prometheus
- v0.14
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




# [[Prometheus|prometheus]] v0.14 Release Notes

Source: [0.14.0](https://github.com/prometheus/prometheus/releases/tag/0.14.0)

- [CHANGE] Configuration format changed and switched to YAML. 
  (See the provided [migration tool](https://github.com/prometheus/migrate/releases).)
- [ENHANCEMENT] Redesign of state-preserving target discovery.
- [ENHANCEMENT] Allow specifying scrape URL scheme and basic HTTP auth for non-static targets.
- [FEATURE] Allow attaching meaningful labels to targets via relabeling.
- [FEATURE] Configuration/rule reloading at runtime.
- [FEATURE] Target discovery via file watches.
- [FEATURE] Target discovery via Consul.
- [ENHANCEMENT] Simplified binary operation evaluation.
- [ENHANCEMENT] More stable component initialization.
- [ENHANCEMENT] Added internal expression testing language.
- [BUGFIX] Fix graph links with path prefix.
- [ENHANCEMENT] Allow building from source without git.
- [ENHANCEMENT] Improve storage iterator performance.
- [ENHANCEMENT] Change logging output format and flags.
- [BUGFIX] Fix memory alignment bug for 32bit systems.
- [ENHANCEMENT] Improve web redirection behavior.
- [ENHANCEMENT] Allow overriding default hostname for Prometheus URLs.
- [BUGFIX] Fix double slash in URL sent to alertmanager.
- [FEATURE] Add resets() query function to count counter resets.
- [FEATURE] Add changes() query function to count the number of times a gauge changed.
- [FEATURE] Add increase() query function to calculate a counter's increase.
- [ENHANCEMENT] Limit retrievable samples to the storage's retention window.


<!-- risk-assessed -->
