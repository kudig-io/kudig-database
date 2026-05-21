---
title: Kube-burner
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- orchestration
- kube-burner
- etcd
- prometheus
- grafana
- cilium
- elasticsearch
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kube-burner 是什么
- 如何 Kube-burner
trigger_keywords:
- Kube-burner
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
---

# Kube-burner

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Kube-burner 是一个 Kubernetes 性能和规模测试工具，通过在集群中创建或删除大量对象来模拟各种负载场景，并收集详细的性能指标。它广泛用于 Kubernetes 发行版（如 OpenShift）的可扩展性测试和基准测试。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **基线测试**: 先在空集群运行获取基线数据，再对比优化后的结果
- **渐进负载**: 从低 QPS 开始逐步提高，找到集群的吞吐瓶颈
- **指标存储**: 使用 Elasticsearch 持久化结果，便于趋势分析和回归检测
- **告警阈值**: 根据 SLO 设定合理的告警阈值，及时发现性能回退
- **资源隔离**: 在专用测试集群运行，避免影响生产环境
- **重复执行**: 每次测试多次运行取平均值，减少偶发因素影响

## 架构定位

在 CNCF 生态中，kube-burner 属于 **Orchestration** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[deployment]]

## Related

- [[slimtoolkit]] — SlimToolkit
- [[cni]] — CNI (Container Network Interface)
- [[entities/cncf-infrastructure.md|cncf-infrastructure]] — CNCF 基础设施与混沌工程项目全景
- [[external-secrets]] — External Secrets Operator
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/kube-burner/kube-burner.md|kube-burner]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
