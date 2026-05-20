---
title: Contour
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- envoy
- ingress
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
- Contour 是什么
- 如何 Contour
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Contour
- cncf
- landscape
---

# Contour

> **成熟度**: Incubating | **加入时间**: 2018-07 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://projectcontour.io |
| **GitHub** | https://github.com/projectcontour/contour |
| **文档** | https://projectcontour.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Networking |

---

## 项目概述

### 简介
Contour 是基于 Envoy 的 Kubernetes Ingress 控制器。它提供 HTTPProxy CRD 扩展原生 Ingress 能力，支持高级流量管理功能。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | Heptio 创建 |
| 2018-07 | 加入 CNCF Sandbox |
| 2020-07 | 晋升为 CNCF Incubating |

### 核心定位
Contour 是 Kubernetes Ingress 的高级实现，专注于安全、可靠的 HTTP 代理，被 VMware Tanzu 等产品采用。

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Contour 架构                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Control Plane                             ││
│  │  ┌─────────────────────────────────────────────────────┐    ││
│  │  │                    Contour                           │    ││
│  │  │  • 监听 Ingress/HTTPProxy                            │    ││
│  │  │  • 生成 Envoy xDS 配置                               │    ││
│  │  │  • 通过 gRPC 推送到 Envoy                            │    ││
│  │  └─────────────────────────────────────────────────────┘    ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │ xDS (gRPC)                        │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Data Plane                                ││
│  │  ┌─────────────────────────────────────────────────────┐    ││
│  │  │                    Envoy                             │    ││
│  │  │  • L7 代理                                           │    ││
│  │  │  • TLS 终止                                          │    ││
│  │  │  • 流量路由                                          │    ││
│  │  └─────────────────────────────────────────────────────┘    ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│                              ▼                                   │
│                    ┌─────────────────┐                          │
│                    │  Backend Pods   │                          │
│                    └─────────────────┘                          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## HTTPProxy 示例

```yaml
# HTTPProxy CRD (比 Ingress 功能更丰富)
apiVersion: projectcontour.io/v1
kind: HTTPProxy
metadata:
  name: my-app
spec:
  virtualhost:
    fqdn: app.example.com
    tls:
      secretName: app-tls
  routes:
    - conditions:
        - prefix: /api
      services:
        - name: api-service
          port: 8080
          weight: 90
        - name: api-service-canary
          port: 8080
          weight: 10
      retryPolicy:
        count: 3
        perTryTimeout: 5s
      timeoutPolicy:
        response: 30s
    - conditions:
        - prefix: /
      services:
        - name: frontend
          port: 80
```

---

## 安装

```bash
kubectl apply -f https://projectcontour.io/quickstart/contour.yaml
```

---

## Contour vs 其他 Ingress

| 特性 | Contour | NGINX | Traefik |
|:---|:---|:---|:---|
| **数据平面** | Envoy | NGINX | 自研 |
| **配置方式** | HTTPProxy CRD | ConfigMap | CRD |
| **流量分割** | 原生支持 | 需 Annotation | 原生支持 |
| **TLS 委托** | 支持 | 不支持 | 支持 |

---

## 参考资源

- [官方文档](https://projectcontour.io/docs)
- [GitHub Repo](https://github.com/projectcontour/contour)
- [CNCF 项目页面](https://www.cncf.io/projects/contour/)

---

**维护者**: Kudig Team | **许可证**: MIT
