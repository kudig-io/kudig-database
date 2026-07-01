---
title: Fluid (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- fluid
- prometheus
- grafana
- networkpolicy
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Fluid 是什么
- 如何 Fluid
trigger_keywords:
- Fluid
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# Fluid

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

Fluid 是 Kubernetes 上的数据集编排和加速系统，为数据密集型应用（如 AI/ML、大数据分析）提供数据抽象层。它通过分布式缓存引擎加速数据访问，实现数据与计算的协同调度。

## 核心能力

- **数据抽象**: Dataset CRD 统一管理数据访问
- **数据加速**: 支持 Alluxio、JuiceFS、Vineyard 等缓存引擎
- **数据感知调度**: 将 Pod 调度到数据缓存所在节点
- **弹性伸缩**: 根据负载自动扩缩缓存集群
- **数据预热**: 提前加载数据到缓存层
- **数据迁移**: 支持数据在不同存储间迁移

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **分层缓存**: 配置 MEM + SSD 多级缓存提高命中率
- **数据预热**: 训练前预热数据减少首次访问延迟
- **亲和性调度**: 让计算任务靠近数据缓存
- **缓存清理**: 定期清理过期缓存释放空间
- **监控告警**: 监控缓存命中率和使用量

## 架构定位

在 CNCF 生态中，fluid 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/networkpolicy.md|[[NetworkPolicy|networkpolicy]]]]
- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[cloudevents]] — CloudEvents
- [[keda]] — KEDA
- [[cozystack]] — Cozystack
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[vineyard]] — Vineyard

- fluid
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
