---
title: Network Service Mesh (NSM)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- istio
- envoy
- helm
- daemonset
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Network Service Mesh (NSM) 是什么
- 如何 Network Service Mesh (NSM)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Network
- Service
- Mesh
- NSM
- cncf
- landscape
cross_refs:
- type: fta
  path: ../topic-fta/list/service-fta.md
  label: '故障树: service'
---


# Network Service Mesh (NSM)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://networkservicemesh.io/ |
| **GitHub** | https://github.com/networkservicemesh |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |
| **最新版本** | v1.13+ |

---

## 项目概述

Network Service Mesh (NSM) 是一个混合/多云的 IP 服务网格，提供 L2/L3 层的网络服务连接能力。与传统的 Service Mesh（如 Istio、Linkerd 专注于 L4-L7）不同，NSM 专注于为应用提供底层网络服务，例如安全隧道、VPN、防火墙等网络功能的动态连接。

### 核心特性

- **L2/L3 网络服务**: 提供比传统 Service Mesh 更低层的网络连接
- **动态网络服务发现**: 自动发现并连接网络服务端点
- **跨集群/跨云连接**: 支持多集群和混合云环境的网络互联
- **零信任安全模型**: 基于 SPIFFE/SPIRE 的工作负载身份认证
- **与 CNI 无关**: 可与任何 CNI 插件共存，不替换现有网络
- **拓扑感知**: 支持本地、远程和跨域的网络服务连接
- **Kernel 和 VPP 数据平面**: 支持 Linux Kernel 和 FD.io VPP 转发

---

## 架构设计

```
┌────────────────────────────────────────────────────────┐
│                    Kubernetes Cluster                    │
│                                                         │
│  ┌─────────────────┐        ┌─────────────────────┐   │
│  │   NSM Client     │        │  Network Service     │   │
│  │   (Application)  │        │  Endpoint (NSE)      │   │
│  │                  │        │                      │   │
│  │  ┌────────────┐ │        │  ┌────────────────┐  │   │
│  │  │ NSC Sidecar│ │◄──────►│  │  NSE Container │  │   │
│  │  └────────────┘ │  Data  │  └────────────────┘  │   │
│  └────────┬────────┘  Path  └──────────┬───────────┘   │
│           │                            │                │
│  ┌────────┴────────────────────────────┴───────────┐   │
│  │              NSMgr (Node Manager)                │   │
│  │         Per-Node DaemonSet                       │   │
│  │                                                  │   │
│  │  ┌──────────┐ ┌───────────┐ ┌────────────────┐ │   │
│  │  │ Registry │ │ Forwarder │ │ SPIRE Agent    │ │   │
│  │  │ Client   │ │ (VPP/Kern)│ │ (mTLS/Auth)    │ │   │
│  │  └──────────┘ └───────────┘ └────────────────┘ │   │
│  └──────────────────┬──────────────────────────────┘   │
│                     │                                   │
│  ┌──────────────────┴──────────────────────────────┐   │
│  │           NSM Registry                           │   │
│  │     (Service Discovery & Registration)           │   │
│  └──────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 说明 |
|:---|:---|
| **NSMgr** | 节点级管理器（DaemonSet），处理网络服务请求和连接 |
| **NSC (Client)** | 网络服务客户端，请求网络连接的应用 Sidecar |
| **NSE (Endpoint)** | 网络服务端点，提供具体网络功能的实现 |
| **Registry** | 服务注册中心，管理网络服务的发现与注册 |
| **Forwarder** | 数据平面转发器，支持 Kernel 和 VPP 两种模式 |
| **SPIRE** | 身份认证和 mTLS 证书管理 |

---

## 快速开始

### 安装 NSM

```bash
# 使用 Helm 安装 NSM
helm repo add nsm https://helm.nsm.dev/
helm repo update

# 安装 NSM 核心组件
helm install nsm nsm/nsm \
  --namespace nsm-system \
  --create-namespace \
  --set spire.enabled=true \
  --set forwarder.type=kernel
```

### 部署网络服务示例

```yaml
# 定义网络服务
apiVersion: networkservicemesh.io/v1
kind: NetworkService
metadata:
  name: secure-tunnel
  namespace: default
spec:
  payload: ETHERNET
  matches:
    - routes:
        - destination_selector:
            app: firewall
      metadata:
        labels:
          via: firewall
    - routes:
        - destination_selector:
            app: vpn-gateway
```

### 部署 NSE（网络服务端点）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vpn-gateway-nse
  labels:
    app: vpn-gateway
spec:
  replicas: 1
  selector:
    matchLabels:
      app: vpn-gateway
  template:
    metadata:
      labels:
        app: vpn-gateway
      annotations:
        networkservicemesh.io/impl: "secure-tunnel"
    spec:
      containers:
        - name: vpn-gateway
          image: ghcr.io/networkservicemesh/cmd-nse-icmp-responder:latest
          env:
            - name: NSM_CONNECT_TO
              value: "unix:///var/lib/networkservicemesh/nsm.io.sock"
            - name: NSM_CIDR_PREFIX
              value: "172.16.1.0/24"
            - name: NSM_SERVICE_NAMES
              value: "secure-tunnel"
          volumeMounts:
            - name: nsm-socket
              mountPath: /var/lib/networkservicemesh
      volumes:
        - name: nsm-socket
          hostPath:
            path: /var/lib/networkservicemesh
            type: DirectoryOrCreate
```

