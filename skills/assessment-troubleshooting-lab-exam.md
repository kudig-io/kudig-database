---
title: Troubleshooting Lab Exam
description: '- 需要完整记录排查步骤、命令、修复和验证过程'
category: skills
tags:
- k8s
- troubleshooting
- skill
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Troubleshooting Lab Exam 是什么
- 如何 Troubleshooting Lab Exam
- Troubleshooting Lab Exam 故障排查
- Troubleshooting Lab Exam 排障步骤
trigger_keywords:
- Troubleshooting
- Lab
- Exam
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Troubleshooting Lab Exam

### 考核说明

- 每个场景满分 100 分，总分 200 分
- 场景随机抽取，考核过程中可查阅文档
- 需要完整记录排查步骤、命令、修复和验证过程
- 最终需要能复现问题修复

---

### 已知信息

- 命名空间：`production`
- Pod 名称：`payment-api-7d9f8b5c6-x2kqm`
- 镜像：`payment-[[Service|service]]:v1.2`
- 资源请求：`cpu: 2, memory: 4Gi`

## 相关链接

- [[skills/FTA Methodology and Core Principles|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview|诊断技能总览]]
- [[skills/troubleshoot-pod-issues|Pod 故障排查]]

## Related

- [[skills/skill-reference-version-matrix|skill-reference-version-matrix]] — Version Matrix
- [[skills/ts-security-auth|ts-security-auth]] — 安全认证故障排查
- [[skills/troubleshoot-pod-issues|troubleshoot-pod-issues]] — Troubleshoot Pod Issues
- [[skills/skill-k8s-node-notready-USAGE-GUIDE|skill-k8s-node-notready-USAGE-GUIDE]] — Usage Guide
- [[kubernetes]] — Kubernetes (CNCF Graduated)
