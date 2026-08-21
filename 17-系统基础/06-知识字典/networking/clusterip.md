---
title: 集群 IP
description: ClusterIP 是 Kubernetes Service 的默认类型，为 Service 分配一个集群内部的虚拟 IP 地址。只有集群内部的
  Pod 可以通...
summary: ClusterIP 是 Kubernetes Service 的默认类型，为 Service 分配一个集群内部的虚拟 IP 地址。只有集群内部的
  Pod 可以通...
category: dictionary
tags:
- k8s
- glossary
- networking
- service
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 集群 IP 是什么
- ClusterIP 详解
trigger_keywords:
- 集群 IP
- ClusterIP
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群 IP

> **英文名**: ClusterIP

## 概述

ClusterIP 是 Kubernetes Service 的默认类型，为 Service 分配一个集群内部的虚拟 IP 地址。只有集群内部的 Pod 可以通过 ClusterIP 访问该 Service。

## 核心概念/原理

### 核心概念

- **虚拟 IP**：ClusterIP 不绑定任何网络接口，由 kube-proxy 通过 iptables/IPVS 规则实现流量转发。
- **分配范围**：由 `--service-cluster-ip-range` 参数指定（如 `10.96.0.0/12`）。
- **无头服务**：设置 `clusterIP: None` 创建 Headless Service，DNS 直接返回后端 Pod IP。

### 使用场景

- 集群内部服务间的通信（如 API 服务调用数据库）。
- 不
需要外部访问的服务使用 NodePort/LoadBalancer/Ingress。

## 关键机制或特性

- ClusterIP 由 kube-proxy 通过 iptables/IPVS 实现，不依赖实际网络接口。
- 分配范围避免与 Pod CIDR 或 Node 网络冲突。
- Headless Service 适合 StatefulSet 和有状态应用的服务发现。

## 使用场景与最佳实践

- 大多数内部服务使用 ClusterIP（默认类型）。
- 需要稳定 DNS 解析到单个 Pod 时使用 Headless Service。
- 监控 ClusterIP 分配池的使用率。

## 架构深度解析

### ClusterIP 工作原理

