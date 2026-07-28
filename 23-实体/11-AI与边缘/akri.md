---
title: Akri (entities)
description: '## 概述'
summary: 'Akri 是一个 Kubernetes 资源接口项目，用于在边缘环境中自动发现和使用异构叶设备（Leaf Devices）。'
category: entities
tags:
- k8s
- cncf
- edge
- akri
- crd
- operator
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Akri 是什么
- 如何 Akri
trigger_keywords:
- Akri
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Akri

> **CNCF 状态**: Sandbox | **类别**: Edge | **主要语言**: Rust

## 概述

Akri 是一个 Kubernetes 资源接口项目，由微软开发，2021 年加入 CNCF 沙箱。它用于在边缘环境中自动发现和使用异构叶设备（Leaf Devices）。Akri 将 IP 摄像头、USB 传感器、OPC UA 服务器等物理设备抽象为 Kubernetes 原生资源，使 Pod 能够像使用 PersistentVolume 一样使用这些边缘设备。通过 Akri 的 Discovery Handler 插件机制，系统持续发现网络中的设备变化，自动创建对应的 Instance 和 Configuration 资源，并为每个设备生成 Broker Pod 进行交互。这使得大规模 IoT 设备管理可以通过标准的 Kubernetes API 和调度器实现，无需编写设备管理代码。

## 核心能力

- **设备自动发现**: 通过 Discovery Handler 插件持续发现网络中的设备变化
- **多协议支持**: ONVIF（IP 摄像头）、OPC UA（工业设备）、udev（USB 设备）、自定义协议
- **设备抽象**: 将物理设备映射为 Kubernetes Instance 资源
- **Broker Pod 管理**: 为每个设备自动创建 Broker Pod 进行交互
- **容量控制**: 设置 capacity 限制单个设备可被多少 Pod 同时使用
- **高可用发现**: 多节点部署 Agent 确保设备发现不中断

## 架构

Akri 采用 Agent + Controller 模式：

- **Akri Controller**: 集群级控制器，监听 Configuration 和 Instance 资源
- **Akri Agent (DaemonSet)**: 部署在每个节点的 Agent，执行设备发现和 Broker 管理
- **Configuration CRD**: 定义设备发现规则（协议、过滤条件、Broker 镜像）
- **Instance CRD**: 每个被发现的设备生成一个 Instance 资源
- **Discovery Handler**: 协议特定的发现插件（ONVIF、OPC UA、udev）
- **Broker Pod**: 与设备交互的工作负载 Pod，通过设备地址连接设备

发现流程：`Configuration → Agent (Discovery Handler) → 发现设备 → Instance → Broker Pod → 设备交互`

## K8s 集成

Akri 以 Helm Chart 方式部署在 Kubernetes 集群中。Akri Agent 作为 DaemonSet 运行在每个节点，通过 Discovery Handler 插件发现设备。发现到设备后创建 Instance CRD，Controller 根据 Configuration 中定义的 Broker 镜像自动创建 Broker Pod。Broker Pod 通过环境变量获取设备地址（IP、端口等），直接与设备通信。通过 Kubernetes 调度器将 Broker Pod 调度到能访问设备的节点。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Device Plugin 机制互补——Device Plugin 管理节点内资源（GPU/CPU），Akri 管理网络可达的设备。

## 生产场景

1. **智能视频分析**: 自动发现 ONVIF IP 摄像头，为每个摄像头启动视频处理 Broker Pod
2. **工业 IoT 监控**: 发现 OPC UA PLC 设备，采集工业传感器数据
3. **USB 设备管理**: 在 K8s 中管理 USB 加密狗、传感器等设备
4. **边缘 AI**: 在边缘集群中自动发现 GPU/NPU 设备并调度推理 Pod

## 安装与配置

### Helm 部署

```bash
# 安装 Akri
helm repo add akri-helm-charts https://project-akri.github.io/akri/
helm install akri akri-helm-charts/akri \
  --set onvif.discovery.enabled=true \
  --set onvif.configuration.enabled=true \
  --set onvif.configuration.brokerPod.image.repository=ghcr.io/project-akri/akri/onvif-broker \
  --namespace akri-system --create-namespace

# 验证部署
kubectl get pods -n akri-system
kubectl get crd | grep akri
```

### 设备发现与使用

```bash
# 查看发现的设备
kubectl get akrii -A
kubectl describe akrii <device-name>
```

