---
title: Kuma
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- jaeger
- envoy
- helm
- ingress
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kuma 是什么
- 如何 Kuma
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kuma
- cncf
- landscape
---


# Kuma

> **成熟度**: Sandbox | **加入时间**: 2020-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kuma.io |
| **GitHub** | https://github.com/kumahq/kuma |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Service Mesh |
| **维护组织** | Kong Inc. |

---

## 项目概述

Kuma 是一个通用服务网格控制平面，设计简单易用且功能强大。它基于 Envoy 代理构建，支持 Kubernetes 和虚拟机环境，可通过单一控制平面管理多个服务网格部署。Kuma 提供开箱即用的策略，帮助团队快速实现零信任安全、可观测性和流量管理。

---

## 核心特性

- **多平台支持**: 同时支持 Kubernetes 和 VM/裸金属环境
- **多区域部署**: 单一控制平面管理多个集群/区域
- **零信任安全**: 自动 mTLS、访问策略、流量权限
- **可观测性**: 集成 Prometheus、Jaeger、Datadog
- **流量管理**: 负载均衡、熔断、重试、超时
- **Gateway 集成**: 内置 Kong Gateway 支持
- **策略即代码**: 声明式 YAML 配置策略

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Kuma Architecture                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Global Control Plane                    │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │               kuma-cp (Global)                       │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   Policies  │  │   Service   │  │   Config   │  │ │   │
│  │  │  │   Manager   │  │   Registry  │  │   Store    │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────┬───────────────────────────────┘   │
│                            │                                    │
│            ┌───────────────┼───────────────┐                   │
│            │               │               │                    │
│  ┌─────────▼─────────┐ ┌───▼────────┐ ┌────▼────────────┐     │
│  │      Zone 1       │ │   Zone 2   │ │     Zone 3      │     │
│  │   (Kubernetes)    │ │   (K8s)    │ │   (Universal)   │     │
│  │                   │ │            │ │                 │     │
│  │ ┌───────────────┐ │ │ ┌────────┐ │ │ ┌─────────────┐ │     │
│  │ │  kuma-cp      │ │ │ │kuma-cp │ │ │ │   kuma-cp   │ │     │
│  │ │  (Zone)       │ │ │ │(Zone)  │ │ │ │   (Zone)    │ │     │
│  │ └───────┬───────┘ │ │ └───┬────┘ │ │ └──────┬──────┘ │     │
│  │         │         │ │     │      │ │        │        │     │
│  │ ┌───────▼───────┐ │ │ ┌───▼────┐ │ │ ┌──────▼──────┐ │     │
│  │ │ Data Plane    │ │ │ │  DPs   │ │ │ │    VMs      │ │     │
│  │ │ Proxies       │ │ │ │        │ │ │ │ kuma-dp     │ │     │
│  │ │ ┌────┐ ┌────┐ │ │ │ │┌────┐  │ │ │ │ ┌────────┐  │ │     │
│  │ │ │DP 1│ │DP 2│ │ │ │ ││DP  │  │ │ │ │ │ Envoy  │  │ │     │
│  │ │ │    │ │    │ │ │ │ │└────┘  │ │ │ │ └────────┘  │ │     │
│  │ │ └────┘ └────┘ │ │ │ └────────┘ │ │ └─────────────┘ │     │
│  │ │  Envoy Sidecars│ │ │           │ │                 │     │
│  │ └───────────────┘ │ │            │ │                 │     │
│  │ ┌───────────────┐ │ │            │ │                 │     │
│  │ │ Service A     │ │ │            │ │                 │     │
│  │ │ Service B     │ │ │            │ │                 │     │
│  │ └───────────────┘ │ │            │ │                 │     │
│  └───────────────────┘ └────────────┘ └─────────────────┘     │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Built-in Policies                      │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────┐    │   │
│  │  │ mTLS    │ │Traffic  │ │  Rate   │ │   Circuit   │    │   │
│  │  │         │ │Permission│ │  Limit  │ │   Breaker   │    │   │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────────┘    │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────┐    │   │
│  │  │  Retry  │ │ Timeout │ │ Health  │ │   Traffic   │    │   │
│  │  │         │ │         │ │  Check  │ │    Route    │    │   │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────────┘    │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Global Control Plane** | 全局控制平面，管理多区域配置 |
| **Zone Control Plane** | 区域控制平面，管理本地数据平面 |
| **Data Plane (Envoy)** | Envoy 代理，处理服务间流量 |
| **Policies** | 声明式策略，定义网格行为 |
| **Universal Mode** | VM/裸金属环境部署模式 |

---

## 快速开始

### Kubernetes 安装

```bash
# 使用 kumactl 安装
curl -L https://kuma.io/installer.sh | VERSION=2.5.0 sh -
cd kuma-2.5.0/bin
export PATH=$PWD:$PATH

# 安装 Kuma 控制平面
kumactl install control-plane | kubectl apply -f -

# 验证安装
kubectl get pods -n kuma-system
kumactl inspect meshes

# 访问 GUI
kubectl port-forward svc/kuma-control-plane -n kuma-system 5681:5681
# 打开 http://localhost:5681/gui
```

### Helm 安装

