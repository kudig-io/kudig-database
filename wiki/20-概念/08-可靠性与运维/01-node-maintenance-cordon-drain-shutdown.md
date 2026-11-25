---
sources:
- "集群基础/控制平面/36-node-maintenance-cordon-drain-shutdown.md"
title: 节点维护 (Cordon/Drain/Shutdown/Swap) Runbook
summary: 系统化汇总 Kubernetes 节点维护全流程：cordon、drain、graceful shutdown、swap 支持与节点重启回归。
category: concepts
tags:
- node-maintenance
- cordon
- drain
- graceful-shutdown
- swap
- pdb
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台运维
estimated_read_time: 25min
intent_queries:
- 如何安全 drain 节点
- cordon 与 drain 区别
- graceful node shutdown 如何配置
- Kubernetes 支持 swap 吗
trigger_keywords:
- cordon
- drain
- graceful shutdown
- swap
- 节点维护
- PDB
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/01-集群基础与系统/03-控制平面/40-node-maintenance-cordon-drain-shutdown.md|节点维护 (Cordon/Drain/Shutdown/Swap) Runbook]]（本页为重排去重桩，正文见主场）
