---
title: Akri
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- crd
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Akri 是什么
- 如何 Akri
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Akri
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

title: Akri
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- crd
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Akri 是什么
- 如何 Akri
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Akri
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

# Akri

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://docs.akri.sh/ |
| **GitHub** | https://github.com/project-akri/akri |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Akri 是一个 Kubernetes 资源接口项目，用于在边缘环境中自动发现和使用异构叶设备（Leaf Devices）。它将 IP 摄像头、USB 传感器、OPC UA 服务器等物理设备抽象为 Kubernetes 原生资源，使 Pod 能够像使用 PersistentVolume 一样使用这些边缘设备。Akri 通过 Discovery Handler 插件机制持续发现网络中的设备变化，并自动调度 Broker Pod 处理设备数据。

### 核心特性

- **设备自动发现**: 通过 Discovery Handler 持续发现网络中的边缘设备（ONVIF 摄像头、USB、OPC UA 等）
- **Kubernetes 原生抽象**: 将物理设备表示为 Kubernetes CRD (Akri Instance)，纳入 K8s 资源管理
- **自动 Broker 调度**: 设备发现后自动部署 Broker Pod 处理设备数据流
- **设备共享策略**: 支持独占和共享两种设备访问模式
- **插件化架构**: Discovery Handler 可扩展，支持自定义协议的设备发现
- **动态响应**: 设备上线/下线时自动创建/删除对应的 Instance 和 Broker

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  Kubernetes Cluster                    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │          Akri Controller                      │    │
│  │  (监听 Instance CRD，调度 Broker Pod)         │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │          Akri Agent (每个 Node 一个)           │    │
│  │  ┌──────────────────────────────────────┐     │    │
│  │  │       Discovery Handlers              │     │    │
│  │  │  ┌────────┐ ┌───────┐ ┌──────────┐  │     │    │
│  │  │  │ ONVIF  │ │  USB  │ │ OPC UA   │  │     │    │
│  │  │  │Handler │ │Handler│ │ Handler  │  │     │    │
│  │  │  └───┬────┘ └───┬───┘ └────┬─────┘  │     │    │
│  │  └──────┼──────────┼──────────┼─────────┘     │    │
│  └─────────┼──────────┼──────────┼───────────────┘    │
│            │          │          │                     │
│  ┌─────────▼──────────▼──────────▼───────────────┐    │
│  │    Akri Instance CRDs (每设备一个)             │    │
│  └─────────┬──────────┬──────────┬───────────────┘    │
│            │          │          │                     │
│  ┌─────────▼───┐ ┌────▼─────┐ ┌─▼──────────────┐    │
│  │Broker Pod 1 │ │Broker 2  │ │ Broker Pod 3   │    │
│  │(处理摄像头) │ │(处理USB) │ │ (处理OPC UA)   │    │
│  └─────────────┘ └──────────┘ └────────────────┘    │
└──────────────────────────────────────────────────────┘
         │              │              │
    ┌────▼────┐   ┌─────▼────┐  ┌─────▼──────┐
    │IP Camera│   │USB Sensor│  │OPC UA      │
    │(ONVIF)  │   │          │  │Server      │
    └─────────┘   └──────────┘  └────────────┘
```

---

## 快速开始

### 安装 Akri

```bash
# 使用 Helm 安装
helm repo add akri-helm-charts https://project-akri.github.io/akri/
helm install akri akri-helm-charts/akri \
  --namespace akri \
  --create-namespace \
  --set agent.enabled=true \
  --set controller.enabled=true
```

### 发现 ONVIF IP 摄像头

```yaml
# onvif-config.yaml
apiVersion: akri.sh/v0
kind: Configuration
metadata:
  name: onvif-camera
spec:
  discoveryHandler:
    name: onvif
    discoveryDetails: |
      ipAddresses:
        action: Include
        items:
          - 192.168.1.0/24
      scopes:
        - "onvif://www.onvif.org/Profile/Streaming"
  brokerSpec:
    brokerPodSpec:
      containers:
        - name: camera-broker
          image: ghcr.io/project-akri/akri/onvif-video-broker:latest
          resources:
            limits:
              "{{PLACEHOLDER}}": "1"  # Akri 自动替换为设备资源名
  instanceServiceSpec:
    type: ClusterIP
    ports:
      - name: grpc
        port: 80
        targetPort: 8083
  configurationServiceSpec:
    type: ClusterIP
    ports:
      - name: grpc
        port: 80
        targetPort: 8083
  capacity: 1  # 每台摄像头分配 1 个 Broker
