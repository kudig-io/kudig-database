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
---

# CNI Plugins

## What is CNI

CNI (Container Network Interface) is the standard plugin interface Kubernetes uses to configure Pod networking. CNI plugins are invoked by kubelet during Pod creation to set up network namespaces, assign IP addresses, and configure routes.

## Major CNI Plugins

| Plugin | Type | Features | Best For |
|--------|------|----------|----------|
| **Calico** | BGP routing | NetworkPolicy enforcement, BGP peering, IPIP/VXLAN overlay | Enterprise, NetworkPolicy-heavy |
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
- [[entities/kubelet.md|kubelet]] — kubelet
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/service-networking.md|service-networking]] — Service Networking
- [[concepts/service-networking.md|Service Networking]]
- [[entities/networkpolicy.md|NetworkPolicy]]
- Kubernetes Network Model

- [[domain-03-networking-traffic/03-cni-plugins-comparison.md|03-cni-plugins-comparison]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.9.md|RELEASE-NOTES-1.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]