---
title: Aeraki Mesh [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- aeraki-mesh
- prometheus
- grafana
- istio
- envoy
- redis
- kafka
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Aeraki Mesh 是什么
- 如何 Aeraki Mesh
trigger_keywords:
- Aeraki
- Mesh
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- redis-basics
created: "2026-05-23"
---

# Aeraki Mesh

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Aeraki Mesh 是 Istio 服务网格的扩展框架，专注于为非 HTTP 协议提供流量管理能力。在微服务架构中，除了 HTTP/gRPC 之外，还广泛使用 Dubbo、Thrift、Redis、Kafka 等协议。Aeraki Mesh 通过扩展 Istio 的数据面（Envoy）和控制面，使这些非 HTTP 协议也能享受服务网格的流量路由、负载均衡、熔断限流和可观测性能力。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **协议识别**: 确保 [[Service|Service]] 端口命名遵循 Istio 协议识别规范 (如 `tcp-dubbo`)
- **版本灰度**: 使用 MetaRouter 进行 Dubbo 版本灰度发布，结合权重控制流量比例
- **Redis 读写分离**: 利用 Redis 协议解析能力实现自动读写分离
- **指标采集**: 启用 Aeraki 协议指标，配合 Prometheus + Grafana 监控非 HTTP 服务
- **渐进扩展**: 先用 MetaProtocol 管理核心协议，再逐步扩展到更多自定义协议

## 架构定位

在 CNCF 生态中，aeraki-mesh 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana|prometheus-grafana]]
- [[istio]]
- [[concepts/controller-pattern|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[grpc]] — gRPC
- [[istio]] — Istio
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- aeraki-mesh
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
