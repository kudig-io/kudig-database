---
title: Envoy
description: Envoy 是高性能的 L7 代理和通信总线，最初由 Lyft 开发，现为 CNCF 毕业项目。它是 Istio、Contour、Gloo
  等云原生项目的数据平...
summary: Envoy 是高性能的 L7 代理和通信总线，最初由 Lyft 开发，现为 CNCF 毕业项目。它是 Istio、Contour、Gloo 等云原生项目的数据平...
category: dictionary
tags:
- k8s
- glossary
- envoy
- service-mesh
- proxy
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Envoy 是什么
- Envoy Proxy 详解
trigger_keywords:
- Envoy
- Envoy Proxy
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Envoy

> **英文名**: Envoy Proxy

## 概述

Envoy 是高性能的 L7 代理和通信总线，最初由 Lyft 开发，现为 CNCF 毕业项目。它是 Istio、Contour、Gloo 等云原生项目的数据平面基础，广泛用于服务网格、API 网关和入口控制器场景。

## 核心概念/原理

### 核心概念

- **Listener**：监听入站连接的端口/地址。
- **Filter Chain**：处理连接的过滤器链（认证、限流、路由等）。
- **Cluster**：上游服务集群（后端端点集合）。
- **Route**：路由规则，将请求映射到 Cluster。

### xDS API

Envoy 通过 xDS（发现服务 API）动态获取配置：

| xDS | 用途 |
|-----|------|
| LDS | Listener 发现 |
| RDS | Route 发现 |
| CDS | Cluster 发现 |
| EDS | Endpoint 发现 |
| SDS | Secret 发现 |

## 关键机制或特性

- **Sidecar 模式**：作为 Pod 的 sidecar 容器运行（Istio 默认）。
- **Gateway 模式**：作为入口/出口网关运行。
- 支持 HTTP/1.1、HTTP/2、gRPC、TCP、UDP 协议。
- 内置熔断、重试、超时、限流等弹性功能。
- 支持 Wasm 扩展自定义过滤器。

## 使用场景与最佳实践

- 使用 Envoy 作为 API Gateway 的数据平面（Gateway API 支持）。
- 配合 Istio 构建服务网格实现 mTLS 和流量管理。
- 使用 Envoy Admin API（`/config_dump`、`/stats`）排查问题。
- 监控 Envoy 的 upstream_rq_time 和 upstream_cx_connect_fail 指标。
- 合理配置 Circuit Breaker 防止级联故障。

## 参考链接

- [Envoy Proxy Official](https://www.envoyproxy.io/)

## 架构深度解析

### 核心架构

```
┌─────────────────────────────────────────────────────┐
│              Envoy Proxy (C++)                      │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Listener    │  │ Filter Chain │  │ Cluster   │  │
│  │ (L4 入口)   │  │ (L4/L7)      │  │ (Upstream)│  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │         xDS API (Dynamic Config)            │  │
│  │  LDS / RDS / CDS / EDS / SDS               │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（envoyproxy/envoy）

| 模块 | 路径 | 职责 |
|------|------|------|
| Listener | `source/common/listener_manager/` | 监听器管理与连接处理 |
| HTTP Filter | `source/extensions/filters/http/` | L7 过滤器（路由/限流/熔断） |
| Network Filter | `source/extensions/filters/network/` | L4 过滤器（TCP proxy） |
| Cluster | `source/common/upstream/` | 上游集群管理与负载均衡 |
| xDS | `source/common/config/` | 动态配置订阅与更新 |
| Admin | `source/server/admin/` | 管理接口（/stats, /config_dump） |

### 请求处理流程

1. 客户端连接 → Listener 接受（TCP/UDP）
2. Listener Filter 链（TLS Inspector、Proxy Protocol）
3. Network Filter 链（TCP Proxy / HTTP Connection Manager）
4. HTTP Filter 链（Router → RateLimit → CORS → ...）
5. Router 选择 Cluster → 负载均衡选择 Upstream
6. 连接池复用 / 新建连接 → 转发请求
7. 响应回传 + 统计指标更新

## 生产案例

### 案例 1：连接池耗尽导致 503 错误

| 时间 | 事件 |
|------|------|
| 18:00 | 服务间调用大量 503 UC (Upstream Connection failure) |
| 18:05 | 检查 Envoy stats：upstream_cx_overflow 计数激增 |
| 18:15 | 根因：上游服务扩容，但 Envoy 连接池限制未调整 |
| 18:30 | 修复：增加 `max_connections` 和 `max_pending_requests` |

**修复命令**：
```bash
# 查看连接池统计 🟢 只读
curl -s localhost:9901/stats | grep upstream_cx
# 查看当前配置 🟢 只读
curl -s localhost:9901/config_dump | jq '.configs[] | select(.dynamic_active_clusters)'
# 调整 Circuit Breaker 🟡 中风险（通过 xDS 或配置文件）
```

### 案例 2：xDS 配置推送失败导致路由异常

**现象**：更新路由规则后，部分 Envoy 实例仍使用旧配置。

**诊断**：xDS 版本冲突（version_info 不匹配），部分实例拒绝新配置。

**修复**：检查控制平面 xDS 服务器状态，强制触发全量推送，或重启异常 Envoy 实例。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 所有 Listener 不可用 | 回滚 Envoy 版本，检查配置语法 |
| P1 | 单 Cluster 连接失败率高 | 检查上游服务，调整 Circuit Breaker |
| P2 | xDS 推送延迟 | 检查控制平面负载，增加 Envoy 副本 |

## 面试要点

1. **Q：Envoy 的 xDS API 是如何工作的？**
   A：xDS 是 Envoy 的动态配置协议：① LDS（Listener）定义监听端口和 Filter 链；② RDS（Route）定义 HTTP 路由规则；③ CDS（Cluster）定义上游服务集群；④ EDS（Endpoint）定义具体实例地址；⑤ SDS（Secret）管理 TLS 证书。Envoy 通过 gRPC 流式订阅，控制平面推送增量更新，实现零停机配置变更。

2. **Q：Envoy 的 Filter Chain 架构设计思想？**
   A：Envoy 采用分层 Filter 架构：① Listener Filter（连接级，如 TLS Inspector）；② Network Filter（L4，如 TCP Proxy）；③ HTTP Filter（L7，如 Router/RateLimit）。每个 Filter 实现统一接口，可组合编排。这种设计使得功能扩展无需修改核心代码，类似 Linux 内核的 Netfilter 架构。

3. **Q：Envoy 与 Nginx 在架构上有何本质区别？**
   A：Envoy 基于 C++11 多线程架构，每个 worker 线程独立处理连接（无锁）；Nginx 基于多进程 + epoll，master-worker 模型。Envoy 原生支持 xDS 动态配置，无需 reload；Nginx 配置变更需要 reload（短暂中断）。Envoy 适合 Service Mesh 数据面（动态配置需求高）；Nginx 适合静态反向代理（配置稳定）。

## Related

- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]
- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium]]
- [[17-系统基础/06-知识字典/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
