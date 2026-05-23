---
title: Kepler [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- cost
- kepler
- prometheus
- grafana
- cilium
- containerd
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kepler 是什么
- 如何 Kepler
trigger_keywords:
- Kepler
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# Kepler

> **CNCF 状态**: Sandbox | **类别**: Cost | **主要语言**: Go, C (eBPF)

## 概述

Kepler (Kubernetes-based Efficient Power Level Exporter) 使用 eBPF 探测器采集系统计数器，结合机器学习模型估算 Kubernetes Pod 和节点级别的能耗。它将能耗数据导出为 Prometheus 指标，帮助组织了解工作负载的碳足迹，支持可持续计算和绿色IT决策。

## 核心能力

- **eBPF 采集**: 低开销的内核级能耗数据采集
- **Pod 级别能耗**: 精确到 Pod 和容器的能耗估算
- **多硬件支持**: CPU (RAPL)、GPU (NVML)、DRAM
- **ML 模型**: 机器学习辅助能耗估算
- **Prometheus 导出**: 标准 Prometheus 指标格式
- **Grafana 仪表板**: 预置可视化面板

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **RAPL 支持**: 确保内核支持 Intel RAPL 或 AMD Energy
- **权限配置**: Kepler 需要特权访问 /sys 和 /proc
- **ML 模型**: 在不支持 RAPL 的环境中使用 ML 估算
- **碳转换**: 结合区域碳强度数据计算碳足迹
- **告警规则**: 设置能耗异常告警
- **优化决策**: 基于能耗数据优化工作负载调度

## 架构定位

在 CNCF 生态中，kepler 属于 **Cost** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[concepts/observability-pillars.md|observability-pillars]]
- networking.md|cilium-ebpf-networking]]
- [[pod-lifecycle]]

## Related

- [[openebs]] — OpenEBS
- [[05-containerd-windows-support]] — [[containerd|containerd]]rd Windows 支持|containerd Windows 支持]]
- [[cortex]] — Cortex
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kepler
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
