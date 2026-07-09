---
title: Kubernetes Networking Domain Guide
description: Kubernetes Networking Domain Guide — Kubernetes 生产运维知识库
summary: Kubernetes Networking Domain Guide — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- networking
- 网络
- service
- cni
- ingress
- dns
- reference
- cilium
- flannel
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Networking Domain Guide 是什么
- 如何 Kubernetes Networking Domain Guide
trigger_keywords:
- Kubernetes
- Networking
- Domain
- Guide
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Networking Domain Guide

## Source

Distilled from 网络 (39 documents, Kubernetes v1.28-v1.32).

## Networking Model

1. **Pod-to-Pod**: Every Pod gets unique IP; pods communicate without NAT across nodes
2. **Service**: Stable virtual IP (ClusterIP) with DNS name, load balances to Pod endpoints
3. **Ingress**: L7 HTTP/HTTPS routing with TLS termination
4. **NetworkPolicy**: Pod-level firewall for ingress/egress traffic

## CNI Plugins

| Plugin | Type | Features |
|--------|------|----------|
| **Calico** | BGP | NetworkPolicy, BGP peering, IPIP/VXLAN |
| **Cilium** | eBPF | L7 policies, identity-aware, observability |
| **Flannel** | Overlay | Simple, minimal, WireGuard encryption |
| **Terway** | ENI | Alibaba Cloud native, high throughput |

## Service Types

| Type | Scope | Use |
|------|-------|-----|
| ClusterIP | Internal | Default microservice communication |
| NodePort | Node IP:port | External access without cloud LB |
| LoadBalancer | Cloud LB | Production external traffic |
| ExternalName | DNS CNAME | External service alias |

## kube-proxy Modes

| Mode | Performance | Scale |
|------|------------|-------|
| iptables | Low latency overhead, linear rule growth | <1000 Services |
| IPVS | High throughput, kernel-level LB | >1000 Services |
| eBPF | Lowest latency, bypasses TCP/IP stack | Unlimited |

## Ingress vs Gateway API

- **Ingress**: Mature, widely adopted, HTTP/HTTPS routing
- **Gateway API**: Next-generation, multi-protocol (HTTP, TCP, UDP, gRPC), multi-tenant, role-separated

## Related

- [[reference|#reference Hub]] — tag hub

- [[cilium]] — Cilium
- [[grpc]] — gRPC
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/service-networking.md|service-networking]] — Service Networking
- [[concepts/service-networking.md|Service Networking]]
- [[entities/cni-plugins.md|CNI Plugins]]
- [[entities/networkpolicy.md|NetworkPolicy]]


<!-- risk-assessed -->
