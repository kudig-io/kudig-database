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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
- [[概念/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合

- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[概念/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[概念/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[概念/service-mesh-architecture.md|Service Mesh Architecture]]
- [[实体/tetragon.md|Tetragon]]
- Hubble

- [[概念/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- 18-kubernetes-ebpf-cilium-deep-practice
- 03-cilium-cni-architecture
- 99-cilium-ebpf-network-guide
- 05-cilium-service-mesh
- 04-cilium-network-policy
- [[故障诊断/FTA故障树/list/cilium-fta.md|cilium-fta]]
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
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[实体/k8s-networking-ecosystem.md|网络体系：CNI、Service、Ingress、Gateway API 与多集群网络]] — Cross-reference
- [[实体/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- [[实体/k8s-networking-domain-guide.md|Kubernetes Networking Domain Guide]] — Cross-reference
- [[概念/eBPF x 运行时安全.md|eBPF x 运行时安全]] — Cross-reference
- [[概念/service-mesh-evolution.md|服务网格演进]] — Cross-reference
- [[概念/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[概念/Kubernetes Core Concepts.md|Kubernetes Core Concepts]] — Cross-reference
- [[概念/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]] — Cross-reference
- [[技能/skill-20-networkpolicy-connectivity.md|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[技能/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
