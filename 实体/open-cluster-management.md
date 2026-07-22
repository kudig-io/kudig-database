---
title: Open Cluster Management (OCM)
description: '## 概述'
summary: 'Open Cluster Management (OCM) 是一个社区驱动的多集群管理平台，提供 Kubernetes 多集群编排的核心能力。OCM 采用 Hub-Spoke 架构，通过轻量级的代理模型实现集群注册、工作负载分发、策略治理和应用生命周期管理。'
category: entities
tags:
- k8s
- cncf
- orchestration
- open-cluster-management
- prometheus
- grafana
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Open Cluster Management (OCM) 是什么
- 如何 Open Cluster Management (OCM)
trigger_keywords:
- Open
- Cluster
- Management
- OCM
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[实体/open-cluster-management.md|Open Cluster Management]] (OCM)

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Open Cluster Management（OCM）是由 Red Hat 开源的多集群管理平台，2021 年加入 CNCF Sandbox。OCM 采用 Hub-Spoke 架构，通过轻量级的代理模型实现集群注册、工作负载分发、策略治理和应用生命周期管理。与其他多集群方案不同，OCM 设计了清晰的 Cluster API、Placement API 和 ManifestWork API，使多集群管理变得声明式和可扩展。它是 Red Hat Advanced Cluster Management（ACM）的开源上游项目。

## 核心特性

- **Hub-Spoke 架构**: Hub 集群集中管理，Klusterlet 代理注册到 Spoke 集群
- **集群注册**: ManagedCluster API 管理集群注册和状态上报
- **工作负载分发**: ManifestWork API 将 K8s 资源分发到托管集群
- **智能调度**: Placement API 支持按标签、拓扑、亲和性选择目标集群
- **策略治理**: Policy 框架支持配置合规检查和安全策略分发
- **Addon 框架**: 可扩展的 Addon 机制，支持自定义功能扩展

## 架构

OCM 采用 Hub-Agent 架构。Hub 集群运行 Registration Operator、Placement Controller 和 Cluster Manager。每个被管集群运行 Klusterlet（包含 Registration Agent 和 Work Agent）。Registration Agent 负责集群注册和证书管理；Work Agent 负责从 Hub 拉取 ManifestWork 并在本地集群应用。Placement Controller 根据 Placement 规则从 ManagedClusterSet 中选择目标集群。所有交互通过 CRD 声明式定义，Hub 不直接访问 Spoke 的 API Server，而是通过 ManifestWork 下发操作。

## Kubernetes 集成

OCM 完全基于 Kubernetes 原生 API 设计。ManagedCluster、ManagedClusterSet、Placement、ManifestWork 均为 CRD。Klusterlet 以 Deployment 形式部署在被管集群中，通过 Lease 机制保持心跳。Addon 框架允许第三方扩展（如observability addon）作为 Kubernetes Controller 运行。策略（Policy）框架通过 Gatekeeper/OPA 或自定义控制器实现合规检查。

## 生产使用场景

1. **多集群应用分发**: 将应用统一部署到开发、测试、生产集群
2. **策略合规管理**: 跨集群统一分发安全策略和配置基线
3. **边缘集群管理**: 管理大量边缘 Kubernetes 集群的生命周期
4. **灾难恢复**: 在多个集群间分发工作负载，实现故障切换

## 安装与配置

```bash
# 安装 clusteradm CLI
curl -L https://raw.githubusercontent.com/open-cluster-management-io/clusteradm/main/install.sh | bash

# Hub 集群初始化
clusteradm init --wait
# 获取 join 命令 (包含 token)
clusteradm get token

# 注册 Spoke 集群
clusteradm join \
  --hub-token <token> \
  --hub-apiserver https://<hub-api>:6443 \
  --cluster-name spoke-1 \
  --wait

# 在 Hub 接受集群注册
clusteradm accept --clusters spoke-1

# 验证注册
kubectl get managedclusters
kubectl get managedclustersets

# Helm 安装 (替代方式)
helm repo add ocm https://open-cluster-management.io/helm-charts
helm install cluster-manager ocm/cluster-manager -n open-cluster-management --create-namespace
```

```yaml
# ManifestWork 示例 - 分发工作负载到 Spoke 集群
apiVersion: work.open-cluster-management.io/v1
kind: ManifestWork
metadata:
  name: nginx-deployment
  namespace: spoke-1  # 目标集群名称
spec:
  workload:
    manifests:
      - apiVersion: apps/v1
        kind: Deployment
        metadata:
          name: nginx
          namespace: default
        spec:
          replicas: 3
          selector:
            matchLabels:
              app: nginx
          template:
            metadata:
              labels:
                app: nginx
            spec:
              containers:
                - name: nginx
                  image: nginx:1.25
---
# Placement 示例 - 智能选择目标集群
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: Placement
metadata:
  name: prod-placement
  namespace: default
spec:
  numberOfClusters: 2
  clusterSets:
    - production
  predicates:
    - requiredClusterSelector:
        labelSelector:
          matchLabels:
            environment: production
  prioritizerPolicy:
    configurations:
      - scoreCoordinate:
          type: BuiltIn
          builtIn: ResourceAllocatableCPU
        weight: 1
```

