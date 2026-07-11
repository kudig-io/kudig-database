---
title: containerd 可观测性
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- 06-containerd-observability
- etcd
- prometheus
- grafana
- containerd
- falco
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 可观测性 是什么
- 如何 containerd 可观测性
trigger_keywords:
- containerd
- 可观测性
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 可观测性

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

Containerd 可观测性是关于监控和诊断 containerd 容器运行时行为的实践方法论。它涵盖了容器生命周期事件追踪、镜像拉取性能监控、运行时资源使用追踪、容器日志采集和 CRI gRPC 调用审计等多个维度。通过系统性的可观测性配置，运维团队可以快速定位容器启动失败、镜像拉取超时、运行时资源竞争等常见问题。

## Key Features（核心能力）

- **Native Metrics**：containerd 内置 Prometheus metrics 端点暴露运行时指标
- **CRI Metrics**：kubelet 通过 CRI 暴露容器操作延迟和错误率指标
- **事件日志**：containerd 通过 CRI 的 container log 接口输出容器 stdout/stderr
- **调试工具**：ctr、crictl、nerdctl 等命令行工具用于运行时调试
- **分布式追踪**：通过 OpenTelemetry 追踪容器镜像拉取链路
- **健康检查**：containerd binary 内置健康检查端点

## 架构与工作原理

可观测性数据分三层采集：Metrics 层通过 containerd metrics_v2 端点暴露镜像拉取计数、容器操作延迟、运行时 GC 统计等指标；Logs 层通过 CRI 接口将容器 stdout/stderr 重定向到 JSON 文件，由 Fluentd/Fluent Bit 采集；Events 层通过 K8s Event API 记录容器生命周期事件。关键指标包括 container_image_pull_duration_seconds、container_runtime_operations_seconds 等。

## K8s 集成

在 K8s 中，containerd 指标通过 kubelet 的 /metrics/cadvisor 和 /metrics/probes 端点暴露。cAdvisor 提供容器级别的 CPU、内存、网络、IO 指标。CRI 通过 kubelet 暴露镜像操作统计。通过 DaemonSet 部署 node-exporter 获取节点级指标。日志通过 DaemonSet 部署 Fluent Bit/Fluentd 自动采集所有节点的容器日志。

## 生产用例

- **容器启动排障**：通过事件和指标快速定位 Pod 启动失败原因
- **镜像拉取优化**：监控镜像拉取延迟和带宽使用，优化 Registry 配置
- **运行时资源监控**：追踪容器运行时的 CPU、内存、IO 使用情况
- **性能基线建立**：建立正常运行基线，支持异常检测和容量规划

## 安装与快速开始

```bash
# 查看 containerd metrics
curl -s http://localhost:1338/v1/metrics | grep containerd

# 使用 crictl 查看运行时状态
crictl info
crictl stats
```

## 对比替代方案

相比 Docker 运行时，containerd 的原生 metrics 更丰富且开销更低。结合 cAdvisor 和 Prometheus，可实现端到端的容器可观测性。

## Related

- [[spiderpool]] — Spiderpool
- [[ratify]] — Ratify
- [[container2wasm]] — container2wasm
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 06-containerd-observability
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference


<!-- risk-assessed -->
