---
title: Jaeger (entities)
description: '## 概述'
summary: 'Jaeger（发音类似"耶格"）是 CNCF 毕业项目，为云原生应用提供分布式链路追踪能力。'
category: entities
tags:
- k8s
- cncf
- observability
- jaeger
- prometheus
- grafana
- kafka
- elasticsearch
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Jaeger 是什么
- 如何 Jaeger
trigger_keywords:
- Jaeger
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Jaeger

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

Jaeger（发音类似"耶格"）是受 Google Dapper 论文启发的分布式链路追踪系统，由 Uber 开发，2017 年加入 CNCF 孵化，2019 年正式毕业（Graduated）。它为云原生微服务架构提供端到端的分布式追踪能力，帮助开发者理解请求在微服务链路中的流转路径，快速定位延迟瓶颈和错误根因。Jaeger 兼容 OpenTracing 和 OpenTelemetry 标准，支持自动和手动埋点。它提供了丰富的后端存储选择（Cassandra、Elasticsearch、Kafka），适合不同规模的部署。Jaeger UI 提供直观的火焰图（Flame Graph）和服务依赖拓扑图，帮助可视化分析微服务调用链。

## 核心能力

- **分布式追踪**: 跨微服务的请求追踪，记录每个服务的处理时间和状态
- **根因分析**: 通过 Trace ID 快速定位延迟瓶颈和错误来源
- **服务依赖图**: 自动构建微服务调用关系拓扑图（Service Dependencies）
- **性能分析**: Span 级别的延迟分析和慢查询定位
- **多存储后端**: 支持 Cassandra、Elasticsearch、Kafka、Badger（内存）
- **OpenTelemetry 兼容**: 支持 OTLP 协议直接接收 OpenTelemetry 数据

## 架构

Jaeger 采用微服务化架构：

- **Jaeger Agent (deprecated)**: 节点上的 UDP 代理（已被 OTel Collector 替代）
- **Jaeger Collector**: 接收 Trace 数据的组件，支持 OTLP/Jaeger/Zipkin 协议
- **Jaeger Storage**: 后端存储（Cassandra/Elasticsearch/Kafka/Badger）
- **Jaeger Query**: 查询服务，为 Jaeger UI 和 API 提供数据查询
- **Jaeger UI**: Web 界面，提供火焰图、服务拓扑图和搜索功能
- **Spark/Flink Job (可选)**: 从存储中计算服务依赖关系拓扑
- **Jaeger Ingester (可选)**: 从 Kafka 消费 Trace 数据写入存储

数据流：`应用 (OTel SDK) → OTel Collector → Jaeger Collector → Storage → UI`

## K8s 集成

Jaeger 通过 Jaeger Operator 以 Kubernetes 原生方式部署。Jaeger CRD 定义 Jaeger 实例配置（存储后端、采样策略、UI 访问等），Operator 自动部署 Collector、Query、UI 组件。应用通过 OpenTelemetry SDK 将 Trace 数据发送到 OTel Collector（DaemonSet 或 Sidecar），Collector 将数据转发给 Jaeger Collector。Jaeger UI 通过 Kubernetes Service 暴露，运维人员可以搜索和分析 Trace。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 Deployment/Service 模式和 Prometheus 可观测性栈集成。

## 生产场景

1. **微服务延迟诊断**: 通过火焰图定位微服务调用链中的延迟瓶颈
2. **错误根因定位**: 通过 Trace 关联 Logs 和 Metrics，快速定位生产问题
3. **服务拓扑发现**: 自动构建微服务依赖关系图，发现意外的服务耦合
4. **性能基准测试**: 在压测中追踪请求链路，量化各服务的处理延迟

## 安装

```bash
# 安装 Jaeger Operator
kubectl create namespace observability
kubectl apply -f https://github.com/jaegertracing/jaeger-operator/releases/latest/download/operator.yaml -n observability

# 创建 Jaeger 实例（使用内存存储，适合开发）
kubectl apply -f - <<EOF
apiVersion: jaegertracing.io/v1
kind: Jaeger
metadata:
  name: simplest
  namespace: observability
EOF

# 创建 Jaeger 实例（使用 Elasticsearch，适合生产）
kubectl apply -f - <<EOF
apiVersion: jaegertracing.io/v1
kind: Jaeger
metadata:
  name: production
  namespace: observability
spec:
  strategy: production
  storage:
    type: elasticsearch
    options:
      es:
        server-urls: http://elasticsearch:9200
    secretName: jaeger-es-secret
  ingress:
    enabled: true
  sampling:
    options:
      default_strategy:
        type: probabilistic
        param: 0.01
EOF

# 访问 Jaeger UI
kubectl port-forward svc/production-query -n observability 16686:16686
# 打开 http://localhost:16686
```

## 对比

| 特性 | Jaeger | Zipkin | Tempo | Datadog APM |
|------|--------|--------|-------|-------------|
| CNCF 状态 | Graduated | 非 CNCF | Incubating (Grafana) | ❌ 商业 |
| 存储后端 | Cassandra/ES/Kafka | MySQL/ES | 对象存储 | SaaS |
| 火焰图 | ✅ | ⚠️ | ✅ | ✅ |
| OTel 兼容 | ✅ | ✅ | ✅ | ✅ |

## 架构定位

在 CNCF 生态中，Jaeger 属于 **Observability** 类别，为云原生应用提供分布式链路追踪能力。

## 参考链接

- [[概念/observability-pillars.md|observability-pillars]]
- [[概念/storage-model.md|storage-model]]

## Related
- [[概念/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]] — 综合

- [[knative]] — Knative
- [[konveyor]] — Konveyor
- [[bfe]] — BFE
- [[score]] — Score
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- jaeger
- [[实体/k8s-observability-ecosystem.md|[[可观测性体系：指标、日志、链路追踪与混沌工程|可观测性体系：指标、日志、链路追踪与混沌工程]]]] — Cross-reference
- [[实体/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[实体/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[概念/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[概念/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[技能/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[实体/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
