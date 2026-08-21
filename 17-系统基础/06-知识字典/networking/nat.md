---
title: 网络地址转换
description: NAT（Network Address Translation，网络地址转换）是将一个 IP 地址和端口映射到另一个的过程。在 Kubernetes
  中，NAT...
summary: NAT（Network Address Translation，网络地址转换）是将一个 IP 地址和端口映射到另一个的过程。在 Kubernetes
  中，NAT...
category: dictionary
tags:
- k8s
- glossary
- nat
- networking
- kube-proxy
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 网络地址转换 是什么
- NAT (Network Address Translation) 详解
trigger_keywords:
- 网络地址转换
- NAT (Network Address Translation)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 网络地址转换

> **英文名**: NAT (Network Address Translation)

## 概述

NAT（Network Address Translation，网络地址转换）是将一个 IP 地址和端口映射到另一个的过程。在 Kubernetes 中，NAT 是 Service 实现流量转发的核心机制，由 kube-proxy 通过 iptables 或 IPVS 规则执行 SNAT 和 DNAT。

## 核心概念/原理

### Kubernetes 中的 NAT 类型

| 类型 | 方向 | 用途 |
|------|------|------|
| DNAT | 入站 | Service ClusterIP → Pod IP |
| SNAT (Masquerade) | 出站 | Pod 访问外部时隐藏源 IP |

### 工作原理

```
Client → Service ClusterIP:Port
       → [kube-proxy DNAT] → Pod IP:Port
Pod → External
       → [Masquerade SNAT] → Node IP → External
```

## 关键机制或特性

- kube-proxy 的 iptables 模式通过 `KUBE-SERVICES` 和 `KUBE-SVC-*` 链实现 DNAT。
- IPVS 模式使用内核 IPVS 模块，性能优于 iptables。
- `externalTrafficPolicy: Local` 保留客户端源 IP（不做 SNAT）。
- `masquerade-all` 配置强制对所有出站流量做 SNAT。

## 使用场景与最佳实践

- 需要保留客户端源 IP 时使用 `externalTrafficPolicy: Local`。
- 大规模集群优先使用 IPVS 模式替代 iptables。
- 排查 NAT 问题时使用 `iptables -t nat -L -n` 检查规则。
- 注意 SNAT 对网络策略和日志的影响（源 IP 变为节点 IP）。

## 架构深度解析

### Kubernetes 中的 NAT 类型

