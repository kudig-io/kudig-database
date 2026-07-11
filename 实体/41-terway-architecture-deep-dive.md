---
title: Terway 架构深度解析
description: '# Terway 架构深度解析'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- networkpolicy
- crd
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
- Terway 架构深度解析 是什么
- 如何 Terway 架构深度解析
trigger_keywords:
- Terway
- 架构深度解析
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Terway 架构深度解析

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

Terway 是阿里云 ACK 集群的默认 CNI（Container Network Interface）插件，由阿里云容器服务团队开发。它通过 ENI（Elastic Network Interface）将 Pod 直接接入阿里云 VPC 网络，使 Pod 获得 VPC 内原生 IP 地址，无需额外的 overlay 封装或 NAT 转换。本页深入解析 Terway 的架构设计、网络模型和数据面路径。

Terway 的核心架构设计目标是"**VPC 原生 + 高性能 + 兼容 K8s 网络模型**"。与 Calico/Flannel 的 overlay 网络不同，Terway 使 Pod IP 就是 VPC IP，可以直接被 VPC 内其他资源（ECS、RDS）访问。与 Cilium 的 VPC-native CNI 相比，Terway 深度集成了阿里云 ENI 能力，提供 EIP 绑定、安全组管理等云原生特性。

## 网络模式

- **ENI 多 IP 模式（共享 ENI）**：一个 ENI 的多个辅助私有 IP 分配给不同 Pod，Pod 通过 Veth Pair + 节点路由共享 ENI 出口。默认模式，IP 利用率高。
- **ENI 独占模式**：每个 Pod 绑定一个完整的 ENI，流量直接通过 ENI 硬件转发，不经过节点内核网络栈。性能最优，但每 Pod 消耗一个 ENI。
- **IPvlan 模式**：使用 Linux IPvlan 驱动在 ENI 上创建虚拟接口，结合 eBPF 实现高性能数据面。

## Architecture

Terway 架构分为三层：**控制面**（terway-controller，管理集群级 ENI 生命周期和 IPAM）、**节点面**（terway-daemon，运行在每个节点，管理本地 ENI 和 IP 分配）、**数据面**（Veth Pair + 内核路由 + iptables/eBPF）。当 kubelet 创建 Pod 时，调用 Terway CNI binary → terway-daemon 分配 VPC IP → 创建 Veth Pair 连接 Pod 和节点网络栈 → 配置路由规则使流量通过 ENI 到达 VPC。

### IPAM 机制

terway-daemon 维护一个本地 IPAM 池。当节点上 Pod 数增加时，terway-daemon 通过阿里云 ENI API 为该节点的 ENI 申请新的辅助 IP，扩充本地池。当 Pod 删除时，IP 回收到本地池。如果本地池耗尽，新的 CNI 调用会阻塞等待 IP 分配，导致 Pod 卡在 ContainerCreating。

### NetworkPolicy 引擎

Terway 支持两种 NetworkPolicy 实现：**iptables 模式**（兼容性好，但规则数多时性能下降）和 **eBPF 模式**（高性能，规则数量不影响转发性能）。eBPF 模式通过在 XDP/TC 层加载 eBPF 程序实现策略匹配，绕过 iptables。

## K8s 集成

Terway 完全实现 Kubernetes CNI 规范。它通过 Kubernetes API Watch Pod/NetworkPolicy 变更，自动配置网络。`terway-daemon` 作为 DaemonSet 运行在 kube-system。也通过 CRD（`SecurityGroup`、`NetworkPolicy` 扩展）提供阿里云特有的安全组管理能力。

## 生产部署要点

- **ENI 多 IP 模式**：建议生产环境使用 ENI 多 IP 模式以提高 IP 利用率
- **密切监控**：监控 ENI 资源使用情况，避免 IP 耗尽
- **NetworkPolicy**：配合 NetworkPolicy 实现 Pod 间访问控制
- **eBPF 模式**：规则数 >100 时切换到 eBPF 引擎

## 生产场景

1. **标准 ACK 集群**：默认使用 ENI 多 IP 模式，Pod 获得原生 VPC IP
2. **高性能计算**：AI/大数据场景使用 ENI 独占模式获得线速网络
3. **网络隔离**：通过安全组 CRD 实现细粒度的 Pod 网络隔离
4. **混合通信**：Pod 直接与 VPC 内 ECS/RDS 通信，无需 NAT

## 操作命令

```bash
# 🟢 查看 Terway 架构配置
kubectl get cm eni-config -n kube-system -o yaml | grep -A5 eni
kubectl get cm terway-config -n kube-system -o yaml

# 🟢 查看 Terway 控制器和 DaemonSet
kubectl get deploy,ds -n kube-system -l app=terway

# 🟢 节点上检查 ENI 数据面
ip link show | grep eth1    # 查看附加的 ENI
ip addr show eth1           # 查看 ENI 的辅助 IP
ip route                    # 查看路由规则
tc filter show dev eth1 ingress  # 查看 eBPF TC 过滤器

# 🟢 查看 eBPF NetworkPolicy 程序
bpftool prog show | grep terway
```

## 对比

| 特性 | Terway | Cilium | Calico | Flannel |
|------|--------|--------|--------|---------|
| VPC 原生 IP | ✅ 阿里云 | ✅ 通用 | ⚠️ | ❌ |
| Overlay | ❌ | ✅ | ✅ | ✅ |
| eBPF 数据面 | ✅ | ✅ | ✅ | ❌ |
| 安全组集成 | ✅ | ❌ | ❌ | ❌ |

## 参考链接

- [[cilium]]
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[实体/cni-plugins.md|cni-plugins]]
- [[实体/networkpolicy.md|networkpolicy]]

## Related

- [[43-terway-crd-operations]] — Terway CRD 资源操作
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[44-terway-operations-manual]]
- [[40-terway-product-overview]]
- [[42-terway-usage-guide]]
- [[46-terway-performance-tuning]]
- [[45-terway-testing-validation]]
- [[47-terway-troubleshooting-fta]]
- 41-terway-architecture-deep-dive
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
