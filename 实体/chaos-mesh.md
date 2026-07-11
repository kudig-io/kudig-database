---
title: Chaos Mesh [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- orchestration
- chaos-mesh
- scheduler
- crd
- operator
- ebpf
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Chaos Mesh 是什么
- 如何 Chaos Mesh
trigger_keywords:
- Chaos
- Mesh
prerequisites:
- kubectl-basics
- ebpf-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Chaos Mesh

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

Chaos Mesh 是一个 CNCF 孵化项目，由 PingCAP 开发，是 Kubernetes 原生的混沌工程平台。它通过 CRD 定义各种故障注入实验（网络延迟、Pod 杀死、IO 错误、时钟偏移等），帮助团队主动发现分布式系统的弱点。Chaos Mesh 支持细粒度的故障注入——可以精确到特定 Pod、Namespace 或标签选择的工作负载，是构建韧性疾病系统的核心工具。项目于 2020 年加入 CNCF 孵化。

## Key Features（核心能力）

- **丰富的故障类型**：支持网络（延迟/丢包/分区）、Pod（Kill/IO 错误）、磁盘、时钟、内核等故障
- **声明式 CRD**：通过 ChaosExperiment 和 ChaosSchedule CRD 定义故障实验
- **细粒度目标选择**：通过 Namespace、Label Selector 精确定位故障目标
- **安全隔离**：通过 ServiceAccount 和 Namespace 限制故障注入范围
- **可视化 Dashboard**：Web UI 管理和监控混沌实验
- **工作流编排**：支持顺序、并行、条件分支的混沌实验编排

## 架构与工作原理

Chaos Mesh 由多个 Controller 组成：ChaosDaemon Controller 管理每个节点上的 chaos-daemon（特权 DaemonSet，执行实际的故障注入）；ChaosControllerManager 管理各类 ChaosExperiment CRD（PodChaos、NetworkChaos、IOChaos 等）；Dashboard 提供 Web 界面。每种故障类型有专门的 Controller 和 Reconcile 逻辑，通过 chaos-daemon 在目标节点上执行故障操作（如 iptables 规则注入、tc 流量控制、mount IO 干扰）。

## K8s 集成

Chaos Mesh 完全以 K8s CRD 方式工作：PodChaos CRD 定义 Pod 故障（如随机 kill Pod）；NetworkChaos CRD 定义网络故障（如延迟、丢包）；IOChaos CRD 定义文件系统故障（如 IO 错误、延迟）。通过 Namespace 和 Label Selector 精确选择故障目标 Pod。chaos-daemon 以 DaemonSet 部署，使用特权模式执行故障注入。通过 ChaosSchedule CRD 可定期自动执行实验。

## 生产用例

- **系统韧性测试**：注入故障验证系统的容错和恢复能力
- **故障演练**：定期执行混沌实验提升团队排障能力
- **CI/CD 集成**：在部署流水线中自动执行混沌测试
- **微服务韧性验证**：验证断路器、重试、超时等韧性机制的有效性

## 安装与快速开始

```bash
curl -sSL https://mirrors.chaos-mesh.org/v2.7.0/install.sh | bash -s -- --microk8s.kubeconfig /path/to/kubeconfig
# 或使用 Helm
helm install chaos-mesh chaos-mesh/chaos-mesh -n chaos-testing --create-namespace
```

## 对比替代方案

相比 LitmusChaos（CNCF 孵化），Chaos Mesh 的故障类型更丰富且 K8s 原生程度更高。相比 Gremlin（商业方案），Chaos Mesh 是开源的且社区活跃。

## Related

- digest-2026-05-21 — Wiki 全量知识库摘要 — 2026-05-21
- [[实体/k8s-advanced-ecosystem.md|k8s-advanced-ecosystem]] — 硬件知识体系、CNCF 全景生态与 eBPF 平台工程
- [[技能/Agent Orchestration Patterns.md|[[Agent Orchestration Patterns for FTA|Agent Orchestration Patterns]]]] — Agent Orchestration Patterns for FTA
- observability.md|cncf-observability]] — CNCF 可观测性项目全景
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- chaos-mesh
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
