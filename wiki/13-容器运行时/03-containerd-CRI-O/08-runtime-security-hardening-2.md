---
title: 容器运行时安全加固指南
description: 面向 containerd/CRI-O 的容器运行时安全加固实践，覆盖 seccomp、AppArmor/SELinux、特权容器限制、User Namespaces、Falco/Tetragon 与镜像签名强制校验
category: container-runtime
tags:
- production
- best-practices
- playbook
- container-runtime
- security
- seccomp
- apparmor
- selinux
- user-namespaces
- falco
- tetragon
- supply-chain
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 安全工程师
estimated_read_time: 25min
intent_queries:
- 容器运行时安全加固指南是什么
- 如何加固 containerd/CRI-O 运行时安全
- seccomp AppArmor SELinux UserNamespaces Falco Tetragon 最佳实践
trigger_keywords:
- 运行时安全
- seccomp
- AppArmor
- SELinux
- User Namespaces
- Falco
- Tetragon
- privileged 限制
prerequisites:
- kubectl-basics
- containerd-basics
- linux-basics
- security-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **内容已并入主场**：[[wiki/13-容器运行时/03-containerd-CRI-O/09-runtime-security-hardening.md|容器运行时安全加固指南]]（本页为重排去重桩，正文见主场）
