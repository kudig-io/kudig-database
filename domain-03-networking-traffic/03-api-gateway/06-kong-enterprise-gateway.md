---
title: 06 - Kong API 网关企业级实践
description: '# 06 - Kong API 网关企业级实践'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- prometheus
- helm
- opa
- redis
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
- Kong API 网关企业级实践 是什么
- 如何 Kong API 网关企业级实践
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- Kong
- API
- 网关企业级实践
- cloud
- native
- api
- gateway
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- prometheus-basics
- kafka-basics
- redis-basics
- policy-basics
- observability-basics
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

# 06 - Kong API 网关企业级实践

> **文档版本**: v1.0 | **适用版本**: Kong Gateway 3.x, [[Kubernetes|Kubernetes]] 1.25+ | **更新日期**: 2026-03-04 | **关键词**: Kong, KIC, Kong [[Ingress|Ingress]] Controller, deck, Konnect, DB-less

<!-- chunk: 目录 -->## 目录

1. [Kong 项目概述](#1-kong-项目概述)
2. [核心架构](#2-核心架构)
3. [部署模式](#3-部署模式)
4. [路由与服务配置](#4-路由与服务配置)
5. [Kong Ingress Controller (KIC)](#5-kong-ingress-controller-kic)
6. [插件生态](#6-插件生态)
7. [声明式配置（deck）](#7-声明式配置deck)
8. [Kong AI Gateway](#8-kong-ai-gateway)
9. [可观测性](#9-可观测性)
10. [CE vs EE 功能对比](#10-ce-vs-ee-功能对比)

---

<!-- chunk: 1. Kong 项目概述 -->## 1. Kong 项目概述

Kong 是全球使用最广泛的开源 API 网关之一，由 Kong Inc. 于 2015 年开源，基于 Nginx 和 OpenResty 构建。

#<!-- chunk: 核心特点 -->## 核心特点

- **成熟稳定**: 10 年以上生产验证，支撑数万亿级 API 调用
- **丰富生态**: 最大的 API 网关插件市场（Kong Plugin Hub）
- **灵活部署**: 支持传统 DB 模式、DB-less 模式和 Konnect 云托管
- **多平台**: Kubernetes、VM、裸金属、多云环境均可部署

#<!-- chunk: 产品线 -->## 产品线

| 产品 | 定位 | 许可 |
|------|------|------|
| **Kong Gateway (CE)** | 开源 API 网关核心 | Apache 2.0 |
| **Kong Gateway (EE)** | 企业版，含 Manager GUI、RBAC、审计等 | 商业许可 |
| **Kong Konnect** | 全球化 SaaS 管理平面 | SaaS 订阅 |
| **Kong Ingress Controller** | Kubernetes 原生控制器 | Apache 2.0 |
| **Kong Mesh** | 服务网格（基于 [[Kuma|Kuma]]） | 商业许可 |

<!-- chunk: 2. 核心架构 -->## 2. 核心架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     Kong Gateway 架构                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────┐       │
│  │                   控制平面                             │       │
│  │  ┌──────────────┐  ┌──────────────┐                  │       │
│  │  │ Admin API    │  │ Kong Manager │                  │       │
│  │  │ (RESTful)    │  │ (GUI, EE)    │                  │       │
│  │  └──────┬───────┘  └──────────────┘                  │       │
│  │         │                                            │       │
│  │  ┌──────▼───────┐                                    │       │
│  │  │ PostgreSQL   │  配置存储（DB 模式）                  │       │
│  │  │ 或 声明式文件  │  声明式配置（DB-less 模式）          │       │
│  │  └──────┬───────┘                                    │       │
│  └─────────│────────────────────────────────────────────┘       │
│            │                                                    │
│  ┌─────────▼────────────────────────────────────────────┐       │
│  │                   数据平面                             │       │
│  │  ┌──────────────────────────────────────────┐        │       │
│  │  │         Nginx + OpenResty + LuaJIT        │        │       │
│  │  │                                          │        │       │
│  │  │  请求 → 路由匹配 → 插件执行 → 上游转发      │        │       │
│  │  │                                          │        │       │
│  │  │  插件执行阶段:                             │        │       │
│  │  │  certificate → rewrite → access →        │        │       │
│  │  │  response → header_filter → body_filter  │        │       │
│  │  │  → log                                   │        │       │
│  │  └──────────────────────────────────────────┘        │       │
│  └──────────────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────────────┘
```

<!-- chunk: 3. 部署模式 -->## 3. 部署模式

#<!-- chunk: 模式一：DB 模式（传统） -->## 模式一：DB 模式（传统）

配置存储在 PostgreSQL 中，支持多节点集群：

```bash
# Helm 安装（DB 模式）
helm install kong kong/kong \
  -n kong \
  --create-namespace \
  --set env.database=postgres \
  --set env.pg_host=postgres.svc \
  --set env.pg_user=kong \
  --set env.pg_password=kongpass \
  --set env.pg_database=kong
```

#<!-- chunk: 模式二：DB-less 模式（推荐 K8s） -->## 模式二：DB-less 模式（推荐 K8s）

无需数据库，配置通过声明式文件加载：

```bash
# Helm 安装（DB-less 模式）
helm install kong kong/kong \
  -n kong \
  --create-namespace \
  --set env.database=off \
  --set ingressController.enabled=true \
  --set ingressController.installCRDs=false
```

#<!-- chunk: 模式三：混合模式（Hybrid） -->## 模式三：混合模式（Hybrid）

控制平面与数据平面分离部署，适合多集群/多区域：

```
┌─────────────────────┐         ┌─────────────────────┐
│  控制平面 (CP)       │         │  数据平面 (DP)       │
│  ┌───────────────┐  │  mTLS   │  ┌───────────────┐  │
│  │ Admin API     │  │◄───────▶│  │ Kong Proxy    │  │
│  │ PostgreSQL    │  │  配置同步 │  │ (无 DB)       │  │
│  │ Kong Manager  │  │         │  │ 实际处理流量   │  │
│  └───────────────┘  │         │  └───────────────┘  │
└─────────────────────┘         └─────────────────────┘
```

```yaml
# 控制平面配置
env:
  role: control_plane
  cluster_cert: /certs/cluster.crt
  cluster_cert_key: /certs/cluster.key

# 数据平面配置
env:
  role: data_plane
  database: "off"
  cluster_control_plane: cp.kong.svc:8005
  cluster_cert: /certs/cluster.crt
  cluster_cert_key: /certs/cluster.key
```

<!-- chunk: 4. 路由与服务配置 -->## 4. 路由与服务配置

#<!-- chunk: Admin API -->## Admin API

```bash
# 创建服务
curl -i -X POST http://localhost:8001/services/ \
  --data name=api-service \
  --data url=http://backend.svc:8080

# 创建路由
curl -i -X POST http://localhost:8001/services/api-service/routes \
  --data name=api-route \
  --data 'hosts[]=api.example.com' \
  --data 'paths[]=/api' \
  --data strip_path=true

# 添加插件
curl -i -X POST http://localhost:8001/routes/api-route/plugins \
  --data name=rate-limiting \
  --data config.minute=100 \
  --data config.policy=redis \
  --data config.redis_host=redis.svc
```

#<!-- chunk: 负载均衡配置 -->## 负载均衡配置

```bash
# 创建上游和目标
curl -i -X POST http://localhost:8001/upstreams \
  --data name=api-upstream \
  --data algorithm=round-robin \
  --data healthchecks.active.http_path=/health \
  --data healthchecks.active.healthy.interval=5 \
  --data healthchecks.active.unhealthy.interval=3

# 添加目标节点
curl -i -X POST http://localhost:8001/upstreams/api-upstream/targets \
  --data target=backend-1.svc:8080 \
  --data weight=100

curl -i -X POST http://localhost:8001/upstreams/api-upstream/targets \
  --data target=backend-2.svc:8080 \
  --data weight=100
```

<!-- chunk: 5. Kong Ingress Controller (KIC) -->## 5. Kong Ingress Controller (KIC)

KIC 将 Kubernetes 资源（Ingress、Gateway API、自定义 CRD）转换为 Kong 配置：

#<!-- chunk: Kubernetes Ingress -->## Kubernetes Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo
  annotations:
    konghq.com/strip-path: "true"
    konghq.com/plugins: rate-limiting-plugin
spec:
  ingressClassName: kong
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

#<!-- chunk: Gateway API -->## Gateway API

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: api-route
  annotations:
    konghq.com/plugins: rate-limiting-plugin
spec:
  parentRefs:
  - name: kong-gateway
  hostnames:
  - "api.example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: api-service
      port: 8080
```

#<!-- chunk: KongPlugin CRD -->## KongPlugin CRD

```yaml
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: rate-limiting-plugin
config:
  minute: 100
  policy: local
plugin: rate-limiting

---
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: jwt-plugin
config:
  claims_to_verify:
  - exp
plugin: jwt
```

<!-- chunk: 6. 插件生态 -->## 6. 插件生态

#<!-- chunk: 核心插件分类 -->## 核心插件分类

| 类别 | CE 插件 | EE 独有 |
|------|--------|---------|
| **认证** | basic-auth, key-auth, jwt, hmac-auth, oauth2 | openid-connect, vault-auth, mutual-tls-auth |
| **安全** | cors, ip-restriction, bot-detection, acl | opa, application-registration |
| **流量** | rate-limiting, request-size-limiting, proxy-cache | rate-limiting-advanced, graphql-rate-limiting |
| **转换** | request-transformer, response-transformer, correlation-id | request-transformer-advanced, jq |
| **日志** | http-log, file-log, syslog, tcp-log, udp-log | kafka-log, datadog |
| **分析** | prometheus, zipkin, opentelemetry | collector |

#<!-- chunk: 自定义 Lua 插件 -->## 自定义 Lua 插件

```lua
-- kong/plugins/my-plugin/handler.lua
local MyPlugin = {
  PRIORITY = 1000,
  VERSION = "1.0.0",
}

function MyPlugin:access(conf)
  local token = kong.request.get_header("X-API-Token")
  if not token or token ~= conf.expected_token then
    return kong.response.exit(401, { message = "Unauthorized" })
  end
  kong.service.request.set_header("X-Authenticated", "true")
end

return MyPlugin
```

```lua
-- kong/plugins/my-plugin/schema.lua
return {
  name = "my-plugin",
  fields = {
    { config = {
      type = "record",
      fields = {
        { expected_token = { type = "string", required = true } },
      },
    }},
  },
}
```

<!-- chunk: 7. 声明式配置（deck） -->## 7. 声明式配置（deck）

deck 是 Kong 的声明式配置管理工具，支持 GitOps 工作流：

```yaml
# kong.yaml
_format_version: "3.0"
_transform: true

services:
- name: api-service
  url: http://backend.svc:8080
  connect_timeout: 60000
  read_timeout: 60000
  write_timeout: 60000
  retries: 5
  routes:
  - name: api-route
    hosts:
    - api.example.com
    paths:
    - /api
    strip_path: true
    plugins:
    - name: rate-limiting
      config:
        minute: 100
        policy: local
    - name: jwt
      config:
        claims_to_verify:
        - exp

plugins:
- name: prometheus
  enabled: true

- name: correlation-id
  config:
    header_name: X-Request-ID
    generator: uuid
```

```bash
# 同步配置
deck gateway sync kong.yaml

# 对比差异
deck gateway diff kong.yaml

# 导出当前配置
deck gateway dump -o current.yaml

# 验证配置语法
deck gateway validate kong.yaml
```

<!-- chunk: 8. Kong AI Gateway -->## 8. Kong AI Gateway

Kong AI Gateway 提供 LLM 代理和 AI 流量管理能力：

```yaml
# AI 代理插件配置
services:
- name: ai-proxy-service
  url: http://localhost:8080  # placeholder
  routes:
  - name: ai-route
    paths:
    - /ai
    plugins:
    - name: ai-proxy
      config:
        route_type: llm/v1/chat
        auth:
          header_name: Authorization
          header_value: "Bearer ${OPENAI_API_KEY}"
        model:
          provider: openai
          name: gpt-4
          options:
            max_tokens: 1024
            temperature: 0.7
```

#<!-- chunk: AI 限流 -->## AI 限流

```yaml
plugins:
- name: ai-rate-limiting-advanced
  config:
    limit:
    - 10000    # tokens per minute
    window_size:
    - 60
    limit_by: consumer
    strategy: redis
    redis:
      host: redis.svc
```

<!-- chunk: 9. 可观测性 -->## 9. 可观测性

#<!-- chunk: Prometheus 指标 -->## Prometheus 指标

```yaml
# KongPlugin CRD 配置 Prometheus
apiVersion: configuration.konghq.com/v1
kind: KongClusterPlugin
metadata:
  name: prometheus
  labels:
    global: "true"
config:
  per_consumer: true
  status_code_metrics: true
  latency_metrics: true
  bandwidth_metrics: true
  upstream_health_metrics: true
plugin: prometheus
```

核心指标：
- `kong_http_requests_total` — HTTP 请求总数
- `kong_request_latency_ms` — 请求延迟
- `kong_upstream_latency_ms` — 上游响应延迟
- `kong_bandwidth_bytes` — 带宽统计
- `kong_upstream_target_health` — 上游健康状态

#<!-- chunk: OpenTelemetry 追踪 -->## OpenTelemetry 追踪

```yaml
plugins:
- name: opentelemetry
  config:
    endpoint: "http://otel-collector.svc:4318/v1/traces"
    resource_attributes:
      service.name: kong-gateway
    header_type: w3c
```

<!-- chunk: 10. CE vs EE 功能对比 -->## 10. CE vs EE 功能对比

| 功能 | CE (开源) | EE (企业) |
|------|----------|----------|
| **核心网关** | ✅ | ✅ |
| **Admin API** | ✅ | ✅ |
| **DB-less 模式** | ✅ | ✅ |
| **混合模式** | ✅ | ✅ |
| **基础插件** | ✅ (40+) | ✅ (40+) |
| **高级插件** | ❌ | ✅ (额外 30+) |
| **Kong Manager GUI** | ❌ | ✅ |
| **RBAC** | ❌ | ✅ |
| **工作空间** | ❌ | ✅ |
| **审计日志** | ❌ | ✅ |
| **开发者门户** | ❌ | ✅ |
| **OpenID Connect** | ❌ | ✅ |
| **Vault 集成** | ❌ | ✅ |
| **FIPS 140-2** | ❌ | ✅ |
| **商业支持** | 社区 | 24/7 SLA |

> 对于预算有限但需要 OIDC 等高级特性的团队，可以考虑 APISIX 或 Higress 作为开源替代方案，这些功能在其社区版中免费提供。

---

<!-- chunk: 参考资料 -->## 参考资料

- [Kong 官方文档](https://docs.konghq.com/)
- [Kong GitHub](https://github.com/Kong/kong)
- [Kong Plugin Hub](https://docs.konghq.com/hub/)
- [Kong Ingress Controller](https://docs.konghq.com/kubernetes-ingress-controller/)
- [deck CLI](https://docs.konghq.com/deck/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[domain-03-networking-traffic/README|Domain 98: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 01 - 云原生 API 网关架构总览
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 05 - Apache APISIX 企业级 API 网关实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移
- 10 - Wasm 插件生态与开发实践

## See Also

- 04-higress-enterprise-gateway
- 05-apisix-enterprise-gateway
- 07-envoy-gateway-enterprise
- 08-traefik-enterprise-gateway
