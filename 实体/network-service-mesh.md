---
title: Network Service Mesh (NSM)
description: '## 概述'
summary: 'Network Service Mesh (NSM) 是一个混合/多云的 IP 服务网格，提供 L2/L3 层的网络服务连接能力。与传统的 Service Mesh（如 Istio、Linkerd 专注于 L4-L7）不同，NSM 专注于为应用提供底层网络服务，例如安全隧道、VPN、防火墙等网络功能的动态连接。'
category: entities
tags:
- k8s
- cncf
- networking
- network-service-mesh
- prometheus
- grafana
- istio
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
- Network Service Mesh (NSM) 是什么
- 如何 Network Service Mesh (NSM)
trigger_keywords:
- Network
- Service
- Mesh
- NSM
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[实体/network-service-mesh.md|Network Service Mesh]]rvice]]Service Mesh）|Service Mesh]] (NSM)

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Network Service Mesh（NSM）是一个混合/多云的 IP 服务网格，提供 L2/L3 层的网络服务连接能力，2019 年加入 CNCF Sandbox。与传统的 Service Mesh（如 Istio、Linkerd 专注于 L4-L7）不同，NSM 专注于为应用提供底层网络服务——例如安全隧道、VPN、防火墙、负载均衡等网络功能的动态按需连接。NSM 通过灵活的拓扑设计满足复杂网络需求。

## 核心特性

- **L2/L3 连接**: 在 Pod 之间建立二层/三层网络连接
- **动态拓扑**: 按需创建网络服务链路，支持复杂网络拓扑
- **多数据平面**: 支持 Kernel 和 VPP（Vector Packet Processor）数据平面
- **SPIFFE 身份**: 集成 SPIRE 进行工作负载身份认证
- **跨集群连接**: 支持跨集群、跨云的网络服务连接
- **NSE 模型**: Network Service Endpoint 可由 Pod 动态提供

## 架构

NSM 采用客户端-服务端模型。核心组件包括：NSMgr（Network Service Manager，每个节点运行一个，管理本地连接）、Forwarder（数据平面，Kernel 或 VPP 模式，处理实际数据转发）、NSC（Network Service Client，发起连接请求的 Pod）、NSE（Network Service Endpoint，提供网络服务的 Pod）。NSC 通过 Pod 内的网卡向 NSMgr 发起 Network Service Request。NSMgr 根据请求选择合适的 NSE，在 NSC 和 NSE 之间建立隧道（VXLAN、Geneve 或直接路由）。

## Kubernetes 集成

NSM 通过 Mutating Webhook 自动为 Pod 注入 NSC init container，配置额外的网络接口。NSMgr 和 Forwarder 以 DaemonSet 部署在每个节点上。Network Service 通过 CRD（NetworkService）定义。NSE Pod 通过特定注解注册为网络服务端点。支持标准的 Kubernetes Service 和 Pod API。与 SPIRE/SPIFFE 集成实现 mTLS 工作负载身份。

## 生产使用场景

1. **安全隧道**: 为 Pod 间通信提供动态加密隧道
2. **多集群网络互通**: 建立跨集群的 L2/L3 网络连接
3. **网络功能链**: 将流量按顺序通过防火墙、负载均衡等网络功能
4. **传统应用迁移**: 为需要 L2 网络的传统应用提供连通性

## 安装

```bash
# Helm 安装
helm repo add networkservicemesh https://networkservicemesh.github.io/charts
helm install nsm networkservicemesh/nsm \
  --set spire.enabled=true \
  --set forwarder.type=kernel
# 注册 NSE
kubectl apply -f - <<EOF
apiVersion: networkservicemesh.io/v1
kind: NetworkService
metadata: { name: secure-intranet }
spec:
  payload: IP
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **NSM** | L2/L3 连接、灵活拓扑 | 社区小、文档少 |
| Submariner | 跨集群 Pod 通信、成熟 | 仅跨集群，非通用 L2/L3 |
| Cilium Cluster Mesh | eBPF 高性能 | 仅 Cilium 环境 |
| Tailscale/Kubelet | 简单 VPN | 非 K8s 原生 |

## 架构定位

在 CNCF 生态中，NSM 属于 **Networking** 类别，专注于 L2/L3 层的动态网络服务连接。它与传统 L4-L7 服务网格互补。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[实体/cni-plugins.md|cni-plugins]]
- [[deployment]]
- [[概念/service-mesh-architecture.md|service-mesh-architecture]]

## Related

- [[chaosblade]] — ChaosBlade
- [[istio]] — Istio
- [[linkerd]] — Linkerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[spire]] — SPIRE

- network-service-mesh
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
