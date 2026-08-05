---
title: Karmada (entities)
description: '## 概述'
summary: 'Karmada（Kubernetes Armada）是开放的多云多集群 Kubernetes 管理系统。'
category: entities
tags:
- k8s
- cncf
- orchestration
- karmada
- etcd
- apiserver
- kubelet
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Karmada 是什么
- 如何 Karmada
trigger_keywords:
- Karmada
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Karmada

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

Karmada（Kubernetes Armada）是开放的多云多集群 Kubernetes 管理系统，由华为开源，2020 年加入 CNCF 沙箱，2023 年晋升孵化项目。它提供统一的 API 来管理跨多个 Kubernetes 集群的工作负载，支持跨集群调度、故障转移和策略驱动的资源分发。Karmada 的核心理念是"Kubernetes 管理 Kubernetes"——它的控制面本身就是一个 Kubernetes API 服务器，使用标准 kubectl 和 CRD 管理多集群应用。Karmada 的 PropagationPolicy 定义资源在哪些集群部署、各集群部署多少副本；OverridePolicy 实现跨集群配置差异化（如不同环境不同镜像版本）。它支持公有云、私有云和边缘集群的统一管理。

## 核心能力

- **多集群管理**: 统一管理多个 Kubernetes 集群（Member Cluster），支持跨云/混合云
- **跨集群调度**: 基于策略（PropagationPolicy）的工作负载分发和副本分配
- **故障转移**: 自动检测 Member Cluster 故障，将工作负载迁移到健康集群
- **Kubernetes 原生**: 完全兼容 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]]
- **集群联邦**: 统一的资源视图，支持多集群 Service 和 DNS（多集群服务发现）
- **配置覆盖**: OverridePolicy 实现跨集群配置差异化

## 架构

Karmada 采用"K8s 管理 K8s"的架构：

- **Karmada Control Plane**: 核心控制面（包含 etcd、kube-apiserver、karmada-controller-manager）
- **karmada-apiserver**: 管理面的 API Server，兼容标准 Kubernetes API
- **karmada-scheduler**: 跨集群调度器，根据 PropagationPolicy 分发工作负载
- **karmada-controller-manager**: 运行多个控制器（cluster、binding、execution 等）
- **karmada-agent**: 部署在 Member Cluster 中的 Agent（Pull 模式），注册集群到控制面
- **PropagationPolicy CRD**: 定义资源分发策略（目标集群、副本数、权重）
- **OverridePolicy CRD**: 定义跨集群配置覆盖（环境差异）

调度流程：`用户创建 Deployment → PropagationPolicy → karmada-scheduler → 分发到 Member Clusters → execution controller apply`

## K8s 集成

Karmada 的控制面本身就是一个标准的 Kubernetes API 服务器。用户通过标准 kubectl 向 Karmada 控制面提交 Deployment/StatefulSet 等资源，并通过 PropagationPolicy CRD 指定分发策略。karmada-scheduler 根据策略将工作负载分发到各 Member Cluster（通过 execution controller 向各集群的 API Server apply 资源）。Member Cluster 可以通过 karmada-agent（Pull 模式）或直接注册（Push 模式）接入。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 完全兼容——对用户而言，Karmada 就像一个拥有无限容量的单集群。

## 生产场景

1. **多云容灾**: 在多个云（AWS + 阿里云）部署应用，实现跨云容灾
2. **混合云管理**: 统一管理私有数据中心和公有云的 Kubernetes 集群
3. **边缘多区域**: 将应用分发到不同地理区域的边缘集群
4. **弹性跨集群扩容**: 主集群资源不足时，自动将工作负载溢出到备用集群

## 安装与配置

```bash
# 安装 karmadactl CLI
curl -s https://raw.githubusercontent.com/karmada-io/karmada/master/hack/install-cli.sh | bash

# 初始化 Karmada 控制面（在已有 K8s 集群上）
karmadactl init --karmada-data-store=etcd --etcd-image=registry.k8s.io/etcd:3.5

# 加入 Member Cluster
kubectl get --raw=/apis/cluster.karmada.io/v1alpha1/clusters

karmadactl join member1 --cluster-kubeconfig=/path/to/member1.kubeconfig
karmadactl join member2 --cluster-kubeconfig=/path/to/member2.kubeconfig

# 部署应用到多集群
kubectl apply -f - <<EOF
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: nginx-propagation
spec:
  resourceSelectors:
  - apiVersion: apps/v1
    kind: Deployment
    name: nginx
  placement:
    clusterAffinity:
      clusterNames: [member1, member2]
    replicaScheduling:
      replicaSchedulingType: Divided
      replicaDivisionPreference: Weighted
      weightPreference:
        staticWeightList:
        - targetCluster: {clusterNames: [member1]}
          weight: 1
        - targetCluster: {clusterNames: [member2]}
          weight: 2
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
spec:
  replicas: 6
  selector:
    matchLabels: {app: nginx}
  template:
    metadata:
      labels: {app: nginx}
    spec:
      containers:
      - image: nginx:latest
        name: nginx
EOF
```

