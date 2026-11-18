---
title: Pod CrashLoopBackOff & OOMKilled 诊断与修复
description: '## 1. 概述'
summary: '1. 应用代码 bug 导致进程启动后立即崩溃，Pod 进入 CrashLoopBackOff'
category: pod
tags:
- k8s
- skills
- sop
- runbook
- kubelet
- prometheus
- grafana
- istio
- envoy
- coredns
tier: core
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- Pod CrashLoopBackOff & OOMKilled 诊断与修复 是什么
- 如何 Pod CrashLoopBackOff & OOMKilled 诊断与修复
trigger_keywords:
- CrashLoopBackOff
- OOMKilled
- 容器崩溃
- 容器重启
- Pod重启
- exit code 137
- exit code 1
- container killed
- 内存溢出
- 频繁重启
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- monitoring-basics
skill_id: SKILL-POD-001
skill_name: Pod CrashLoopBackOff & OOMKilled 诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---

> **内容已并入主场**：[[wiki/19-故障诊断/08-技能体系/02-pod-crashloop-oomkilled.md|Pod CrashLoopBackOff & OOMKilled 诊断与修复]]（本页为重排去重桩，正文见主场）