## 运维操作

```bash
# 🟢 检查 Hub 组件状态
kubectl get pods -n open-cluster-management
kubectl get pods -n open-cluster-management-hub

# 🟢 检查托管集群状态
kubectl get managedclusters
kubectl describe managedcluster spoke-1

# 🟢 检查 ManifestWork 状态
kubectl get manifestworks -A
kubectl describe manifestwork nginx-deployment -n spoke-1

# 🟢 检查 Placement 决策
kubectl get placementdecisions -A
kubectl describe placementdecision <name> -n <ns>

# 🟢 检查 Klusterlet 状态 (Spoke 集群)
kubectl get pods -n open-cluster-management-agent
kubectl get klusterlet

# 🟡 分离托管集群
clusteradm unjoin --cluster-name spoke-1

# 🟢 检查 Addon 状态
kubectl get managedclusteraddons -A
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 集群注册失败 | Token 过期/网络不通 | 检查 Klusterlet 日志 | 重新获取 Token |
| ManagedCluster 不可用 | Klusterlet 未运行 | `kubectl get pods -n open-cluster-management-agent` | 重启 Klusterlet |
| ManifestWork 未应用 | Work Agent 异常 | `kubectl describe manifestwork` | 检查 Work Agent 日志 |
| Placement 无决策 | 无匹配集群 | `kubectl get placementdecisions` | 检查集群标签/ClusterSet |
| 心跳丢失 | 网络中断/Lease 过期 | 检查 ManagedCluster 状态 | 检查网络连通性 |
| Addon 未就绪 | Addon 部署失败 | `kubectl get managedclusteraddons` | 检查 Addon Pod 状态 |

### 排查流程

```
OCM 多集群异常
├── 集群注册失败
│   ├── 检查 Hub 集群 Registration Service 状态
│   ├── 检查 Token 有效性
│   ├── 检查 Spoke 到 Hub 网络连通性
│   └── 检查 Klusterlet Pod 日志
├── 工作负载分发失败
│   ├── kubectl get manifestworks → 检查状态
│   ├── kubectl describe manifestwork → 查看事件
│   ├── 检查 Spoke 集群 Work Agent
│   └── 检查目标命名空间 RBAC
└── Placement 无决策
    ├── 检查 ManagedClusterSet 绑定
    ├── 检查集群标签匹配
    └── 检查 Placement 谓词配置
```

## 生产案例

### 案例 1: 多集群应用统一分发

- **场景**: 3 个区域集群需要统一部署应用，手动 kubectl apply 容易遗漏
- **排查**: 各集群应用版本不一致；配置漂移难以发现
- **方案**: 部署 OCM Hub；3 个集群注册为 Spoke；ManifestWork 统一分发应用；Placement 按区域选择集群
- **效果**: 应用分发时间从 30 分钟降至 2 分钟；配置一致性 100%

### 案例 2: 边缘集群策略合规管理

- **场景**: 50 个边缘集群需要统一安全策略，手动检查不现实
- **排查**: 部分边缘集群未应用最新安全策略；合规审计困难
- **方案**: OCM Policy 框架分发 NetworkPolicy/PSA 配置；定期合规检查；不合规自动告警
- **效果**: 策略分发自动化；合规率从 60% 提升至 98%

## 对比与替代方案

| 维度 | OCM | Karmada | ArgoCD+AppSet | Clusternet |
|------|-----|---------|---------------|------------|
| 架构 | Hub-Spoke | Hub-Spoke | GitOps | Hub-Spoke |
| 调度能力 | Placement | 强 (副本调度) | 无 | 中 |
| 策略治理 | ✅ Policy | 部分 | ❌ | 部分 |
| 轻量级 | ✅ | 中 | ✅ | ✅ |
| CNCF 状态 | Sandbox | Incubating | Graduated | Sandbox |
| 适用场景 | 多集群管理 | 应用分发 | GitOps | 边缘 |

## 检查清单

- [ ] Hub 集群组件全部 Running
- [ ] Spoke 集群 Klusterlet 正常运行
- [ ] ManagedCluster 状态 Available
- [ ] ManifestWork 应用成功
- [ ] Placement 决策正确
- [ ] 网络连通性验证 (Hub ↔ Spoke)
- [ ] RBAC 权限配置正确
- [ ] 监控覆盖集群健康状态

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[fluid]] — Fluid
- [[kuasar]] — Kuasar
- [[longhorn]] — Longhorn
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference

<!-- risk-assessed -->
