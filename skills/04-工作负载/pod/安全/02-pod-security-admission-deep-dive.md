---
title: Pod Security Admission 深度配置
description: 'PSA 替代 PSP：Privileged/Baseline/Restricted 三级策略、命名空间标签配置、从 PSP 迁移到 PSA、自定义准入策略补充、渐进式部署'
summary: 'PSA 替代 PSP：三级安全策略、命名空间标签配置、PSP 迁移与渐进式部署'
category: security-compliance
tags:
- pod-security
- admission-controller
- psp
- psa
- security-policy
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Pod Security Admission 是什么
- 如何配置 PSA 替代 PSP
trigger_keywords:
- Pod Security Admission
- PSA
- PSP
- Pod Security Standards
- 准入控制
prerequisites:
- kubectl-basics
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

> **内容已并入主场**：[[wiki/08-安全/01-身份与访问/02-pod-security-admission-deep-dive.md|Pod Security Admission 深度配置]]（本页为重排去重桩，正文见主场）
