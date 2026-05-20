---
title: Kube-OVN
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- redis
- statefulset
- ingress
- gateway
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kube-OVN 是什么
- 如何 Kube-OVN
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kube-OVN
- cncf
- landscape
---

# Kube-OVN

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kubeovn.github.io/docs/ |
| **GitHub** | https://github.com/kubeovn/kube-ovn |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kube-OVN 是一个基于 OVN/OVS 的高级 Kubernetes 网络 CNI 插件，将 SDN（软件定义网络）的能力引入 Kubernetes。它提供子网管理、固定 IP、QoS、网络策略、EIP/SNAT、VPC 多租户等企业级网络功能，是 Kubernetes 网络功能最丰富的 CNI 之一。

### 核心特性

- **子网管理**: 自定义子网 CIDR，支持多子网、VLAN 和 Overlay/Underlay 混合
- **固定 IP**: 为 Pod 和 StatefulSet 分配固定 IP 地址
- **VPC 多租户**: 完全隔离的虚拟私有网络，租户间网络不可达
- **QoS**: 带宽限速、DSCP 标记、流量整形
- **网关和 NAT**: EIP、DNAT、SNAT 实现外部网络访问
- **ACL/NetworkPolicy**: 支持 Kubernetes NetworkPolicy 和扩展 ACL
- **流量镜像**: 将流量镜像到指定 Pod 进行分析
- **多集群互联**: 基于 OVN-IC 实现跨集群网络互通

---

## 架构设计

```
┌────────────────────────────────────────────────────┐
│                 Kubernetes Cluster                   │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │            Kube-OVN Controller                │   │
│  │  (Subnet/IP/VPC/NAT CRD management)          │   │
│  └──────────────────────┬───────────────────────┘   │
│                         │                            │
│  ┌──────────────────────┴───────────────────────┐   │
│  │              OVN Northbound DB                │   │
│  │  (Logical network topology)                   │   │
│  └──────────────────────┬───────────────────────┘   │
│                         │                            │
│  ┌──────────────────────┴───────────────────────┐   │
│  │              OVN Southbound DB                │   │
│  │  (Physical bindings, flow tables)             │   │
│  └──────────────────────┬───────────────────────┘   │
│                         │                            │
│  ┌──────────────────────┴───────────────────────┐   │
│  │         ovn-controller (per node)             │   │
│  │  ┌─────────────────────────────────────────┐ │   │
│  │  │            OVS Bridge (br-int)           │ │   │
│  │  │                                          │ │   │
│  │  │  Pod-A ──► veth ──► OVS Port            │ │   │
│  │  │  Pod-B ──► veth ──► OVS Port            │ │   │
│  │  │                    ▼                     │ │   │
│  │  │           OpenFlow Tables                │ │   │
│  │  │    (ACL, QoS, NAT, Routing)             │ │   │
│  │  └─────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Kube-OVN

```bash
# 使用安装脚本
wget https://raw.githubusercontent.com/kubeovn/kube-ovn/release-1.13/dist/images/install.sh
bash install.sh

# 或使用 Helm
helm repo add kubeovn https://kubeovn.github.io/kube-ovn/
helm install kube-ovn kubeovn/kube-ovn \
  --namespace kube-system \
  --set MASTER_NODES="node1,node2,node3" \
  --set POD_CIDR="10.16.0.0/16" \
  --set SVC_CIDR="10.96.0.0/12" \
  --set JOIN_CIDR="100.64.0.0/16"
```

### 验证安装

```bash
# 检查组件状态
kubectl get pods -n kube-system -l app=kube-ovn

# 查看默认子网
kubectl get subnet

# 查看 IP 分配
kubectl get ip
```

---

## 配置详解

### 自定义子网

```yaml
apiVersion: kubeovn.io/v1
kind: Subnet
metadata:
  name: app-subnet
spec:
  protocol: IPv4
  cidrBlock: 10.17.0.0/16
  gateway: 10.17.0.1
  excludeIps:
    - 10.17.0.1..10.17.0.10
  namespaces:
    - production
    - staging
  gatewayType: distributed  # 或 centralized
  natOutgoing: true
  private: false
