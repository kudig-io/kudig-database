---
title: LoxiLB [entities]
description: '## 概述'
summary: 'LoxiLB 是一个基于 eBPF 的云原生负载均衡器，专注于为 Kubernetes 提供高性能的 L4 负载均衡服务。'
category: entities
tags:
- k8s
- cncf
- networking
- loxilb
- cilium
- opa
- ingress
- crd
- operator
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- LoxiLB 是什么
- 如何 LoxiLB
trigger_keywords:
- LoxiLB
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# LoxiLB

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, C

## 概述

LoxiLB 是一个基于 eBPF 的云原生负载均衡器，由 LoxiLab 团队开发，2023 年加入 CNCF 沙箱。它专注于为 Kubernetes 提供高性能的 L4 负载均衡服务，可作为 Kubernetes 的 [[Service|Service]] LoadBalancer、[[Ingress|Ingress]] 控制器或独立负载均衡网关运行。LoxiLB 利用 eBPF/XDP 技术在内核数据面实现线速转发，支持 BGP、ECMP、DSR（Direct Server Return）等高级网络特性，在 10Gbps+ 吞吐场景下相比 kube-proxy/iptables 有数量级的性能提升。它还原生支持 IPv6、NAT46/64、防火墙和流量镜像等功能。

## 核心能力

- **eBPF/XDP 数据面**: 内核态线速转发，绕过传统 iptables 性能瓶颈
- **BGP/ECMP 路由**: 与上游路由器建立 BGP 邻居，实现 VIP 自动广播和多路径负载均衡
- **DSR 模式**: Direct Server Return，后端直接回包给客户端，减少 LB 节点带宽压力
- **多协议支持**: TCP、UDP、SCTP、HTTP、QUIC 负载均衡
- **健康检查**: 主动/被动健康检查，自动剔除不健康后端
- **NAT 和防火墙**: 内置 SNAT、DNAT、NAT46/64 和 ACL 规则能力

## 架构

LoxiLB 采用 eBPF 驱动的高性能架构：

- **LoxiLB Agent**: 用户态控制面，管理负载均衡规则、BGP 邻居和健康检查
- **eBPF 程序**: 挂载在 XDP/TC hook，在内核态处理数据包转发和负载均衡
- **eBPF Maps**: 内核态数据结构，存储 LB 规则、后端列表和连接状态
- **BGP 守护进程**: 与上游路由器交换路由信息，广播 VIP 可达性
- **kube-loxilb**: Kubernetes 集成组件，监听 Service 资源并创建 LoxiLB 规则

数据流：`客户端 → 路由器 (BGP) → LoxiLB 节点 (eBPF/XDP) → 后端 Pod → 客户端 (DSR)`

## K8s 集成

LoxiLB 通过 **kube-loxilb** 组件与 Kubernetes 集成。kube-loxilb 以 Deployment 方式运行在集群中，监听类型为 LoadBalancer 的 Service 资源，自动为其分配 External IP 并在 LoxiLB 中创建负载均衡规则。通过 `loxilb.io/rr-mode` 等 annotation 控制 BGP/DSR 行为。LoxiLB 可以作为集群内组件（in-cluster 模式）或集群外独立 LB 节点（external 模式）运行，适合裸金属集群和私有云场景。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中基于 iptables 的 kube-proxy 相比，eBPF 数据面消除了规则同步延迟。

## 生产场景

1. **裸金属集群 LB**: 无云厂商 LoadBalancer 时，为 Kubernetes Service 提供外部可达的 VIP
2. **高性能流量入口**: 5G UPF、电信级 VNF 等需要 10Gbps+ 吞吐的 L4 负载均衡场景
3. **多集群流量调度**: 通过 BGP Anycast 实现跨集群的流量调度和容灾
4. **DSR 高吞吐场景**: 视频流、大文件传输等回程流量大的场景，通过 DSR 降低 LB 节点压力

## 安装

```bash
# 方式一：Helm 安装（集群内模式）
helm repo add loxilb https://loxilb-io.github.io/loxilb/
helm install loxilb loxilb/loxilb -n kube-system
helm install kube-loxilb loxilb/kube-loxilb -n kube-system

# 方式二：直接 YAML 部署
kubectl apply -f https://github.com/loxilb-io/loxilb/raw/main/manifest/loxilb.yaml
kubectl apply -f https://github.com/loxilb-io/kube-loxilb/raw/main/manifest/kube-loxilb.yaml

# 创建 LoadBalancer 类型 Service
kubectl expose deployment my-app --port=80 --type=LoadBalancer
```

## 对比

| 特性 | LoxiLB | MetalLB | kube-vip | Cilium LB |
|------|--------|---------|----------|-----------|
| 数据面 | eBPF/XDP | iptables/IPVS | iptables/arpping | eBPF |
| 性能 | 极高（线速） | 中 | 中 | 高 |
| BGP | ✅ | ✅ | ⚠️ 有限 | ❌ |
| DSR | ✅ | ❌ | ❌ | ⚠️ 有限 |

## 架构定位

在 CNCF 生态中，LoxiLB 属于 **Networking** 类别，为云原生应用提供基于 eBPF 的高性能 L4 负载均衡能力。

## 参考链接

- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]

## Related

- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[serverless-devs]] — Serverless Devs
- [[sermant]] — Sermant
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- loxilb
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
