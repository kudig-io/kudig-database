---
title: Cilium
description: Cilium — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- ebpf
- cni
- networking
- security
- cilium
- envoy
- kafka
- networkpolicy
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium 是什么
- 如何 Cilium
trigger_keywords:
- Cilium
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- kafka-basics
---

# Cilium

Cilium is an eBPF-based networking, security, and observability platform for Kubernetes, graduated from CNCF in 2023.

## Key Facts

- **Status**: CNCF graduated (2023)
- **Technology**: eBPF programs in Linux kernel
- **Kernel Requirement**: Linux 5.10+ (5.15+ for BTF, 6.1+ for advanced features)
- **Components**: Cilium Agent, Cilium Operator, CNI Plugin, Hubble Relay

## Capabilities

| Capability | Description |
|------------|-------------|
| CNI | Pod networking, IPAM, Kubernetes Service routing |
| NetworkPolicy | L3/L4 policies + L7 HTTP/gRPC/Kafka policies |
| Service Mesh | Sidecar-less mesh via eBPF + optional Envoy for L7 |
| Load Balancing | eBPF-based kube-proxy replacement (Maglev, ECMP) |
| Encryption | WireGuard or IPSec for Pod-to-Pod encryption |
| Observability | Hubble for L3/L4/L7 flow visualization |

## Hubble Integration

Hubble provides network flow observability:
- **Hubble Relay**: Aggregates flow data from all Cilium agents
- **Hubble CLI**: Command-line flow analysis
- **Hubble UI**: Web-based service dependency map

## kube-proxy Replacement

Cilium can replace kube-proxy entirely using eBPF for Service load balancing. Benefits: higher throughput, lower latency, no iptables/IPVS rules to manage.

## Related
- [[synthesis/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合

- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[concepts/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]
- [[entities/tetragon.md|Tetragon]]
- Hubble

- [[synthesis/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- [[domain-19-landscape-references/18-kubernetes-ebpf-cilium-deep-practice.md|18-kubernetes-ebpf-cilium-deep-practice]]
- [[domain-03-networking-traffic/03-cilium-cni-architecture.md|03-cilium-cni-architecture]]
- [[domain-03-networking-traffic/99-cilium-ebpf-network-guide.md|99-cilium-ebpf-network-guide]]
- [[domain-03-networking-traffic/05-cilium-service-mesh.md|05-cilium-service-mesh]]
- [[domain-03-networking-traffic/04-cilium-network-policy.md|04-cilium-network-policy]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/cilium-fta.md|cilium-fta]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.9.md|RELEASE-NOTES-1.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.18.md|RELEASE-NOTES-1.18]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.19.md|RELEASE-NOTES-1.19]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.16.md|RELEASE-NOTES-1.16]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.12.md|RELEASE-NOTES-1.12]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.13.md|RELEASE-NOTES-1.13]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.17.md|RELEASE-NOTES-1.17]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.10.md|RELEASE-NOTES-1.10]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.14.md|RELEASE-NOTES-1.14]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.15.md|RELEASE-NOTES-1.15]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.11.md|RELEASE-NOTES-1.11]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.10.md|RELEASE-NOTES-0.10]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[references/k8s-networking-ecosystem|网络体系：CNI、Service、Ingress、Gateway API 与多集群网络]] — Cross-reference
- [[references/k8s-difficulty-index|Kubernetes Difficulty Index]] — Cross-reference
- [[references/k8s-networking-domain-guide|Kubernetes Networking Domain Guide]] — Cross-reference
- [[synthesis/eBPF x 运行时安全|eBPF x 运行时安全]] — Cross-reference
- [[concepts/service-mesh-evolution|服务网格演进]] — Cross-reference
- [[concepts/cni-networking-model|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/Kubernetes Core Concepts|Kubernetes Core Concepts]] — Cross-reference
- [[concepts/tcp-udp-protocol-stack|TCP/UDP Protocol Stack]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/networkpolicy-fta|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/service-mesh-index|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
