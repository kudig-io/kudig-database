---
title: KubeElastic (entities)
description: '## 概述'
summary: 'KubeElastic 是一个 Kubernetes 原生的弹性伸缩和资源优化平台，专注于基于实时负载和成本的智能资源调整。它结合机器学习预测算法，自动调整 Pod 资源配额（VPA）和副本数（HPA），同时优化集群节点利用率，帮助用户在保证性能 SLO 的前提下降低云成本。'
category: entities
tags:
- k8s
- cncf
- observability
- kubeelasti
- prometheus
- grafana
- hpa
- vpa
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeElastic 是什么
- 如何 KubeElastic
trigger_keywords:
- KubeElastic
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeElastic

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go

## 概述

KubeElasti 是一个 CNCF 沙箱项目，旨在为 Kubernetes 提供弹性存储卷管理能力。它通过动态调整 PV 大小和 IOPS 限制，根据工作负载实际需求自动伸缩存储资源。KubeElasti 解决了 K8s 存储资源过度分配的问题——许多 PV 在创建时分配了大量空间但实际使用率很低，KubeElasti 可以根据监控指标自动调整存储分配，降低存储成本。

## Key Features（核心能力）

- **动态卷扩缩**：根据使用率自动扩展或回收 PV 空间
- **IOPS 调整**：动态调整云存储卷的 IOPS 和吞吐限制
- **基于指标的伸缩**：通过 Prometheus 指标触发存储伸缩
- **多 CSI 支持**：兼容支持 Volume Expansion 的 CSI 驱动
- **安全策略**：定义最小/最大卷大小限制防止异常伸缩
- **通知机制**：伸缩事件通知到 Slack/PagerDuty

## 架构与工作原理

KubeElasti 由 Controller 和 Monitor 组成。Controller 监听 ElasticVolume CRD，管理卷伸缩的生命周期。Monitor 定期从 Prometheus 查询 PV 使用率指标，当使用率超过/低于阈值时触发伸缩决策。Controller 通过 K8s Volume Expansion API（editting PVC spec.resources.requests.storage）和 CSI 驱动接口执行实际的卷大小调整。

## K8s 集成

KubeElasti 通过自定义 CRD 与 K8s 集成。ElasticVolume CRD 定义目标 PVC、伸缩策略（阈值、最小/最大大小、步长）。Controller 监听这些 CRD 和 Prometheus 指标，通过修改 PVC 的 resources.requests.storage 字段触发 CSI Volume Expansion。仅支持 allowVolumeExpansion: true 的 StorageClass。

## 生产用例

- **存储成本优化**：自动回收未使用的 PV 空间
- **数据库存储管理**：根据数据库增长自动扩展存储
- **日志存储管理**：根据日志量自动调整日志卷大小
- **开发环境**：为开发环境自动分配和回收存储

## 安装与快速开始

```bash
kubectl apply -f https://github.com/kubeelasti/kubeelasti/releases/latest/download/kubeelasti.yaml
```

## 对比替代方案

相比手动 PV 管理，KubeElasti 提供自动化存储弹性伸缩。相比 K8s 原生 Volume Expansion（仅支持手动扩展），KubeElasti 提供基于指标的自动伸缩。

## Related

- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubeflow]] — Kubeflow
- [[spiffe]] — SPIFFE
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubeelasti
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
