---
title: Envoy
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- envoy
- prometheus
- grafana
- istio
- gateway
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Envoy 是什么
- 如何 Envoy
trigger_keywords:
- Envoy
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
---

# Envoy

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: C++

## 概述

description: '## 项目概述'

## 核心能力

- **L3/L4 代理**: TCP/UDP 代理，支持 TLS 终止
- **L7 代理**: HTTP/2、gRPC、WebSocket 支持
- **服务发现**: 支持多种服务发现机制
- **负载均衡**: 多种负载均衡算法
- **健康检查**: 主动和被动健康检查
- **可观测性**: 丰富的统计、日志、追踪支持

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 启用 TLS 终止和 mTLS
- 配置合理的超时和重试策略
- 使用动态配置（xDS）而非静态配置
- 启用访问日志和追踪
- 合理配置连接池大小
- 启用 HTTP/2 和连接复用

## 架构定位

在 CNCF 生态中，envoy 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]]

## Related

- [[litmus]] — LitmusChaos
- [[pixie]] — Pixie
- [[dapr]] — Dapr
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- [[domain-03-networking-traffic/04-envoy-proxy-enterprise.md|04-envoy-proxy-enterprise]]
- [[domain-03-networking-traffic/99-envoy-gateway-enterprise-guide.md|99-envoy-gateway-enterprise-guide]]
- [[domain-03-networking-traffic/07-envoy-gateway-enterprise.md|07-envoy-gateway-enterprise]]
- [[domain-19-landscape-references/graduated/envoy/envoy.md|envoy]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.9.md|RELEASE-NOTES-1.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.28.md|RELEASE-NOTES-1.28]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.18.md|RELEASE-NOTES-1.18]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.19.md|RELEASE-NOTES-1.19]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.29.md|RELEASE-NOTES-1.29]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.16.md|RELEASE-NOTES-1.16]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.22.md|RELEASE-NOTES-1.22]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.32.md|RELEASE-NOTES-1.32]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.26.md|RELEASE-NOTES-1.26]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.12.md|RELEASE-NOTES-1.12]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.36.md|RELEASE-NOTES-1.36]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.27.md|RELEASE-NOTES-1.27]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.13.md|RELEASE-NOTES-1.13]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.37.md|RELEASE-NOTES-1.37]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.17.md|RELEASE-NOTES-1.17]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.23.md|RELEASE-NOTES-1.23]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.33.md|RELEASE-NOTES-1.33]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.24.md|RELEASE-NOTES-1.24]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.10.md|RELEASE-NOTES-1.10]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.34.md|RELEASE-NOTES-1.34]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.14.md|RELEASE-NOTES-1.14]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.20.md|RELEASE-NOTES-1.20]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.30.md|RELEASE-NOTES-1.30]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.15.md|RELEASE-NOTES-1.15]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.21.md|RELEASE-NOTES-1.21]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.31.md|RELEASE-NOTES-1.31]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.25.md|RELEASE-NOTES-1.25]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.11.md|RELEASE-NOTES-1.11]]
- [[domain-19-landscape-references/topic-release-notes/networking/envoy/RELEASE-NOTES-1.35.md|RELEASE-NOTES-1.35]]
- [[journal/digest-2026-05-21-full|Wiki 全量知识库摘要 — 2026-05-21]] — Cross-reference
- [[references/networking-terms|K8s 网络术语参考]] — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/tooling-terms|K8s 工具链术语参考]] — Cross-reference
- [[references/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[concepts/service-mesh-evolution|服务网格演进]] — Cross-reference
- [[concepts/microservice-resilience-patterns|Microservice Resilience Patterns]] — Cross-reference
- [[skills/service-mesh-istio-fta|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/service-mesh-index|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
