---
title: prometheus v0.12 Release Notes
description: prometheus v0.12 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v0.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
- operator
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
- prometheus v0.12 Release Notes 是什么
- 如何 prometheus v0.12 Release Notes
trigger_keywords:
- prometheus
- v0.12
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




# [[Prometheus|prometheus]] v0.12 Release Notes

Source: [0.12.0](https://github.com/prometheus/prometheus/releases/tag/0.12.0)

This is the release that fixes the annoying and embarrassing fingerprinting bug (https://github.com/prometheus/client_golang/issues/74). All metrics stored with previous versions of Prometheus cannot be used anymore. A version guard will protect you from accidentally running the Prometheus server with an incompatible storage. Implementing a conversion tool would be a lot of work (but if somebody wants to do it, be our guest...), so there is no other solution right now but wiping the storage or stick with v0.11.1.

To sweeten the deal, fingerprinting is now more efficient, and we have also thrown in new features (OR operator and vector matching options).
- [CHANGE] Use client_golang v0.3.1. THIS CHANGES FINGERPRINTING AND INVALIDATES
  ALL PERSISTED FINGERPRINTS. You have to wipe your storage to use this or
  later versions. There is a version guard in place that will prevent you to
  run Prometheus with the stored data of an older Prometheus.
- [BUGFIX] The change above fixes a weakness in the fingerprinting algorithm.
- [ENHANCEMENT] The change above makes fingerprinting faster and less allocation
  intensive.
- [FEATURE] OR operator and vector matching options. See docs for details.
- [ENHANCEMENT] Scientific notation and special float values (Inf, NaN) now
  supported by the expression language.
- [CHANGE] Dockerfile makes Prometheus use the Docker volume to store data
  (rather than /tmp/metrics).
- [CHANGE] Makefile uses Go 1.4.2.


<!-- risk-assessed -->
