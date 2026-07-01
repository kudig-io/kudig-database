---
title: 01 - 云原生 API 网关架构总览
description: '# 01 - 云原生 API 网关架构总览'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- etcd
- prometheus
- istio
- opa
- postgresql
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 云原生 API 网关架构总览 是什么
- 如何 云原生 API 网关架构总览
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- 云原生
- API
- 网关架构总览
- cloud
- native
- api
- gateway
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- prometheus-basics
- etcd-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-05-23"
---

# 01 - 云原生 API 网关架构总览

> **文档版本**: v1.0 | **适用版本**: [[Kubernetes|Kubernetes]] 1.25+ | **更新日期**: 2026-03-04 | **关键词**: API Gateway, [[Ingress|Ingress]] Controller, Gateway API, 控制平面, 数据平面

## 目录

1. [什么是云原生 API 网关](#1-什么是云原生-api-网关)
2. [API 网关 vs Ingress 控制器 vs 服务网格网关](#2-api-网关-vs-ingress-控制器-vs-服务网格网关)
3. [请求生命周期](#3-请求生命周期)
4. [控制平面与数据平面分离](#4-控制平面与数据平面分离)
5. [主流产品 CNCF 生态定位](#5-主流产品-cncf-生态定位)
6. [架构演进：从 Ingress 到 Gateway API](#6-架构演进从-ingress-到-gateway-api)
7. [产品选型决策树](#7-产品选型决策树)

---

## 1. 什么是云原生 API 网关

云原生 API 网关是运行在 Kubernetes 集群边缘的流量入口组件，负责管理集群外部（南北向）流量的接入、路由、安全和可观测性。与传统 API 网关（如 Nginx + 手动配置）不同，云原生 API 网关具备以下核心特征：

- **声明式配置**: 通过 Kubernetes CRD 或 Gateway API 资源声明式管理路由规则
- **动态热更新**: 路由、插件配置实时生效，无需重启或 reload
- **可编程扩展**: 支持 Wasm、Lua 或多语言插件运行时，灵活扩展网关能力
- **Kubernetes 原生**: 深度集成 [[Service|Service]] Discovery、RBAC、ConfigMap/Secret 等 K8s 原语
- **可观测性内建**: 原生暴露 Prometheus 指标、结构化访问日志和分布式链路追踪

### 核心职责

```
┌─────────────────────────────────────────────────────────────┐
│                    云原生 API 网关核心职责                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  流量路由    │  │  安全防护    │  │  流量管控    │         │
│  │  - 路径匹配  │  │  - TLS 终止  │  │  - 限流熔断  │         │
│  │  - 域名分发  │  │  - JWT 验证  │  │  - 金丝雀发布 │         │
│  │  - Header 路由│  │  - mTLS     │  │  - 流量镜像  │         │
│  │  - 权重分流  │  │  - WAF      │  │  - 重试超时  │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  协议支持    │  │  可观测性    │  │  插件扩展    │         │
│  │  - HTTP/HTTPS│  │  - 指标     │  │  - Wasm     │         │
│  │  - gRPC     │  │  - 日志     │  │  - Lua      │         │
│  │  - WebSocket │  │  - 追踪     │  │  - 多语言   │         │
│  │  - TCP/UDP  │  │  - 健康检查 │  │  - 自定义   │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
└─────────────────────────────────────────────────────────────┘
```

## 2. API 网关 vs Ingress 控制器 vs 服务网格网关

### 概念对比

| 维度 | Ingress 控制器 | API 网关 | 服务网格网关 |
|------|---------------|---------|-------------|
| **流量方向** | 南北向（入口） | 南北向（入口） | 南北向 + 东西向 |
| **API 接口** | Ingress 资源 | Gateway API / 自定义 CRD | Gateway API (GAMMA) + mesh CRD |
| **核心能力** | 基础路由、TLS 终止 | 路由 + 安全 + 限流 + 插件 | 路由 + mTLS + 流量治理 |
| **插件扩展** | 注解驱动、有限 | Wasm/Lua/多语言、丰富 | Sidecar 过滤器链 |
| **配置模型** | 注解 + 基础 spec | 声明式 CRD / Admin API | 控制平面下发 |
| **典型产品** | Nginx Ingress | Higress, APISIX, Kong | Istio Gateway |
| **适用场景** | 简单 HTTP 路由 | 企业 API 管理平台 | 微服务内部通信 |
| **运维复杂度** | 低 | 中 | 高 |

### 架构定位

```
                    ┌─────────────────────────────────┐
  外部流量 ──────────│      API 网关 / Ingress 控制器    │──────────── K8s 集群
  (南北向)          │    (domain-03-networking-traffic 本领域覆盖范围)      │
                    └──────────────┬──────────────────┘
                                   │
                    ┌──────────────▼──────────────────┐
                    │        Service / Pod              │
                    │   ┌───────────────────────┐      │
  内部流量 ─────────│───│    服务网格 Sidecar     │      │
  (东西向)          │   │   (domain-03-networking-traffic 覆盖范围)  │      │
                    │   └───────────────────────┘      │
                    └─────────────────────────────────┘
```

### 功能边界

- **本领域（domain-40）覆盖**: 从集群外部到 Service 的南北向流量入口管理
- **domain-03-networking-traffic 覆盖**: Service 到 Service 的东西向流量治理（Istio、Linkerd 等服务网格）
- **domain-03-networking-traffic 覆盖**: Kubernetes 网络基础（CNI、Service、DNS、NetworkPolicy）

## 3. 请求生命周期

一个典型的 API 网关请求处理流程：

```
客户端请求
    │
    ▼
┌─────────────────────────────────────────────────────┐
│                    API 网关                           │
│                                                     │
│  1. TCP 连接建立 (L4)                                │
│     │                                               │
│  2. TLS 握手 / SNI 路由                              │
│     │                                               │
│  3. HTTP 解析 (L7)                                   │
│     │                                               │
│  4. 路由匹配                                         │
│     │  - Host 匹配                                   │
│     │  - Path 匹配                                   │
│     │  - Header/Query 匹配                           │
│     │                                               │
│  5. 插件链执行（请求阶段）                              │
│     │  - 认证 (JWT/OIDC/API Key)                     │
│     │  - 鉴权 (RBAC/OPA)                             │
│     │  - 限流 (Rate Limiting)                        │
│     │  - 请求转换 (Header/Body Rewrite)               │
│     │  - WAF 检测                                    │
│     │                                               │
│  6. 负载均衡 → 转发至 Upstream                        │
│     │  - 轮询 / 加权 / 一致性哈希 / 最少连接            │
│     │                                               │
│  7. 插件链执行（响应阶段）                              │
│     │  - 响应转换                                     │
│     │  - CORS 处理                                   │
│     │  - 压缩                                        │
│     │                                               │
│  8. 指标采集 + 访问日志 + 追踪上报                      │
│     │                                               │
│  9. 响应返回客户端                                    │
└─────────────────────────────────────────────────────┘
```

## 4. 控制平面与数据平面分离

现代 API 网关普遍采用控制平面/数据平面分离架构：

```
┌─────────────────────────────────┐    ┌─────────────────────────────────┐
│          控制平面                 │    │          数据平面                 │
│                                 │    │                                 │
│  ┌───────────┐                  │    │  ┌───────────┐                  │
│  │ 配置管理   │  Admin API /     │    │  │ 代理引擎   │  实际处理流量      │
│  │ 路由发现   │  CRD Controller  │───▶│  │ 插件执行   │  Envoy / Nginx   │
│  │ 证书管理   │  Kubernetes API  │    │  │ 负载均衡   │  OpenResty       │
│  └───────────┘                  │    │  └───────────┘                  │
│                                 │    │                                 │
│  配置存储:                       │    │  无状态、可水平扩展              │
│  - etcd (APISIX)                │    │  - 高性能请求处理                │
│  - PostgreSQL (Kong)            │    │  - 实时指标暴露                  │
│  - Kubernetes CRD (Higress/EG)  │    │  - 访问日志输出                  │
└─────────────────────────────────┘    └─────────────────────────────────┘
```

### 各产品架构对比

| 产品 | 控制平面 | 数据平面 | 配置存储 | 配置下发协议 |
|------|---------|---------|---------|------------|
| **Higress** | Istiod (定制版) | Envoy | Kubernetes CRD | xDS (gRPC) |
| **APISIX** | APISIX Admin API | OpenResty (Nginx + LuaJIT) | etcd | Watch (etcd) |
| **Kong** | Kong Admin API / KIC | Kong (Nginx + Lua) | PostgreSQL / DB-less | DB 轮询 / 声明式 |
| **Envoy Gateway** | EG Controller | Envoy | Kubernetes CRD | xDS (gRPC) |
| **Traefik** | Traefik 内置 | Traefik (Go 原生) | Provider 动态发现 | 内置 Provider |

## 5. 主流产品 CNCF 生态定位

```
┌─────────────────────────────────────────────────────────────────────┐
│                       CNCF 云原生 API 网关生态                       │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  Graduated（毕业）                                                   │
│  ┌──────────┐                                                       │
│  │  Envoy   │  高性能 L7 代理，多个 API 网关的底层数据平面              │
│  └──────────┘                                                       │
│                                                                     │
│  Incubating（孵化中）                                                │
│  ┌──────────────────┐  ┌──────────┐                                 │
│  │ Emissary-Ingress │  │ Contour  │                                 │
│  └──────────────────┘  └──────────┘                                 │
│                                                                     │
│  Sandbox（沙箱）                                                     │
│  ┌──────────┐  ┌──────────┐                                         │
│  │ Higress  │  │ Kgateway │                                         │
│  └──────────┘  └──────────┘                                         │
│                                                                     │
│  Apache 基金会                                                       │
│  ┌──────────┐                                                       │
│  │  APISIX  │  Apache 顶级项目                                       │
│  └──────────┘                                                       │
│                                                                     │
│  商业开源                                                            │
│  ┌──────────┐  ┌──────────┐                                         │
│  │  Kong    │  │ Traefik  │                                         │
│  └──────────┘  └──────────┘                                         │
└─────────────────────────────────────────────────────────────────────┘
```

| 产品 | 基金会/组织 | 成熟度 | 数据平面 | 主要语言 | GitHub Stars |
|------|-----------|--------|---------|---------|-------------|
| **Higress** | CNCF Sandbox | 早期 | Envoy | Go + C++ | 3k+ |
| **APISIX** | Apache TLP | 成熟 | OpenResty | Lua + Go | 14k+ |
| **Kong** | Kong Inc. | 成熟 | Nginx + Lua | Lua + Go | 39k+ |
| **Envoy Gateway** | CNCF (Envoy) | 成长中 | Envoy | Go + C++ | 1.5k+ |
| **Traefik** | Traefik Labs | 成熟 | Go 原生 | Go | 51k+ |

## 6. 架构演进：从 Ingress 到 Gateway API

### 演进路线

```
2015 ──── Kubernetes Ingress 资源 ─────────────────────────────────────
          │  单一资源、注解驱动、厂商碎片化
          │
2018 ──── 自定义 CRD 兴起 ────────────────────────────────────────────
          │  IngressRoute (Traefik)、KongIngress (Kong)
          │  VirtualService (Istio)、ApisixRoute (APISIX)
          │
2020 ──── Gateway API SIG 成立 ────────────────────────────────────────
          │  统一标准提案、角色模型设计
          │
2023 ──── Gateway API v1.0 GA ─────────────────────────────────────────
          │  GatewayClass、Gateway、HTTPRoute 稳定
          │
2024 ──── Gateway API v1.1 ────────────────────────────────────────────
          │  BackendTLSPolicy、Gateway API for Mesh (GAMMA)
          │
2025+ ─── 生态收敛 ────────────────────────────────────────────────────
          所有主流 API 网关支持 Gateway API 作为标准接口
```

> 详细的 Gateway API 技术解析请参考 [02-Kubernetes Gateway API 标准深度解析](./02-kubernetes-gateway-api-deep-dive.md) 以及 [Domain-5: 网络 - Gateway API 概览](../domain-03-networking-traffic/35-gateway-api-overview.md)。

## 7. 产品选型决策树

```
开始选型
  │
  ├── 是否需要 AI 网关能力（LLM 代理、Token 限流、语义缓存）？
  │     └── 是 → Higress（AI 网关原生支持）
  │
  ├── 是否需要丰富的插件生态 + 企业级管理控制台？
  │     └── 是 → Kong（最大插件市场）或 APISIX（100+ 内置插件）
  │
  ├── 是否需要 Gateway API 原生体验、最小化运维开销？
  │     └── 是 → Envoy Gateway（Gateway API First 设计）
  │
  ├── 是否需要轻量级部署 + 自动 TLS 证书管理？
  │     └── 是 → Traefik（ACME 原生支持、单二进制部署）
  │
  ├── 是否已在使用 Istio 服务网格？
  │     └── 是 → Higress（Istiod 控制平面复用）或 Istio Gateway
  │
  └── 是否有阿里云 ACK 生态需求？
        └── 是 → Higress（阿里云原生支持）
```

> 详细的多维度对比矩阵请参考 [03-API 网关选型指南](./03-api-gateway-selection-guide.md)。

---

## 参考资料

- [Kubernetes Gateway API 官方文档](https://gateway-api.sigs.k8s.io/)
- [CNCF Landscape - API Gateway](https://landscape.cncf.io/)
- [Domain-5: 网络 - Ingress 与 API Gateway 对比表](../domain-03-networking-traffic/36-api-gateway-patterns.md)
- [Domain-26: 服务网格与微服务治理](../domain-03-networking-traffic)

---

## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[domain-03-networking-traffic/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 05 - Apache APISIX 企业级 API 网关实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移
- 10 - Wasm 插件生态与开发实践

## See Also

- 14-api-gateway-production-operations
- 99-envoy-gateway-enterprise-guide
- 02-kubernetes-gateway-api-deep-dive
- 03-api-gateway-selection-guide
