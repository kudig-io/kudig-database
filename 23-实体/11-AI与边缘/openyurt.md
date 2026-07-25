---
title: OpenYurt (entities)
description: '## 概述'
summary: 'OpenYurt 是阿里云开源的边缘计算平台，将原生 Kubernetes 能力无缝扩展到边缘场景。'
category: entities
tags:
- k8s
- cncf
- edge
- openyurt
- kubelet
- cilium
- crd
- operator
- ebpf
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenYurt 是什么
- 如何 OpenYurt
trigger_keywords:
- OpenYurt
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenYurt

> **CNCF 状态**: Incubating | **类别**: Edge | **主要语言**: Go

## 概述

OpenYurt 是阿里云开源的边缘计算平台，2020 年加入 CNCF 沙箱，2022 年晋升孵化项目。它将原生 Kubernetes 能力无缝扩展到边缘场景，解决了边缘网络不稳定、节点自治、多区域管理等边缘计算特有挑战。OpenYurt 的设计理念是"无侵入增强"——不修改 Kubernetes 核心代码，通过组件转换（Convert）将标准 Kubernetes 集群扩展为边缘集群。边缘节点与云端断连时，OpenYurt 的 YurtHub 组件缓存云端数据，实现边缘自治——Pod 继续运行不中断。NodePool 将边缘节点按区域分组管理，Raven 提供跨 NodePool 的网络通信能力。

## 核心能力

- **边缘自治**: 边缘节点与云端断连时，YurtHub 缓存数据，Pod 继续正常运行
- **单元化管理**: NodePool CRD 实现边缘节点按地理位置/业务单元分组管理
- **流量闭环**: RavenService 确保应用流量在同一 NodePool 内闭环，减少跨域延迟
- **云边协同**: 统一的云端控制面管理所有边缘节点
- **无侵入增强**: 通过 yurtctl convert 将标准 K8s 集群转换为 OpenYurt 集群
- **边缘设备管理**: 集成 EdgeX Foundry 管理 IoT 设备

## 架构

OpenYurt 采用云边分离架构：

- **YurtHub**: 部署在边缘节点上的组件，缓存云端数据实现边缘自治
- **NodePool**: 将边缘节点按区域分组，每个 NodePool 有自己的网络
- **Raven**: 提供跨 NodePool 的网络通信（VPN/Gateway）
- **YurtAppSet (原 UnitedDeployment)**: 跨 NodePool 部署应用的工作负载
- **YurtAppDaemon**: 在指定 NodePool 中以 DaemonSet 方式部署
- **Yurt-Manager**: 集群级控制器，管理 NodePool、YurtAppSet 等 CRD

架构：`云端控制面 → YurtHub (缓存) → 边缘 kubelet → Pod`

## K8s 集成

OpenYurt 通过 yurtctl 工具将标准 Kubernetes 集群一键转换为 OpenYurt 集群——无需修改核心组件。YurtHub 以 DaemonSet 部署在边缘节点上，代理 kubelet 对 API Server 的请求并缓存响应。NodePool CRD 将边缘节点分组，YurtAppSet 跨 NodePool 部署应用。Raven VPN Controller 建立跨 NodePool 的网络隧道。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 完全兼容——对用户而言，OpenYurt 集群就是一个标准 Kubernetes 集群加上边缘能力。

## 生产场景

1. **IoT 边缘计算**: 在工厂、园区部署边缘节点运行数据采集应用
2. **CDN/边缘缓存**: 在全球边缘节点部署缓存服务，低延迟服务用户
3. **车联网边缘**: 在路侧单元（RSU）运行 AI 推理应用
4. **离线边缘节点**: 网络不稳定的远程站点保证业务连续性

## 安装与配置

```bash
# 安装 yurtctl
wget https://github.com/openyurtio/openyurt/releases/latest/download/yurtctl
chmod +x yurtctl && mv yurtctl /usr/local/bin/
yurtctl version

# 将标准 K8s 集群转换为 OpenYurt
yurtctl convert --cloud-nodes master1 --provider kubeadm
```

### NodePool 配置