## 运维操作

```bash
# 🟢 查看集群状态
kubectl get clusters
kubectl describe cluster member1
karmadactl get clusters

# 🟢 查看资源分发状态
kubectl get resourcebindings
kubectl get clusterresourcebindings
kubectl get works -A

# 🟡 更新分发策略
kubectl apply -f updated-propagation-policy.yaml

# 🟡 故障转移（手动）
kubectl patch cluster member1 --type=merge -p '{"spec":{"taints":[{"key":"cluster.karmada.io/unreachable","effect":"NoExecute"}]}}'

# 🔴 移除 Member Cluster
karmadactl unjoin member1 --cluster-kubeconfig=/path/to/member1.kubeconfig

# 🔴 删除 Karmada 控制面
karmadactl deinit
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 集群状态 NotReady | 网络断开/Agent 崩溃 | `kubectl describe cluster member1` | 检查网络和 Agent Pod |
| 资源未分发 | PropagationPolicy 不匹配 | `kubectl get resourcebindings` | 检查 resourceSelectors |
| 副本分配不均 | 调度策略配置错误 | `kubectl get works -A` | 调整 weightPreference |
| 故障转移未触发 | 阈值未达标 | `kubectl get clusters -o yaml` | 检查 taint 和 toleration |
| 控制面异常 | etcd 集群不健康 | `etcdctl endpoint health --cluster` | 修复 etcd 集群 |

```
排查流程:
├── 分发异常
│   ├── kubectl get clusters → 集群状态
│   ├── kubectl get resourcebindings → 绑定状态
│   ├── kubectl get works -n member1 → Work 状态
│   └── kubectl logs karmada-controller-manager → 控制器日志
├── 集群连接问题
│   ├── karmadactl get clusters → 集群健康
│   ├── 检查 Agent Pod → kubectl get pods -n karmada-system
│   └── 网络连通性 → ping/curl member API
└── 故障转移问题
    ├── 检查 taint 配置 → cluster spec
    ├── 检查 Failover 策略 → ClusterPropagationPolicy
    └── 查看事件 → kubectl get events -n karmada-system
```

## 生产案例

### 案例1: 跨地域多活部署

- **场景**: 电商应用需要在华北、华东、华南三个区域集群同时提供服务
- **排查**: 初始用单集群 + CDN，区域故障时全站不可用
- **方案**:
  1. 部署 Karmada 控制面管理 3 个区域集群
  2. 配置 Divided 调度策略，按权重分配副本
  3. 启用 Failover 策略，集群故障时自动迁移工作负载
- **效果**: 单区域故障时服务自动切换，RTO < 30s，可用性达 99.99%

### 案例2: 混合云弹性扩容

- **场景**: 私有云集群资源不足，大促期间需溢出到公有云
- **排查**: 私有云 500 节点已满，公有云集群空闲
- **方案**:
  1. 将公有云集群加入 Karmada 管理
  2. 配置 ClusterAffinity 优先级：私有云 > 公有云
  3. 设置 OverridePolicy 调整公有云副本的资源配置
- **效果**: 大促期间自动溢出 200 副本到公有云，成本降低 40%

## 对比

| 特性 | Karmada | KubeFed (deprecated) | Clusternet | Open Cluster Mgmt |
|------|---------|---------------------|------------|-------------------|
| 调度策略 | ✅ PropagationPolicy | ⚠️ 有限 | ✅ | ✅ |
| 故障转移 | ✅ | ⚠️ | ✅ | ✅ |
| K8s 原生 | ✅ | ✅ | ✅ | ✅ |
| CNCF 状态 | Incubating | Archived | Sandbox | Incubating |

## 架构定位

在 CNCF 生态中，Karmada 属于 **Orchestration** 类别，为云原生应用提供多云多集群管理能力。

## 参考链接

- [[etcd]]
- [[deployment]]
- [[operator-pattern]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[23-实体/02-K8s核心组件/kube-apiserver.md|kube-apiserver]]

## Related

- [[23-实体/02-K8s核心组件/virtual-kubelet.md|kubelet]]]] — Virtual Kubelet
- [[kudo]] — KUDO
- [[23-实体/03-运行时/01-containerd-v2-features]] — containerd 2.0 新特性
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[armada]] — Armada

- 08-multicloud-federation-karmada
- karmada
- [[22-概念/11-交叉分析/etcd × 高可用模式.md|etcd × 高可用模式]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
