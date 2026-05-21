---
title: Istio
description: '- [[skills/k8s-network-security-guide.md|k8s-network-security-guide]] — Kubernetes 网络安全最佳实践'
category: entities
tags:
- k8s
- service-mesh
- istio
- envoy
- mtls
- traffic-management
- ingress
- gateway
- helm
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Istio 是什么
- 如何 Istio
trigger_keywords:
- Istio
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
---

# Istio

Istio is the most widely adopted service mesh, graduated from CNCF in 2023. It provides transparent traffic management, security, and observability for microservices.

## Key Facts

- **Status**: CNCF graduated (2023)
- **Data Plane**: Envoy proxy (C++)
- **Control Plane**: istiod (merged Pilot+Citadel+Galley)
- **Memory**: ~2GB control plane, ~100MB/sidecar
- **Modes**: Sidecar (traditional), Ambient (sidecarless, GA v1.29)

## Core Resources

| Resource | Purpose |
|----------|---------|
| VirtualService | Traffic routing, weight splitting, retries, timeouts |
| DestinationRule | Connection pools, outlier detection, traffic policies |
| Gateway | Ingress/egress traffic entry point |
| PeerAuthentication | mTLS mode (STRICT/PERMISSIVE/DISABLE) |
| AuthorizationPolicy | L7 access control (allow/deny rules) |
| RequestAuthentication | JWT validation for external services |

## Ambient Mesh (v1.29 GA)

Istio Ambient replaces sidecars with:
- **ztunnel**: Node-level L4 proxy (Rust, ~50MB/node) for mTLS and L4 policies
- **Waypoint Proxy**: Per-service L7 proxy for advanced traffic management

Benefits: lower resource overhead, simpler operations, no sidecar injection issues.

## Related

- [[skills/k8s-network-security-guide.md|k8s-network-security-guide]] — Kubernetes 网络安全最佳实践
- [[03-istio-security-hardening]] — Istio 安全加固
- [[envoy]] — Envoy
- [[concepts/microservice-resilience-patterns.md|microservice-resilience-patterns]] — Microservice Resilience Patterns
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]
- [[concepts/microservice-resilience-patterns.md|Microservice Resilience Patterns]]
- [[envoy|Envoy Proxy]]
- [[linkerd|Linkerd]]

- [[domain-19-landscape-references/09-kubernetes-service-mesh-istio-integration.md|09-kubernetes-service-mesh-istio-integration]]
- [[domain-19-landscape-references/graduated/istio/02-istio-advanced-traffic-management.md|02-istio-advanced-traffic-management]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.9.md|RELEASE-NOTES-1.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.28.md|RELEASE-NOTES-1.28]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.18.md|RELEASE-NOTES-1.18]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.19.md|RELEASE-NOTES-1.19]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.29.md|RELEASE-NOTES-1.29]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.16.md|RELEASE-NOTES-1.16]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.22.md|RELEASE-NOTES-1.22]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.26.md|RELEASE-NOTES-1.26]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.12.md|RELEASE-NOTES-1.12]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.27.md|RELEASE-NOTES-1.27]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.13.md|RELEASE-NOTES-1.13]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.17.md|RELEASE-NOTES-1.17]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.23.md|RELEASE-NOTES-1.23]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.24.md|RELEASE-NOTES-1.24]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.10.md|RELEASE-NOTES-1.10]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.14.md|RELEASE-NOTES-1.14]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.20.md|RELEASE-NOTES-1.20]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.15.md|RELEASE-NOTES-1.15]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.21.md|RELEASE-NOTES-1.21]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.1.md|RELEASE-NOTES-0.1]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.25.md|RELEASE-NOTES-1.25]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-1.11.md|RELEASE-NOTES-1.11]]
- [[domain-19-landscape-references/topic-release-notes/networking/istio/RELEASE-NOTES-0.5.md|RELEASE-NOTES-0.5]]
- [[domain-03-networking-traffic/99-istio-service-mesh-guide.md|99-istio-service-mesh-guide]]
- [[domain-03-networking-traffic/01-istio-enterprise-service-mesh.md|01-istio-enterprise-service-mesh]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/service-mesh-istio-fta.md|Service Mesh(Istio) 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md|05-service-mesh-istio-troubleshooting]]
- [[domain-19-landscape-references/graduated/istio/istio.md|istio]]
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[references/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[concepts/service-mesh-evolution|服务网格演进]] — Cross-reference
- [[concepts/bp-security|最佳实践：Security]] — Cross-reference
- [[skills/learn-05-ingress-basics|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[skills/service-mesh-istio-fta|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[skills/ts-cloud-provider|云服务商集成排查]] — Cross-reference
- [[skills/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/service-mesh-index|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
