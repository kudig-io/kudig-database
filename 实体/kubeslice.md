---
title: KubeSlice (entities)
description: '## 概述'
summary: 'KubeSlice 是一个多集群网络平台，通过创建逻辑 Slice（网络切片）覆盖层，在多个 Kubernetes 集群之间建立扁平的、安全的网络连接。每个 Slice 提供独立的网络命名空间、QoS 策略和安全隔离，使跨集群的应用能够像在同一集群内一样通信，同时保持网络隔离和带宽保障。'
category: entities
tags:
- k8s
- cncf
- networking
- kubeslice
- istio
- cilium
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
- KubeSlice 是什么
- 如何 KubeSlice
trigger_keywords:
- KubeSlice
prerequisites:
- kubectl-basics
- service-mesh-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeSlice

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

KubeSlice 是由 Avesha 开发的多集群网络切片平台，2022 年进入 CNCF Sandbox。它通过创建逻辑 **Slice（网络切片）** 覆盖层，在多个 Kubernetes 集群之间建立扁平、安全、高性能的网络连接。每个 Slice 是一个跨集群的虚拟网络命名空间，支持独立的 QoS 策略、安全隔离和带宽保障，使跨集群的应用能够像在同一集群内一样通信。

KubeSlice 使用 WireGuard 或 OpenVPN 构建加密隧道，通过 Slice Gateway 连接各集群的 Slice 网络。它解决了多集群网络的核心痛点：跨集群 Pod 通信需要复杂的 NAT/Ingress 配置、网络策略无法跨集群生效、缺乏端到端的 QoS 保障。

## Key Features

- **逻辑网络切片**：跨集群创建 Slice 虚拟网络，Pod 通过 Slice 实现跨集群通信
- **QoS 保障**：为每个 Slice 配置带宽限制和优先级，确保关键业务网络质量
- **安全隔离**：namespaceIsolation 确保 Slice 间网络安全隔离
- **多隧道协议**：支持 WireGuard（高性能）和 OpenVPN（兼容性）隧道
- **Service Import/Export**：跨集群的 Service 发现和访问
- **应用连接器**：将外部服务（数据库、遗留系统）接入 Slice 网络

## Architecture

KubeSlice 由 **Slice Controller**（运行在 Controller 集群，管理 Slice CRD）、**Slice Operator**（运行在每个工作集群，配置本地网络）、**Slice Gateway**（跨集群隧道节点，基于 WireGuard/OpenVPN）和 **NetOp Controller**（网络操作控制器）组成。Slice 创建后，参与的命名空间中的 Pod 获得额外的虚拟网络接口（`slice-interface`），流量通过此接口经 Gateway 隧道跨集群传输。

## K8s 集成

KubeSlice 通过 CRD 扩展 Kubernetes API。核心 CRD 包括 `Slice`（定义网络切片）、`ServiceExportConfig`（导出 Service 到 Slice）、`ServiceImportConfig`（从 Slice 导入 Service）和 `ApplicationConnector`（连接外部服务）。安装通过 Helm Chart 完成，Operator 自动在每个节点配置网络接口和路由规则。

## 生产部署要点

- **Slice 规划**：按业务域划分 Slice，每个 Slice 服务于一组关联的微服务
- **QoS 配置**：为关键业务 Slice 配置带宽保障，避免非关键流量抢占
- **网络隔离**：启用 namespaceIsolation 确保 Slice 间的安全隔离
- **网关选择**：低延迟场景使用 WireGuard，兼容性优先使用 OpenVPN
- **监控**：监控各 Slice 的带宽利用率、延迟和网关隧道状态

## 生产场景

1. **跨集群微服务通信**：A 集群的订单服务和 B 集群的库存服务通过 Slice 直接通信
2. **边缘到云数据同步**：边缘集群通过 Slice 安全回传数据到中心集群
3. **多租户网络隔离**：不同租户使用独立 Slice，QoS 和安全策略隔离
4. **混合云互联**：本地数据中心和公有云集群通过 Slice 安全互联

## 安装

```bash
# 安装 KubeSlice Controller（在 Controller 集群）
helm repo add kubeslice https://kubeslice.github.io/kubeslice/
helm install kubeslice-controller kubeslice/kubeslice-controller \
  -n kubeslice-controller --create-namespace

# 在工作集群安装 Slice Operator
helm install kubeslice-worker kubeslice/kubeslice-worker \
  -n kubeslice-system --create-namespace \
  --set clusterName=cluster-1

# 创建 Slice
kubectl apply -f - <<EOF
apiVersion: networking.kubeslice.io/v1beta1
kind: Slice
metadata:
  name: ecommerce-slice
spec:
  defaultSliceGatewayType: wireguard
  namespaceIsolationEnabled: true
  qosProfile:
    queueType: pq
    priority: 5
EOF
```

## 对比

| 特性 | KubeSlice | Submariner | Cilium Cluster Mesh |
|------|-----------|------------|---------------------|
| 网络模型 | Overlay Slice | L3 网关 | Pod CIDR 路由 |
| QoS | ✅ | ❌ | ⚠️ |
| 安全隔离 | ✅ Slice 级 | ⚠️ | ✅ NetworkPolicy |
| 隧道协议 | WireGuard/OpenVPN | IPsec | 无（原生路由） |

## 参考链接

- [[istio]]
- [[cilium]]
- [[deployment]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[kubeclipper]] — KubeClipper
- [[runme-notebooks]] — Runme
- [[operator-framework]] — Operator Framework
- [[clusternet]] — Clusternet
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubeslice
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
