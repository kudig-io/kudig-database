---
title: Clusternet (entities)
description: '## 概述'
summary: 'Clusternet 是一个多集群管理和应用分发平台，专为管理跨云、跨区域的 Kubernetes 集群而设计。它采用 Hub-Agent 架构，支持 Pull 和 Push 两种模式进行集群注册，能够将应用资源（Deployment、[[Service|Service]]、Helm Release 等）智能分发到多个子集群。'
category: entities
tags:
- k8s
- cncf
- orchestration
- clusternet
- prometheus
- grafana
- helm
- crd
- operator
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Clusternet 是什么
- 如何 Clusternet
trigger_keywords:
- Clusternet
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Clusternet

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Clusternet 是一个多集群管理和应用分发平台，专为管理跨云、跨区域的 Kubernetes 集群而设计。它采用 Hub-Agent 架构，支持 Pull 和 Push 两种模式进行集群注册，能够将应用资源（Deployment、[[Service|Service]]、Helm Release 等）智能分发到多个子集群。Clusternet 特别适合边缘计算和混合云场景，即使子集群位于 NAT 或防火墙后面也...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **边缘优先 Pull 模式**: 边缘集群通常位于 NAT 后，使用 Agent 主动连接 Hub
- **标签规范**: 统一集群标签体系 (location, tier, env)，便于 Subscription 选择
- **Hub 高可用**: Hub 集群部署多副本，配置持久化存储
- **渐进式分发**: 先通过标签选择少量集群验证，再扩大分发范围
- **监控 Agent 状态**: 监控 ManagedCluster 的 conditions，及时发现断连集群

## 架构定位

在 CNCF 生态中，clusternet 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[runme-notebooks]] — Runme
- [[operator-framework]] — Operator Framework
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[helm]] — Helm

- clusternet
- [[concepts/etcd x 高可用模式.md|etcd × 高可用模式]] — Cross-reference
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
