---
title: 05 - Apache APISIX 企业级 API 网关实践
description: '# 05 - Apache APISIX 企业级 API 网关实践'
summary: '# 05 - Apache APISIX 企业级 API 网关实践'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- etcd
- prometheus
- helm
- docker
- redis
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Apache APISIX 企业级 API 网关实践 是什么
- 如何 Apache APISIX 企业级 API 网关实践
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- Apache
- APISIX
- 企业级
- API
- 网关实践
- cloud
- native
- api
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- prometheus-basics
- etcd-basics
- kafka-basics
- redis-basics
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
---



# 05 - Apache APISIX 企业级 API 网关实践

> **文档版本**: v1.0 | **适用版本**: APISIX 3.x, [[Kubernetes|Kubernetes]] 1.25+ | **更新日期**: 2026-03-04 | **关键词**: APISIX, OpenResty, [[etcd|etcd]], Lua, 插件, APISIX [[Ingress|Ingress]] Controller

<!-- chunk: 目录 -->## 目录

1. [APISIX 项目概述](#1-apisix-项目概述)
2. [核心架构](#2-核心架构)
3. [部署安装](#3-部署安装)
4. [路由与上游配置](#4-路由与上游配置)
5. [插件生态](#5-插件生态)
6. [多语言插件运行时](#6-多语言插件运行时)
7. [APISIX Ingress Controller](#7-apisix-ingress-controller)
8. [声明式配置（ADC）](#8-声明式配置adc)
9. [可观测性](#9-可观测性)
10. [生产环境最佳实践](#10-生产环境最佳实践)

---

<!-- chunk: 1. APISIX 项目概述 -->## 1. APISIX 项目概述

Apache APISIX 是 Apache 软件基金会顶级项目，由 API7.ai 主导开发，是一款高性能、可扩展的云原生 API 网关。

## 核心特点

- **极致性能**: 基于 OpenResty（Nginx + LuaJIT），单核可达 10K+ QPS
- **丰富插件**: 100+ 内置插件，覆盖认证、限流、安全、可观测等场景
- **多语言扩展**: 支持 Lua、Go、Java、Python、Wasm 插件运行时
- **动态配置**: 基于 etcd 实时配置下发，毫秒级生效
- **全协议支持**: HTTP/HTTPS、[[gRPC|gRPC]]、WebSocket、TCP/UDP、MQTT

## 发展历程

| 时间 | 里程碑 |
|------|--------|
| 2019-06 | 开源发布 |
| 2019-10 | 进入 Apache 孵化器 |
| 2021-06 | 成为 Apache 顶级项目 |
| 2023-03 | APISIX 3.0 发布（Wasm 支持） |
| 2024-06 | Gateway API 支持 |

<!-- chunk: 2. 核心架构 -->## 2. 核心架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     APISIX 架构总览                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────┐       │
│  │                   控制平面                             │       │
│  │  ┌──────────────┐  ┌──────────────┐                  │       │
│  │  │ Admin API    │  │ APISIX       │                  │       │
│  │  │ (RESTful)    │  │ Dashboard    │                  │       │
│  │  └──────┬───────┘  └──────────────┘                  │       │
│  │         │                                            │       │
│  │  ┌──────▼───────┐                                    │       │
│  │  │    etcd      │  配置存储（路由、插件、上游、证书）     │       │
│  │  └──────┬───────┘                                    │       │
│  └─────────│────────────────────────────────────────────┘       │
│            │  etcd Watch (实时推送)                              │
│  ┌─────────▼────────────────────────────────────────────┐       │
│  │                   数据平面                             │       │
│  │  ┌──────────────────────────────────────────┐        │       │
│  │  │           OpenResty (Nginx + LuaJIT)      │        │       │
│  │  │                                          │        │       │
│  │  │  请求 → 路由匹配 → 插件链执行 → 上游转发    │        │       │
│  │  │                                          │        │       │
│  │  │  插件执行阶段:                             │        │       │
│  │  │  rewrite → access → before_proxy →       │        │       │
│  │  │  header_filter → body_filter → log       │        │       │
│  │  └──────────────────────────────────────────┘        │       │
│  │                                                      │       │
│  │  ┌──────────────────────────────────────────┐        │       │
│  │  │         多语言插件运行时（可选）              │        │       │
│  │  │  Go Runner │ Java Runner │ Python Runner  │        │       │
│  │  │  Wasm Runtime                             │        │       │
│  │  └──────────────────────────────────────────┘        │       │
│  └──────────────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────────────┘
```

## 核心概念

| 概念 | 说明 |
|------|------|
| **Route** | 路由规则，定义请求匹配条件和转发目标 |
| **Upstream** | 上游服务，定义后端服务地址和负载均衡策略 |
| **Service** | 服务抽象，关联路由和上游 |
| **Plugin** | 插件，在请求生命周期各阶段执行逻辑 |
| **Consumer** | 消费者，关联认证信息和插件策略 |
| **Plugin Config** | 插件配置组，可复用的插件配置集合 |
| **Global Rule** | 全局规则，应用于所有路由的插件 |

<!-- chunk: 3. 部署安装 -->## 3. 部署安装

## Helm 安装（Kubernetes）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```bash
# 添加 Helm 仓库
helm repo add apisix https://charts.apiseven.com
helm repo update

# 安装 APISIX（含 etcd）
helm install apisix apisix/apisix \
  -n apisix \
  --create-namespace \
  --set gateway.type=LoadBalancer \
  --set ingress-controller.enabled=true \
  --set dashboard.enabled=true

# 验证安装
kubectl get pods -n apisix
```

## Docker Compose（开发/测试）

```yaml
version: "3"
services:
  apisix:
    image: apache/apisix:3.9-debian
    ports:
      - "9080:9080"
      - "9443:9443"
      - "9180:9180"   # Admin API
    volumes:
      - ./apisix_conf/config.yaml:/usr/local/apisix/conf/config.yaml
    depends_on:
      - etcd

  etcd:
    image: bitnami/etcd:3.5
    environment:
      - ALLOW_NONE_AUTHENTICATION=yes
    ports:
      - "2379:2379"

  apisix-dashboard:
    image: apache/apisix-dashboard:3.0
    ports:
      - "9000:9000"
    volumes:
      - ./dashboard_conf/conf.yaml:/usr/local/apisix-dashboard/conf/conf.yaml
```

<!-- chunk: 4. 路由与上游配置 -->## 4. 路由与上游配置

## Admin API 创建路由

```bash
# 创建上游
curl http://127.0.0.1:9180/apisix/admin/upstreams/1 \
  -H 'X-API-KEY: edd1c9f034335f136f87ad84b625c8f1' \
  -X PUT -d '{
    "type": "roundrobin",
    "nodes": {
      "httpbin.org:80": 1
    }
  }'

# 创建路由
curl http://127.0.0.1:9180/apisix/admin/routes/1 \
  -H 'X-API-KEY: edd1c9f034335f136f87ad84b625c8f1' \
  -X PUT -d '{
    "uri": "/api/*",
    "host": "api.example.com",
    "upstream_id": "1",
    "plugins": {
      "limit-count": {
        "count": 100,
        "time_window": 60,
        "rejected_code": 429
      }
    }
  }'
```

## 负载均衡策略

```json
{
  "type": "chash",
  "key": "remote_addr",
  "nodes": {
    "backend-1:8080": 3,
    "backend-2:8080": 2,
    "backend-3:8080": 1
  },
  "retries": 3,
  "retry_timeout": 6,
  "checks": {
    "active": {
      "type": "http",
      "http_path": "/health",
      "healthy": {
        "interval": 5,
        "successes": 2
      },
      "unhealthy": {
        "interval": 3,
        "http_failures": 3
      }
    }
  }
}
```

支持的负载均衡算法：
- `roundrobin` — 加权轮询（默认）
- `chash` — 一致性哈希
- `ewma` — 指数加权移动平均（最小延迟）
- `least_conn` — 最少连接

<!-- chunk: 5. 插件生态 -->## 5. 插件生态

## 常用插件速查

| 类别 | 插件 | 说明 |
|------|------|------|
| **认证** | key-auth | API Key 认证 |
| | jwt-auth | JWT Token 验证 |
| | openid-connect | OIDC/OAuth2 认证 |
| | hmac-auth | HMAC 签名认证 |
| | basic-auth | HTTP Basic 认证 |
| **安全** | cors | CORS 跨域配置 |
| | ip-restriction | IP 黑白名单 |
| | ua-restriction | User-Agent 限制 |
| | uri-blocker | URI 黑名单 |
| | csrf | CSRF 防护 |
| **限流** | limit-count | 固定窗口限流 |
| | limit-req | 漏桶限流 |
| | limit-conn | 并发连接限制 |
| **转换** | proxy-rewrite | URL 重写 |
| | response-rewrite | 响应重写 |
| | request-validation | 请求体 JSON Schema 校验 |
| | grpc-transcode | HTTP → gRPC 转换 |
| **可观测** | prometheus | Prometheus 指标 |
| | skywalking | SkyWalking 追踪 |
| | opentelemetry | OpenTelemetry 集成 |
| | http-logger | HTTP 日志上报 |
| | kafka-logger | Kafka 日志上报 |
| **流量** | traffic-split | 流量分割/金丝雀 |
| | proxy-mirror | 流量镜像 |

## 插件配置示例

```json
{
  "uri": "/api/*",
  "plugins": {
    "jwt-auth": {},
    "limit-count": {
      "count": 1000,
      "time_window": 60,
      "group": "api_group",
      "policy": "redis",
      "redis_host": "redis.svc",
      "redis_port": 6379
    },
    "proxy-rewrite": {
      "regex_uri": ["^/api/(.*)$", "/$1"]
    },
    "opentelemetry": {
      "sampler": {
        "name": "always_on"
      }
    }
  }
}
```

<!-- chunk: 6. 多语言插件运行时 -->## 6. 多语言插件运行时

APISIX 支持通过 Plugin Runner 机制运行 Go、Java、Python 编写的插件：

## Go 插件示例

```go
package main

import (
    "net/http"
    "github.com/apache/apisix-go-plugin-runner/pkg/plugin"
    "github.com/apache/apisix-go-plugin-runner/pkg/runner"
)

type CustomAuth struct{}

func (p *CustomAuth) Name() string {
    return "custom-auth"
}

func (p *CustomAuth) RequestFilter(conf interface{}, w http.ResponseWriter, r pkgHTTP.Request) {
    token := r.Header().Get("Authorization")
    if token == "" {
        w.WriteHeader(http.StatusUnauthorized)
        return
    }
    // 验证逻辑
}

func main() {
    runner.Run(runner.RunnerConfig{}, &CustomAuth{})
}
```

## 配置 Plugin Runner

```yaml
# config.yaml
ext-plugin:
  cmd:
  - "/usr/local/apisix-go-plugin-runner/go-runner"
  - "run"
```

<!-- chunk: 7. APISIX Ingress Controller -->## 7. APISIX Ingress Controller

APISIX Ingress Controller 在 Kubernetes 中以 CRD 方式管理 APISIX 配置：

## 自定义 CRD 路由

```yaml
apiVersion: apisix.apache.org/v2
kind: ApisixRoute
metadata:
  name: demo-route
spec:
  http:
  - name: rule1
    match:
      hosts:
      - api.example.com
      paths:
      - /api/*
    backends:
    - serviceName: api-service
      servicePort: 8080
      weight: 90
    - serviceName: api-service-canary
      servicePort: 8080
      weight: 10
    plugins:
    - name: limit-count
      enable: true
      config:
        count: 1000
        time_window: 60
```

## Kubernetes Ingress 兼容

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo
  annotations:
    k8s.apisix.apache.org/plugin-config-name: "shared-plugins"
spec:
  ingressClassName: apisix
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

<!-- chunk: 8. 声明式配置（ADC） -->## 8. 声明式配置（ADC）

APISIX Declarative CLI (ADC) 支持 GitOps 风格的声明式配置管理：

```yaml
# apisix.yaml
services:
- name: api-service
  upstream:
    type: roundrobin
    nodes:
    - host: backend-1
      port: 8080
      weight: 1
  routes:
  - name: api-route
    uris:
    - /api/*
    hosts:
    - api.example.com
    plugins:
      jwt-auth: {}
      limit-count:
        count: 1000
        time_window: 60
```

```bash
# 同步配置到 APISIX
adc sync -f apisix.yaml

# 对比差异
adc diff -f apisix.yaml

# 导出当前配置
adc dump -o current.yaml
```

<!-- chunk: 9. 可观测性 -->## 9. 可观测性

## Prometheus 指标

```bash
# APISIX 内置 Prometheus 指标端点
curl http://127.0.0.1:9091/apisix/prometheus/metrics

# 核心指标
# apisix_http_status                    - HTTP 状态码计数
# apisix_http_latency_bucket            - 延迟分布
# apisix_bandwidth                      - 带宽统计
# apisix_upstream_status                - 上游健康状态
# apisix_node_info                      - 节点信息
```

## OpenTelemetry 集成

```json
{
  "plugins": {
    "opentelemetry": {
      "sampler": {
        "name": "always_on"
      },
      "additional_attributes": ["route_id", "service_id"],
      "additional_header_prefix_attributes": ["x-"]
    }
  }
}
```

<!-- chunk: 10. 生产环境最佳实践 -->## 10. 生产环境最佳实践

## etcd 高可用

```yaml
# etcd 集群部署（至少 3 节点）
etcd:
  replicaCount: 3
  persistence:
    enabled: true
    size: 20Gi
    storageClass: fast-ssd
  resources:
    requests:
      cpu: "1"
      memory: "2Gi"
```

## APISIX 资源配置

```yaml
gateway:
  replicas: 3
  resources:
    requests:
      cpu: "2"
      memory: "2Gi"
    limits:
      cpu: "4"
      memory: "4Gi"
```

## 关键调优参数

```yaml
# config.yaml
nginx_config:
  worker_processes: auto
  worker_connections: 65536
  keepalive_timeout: 60
  
  http:
    lua_shared_dict:
      prometheus-metrics: 50m
      plugin-limit-count-redis-cluster-slot-lock: 1m

apisix:
  enable_admin: true
  ssl:
    enable: true
    listen: 9443
  router:
    http: radixtree_host_uri    # 高性能路由匹配
```

---

<!-- chunk: 参考资料 -->## 参考资料

- [Apache APISIX 官方文档](https://apisix.apache.org/docs/)
- [APISIX GitHub](https://github.com/apache/apisix)
- [APISIX 插件中心](https://apisix.apache.org/plugins/)
- [APISIX Ingress Controller](https://apisix.apache.org/docs/ingress-controller/getting-started/)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[domain-03-networking-traffic/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 01 - 云原生 API 网关架构总览
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移
- 10 - Wasm 插件生态与开发实践

## See Also

- 03-api-gateway-selection-guide
- 04-higress-enterprise-gateway
- 06-kong-enterprise-gateway
- 07-envoy-gateway-enterprise

```