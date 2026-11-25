---
sources:
- "安全/运行时安全/18-security-context-fields-reference.md"
title: Security Context 字段完整参考
summary: 系统化汇总 Kubernetes Pod 级与容器级 securityContext 全字段、Linux 安全模块与 Pod Security Standards 映射。
category: concepts
tags:
- security-context
- seccomp
- apparmor
- selinux
- capabilities
- pod-security
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 平台架构师
estimated_read_time: 22min
intent_queries:
- securityContext 有哪些字段
- runAsNonRoot 与 runAsUser 区别
- seccomp AppArmor SELinux 在 K8s 怎么配置
- capabilities 如何 drop
trigger_keywords:
- securityContext
- seccomp
- AppArmor
- SELinux
- capabilities
- runAsNonRoot
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/07-安全/03-运行时安全/07-security-context-fields-reference.md|Security Context 字段完整参考]]（本页为重排去重桩，正文见主场）
