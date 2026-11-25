---
title: CNI 数据平面对比
description: iptables vs IPVS vs eBPF 三种 K8s 数据平面在 Pod-to-Service 数据路径上的差异
category: assets
tags:
- architecture
- diagram
- mermaid
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
---

# CNI 数据平面对比：iptables vs IPVS vs eBPF

## 三种数据路径对比图

```mermaid
graph LR
    subgraph SRC["源 Pod"]
        APP[应用 socket]
    end

    subgraph IPT["iptables 模式（默认）"]
        IPT1[conntrack 查询]
        IPT2[PREROUTING 链<br/>KUBE-SERVICES]
        IPT3[逐条匹配规则<br/>O(n) 线性扫描]
        IPT4[DNAT 改写目标 IP]
        IPT5[路由 → Pod]
        APP --> IPT1 --> IPT2 --> IPT3 --> IPT4 --> IPT5
    end

    subgraph IPVS["IPVS 模式"]
        IPVS1[Netfilter hook]
        IPVS2[IPVS 内核哈希表<br/>O(1) 查找]
        IPVS3[调度算法<br/>rr / lc / sh / wlc]
        IPVS4[DNAT / Direct Routing]
        IPVS5[路由 → Pod]
        APP --> IPVS1 --> IPVS2 --> IPVS3 --> IPVS4 --> IPVS5
    end

    subgraph EBPF["eBPF 模式（Cilium）"]
        EBPF1[socket 层 hook<br/>sockmap]
        EBPF2[BPF map 查 Service 后端]
        EBPF3[直接 connect 到 Pod IP<br/>跳过 iptables/qdisc]
        EBPF4[XDP / TC 加速]
        EBPF5[Pod 收包]
        APP --> EBPF1 --> EBPF2 --> EBPF3 --> EBPF4 --> EBPF5
    end
```

## 评估维度对比

| 维度 | iptables | IPVS | eBPF (Cilium) |
|---|---|---|---|
| 数据结构 | 线性规则链 | 内核哈希表 | BPF map（多级哈希） |
| 查找复杂度 | O(n) | O(1) | O(1) |
| 大规模 Service（>10k） | 规则更新慢、内存高 | 表现稳定 | 表现稳定 |
| 调度算法 | 随机 | rr/lc/sh/wlc/sed/dh | rr/random/maglev |
| 数据路径跳过 iptables | 否 | 否（仍用 NF hook） | 是（bypass 完整链路） |
| L7 能力 | 无 | 无 | 有（Envoy on eBPF） |
| NetworkPolicy | 需 CNI 额外实现 | 需 CNI 额外实现 | 原生 L4/L7 |
| 内核依赖 | 全部 Linux | ip_vs 模块 | ≥4.10 内核 / 5.4+ 推荐 |
| 状态同步 | kube-proxy 全量重写 | ipset + 增量 | BPF map 增量 |

## 详细解释

### iptables 模式

kube-proxy 将每条 Service 渲染为 PREROUTING 链中的若干规则：先匹配目的 IP（KUBE-SERVICES），再进入 KUBE-SVC-XXX 链（按概率跳转到 KUBE-SEP-XXX 后端规则），最终 DNAT。问题是规则数随 Service×Endpoint 线性增长——1 万 Service、平均 5 副本约 6 万条规则，每次 Endpoint 变更都要重建部分链，更新延迟可达数秒；内核 netfilter 也对长链做线性扫描。

### IPVS 模式

IPVS（IP Virtual Server）是专为负载均衡设计的内核模块，基于哈希表保存虚拟服务到后端的映射，查找 O(1)。kube-proxy 仍通过 Netfilter hook 介入，但后端表查询快得多。支持 10 种调度算法，原生处理 connection reuse（session affinity）、connection syncing。1.29 引入 nftables 模式（alpha）作为 iptables 的现代替代，结合 IPVS 的查询性能。

### eBPF 模式

Cilium 用 eBPF 在内核态 socket 层、TC、XDP 注入程序，Pod 访问 Service 时：①在 connect 系统调用阶段由 BPF 程序直接将目的 IP 改写为选中的后端 Pod IP；②完全跳过 iptables、conntrack、qdisc；③回程也通过 socket rewrite 还原，使应用无感知。这带来量级延迟改善（Cilium 基准：相比 iptables 时延 ↓50%、CPU ↓ 显著），并支持 L7 策略（HTTP/gRPC/Kafka）。代价是要求较新内核与 CONFIG_BPF、CONFIG_XDP 启用。

## 选型建议

- **小集群 / 兼容性优先**：iptables（默认）。
- **>1000 节点 / Service 数大**：IPVS，平滑迁移、改动小。
- **追求性能、可观测性、L7 策略、Service Mesh 一体**：Cilium eBPF（但要求新内核与运维能力）。
- **未来趋势**：nftables 渐进替代 iptables；eBPF 数据面与 Gateway API 深度整合成为云原生新基线。