```
┌──────────────────────────────────────────────────────────────┐
│  SNAT（源地址转换）                                            │
│  Pod(10.244.1.5) ──→ 外部(203.0.113.5)                       │
│       │  MASQUERADE：源地址 → 节点 IP（eth0 主地址）           │
│       │  回程流量由 conntrack 反向转换恢复                    │
│       ▼                                                       │
│  DNAT（目的地址转换）                                          │
│  客户端(172.16.0.5) ──→ ClusterIP:80                          │
│       │  KUBE-SERVICES 链匹配 → 后端 Pod IP:port              │
│       │  回程由 conntrack 恢复为 ClusterIP                    │
│       ▼                                                       │
│  MASQUERADE（双向）                                            │
│  节点 A → 节点 B 的跨节点转发（externalTrafficPolicy: Cluster）│
│       │  源地址从 Pod IP 变为节点 A IP（避免回程路由问题）     │
│       ▼                                                       │
│  conntrack 表：所有 NAT 会话的映射记录                          │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| kube-proxy iptables | `pkg/proxy/iptables/proxier.go` | 生成 KUBE-SERVICES/KUBE-NODEPORTS 链与 DNAT 规则、MASQUERADE 规则 |
| kube-proxy IPVS | `pkg/proxy/ipvs/proxier.go` | VS/RS 条目 + `--masquerade-all` 全量 SNAT 策略 |
| conntrack 管理 | kubelet `pkg/kubelet/network/` | 清理异常 conntrack 条目（如 IP 变化后的残留映射） |
| 内核 netfilter | Linux `net/ipv4/netfilter/` | conntrack 表维护、NAT 转换执行 |

### 流程步骤

1. Pod 访问外部：出站包经 `KUBE-POSTROUTING` 链匹配 MASQUERADE 规则，源 IP 改为节点 IP。
2. 回程包到达节点：conntrack 查表还原源 IP 为 Pod IP，转发给 Pod。
3. 外部访问 ClusterIP：`KUBE-SERVICES` 链 DNAT 到后端 Pod；回程反向还原。
4. 跨节点转发（Cluster 模式）：第二次 MASQUERADE 保证回程路径一致（到源节点再还原）。
5. conntrack 表容量决定集群 NAT 吞吐上限，表满时新连接被丢弃。

## 生产案例

### 案例 1：conntrack 表满导致集群大面积网络故障

| 时间 | 事件 |
|------|------|
| 10:00 | 压测流量涌入，节点 CPU 飙升 |
| 10:05 | 大量服务报连接超时，`dmesg` 出现 `nf_conntrack: table full, dropping packet` |
| 10:10 | `conntrack -S` 显示 insert_failed 计数暴涨 |
| 10:15 | 临时调大 `nf_conntrack_max`（从 65 万 → 100 万）缓解 |
| 10:30 | 排查到某服务短连接风暴（HTTP 连接不复用），每秒新建 2 万连接 |
| 11:00 | 应用侧修复连接复用 + 调大 conntrack + 部署 NodeLocal DNSCache 后恢复 |

**根因**：短连接风暴 + DNS 查询占用 conntrack 条目，表容量耗尽后新连接全部丢弃。
**修复命令**：
```bash
# 查看 conntrack 统计 🟢 只读
conntrack -S
# 查看表容量与当前条目 🟢 只读
sysctl net.netfilter.nf_conntrack_max net.netfilter.nf_conntrack_count
# 临时调大（持久化需写入 /etc/sysctl.d/）🟡 中风险
sudo sysctl -w net.netfilter.nf_conntrack_max=1000000
```

### 案例 2：externalTrafficPolicy=Local 后连接全部失败

**现象**：将服务改为 Local 模式后，云 LB 健康检查失败，服务从 LB 摘除。
**诊断**：健康检查打到 `healthCheckNodePort`，但节点上无该服务后端 Pod，返回 503；`kubectl get svc -o yaml` 确认 `externalTrafficPolicy: Local`。
**修复**：确保每个节点都有后端 Pod（DaemonSet 化）或使用拓扑感知调度；或改回 Cluster 模式接受源 IP 丢失。

## 对比评测

| 维度 | iptables NAT | IPVS NAT | eBPF（Cilium） |
|------|-------------|----------|----------------|
| 规则复杂度 | O(n) 链式匹配 | O(1) 哈希查找 | O(1) BPF map |
| 大规模性能 | 差（规则多时慢） | 好 | 极好 |
| 源 IP 保留 | 需 Local 策略 | 同左 | 原生支持（DSR） |
| 可观测性 | 弱 | 中（ipvsadm） | 强（Hubble） |
| 适用规模 | < 5000 条规则 | 大规模 | 大规模 + 性能敏感 |

**选型建议**：大规模集群用 IPVS 或 eBPF；需保留源 IP 且要求高性能时优先 eBPF DSR 模式。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 连接超时/丢包 | `dmesg \| grep nf_conntrack`；`conntrack -S` | conntrack 表满 |
| 源 IP 变节点 IP | `iptables -t nat -L KUBE-POSTROUTING` | 默认 MASQUERADE、Cluster 策略 |
| 服务间访问偶发失败 | `conntrack -L -d <svcIP>` | 残留 conntrack 条目（服务 IP 复用） |
| IPVS 模式下规则不生效 | `ipvsadm -Ln` | kube-proxy 未同步、rs 异常 |
| 删除服务后旧连接仍通 | `conntrack -L -d <oldIP>` | conntrack 未清理（可 conntrack -D） |

## 生产部署清单

- [ ] conntrack 上限按节点 Pod 数估算（Pod 数 × 会话数 × 系数）
- [ ] 应用侧连接复用已开启（HTTP keep-alive、连接池）
- [ ] 确认 externalTrafficPolicy 与源 IP 需求匹配
- [ ] IPVS 模式启用 `--masquerade-all=false` 且验证回程路由
- [ ] 监控 conntrack 使用率（node_exporter `nf_conntrack_entries`）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | conntrack 表满导致业务受损 | 立即调大上限 + 定位连接风暴源 |
| P1 | 集群规模 > 1000 节点仍用 iptables | 评估切换 IPVS/eBPF 数据面 |
| P1 | 业务强依赖源 IP（风控/审计） | 迁移到 Local 策略或 eBPF DSR |
| P2 | NAT 运行稳定且无告警 | 保持现状，定期压测验证上限 |

## 面试要点

> 以下 Q&A 覆盖 NAT 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Kubernetes 中 SNAT、DNAT、MASQUERADE 分别用在哪些场景？**
   A：DNAT 用于 Service 流量转发（ClusterIP:port → Pod IP:port，KUBE-SERVICES 链）；SNAT/MASQUERADE 用于：① Pod 出站访问外部（源 IP 换成节点 IP）；② externalTrafficPolicy=Cluster 时的跨节点二次转发（保证回程路径）；③ 某些 CNI 的 masquerade-all 模式。MASQUERADE 与 SNAT 的区别是前者动态选择出口 IP（多 IP 网卡时），后者固定指定源地址。

2. **Q：为什么外部访问 Service 会看到源 IP 是节点 IP？如何保留真实源 IP？**
   A：默认 Cluster 模式下，外部流量 DNAT 到 Pod 时，若后端不在入口节点，会再次转发并做 MASQUERADE，源 IP 变为入口节点 IP。保留源 IP 的方法：① `externalTrafficPolicy: Local`（仅本节点后端接收，无二次转发）；② eBPF DSR 模式（Cilium 直接回复客户端，天然保留源 IP）；③ 云 LB 开启 proxy protocol 并透传。注意 Local 模式牺牲了跨节点负载均衡。

3. **Q：conntrack 表满的故障特征是什么？如何预防？**
   A：特征：`dmesg` 报 `nf_conntrack: table full, dropping packet`，新连接超时但已有连接正常，`conntrack -S` 的 insert_failed 增长。预防：① 按峰值并发估算并调大 `nf_conntrack_max`；② 应用层连接复用（减少新建连接）；③ 大集群用 NodeLocal DNSCache 减少 DNS 连接占表；④ 监控 `nf_conntrack_entries` 使用率，>80% 告警；⑤ 短连接风暴场景配合连接跟踪超时调优（`nf_conntrack_tcp_timeout_established`）。

## 参考链接

- [NAT - Wikipedia](https://en.wikipedia.org/wiki/Network_address_translation)

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|ClusterIP]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|NodePort]]
- [[17-系统基础/06-知识字典/networking/loadbalancer.md|LoadBalancer]]
- [[17-系统基础/06-知识字典/fundamentals/kube-proxy.md|Kube-proxy]]


<!-- risk-assessed -->
