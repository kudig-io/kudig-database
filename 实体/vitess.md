---
title: Vitess (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- vitess
- etcd
- prometheus
- grafana
- argocd
- mysql
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Vitess 是什么
- 如何 Vitess
trigger_keywords:
- Vitess
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Vitess

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **水平分片**: 自动化数据分片和路由
- **连接池**: 高效的连接复用，保护 MySQL
- **查询路由**: 智能查询分发
- **在线 DDL**: 无停机表结构变更
- **VReplication**: 数据迁移和同步
- **备份恢复**: 自动化备份和时间点恢复

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 使用 Kubernetes Operator 部署
- 配置足够的 VTGate 实例
- 启用半同步复制
- 配置自动故障转移
- 选择合适的分片键
- 启用查询缓存

## 架构定位

在 CNCF 生态中，vitess 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[tinkerbell]] — Tinkerbell
- [[sops]] — SOPS (Secrets OPerationS) OPerationS)
- [[实体/argocd.md|argocd]] — ArgoCD
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- vitess
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
