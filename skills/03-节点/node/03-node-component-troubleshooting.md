---
title: 节点组件故障排查
description: 针对 kubelet、containerd/CRI-O、kube-proxy、CNI 等节点核心组件的故障排查技能，包含组件状态检查、日志分析、性能诊断和修复方案
summary: 节点组件是集群工作节点上的核心守护进程，本技能覆盖 kubelet/containerd/kube-proxy/CNI 四大组件的完整故障排查路径
category: skill
tags:
- k8s
- node
- troubleshooting
- kubelet
- containerd
- kube-proxy
- cni
- cri
- component
- sop
sources:
- 故障诊断/高级排障/35-node-component-troubleshooting.md
- 故障诊断/高级排障/structural-02-node-components/01-kubelet-troubleshooting.md
- 故障诊断/高级排障/structural-02-node-components/04-node-troubleshooting.md
- 故障诊断/技能体系/01-node-notready.md
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
estimated_read_time: 20min
intent_queries:
- kubelet 故障怎么排查
- containerd 异常如何处理
- kube-proxy 不工作怎么办
- 节点组件异常怎么诊断
- CNI 插件故障怎么排查
trigger_keywords:
- kubelet
- containerd
- kube-proxy
- CNI
- 节点组件
- 容器运行时
- 组件故障
- PLEG
- cri
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- node-architecture
- networking-basics
skill_id: SKILL-NODE-003
skill_name: 节点组件故障排查
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L2-semi-auto
fta_path: TE-1 -> IE-1.1/IE-1.2
---

> **内容已并入主场**：[[wiki/17-故障诊断/04-高级排障/01-node-component-troubleshooting.md|节点组件故障排查]]（本页为重排去重桩，正文见主场）
