---
title: Pod CrashLoopBackOff & OOMKilled 诊断与修复
description: 针对 Pod CrashLoopBackOff 和 OOMKilled 两大高频故障的完整诊断技能，包含症状识别、快速分级、Exit Code 决策树、分阶段诊断流程、证据三元组、根因分类、修复操作、验证确认与升级协议
summary: 生产环境中 30-40% 的 Pod 工单为 CrashLoopBackOff/OOMKilled，本技能提供从症状识别到根因确认、修复验证的完整生产级诊断路径
category: skill
tags:
- k8s
- pod
- troubleshooting
- crashloop
- oomkilled
- exit-code
- memory
- sop
- runbook
- observability
- slo
sources:
- 故障诊断/topic-skills/02-pod-crashloop-oomkilled.md
- 故障诊断/FTA故障树/list/pod-fta.md
- 故障诊断/FTA故障树/kubernetes-fta-full-analysis.md
- code/kubernetes-release-1.28/
- code/kubernetes-release-1.34/
- code/kubernetes-1.36.2/
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 30min
intent_queries:
- Pod CrashLoopBackOff 怎么排查
- OOMKilled 如何解决
- 容器反复重启什么原因
- Exit Code 137 是什么意思
- Pod 频繁崩溃怎么诊断
trigger_keywords:
- CrashLoopBackOff
- OOMKilled
- 容器崩溃
- 容器重启
- Pod重启
- exit code 137
- exit code 1
- 内存溢出
- 频繁重启
- Back-off restarting
prerequisites:
- kubectl-basics
- pod-lifecycle
- troubleshooting-methodology
skill_id: SKILL-POD-001
skill_name: Pod CrashLoopBackOff & OOMKilled 诊断与修复
version: 2.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L2-semi-auto
fta_path: TE-2 -> IE-2.1 -> BE-2.1/BE-2.3
---

> **内容已并入主场**：[[skills/09-技能体系/02-pod-crashloop-oomkilled.md|Pod CrashLoopBackOff & OOMKilled 诊断与修复]]（本页为重排去重桩，正文见主场）