```

### 固定 IP

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: fixed-ip-pod
  annotations:
    ovn.kubernetes.io/ip_address: "10.17.0.100"
    ovn.kubernetes.io/mac_address: "00:00:00:AB:CD:EF"
    ovn.kubernetes.io/logical_switch: "app-subnet"
spec:
  containers:
    - name: app
      image: nginx:latest
---
# StatefulSet 固定 IP 池
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis
  annotations:
    ovn.kubernetes.io/ip_pool: "10.17.0.50,10.17.0.51,10.17.0.52"
spec:
  replicas: 3
  selector:
    matchLabels:
      app: redis
  template:
    metadata:
      labels:
        app: redis
    spec:
      containers:
        - name: redis
          image: redis:7
```

### VPC 多租户

```yaml
# 创建 VPC
apiVersion: kubeovn.io/v1
kind: Vpc
metadata:
  name: tenant-a-vpc
spec:
  namespaces:
    - tenant-a
  staticRoutes:
    - cidr: 0.0.0.0/0
      nextHopIP: 10.18.0.1
      policy: policyDst
---
# VPC 子网
apiVersion: kubeovn.io/v1
kind: Subnet
metadata:
  name: tenant-a-subnet
spec:
  vpc: tenant-a-vpc
  cidrBlock: 10.18.0.0/24
  gateway: 10.18.0.1
  namespaces:
    - tenant-a
---
# VPC NAT 网关
apiVersion: kubeovn.io/v1
kind: VpcNatGateway
metadata:
  name: tenant-a-gw
spec:
  vpc: tenant-a-vpc
  subnet: tenant-a-subnet
  lanIp: 10.18.0.254
  eips:
    - eipCIDR: 172.18.0.10/24
      gateway: 172.18.0.1
  snatRules:
    - eip: 172.18.0.10
      internalCIDR: 10.18.0.0/24
```

### QoS 带宽限制

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: bandwidth-limited
  annotations:
    ovn.kubernetes.io/ingress_rate: "100"   # Mbps 入站带宽
    ovn.kubernetes.io/egress_rate: "50"     # Mbps 出站带宽
spec:
  containers:
    - name: app
      image: nginx:latest
```

### NetworkPolicy

```yaml
apiVersion: kubeovn.io/v1
kind: Subnet
metadata:
  name: secure-subnet
spec:
  cidrBlock: 10.19.0.0/24
  acls:
    - direction: from-lport
      priority: 1002
      match: "ip4.src == 10.19.0.0/24 && ip4.dst == 10.19.0.0/24"
      action: allow-related
    - direction: from-lport
      priority: 1001
      match: "ip4.src == 10.19.0.0/24"
      action: drop
```

---

## 高级功能

### 流量镜像

```yaml
apiVersion: kubeovn.io/v1
kind: OvnEip
metadata:
  name: mirror-config
  annotations:
    ovn.kubernetes.io/mirror: "true"
---
# 在 Pod 注解中启用镜像
metadata:
  annotations:
    ovn.kubernetes.io/mirror: "true"
    ovn.kubernetes.io/mirror_iface: "mirror0"
```

### 多集群互联 (OVN-IC)

```yaml
# 集群 A 配置
apiVersion: kubeovn.io/v1
kind: Subnet
metadata:
  name: shared-subnet
spec:
  cidrBlock: 10.20.0.0/16
  enableInterConnection: true
```

---

## 监控

| 指标 | 说明 |
|:---|:---|
| `kube_ovn_subnet_ip_total` | 子网 IP 总数 |
| `kube_ovn_subnet_ip_used` | 已分配 IP 数 |
| `kube_ovn_ovs_info` | OVS 版本和状态 |
| `kube_ovn_node_port_bindint` | 节点端口绑定状态 |

---

## 最佳实践

1. **子网规划**: 提前规划子网 CIDR，预留足够的 IP 空间用于扩容
2. **VPC 隔离**: 不同租户使用独立 VPC 实现网络级隔离
3. **固定 IP**: StatefulSet 使用 IP Pool，Pod 使用固定 IP 适配传统应用
4. **QoS 管理**: 对流量敏感的应用配置带宽限制，防止资源争抢
5. **监控 OVS**: 关注 OVS flow table 大小和连接数，避免性能瓶颈
6. **高可用**: OVN 数据库部署在多个 Master 节点实现 HA

---

## 参考资源

- [Kube-OVN 官方文档](https://kubeovn.github.io/docs/)
- [Kube-OVN GitHub](https://github.com/kubeovn/kube-ovn)
- [OVN 架构文档](https://www.ovn.org/en/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
