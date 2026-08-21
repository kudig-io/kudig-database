---
title: Headless Service 无头服务
description: Headless Service 是 clusterIP 设为 None 的特殊 Service，不分配虚拟 IP，而是通过 DNS 直接返回后端
  Pod 的 ...
summary: Headless Service 是 clusterIP 设为 None 的特殊 Service，不分配虚拟 IP，而是通过 DNS 直接返回后端
  Pod 的 ...
category: dictionary
tags:
- k8s
- glossary
- networking
- service
- dns
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Headless Service 无头服务 是什么
- Headless Service 详解
trigger_keywords:
- Headless Service 无头服务
- Headless Service
- dictionary
prerequisites:
- kubernetes
---



# Headless Service 无头服务（Headless Service）

## 概述

Headless Service 是 clusterIP 设为 None 的特殊 Service，不分配虚拟 IP，而是通过 DNS 直接返回后端 Pod 的 IP 地址列表，适用于需要客户端直接连接 Pod 的场景。

## 核心概念/原理

- **clusterIP: None**：不分配 ClusterIP
- **DNS 记录**：为每个 Pod 创建 A/AAAA 记录
- **直接连接**：客户端通过 DNS 获取 Pod IP 直连
- **有状态应用**：StatefulSet 的标准网络方案

## 关键机制或特性

- `clusterIP: None` 定义 Headless Service
- DNS 格式：`pod-name.svc-name.namespace.svc.cluster.local`
- 有 selector 时返回匹配 Pod 的 IP 列表
- 无 selector 时配合 EndpointSlice 手动管理
- StatefulSet 必须使用 Headless Service
- 与 Service Mesh 的集成（Istio 自动处理）
- DNS SRV 记录支持端口发现

## 使用场景与最佳实践

- StatefulSet（数据库集群）的网络标识
- 服务发现的客户端直连模式
- 需要知道具体后端地址的场景
- gRPC 客户端的 DNS 负载均衡
- 最佳实践：配合 StatefulSet 使用、DNS TTL 调优

## 架构深度解析

### 工作机制

