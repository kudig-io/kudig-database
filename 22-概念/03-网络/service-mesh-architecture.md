---
title: Service Mesh Architecture
description: '- [[22-概念/11-交叉分析/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis'
summary: '- [[22-概念/11-交叉分析/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis'
category: concepts
tags:
- k8s
- service-mesh
- istio
- envoy
- mtls
- microservices
- prometheus
- grafana
- jaeger
- cilium
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Service Mesh Architecture 是什么
- 如何 Service Mesh Architecture
trigger_keywords:
- Service
- Mesh
- Architecture
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- logging-basics
- tracing-basics
- observability-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Service|Service]]Service Mesh）|Service Mesh]] Architecture

## What is a Service Mesh

A service mesh is an infrastructure layer that handles service-to-service communication through transparent proxies. It moves networking logic (mTLS, retries, timeouts, traffic splitting, observability) out of application code and into the infrastructure layer.

## Architecture Modes

| Mode | How It Works | Resource Overhead | Key Products |
|------|-------------|-------------------|-------------|
| Sidecar | Proxy container injected into every Pod | ~100MB/Pod (Envoy), ~20MB/Pod (Rust) | Istio Sidecar, Linkerd |
| Ambient (Sidecar-less) | Node-level L4 proxy (ztunnel) + per-service L7 proxy (Waypoint) | ~50MB/node + waypoint per service | Istio Ambient (GA v1.29) |
| eBPF (Kernel) | Network rules in Linux kernel via eBPF programs | ~10MB, near-zero latency | Cilium Service Mesh |
| Per-node Agent | One proxy DaemonSet per node | ~256MB/node | Traefik Mesh |

## Major Platforms Comparison

| Feature | Istio | Linkerd | Consul Connect | Cilium Mesh | Dapr |
|---------|-------|---------|---------------|-------------|------|
| Auto mTLS | Yes | Yes | Yes | Yes | Yes |
| L7 Traffic Routing | Yes | Limited | Yes | Limited | No |
| Canary Release | Yes | Yes (SMI) | Yes | Yes | No |
| Fault Injection | Yes | Yes | No | No | No |
| Traffic Mirroring | Yes | No | No | No | No |
| WASM Extension | Yes | No | No | No | No |
| Multi-cluster | Yes | Yes | Yes | Yes | No |
| VM Support | Yes | No | Yes | No | Yes |
| Gateway API | Yes | No | No | Yes | No |
| Sidecar-less Mode | Ambient | No | No | eBPF | No |

## Performance Comparison

| Metric | Istio Sidecar | Istio Ambient L4 | Linkerd | Cilium eBPF |
|--------|--------------|------------------|---------|-------------|
| Proxy Memory/Pod | ~100MB | ~50MB/node | ~20MB | ~10MB |
| P50 Latency Overhead | +1.8ms | +0.3ms | +0.3ms | +0.1ms |
| P99 Latency Overhead | +4.2ms | +0.8ms | +0.7ms | +0.3ms |
| mTLS Performance Cost | ~5% | <1% | <1% | <1% |

## Core Capabilities

**Traffic Management**: VirtualService and DestinationRule (Istio) or TrafficSplit (Linkerd/SMI) control routing, weight splitting, retries, timeouts, and fault injection. Enables canary, blue-green, and A/B deployments.

**Security**: Automatic mTLS encrypts all service-to-service traffic using SPIFFE/SPIRE identity framework. AuthorizationPolicy provides L7 access control (HTTP method, path, namespace, identity). Certificate rotation happens automatically (default 24h TTL).

**Observability**: Data plane proxies automatically export golden metrics (latency, traffic, errors, saturation) to Prometheus. Distributed tracing via OpenTelemetry to Jaeger or Grafana Tempo. Access logs collected by Loki.

**Resilience Patterns**: Circuit breaker (Outlier Detection), retry with backoff, layered timeouts, bulkhead isolation (connection pools), and rate limiting at gateway/mesh/application layers.

## Selection Guidelines

- Services < 10: No mesh needed, K8s Service + Ingress + NetworkPolicy is sufficient
- Services 10-50: Consider Linkerd for lightweight deployment
- Services > 50: Choose Istio (full-featured) or Linkerd (simpler)
- Performance-critical: Cilium eBPF mesh
- Multi-cluster: Istio (most mature)
- Small team: Linkerd (easiest to operate)

## 源码实现分析

### Istio Sidecar 注入机制

