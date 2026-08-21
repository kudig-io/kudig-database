---
title: LoxiLB eBPF 负载均衡
description: LoxiLB 是基于 eBPF 的高性能外部负载均衡器，专为 Kubernetes 设计，提供 L4/L7 负载均衡和 NAT，可替代 MetalLB
  + ku...
summary: LoxiLB 是基于 eBPF 的高性能外部负载均衡器，专为 Kubernetes 设计，提供 L4/L7 负载均衡和 NAT，可替代 MetalLB
  + ku...
category: dictionary
tags:
- k8s
- glossary
- networking
- load-balancer
- ebpf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- LoxiLB eBPF 负载均衡 是什么
- LoxiLB 详解
trigger_keywords:
- LoxiLB eBPF 负载均衡
- LoxiLB
- dictionary
prerequisites:
- kubernetes
---



# LoxiLB eBPF 负载均衡（LoxiLB）

## 概述

LoxiLB 是基于 eBPF 的高性能外部负载均衡器，专为 Kubernetes 设计，提供 L4/L7 负载均衡和 NAT，可替代 MetalLB + kube-proxy + external LB 的组合。

## 核心概念/原理

- **eBPF 驱动**：使用 eBPF/XDP 实现高性能数据面
- **多模式**：L4/L7 负载均衡、NAT、FW、Egress
- **K8s 原生**：Operator 模式部署，自动感知 Service
- **轻量级**：单进程，资源占用极低

## 关键机制或特性

- Service Type LoadBalancer 自动分配
- kube-proxy 替代（eBPF 模式）
- L4/L7 负载均衡（IPVS 替代）
- 多集群负载均衡
- SCTP 支持（5G/Telco 场景）
- 健康检查和故障转移
- Prometheus 指标导出

## 使用场景与最佳实践

- 裸金属/边缘环境的 LoadBalancer 实现
- MetalLB + kube-proxy 的统一替代
- 5G/Telco 的 SCTP 负载均衡
- 需要 eBPF 高性能的网络方案
- 轻量级外部负载均衡

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes 集群                       │
│  ┌──────────────────────────────────────────────────┐   │
│  │  loxilb 控制面（loxilb-operator + kube-loxilb）    │   │
│  │  - 监听 LoadBalancer Service / EndpointSlice      │   │
│  │  - 生成 NAT 规则与负载均衡配置下发                 │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  loxilb 数据面（每节点 DaemonSet，eBPF/XDP）       │   │
│  │  - XDP 入口处理：DNAT + 负载均衡                  │   │
│  │  - 会话跟踪（conntrack）与健康检查                │   │
│  │  - 支持 L4（TCP/UDP/SCTP）+ 有限 L7               │   │
│  └──────────────────────────────────────────────────┘   │
│                                                         │
│  外部客户端 ──▶ VIP（XDP 钩子）──▶ 后端 Pod（DNAT）        │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（loxilb-io/loxilb）

| 模块 | 路径 | 职责 |
|------|------|------|
| 数据面 | `src/` | XDP/eBPF 负载均衡、NAT、会话管理 |
| 控制面 | `pkg/` | 规则生成、健康检查、API 服务 |
| K8s 集成 | `kube-loxilb/` | K8s Service/EndpointSlice 监听与下发 |
| Operator | `loxilb-operator/` | 部署编排与配置管理 |

### 流量处理流程（XDP 模式）

1. 外部请求到达节点网卡，XDP 程序在驱动层（早于协议栈）拦截
2. 匹配 VIP 会话表：已建立会话直接转发（快速路径）
3. 新连接：哈希选择后端 Pod，执行 DNAT 并记录会话
4. 健康检查剔除失败后端，会话迁移到健康实例
5. 支持 SCTP 等特殊协议（kube-proxy 无法处理的场景）

## 生产案例

### 案例 1：XDP 模式与网卡特性冲突导致丢包

| 时间 | 事件 |
|------|------|
| 15:00 | 启用 XDP 模式后，部分节点出现随机丢包 |
| 15:10 | `xdpdump` 抓包发现 GRO 合并的报文被 XDP 程序丢弃 |
| 15:20 | 定位为网卡 GRO/LRO 开启时，XDP 程序未处理合并报文 |
| 15:35 | 关闭 GRO（`ethtool -K eth0 gro off`）或升级驱动，丢包消失 |

**根因**：部分网卡驱动在 GRO 合并后提交给 XDP 的报文包长超过预期，程序校验失败丢包；与驱动/固件版本相关。

