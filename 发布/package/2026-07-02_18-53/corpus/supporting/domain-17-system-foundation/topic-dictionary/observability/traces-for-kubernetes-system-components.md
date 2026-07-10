---
title: Kubernetes 系统组件链路追踪
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- containerd
- cri-o
- webhook
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 系统组件链路追踪 是什么
- 如何 Kubernetes 系统组件链路追踪
trigger_keywords:
- Kubernetes
- 系统组件链路追踪
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 系统组件链路追踪

## 概述

FEATURE STATE: `Kubernetes v1.27 [beta]`

系统组件链路追踪记录了集群中各操作之间的延迟和关系。Kubernetes 组件通过 **[[OpenTelemetry|OpenTelemetry]] Protocol (OTLP)** 使用 [[gRPC|gRPC]] exporter 发出追踪数据（trace spans），这些数据可以通过 OpenTelemetry Collector 收集并路由到追踪后端，用于可视化端到端请求流、诊断性能问题和识别瓶颈。

## 核心概念/原理

- **Trace（追踪）**：描述一个请求在系统中经过的完整路径，由多个 Span（跨度）组成。
- **Span**：追踪中的一个基本单元，记录某个操作的名称、开始时间、持续时间以及相关的属性和事件。
- **OTLP（OpenTelemetry Protocol）**：用于传输可观测性数据的标准协议，Kubernetes 组件通过 gRPC 方式导出 spans。
- **OpenTelemetry Collector**：接收、处理（如采样、脱敏）并转发 spans 到追踪后端的组件。

## 关键机制或特性

### 追踪收集方式

Kubernetes 组件内置了 OTLP gRPC exporter，支持两种收集模式：

1. **通过 OpenTelemetry Collector**：组件将 spans 发送到 collector，由 collector 统一处理后转发到后端。
2. **直接发送到后端**：在追踪配置文件中直接指定后端 endpoint，无需 collector，简化架构。

默认情况下，Kubernetes 组件使用 IANA OpenTelemetry 端口 `4317` 通过 gRPC 导出 traces。

### 环境变量配置

- `OTEL_EXPORTER_OTLP_HEADERS`：配置追踪后端的请求头，包括认证信息。
- `OTEL_RESOURCE_ATTRIBUTES`：配置资源属性，如集群名称、命名空间、Pod 名称等。

### kube-apiserver 追踪

kube-apiserver 为以下场景生成 spans：

- 传入的 HTTP 请求
- 对外部 webhook 的请求
- 对 [[domain-17-system-foundation/知识字典/fundamentals/etcd.md|etcd]] 的请求
- 重入请求（re-entrant requests）

kube-apiserver 会在对外请求时传播 **W3C Trace Context**，但不会利用传入请求附带的 trace context（因为 apiserver 通常是公共端点）。

启用方式：通过 `--tracing-config-file=<path-to-config>` 提供追踪配置文件。

示例配置：

```yaml
apiVersion: apiserver.config.k8s.io/v1
kind: TracingConfiguration
# endpoint: localhost:4317  # 默认值
samplingRatePerMillion: 100
```

### kubelet 追踪

FEATURE STATE: `Kubernetes v1.34 [stable]`（默认启用）

kubelet 的 CRI 接口和认证 HTTP 服务器已埋点生成 trace spans。配置项包括 endpoint 和采样率，也支持 trace context 传播。如果未配置 endpoint，默认使用 `localhost:4317`。

Kubernetes v1.35 的 kubelet 从以下位置收集 spans：

- 垃圾回收
- Pod 同步流程
- 每个 gRPC 方法

kubelet 会在 gRPC 请求中传播 trace context，使支持追踪埋点的容器运行时（如 CRI-O、containerd）能够将其 spans 与 kubelet 的 spans 关联，形成父子链路。

启用方式：在 kubelet 配置中应用 tracing 配置。

示例配置片段：

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
tracing:
  # endpoint: localhost:4317
  samplingRatePerMillion: 100
```

`samplingRatePerMillion` 设为 `1000000` 时，将采样并发送**所有** spans。

## 使用场景

- **控制平面性能诊断**：追踪请求在 apiserver、etcd、webhook 之间的流转，定位高延迟环节。
- **节点问题排查**：通过 kubelet 与容器运行时的关联 spans，分析 Pod 启动、同步、垃圾回收的耗时。
- **端到端请求可视化**：将控制平面、节点组件和应用的 spans 串联，形成完整的请求链路图。
- **容量规划与优化**：识别频繁出现的慢路径和热点组件，指导扩容和架构优化。

## 最佳实践/注意事项

- 导出 spans 会带来一定的网络和 CPU 开销，生产环境中建议从较低的采样率开始（如 `samplingRatePerMillion=100`，即 0.01%）。
- 如果启用追踪后观察到性能问题，可通过降低采样率或完全移除追踪配置来消除影响。
- 追踪埋点目前仍在积极开发中，在达到稳定版之前，span 名称、属性、埋点端点等都可能在不同版本中发生变化，不保证向后兼容。
- 结合 OpenTelemetry Collector 使用可以实现采样、脱敏、批处理等高级功能，降低对后端的直接压力。
- 配置 `OTEL_RESOURCE_ATTRIBUTES` 以便在追踪后端中清晰区分不同集群、命名空间和组件实例。

## 参考链接

- [Traces For Kubernetes System Components - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/system-traces/)

## Related

- [[domain-19-landscape-references/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
