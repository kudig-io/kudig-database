---
title: KubeSlice
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- istio
- cilium
- helm
- ingress
- gateway
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KubeSlice 是什么
- 如何 KubeSlice
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KubeSlice
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- cilium-basics
---

title: KubeSlice
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- istio
- cilium
- helm
- ingress
- gateway
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KubeSlice 是什么
- 如何 KubeSlice
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KubeSlice
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
# KubeSlice

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kubeslice.io/ |
| **GitHub** | https://github.com/kubeslice |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KubeSlice 是一个多集群网络平台，通过创建逻辑 Slice（网络切片）覆盖层，在多个 Kubernetes 集群之间建立扁平的、安全的网络连接。每个 Slice 提供独立的网络命名空间、QoS 策略和安全隔离，使跨集群的应用能够像在同一集群内一样通信，同时保持网络隔离和带宽保障。

### 核心特性

- **网络切片**: 创建逻辑 Slice 覆盖网络，跨集群提供独立的网络平面
- **多集群连接**: 基于 VPN 隧道（WireGuard/IPsec）建立集群间安全互联
- **QoS 保障**: 为每个 Slice 配置带宽限制、优先级和流量整形策略
- **服务发现**: 跨集群自动服务发现和 DNS 解析
- **网络隔离**: 不同 Slice 之间的流量完全隔离，支持零信任网络模型
- **Namespace 映射**: 自动管理跨集群命名空间的关联和同步

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│              KubeSlice Controller                     │
│              (管理集群)                               │
│                                                       │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────┐ │
│  │ Slice Config  │  │  Cluster     │  │  Service   │ │
│  │ Controller    │  │  Registration│  │  Export/   │ │
│  │              │  │  Manager     │  │  Import    │ │
│  └──────┬───────┘  └──────┬───────┘  └─────┬──────┘ │
└─────────┼─────────────────┼─────────────────┼────────┘
          │                 │                 │
    ┌─────▼──────────────────────────────────────┐
    │            Slice Overlay Network            │
    │         (WireGuard/IPsec Tunnels)           │
    └─────┬──────────────────────────────┬───────┘
          │                              │
┌─────────▼──────────┐        ┌─────────▼──────────┐
│  Worker Cluster 1   │        │  Worker Cluster 2   │
│  ┌───────────────┐  │        │  ┌───────────────┐  │
│  │ Slice Operator │  │        │  │ Slice Operator │  │
│  └───────┬───────┘  │        │  └───────┬───────┘  │
│  ┌───────▼───────┐  │        │  ┌───────▼───────┐  │
│  │ Slice Gateway  │  │◄──────►│  │ Slice Gateway  │  │
│  │ (VPN Tunnel)   │  │        │  │ (VPN Tunnel)   │  │
│  └───────┬───────┘  │        │  └───────┬───────┘  │
│  ┌───────▼───────┐  │        │  ┌───────▼───────┐  │
│  │ Slice vL3     │  │        │  │ Slice vL3     │  │
│  │ (NSM)         │  │        │  │ (NSM)         │  │
│  └───────┬───────┘  │        │  └───────┬───────┘  │
│  ┌───────▼───────┐  │        │  ┌───────▼───────┐  │
│  │  App Pods      │  │        │  │  App Pods      │  │
│  │  (Slice NS)    │  │        │  │  (Slice NS)    │  │
│  └───────────────┘  │        │  └───────────────┘  │
└─────────────────────┘        └─────────────────────┘
```

---

## 快速开始

### 安装 KubeSlice Controller

```bash
# 在管理集群上安装 Controller
helm repo add kubeslice https://kubeslice.github.io/kubeslice/
helm install kubeslice-controller kubeslice/kubeslice-controller \
  --namespace kubeslice-controller \
  --create-namespace \
  --set kubesliceController.endpoint=https://controller-api:443
```

### 注册 Worker 集群

```yaml
# cluster-registration.yaml
apiVersion: controller.kubeslice.io/v1alpha1
kind: Cluster
metadata:
  name: worker-cluster-1
  namespace: kubeslice-project
spec:
  networkInterface: eth0
  clusterProperty:
    geoLocation:
      cloudProvider: aws
      cloudRegion: us-west-2
```

```bash
# 在每个 Worker 集群上安装 Slice Operator
helm install kubeslice-worker kubeslice/kubeslice-worker \
  --namespace kubeslice-system \
  --create-namespace \
  --set kubesliceNetworking.enabled=true \
  --set cluster.name=worker-cluster-1 \
  --set cluster.endpoint=https://worker-1-api:6443 \
  --set controllerSecret.namespace=kubeslice-controller \
  --set controllerSecret.endpoint=https://controller-api:443
