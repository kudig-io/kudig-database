---
title: TiKV (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- tikv
- scheduler
- prometheus
- grafana
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- TiKV 是什么
- 如何 TiKV
trigger_keywords:
- TiKV
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# TiKV

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Rust

## 概述

description: '## 项目概述'

## 核心能力

- **分布式事务**: 完整的 ACID 事务支持，基于 Percolator 模型
- **水平扩展**: 自动数据分片（Region）和负载均衡
- **高可用**: Multi-Raft 共识协议，自动故障转移
- **强一致性**: 线性一致读写，支持快照隔离
- **协处理器**: 下推计算能力，减少数据传输
- **RawKV/TxnKV**: 支持原始 KV 和事务 KV 两种 API

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 至少部署 3 个 TiKV 节点
- PD 部署 3 个节点保证高可用
- 使用 SSD 存储
- 配置合理的 Region 大小
- 开启 Titan（大 Value 优化）
- 调整 RocksDB 参数

## 架构定位

在 CNCF 生态中，tikv 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/storage-model.md|storage-model]]
- [[entities/kube-scheduler.md|kube-scheduler]]

## Related

- [[32-发布/package/2026-07-02_18-53/corpus/supporting/skills/training-lecturer/11-workloads/index|release-notes-observability]] — 发布说明索引 — 可观测性
- [[entities/cncf-observability.md|cncf-observability]] — CNCF 可观测性项目全景
- [[chaos-mesh]] — Chaos Mesh
- [[kubean]] — Kubean
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tikv
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
