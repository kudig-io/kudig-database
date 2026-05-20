---
title: Cozystack
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- prometheus
- grafana
- helm
- argocd
- flux
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Cozystack 是什么
- 如何 Cozystack
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Cozystack
- cncf
- landscape
---

# Cozystack

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://cozystack.io/ |
| **GitHub** | https://github.com/aenix-io/cozystack |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Cozystack 是一个开源的 PaaS 平台，基于 Kubernetes 构建，旨在提供类似云厂商的托管服务体验。它允许平台工程师在裸金属或任何基础设施上快速搭建一个完整的云平台，提供托管 Kubernetes 集群、数据库（PostgreSQL、MySQL、Redis）、消息队列、监控等服务。Cozystack 使用 FluxCD 实现 GitOps 管理，通过 Talos Linux 作为节点操作系统。

### 核心特性

- **一键平台**: 在裸金属上快速搭建包含计算、存储、网络的完整云平台
- **托管 K8s**: 提供自服务的 Kubernetes 集群创建和管理
- **托管服务**: PostgreSQL、MySQL、Redis、Kafka、RabbitMQ 等即开即用
- **FluxCD GitOps**: 全平台通过 GitOps 方式管理配置
- **Talos Linux**: 使用不可变的 Talos Linux 作为节点 OS
- **多租户**: 内置租户隔离和资源配额管理
- **Dashboard**: 自服务 Web 控制台

---

## 快速开始

### 安装

```bash
# 在 Talos Linux 集群上安装 Cozystack
talosctl apply-config --insecure --nodes <node-ip> --file controlplane.yaml

# 安装 Cozystack
kubectl apply -f https://cozystack.io/install.yaml

# 或使用 Helm
helm repo add cozystack https://aenix-io.github.io/cozystack/
helm install cozystack cozystack/cozystack \
  --namespace cozystack-system \
  --create-namespace
```

### 创建托管 Kubernetes 集群

```yaml
apiVersion: cozystack.io/v1alpha1
kind: KubernetesCluster
metadata:
  name: dev-cluster
  namespace: tenant-dev
spec:
  version: "1.29"
  controlPlane:
    replicas: 3
    machineType: "medium"
  workers:
    - name: default
      replicas: 3
      machineType: "large"
  networking:
    podCIDR: "10.244.0.0/16"
    serviceCIDR: "10.96.0.0/12"
```

### 创建托管数据库

```yaml
apiVersion: cozystack.io/v1alpha1
kind: PostgreSQL
metadata:
  name: app-db
  namespace: tenant-dev
spec:
  version: "16"
  replicas: 3
  storage:
    size: 100Gi
    storageClass: ceph-rbd
  resources:
    requests:
      cpu: "2"
      memory: "4Gi"
  backup:
    schedule: "0 2 * * *"
    retention: 7
```

---

## 平台服务目录

| 服务 | 类型 | 高可用 | 备份 |
|:---|:---|:---|:---|
| Kubernetes | 容器编排 | 多控制面 | etcd 快照 |
| PostgreSQL | 关系数据库 | 主从复制 | PITR |
| MySQL | 关系数据库 | 主从复制 | 定期快照 |
| Redis | 缓存/KV | Sentinel/Cluster | RDB |
| Kafka | 消息队列 | 多副本 | - |
| RabbitMQ | 消息队列 | 镜像队列 | - |
| Monitoring | 监控 | - | - |

---

## 与其他方案对比

| 特性 | Cozystack | Rancher | OpenShift | Gardener |
|:---|:---|:---|:---|:---|
| 定位 | 裸金属 PaaS | K8s 管理 | 企业 PaaS | K8s 托管 |
| 节点 OS | Talos (不可变) | 任意 Linux | RHCOS | 任意 |
| 托管服务 | DB/MQ/监控 | 有限 | Operator Hub | 不内置 |
| GitOps | FluxCD | Fleet | ArgoCD | 不内置 |
| 适用场景 | 私有云/裸金属 | 多云 K8s | 企业平台 | 多云 K8s |

---

## 最佳实践

1. **基础设施规划**: 预先规划存储（Ceph）和网络拓扑
2. **租户隔离**: 为每个团队创建独立的租户命名空间和资源配额
3. **GitOps 管理**: 将所有平台配置纳入 Git 仓库管理
4. **监控**: 利用内置 Prometheus/Grafana 监控平台和租户服务
5. **备份策略**: 为所有有状态服务配置定期备份

---

## 参考资源

- [Cozystack 官方文档](https://cozystack.io/docs/)
- [Cozystack GitHub](https://github.com/aenix-io/cozystack)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
