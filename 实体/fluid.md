---
title: Fluid (entities)
description: '## 概述'
summary: 'Fluid 是 Kubernetes 上的数据集编排和加速系统，为数据密集型应用（如 AI/ML、大数据分析）提供数据抽象层。'
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
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Fluid

> **CNCF 状态**: Incubating | **类别**: Data/Storage | **主要语言**: Go

## 概述

Fluid 是 Kubernetes 上的数据集编排和加速系统，由南京大学、阿里云、腾讯云等联合开发，2021 年加入 CNCF 沙箱，2024 年晋升孵化项目。它为数据密集型应用（如 AI/ML 训练、大数据分析）提供数据抽象层，通过分布式缓存引擎（Alluxio、JuiceFS、Vineyard、GooseFS）加速数据访问。Fluid 将数据抽象为 Kubernetes 一等公民——通过 Dataset CRD 声明数据集来源和缓存策略，系统自动在节点上部署缓存集群，并将计算 Pod 调度到数据缓存所在节点（数据感知调度），实现"数据靠近计算"的优化。相比直接从 HDFS/S3 读取数据，Fluid 可以将 AI 训练的数据加载速度提升数倍到数十倍。

## 核心能力

- **数据抽象**: Dataset CRD 统一管理数据集来源（S3、HDFS、NFS、PVC）和访问
- **多缓存引擎**: 支持 Alluxio、JuiceFS、Vineyard、GooseFS 等分布式缓存
- **数据感知调度**: 将计算 Pod 自动调度到数据缓存所在的节点
- **弹性伸缩**: 根据 IO 负载自动扩缩缓存集群（与 HPA 集成）
- **数据预热**: 提前从远程拉取数据到缓存层，减少首次访问延迟
- **多级缓存**: 支持 MEM + SSD/HDD 多级存储，自动分层

## 架构

Fluid 采用 Dataset + Runtime 分离设计：

- **Dataset CRD**: 声明数据集来源（mounts）、缓存策略和访问模式
- **Runtime CRD**: 缓存引擎配置（AlluxioRuntime/JindoRuntime/JuiceFSRuntime）
- **Fluid Controller**: 管理 Dataset 和 Runtime 的生命周期
- **Alluxio/Jindo Cluster**: 自动部署的分布式缓存集群（Master + Worker）
- **Fuse Recovery**: 将缓存挂载为 FUSE 文件系统供 Pod 使用
- **Scheduler Extender**: 自定义调度器扩展，实现数据感知调度

数据流：`远程存储 (S3/HDFS) → Alluxio 缓存 → FUSE → Pod 容器 → 应用读取`

## K8s 集成

Fluid 通过 CRD 扩展 Kubernetes，深度集成调度器。Dataset CRD 定义数据集和 Runtime 配置，Fluid Controller 自动部署分布式缓存集群（如 AlluxioRuntime 创建 Alluxio Master/Worker Pod）。缓存以 FUSE 方式挂载到计算 Pod 中（通过 PVC 关联 Dataset）。Fluid 的调度器扩展（Scheduler Extender）在 Pod 调度时检查节点上的数据缓存位置，优先将 Pod 调度到有缓存的节点。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 PV/PVC 机制和调度器扩展机制完全兼容。

## 生产场景

1. **AI/ML 训练加速**: 预热训练数据集到缓存层，GPU Pod 从本地缓存读取，加速训练
2. **大数据分析**: Spark/Presto 查询通过 Fluid 缓存加速 HDFS 数据访问
3. **多模型推理**: 多个推理 Pod 共享同一模型文件的缓存，减少重复下载
4. **边缘数据同步**: 在边缘节点缓存中心数据，支持离线推理

## 安装

```bash
# Helm 安装 Fluid
helm repo add fluid https://fluid-cloudnative.github.io/charts
helm install fluid fluid/fluid-fluid -n fluid-system --create-namespace

# 创建 Dataset（数据来源 S3）
kubectl apply -f - <<EOF
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: training-data
spec:
  mounts:
  - mountPoint: s3://my-bucket/training-data
    name: training-data
    options:
      alluxio.underfs.s3.endpoint: https://s3.amazonaws.com
EOF

# 创建 Alluxio Runtime（缓存引擎）
kubectl apply -f - <<EOF
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: training-data
spec:
  replicas: 3
  tieredstore:
    levels:
    - mediumtype: MEM
      path: /dev/shm
      quota: 20Gi
    - mediumtype: SSD
      path: /var/alluxio/ssd
      quota: 100Gi
EOF

# 数据预热
kubectl exec -it alluxio-master-0 -- alluxio fs distributedLoad /training-data

# 使用数据的 Pod
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: training-job
spec:
  containers:
  - name: trainer
    image: tensorflow:latest
    volumeMounts:
    - mountPath: /data
      name: training-data
  volumes:
  - name: training-data
    persistentVolumeClaim:
      claimName: training-data
EOF
```

## 对比

| 特性 | Fluid | Alluxio (standalone) | JuiceFS CSI | OpenEBS |
|------|-------|----------------------|-------------|---------|
| 数据感知调度 | ✅ | ❌ | ❌ | ❌ |
| 多缓存引擎 | ✅ | ❌ Alluxio only | ❌ JuiceFS | ❌ |
| K8s 原生 | ✅ CRD | ❌ | ✅ CSI | ✅ |
| 数据预热 | ✅ | ⚠️ 手动 | ❌ | ❌ |

## 架构定位

在 CNCF 生态中，Fluid 属于 **Data/Storage** 类别，为云原生应用提供数据集编排和加速能力。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/networkpolicy.md|[[NetworkPolicy|networkpolicy]]]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[cloudevents]] — CloudEvents
- [[keda]] — KEDA
- [[cozystack]] — Cozystack
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[vineyard]] — Vineyard

- fluid
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
