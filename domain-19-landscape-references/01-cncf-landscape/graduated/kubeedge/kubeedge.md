---
title: KubeEdge
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- kubelet
- scheduler
- helm
- gateway
- crd
- operator
- gpu
- nvidia
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KubeEdge 是什么
- 如何 KubeEdge
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KubeEdge
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- gpu-scheduling-basics
---

title: KubeEdge
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kubelet
- scheduler
- helm
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
- KubeEdge 是什么
- 如何 KubeEdge
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KubeEdge
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

# KubeEdge

> **成熟度**: Graduated | **加入时间**: 2019-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kubeedge.io |
| **GitHub** | https://github.com/kubeedge/kubeedge |
| **文档** | https://kubeedge.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Edge Computing |

---

## 项目概述

### 简介
KubeEdge 是构建边缘计算平台的开源系统，将 Kubernetes 的容器编排和管理能力从云端扩展到边缘。它使边缘节点能够运行容器化工作负载，同时保持与云端的协同。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2018-11 | 由华为开源发布 |
| 2019-03 | 加入 CNCF Sandbox |
| 2020-09 | 晋升为 CNCF Incubating |
| 2022-09 | 晋升为 CNCF Graduated |

### 核心定位
KubeEdge 是 Kubernetes 原生的边缘计算框架，专为边缘场景设计，支持离线自治、边云协同、设备管理，是构建边缘云的基础设施。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      KubeEdge 架构                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                     Cloud Side (云端)                        ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                 Kubernetes Master                        │││
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐              │││
│  │  │  │API Server│  │Scheduler │  │Controller│              │││
│  │  │  └──────────┘  └──────────┘  └──────────┘              │││
│  │  └─────────────────────────────────────────────────────────┘││
│  │                            │                                 ││
│  │                            ▼                                 ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                    CloudCore                             │││
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐        │││
│  │  │  │ CloudHub   │  │EdgeController│ │DeviceController│   │││
│  │  │  │ (消息通道) │  │ (边缘控制) │  │ (设备控制)│         │││
│  │  │  └────────────┘  └────────────┘  └────────────┘        │││
│  │  └─────────────────────────────────────────────────────────┘││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│                              │ WebSocket/QUIC                    │
│                              │ (双向通道)                        │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Edge Side (边缘端)                        ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                    EdgeCore                              │││
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐              │││
│  │  │  │ EdgeHub  │  │ MetaManager│ │ Edged   │              │││
│  │  │  │(消息同步)│  │ (元数据)  │  │(轻量kubelet)│           │││
│  │  │  └──────────┘  └──────────┘  └──────────┘              │││
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐              │││
│  │  │  │EventBus  │  │ServiceBus │  │DeviceTwin│              │││
│  │  │  │(MQTT桥接)│  │(服务访问) │  │(设备孪生) │              │││
│  │  │  └──────────┘  └──────────┘  └──────────┘              │││
│  │  └─────────────────────────────────────────────────────────┘││
│  │                            │                                 ││
│  │                            ▼                                 ││
│  │  ┌─────────────────────────────────────────────────────────┐││
│  │  │                  IoT Devices                             │││
│  │  │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐        │││
│  │  │  │传感器  │  │摄像头  │  │ PLC    │  │其他设备│        │││
│  │  │  └────────┘  └────────┘  └────────┘  └────────┘        │││
│  │  └─────────────────────────────────────────────────────────┘││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 位置 | 功能 |
|:---|:---|:---|
| **CloudHub** | 云端 | WebSocket 通信枢纽 |
| **EdgeController** | 云端 | 边缘节点生命周期管理 |
| **DeviceController** | 云端 | 设备 CRD 管理 |
| **EdgeHub** | 边缘 | 云端通信客户端 |
| **MetaManager** | 边缘 | 元数据本地缓存 |
| **Edged** | 边缘 | 轻量级 kubelet |
| **DeviceTwin** | 边缘 | 设备数字孪生 |
| **EventBus** | 边缘 | MQTT 消息桥接 |

---

## 核心功能

### 1. 边缘自治 (Offline Autonomy)

