---
title: Envoy (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
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
tier: supporting
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- 04-envoy-proxy-enterprise
- 99-envoy-gateway-enterprise-guide
- 07-envoy-gateway-enterprise
- envoy
- RELEASE-NOTES-1.9
- RELEASE-NOTES-1.28
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-1.29
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.22
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.32
- RELEASE-NOTES-1.26
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.36
- RELEASE-NOTES-1.27
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.37
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.23
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.33
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.24
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.34
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.20
- RELEASE-NOTES-1.30
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.21
- RELEASE-NOTES-1.31
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.25
- RELEASE-NOTES-1.11
- RELEASE-NOTES-1.35
- Wiki 全量知识库摘要 — 2026-05-21 — Cross-reference
- [[entities/networking-terms.md|[[K8s 网络术语参考|K8s 网络术语参考]]]] — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[entities/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[entities/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[entities/k8s-platform-extensions.md|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[concepts/service-mesh-evolution.md|服务网格演进]] — Cross-reference
- [[concepts/microservice-resilience-patterns.md|Microservice Resilience Patterns]] — Cross-reference
- [[skills/service-mesh-istio-fta.md|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
