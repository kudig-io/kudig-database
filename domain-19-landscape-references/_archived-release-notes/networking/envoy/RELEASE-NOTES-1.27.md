---
title: envoy v1.27 Release Notes
description: envoy v1.27 Release Notes — Kubernetes 生产运维知识库
summary: envoy v1.27 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- envoy v1.27 Release Notes 是什么
- 如何 envoy v1.27 Release Notes
trigger_keywords:
- envoy
- v1.27
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




# [[Envoy|envoy]] v1.27 Release Notes

Source: [v1.27.7](https://github.com/envoyproxy/envoy/releases/tag/v1.27.7)

repo: Release v1.27.7


**Summary of changes**:

- [CVE-2024-39305](https://github.com/envoyproxy/envoy/security/advisories/GHSA-fp35-g349-h66f) A bug where additional cookie attributes are not sent properly to clients.

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.27.7
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.27.7/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.27.7/version_history/v1.27/v1.27.7
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.27.6...v1.27.7

Signed-off-by: Yan Avlasov <yavlasov@google.com>
Signed-off-by: Ryan Northey <ryan@synca.io>

<!-- risk-assessed -->