```

```bash
kubectl apply -f onvif-config.yaml

# 查看发现的设备
kubectl get akrii -n akri
# NAME                                AGE
# onvif-camera-a1b2c3                 30s
# onvif-camera-d4e5f6                 30s

# 查看自动创建的 Broker Pod
kubectl get pods -n akri -l akri.sh/configuration=onvif-camera
```

### 发现 USB 设备

```yaml
# usb-config.yaml
apiVersion: akri.sh/v0
kind: Configuration
metadata:
  name: usb-sensor
spec:
  discoveryHandler:
    name: udev
    discoveryDetails: |
      udevRules:
        - 'SUBSYSTEM=="video4linux"'
  brokerSpec:
    brokerPodSpec:
      containers:
        - name: usb-broker
          image: my-registry/usb-sensor-broker:latest
          securityContext:
            privileged: true
  capacity: 1
```

---

## 高级功能

### OPC UA 工业设备发现

```yaml
apiVersion: akri.sh/v0
kind: Configuration
metadata:
  name: opcua-devices
spec:
  discoveryHandler:
    name: opcua
    discoveryDetails: |
      opcuaDiscoveryMethod:
        standard:
          discoveryUrls:
            - "opc.tcp://192.168.1.100:4840/"
            - "opc.tcp://192.168.1.101:4840/"
      applicationNames:
        action: Include
        items:
          - "TemperatureSensor"
          - "PressureSensor"
  brokerSpec:
    brokerPodSpec:
      containers:
        - name: opcua-broker
          image: ghcr.io/project-akri/akri/opcua-monitoring-broker:latest
          env:
            - name: IDENTIFIER
              value: "ns=2;s=Temperature"
            - name: NAMESPACE_INDEX
              value: "2"
```

### 自定义 Discovery Handler

```yaml
# 注册自定义 Discovery Handler
apiVersion: akri.sh/v0
kind: Configuration
metadata:
  name: custom-device
spec:
  discoveryHandler:
    name: my-custom-handler
    discoveryDetails: |
      protocol: "mqtt"
      broker: "mqtt://broker.local:1883"
      topic: "devices/+/status"
  discoveryHandler:
    name: my-custom-handler
  brokerSpec:
    brokerPodSpec:
      containers:
        - name: mqtt-broker
          image: my-registry/mqtt-device-broker:latest
```

### 设备共享模式

```yaml
# 多个 Broker Pod 共享同一设备
apiVersion: akri.sh/v0
kind: Configuration
metadata:
  name: shared-camera
spec:
  discoveryHandler:
    name: onvif
    discoveryDetails: |
      ipAddresses:
        action: Include
        items: ["192.168.1.50"]
  capacity: 3  # 允许 3 个 Broker 共享同一摄像头
  brokerSpec:
    brokerPodSpec:
      containers:
        - name: analytics
          image: my-registry/video-analytics:latest
```

---

## 与其他方案对比

| 特性 | Akri | KubeEdge DeviceModel | OpenYurt IoT | EdgeX Foundry |
|:---|:---|:---|:---|:---|
| 设备发现 | 自动持续发现 | 手动注册 | 手动注册 | 自动发现 |
| K8s 原生 | CRD + Pod | CRD + Device Twin | CRD | 独立平台 |
| 协议支持 | ONVIF/USB/OPC UA | MQTT/BLE/Modbus | MQTT | 多协议 |
| Broker 自动化 | 自动调度 | 需手动部署 | 需手动部署 | 自动 |
| 设备共享 | 支持 | 不支持 | 不支持 | 支持 |
| 语言 | Rust | Go | Go | Go/C |

---

## 最佳实践

1. **网络规划**: 确保 Akri Agent 节点能访问设备网络，配置正确的 IP 段和协议端口
2. **资源限制**: 为 Broker Pod 设置合理的资源限制，避免视频流处理消耗过多节点资源
3. **设备容量**: 根据设备处理能力设置 capacity，避免过多 Broker 同时访问同一设备
4. **安全配置**: ONVIF 摄像头配置认证凭据，OPC UA 设备配置证书
5. **高可用**: 在多节点部署 Agent，确保设备发现不因单节点故障中断

---

## 参考资源

- [Akri 官方文档](https://docs.akri.sh/)
- [Akri GitHub](https://github.com/project-akri/akri)
- [ONVIF Discovery Handler](https://docs.akri.sh/discovery-handlers/onvif)
- [OPC UA Discovery Handler](https://docs.akri.sh/discovery-handlers/opcua)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
