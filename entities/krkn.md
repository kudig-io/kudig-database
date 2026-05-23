---
title: Krkn
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- chaos
- krkn
- etcd
- prometheus
- grafana
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Krkn 是什么
- 如何 Krkn
trigger_keywords:
- Krkn
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
created: "2026-05-23"
---

# Krkn

> **CNCF 状态**: Sandbox | **类别**: Chaos | **主要语言**: Python

## 概述

Krkn（原名 Kraken）是一个面向 Kubernetes 的混沌工程工具，通过向集群注入各种故障场景来测试系统的弹性和可靠性。它支持节点故障、Pod 中断、网络混沌、CPU/内存压力、时间偏移等多种混沌场景，并提供基于 Cerberus 的健康检查和告警机制，帮助团队在生产环境之前发现系统弱点。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **渐进式注入**: 从小范围、低强度开始，逐步扩大混沌范围
- **健康检查**: 始终启用 Cerberus 监控集群状态，设置安全阀
- **非生产先行**: 先在测试/预发环境验证混沌场景
- **SLO 驱动**: 基于 SLO 定义验收标准，混沌测试通过=SLO 不受影响
- **团队协作**: 提前通知相关团队，记录混沌测试的发现和改进措施

## 架构定位

在 CNCF 生态中，krkn 属于 **Chaos** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[operator-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[devfile]] — Devfile
- [[cohdi]] — Cohdi
- [[koordinator]] — Koordinator
- [[oxia]] — Oxia
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- krkn
- [[entities/cncf-infrastructure|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]
