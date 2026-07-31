---
title: K8GB 全球负载均衡
description: K8GB（Kubernetes Global Balancer）是 CNCF Sandbox 项目，实现跨多个 Kubernetes 集群的全球流量负载均衡，基...
summary: K8GB（Kubernetes Global Balancer）是 CNCF Sandbox 项目，实现跨多个 Kubernetes 集群的全球流量负载均衡，基...
category: dictionary
tags:
- k8s
- glossary
- networking
- dns
- multi-cluster
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8GB 全球负载均衡 是什么
- K8GB 详解
trigger_keywords:
- K8GB 全球负载均衡
- K8GB
- dictionary
prerequisites:
- kubernetes
---



# K8GB 全球负载均衡（K8GB）

## 概述

K8GB（Kubernetes Global Balancer）是 CNCF Sandbox 项目，实现跨多个 Kubernetes 集群的全球流量负载均衡，基于 DNS 和 GSLB 策略将用户请求路由到最优集群。

## 核心概念/原理

- **DNS 级负载均衡**：通过 CoreDNS 插件或外部 DNS 提供商实现 GSLB
- **健康检查驱动**：基于端点健康状态自动摘除故障集群
- **多策略路由**：支持 Round Robin、地理位置、故障转移等策略
- **CNCF Sandbox**：轻量级的全球流量管理方案

## 关键机制或特性

- GslbIngress CRD 定义全局流量策略
- 集成 Infoblox、Route53、NS1 等 DNS 提供商
- 基于 Prometheus 的健康检查指标
- 支持加权 Round Robin 和 GeoIP 路由
- 零停机集群维护和故障转移
- 与 Flagger / Argo Rollouts 配合使用

## 使用场景与最佳实践

- 多区域/多集群的高可用部署
- 灾难恢复场景下的流量切换
- 基于地理位置的用户路由
- 灰度发布中的全球流量分配

## 参考链接

- https://www.k8gb.io/
- https://github.com/k8gb-io/k8gb

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              K8GB Controller (per cluster)           │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Gslb CRD    │  │ DNS Provider │  │ Health    │  │
│  │ Reconciler  │  │ (Infoblox/   │  │ Check     │  │
│  │             │  │  Route53)    │  │ Controller│  │
│  └──────┬──────┘  └──────┬───────┘  └─────┬─────┘  │
│         │                │                 │        │
│  ┌──────▼────────────────▼─────────────────▼────┐  │
│  │           CoreDNS + k8gb plugin             │  │
│  │  (Zone delegation: gslb.example.com)        │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（k8gb-io/k8gb）

| 模块 | 路径 | 职责 |
|------|------|------|
| Controller | `controllers/gslb_controller.go` | Gslb CRD Reconcile 主循环 |
| DNS Provider | `controllers/providers/` | Infoblox/Route53/NS1 抽象层 |
| Health Check | `controllers/depresolver.go` | 端点健康状态检测 |
| CoreDNS Plugin | `coredns/` | 自定义 DNS 解析插件 |
| Metrics | `controllers/metrics.go` | Prometheus 指标暴露 |

### GSLB 解析流程

1. 用户请求 `app.gslb.example.com` → 本地 DNS 递归
2. 权威 DNS 委派到最近集群的 CoreDNS
3. CoreDNS k8gb 插件查询 Gslb CR 状态
4. 根据策略（roundRobin/geoIP/failover）返回最优集群 IP
5. 健康检查失败时自动摘除故障集群

## 生产案例

### 案例 1：DNS TTL 过长导致故障转移延迟

| 时间 | 事件 |
|------|------|
| 14:00 | 集群 A 完全宕机 |
| 14:01 | K8GB 检测到健康检查失败，更新 DNS 记录 |
| 14:01-14:05 | 部分用户仍访问集群 A（DNS 缓存未过期） |
| 14:05 | 确认根因：TTL 设置为 300s，客户端缓存未刷新 |
| 14:10 | 修复：将 TTL 调整为 30s，启用 EDNS Client Subnet |

**修复命令**：
```bash
# 检查 Gslb CR 状态 🟢 只读
kubectl get gslb -A -o yaml | grep -A5 "status:"
# 查看 DNS 解析结果 🟢 只读
dig app.gslb.example.com @coredns-ip +short
# 调整 TTL 🟡 中风险
kubectl patch gslb app-gslb -p '{"spec":{"dnsName":"app.gslb.example.com","ttl":30}}'
```

### 案例 2：多集群 Zone 委派冲突

**现象**：两个集群同时声明为 `gslb.example.com` 的权威，导致解析结果不一致。

**诊断**：检查各集群 CoreDNS Corefile 中的 zone 配置，确认 NS 记录委派关系。

**修复**：确保每个地理区域只有一个集群持有 zone 主权，其他集群通过 NS 委派获取解析结果。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 所有集群 DNS 解析失败 | 切换到备用 DNS 提供商，手动更新记录 |
| P1 | 单集群健康检查误判 | 调整健康检查阈值，临时强制路由 |
| P2 | DNS 解析延迟增加 | 检查 CoreDNS 负载，增加副本 |

## 面试要点

1. **Q：K8GB 与云厂商 GSLB（如 AWS Route53 健康检查）有何区别？**
   A：K8GB 是 Kubernetes 原生的开源方案，通过 CRD 声明式管理，与 GitOps 工作流无缝集成；云厂商 GSLB 是托管服务，功能更丰富（延迟路由、地理围栏）但存在供应商锁定。K8GB 适合多云/混合云环境，云 GSLB 适合单一云深度集成。

2. **Q：K8GB 如何处理脑裂场景（两个集群都认为对方宕机）？**
   A：K8GB 采用“本地视角”策略：每个集群只负责自己区域的 DNS 解析，通过 NS 委派实现分布式决策。脑裂时各集群独立服务本区域用户，不会产生全局冲突。恢复后通过 Gslb CR 的 `spec.healthCheck` 自动重新收敛。

3. **Q：如何测试 K8GB 的故障转移是否正常工作？**
   A：① 使用 `kubectl scale deploy --replicas=0` 模拟集群故障；② 观察 CoreDNS 日志确认健康检查状态变更；③ 使用 `dig` 从不同地理位置验证解析结果；④ 检查 Prometheus 指标 `k8gb_endpoint_status` 确认端点摘除。

## Related

- [[17-系统基础/06-知识字典/networking/consul.md|Consul]]
- [[17-系统基础/06-知识字典/networking/linkerd.md|Linkerd]]
- [[17-系统基础/06-知识字典/operations/flagger.md|Flagger]]
