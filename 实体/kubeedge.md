---
title: KubeEdge (entities)
description: '## 概述'
summary: 'KubeEdge 将 Kubernetes 的编排能力延伸到边缘计算场景，由华为开发，CNCF 毕业（Graduated）项目。'
category: entities
tags:
- k8s
- cncf
- edge
- kubeedge
- containerd
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeEdge 是什么
- 如何 KubeEdge
trigger_keywords:
- KubeEdge
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeEdge

> **CNCF 状态**: Graduated | **类别**: Edge | **主要语言**: Go

## 概述

KubeEdge 是一个将 Kubernetes 的编排能力延伸到边缘计算场景的开放平台，由华为开发，2019 年加入 CNCF 孵化，2023 年正式毕业（Graduated）。它将原生 Kubernetes 的能力无缝扩展到边缘节点，支持在边缘设备上运行容器化应用，同时解决边缘网络不稳定、离线自治、资源受限等边缘计算特有挑战。KubeEdge 的核心架构分为云端（CloudHub）和边缘端（EdgeHub）两部分，通过 WebSocket/quic 协议建立云边通信。它支持边缘节点离线自治——即使与云端断连，边缘节点上的 Pod 仍可正常运行。KubeEdge 还提供设备管理能力（Device CRD），可以直接管理 IoT 传感器、摄像头等边缘设备。

## 核心能力

- **边缘离线自治**: 边缘节点与云端断连时，本地 Pod 继续运行，恢复连接后自动同步状态
- **云边通信**: 通过 WebSocket/quic 协议建立安全的云边隧道，适应低带宽/高延迟网络
- **设备管理（Device CRD）**: 通过 Kubernetes CRD 管理 IoT 设备（传感器、摄像头、PLC 等）
- **边缘工作负载**: 支持 Deployment、StatefulSet、DaemonSet 等标准工作负载部署到边缘
- **资源优化**: 边缘端仅运行轻量级 EdgeCore，适合资源受限设备
- **多架构支持**: 支持 x86_64、ARM64、ARMv7 等边缘设备架构

## 架构

KubeEdge 采用云边分离的双层架构：

**云端组件**：
- **CloudHub**: 云端通信网关，管理与所有边缘节点的 WebSocket 连接
- **EdgeController**: Kubernetes 控制面的适配器，将 K8s 资源同步到边缘
- **DeviceController**: 管理设备元数据和状态的控制器

**边缘端组件**：
- **EdgeHub**: 边缘端通信代理，与 CloudHub 建立 WebSocket/quic 连接
- **Edged**: 轻量级 kubelet 替代，管理边缘节点上的容器生命周期
- **MetaManager**: 边缘端元数据存储（SQLite），支持离线自治
- **DeviceTwin**: 设备数字孪生，维护设备状态和期望值
- **EventBus**: MQTT 消息总线，与边缘设备通信

数据流：`云端 K8s API → CloudHub → EdgeHub → Edged → 容器运行时 → 边缘 Pod`

## K8s 集成

KubeEdge 以 Kubernetes 原生方式扩展集群。边缘节点通过 keadm 工具注册到云端 Kubernetes 集群，成为标准 Node 资源（带 `node-role.kubernetes.io/edge` 标签）。云端部署 CloudCore（以 Deployment 或二进制运行），管理所有边缘节点的通信。通过标准 kubectl 可以将工作负载调度到边缘节点（使用 nodeSelector 指定 edge 节点）。Device CRD 定义边缘 IoT 设备，DeviceController 将设备指令下发到边缘 DeviceTwin。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 完全兼容——对用户而言，边缘节点就是普通的 K8s 节点。

## 生产场景

1. **IoT 边缘计算**: 在工厂、园区部署边缘节点运行数据采集和预处理应用
2. **CDN/边缘缓存**: 在全球边缘节点部署缓存服务，低延迟服务用户
3. **智能交通**: 在路侧边缘设备运行 AI 推理应用（车牌识别、交通流量分析）
4. **离线自治场景**: 网络不稳定的远程站点（油井、船舶）保证业务连续性

## 安装

```bash
# 云端：安装 keadm
wget https://github.com/kubeedge/kubeedge/releases/download/v1.15.0/keadm-v1.15.0-linux-amd64.tar.gz
tar -xzf keadm-*.tar.gz && mv keadm /usr/local/bin/

# 初始化云端
keadm init --advertise-address=$CLOUD_IP --kubeedge-version=v1.15.0

# 边缘端：注册节点
keadm join --cloudcore-ipport=$CLOUD_IP:10000 \
  --edgenode-id=edge-node-1 --kubeedge-version=v1.15.0

# 获取边缘节点状态
kubectl get nodes --selector=node-role.kubernetes.io/edge=

# 部署应用到边缘节点
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: edge-app
spec:
  replicas: 1
  selector:
    matchLabels:
      app: edge-app
  template:
    metadata:
      labels:
        app: edge-app
    spec:
      nodeSelector:
        node-role.kubernetes.io/edge: ""
      containers:
      - name: app
        image: nginx:latest
EOF
```

## 对比

| 特性 | KubeEdge | OpenYurt | Akri | k3s |
|------|----------|----------|------|-----|
| 离线自治 | ✅ | ✅ | ❌ | ✅ |
| 设备管理 | ✅ Device CRD | ⚠️ via EdgeX | ✅ | ❌ |
| 云边分离 | ✅ | ✅ | ❌ | ❌ |
| CNCF 状态 | Graduated | Incubating | Sandbox | 非 CNCF |

## 架构定位

在 CNCF 生态中，KubeEdge 属于 **Edge** 类别，为云原生应用提供边缘计算扩展能力。

## 参考链接

- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[bank-vaults]] — Bank-Vaults
- [[thanos]] — Thanos
- [[03-containerd-security-hardening]] — [[containerd|containerd]]rd 安全加固|containerd 安全加固]]
- [[k0s]] — K0s
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 16-kubernetes-edge-computing-kubeedge-practice
- 03-kubeedge-architecture-deployment
- 04-kubeedge-device-edge-apps
- 09-edge-computing-kubeedge
- kubeedge
- [[实体/interlink.md|InterLink]]
- [[实体/kairos.md|Kairos]]
- [[实体/k8s-cloud-provider-comparison.md|云厂商托管 Kubernetes 服务全景对比（13 家）]] — Cross-reference
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[生态参考/领域索引/node-index.md|Node 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
