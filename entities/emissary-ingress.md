---
title: Emissary-Ingress (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- emissary-ingress
- prometheus
- grafana
- envoy
- containerd
- ingress
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Emissary-Ingress 是什么
- 如何 Emissary-Ingress
trigger_keywords:
- Emissary-Ingress
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# Emissary-[[Ingress|Ingress]]

> **CNCF 状态**: Incubating | **类别**: Networking | **主要语言**: Python, Go

## 概述

Emissary-Ingress（原 Ambassador API Gateway）是 Kubernetes 原生的 API 网关，基于 Envoy Proxy 构建。它提供丰富的流量管理、认证授权和可观测性能力，是微服务架构的入口层解决方案。

## 核心能力

- **Kubernetes 原生**: CRD 方式配置，声明式管理
- **基于 Envoy**: 利用 Envoy 的高性能和可扩展性
- **自助服务**: 开发者可自主配置路由规则
- **金丝雀发布**: 支持权重路由和 A/B 测试
- **认证集成**: OAuth2、JWT、API Key、外部认证
- **速率限制**: 细粒度的流量控制

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **高可用**: 部署多副本，配置 PodDisruptionBudget
- **资源限制**: 为 Envoy 配置合适的 CPU/Memory
- **渐进部署**: 使用金丝雀发布验证新版本
- **监控告警**: 配置请求延迟和错误率告警
- **安全加固**: 启用 TLS、认证、速率限制

## 架构定位

在 CNCF 生态中，emissary-ingress 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana|prometheus-grafana]]
- [[entities/crd-custom-resources|crd-custom-resources]]
- [[concepts/secrets-management|secrets-management]]
- [[pod-lifecycle]]
- [[concepts/security-defense-depth|security-defense-depth]]

## Related

- [[04-containerd-upgrade-migration]] — containerd 升级迁移
- [[spin]] — Spin
- [[backstage]] — Backstage
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- emissary-ingress
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
