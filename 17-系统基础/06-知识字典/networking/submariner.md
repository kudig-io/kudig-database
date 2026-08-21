---
title: Submariner 多集群网络
description: Submariner 是 Red Hat 主导的 CNCF Sandbox 项目，专注于解决 Kubernetes 多集群间的网络互联问题，实现跨集群
  Serv...
summary: Submariner 是 Red Hat 主导的 CNCF Sandbox 项目，专注于解决 Kubernetes 多集群间的网络互联问题，实现跨集群
  Serv...
category: dictionary
tags:
- k8s
- glossary
- networking
- multi-cluster
- cni
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Submariner 多集群网络 是什么
- Submariner 详解
trigger_keywords:
- Submariner 多集群网络
- Submariner
- dictionary
prerequisites:
- kubernetes
---



# Submariner 多集群网络（Submariner）

## 概述

Submariner 是 Red Hat 主导的 CNCF Sandbox 项目，专注于解决 Kubernetes 多集群间的网络互联问题，实现跨集群 Service 发现和 Pod 直通，无需依赖外部网络方案。

## 核心概念/原理

- **跨集群网络**：在不同 K8s 集群间建立安全的 IPsec/WireGuard 隧道
- **Service 发现**：基于 MCS（Multi-Cluster Services）API 实现跨集群服务发现
- **CNI 无关**：兼容 Flannel、Calico、Cilium、OVN 等各种 CNI
- **Gateway 模型**：每个集群通过 Gateway 节点建立隧道连接

## 关键机制或特性

- 支持 IPsec 和 WireGuard 两种隧道协议
- Globalnet 解决集群 CIDR 重叠问题
- 与 K8s MCS API 标准对齐
- Submariner Operator 简化部署
- 内置连接状态监控和健康检查
- 支持 Headless Service 和 StatefulSet 跨集群访问

## 使用场景与最佳实践

- 多集群应用的服务间通信
- 集群迁移期间的流量平滑切换
- 混合云/多云环境的网络打通
- 开发/测试环境的跨集群联调

## 参考链接

- https://submariner.io/
- https://github.com/submariner-io/submariner

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│           Submariner (per cluster)                  │
├─────────────────────────────────────────────────────┤
│  ┌───────────┐  ┌────────────┐  ┌─────────────┐  │
│  │ Gateway   │  │ Route Agent│  │ Lighthouse  │  │
│  │ (IPsec/   │  │ (DaemonSet)│  │ (DNS 跨集群)│  │
│  │  VXLAN)   │  │            │  │             │  │
│  └─────┬─────┘  └──────┬─────┘  └──────┬──────┘  │
│        │               │               │         │
│  ┌─────▼───────────────▼───────────────▼─────┐  │
│  │     Cross-cluster Network Plane          │  │
│  │  (IPsec Tunnel / VXLAN / Globalnet)     │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（submariner-io/submariner）

| 模块 | 路径 | 职责 |
|------|------|------|
| Gateway | `pkg/cableengine/` | IPsec/VXLAN 隧道管理 |
| Route Agent | `pkg/routeagent/` | 节点路由规则管理 |
| Lighthouse | `pkg/lighthouse/` | 跨集群 DNS 解析 |
| Globalnet | `pkg/globalnet/` | 重叠 CIDR 的 SNAT/DNAT |
| Broker | `pkg/broker/` | 集群间元数据交换 |

### 跨集群连接流程

1. 集群 A/B 通过 Broker 交换 Gateway 端点信息
2. Gateway 建立 IPsec 隧道（IKEv2 + ESP）
3. Route Agent 在各节点添加跨集群路由规则
4. Pod 访问跨集群 Service → 路由到 Gateway → 隧道转发
5. Lighthouse 提供跨集群 DNS 解析（`svc.clusterB.svc.clusterset.local`）

## 生产案例

### 案例 1：IPsec 隧道 MTU 不匹配导致大包丢失

| 时间 | 事件 |
|------|------|
| 11:00 | 跨集群服务访问正常，但大文件传输失败 |
| 11:15 | 确认：TCP 小包正常，> 1400 字节包丢失 |
| 11:30 | 根因：IPsec 封装开销未计入 MTU，物理网卡 1500 - 封装 = 1438 |
| 11:45 | 修复：设置 Pod MTU 为 1400，启用 PMTU Discovery |

**修复命令**：
```bash
# 检查隧道 MTU 🟢 只读
ip link show vx-submariner | grep mtu
# 测试跨集群连通性 🟢 只读
subctl verify --only connectivity
# 调整 CNI MTU 🟡 中风险
kubectl patch cm cni-config -n kube-system -p '{"data":{"mtu":"1400"}}'
```

