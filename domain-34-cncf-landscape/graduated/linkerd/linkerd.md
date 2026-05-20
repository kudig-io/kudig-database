---
title: Linkerd
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- grafana
- jaeger
- istio
- envoy
- gateway
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Linkerd 是什么
- 如何 Linkerd
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Linkerd
- cncf
- landscape
---


# Linkerd

> **成熟度**: Graduated | **加入时间**: 2017-01 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://linkerd.io |
| **GitHub** | https://github.com/linkerd/linkerd2 |
| **文档** | https://linkerd.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Rust, Go |
| **CNCF 分类** | Service Mesh |

---

## 项目概述

### 简介
Linkerd 是世界上第一个服务网格，也是最轻量、最快的 Kubernetes 服务网格。它使用 Rust 编写的超轻量代理 (linkerd2-proxy)，为服务间通信提供零配置 mTLS、可观测性和可靠性功能。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2016-01 | Buoyant 发布 Linkerd 1.0 (Scala/JVM) |
| 2017-01 | 成为 CNCF 首批项目 |
| 2018-09 | Linkerd 2.0 发布 (Rust 代理) |
| 2021-07 | 晋升为 CNCF Graduated |

### 核心定位
Linkerd 是追求**简单**和**性能**的服务网格首选。与 Istio 相比，它更轻量、更安全、更易于运维，专注于做好服务网格的核心功能。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      Linkerd 架构                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                   Control Plane                              ││
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            ││
│  │  │ Destination │ │  Identity   │ │ Proxy       │            ││
│  │  │ (服务发现)  │ │ (证书管理)  │ │ Injector    │            ││
│  │  └─────────────┘ └─────────────┘ └─────────────┘            ││
│  │  ┌─────────────┐ ┌─────────────┐                            ││
│  │  │  Heartbeat  │ │    Viz      │ (可选扩展)                 ││
│  │  │ (遥测上报)  │ │  (可视化)   │                            ││
│  │  └─────────────┘ └─────────────┘                            ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│                              │ gRPC                              │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Data Plane                                ││
│  │                                                              ││
│  │   Pod A                           Pod B                      ││
│  │  ┌──────────────────┐           ┌──────────────────┐        ││
│  │  │ ┌──────────────┐ │   mTLS    │ ┌──────────────┐ │        ││
│  │  │ │   App        │ │◄─────────►│ │   App        │ │        ││
│  │  │ └──────────────┘ │           │ └──────────────┘ │        ││
│  │  │       ▲          │           │       ▲          │        ││
│  │  │       │          │           │       │          │        ││
│  │  │ ┌─────┴────────┐ │           │ ┌─────┴────────┐ │        ││
│  │  │ │linkerd-proxy │ │           │ │linkerd-proxy │ │        ││
│  │  │ │   (Rust)     │ │           │ │   (Rust)     │ │        ││
│  │  │ │  < 10MB RAM  │ │           │ │  < 10MB RAM  │ │        ││
│  │  │ └──────────────┘ │           │ └──────────────┘ │        ││
│  │  └──────────────────┘           └──────────────────┘        ││
│  │                                                              ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 与 Istio 对比

```
┌─────────────────────────────────────────────────────────────────┐
│                  Linkerd vs Istio                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  特性              Linkerd              Istio                   │
│  ─────────────────────────────────────────────────────────────  │
│  代理              linkerd2-proxy       Envoy                   │
│                    (Rust, 专用)         (C++, 通用)             │
│  代理内存          ~10MB                ~50-100MB               │
│  控制平面内存      ~200MB               ~1GB+                   │
│  安装复杂度        简单 (2条命令)       复杂                    │
│  学习曲线          低                   高                      │
│  功能丰富度        核心功能             全功能                  │
│  mTLS              默认开启             需配置                  │
│  Gateway           基础                 强大 (Envoy Gateway)    │
│  多集群            支持                 支持                    │
│  适用场景          追求简单和性能       需要高级功能            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 安装部署

### CLI 安装

```bash
# 安装 Linkerd CLI
curl -sL https://run.linkerd.io/install | sh
export PATH=$PATH:$HOME/.linkerd2/bin

# 或使用 Homebrew
brew install linkerd

# 验证 CLI
linkerd version
```

### 集群安装

```bash
# 预检查
linkerd check --pre

# 安装 CRDs
linkerd install --crds | kubectl apply -f -

# 安装控制平面
linkerd install | kubectl apply -f -

# 验证安装
linkerd check

# 安装可视化扩展 (可选)
linkerd viz install | kubectl apply -f -

# 访问 Dashboard
linkerd viz dashboard &
```

### 高可用安装

```bash
# 高可用模式安装
linkerd install --ha | kubectl apply -f -

# 自定义资源配置
linkerd install \
  --controller-replicas 3 \
  --proxy-cpu-request 100m \
  --proxy-memory-request 64Mi \
  --proxy-cpu-limit 1 \
  --proxy-memory-limit 256Mi \
  | kubectl apply -f -
```

---

## 核心功能

### 1. 自动 mTLS

```bash
# 注入 Sidecar (自动获得 mTLS)
kubectl get deploy -o yaml | linkerd inject - | kubectl apply -f -

# 或使用注解自动注入
kubectl annotate namespace default linkerd.io/inject=enabled

