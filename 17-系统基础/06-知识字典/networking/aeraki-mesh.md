---
title: Aeraki Mesh 七层网格
description: Aeraki Mesh 是腾讯开源的服务网格方案，专注于解决 Istio 只支持 HTTP/gRPC 协议的局限性，通过 Aeraki 协议框架将服务网格能力扩...
summary: Aeraki Mesh 是腾讯开源的服务网格方案，专注于解决 Istio 只支持 HTTP/gRPC 协议的局限性，通过 Aeraki 协议框架将服务网格能力扩...
category: dictionary
tags:
- k8s
- glossary
- networking
- service-mesh
- l7
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Aeraki Mesh 七层网格 是什么
- Aeraki Mesh 详解
trigger_keywords:
- Aeraki Mesh 七层网格
- Aeraki Mesh
- dictionary
prerequisites:
- kubernetes
---



# Aeraki Mesh 七层网格（Aeraki Mesh）

## 概述

Aeraki Mesh 是腾讯开源的服务网格方案，专注于解决 Istio 只支持 HTTP/gRPC 协议的局限性，通过 Aeraki 协议框架将服务网格能力扩展到 TCP 和任意七层协议（Dubbo、Thrift、Redis 等）。

## 核心概念/原理

- **协议扩展**：将 Istio 的流量管理扩展到任意 L7 协议
- **Dubbo 支持**：完整支持 Apache Dubbo 协议的流量治理
- **Redis 支持**：Redis 协议的流量镜像、故障注入等
- **腾讯开源**：基于腾讯大规模微服务实践

## 关键机制或特性

- Aeraki Protocol Framework 协议扩展框架
- 支持 Dubbo、Thrift、Redis、MySQL 等非 HTTP 协议
- Aeraki Mesh CRD 定义七层路由规则
- 与 Istio 控制面无缝集成
- MetaProtocol 元协议框架（协议无关的流量治理）
- LazyXDS 按需加载优化大规模集群性能

## 使用场景与最佳实践

- 使用 Dubbo/Thrift 等传统 RPC 框架的微服务网格化
- 需要非 HTTP 协议流量治理的场景
- Istio 生态的协议扩展
- 传统微服务向服务网格迁移
- 多协议混合环境的统一管理

## 参考链接

- https://www.aeraki.net/
- https://github.com/aeraki-mesh/aeraki

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              Istio Control Plane (istiod)            │
├─────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────┐  │
│  │         Aeraki Controller                    │  │
│  │  ┌───────────┐  ┌─────────────────────┐  │  │
│  │  │ Protocol  │  │ MetaProtocol        │  │  │
│  │  │ Detector  │  │ Codec Factory       │  │  │
│  │  └─────┬─────┘  └──────────┬───────────┘  │  │
│  │        │                    │              │  │
│  │  ┌─────▼────────────────────▼───────────┐  │  │
│  │  │   Envoy Filter Generator (xDS)       │  │  │
│  │  └──────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────┤
│  Data Plane: Envoy Sidecar + MetaProtocol Filter  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（aeraki-mesh/aeraki）

| 模块 | 路径 | 职责 |
|------|------|------|
| 主控制器 | `pkg/controller/` | 监听 ServiceEntry/DR，触发 Reconcile |
| 协议插件 | `pkg/envoyfilter/` | Dubbo/Thrift/Redis 协议转 EnvoyFilter |
| MetaProtocol | `pkg/bootstrap/` | 元协议框架初始化 |
| LazyXDS | `pkg/lazyxds/` | 按需加载 xDS 配置 |
| CRD | `api/` | MetaRouter/MetaDestinationRule 定义 |

### 协议扩展机制

1. Aeraki 监听 ServiceEntry 的 protocol 字段
2. 根据协议类型选择对应的 Protocol Generator
3. 生成 EnvoyFilter CR（包含 MetaProtocol filter 配置）
4. Istio 将 EnvoyFilter 下发到 Sidecar
5. Envoy 使用 MetaProtocol codec 解析应用层协议

## 生产案例

### 案例 1：Dubbo 协议路由规则不生效

| 时间 | 事件 |
|------|------|
| 09:00 | 发布 Dubbo 服务新版本，配置 MetaRouter 路由规则 |
| 09:10 | 发现流量未按版本路由，全部进入 v1 |
| 09:20 | 检查 Aeraki 日志：ServiceEntry protocol 未设置为 `dubbo` |
| 09:30 | 修复：修改 ServiceEntry ports protocol 为 `dubbo`，重新触发 Reconcile |

**修复命令**：
```bash
# 检查 ServiceEntry 协议配置 🟢 只读
kubectl get serviceentry -A -o yaml | grep -A3 "protocol:"
# 查看 Aeraki 控制器日志 🟢 只读
kubectl logs -n istio-system deploy/aeraki --tail=50
# 修复协议声明 🟡 中风险
kubectl patch serviceentry dubbo-svc -p '{"spec":{"ports":[{"protocol":"dubbo"}]}}'
```

### 案例 2：LazyXDS 导致大规模集群 Sidecar 初始化慢

**现象**：5000+ Pod 集群中，新 Pod 启动时 Sidecar 初始化耗时 > 60s。

**诊断**：LazyXDS 按需加载在大规模场景下产生大量 xDS 请求，istiod 队列积压。

**修复**：调整 LazyXDS 批量加载策略，增加 istiod 副本数，启用 xDS 增量推送（Delta xDS）。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 所有非 HTTP 协议流量中断 | 回滚 Aeraki 版本，临时移除 Sidecar |
| P1 | 单协议路由规则不生效 | 检查 ServiceEntry 协议声明，重启 Aeraki |
| P2 | xDS 推送延迟 > 10s | 扩容 istiod，调整 LazyXDS 参数 |

## 面试要点

1. **Q：Aeraki Mesh 如何扩展 Istio 支持非 HTTP 协议？**
   A：Aeraki 通过 Protocol Generator 插件架构实现：① 监听 ServiceEntry 的 protocol 字段；② 根据协议类型（dubbo/thrift/redis）选择对应 Generator；③ 生成包含 MetaProtocol filter 的 EnvoyFilter CR；④ Envoy 使用 MetaProtocol codec 解析应用层协议，实现路由/限流/故障注入等能力。

2. **Q：MetaProtocol 元协议框架的设计思想是什么？**
   A：MetaProtocol 是协议无关的流量治理框架：将协议解析（codec）与流量策略（route/filter）解耦。新增协议只需实现 Codec 接口（encode/decode），无需修改路由/限流等通用逻辑。这类似于 Envoy 的 Network Filter 架构，但针对 L7 协议做了抽象。

3. **Q：LazyXDS 解决了什么问题？如何实现？**
   A：大规模集群中，istiod 需要向所有 Sidecar 推送全量 xDS 配置，导致内存和带宽压力巨大。LazyXDS 实现按需加载：Sidecar 启动时只请求自身相关的 Service 配置，通过 on-demand xDS 协议动态获取。实现路径：`pkg/lazyxds/` 拦截 xDS 请求，根据 Pod 的 Service 关联关系过滤配置。

## Related

- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/networking/linkerd.md|Linkerd]]
