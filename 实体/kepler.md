---
title: Kepler [entities]
description: '## 概述'
summary: 'Kepler (Kubernetes-based Efficient Power Level Exporter) 使用 eBPF 探测器采集系统计数器，结合机器学习模型估算 Kubernetes Pod 和节点级别的能耗。它将能耗数据导出为 Prometheus 指标，帮助组织了解工作负载的碳足迹，支持可持续计算和绿色IT决策。'
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
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kepler

> **CNCF 状态**: Sandbox | **类别**: Cost | **主要语言**: Go, C (eBPF)

## 概述

Kepler（Kubernetes-based Efficient Power Level Exporter）是由 Red Hat/Sustainable Computing 工作组开发的开源工具，2023 年加入 CNCF Sandbox。它使用 eBPF 探测器采集系统计数器，结合机器学习模型估算 Kubernetes Pod 和节点级别的能耗。Kepler 将能耗数据导出为 Prometheus 指标，帮助组织了解工作负载的碳足迹，支持可持续计算（Sustainable Computing）和绿色 IT 决策。

## 核心特性

- **eBPF 低开销采集**: 内核级系统计数器采集，极低性能开销
- **Pod 级别能耗**: 精确到 Pod 和容器粒度的能耗估算
- **多硬件支持**: CPU（Intel RAPL / AMD Energy）、GPU（NVML）、DRAM
- **ML 估算模型**: 在不支持 RAPL 的环境中使用机器学习模型估算能耗
- **Prometheus 导出**: 标准化的 `kepler_*` Prometheus 指标
- **Grafana 仪表盘**: 预置可视化面板展示能耗和碳足迹

## 架构

Kepler 以 DaemonSet 形式部署在每个节点上。核心组件包括：eBPF 程序（通过 bpftrace/perf 采集 CPU 周期、缓存引用等硬件计数器）、Kepler Exporter（聚合计数器，结合 RAPL 读数或 ML 模型计算能耗）、Power Model（预训练的机器学习模型，根据 CPU 利用率、指令数等特征估算瓦特级功耗）。能耗数据按 Pod 聚合（通过 cgroup ID 关联），导出为 Prometheus 指标（如 `kepler_pod_package_energy_millijoule`）。

## Kubernetes 集成

Kepler 通过 DaemonSet 部署在所有节点，以特权模式运行以加载 eBPF 程序和读取 RAPL（/sys/class/powercap）。自动发现 Pod 和容器元数据（通过 Kubernetes API 和 cgroup）。能耗指标按 Pod、Container、Node 三个维度导出。通过 ServiceMonitor 集成 Prometheus。支持通过 Kepler Operator 或 Helm Chart 部署。可与 Kepler Model Server 配合，动态训练和更新估算模型。

## 生产使用场景

1. **碳足迹追踪**: 量化每个微服务的能耗和碳排放，支持 ESG 报告
2. **能耗优化**: 基于能耗数据优化 Pod 调度，将高能耗工作负载调度到低碳电网区域
3. **FinOps 成本分析**: 结合云成本数据，计算单位产出的能耗效率
4. **可持续 K8s**: 为绿色 Kubernetes 运营提供数据基础

## 安装

```bash
# Helm 安装
helm repo add kepler https://sustainable-computing.io/kepler
helm install kepler kepler/kepler -n kepler --create-namespace
# 验证指标
kubectl port-forward -n kepler svc/kepler-exporter 9102:9102
curl localhost:9102/metrics | grep kepler_pod
# Grafana Dashboard
# 导入 Grafana Dashboard ID: 18122
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kepler** | K8s 原生、eBPF、Pod 级别 | 需内核 RAPL 支持 |
| Scaphandre | 多平台支持 | 非 K8s 原生 |
| node_exporter + RAPL | 简单 | 仅节点级、无 Pod 粒度 |
| 云厂商碳工具 | 云原生集成 | 厂商绑定 |

## 架构定位

在 CNCF 生态中，Kepler 属于 **Sustainability / Observability** 类别，是 CNCF TAG Environmental Sustainability 的旗舰项目。它将能耗可观测性引入 Kubernetes。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/observability-pillars.md|observability-pillars]]
- networking.md|cilium-ebpf-networking]]
- [[pod-lifecycle]]

## Related

- [[openebs]] — OpenEBS
- [[05-containerd-windows-support]] — [[containerd|containerd]]rd Windows 支持|containerd Windows 支持]]
- [[cortex]] — Cortex
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kepler
- [[实体/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
