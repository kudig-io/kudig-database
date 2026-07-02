---
title: Service Networking
description: '- [[concepts/cni-networking-model.md|cni-networking-model]] — CNI 网络模型与插件对比'
summary: '- [[concepts/cni-networking-model.md|cni-networking-model]] — CNI 网络模型与插件对比'
category: concepts
tags:
- k8s
- networking
- service
- kube-proxy
- load-balancing
- dns
- cilium
- coredns
- ingress
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Service Networking 是什么
- 如何 Service Networking
trigger_keywords:
- Service
- Networking
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Service|Service]] Networking

## Service Types

| Type | Purpose | Use Case |
|------|---------|----------|
| **ClusterIP** | Internal virtual IP | Default; microservice-to-microservice communication |
| **NodePort** | Expose on each node's IP:port | External access without cloud load balancer |
| **LoadBalancer** | Cloud provider LB integration | Production external traffic (SLB/ALB/NLB) |
| **ExternalName** | DNS CNAME to external name | Access external services by DNS alias |

## Service Discovery

Clients discover services via DNS:
- **FQDN**: `my-svc.my-ns.svc.cluster.local`
- **CoreDNS** resolves to ClusterIP
- **kube-proxy** routes ClusterIP to backend [[Pods|Pods]]

## Load Balancing Modes

| Mode | Latency | Throughput | Service Scale | Recommended |
|------|---------|------------|---------------|-------------|
| **iptables** | High | Low | <1000 Services | Small clusters |
| **IPVS** | Medium | High | >1000 Services | Production clusters |
| **eBPF (Cilium)** | Lowest | Highest | Unlimited | High-performance, modern kernels |

## EndpointSlice

Since Kubernetes v1.21, EndpointSlice replaces Endpoints as the scalable way to track Service backends. EndpointSlice supports:
- Up to 100 endpoints per slice (vs 1000 in Endpoints)
- Multiple address types (IPv4, IPv6, FQDN)
- Topology-aware routing

## [[Ingress|Ingress]] and Gateway API

- **Ingress**: L7 HTTP/HTTPS routing with TLS termination (nginx, ALB, etc.)
- **Gateway API**: Next-generation successor to Ingress with richer routing, multi-tenant support, and standardized resource types (HTTPRoute, TCPRoute, etc.)

## Related

- [[concepts/cni-networking-model.md|cni-networking-model]] — CNI 网络模型与插件对比
- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[entities/cni-plugins.md|CNI Plugins]]
- [[coredns|CoreDNS]]
- Kubernetes Network Model
- Ingress Controller

- 10-service-networking-events

<!-- risk-assessed -->
