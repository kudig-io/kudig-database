---
title: Clusterpedia [entities]
description: 'summary: "Clusterpedia 是一个多集群资源的统一搜索和查询引擎，类似于 Kubernetes 资源的 "百科全书"。'
summary: 'Clusterpedia 是一个多集群资源的统一搜索和查询引擎，类似于 Kubernetes 资源的 "百科全书"。它将多个集群的资源同步到统一的存储中，提供与 kubectl 兼容的 API 进行跨集群的资源搜索、过滤和分页查询。'
category: entities
tags:
- k8s
- cncf
- orchestration
- clusterpedia
- postgresql
- rbac
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
- Clusterpedia 是什么
- 如何 Clusterpedia
trigger_keywords:
- Clusterpedia
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Clusterpedia

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Clusterpedia 是一个多集群资源统一搜索引擎，名称取自"Cluster + Encyclopedia（百科全书）"。2021 年进入 CNCF Sandbox。它将多个 Kubernetes 集群的资源元数据同步到统一的存储后端，提供与 `kubectl` 完全兼容的查询 API，支持跨集群的资源搜索、过滤、排序和分页。

在多集群管理场景中，运维人员经常需要回答"哪些集群有名为 `myapp` 的 Deployment？"、"集群 A 的 Pod 异常是否也出现在集群 B 中？"等问题。传统方式需要逐个切换 kubectl context 查询，效率低下。Clusterpedia 通过统一的 API Server 解决这一痛点，支持复杂查询语法（标签选择器、字段过滤、资源类型过滤）。

## Key Features

- **kubectl 兼容 API**：使用标准的 `kubectl get` 语法查询，支持 `--selector`、`--field-selector`
- **跨集群搜索**：一次查询覆盖所有注册的集群，支持按集群过滤
- **资源同步**：通过 informer 机制增量同步资源，对源集群影响极小
- **多种存储后端**：支持 PostgreSQL、MySQL 和内置内存存储
- **分页与排序**：大数据集分页查询，按创建时间、名称等排序
- **RBAC 集成**：基于 Kubernetes RBAC 控制用户可查询的集群和资源范围

## Architecture

Clusterpedia 由 **Clusterpedia API Server**（兼容 Kubernetes API 的查询入口）、**PediaCluster Controller**（管理源集群注册和连接）、**Storage Layer**（将资源同步到 DB 后端）和 **Resource Syncer**（从源集群增量同步资源变更）组成。每个注册的 PediaCluster 通过 kubeconfig 连接到源集群的 API Server，使用 Watch 机制实时同步资源状态。

## K8s 集成

Clusterpedia 的 API Server 实现 Kubernetes API 规范，因此可以配置为 `kubectl` 的额外集群上下文。用户通过 `kubectl --context=clusterpedia get pods --all-clusters` 执行跨集群查询。也支持通过 `kubebuilder` 的资源发现机制自动识别可用资源类型。RBAC 权限映射允许细粒度控制用户可查询的集群和命名空间。

## 生产部署要点

- **资源选择**：只同步需要查询的资源类型，减少存储和同步开销
- **存储后端**：大规模场景使用 PostgreSQL，小规模使用内置存储
- **标签查询**：利用 search label 实现复杂的跨集群查询
- **增量同步**：Clusterpedia 使用增量同步，对源集群影响极小
- **权限控制**：配置 RBAC 限制用户可查询的集群和资源范围

## 生产场景

1. **多集群资源审计**：搜索所有集群中使用了特定镜像的 Pod，用于安全排查
2. **跨集群故障排查**：检查异常资源是否在多个集群中同时出现
3. **资源清单汇总**：统计所有集群的 Deployment/Service 总数和分布
4. **合规性检查**：查找缺少必要标签或安全配置的工作负载

## 安装

```bash
# 使用 Helm 安装 Clusterpedia
helm repo add clusterpedia https://clusterpedia.io
helm install clusterpedia clusterpedia/clusterpedia -n clusterpedia-system --create-namespace \
  --set storage.type=postgres \
  --set storage.postgres.host=postgres.database.svc.cluster.local

# 注册源集群
kubectl apply -f - <<EOF
apiVersion: cluster.clusterpedia.io/v1alpha2
kind: PediaCluster
metadata:
  name: cluster-prod
spec:
  kubeconfig: "<base64-encoded-kubeconfig>"
  syncResources:
  - group: ""
    resources: ["pods","services","namespaces"]
  - group: "apps"
    resources: ["deployments","statefulsets"]
EOF

# 配置 kubectl 上下文
kubectl config set-cluster clusterpedia --server=https://clusterpedia.clusterpedia-system.svc
kubectl --context=clusterpedia get pods --all-clusters
```

## 对比

| 特性 | Clusterpedia | Karmada | KubeFed |
|------|-------------|---------|---------|
| 功能定位 | 多集群查询/搜索 | 多集群编排/调度 | 多集群联邦 |
| 资源同步 | 只读同步 | 读写管理 | 读写管理 |
| kubectl 兼容 | ✅ | ✅ | ⚠️ |
| 存储后端 | PostgreSQL/MySQL | etcd | etcd |

## 参考链接

- [[deployment]]
- [[crd-custom-resources]]
- [[pod-lifecycle]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- index/cluster-index|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
