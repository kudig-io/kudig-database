---
title: BFE
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- bfe
- prometheus
- grafana
- ingress
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- BFE 是什么
- 如何 BFE
trigger_keywords:
- BFE
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---



# BFE

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

BFE 是百度开源的现代化七层负载均衡器和反向代理，处理百度内部每天数万亿级别的请求。它提供高级流量路由、安全防护、可观测性等能力，支持 HTTP/HTTPS/HTTP2/QUIC 等协议，适合作为 Kubernetes [[Ingress|Ingress]] Controller 或独立的流量网关。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **路由规则**: 使用 BFE 的条件表达式实现复杂路由逻辑
- **安全防护**: 启用内置 WAF 模块保护后端服务
- **QUIC/HTTP3**: 对延迟敏感的前端服务启用 QUIC 支持
- **流量管控**: 使用权重路由实现金丝雀发布和灰度策略
- **监控**: 利用 8421 端口的监控接口集成 Prometheus

## 架构定位

在 CNCF 生态中，bfe 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[concepts/controller-pattern.md|controller-pattern]]

## Related

- [[meshery]] — Meshery
- [[knative]] — Knative
- [[konveyor]] — Konveyor
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bfe
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