```
┌─────────────────────────────────────────────────────────┐
│                    客户端 Pod                            │
│  DNS: my-svc.ns.svc.cluster.local → 10.96.0.50（VIP）   │
│          │                                              │
│          ▼                                              │
│  ┌──────────────────────────────────────────────────┐   │
│  │  节点内核（kube-proxy 生成的规则）                 │   │
│  │  ┌────────────────────────────────────────────┐  │   │
│  │  │ iptables 模式：DNAT 规则链                 │  │   │
│  │  │ - KUBE-SERVICES：匹配 VIP:port             │  │   │
│  │  │ - KUBE-SVC-XXX：随机选 Endpoint（RR）       │  │   │
│  │  │ - KUBE-SEP-XXX：DNAT 到 Pod IP             │  │   │
│  │  ├────────────────────────────────────────────┤  │   │
│  │  │ IPVS 模式：虚拟服务表                       │  │   │
│  │  │ - VirtualServer 10.96.0.50:80              │  │   │
│  │  │ - RealServer 10.0.1.5:8080 (rr/加权)       │  │   │
│  │  └────────────────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────┘   │
│          │                                              │
│          ▼                                              │
│  后端 Pod 10.0.1.5 / 10.0.2.7（由 EndpointSlice 驱动）   │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| Service 分配 | `pkg/registry/core/service/allocator` | ClusterIP 从 CIDR 池分配（bitmap） |
| kube-proxy | `pkg/proxy/iptables/` / `pkg/proxy/ipvs/` | 规则生成与同步 |
| 虚拟 IP 管理 | `pkg/controller/serviceipallocator/` | 地址分配控制器 |

### 流量转发流程（iptables 模式）

1. 客户端访问 `10.96.0.50:80`，报文进入本机 PREROUTING
2. `KUBE-SERVICES` 链匹配目标 VIP，跳转 `KUBE-SVC-XXX`
3. `KUBE-SVC-XXX` 链按规则（statistic random / 权重）选择 Endpoint
4. 命中 `KUBE-SEP-XXX` 链执行 DNAT：目标改为 Pod IP
5. 报文按 Pod 网络转发；回程报文由 conntrack 记录还原源地址

## 生产案例

### 案例 1：ClusterIP 地址池耗尽导致 Service 创建失败

| 时间 | 事件 |
|------|------|
| 10:00 | 新 Service 创建报 `Failed to allocate cluster IP` |
| 10:10 | `kubectl get svc -A | wc -l` 统计接近池容量 |
| 10:20 | 确认 `--service-cluster-ip-range` 为 /16（65534 个）已用 98% |
| 10:40 | 清理无用 Service 并规划扩容方案 |

**根因**：ClusterIP 分配基于服务网段 bitmap；服务数量逼近池容量（或大量 Service 未回收）导致分配失败。

**修复命令**：
```bash
# 统计 Service 数量与分配情况 🟢 只读
kubectl get svc -A --no-headers | wc -l
kubectl get svc -A -o json | jq '[.items[].spec.clusterIP] | length'
# 清理无 owner 的遗留 Service 🟡 中风险
kubectl get svc -A | grep -v kube-system | awk '{print $1, $2}' | xargs -n2 kubectl delete svc -n $1 $2
# 扩容服务网段（需 kube-apiserver 参数 + 集群重建或双网段支持）
```

### 案例 2：iptables 规则膨胀导致节点网络延迟飙升

**现象**：Service 数量超过 5000 后，节点网络延迟与 CPU 明显上升。

**诊断**：iptables 链线性遍历 + 每次 Service 变更全量更新规则（`iptables-restore` 全量重载）；规则数达到万级后性能急剧下降。

**修复**：切换 kube-proxy 为 IPVS 模式（哈希查找 O(1)）；或升级使用 eBPF 数据面（Cilium/kube-proxy 替代）。

## 对比评测

| 维度 | iptables 模式 | IPVS 模式 | eBPF 模式 |
|------|--------------|-----------|-----------|
| 查找复杂度 | O(n) 线性 | O(1) 哈希 | O(1) 哈希 |
| 更新方式 | 全量 reload | 增量 | 增量 |
| 高级算法 | 仅 RR | RR/WRR/LC 等 | RR/一致性哈希 |
| 内核要求 | 无 | 需 IPVS 模块 | 需 eBPF（5.10+） |
| 适用规模 | <3000 Service | >3000 Service | 大规模高性能 |

**选型建议**：小集群默认 iptables；大集群切 IPVS；追求性能与可观测性用 eBPF（Cilium）。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| VIP 不通 | `iptables -t nat -L KUBE-SERVICES -n` | 规则未生成或 kube-proxy 异常 |
| 分配失败 | `kubectl describe svc` 看事件 | 地址池耗尽 |
| 延迟高 | `iptables -t nat -L -n | wc -l` | 规则膨胀 |
| 负载不均 | `ipvsadm -ln` 查 RealServer | 权重或会话保持配置 |

## 生产部署清单

- [ ] 服务网段容量规划（Service 数 × 1.5 余量）
- [ ] Service 数量监控与清理机制（无主 Service 定期回收）
- [ ] 大规模集群评估 IPVS/eBPF 数据面
- [ ] kube-proxy 健康与规则同步延迟监控
- [ ] 变更窗口内执行 Service 批量操作（避免规则风暴）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | VIP 流量中断或分配失败 | 立即检查 kube-proxy 与地址池状态 |
| P1 | Service 规模逼近性能拐点 | 规划 IPVS/eBPF 数据面迁移 |
| P2 | 服务网段扩容 | 评估双网段支持或集群重建窗口 |

## 面试要点

> 以下 Q&A 覆盖 ClusterIP 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：ClusterIP 是如何分配与回收的？**
   A：kube-apiserver 启动时以 `--service-cluster-ip-range` 定义网段，内部用 bitmap 分配器（`RangeAllocator`）管理：创建 Service 时从可用位取一个 IP，删除时回收；支持 `clusterIP: ""` 自动分配或显式指定。耗尽时 Service 创建报错并产生事件。

2. **Q：iptables 模式与 IPVS 模式的本质区别？**
   A：iptables 是链式匹配（逐条比较，O(n)）且规则更新为全量 `iptables-restore`；IPVS 是内核级虚拟服务表（哈希查找 O(1)），支持 WRR/LC 等调度算法与增量更新。Service 数千级后 iptables 更新延迟与 CPU 开销显著，IPVS 是大集群标准选择。

3. **Q：客户端访问 ClusterIP 时回程流量如何处理？**
   A：入向流量经 DNAT 改写目标地址后转发到 Pod，conntrack 记录映射（VIP→Pod）；Pod 响应时 conntrack 自动还原源地址为 VIP，客户端无感知。这也是为什么 kube-proxy 依赖 conntrack——节点重启或 conntrack 表溢出会导致连接异常。

## 参考链接

- [ClusterIP - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/service/#type-clusterip)

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|Nodeport]]
- [[17-系统基础/06-知识字典/networking/loadbalancer.md|Loadbalancer]]
- [[17-系统基础/06-知识字典/networking/headless-service.md|Headless Service]]


<!-- risk-assessed -->
