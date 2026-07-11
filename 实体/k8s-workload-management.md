---
title: 工作负载管理：Pod 生命周期、调度策略与弹性伸缩
description: '# 工作负载管理'
summary: 'Init Containers → Main Containers → Sidecar Containers 执行顺序。'
category: reference
tags:
- k8s
- workloads
- pod
- scheduling
- hpa
- vpa
- autoscaling
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 工作负载管理：Pod 生命周期、调度策略与弹性伸缩 是什么
- 如何 工作负载管理：Pod 生命周期、调度策略与弹性伸缩
trigger_keywords:
- 工作负载管理：Pod
- 生命周期
- 调度策略与弹性伸缩
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 工作负载管理

> **类别**: Core Concept | **适用版本**: Kubernetes 1.27+

## 概述

工作负载管理是 Kubernetes 的核心能力之一，涵盖 Pod 生命周期管理、调度策略配置、资源管理和弹性伸缩四大领域。Kubernetes 通过声明式 API 让开发者定义期望的工作负载状态（如 Deployment、StatefulSet、DaemonSet），由控制器自动驱动实际状态向期望状态收敛。理解 Pod 生命周期、探针机制、调度策略和弹性伸缩原理是构建可靠生产级应用的基础。Kubernetes 调度器负责将 Pod 分配到合适的节点，而 HPA/VPA/Cluster Autoscaler 则根据负载动态调整资源供给。

## 核心能力

- **Pod 生命周期管理**: 通过 Init Container、Main Container、Sidecar Container 有序管理容器启动
- **探针机制**: livenessProbe（存活检测）、readinessProbe（就绪检测）、startupProbe（启动检测）
- **调度策略**: nodeSelector、nodeAffinity、podAffinity/AntiAffinity、taints & tolerations、topologySpreadConstraints
- **QoS 管理**: Guaranteed、Burstable、BestEffort 三级服务质量保证
- **弹性伸缩**: HPA（水平）、VPA（垂直）、Cluster Autoscaler（节点）、KEDA（事件驱动）
- **优先级与抢占**: PriorityClass 定义 Pod 优先级，资源不足时驱逐低优先级 Pod

## 架构

工作负载管理涉及多个 Kubernetes 核心组件协作：

- **kube-scheduler**: 监听未调度的 Pod，根据过滤（Filter）和打分（Score）两阶段算法选择最佳节点
- **kube-controller-manager**: 运行 Deployment、ReplicaSet、StatefulSet、DaemonSet 等控制器
- **kubelet**: 管理节点上的 Pod 生命周期，执行探针检查和容器重启
- **metrics-server / Prometheus Adapter**: 为 HPA 提供 CPU/Memory 和自定义指标
- **Cluster Autoscaler**: 监听 Pending Pod，触发云厂商 API 扩展节点

Pod 状态流转：`Pending → Running → Succeeded / Failed / Unknown`

## K8s 集成

工作负载管理是 Kubernetes 原生核心能力，通过内置控制器和调度器实现，无需额外安装。通过 kubectl、Helm 或 GitOps 工具（ArgoCD/Flux）管理各类工作负载 CRD。HPA 可通过 `kubectl autoscale` 或 Prometheus Adapter 集成自定义指标。生产环境推荐配合 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 ResourceQuota、LimitRange 和 PodDisruptionBudget 使用。

## 生产场景

1. **微服务滚动发布**: 使用 Deployment + maxSurge/maxUnavailable 实现零停机滚动更新
2. **有状态数据库部署**: StatefulSet + 持久化存储 + 有序启停，管理 MySQL、PostgreSQL 等数据库
3. **GPU/AI 推理弹性伸缩**: KEDA 基于 Prometheus 指标或消息队列深度，自动伸缩推理 Pod
4. **全局负载均衡**: topologySpreadConstraints 确保 Pod 跨可用区均匀分布

## 安装

```bash
# HPA 基础 — 安装 metrics-server
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

# KEDA — 事件驱动伸缩
helm repo add kedacore https://kedacore.github.io/charts
helm install keda kedacore/keda --namespace keda-system --create-namespace

# Prometheus Adapter — 自定义指标 HPA
helm install prometheus-adapter prometheus-community/prometheus-adapter -n monitoring
```

## 对比

| 伸缩组件 | 维度 | 触发条件 | 适用场景 |
|----------|------|----------|----------|
| HPA | Pod 副本数 | CPU/Memory/自定义指标 | Web 服务、API |
| VPA | Pod 资源配置 | 历史使用量分析 | 资源调优 |
| Cluster Autoscaler | 节点数 | Pending Pod | 集群扩缩容 |
| KEDA | 事件驱动 | 消息队列/外部事件 | 消费者、批处理 |

## 生产部署要点

QoS 优先级（OOM 时驱逐顺序）：
1. **BestEffort**: 未设置 requests/limits → 最先被驱逐
2. **Burstable**: requests < limits → 次优先被驱逐
3. **Guaranteed**: requests = limits → 最后被驱逐

HPA 经典公式：`目标副本数 = ceil(当前副本数 × (当前指标值 / 目标指标值))`

---

> 来源：.zread/wiki/drafts/8-gong-zuo-fu-zai-guan-li-pod-sheng-ming-zhou-qi-diao-du-ce-lue-yu-dan-xing-shen-suo.md

## Related

- [[keda]] — KEDA

- [[平台工程/代码分析/deployment-create/08-hpa-integration.md|Deployment 与 HPA 集成源码分析]]

<!-- risk-assessed -->
