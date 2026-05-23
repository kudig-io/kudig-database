---
title: Service Mesh Architecture
description: '- [[synthesis/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis'
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
created: "2026-05-23"
---

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

## Related

- [[concepts/deployment-controller-architecture.md|deployment-controller-architecture]]

- [[concepts/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[istio]] — Istio
- [[envoy]] — Envoy
- [[concepts/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]]
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[concepts/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[istio|Istio]]
- [[linkerd|Linkerd]]
- [[envoy|Envoy Proxy]]
- [[synthesis/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis
- [[synthesis/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — synthesis

- 14-service-mesh-architecture