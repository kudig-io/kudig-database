---
title: Metal3-io
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- opa
- gateway
- crd
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
- Metal3-io 是什么
- 如何 Metal3-io
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Metal3-io
- cncf
- landscape
---


# Metal3-io

> **成熟度**: Incubating | **加入时间**: 2020-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://metal3.io |
| **GitHub** | https://github.com/metal3-io |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Provisioning & Bare Metal |

---

## 项目概述

Metal3（Metal Kubed）提供裸金属基础设施的 Kubernetes 原生管理能力。它基于 Cluster API 实现裸金属服务器的自动发现、配置和生命周期管理，实现"裸金属即服务"。

## 核心特性

- **Kubernetes 原生**: CRD 方式管理裸金属服务器
- **Cluster API 集成**: 统一的集群生命周期管理
- **自动发现**: 通过 IPMI/Redfish 发现服务器
- **配置管理**: 自动化操作系统安装和配置
- **生命周期管理**: 开机、关机、重装、回收
- **无代理**: 使用 BMC 协议，无需在服务器安装代理

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Metal3 Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                 Management Cluster                         │ │
│  │                                                            │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │              Cluster API Controllers                │  │ │
│  │  │  ┌──────────────┐  ┌───────────────────────────┐   │  │ │
│  │  │  │   CAPI       │  │   CAPM3 (Metal3 Provider) │   │  │ │
│  │  │  │   Core       │  │                           │   │  │ │
│  │  │  └──────────────┘  └───────────────────────────┘   │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  │                                                            │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │           Bare Metal Operator (BMO)                 │  │ │
│  │  │  ┌──────────────┐  ┌───────────────────────────┐   │  │ │
│  │  │  │BareMetalHost │  │    Ironic Conductor      │   │  │ │
│  │  │  │ Controller   │  │    (Provisioning)        │   │  │ │
│  │  │  └──────────────┘  └───────────────────────────┘   │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│               IPMI / Redfish / BMC API                          │
│                              │                                   │
│  ┌───────────────────────────┼───────────────────────────────┐  │
│  │                           ▼                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │  │
│  │  │ Bare Metal  │  │ Bare Metal  │  │   Bare Metal    │  │  │
│  │  │  Server 1   │  │  Server 2   │  │   Server N      │  │  │
│  │  │             │  │             │  │                 │  │  │
│  │  │  ┌───────┐  │  │  ┌───────┐  │  │  ┌───────────┐ │  │  │
│  │  │  │  BMC  │  │  │  │  BMC  │  │  │  │    BMC    │ │  │  │
│  │  │  └───────┘  │  │  └───────┘  │  │  └───────────┘ │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │  │
│  │                    Bare Metal Pool                        │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 |
|------|------|
| Bare Metal Operator | 管理 BareMetalHost CRD |
| Ironic | 裸金属配置引擎 |
| CAPM3 | Cluster API Metal3 Provider |
| IP Address Manager | IP 地址分配管理 |

---

## 快速开始

### 安装 Metal3

```bash
# 使用 clusterctl 安装
clusterctl init --infrastructure metal3

# 或 Helm 安装
helm repo add metal3 https://metal3-io.github.io/helm-charts
helm install metal3 metal3/baremetal-operator \
  --namespace metal3 \
  --create-namespace
```

### 部署 Ironic

```yaml
# ironic-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ironic
  namespace: metal3
spec:
  replicas: 1
  selector:
    matchLabels:
      app: ironic
  template:
    spec:
      containers:
        - name: ironic
          image: quay.io/metal3-io/ironic:latest
          ports:
            - containerPort: 6385
          env:
            - name: PROVISIONING_IP
              value: "192.168.1.10"
```

---

## BareMetalHost 定义

```yaml
apiVersion: metal3.io/v1alpha1
kind: BareMetalHost
metadata:
  name: node-1
  namespace: metal3
spec:
  online: true
  bootMACAddress: "00:11:22:33:44:55"
  bmc:
    address: ipmi://192.168.1.100
    credentialsName: node-1-bmc-secret
  rootDeviceHints:
    deviceName: /dev/sda
---
apiVersion: v1
kind: Secret
metadata:
  name: node-1-bmc-secret
  namespace: metal3
type: Opaque
stringData:
  username: admin
  password: password123
```

### BMC 协议支持

| 协议 | 地址格式 |
|------|----------|
| IPMI | ipmi://host:port |
| Redfish | redfish://host/path |
| iDRAC | idrac://host |
| iLO | ilo://host |

---

## Cluster API 集群

```yaml
# Metal3Cluster
apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
kind: Metal3Cluster
metadata:
  name: my-cluster
  namespace: metal3
spec:
  controlPlaneEndpoint:
    host: 192.168.1.50
    port: 6443
  noCloudProvider: true
---
# Metal3MachineTemplate
apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
kind: Metal3MachineTemplate
metadata:
  name: my-cluster-controlplane
  namespace: metal3
spec:
  template:
    spec:
      image:
        url: http://images.example.com/ubuntu-22.04.qcow2
        checksum: http://images.example.com/ubuntu-22.04.qcow2.md5
      hostSelector:
        matchLabels:
          role: control-plane
---
# Cluster
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: my-cluster
  namespace: metal3
spec:
  clusterNetwork:
    pods:
      cidrBlocks: ["10.244.0.0/16"]
    services:
      cidrBlocks: ["10.96.0.0/12"]
  infrastructureRef:
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
    kind: Metal3Cluster
    name: my-cluster
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: my-cluster-controlplane
```

---

## 服务器生命周期

```bash
# 查看服务器状态
kubectl get baremetalhost -n metal3

# 开机
kubectl patch baremetalhost node-1 -n metal3 --type merge -p '{"spec":{"online":true}}'

# 关机
kubectl patch baremetalhost node-1 -n metal3 --type merge -p '{"spec":{"online":false}}'

# 重新配置
kubectl annotate baremetalhost node-1 -n metal3 \
  reboot.metal3.io="true"
```

### 状态流转

```
Registering -> Inspecting -> Available -> Provisioning -> Provisioned
                               ↓
                          Deprovisioning
```

---

## IP 地址管理

```yaml
apiVersion: ipam.metal3.io/v1alpha1
kind: IPPool
metadata:
  name: provisioning-pool
  namespace: metal3
spec:
  clusterName: my-cluster
  pools:
    - start: 192.168.1.100
      end: 192.168.1.200
      prefix: 24
      gateway: 192.168.1.1
  namePrefix: my-cluster
```

---

## 最佳实践

1. **BMC 网络**: 确保管理集群可访问所有 BMC
2. **镜像管理**: 使用 HTTP 服务器托管操作系统镜像
3. **硬件标签**: 使用标签区分不同硬件配置
4. **DHCP 配置**: 配置 PXE 启动所需的 DHCP
5. **监控**: 监控配置进度和 BMC 连接状态

---

## 参考资源

- [官方文档](https://metal3.io/documentation)
- [GitHub Repo](https://github.com/metal3-io)
- [Cluster API 文档](https://cluster-api.sigs.k8s.io/)
- [Ironic 文档](https://docs.openstack.org/ironic/latest/)

---

**维护者**: Kudig Team | **许可证**: MIT