```bash
helm repo add kuma https://kumahq.github.io/charts
helm repo update

helm install kuma kuma/kuma \
  --namespace kuma-system \
  --create-namespace \
  --set controlPlane.mode=zone
```

---

## 启用 Sidecar 注入

### 命名空间注入

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: my-app
  labels:
    kuma.io/sidecar-injection: enabled
```

### Pod 注解

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: my-pod
  annotations:
    kuma.io/sidecar-injection: enabled
spec:
  containers:
    - name: app
      image: my-app:latest
```

---

## 策略配置

### mTLS 策略

```yaml
apiVersion: kuma.io/v1alpha1
kind: Mesh
metadata:
  name: default
spec:
  mtls:
    enabledBackend: ca-1
    backends:
      - name: ca-1
        type: builtin
        dpCert:
          rotation:
            expiration: 1d
        conf:
          caCert:
            RSAbits: 2048
            expiration: 10y
```

### 流量权限

```yaml
apiVersion: kuma.io/v1alpha1
kind: MeshTrafficPermission
metadata:
  namespace: kuma-system
  name: allow-frontend-to-backend
spec:
  targetRef:
    kind: MeshService
    name: backend
  from:
    - targetRef:
        kind: MeshService
        name: frontend
      default:
        action: Allow
```

### 重试策略

```yaml
apiVersion: kuma.io/v1alpha1
kind: MeshRetry
metadata:
  namespace: kuma-system
  name: retry-api
spec:
  targetRef:
    kind: MeshService
    name: api-service
  to:
    - targetRef:
        kind: MeshService
        name: database
      default:
        tcp:
          maxConnectAttempt: 5
        http:
          numRetries: 3
          perTryTimeout: 3s
          backOff:
            baseInterval: 100ms
            maxInterval: 10s
```

### 熔断策略

```yaml
apiVersion: kuma.io/v1alpha1
kind: MeshCircuitBreaker
metadata:
  namespace: kuma-system
  name: circuit-breaker-db
spec:
  targetRef:
    kind: MeshService
    name: database
  to:
    - targetRef:
        kind: MeshService
        name: backend
      default:
        connectionLimits:
          maxConnections: 100
          maxPendingRequests: 100
          maxRetries: 3
          maxRequests: 1000
```

### 流量路由

```yaml
apiVersion: kuma.io/v1alpha1
kind: MeshHTTPRoute
metadata:
  namespace: kuma-system
  name: canary-route
spec:
  targetRef:
    kind: MeshService
    name: frontend
  to:
    - targetRef:
        kind: MeshService
        name: backend
      rules:
        - matches:
            - headers:
                - type: Exact
                  name: x-version
                  value: canary
          default:
            backendRefs:
              - kind: MeshService
                name: backend-canary
                weight: 100
        - default:
            backendRefs:
              - kind: MeshService
                name: backend
                weight: 90
              - kind: MeshService
                name: backend-canary
                weight: 10
```

---

## 多区域部署

### 全局控制平面

```bash
# 部署全局控制平面
kumactl install control-plane \
  --mode=global | kubectl apply -f -

# 生成区域加入令牌
kumactl generate zone-token --zone=zone-1 --valid-for=720h > zone-token
```

### 区域控制平面

```bash
# 部署区域控制平面
kumactl install control-plane \
  --mode=zone \
  --zone=zone-1 \
  --ingress-enabled \
  --kds-global-address grpcs://global-cp.example.com:5685 | kubectl apply -f -

# 验证多区域连接
kumactl inspect zones
```

---

## 可观测性

### Prometheus 集成

```yaml
apiVersion: kuma.io/v1alpha1
kind: Mesh
metadata:
  name: default
spec:
  metrics:
    enabledBackend: prometheus-1
    backends:
      - name: prometheus-1
        type: prometheus
        conf:
          skipMTLS: false
          port: 5670
          path: /metrics
          tags:
            kuma.io/service: dataplane-metrics
```

### Jaeger 追踪

```yaml
apiVersion: kuma.io/v1alpha1
kind: Mesh
metadata:
  name: default
spec:
  tracing:
    defaultBackend: jaeger-1
    backends:
      - name: jaeger-1
        type: zipkin
        conf:
          url: http://jaeger-collector.observability:9411/api/v2/spans
          traceId128bit: true
          apiVersion: httpJson
```

---

## 最佳实践

1. **渐进式启用**: 先在非生产环境测试策略
2. **mTLS 优先**: 生产环境始终启用 mTLS
3. **多区域规划**: 合理划分区域，减少跨区流量
4. **监控覆盖**: 配置完整的可观测性堆栈
5. **策略精细化**: 从宽松策略开始，逐步收紧
6. **版本管理**: 使用 GitOps 管理策略版本

---

## 参考资源

- [官方文档](https://kuma.io/docs)
- [GitHub Repo](https://github.com/kumahq/kuma)
- [策略参考](https://kuma.io/docs/latest/policies/)
- [多区域部署](https://kuma.io/docs/latest/production/deployment/multi-zone/)
- [Kong 集成](https://kuma.io/docs/latest/explore/gateway/)

---

**维护者**: Kudig Team | **许可证**: MIT
