---
title: Pixie [entities]
description: '## 概述'
summary: 'Pixie 是一个 Kubernetes 原生的可观测性平台，使用 eBPF 自动采集遥测数据，无需代码变更或手动 instrumentation。它提供对服务通信 (HTTP、gRPC、DNS、MySQL、PostgreSQL、Redis、Kafka)、资源使用和应用性能的即时可见性。Pixie 数据在集群内处理，支持 PxL 查询语言进行分析。'
category: entities
tags:
- k8s
- cncf
- observability
- pixie
- prometheus
- grafana
- istio
- redis
- mysql
- postgresql
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pixie 是什么
- 如何 Pixie
trigger_keywords:
- Pixie
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- kafka-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pixie

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: C++, Go

## 概述

Pixie 是一个 Kubernetes 原生的可观测性平台，使用 eBPF 自动采集遥测数据，无需代码变更或手动 instrumentation。它提供对服务通信 (HTTP、gRPC、DNS、MySQL、PostgreSQL、Redis、Kafka)、资源使用和应用性能的即时可见性。Pixie 数据在集群内处理，支持 PxL 查询语言进行分析。

## 核心能力

- **零 instrumentation**: 基于 eBPF 自动采集，无需代码变更
- **协议解析**: 自动解析 HTTP、gRPC、MySQL、PostgreSQL、Redis、DNS、Kafka
- **PxL 查询语言**: Python 风格的数据查询和分析
- **边缘计算**: 数据在集群内处理，不外传
- **即时可见性**: 安装即可获得 [[Service|Service]] Map、请求追踪
- **Flamegraph**: CPU 性能分析火焰图

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **内核版本**: 确保 Linux 内核 >= 4.14 (推荐 5.3+)
- **资源预留**: PEM 约需 2GB 内存/节点
- **数据保留**: 默认短期保留，重要数据导出外部存储
- **PxL 复用**: 编写通用 PxL 脚本供团队复用
- **安全**: 数据不出集群，适合合规环境
- **TLS**: Pixie 自动追踪 TLS 加密通信

## 架构定位

在 CNCF 生态中，pixie 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[operator-pattern]]
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]]
- [[concepts/observability-pillars.md|observability-pillars]]

## Related

- [[02-istio-advanced-traffic-management]] — Istio 高级流量管理
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[litmus]] — LitmusChaos
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- pixie
- [[entities/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
