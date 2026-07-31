---
title: MetalLB
description: MetalLB 是裸金属（Bare Metal）Kubernetes 集群的负载均衡器实现。它为不支持云厂商 LoadBalancer 的环境（如
  on-pre...
summary: MetalLB 是裸金属（Bare Metal）Kubernetes 集群的负载均衡器实现。它为不支持云厂商 LoadBalancer 的环境（如
  on-pre...
category: dictionary
tags:
- k8s
- glossary
- metallb
- loadbalancer
- networking
- bare-metal
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- MetalLB 是什么
- MetalLB 详解
trigger_keywords:
- MetalLB
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# MetalLB

> **英文名**: MetalLB

## 概述

MetalLB 是裸金属（Bare Metal）Kubernetes 集群的负载均衡器实现。它为不支持云厂商 LoadBalancer 的环境（如 on-premises）提供 LoadBalancer 类型的 Service 支持，是本地 K8s 集群的必备组件。

## 核心概念/原理

### 工作模式

| 模式 | 说明 | 适用场景 |
|------|------|----------|
| Layer 2 | ARP/NDP 应答 | 简单场景，单节点故障转移 |
| BGP | 与路由器对等 | 大规模，多路径，快速故障转移 |

### 工作原理

```
External Client → LoadBalancer IP → [MetalLB ARP/BGP] → Node → kube-proxy → Pod
```

## 关键机制或特性

- **IP Address Pool**：定义可分配的 IP 地址范围。
- **L2 Advertisement**：通过 ARP 通告 VIP。
- **BGP Advertisement**：通过 BGP 协议通告路由。
- **speaker** DaemonSet：每节点运行，负责 IP 通告。
- **controller**：分配 IP 和管理配置。

## 使用场景与最佳实践

- 裸金属集群必须安装 MetalLB 支持 LoadBalancer Service。
- 简单场景使用 Layer 2 模式。
- 大规模生产环境使用 BGP 模式配合 ToR 交换机。
- 为不同 Service 分配不同的 IP Pool。
- 监控 MetalLB 的 BGP 会话状态和 IP 分配情况。

## 参考链接

- [MetalLB Official](https://metallb.universe.tf/)

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              MetalLB                                │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Controller  │  │ Speaker      │  │ L2/BGP    │  │
│  │ (IP 分配)   │  │ (DaemonSet)  │  │ Protocol  │  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │     IPAddressPool / L2Advertisement /       │  │
│  │     BGPAdvertisement / BGPPeer CRDs         │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（metallb/metallb）

| 模块 | 路径 | 职责 |
|------|------|------|
| Controller | `internal/controller/` | IP 分配逻辑、Pool 管理 |
| Speaker | `internal/speaker/` | 协议宣告（ARP/NDP/BGP） |
| L2 | `internal/layer2/` | ARP/NDP 响应实现 |
| BGP | `internal/bgp/` | BGP 会话管理（GoBGP） |
| CRD | `api/` | IPAddressPool/Advertisement 定义 |

### L2 模式工作原理

1. Service 创建 type=LoadBalancer → Controller 从 Pool 分配 IP
2. Speaker 通过 Leader Election 选举一个节点负责宣告
3. 负责节点响应 ARP/NDP 请求（“我是这个 IP”）
4. 流量到达该节点 → kube-proxy/IPVS 转发到 Pod
5. 节点故障时 Leader 切换，新节点接管 ARP 响应

## 生产案例

### 案例 1：ARP 风暴导致网络拥塞

| 时间 | 事件 |
|------|------|
| 08:00 | 网络监控发现 ARP 广播流量异常高 |
| 08:10 | 确认：MetalLB Speaker 在每个节点都发送 ARP 宣告 |
| 08:20 | 根因：L2Advertisement 未限制节点选择，所有 Speaker 都在宣告 |
| 08:30 | 修复：配置 `nodeSelectors` 限制只有特定节点参与 L2 宣告 |

**修复命令**：
```bash
# 检查 Speaker 状态 🟢 只读
kubectl get pods -n metallb-system -l app=speaker -o wide
# 查看 ARP 宣告 🟢 只读
kubectl logs -n metallb-system -l app=speaker --tail=50
# 限制 L2 宣告节点 🟡 中风险
kubectl patch l2advertisement default -n metallb-system -p '{"spec":{"nodeSelectors":[{"matchLabels":{"network":"lb"}}]}}'
```

### 案例 2：BGP 会话 flap 导致路由不稳定

**现象**：LoadBalancer IP 间歇性不可达，BGP 会话频繁重建。

**诊断**：ToR 交换机 BGP keepalive 超时设置过短，网络抨动时误判会话断开。

**修复**：调整 BGPPeer 的 `holdTime` 和 `keepaliveTime` 参数，启用 BFD 快速检测。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 所有 LB IP 不可达 | 检查 Speaker 状态，手动发送 ARP |
| P1 | 单 IP 无法访问 | 检查 Leader 节点，强制重新选举 |
| P2 | IP Pool 即将耗尽 | 扩展 Pool 范围，清理未使用 IP |

## 面试要点

1. **Q：MetalLB L2 模式与 BGP 模式的区别和适用场景？**
   A：L2 模式通过 ARP/NDP 宣告 IP，无需网络设备配合，但流量必须经过单节点（Leader），存在瓶颈；BGP 模式通过 BGP 协议向 ToR 交换机宣告路由，支持 ECMP 多路径，但需要网络设备支持 BGP。L2 适合小型集群/开发环境；BGP 适合生产环境大规模部署。

2. **Q：MetalLB 如何与 kube-proxy 协作？**
   A：MetalLB 只负责 IP 分配和宣告，不负责流量转发。流量到达节点后由 kube-proxy（iptables/IPVS）或 Cilium/Calico 的 Service 实现转发到 Pod。MetalLB 与 kube-proxy 是互补关系，不是替代关系。

3. **Q：如何在 MetalLB 中实现多租户 IP 隔离？**
   A：使用 IPAddressPool 的 `serviceAllocation` 字段：① 创建多个 Pool（如 pool-tenant-a、pool-tenant-b）；② 通过 `namespaces` 或 `namespaceSelectors` 限制每个 Pool 只能被特定 Namespace 使用；③ Service 通过 `spec.loadBalancerClass` 指定使用哪个 Pool。

## Related

- [[17-系统基础/06-知识字典/networking/loadbalancer.md|LoadBalancer]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|NodePort]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/fundamentals/kube-proxy.md|Kube-proxy]]


<!-- risk-assessed -->