```

### 创建 Slice

```yaml
# slice-config.yaml
apiVersion: controller.kubeslice.io/v1alpha1
kind: SliceConfig
metadata:
  name: app-slice
  namespace: kubeslice-project
spec:
  sliceSubnet: 10.1.0.0/16
  sliceType: Application
  sliceGatewayProvider:
    sliceGatewayType: OpenVPN  # 或 WireGuard
    sliceCaType: Local
  sliceIpamType: Local
  clusters:
    - worker-cluster-1
    - worker-cluster-2
  qosProfileDetails:
    queueType: HTB
    bandwidthCeilingKbps: 10240  # 10 Mbps 带宽上限
    bandwidthGuaranteedKbps: 5120  # 5 Mbps 保障带宽
    priority: 1
    tcType: BANDWIDTH_CONTROL
    dscpClass: AF11
  namespaceIsolationProfile:
    applicationNamespaces:
      - namespace: my-app
        clusters:
          - worker-cluster-1
          - worker-cluster-2
    isolationEnabled: true
```

```bash
kubectl apply -f slice-config.yaml
```

---

## 高级功能

### 跨集群服务发现

```yaml
# service-export.yaml - 在 worker-cluster-1 上导出服务
apiVersion: networking.kubeslice.io/v1beta1
kind: ServiceExport
metadata:
  name: backend-api
  namespace: my-app
spec:
  slice: app-slice
  selector:
    matchLabels:
      app: backend-api
  ports:
    - name: http
      port: 8080
      protocol: TCP
  ingressEnabled: false
```

```yaml
# 在 worker-cluster-2 上，服务自动可通过 DNS 发现
# backend-api.my-app.svc.slice.local
# 应用可直接访问跨集群服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
  namespace: my-app
spec:
  template:
    spec:
      containers:
        - name: frontend
          image: my-frontend:latest
          env:
            - name: BACKEND_URL
              value: "http://backend-api.my-app.svc.slice.local:8080"
```

### QoS 策略配置

```yaml
# qos-profile.yaml
apiVersion: controller.kubeslice.io/v1alpha1
kind: SliceQoSConfig
metadata:
  name: premium-qos
  namespace: kubeslice-project
spec:
  # 带宽控制
  bandwidthCeilingKbps: 102400    # 100 Mbps 上限
  bandwidthGuaranteedKbps: 51200  # 50 Mbps 保障
  # 优先级和 DSCP 标记
  priority: 0  # 最高优先级
  dscpClass: EF  # Expedited Forwarding
  # 流量整形
  tcType: BANDWIDTH_CONTROL
  queueType: HTB
```

### 网络策略隔离

```yaml
# slice-network-policy.yaml
apiVersion: controller.kubeslice.io/v1alpha1
kind: SliceConfig
metadata:
  name: isolated-slice
spec:
  sliceSubnet: 10.2.0.0/16
  clusters:
    - worker-cluster-1
    - worker-cluster-2
  namespaceIsolationProfile:
    isolationEnabled: true
    allowedNamespaces:
      - namespace: monitoring
        clusters:
          - "*"  # 允许监控命名空间访问
  externalGatewayConfig:
    - ingress:
        enabled: true
      egress:
        enabled: false  # 禁止 Slice 内流量出站
      gatewayType: istio
```

---

## 与其他方案对比

| 特性 | KubeSlice | Submariner | Cilium CM | Skupper |
|:---|:---|:---|:---|:---|
| 网络切片 | 独立 Slice 隔离 | 单一隧道 | ClusterMesh | 虚拟网络 |
| QoS 保障 | 带宽/优先级/DSCP | 不支持 | 有限 | 不支持 |
| VPN 隧道 | WireGuard/OpenVPN | IPsec/VXLAN | WireGuard | 无隧道 |
| 服务发现 | 自动 DNS | ServiceImport | 集群服务 | 自动 |
| 网络隔离 | Slice 级隔离 | 集群级 | 策略级 | 服务级 |
| NSM 集成 | 原生支持 | 不支持 | 不支持 | 不支持 |

---

## 最佳实践

1. **Slice 规划**: 按业务域划分 Slice，每个 Slice 服务于一组关联的微服务
2. **QoS 配置**: 为关键业务 Slice 配置带宽保障，避免非关键流量抢占
3. **网络隔离**: 启用 namespaceIsolation 确保 Slice 间的安全隔离
4. **网关选择**: 低延迟场景使用 WireGuard，兼容性优先使用 OpenVPN
5. **监控**: 监控各 Slice 的带宽利用率、延迟和网关隧道状态

---

## 参考资源

- [KubeSlice 官方文档](https://kubeslice.io/documentation/)
- [KubeSlice GitHub](https://github.com/kubeslice)
- [KubeSlice Worker Operator](https://github.com/kubeslice/worker-operator)
- [Network Service Mesh](https://networkservicemesh.io/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