### 案例 2：Globalnet SNAT 端口耗尽

**现象**：重叠 CIDR 场景下，跨集群连接间歇性失败。

**诊断**：Globalnet 使用 SNAT 解决 IP 冲突，但单 IP 端口数有限（~64K），高并发场景耗尽。

**修复**：增加 Globalnet IP 池大小，或重新规划集群 Pod CIDR 避免重叠。

## 对比评测

| 维度 | Submariner | Cilium ClusterMesh | KubeFed + 手动网络 |
|------|-----------|-------------------|-------------------|
| 网络连通 | 隧道（IPsec/WireGuard） | eBPF 直连/隧道 | 需自建 |
| 服务发现 | Lighthouse（DNS） | ClusterIP 全局 | 手动 |
| 故障转移 | 手动/外部 GSLB | 手动 | 手动 |
| 多集群规模 | 中（网关节点） | 大（扁平） | 任意 |
| 部署复杂度 | 中 | 高（统一 CNI） | 低 |

**选型建议**：异构 CNI 多集群互联选 Submariner；统一 Cilium 环境优先 ClusterMesh；仅需服务发现可叠加。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 跨集群不通 | `subctl show connections` | 网关节点 IPsec 隧道 Down |
| 服务发现失败 | `kubectl get endpoints -n lighthouse` | Lighthouse 同步异常、DNS 配置缺失 |
| 隧道抖动 | 检查 IPsec/WireGuard 日志 | 防火墙端口未放行、MTU 问题 |
| 网关漂移 | `subctl show gateways` | 网关节点故障、健康检查剔除 |

## 生产部署清单

- [ ] 网关节点选择（≥2 高可用）且网络可达（UDP 4500/500 或 51820）
- [ ] IPsec/WireGuard 密钥管理与轮换流程已建立
- [ ] Lighthouse 的 DNS 配置（CoreDNS 插件）已注入各集群
- [ ] MTU 调整（隧道封装开销）并验证跨集群大包
- [ ] 跨集群故障演练（断网关 → 恢复验证）

## 常见误区与设计要点

- **误区 1**：所有集群都要全互联——按业务需求建 Broker 拓扑，避免网状爆炸。
- **误区 2**：忽略 MTU——隧道封装后不调整 MTU 会导致跨集群大包失败。
- **设计要点**：网关节点独立打标签并排空业务负载；用 Globalnet 解决集群间网段重叠；监控隧道健康（`subctl diagnose`）。

## 性能参考

- 隧道吞吐：IPsec 约 60-80% 物理带宽（受加密开销），WireGuard 略高。
- 延迟增加：跨集群一跳约 0.5-1ms（同地域），跨地域受物理距离主导。
- 网关节点规格建议：8C16G 起，按跨集群流量规划。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 所有跨集群隧道断开 | 检查 Gateway 节点状态，重建隧道 |
| P1 | 单集群 DNS 解析失败 | 重启 Lighthouse Agent，检查 ServiceExport |
| P2 | 隧道延迟增加 > 50ms | 检查网络质量，考虑切换 Gateway 节点 |

## 面试要点

1. **Q：Submariner 与 Cilium ClusterMesh 有何区别？**
   A：Submariner 是独立的多集群网络方案，通过 IPsec/VXLAN 隧道连接集群，不依赖特定 CNI；Cilium ClusterMesh 是 Cilium 内置功能，基于 eBPF 实现跨集群负载均衡和策略。Submariner 优势在于 CNI 无关性和 Globalnet（重叠 CIDR 支持）；Cilium 优势在于无隧道开销和统一策略。

2. **Q：Submariner 的 Globalnet 如何解决 IP 地址冲突？**
   A：Globalnet 为每个集群分配唯一的 GlobalCIDR，跨集群流量经过 SNAT 转换为 GlobalIP。实现：① 集群加入时分配 GlobalCIDR；② 出口流量经 Globalnet 组件 SNAT；③ 返回流量 DNAT 还原。代价是丢失源 IP，需配合 Proxy Protocol 保留。

3. **Q：如何诊断 Submariner 跨集群连接问题？**
   A：使用 `subctl` 工具链：① `subctl show all` 查看集群状态；② `subctl verify --only connectivity` 运行连通性测试；③ `subctl diagnose all` 全面诊断；④ 检查 Gateway 日志和 Route Agent 路由表。

## Related

- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium Cluster Mesh]]
- [[17-系统基础/06-知识字典/networking/linkerd.md|Linkerd]]
- [[17-系统基础/06-知识字典/networking/consul.md|Consul]]