```
┌─────────────────────────────────────────────────────────┐
│                    客户端 Pod                            │
│  DNS 查询: my-svc.ns.svc.cluster.local                  │
│          │                                              │
│          ▼                                              │
│  ┌──────────────────────────────────────────────────┐   │
│  │        CoreDNS（headless 服务返回 A 记录集）        │   │
│  │  my-svc.ns.svc.cluster.local. 300 IN A 10.0.1.5  │   │
│  │  my-svc.ns.svc.cluster.local. 300 IN A 10.0.2.7  │   │
│  │  my-svc.ns.svc.cluster.local. 300 IN A 10.0.3.9  │   │
│  │  (每个 Ready 的 Endpoint 一条 A 记录)             │   │
│  └──────────────────────────────────────────────────┘   │
│          │                                              │
│          ▼ 客户端自行选择（随机/RR/自定义策略）            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
│  │ Pod 10.0.1.5 │  │ Pod 10.0.2.7 │  │ Pod 10.0.3.9 │   │
│  │ (直接访问，   │  │ (直接访问，   │  │ (直接访问，   │   │
│  │  无 VIP/无    │  │  无 VIP/无    │  │  无 VIP/无    │   │
│  │  kube-proxy) │  │  kube-proxy) │  │  kube-proxy) │   │
│  └──────────────┘  └──────────────┘  └──────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| Service 控制器 | `pkg/controller/endpointslice` | 为 headless Service 生成 EndpointSlice |
| Endpoint 写入 | `pkg/registry/core/service` | headless 无 ClusterIP，直接写 Endpoints |
| DNS 生成 | `cluster/addons/dns/coredns` | CoreDNS 基于 Endpoints 生成 A/SRV 记录 |

### 与普通 Service 的差异

| 维度 | ClusterIP Service | Headless Service |
|------|-------------------|------------------|
| ClusterIP | 分配 VIP | 无（`clusterIP: None`） |
| 负载均衡 | kube-proxy（iptables/IPVS） | 客户端自行处理（DNS 轮询） |
| DNS 记录 | 1 条 A 记录指向 VIP | 每条 Endpoint 一条 A 记录 |
| SRV 记录 | 不支持 | 支持（端口发现） |
| 适用场景 | 通用服务 | 状态化工作负载、客户端直连 |

## 生产案例

### 案例 1：Headless 服务 DNS 记录风暴压垮 CoreDNS

| 时间 | 事件 |
|------|------|
| 10:00 | 发布新的 StatefulSet 副本数为 500，客户端全量使用 headless 服务 |
| 10:05 | CoreDNS 负载飙升，查询延迟从 1ms 涨至 5s |
| 10:15 | 定位为每个客户端频繁查询 headless A 记录，DNS 缓存命中率低 |
| 10:30 | 优化：客户端启用连接复用 + DNS 缓存 + 使用 SRV 批量查询 |

**根因**：headless 服务每条 Endpoint 生成独立 A 记录，客户端按 DNS 轮询每次建连都查一次完整记录集；记录数 × 查询频率叠加导致 CoreDNS 过载。

**修复命令**：
```bash
# 查看 CoreDNS 指标 🟢 只读
kubectl -n kube-system get svc coredns -o wide
# 调整 CoreDNS 缓存 TTL（ConfigMap）🟡 中风险
kubectl -n kube-system edit cm coredns
# cache 30 改为 cache 60 并增加 cache 预取
# 客户端侧优化：Java 启用 DNS 缓存（JVM 参数）
# -Dnetworkaddress.cache.ttl=60 -Dnetworkaddress.cache.negative.ttl=10
```

### 案例 2：StatefulSet Pod 重建后 DNS 记录滞后

**现象**：数据库主节点切换（Pod 删除重建），客户端连接仍指向旧 IP，连接失败。

**诊断**：Pod 重建后 IP 变化，DNS 记录更新依赖 EndpointSlice 同步 + CoreDNS 缓存刷新；期间旧记录仍被解析。

**修复**：为 headless 服务配置 `publishNotReadyAddresses: true` 配合 StatefulSet 的 stable network identity（用 `pod-name.svc.ns` 直连而非 VIP 语义）；客户端实现重试与主动刷新 DNS 缓存。

## 对比评测

| 场景 | Headless + DNS | ClusterIP Service | Service Mesh（Istio） |
|------|---------------|-------------------|----------------------|
| 负载均衡 | 客户端 DNS 轮询 | 内核转发 | Envoy L7 均衡 |
| 故障转移 | 客户端感知 | 内核剔除 | 代理剔除 |
| 状态感知 | 需自定义 | 无 | 无 |
| 性能 | 直连最快 | 中 | 最低（代理路径） |
| 复杂度 | 低 | 低 | 高 |

**选型建议**：数据库/消息队列等需要客户端感知实例的选 headless；无状态服务选 ClusterIP；需要灰度/熔断等治理选 Mesh。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| A 记录缺失 | `kubectl get endpointslices` | Pod 未 Ready 或未发布 |
| DNS 轮询失效 | `dig my-svc.ns.svc @<coredns>` | 客户端缓存 TTL 过长 |
| 连接失败率高 | 对比记录数与 Pod 数 | EndpointSlice 同步延迟 |
| 无法使用 SRV | `dig SRV my-svc.ns.svc` | 未开启 publishNotReadyAddresses |

## 生产部署清单

- [ ] StatefulSet + headless 组合时使用 stable network identity 直连
- [ ] 客户端配置 DNS 缓存 TTL 与重试策略
- [ ] CoreDNS 容量规划（记录数 × QPS 评估）
- [ ] 关键服务禁止用 headless 做无状态负载均衡
- [ ] 配置 EndpointSlice 指标监控同步延迟

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 客户端大面积连接失败（记录滞后） | 立即刷新 DNS 缓存并检查 EndpointSlice 同步 |
| P1 | 服务规模增长导致 DNS 压力 | 迁移至 Service Mesh 或客户端缓存优化 |
| P2 | 从 ClusterIP 迁移 headless | 客户端改造直连逻辑，灰度验证 |

## 面试要点

> 以下 Q&A 覆盖 headless Service 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：headless Service 与普通 Service 在 DNS 和负载均衡上有什么区别？**
   A：普通 Service 有 ClusterIP，DNS 只返回 1 条 A 记录（指向 VIP），负载均衡由 kube-proxy 完成；headless（`clusterIP: None`）没有 VIP，DNS 为每个 Ready 的 Endpoint 生成一条 A 记录，客户端拿到全部后端 IP 自行选择，负载均衡责任从内核转移到客户端。

2. **Q：为什么 StatefulSet 通常配合 headless Service 使用？**
   A：StatefulSet 要求 Pod 有稳定的网络身份（`pod-0.svc.ns.svc.cluster.local`），headless 服务会为每个 Pod 生成稳定的 DNS 名称；同时数据库类有状态服务需要客户端直连特定实例（主/从感知），headless 的"全部记录返回"语义正好满足。

3. **Q：headless 服务的 publishNotReadyAddresses 参数有什么作用？**
   A：默认情况下 DNS 只发布 Ready 状态的 Pod；设置为 true 后，未 Ready 的 Pod 也会被发布（A/SRV 记录），适用于需要"启动过程中即可被发现"的场景（如集群引导、滚动升级），代价是客户端可能连到未就绪实例，需配合客户端重试。

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/service/#headless-services
- https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/dns.md|DNS]]
- [[17-系统基础/06-知识字典/workloads/statefulset.md|StatefulSet]]
