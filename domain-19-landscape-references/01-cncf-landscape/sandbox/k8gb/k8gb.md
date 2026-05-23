---
title: K8GB
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- k8gb
- coredns
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8GB 是什么
- 如何 K8GB
trigger_keywords:
- K8GB
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# K8GB

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

K8GB 是一个 Kubernetes 原生的全局负载均衡解决方案，基于 DNS 实现跨集群的流量调度。它使用 Kubernetes CRD (GslbStrategy) 定义全局负载均衡策略，通过 CoreDNS 和 ExternalDNS 实现多集群间的 DNS 基础的流量管理，支持轮询、地理位置和故障转移策略。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **DNS 委托**: 正确配置 DNS 委托，将 gslb 子域委托给 K8GB 管理的 CoreDNS
- **TTL 配置**: 设置合理的 DNS TTL，平衡故障切换速度和 DNS 缓存效率
- **健康检查**: 确保后端服务有适当的健康检查端点
- **脑裂保护**: 配置 splitBrainThresholdSeconds 防止网络分区导致的脑裂
- **地理标签**: 为每个集群配置准确的地理标签（geoTag）

## 架构定位

在 CNCF 生态中，k8gb 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources|crd-custom-resources]]
- [[concepts/controller-pattern|controller-pattern]]

## Related

- [[chaos-mesh]] — [[Chaos Mesh|Chaos Mesh]]
- [[kubean]] — Kubean
- [[tikv]] — TiKV
- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- k8gb
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/dns-index|DNS 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