```go
// istio/pilot/pkg/kube/inject/webhook.go
func (wh *Webhook) inject(pod *v1.Pod, namespace string) (*v1.Pod, error) {
    // 1. MutatingWebhookConfiguration 拦截 Pod 创建请求
    // 2. 检查 namespace 是否有 istio-injection=enabled 标签
    // 3. 注入 Envoy sidecar 容器
    pod.Spec.Containers = append(pod.Spec.Containers, v1.Container{
        Name:  "istio-proxy",
        Image: "docker.io/istio/proxyv2:" + version,
        Args:  []string{"proxy", "sidecar"},
        Env: []v1.EnvVar{
            {Name: "ISTIO_META_POD_NAME", Value: pod.Name},
            {Name: "ISTIO_META_MESH_ID", Value: meshID},
        },
    })
    // 4. 注入 initContainer (istio-init) 配置 iptables 重定向
    // 所有入站/出站流量 → 15001/15006 → Envoy
    return pod, nil
}
```

### 流量拦截链路

```
Client Pod (App Container)
    │ localhost:8080
    ▼
Envoy Sidecar (outbound)
    │ 1. mTLS 加密
    │ 2. 路由规则 (VirtualService)
    │ 3. 重试/超时/熔断
    ▼
Network (mTLS encrypted)
    │
    ▼
Server Pod Envoy Sidecar (inbound)
    │ 1. mTLS 解密 + 身份验证
    │ 2. AuthorizationPolicy 检查
    │ 3. 指标采集 (Prometheus)
    ▼
Server App Container (localhost:8080)
```

## 使用场景

### 场景一：金丝雀发布（流量分割）

```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: web-routes
spec:
  hosts: [web.default.svc.cluster.local]
  http:
  - route:
    - destination:
        host: web.default.svc.cluster.local
        subset: v1
      weight: 90          # 90% 到稳定版
    - destination:
        host: web.default.svc.cluster.local
        subset: v2
      weight: 10          # 10% 到新版本
---
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: web-subsets
spec:
  host: web.default.svc.cluster.local
  subsets:
  - name: v1
    labels:
      version: v1
  - name: v2
    labels:
      version: v2
```

### 场景二：服务间授权

```yaml
# 仅允许 frontend 访问 backend 的 GET /api/*
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: backend-policy
spec:
  selector:
    matchLabels:
      app: backend
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/default/sa/frontend"]
    to:
    - operation:
        methods: ["GET"]
        paths: ["/api/*"]
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 所有服务都需要 Mesh | <10 个服务用 K8s 原生即可，Mesh 增加复杂度 |
| Mesh 零性能开销 | 每跳增加 0.3-2ms 延迟 + ~50-100MB 内存/Pod |
| Mesh 替代 NetworkPolicy | Mesh 做 L7 授权，NetworkPolicy 做 L3/L4 隔离，互补 |
| 安装 Mesh 即自动 mTLS | 需配置 PeerAuthentication 启用 STRICT mTLS |
| Sidecar 注入无侵入 | 修改 Pod spec、增加资源消耗、影响启动时间 |
| Mesh 解决所有网络问题 | DNS、内核参数、网络策略等基础问题 Mesh 无法解决 |

## 面试要点

1. **Service Mesh 的核心价值？** — 将网络治理（重试/超时/熔断/路由）从应用代码下沉到基础设施层。三大能力：流量管理（金丝雀/蓝绿）、安全（mTLS+授权）、可观测性（自动指标/追踪）。应用无感知。

2. **Istio 与 Linkerd 如何选择？** — Istio：功能全面、多集群成熟、生态丰富，但复杂度高、资源消耗大；Linkerd：轻量（Rust proxy）、简单、低延迟，但功能较少。小团队选 Linkerd，大企业选 Istio。

3. **Sidecar 模式的优缺点？** — 优点：语言无关、应用无侵入、统一策略；缺点：每 Pod 额外资源消耗、增加延迟、调试复杂度增加。替代方案：Sidecarless（Ambient Mesh）、eBPF（Cilium）。

4. **mTLS 如何工作？** — 每个服务获得 SPIFFE 身份（基于 ServiceAccount）；证书由 Istiod CA 签发（默认 24h TTL 自动轮换）；Envoy 自动加密所有服务间通信；零信任：即使内网也加密+验证身份。

## Related

- [[22-概念/02-工作负载/deployment-controller-architecture.md|deployment-controller-architecture]]

- [[22-概念/03-网络/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[22-概念/05-安全/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[istio]] — Istio
- [[envoy]] — Envoy
- [[22-概念/03-网络/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]]
- [[22-概念/05-安全/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[istio|Istio]]
- [[linkerd|Linkerd]]
- [[envoy|Envoy Proxy]]
- [[22-概念/11-交叉分析/eBPF × 运行时安全.md|eBPF x 运行时安全]] — synthesis
- [[22-概念/11-交叉分析/服务网格 × 零信任安全.md|服务网格 x 零信任安全]] — synthesis

- 14-service-mesh-architecture

<!-- risk-assessed -->
