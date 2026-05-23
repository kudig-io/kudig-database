---
title: CNI Plugins
description: CNI Plugins — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- cni
- networking
- calico
- cilium
- flannel
- terway
- kubelet
- networkpolicy
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNI Plugins 是什么
- 如何 CNI Plugins
trigger_keywords:
- CNI
- Plugins
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- cni-basics
created: "2026-05-23"
---

# CNI Plugins

## What is CNI

CNI (Container Network Interface) is the standard plugin interface Kubernetes uses to configure Pod networking. CNI plugins are invoked by [[kubelet|kubelet]] during Pod creation to set up network namespaces, assign IP addresses, and configure routes.

## Major CNI Plugins

| Plugin | Type | Features | Best For |
|--------|------|----------|----------|
| **Calico** | BGP routing | [[NetworkPolicy|NetworkPolicy]] enforcement, BGP peering, IPIP/VXLAN overlay | Enterprise, NetworkPolicy-heavy |
| **Cilium** | eBPF-based | L7 policy, identity-aware security, observability, service mesh replacement | High-performance, security-focused |
| **Flannel** | Overlay (VXLAN/UDP/WireGuard) | Simple, minimal overhead, dual-stack, WireGuard encryption | Small clusters, simplicity |
| **Terway** | Alibaba Cloud ENI | Direct ENI IP allocation, high throughput, VPC-native | Alibaba Cloud environments |

## CNI Requirements

Every CNI plugin must satisfy:
- Each Pod gets a unique IP address
- Pods on the same node can communicate without NAT
- Pods on different nodes can communicate without NAT (cluster-wide flat network)
- No special port mapping needed

## IPAM (IP Address Management)

CNI plugins handle IP allocation through IPAM plugins:
- **host-local**: Node-scoped IP range allocation
- **DHCP**: External DHCP server
- **Static**: Fixed IP assignment
- **Cloud provider**: VPC subnet allocation (Terway)

## Selection Criteria

Choose based on:
- **Scale**: Flannel for small, Calico/Cilium for large
- **Security**: Cilium for eBPF-based L7 policies, Calico for standard NetworkPolicy
- **Cloud integration**: Terway for Alibaba Cloud, AWS VPC CNI for AWS
- **Performance**: Cilium eBPF > Calico BGP > Flannel VXLAN

## Related
- [[synthesis/CNI 插件 × NetworkPolicy|CNI 插件 × NetworkPolicy]] — 综合

- [[cilium]] — Cilium
- [[entities/kubelet|kubelet]] — kubelet
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/service-networking|service-networking]] — Service Networking
- [[concepts/service-networking|Service Networking]]
- [[entities/networkpolicy|NetworkPolicy]]
- Kubernetes Network Model

- 03-cni-plugins-comparison
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- RELEASE-NOTES-0.7
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4