**修复命令**：
```bash
# 检查网卡特性 🟢 只读
ethtool -k eth0 | grep -E "generic-receive-offload|large-receive-offload"
# 关闭 GRO/LRO（节点侧）🟡 中风险
ethtool -K eth0 gro off
ethtool -K eth0 lro off
# 查看 loxilb 丢包统计 🟢 只读
kubectl -n kube-system logs ds/loxilb | grep -i drop | tail -20
```

### 案例 2：会话同步失效导致滚动升级期间连接中断

**现象**：节点滚动升级（重启 loxilb）期间，存量长连接全部中断。

**诊断**：loxilb 默认会话表在内存，节点重启即丢失；未启用会话同步（sync）或主备模式。

**修复**：启用 loxilb 的会话同步（`--sync`）或部署主备（Active-Standby）模式；业务侧配置连接重连与超时重试。

## 对比评测

| 维度 | loxilb | MetalLB | kube-vip | kube-proxy IPVS |
|------|--------|---------|----------|-----------------|
| 数据面 | XDP/eBPF（驱动层） | FRR（用户态） | IPVS/nftables | IPVS |
| 性能 | 最高（旁路协议栈） | 中 | 中 | 中 |
| 协议支持 | TCP/UDP/SCTP | TCP/UDP | TCP/UDP | TCP/UDP |
| 会话保持 | ✅ 内置会话表 | ❌ | ❌ | ✅ |
| 适用场景 | 5G/Telco 高性能 | 通用裸金属 | 轻量 HA+LB | 通用 |

**选型建议**：5G/Telco 或超高吞吐选 loxilb；通用场景 MetalLB/kube-vip 足够；保持 kube-proxy 语义选 IPVS 模式。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 随机丢包 | `xdpdump -i eth0` | GRO 合并报文或驱动缺陷 |
| VIP 不通 | `loxilb -c` 查规则 | 规则未下发或网卡绑定失败 |
| 连接中断 | 查会话表 | 会话未同步或健康检查误判 |
| 后端不健康 | `kubectl get endpointslices` | 探针失败或 Endpoint 更新延迟 |

## 生产部署清单

- [ ] 网卡驱动与固件版本确认，GRO/LRO 策略明确
- [ ] 生产启用会话同步或主备模式
- [ ] VIP 与后端健康检查周期调优
- [ ] XDP 模式性能压测（iperf3）验证吞吐
- [ ] 节点升级演练验证连接保持能力

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 丢包或连接中断影响业务 | 立即回退 iptables/IPVS 模式排查 |
| P1 | 网卡驱动升级 | 预发节点验证 XDP 兼容性再全量 |
| P2 | 需要 L7 能力扩展 | 评估叠加 Ingress/网关层而非扩展 loxilb |

## 运维要点

- loxilb 以独立 Pod 运行，数据面使用 eBPF/DPDK，需特权模式与独立网卡（默认 eBPF 模式）。
- 通过 `kubectl get lb` 查看负载均衡实例，`loxilb` CRD 与 `kubectl port-forward` 结合调试。
- 多副本部署时使用 BGP/ECMP 或外部 LB 分发，避免单点。
- 升级先验证内核 eBPF 兼容性（内核 ≥ 5.4），DPDK 模式需独立节点池。

## 面试要点

> 以下 Q&A 覆盖 loxilb 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：loxilb 的 XDP 模式为什么比 IPVS 性能更高？**
   A：XDP 在网络驱动层（早于内核协议栈）处理报文，避免 sk_buff 分配、协议栈解析与上下文切换；IPVS 位于 netfilter 框架，报文已进入协议栈。XDP 模式下单核可处理数百万 PPS，且旁路协议栈降低了延迟抖动。

2. **Q：loxilb 如何与 Kubernetes Service 模型对接？**
   A：kube-loxilb 组件监听 LoadBalancer Service 与 EndpointSlice：为 Service 分配/绑定 VIP，将 EndpointSlice 变化编译为 loxilb 的负载均衡规则（含权重与健康检查），实现 K8s 声明式语义下的高性能负载均衡，无需 kube-proxy。

3. **Q：SCTP 负载均衡为什么是 loxilb 的差异化能力？**
   A：SCTP 是 5G 信令面（NGAP/S1AP）的核心协议，而 kube-proxy 的 iptables/IPVS 对 SCTP 支持有限（IPVS 需内核支持且无会话关联处理）；loxilb 在 eBPF 层实现完整的 SCTP 会话跟踪与 NAT，满足 Telco 场景的硬性要求。

## 参考链接

- https://loxilb.io/
- https://github.com/loxilb-io/loxilb

## Related

- [[17-系统基础/06-知识字典/networking/metallb.md|MetalLB]]
- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium]]
- [[17-系统基础/06-知识字典/networking/kube-vip.md|kube-vip]]
