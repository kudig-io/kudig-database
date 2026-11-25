---
title: opa v1.13 Release Notes
description: opa v1.13 Release Notes — Kubernetes 生产运维知识库
summary: opa v1.13 Release Notes — Kubernetes 生产运维知识库
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
- opa v1.13 Release Notes 是什么
- 如何 opa v1.13 Release Notes
trigger_keywords:
- opa
- v1.13
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




# opa v1.13 Release Notes

Source: [v1.13.2](https://github.com/open-policy-agent/opa/releases/tag/v1.13.2)

This release updates the version of Go used to build the OPA binaries and images to 1.25.7.
That version of the Go standard library contains a fix for [GO-2026-4337](https://pkg.go.dev/vuln/GO-2026-4337).

**Full Changelog**: https://github.com/open-policy-agent/opa/compare/v1.13.1...v1.13.2

<!-- risk-assessed -->