# 验证 mTLS
linkerd viz edges deploy
linkerd viz tap deploy/web
```

```
┌─────────────────────────────────────────────────────────────────┐
│                    自动 mTLS 流程                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Pod A                                Pod B                      │
│  ┌────────────────────┐              ┌────────────────────┐     │
│  │ App (明文请求)     │              │ App (明文响应)     │     │
│  │    ▼               │              │    ▲               │     │
│  │ linkerd-proxy ─────┼──── mTLS ────┼─► linkerd-proxy    │     │
│  │                    │              │                    │     │
│  │ 证书自动轮换       │              │ 证书自动轮换       │     │
│  │ (24小时有效期)     │              │ (24小时有效期)     │     │
│  └────────────────────┘              └────────────────────┘     │
│                                                                  │
│  • 零配置: 注入即生效                                            │
│  • 透明: 应用无感知                                              │
│  • 自动: 证书自动签发和轮换                                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 2. 可观测性

```bash
# 实时流量监控
linkerd viz top deploy/web

# 流量详情
linkerd viz tap deploy/web

# 服务指标
linkerd viz stat deploy

# 路由指标
linkerd viz routes deploy/web

# Grafana 仪表板
linkerd viz dashboard
```

### 3. 流量管理

```yaml
# ServiceProfile 定义路由
apiVersion: linkerd.io/v1alpha2
kind: ServiceProfile
metadata:
  name: webapp.default.svc.cluster.local
  namespace: default
spec:
  routes:
    - name: GET /api/users
      condition:
        method: GET
        pathRegex: /api/users
      responseClasses:
        - condition:
            status:
              min: 500
              max: 599
          isFailure: true
    - name: POST /api/orders
      condition:
        method: POST
        pathRegex: /api/orders
      # 超时配置
      timeout: 30s
```

### 4. 重试和超时

```yaml
# 自动重试配置
apiVersion: linkerd.io/v1alpha2
kind: ServiceProfile
metadata:
  name: backend.default.svc.cluster.local
spec:
  routes:
    - name: GET /api/data
      condition:
        method: GET
        pathRegex: /api/data
      # 幂等请求自动重试
      isRetryable: true
      timeout: 10s
```

### 5. 流量分割 (金丝雀发布)

```yaml
# TrafficSplit 资源
apiVersion: split.smi-spec.io/v1alpha2
kind: TrafficSplit
metadata:
  name: web-split
  namespace: default
spec:
  service: web
  backends:
    - service: web-stable
      weight: 900  # 90%
    - service: web-canary
      weight: 100  # 10%
```

---

## 多集群

```bash
# 集群 1: 安装 multicluster 扩展
linkerd multicluster install | kubectl apply -f -

# 集群 1: 导出 Link 配置
linkerd multicluster link --cluster-name cluster1 > link.yaml

# 集群 2: 应用 Link
kubectl apply -f link.yaml

# 验证连接
linkerd multicluster check

# 镜像服务
kubectl label svc/web mirror.linkerd.io/exported=true
```

```
┌─────────────────────────────────────────────────────────────────┐
│                    Linkerd 多集群                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│   Cluster 1                            Cluster 2                │
│   ┌─────────────────┐                  ┌─────────────────┐      │
│   │ web-cluster1    │                  │ web (mirror)    │      │
│   │ (源服务)        │ ◄── mTLS ───────►│                 │      │
│   │                 │    Gateway       │ api-cluster2    │      │
│   │ api (mirror)    │                  │ (源服务)        │      │
│   └─────────────────┘                  └─────────────────┘      │
│                                                                  │
│   特点:                                                          │
│   • Pod 到 Pod mTLS                                              │
│   • 无需 VPN 或专用网络                                          │
│   • 自动服务镜像                                                 │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 扩展功能

### Linkerd Viz (可视化)

```bash
# 安装
linkerd viz install | kubectl apply -f -

# Dashboard
linkerd viz dashboard

# 命令行工具
linkerd viz top deploy/web
linkerd viz tap ns/default
linkerd viz stat deploy -n default
```

### Linkerd Jaeger (分布式追踪)

```bash
# 安装 Jaeger 扩展
linkerd jaeger install | kubectl apply -f -

# 启用追踪采样
kubectl annotate namespace default \
  config.linkerd.io/trace-collector=collector.linkerd-jaeger:55678

# 访问 Jaeger UI
kubectl -n linkerd-jaeger port-forward svc/jaeger 16686:16686
```

---

## 生产最佳实践

```yaml
# 推荐配置
apiVersion: v1
kind: Namespace
metadata:
  name: my-app
  annotations:
    # 自动注入
    linkerd.io/inject: enabled
    # 配置代理资源
    config.linkerd.io/proxy-cpu-request: "100m"
    config.linkerd.io/proxy-memory-request: "64Mi"
    config.linkerd.io/proxy-cpu-limit: "1"
    config.linkerd.io/proxy-memory-limit: "256Mi"
    # 启用协议检测超时
    config.linkerd.io/skip-outbound-ports: "25,587"  # SMTP
```

---

## 参考资源

- [官方文档](https://linkerd.io/docs)
- [GitHub Repo](https://github.com/linkerd/linkerd2)
- [CNCF 项目页面](https://www.cncf.io/projects/linkerd/)
- [Linkerd 博客](https://linkerd.io/blog/)
- [Buoyant 企业版](https://buoyant.io/)

---

**维护者**: Kudig Team | **许可证**: MIT
