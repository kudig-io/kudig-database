---
title: Cilium (entities)
description: Cilium — Kubernetes 生产运维知识库
summary: Cilium — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
| CNI | Pod networking, IPAM, Kubernetes [[Service|Service]] routing |
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
- [[concepts/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合

- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[concepts/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]
- [[entities/tetragon.md|Tetragon]]
- Hubble

- [[concepts/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- 18-kubernetes-ebpf-cilium-deep-practice
- 03-cilium-cni-architecture
- 99-cilium-ebpf-network-guide
- 05-cilium-service-mesh
- 04-cilium-network-policy
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/cilium-fta.md|cilium-fta]]
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.11
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[entities/k8s-networking-ecosystem.md|网络体系：CNI、Service、Ingress、Gateway API 与多集群网络]] — Cross-reference
- [[entities/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- [[entities/k8s-networking-domain-guide.md|Kubernetes Networking Domain Guide]] — Cross-reference
- [[concepts/eBPF x 运行时安全.md|eBPF x 运行时安全]] — Cross-reference
- [[concepts/service-mesh-evolution.md|服务网格演进]] — Cross-reference
- [[concepts/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/Kubernetes Core Concepts.md|Kubernetes Core Concepts]] — Cross-reference
- [[concepts/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity.md|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
