---
title: "Pod Security Standards 迁移"
description: "Pod Security Standards 迁移：PSP→PSA 迁移路径、Privileged/Baseline/Restricted 配置、Admission 配置与常见兼容问题"
summary: "面向 SRE 与安全工程师的 Pod Security Standards 完整迁移指南，覆盖从 PodSecurityPolicy 到 Pod Security Admission 的迁移路径、三级标准配置与兼容问题处理。"
category: 安全
tags:
- pod-security
- psa
- psp
- admission
- security
- migration
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 安全工程师
estimated_read_time: 20min
intent_queries:
- "如何从 PSP 迁移到 Pod Security Admission"
- "Privileged Baseline Restricted 三个级别如何配置"
- "PSA 迁移有哪些兼容性问题"
trigger_keywords:
- pod security
- psa
- psp
- admission
- privileged
- baseline
- restricted
prerequisites:
- kubectl-basics
- rbac-basics
- security-fundamentals
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **内容已并入主场**：[[wiki/08-安全/04-策略治理/06-pod-security-standards-migration.md|Pod Security Standards 迁移]]（本页为重排去重桩，正文见主场）
