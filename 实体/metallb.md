---
title: MetalLB (entities)
description: '## 概述'
summary: 'MetalLB 是为裸金属 Kubernetes 集群提供的负载均衡器实现。在云环境中，Kubernetes LoadBalancer 类型的 [[Service|Service]] 由云提供商自动配置。MetalLB 填补了裸金属环境的空白，通过 Layer 2 (ARP/NDP) 或 BGP 协议为 Service 分配和公告外部 IP 地址。'
category: entities
tags:
- k8s
- cncf
- networking
- metallb
- prometheus
- grafana
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- MetalLB 是什么
- 如何 MetalLB
trigger_keywords:
- MetalLB
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# MetalLB

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

MetalLB 是为裸金属 Kubernetes 集群提供的 LoadBalancer 实现，2021 年加入 CNCF Sandbox，后晋升为 Incubating。在云环境中，Kubernetes LoadBalancer 类型的 [[Service|Service]] 由云提供商自动配置。MetalLB 填补了裸金属环境的空白，通过 Layer 2（ARP/NDP）或 BGP 协议为 Service 分配和公告外部 IP 地址。它是裸金属集群使用 LoadBalancer Service 的标准方案。

## 核心特性

- **Layer 2 模式**: 使用 ARP（IPv4）/ NDP（IPv6）响应本地网络请求
- **BGP 模式**: 与网络路由器建立 BGP 会话公告 Service IP
- **IP 地址池**: 灵活配置可分配的 IP 地址范围和分配策略
- **自动故障转移**: Leader 选举确保 L2 模式高可用
- **双栈支持**: 同时支持 IPv4 和 IPv6
- **CRD 配置**: 使用 IPAddressPool、L2Advertisement、BGPAdvertisement CRD

## 架构

MetalLB 由两个组件组成。MetalLB Controller（Deployment，集群级单实例）监听 Kubernetes Service 变更，当发现 `type: LoadBalancer` 的 Service 时，从 IPAddressPool 中分配一个 IP 并更新 Service 的 `status.loadBalancer.ingress`。MetalLB Speaker（DaemonSet，每个节点一个）负责公告 IP 地址。Layer 2 模式下，Leader 节点的 Speaker 发送 Gratuitous ARP/NDP 响应，使局域网将流量发到该节点。BGP 模式下，所有 Speaker 节点与上游路由器建立 BGP 会话，公告 Service VIP，路由器通过 ECMP 将流量分发到多个节点。

## Kubernetes 集成

MetalLB 通过 CRD 和 Cloud Controller Manager 接口集成。IPAddressPool CRD 定义 IP 地址池。L2Advertisement/BGPAdvertisement CRD 定义公告模式。Controller 作为 Kubernetes Service 的 LoadBalancer Controller 运行，自动为 `type: LoadBalancer` Service 分配 IP。Speaker 通过 DaemonSet 运行。支持与 kube-proxy 协同工作——MetalLB 负责将外部流量引入集群节点，kube-proxy 负责将流量路由到目标 Pod。

## 生产使用场景

1. **裸金属集群入口**: 为裸金属 K8s 集群的 Ingress Controller 提供 LoadBalancer IP
2. **BGP 负载均衡**: 与数据中心交换机建立 BGP，实现多节点流量分发
3. **多 VIP 管理**: 为多个服务分配和管理外部 IP
4. **混合网络**: 在非云环境中实现类似云 ELB 的流量入口

## 安装

```bash
# Helm 安装
helm repo add metallb https://metallb.github.io/metallb
helm install metallb metallb/metallb -n metallb-system --create-namespace
# 配置 IP 地址池和公告模式
kubectl apply -f - <<EOF
apiVersion: metallb.io/v1beta1
kind: IPAddressPool
metadata: { name: default-pool }
spec:
  addresses: ["192.168.1.240-192.168.1.250"]
---
apiVersion: metallb.io/v1beta1
kind: L2Advertisement
metadata: { name: default }
spec:
  ipAddressPools: ["default-pool"]
EOF
# 创建 LoadBalancer Service
kubectl expose deployment web --port=80 --type=LoadBalancer
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **MetalLB** | CNCF Incubating、L2+BGP | L2 单节点瓶颈 |
| kube-vip | 轻量、双用途 | BGP 不如 MetalLB |
| Porter (VXLAN) | VXLAN 隧道 | 功能单一 |
| Cloud LB (ELB/SLB) | 云原生、成熟 | 仅限云环境 |

## 架构定位

在 CNCF 生态中，MetalLB 属于 **Networking / Load Balancing** 类别，是裸金属 Kubernetes 集群 LoadBalancer 的事实标准。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[cortex]] — Cortex
- [[kepler]] — Kepler
- [[kubestellar]] — KubeStellar
- [[athenz]] — Athenz
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- metallb
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