### 部署 NSC（客户端应用）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-application
spec:
  replicas: 1
  selector:
    matchLabels:
      app: my-application
  template:
    metadata:
      labels:
        app: my-application
      annotations:
        # 请求网络服务连接
        networkservicemesh.io/svc: "secure-tunnel"
    spec:
      containers:
        - name: app
          image: alpine:latest
          command: ["sleep", "infinity"]
```

---

## 高级配置

### 跨集群网络服务

```yaml
# Cluster A - 注册远程网络服务
apiVersion: networkservicemesh.io/v1
kind: NetworkService
metadata:
  name: cross-cluster-svc
spec:
  payload: IP
  matches:
    - routes:
        - destination_selector:
            cluster: cluster-b
---
# 使用 Interdomain NSM 组件
# 部署 registry-proxy-dns 实现跨集群服务发现
apiVersion: apps/v1
kind: Deployment
metadata:
  name: registry-proxy-dns
  namespace: nsm-system
spec:
  template:
    spec:
      containers:
        - name: registry-proxy-dns
          image: ghcr.io/networkservicemesh/cmd-registry-proxy-dns:latest
          env:
            - name: NSM_DOMAIN
              value: "cluster-a.example.com"
            - name: NSM_LISTEN_ON
              value: ":5053"
```

### VPP 数据平面配置

```yaml
# 使用 VPP 替代 Kernel 转发获得更高性能
helm install nsm nsm/nsm \
  --namespace nsm-system \
  --set forwarder.type=vpp \
  --set forwarder.vpp.resources.limits.memory=512Mi \
  --set forwarder.vpp.resources.limits.cpu=500m
```

### 多网络接口请求

```yaml
# 客户端请求多个网络服务连接
metadata:
  annotations:
    networkservicemesh.io/svc: |
      secure-tunnel?color=blue
      monitoring-network?interface=nsm1
```

---

## 使用场景

### 场景一：安全 VPN 隧道

```
Application Pod ──► NSC ──► NSMgr ──► VPN NSE ──► Remote Network
                          (mTLS)      (IPsec/WG)
```

适用于需要将 Pod 安全连接到外部网络的场景，例如数据库访问、遗留系统集成。

### 场景二：网络功能链 (Service Function Chaining)

```
Client ──► Firewall NSE ──► IDS NSE ──► Load Balancer NSE ──► Backend
```

将多个网络功能串联，实现流量的安全检查和处理。

### 场景三：混合云互联

```
AWS Cluster ──► NSM Registry ◄── On-Prem Cluster
                    │
            ┌───────┴───────┐
            │ Secure Tunnel  │
            │ (Cross-Cloud)  │
            └───────────────┘
```

---

## 监控与可观测性

### Prometheus 指标

```yaml
# NSM 组件暴露 Prometheus 指标
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: nsm-metrics
  namespace: nsm-system
spec:
  selector:
    matchLabels:
      app: nsm
  endpoints:
    - port: metrics
      interval: 30s
      path: /metrics
```

### 关键指标

| 指标 | 说明 |
|:---|:---|
| `nsm_connection_count` | 当前活跃连接数 |
| `nsm_request_duration_seconds` | 连接请求耗时 |
| `nsm_heal_count` | 连接自愈次数 |
| `nsm_close_count` | 连接关闭总数 |
| `nsm_registry_entries` | 注册的网络服务数量 |

---

## 与传统 Service Mesh 对比

| 特性 | NSM | Istio/Linkerd |
|:---|:---|:---|
| **OSI 层级** | L2/L3 | L4/L7 |
| **主要功能** | 网络连接、隧道、VPN | 流量管理、可观测性 |
| **数据平面** | Kernel/VPP | Envoy |
| **与 CNI 关系** | 共存互补 | 依赖 CNI |
| **典型场景** | 跨网络互联、NFV | 微服务通信治理 |
| **协议支持** | 任意 L2/L3 协议 | HTTP/gRPC/TCP |

---

## 最佳实践

1. **安全优先**: 始终启用 SPIRE 进行工作负载身份认证和 mTLS 加密
2. **数据平面选择**: 低延迟/高吞吐场景使用 VPP，常规场景使用 Kernel
3. **资源规划**: 每个节点的 NSMgr 需要预留足够的 CPU 和内存
4. **网络规划**: 提前规划 NSM 使用的 IP 地址段，避免与集群网络冲突
5. **跨集群部署**: 使用 DNS 代理实现跨集群服务发现，确保集群间网络可达
6. **故障恢复**: NSM 内置连接自愈机制，确保 NSE 具有适当的健康检查

---

## 参考资源

- [NSM 官方文档](https://networkservicemesh.io/docs/)
- [NSM GitHub 组织](https://github.com/networkservicemesh)
- [NSM 示例仓库](https://github.com/networkservicemesh/deployments-k8s)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