```
┌─────────────────────────────────────────────────────────────────┐
│                    边缘自治机制                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│    正常状态:                                                     │
│    Cloud ◄─────────────────────────────────────────► Edge       │
│           实时同步 Pod/ConfigMap/Secret                         │
│                                                                  │
│    网络断开:                                                     │
│    Cloud ◄ ─ ─ ─ ─ ─ X ─ ─ ─ ─ ─ ► Edge                        │
│                                    │                             │
│                                    ▼                             │
│                          ┌─────────────────┐                    │
│                          │  SQLite 缓存    │                    │
│                          │  • Pod 定义     │                    │
│                          │  • ConfigMap    │                    │
│                          │  • Secret       │                    │
│                          │  • 设备状态     │                    │
│                          └─────────────────┘                    │
│                                    │                             │
│                          继续运行容器和设备                      │
│                                                                  │
│    网络恢复:                                                     │
│    Cloud ◄─────────────────────────────────────────► Edge       │
│           自动同步状态变更                                       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 2. 设备管理 (Device Management)

```yaml
# Device CRD 定义
apiVersion: devices.kubeedge.io/v1beta1
kind: Device
metadata:
  name: temperature-sensor-01
  labels:
    type: temperature
    location: factory-1
spec:
  deviceModelRef:
    name: temperature-sensor-model
  protocol:
    modbus:
      slaveID: 1
  nodeSelector:
    nodeSelectorTerms:
      - matchExpressions:
          - key: "node-name"
            operator: In
            values:
              - edge-node-1
  propertyVisitors:
    - propertyName: temperature
      reportCycle: 10000  # 10秒
      collectCycle: 5000  # 5秒
      modbus:
        register: HoldingRegister
        offset: 0
        limit: 1

---
# Device Model 定义
apiVersion: devices.kubeedge.io/v1beta1
kind: DeviceModel
metadata:
  name: temperature-sensor-model
spec:
  protocol: modbus
  properties:
    - name: temperature
      description: "Current temperature"
      type:
        int:
          accessMode: ReadOnly
          maximum: 100
          minimum: -40
          unit: "Celsius"
```

### 3. 边云消息通道

```yaml
# EdgeCore 配置
modules:
  edgeHub:
    enable: true
    heartbeat: 15
    websocket:
      url: wss://cloudcore.example.com:10000/xxx
      # 支持 QUIC 协议
      # quic:
      #   url: cloudcore.example.com:10001
    
  metaManager:
    enable: true
    # 本地 SQLite 存储
    metaServer:
      enable: true
      
  eventBus:
    enable: true
    mqttMode: internal  # 内置 MQTT broker
    mqttQOS: 0
    mqttRetain: false
```

### 4. 服务网格集成

```yaml
# EdgeMesh - 边缘服务网格
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-service
  namespace: default
spec:
  template:
    spec:
      containers:
        - name: app
          image: myapp:latest
          ports:
            - containerPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: my-service
spec:
  selector:
    app: my-service
  ports:
    - port: 80
      targetPort: 8080
# EdgeMesh 自动实现边缘节点间服务发现和负载均衡
```

---

## 安装部署

### 云端安装

```bash
# 使用 keadm 工具安装
# 下载 keadm
wget https://github.com/kubeedge/kubeedge/releases/download/v1.15.0/keadm-v1.15.0-linux-amd64.tar.gz
tar -xvf keadm-v1.15.0-linux-amd64.tar.gz

# 安装 CloudCore
./keadm init --advertise-address=<云端IP> \
  --kubeedge-version=1.15.0 \
  --kube-config=/root/.kube/config

# 或使用 Helm
helm repo add kubeedge https://kubeedge.io/charts
helm install cloudcore kubeedge/cloudcore \
  --namespace kubeedge \
  --create-namespace
```

### 边缘端安装

```bash
# 获取 token
./keadm gettoken --kube-config=/root/.kube/config

# 加入边缘节点
./keadm join \
  --cloudcore-ipport=<云端IP>:10000 \
  --token=<token> \
  --kubeedge-version=1.15.0 \
  --edgenode-name=edge-node-1
```

### 配置示例

```yaml
# cloudcore.yaml
apiVersion: cloudcore.config.kubeedge.io/v1alpha1
kind: CloudCore
metadata:
  name: cloudcore
spec:
  modules:
    cloudHub:
      enable: true
      websocket:
        port: 10000
        address: "0.0.0.0"
      quic:
        port: 10001
        address: "0.0.0.0"
        maxIncomingStreams: 10000
    edgeController:
      enable: true
      load:
        UpdatePodStatusWorkers: 1
    deviceController:
      enable: true