```yaml
# 使用设备的 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: video-processor
spec:
  selector:
    matchLabels:
      app: video-processor
  template:
    metadata:
      labels:
        app: video-processor
    spec:
      containers:
      - name: processor
        image: video-processing:latest
        resources:
          limits:
            akri.sh/onvif-camera: "1"
      nodeSelector:
        akri.io/onvif-camera: "true"
---
# udev 设备发现配置
apiVersion: akri.sh/v0
kind: Configuration
metadata:
  name: udev-serial
spec:
  discoveryHandler:
    name: udev
    discoveryDetails: |
      groupRecursive: false
      udevRules:
        - 'KERNEL=="ttyUSB[0-9]*", ATTRS{idVendor}=="0403"'
  brokerProperties:
    SERIAL_NUMBER: "{{devnode}}"
```

## 运维操作

```bash
# 🟢 查看已发现的设备实例
kubectl get akrii -A -o wide

# 🟢 查看设备配置
kubectl get akric -A

# 🟢 检查 Agent 状态
kubectl get pods -n akri-system -l app=akri-agent

# 🟡 添加新的设备配置
kubectl apply -f udev-config.yaml

# 🔴 删除设备配置（会释放设备资源）
kubectl delete akric udev-serial

# 🔴 重启 Akri Agent
kubectl rollout restart daemonset/akri-agent -n akri-system
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 设备未发现 | Agent 未运行 | `kubectl get pods -n akri-system -l app=akri-agent` | 检查 Agent DaemonSet 状态 |
| 设备不可用 | 设备被其他 Pod 占用 | `kubectl get akrii <name> -o yaml` | 检查设备分配状态 |
| udev 规则不匹配 | 规则语法错误 | `udevadm test /sys/class/...` | 修正 udevRules 配置 |
| 网络设备发现失败 | 网络不可达 | `ping <device-ip>` | 检查网络连通性和防火墙 |
| Pod Pending: Insufficient akri.sh/... | 设备资源不足 | `kubectl describe node \| grep akri` | 确认设备已发现并注册 |

**排查流程：**
```
设备未被发现
├── 检查 Agent 状态 → kubectl get pods -n akri-system
├── 检查设备物理连接 → lsusb / ls /dev/ttyUSB*
├── 检查 udev 规则 → udevadm info /dev/<device>
├── 检查 Configuration → kubectl get akric -o yaml
└── 检查 Agent 日志 → kubectl logs -n akri-system -l app=akri-agent
```

## 生产案例

### 案例一：工业摄像头管理

- **场景**: 工厂 50+ 个 ONVIF 摄像头需要被 K8s 上的视频分析 Pod 访问
- **排查**: 手动配置设备 IP 和端口，设备更换后需重新配置
- **方案**: Akri 自动发现 ONVIF 摄像头，Pod 通过资源请求自动分配设备
- **效果**: 设备即插即用，更换设备无需修改 Pod 配置，运维工作量降低 80%

### 案例二：边缘传感器集群

- **场景**: 边缘节点连接多个 USB 传感器，需要动态分配给数据处理 Pod
- **排查**: 使用 Akri udev 发现规则自动识别 USB 传感器
- **方案**: 配置 udev 规则匹配特定 vendor ID，Pod 通过资源限制请求传感器
- **效果**: 传感器热插拔自动感知，Pod 自动迁移到有新传感器的节点

## 对比

| 特性 | Akri | Device Plugin | EdgeX Foundry | KubeEdge Device | 适用场景 |
|------|------|--------------|----------------|-----------------|----------|
| 设备发现 | ✅ 自动 | ❌ 手动 | ✅ | ✅ | Akri 最简单 |
| 网络设备 | ✅ | ❌ 仅本地 | ✅ | ✅ | - |
| K8s 原生 | ✅ CRD | ✅ | ❌ | ✅ CRD | - |
| CNCF 状态 | Sandbox | K8s 内置 | 非 CNCF | Graduated | - |
| 轻量级 | ✅ | ✅ | ❌ 重量级 | ⚠️ | 边缘场景 |

## 架构定位

在 CNCF 生态中，Akri 属于 **Edge** 类别，为云原生应用提供边缘设备自动发现和抽象能力。

## 参考链接

- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]]

## Related

- [[podman-desktop]] — Podman Desktop
- [[openyurt]] — OpenYurt
- [[carina]] — Carina
- [[spire]] — SPIRE
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- akri
- [[23-实体/cncf-edge-ai.md|[[23-实体/15-参考与索引/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