```yaml
apiVersion: apps.openyurt.io/v1alpha1
kind: NodePool
metadata:
  name: beijing-edge
spec:
  type: Edge
  labels:
    region: beijing
  annotations:
    nodepool.openyurt.io/enable-autonomy: "true"
```

### YurtAppSet 跨池部署

```yaml
apiVersion: apps.openyurt.io/v1alpha1
kind: YurtAppSet
metadata:
  name: edge-app
spec:
  workload:
    workloadTemplate:
      deploymentTemplate:
        metadata:
          name: edge-app
        spec:
          template:
            spec:
              containers:
              - name: app
                image: nginx:latest
  topology:
    pools:
    - name: beijing-edge
      replicas: 2
    - name: shanghai-edge
      replicas: 3
```

```bash
# 将节点加入 NodePool
kubectl label node edge-node-1 openyurt.io/node-pool=beijing-edge
```

## 运维操作

```bash
# 🟢 查看 NodePool 状态
kubectl get nodepools
kubectl describe nodepool beijing-edge

# 🟢 查看边缘节点自治状态
kubectl get nodes -l openyurt.io/node-pool=beijing-edge

# 🟡 添加边缘节点
yurtctl join --node-name edge-node-2 --provider kubeadm

# 🟡 更新边缘应用
kubectl apply -f yurtappset-updated.yaml

# 🔴 删除 NodePool
kubectl delete nodepool beijing-edge
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 边缘节点 NotReady | 网络断开/自治未启用 | `kubectl get nodes` | 检查 YurtHub 状态 |
| 应用未下发到边缘 | NodePool 标签不匹配 | `kubectl describe yurtappset` | 确认 topology 配置 |
| YurtHub 异常 | 证书过期 | `kubectl logs yurthub-pod` | 轮换证书 |
| 边缘自治失败 | 本地缓存不足 | 检查节点 /var/lib/openyurt | 增加缓存空间 |
| 节点转换失败 | 集群版本不兼容 | `yurtctl convert --help` | 检查版本兼容性 |

```
排查流程:
├── 边缘节点离线
│   ├── 检查节点网络连通性
│   ├── kubectl logs yurthub → YurtHub 状态
│   └── 确认 autonomy annotation 已启用
├── 应用下发异常
│   ├── kubectl describe yurtappset → 查看状态
│   ├── 确认 NodePool 存在且标签正确
│   └── 检查边缘节点资源充足
└── 转换失败
    ├── yurtctl convert --help → 检查参数
    └── 确认集群版本兼容
```

## 生产案例

### 案例 1: CDN 边缘节点管理

- **场景**: 全国 200+ 边缘节点运行 CDN 服务，网络不稳定
- **方案**: 部署 OpenYurt，按地域创建 NodePool；启用边缘自治，网络断开时本地服务不中断
- **效果**: 网络故障时业务连续性 99.99%，运维效率提升 5x

### 案例 2: 无侵入边缘化改造

- **场景**: 已有标准 K8s 集群需要扩展边缘能力，不能影响现有业务
- **方案**: 使用 `yurtctl convert` 无侵入转换；边缘节点通过 YurtHub 缓存实现自治
- **效果**: 零停机完成边缘化改造，现有业务完全不受影响

## 对比

| 特性 | OpenYurt | KubeEdge | k3s | Akri | 适用场景 |
|------|----------|----------|-----|------|----------|
| 无侵入转换 | ✅ | ❌ | ❌ | ❌ | 存量集群 |
| 边缘自治 | ✅ YurtHub | ✅ | ✅ | ❌ | 离线场景 |
| NodePool | ✅ | ❌ | ❌ | ❌ | 多地域 |
| 设备管理 | ⚠️ | ✅ | ❌ | ✅ | IoT |
| CNCF 状态 | Incubating | Graduated | 非 CNCF | Sandbox | 生态 |

## 架构定位

在 CNCF 生态中，OpenYurt 属于 **Edge** 类别，为云原生应用提供无侵入的边缘计算扩展能力。

## 参考链接

- networking.md|cilium-ebpf-networking]]
- [[deployment]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[23-实体/kubelet.md|[[kubelet|kubelet]]]]

## Related

- [[paralus]] — Paralus
- [[hexa]] — Hexa
- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — Podman Desktop
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 05-openyurt-architecture
- openyurt
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
