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

OpenYurt 通过 yurtctl 工具将标准 Kubernetes 集群一键转换为 OpenYurt 集群——无需修改核心组件。YurtHub 以 DaemonSet 部署在边缘节点上，代理 kubelet 对 API Server 的请求并缓存响应。NodePool CRD 将边缘节点分组，YurtAppSet 跨 NodePool 部署应用。Raven VPN Controller 建立跨 NodePool 的网络隧道。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 完全兼容——对用户而言，OpenYurt 集群就是一个标准 Kubernetes 集群加上边缘能力。

## 生产场景

1. **IoT 边缘计算**: 在工厂、园区部署边缘节点运行数据采集应用
2. **CDN/边缘缓存**: 在全球边缘节点部署缓存服务，低延迟服务用户
3. **车联网边缘**: 在路侧单元（RSU）运行 AI 推理应用
4. **离线边缘节点**: 网络不稳定的远程站点保证业务连续性

## 安装

```bash
# 安装 yurtctl
wget https://github.com/openyurtio/openyurt/releases/latest/download/yurtctl
chmod +x yurtctl && mv yurtctl /usr/local/bin/

# 将标准 K8s 集群转换为 OpenYurt
yurtctl convert --cloud-nodes master1 --provider kubeadm

# 创建 NodePool（边缘节点池）
kubectl apply -f - <<EOF
apiVersion: apps.openyurt.io/v1alpha1
kind: NodePool
metadata:
  name: beijing-edge
spec:
  type: Edge
  labels:
    region: beijing
EOF

# 将节点加入 NodePool
kubectl label node edge-node-1 openyurt.io/node-pool=beijing-edge

# 跨 NodePool 部署应用
kubectl apply -f - <<EOF
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
    - name: shanghai-edge
EOF
```

## 对比

| 特性 | OpenYurt | KubeEdge | k3s | Akri |
|------|----------|----------|-----|------|
| 无侵入转换 | ✅ | ❌ | ❌ | ❌ |
| 边缘自治 | ✅ YurtHub | ✅ | ✅ | ❌ |
| NodePool | ✅ | ❌ | ❌ | ❌ |
| CNCF 状态 | Incubating | Graduated | 非 CNCF | Sandbox |

## 架构定位

在 CNCF 生态中，OpenYurt 属于 **Edge** 类别，为云原生应用提供无侵入的边缘计算扩展能力。

## 参考链接

- networking.md|cilium-ebpf-networking]]
- [[deployment]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[实体/kubelet.md|[[kubelet|kubelet]]]]

## Related

- [[paralus]] — Paralus
- [[hexa]] — Hexa
- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — Podman Desktop
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 05-openyurt-architecture
- openyurt
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