```

---

## 使用场景

### 1. 智能制造

```
┌────────────────────────────────────────────────────────────────┐
│                     工业边缘计算                                 │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────────────┐         ┌─────────────────┐              │
│   │   Cloud K8s     │◄───────►│    CloudCore    │              │
│   │  (数据分析)     │   WAN   │   (边缘管理)    │              │
│   └─────────────────┘         └────────┬────────┘              │
│                                        │                        │
│              ┌─────────────────────────┼─────────────────────┐ │
│              │                Factory  │                      │ │
│              │   ┌─────────────────────┴─────────────────┐   │ │
│              │   │              EdgeCore                  │   │ │
│              │   │  ┌────────┐ ┌────────┐ ┌────────┐     │   │ │
│              │   │  │质检 AI │ │生产监控│ │设备管理│     │   │ │
│              │   │  └────────┘ └────────┘ └────────┘     │   │ │
│              │   └───────────────────┬───────────────────┘   │ │
│              │                       │                        │ │
│              │   ┌───────────────────┼───────────────────┐   │ │
│              │   │      Modbus / OPC-UA / MQTT           │   │ │
│              │   └───────────────────┼───────────────────┘   │ │
│              │                       │                        │ │
│              │   ┌─────────┐ ┌─────────┐ ┌─────────┐        │ │
│              │   │   PLC   │ │   CNC   │ │ 传感器  │        │ │
│              │   └─────────┘ └─────────┘ └─────────┘        │ │
│              └───────────────────────────────────────────────┘ │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

### 2. 智慧零售

```yaml
# 门店边缘部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: face-recognition
spec:
  template:
    spec:
      nodeSelector:
        node-role.kubernetes.io/edge: ""
        store-id: "store-001"
      containers:
        - name: face-ai
          image: face-recognition:latest
          resources:
            limits:
              nvidia.com/gpu: 1
          volumeMounts:
            - name: camera-feed
              mountPath: /dev/video0
```

### 3. 车联网

```yaml
# 车载边缘节点
apiVersion: devices.kubeedge.io/v1beta1
kind: Device
metadata:
  name: vehicle-gateway
spec:
  deviceModelRef:
    name: vehicle-obd-model
  protocol:
    bluetooth:
      macAddress: "AA:BB:CC:DD:EE:FF"
  propertyVisitors:
    - propertyName: speed
      reportCycle: 1000
    - propertyName: engineRPM
      reportCycle: 500
    - propertyName: location
      reportCycle: 5000
```

---

## EdgeMesh 服务网格

```
┌─────────────────────────────────────────────────────────────────┐
│                    EdgeMesh 架构                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   边缘节点 A                      边缘节点 B                      │
│   ┌─────────────────┐            ┌─────────────────┐            │
│   │ ┌─────────────┐ │            │ ┌─────────────┐ │            │
│   │ │  Service A  │ │   P2P      │ │  Service B  │ │            │
│   │ │ (frontend)  │◄┼────────────┼►│  (backend)  │ │            │
│   │ └─────────────┘ │  LibP2P    │ └─────────────┘ │            │
│   │       ▲         │            │       ▲         │            │
│   │       │         │            │       │         │            │
│   │ ┌─────┴───────┐ │            │ ┌─────┴───────┐ │            │
│   │ │ EdgeMesh    │ │            │ │ EdgeMesh    │ │            │
│   │ │ Agent       │ │            │ │ Agent       │ │            │
│   │ │ • DNS       │ │            │ │ • DNS       │ │            │
│   │ │ • Proxy     │ │            │ │ • Proxy     │ │            │
│   │ │ • Tunnel    │ │            │ │ • Tunnel    │ │            │
│   │ └─────────────┘ │            │ └─────────────┘ │            │
│   └─────────────────┘            └─────────────────┘            │
│                                                                  │
│   特性: 跨子网服务发现、边边通信、无需云端中转                    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 参考资源

- [官方文档](https://kubeedge.io/docs)
- [GitHub Repo](https://github.com/kubeedge/kubeedge)
- [CNCF 项目页面](https://www.cncf.io/projects/kubeedge/)
- [EdgeMesh](https://github.com/kubeedge/edgemesh)
- [设备管理指南](https://kubeedge.io/docs/developer/device_crd/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[concepts/node-lifecycle-management.md|node-lifecycle-management]]
- [[references/k8s-cloud-provider-comparison|云厂商托管 Kubernetes 服务全景对比（13 家）]] — Cross-reference
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
