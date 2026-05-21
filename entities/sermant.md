---
title: Sermant
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- service-mesh
- sermant
- prometheus
- grafana
- istio
- cilium
- opa
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Sermant 是什么
- 如何 Sermant
trigger_keywords:
- Sermant
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- policy-basics
---

# Sermant

> **CNCF 状态**: Sandbox | **类别**: Service Mesh | **主要语言**: Java

## 概述

Sermant 是华为开源的基于 Java Agent 的无代理服务网格方案，通过 Java Instrumentation 机制（字节码增强）为 Java 微服务提供服务治理能力，无需修改应用代码或部署 Sidecar 代理。它支持流量路由、限流熔断、负载均衡、服务注册发现等功能，特别适合 Java 技术栈的微服务架构。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **插件按需加载**: 只启用需要的插件，减少 Agent 对应用启动时间的影响
- **灰度验证**: 先在测试环境挂载 Agent，验证字节码增强不影响业务逻辑
- **版本兼容**: 确认 Sermant 版本与目标框架版本（Spring Boot/Dubbo）兼容
- **配置热更新**: 利用 Sermant Backend 实现运行时动态调整治理策略
- **监控集成**: 开启监控插件将治理指标上报到 Prometheus

## 架构定位

在 CNCF 生态中，sermant 属于 **Service Mesh** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[deployment]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]

## Related

- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[serverless-devs]] — Serverless Devs
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/sermant/sermant.md|sermant]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
