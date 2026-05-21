---
title: kcp (Kubernetes-like Control Plane)
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- etcd
- rbac
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kcp (Kubernetes-like Control Plane) 是什么
- 如何 kcp (Kubernetes-like Control Plane)
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- kcp
- Kubernetes-like
- Control
- Plane
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
---

title: kcp (Kubernetes-like Control Plane)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- rbac
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- kcp (Kubernetes-like Control Plane) 是什么
- 如何 kcp (Kubernetes-like Control Plane)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- kcp
- Kubernetes-like
- Control
- Plane
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# kcp (Kubernetes-like Control Plane)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://www.kcp.io/ |
| **GitHub** | https://github.com/kcp-dev/kcp |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

kcp 是一个类 Kubernetes API 服务器，提供多租户、逻辑隔离的控制平面，不需要管理实际的容器或 Pod。它利用 Kubernetes 的 API 机制（CRD、控制器、准入控制等），将其从容器编排中解耦出来，作为通用的 API 平台使用。kcp 支持在单个服务器上运行数千个逻辑集群（Workspace），每个 Workspace 拥有独立的 API 视图和资源隔离。

### 核心特性

- **逻辑集群 (Workspace)**: 在单个 kcp 实例上创建数千个隔离的 API Workspace
- **透明多集群**: 将工作负载透明调度到多个物理 Kubernetes 集群
- **API 导出/绑定**: 服务提供者可以导出 API，消费者通过绑定使用
- **CRD 兼容**: 完整支持 Kubernetes CRD 和控制器模式
- **高密度多租户**: 比创建完整 K8s 集群更轻量的租户隔离方案
- **SyncTarget**: 将 kcp 中的资源同步到实际的 Kubernetes 集群执行

---

## 架构设计

```
┌─────────────────────────────────────────────────────┐
│                   kcp Server                         │
│                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐         │
│  │Workspace │  │Workspace │  │Workspace │  ...     │
│  │  Tenant A│  │  Tenant B│  │  Tenant C│         │
│  │  ┌─────┐ │  │  ┌─────┐ │  │  ┌─────┐ │         │
│  │  │ CRDs│ │  │  │ CRDs│ │  │  │ CRDs│ │         │
│  │  │ APIs│ │  │  │ APIs│ │  │  │ APIs│ │         │
│  │  └─────┘ │  │  └─────┘ │  │  └─────┘ │         │
│  └──────────┘  └──────────┘  └──────────┘         │
│                                                     │
│  ┌─────────────────────────────────────────┐       │
│  │         API Export / Binding             │       │
│  └─────────────────────────────────────────┘       │
│                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐         │
│  │ Syncer A │  │ Syncer B │  │ Syncer C │         │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘         │
└───────┼──────────────┼──────────────┼───────────────┘
        │              │              │
   ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
   │ K8s     │   │ K8s     │   │ K8s     │
   │Cluster 1│   │Cluster 2│   │Cluster 3│
   └─────────┘   └─────────┘   └─────────┘
```

---

## 快速开始

### 安装 kcp

```bash
# 下载最新版本
curl -LO "https://github.com/kcp-dev/kcp/releases/latest/download/kcp_$(uname -s)_$(uname -m).tar.gz"
tar xzf kcp_*.tar.gz

# 启动 kcp 服务器
./bin/kcp start
```

### 创建 Workspace

```bash
# 配置 kubeconfig 指向 kcp
export KUBECONFIG=.kcp/admin.kubeconfig

# 创建组织 Workspace
kubectl kcp workspace create my-org --type=organization --enter

# 创建应用 Workspace
kubectl kcp workspace create my-app --enter

# 查看当前 Workspace
kubectl kcp workspace .
```

### 使用 CRD

```yaml
# 在 Workspace 中创建 CRD，与标准 K8s CRD 完全兼容
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: widgets.example.com
spec:
  group: example.com
  versions:
    - name: v1alpha1
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                size:
                  type: string
                color:
                  type: string
  scope: Namespaced
  names:
    plural: widgets
    singular: widget
    kind: Widget
```

---

## API 导出与绑定

### 服务提供者导出 API

```yaml
apiVersion: apis.kcp.io/v1alpha1
kind: APIExport
metadata:
  name: widgets.example.com
spec:
  latestResourceSchemas:
    - v1alpha1.widgets.example.com
  permissionClaims:
    - group: ""
      resource: "configmaps"
      identityHash: ""
```

### 消费者绑定 API

```yaml
apiVersion: apis.kcp.io/v1alpha1
kind: APIBinding
metadata:
  name: widgets
spec:
  reference:
    export:
      path: "root:my-org:platform"
      name: widgets.example.com
  permissionClaims:
    - group: ""
      resource: "configmaps"
      state: Accepted
```

---

## 多集群同步

### 注册物理集群

```yaml
apiVersion: workload.kcp.io/v1alpha1
kind: SyncTarget
metadata:
  name: production-cluster
spec:
  kubeconfig: |
    # 目标集群的 kubeconfig
```

### 配置 Placement

```yaml
apiVersion: scheduling.kcp.io/v1alpha1
kind: Placement
metadata:
  name: default-placement
spec:
  locationSelectors:
    - matchLabels:
        env: production
  locationResource:
    group: workload.kcp.io
    version: v1alpha1
    resource: synctargets
```

---

## 与传统方案对比

| 特性 | kcp | vCluster | 多集群联邦 |
|:---|:---|:---|:---|
| 资源开销 | 极低 (逻辑隔离) | 中 (虚拟集群) | 高 (独立集群) |
| API 兼容 | Kubernetes API | 完整 K8s | 完整 K8s |
| 租户密度 | 数千/实例 | 数百/集群 | 1:1 |
| 工作负载执行 | 同步到物理集群 | 同步到宿主集群 | 直接执行 |
| API 共享 | Export/Binding | 不支持 | 有限 |
| 适用场景 | SaaS 平台/API 平台 | 开发/测试 | 生产隔离 |

---

## 最佳实践

1. **Workspace 层级设计**: 使用组织级 Workspace 管理团队边界，应用级 Workspace 管理服务
2. **API 版本管理**: 通过 APIExport 版本化你的平台 API，确保向后兼容
3. **Syncer 高可用**: 为每个物理集群部署多副本 Syncer 保证同步可靠性
4. **RBAC 策略**: 利用 kcp 的多租户 RBAC 实现最小权限原则
5. **监控**: 监控 kcp 的 etcd 存储使用和 API 请求延迟

---

## 参考资源

- [kcp 官方文档](https://docs.kcp.io/)
- [kcp GitHub](https://github.com/kcp-dev/kcp)
- [kcp 社区](https://github.com/kcp-dev/kcp/discussions)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/kcp.md|kcp]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
