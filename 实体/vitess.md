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
last_updated: 2026-07
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（信息收集，无副作用）。



# Vitess

> **CNCF 状态**: Graduated | **类别**: Database | **主要语言**: Go

## 概述

Vitess 是一个用于 MySQL 水平扩展的数据库集群系统和分片中间件，最初由 YouTube 开发并于 2018 年作为 CNCF 孵化项目加入，2020 年正式毕业（Graduated）。它将 MySQL 的功能与 NoSQL 数据库的可扩展性相结合，为大规模 MySQL 部署提供连接池、查询路由、水平分片和在线 DDL 等能力。Vitess 在 YouTube 每天处理数十亿条 SQL 查询，支撑了全球最大的 MySQL 部署之一。它通过 VTGate 代理层对应用透明地分发查询，通过 VTTablet 管理每个 MySQL 实例，并通过 VReplication 实现跨分片的数据同步和迁移。

## 核心能力

- **水平分片**: 自动化数据分片和路由，支持 hash、range 等分片策略，无需修改应用代码
- **连接池**: VTGate 高效的连接复用，保护后端 MySQL 不被过多连接压垮
- **查询路由**: 智能查询分发，将查询定向到正确的分片，支持跨分片查询合并
- **在线 DDL**: 无停机表结构变更（Online DDL），支持 gh-ost 和 pt-osc 策略
- **VReplication**: 高性能数据迁移和同步引擎，支持分片拆分、合并和迁移
- **备份恢复**: 自动化备份和时间点恢复（PITR），支持本地和云存储后端

## 架构

Vitess 采用分层代理架构，核心组件包括：

- **VTGate**: 轻量级无状态代理，接收 SQL 查询并路由到正确的 VTTablet，支持多租户
- **VTTablet**: 部署在每个 MySQL 实例旁的 sidecar 进程，管理 MySQL 生命周期、执行查询、处理复制
- **Topology Service**: 基于 etcd 的元数据存储，保存分片拓扑、VSchema 和路由信息
- **VTOrc**: 自动故障检测和修复组件，负责 MySQL 复制拓扑的管理和高可用切换
- **vtctld**: 管理界面，提供 Web UI 和 CLI 用于集群运维操作

数据流：`Application → VTGate (SQL 解析/路由) → VTTablet (查询执行) → MySQL`

## K8s 集成

Vitess 通过 **Vitess Operator** 实现与 Kubernetes 的深度集成。Operator 管理 VTGate、VTTablet 和 MySQL 的生命周期，使用 CRD（`EtcdLockserver`、`VitessCluster`）声明式定义集群拓扑。VTTablet 作为 sidecar 与 MySQL 运行在同一 Pod 中，通过本地 Unix socket 通信。Vitess Operator 支持自动故障转移、滚动升级、备份调度和分片重平衡，全部以 Kubernetes 原生方式管理。生产环境推荐配合 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 PodDisruptionBudget 和 topologySpreadConstraints 使用。

## 生产场景

1. **大规模 MySQL 分片**: 单表数据量超过单机容量时，通过 Vitess 水平分片将数据分散到多个 MySQL 实例
2. **多租户 SaaS 平台**: 利用 Vitess 的多租户能力为不同租户提供隔离的数据库访问
3. **零停机数据库迁移**: 使用 VReplication 在不同分片策略之间迁移数据，无需停服
4. **云原生 MySQL 高可用**: 在 Kubernetes 上运行 MySQL，借助 Operator 实现自动化运维

## 安装

```bash
# 安装 Vitess Operator
kubectl apply -f https://raw.githubusercontent.com/planetscale/vitess-operator/v2.10.0/deploy/operator.yaml

# 部署示例集群
kubectl apply -f https://raw.githubusercontent.com/planetscale/vitess-operator/v2.10.0/examples/local/example.yaml

# 安装 vtctlclient CLI
brew install vitess
```

## 对比

| 特性 | Vitess | ProxySQL | MySQL Router |
|------|--------|----------|--------------|
| 分片能力 | ✅ 原生水平分片 | ❌ 仅代理 | ❌ 仅代理 |
| 在线 DDL | ✅ 支持 | ❌ 不支持 | ❌ 不支持 |
| 多租户 | ✅ 原生支持 | ⚠️ 有限 | ❌ 不支持 |
| CNCF 状态 | Graduated | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，Vitess 属于 **Database** 类别，为云原生应用提供关键的 MySQL 水平扩展和集群管理能力。

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
