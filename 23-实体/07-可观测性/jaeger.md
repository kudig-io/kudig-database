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

Jaeger 通过 Jaeger Operator 以 Kubernetes 原生方式部署。Jaeger CRD 定义 Jaeger 实例配置（存储后端、采样策略、UI 访问等），Operator 自动部署 Collector、Query、UI 组件。应用通过 OpenTelemetry SDK 将 Trace 数据发送到 OTel Collector（DaemonSet 或 Sidecar），Collector 将数据转发给 Jaeger Collector。Jaeger UI 通过 Kubernetes Service 暴露，运维人员可以搜索和分析 Trace。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 Deployment/Service 模式和 Prometheus 可观测性栈集成。

## 生产场景

1. **微服务延迟诊断**: 通过火焰图定位微服务调用链中的延迟瓶颈
2. **错误根因定位**: 通过 Trace 关联 Logs 和 Metrics，快速定位生产问题
3. **服务拓扑发现**: 自动构建微服务依赖关系图，发现意外的服务耦合
4. **性能基准测试**: 在压测中追踪请求链路，量化各服务的处理延迟

## 安装与配置

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

### 采样策略配置

```yaml
apiVersion: jaegertracing.io/v1
kind: Jaeger
metadata:
  name: production
  namespace: observability
spec:
  strategy: production
  collector:
    maxReplicas: 5
    resources:
      requests:
        cpu: 500m
        memory: 512Mi
      limits:
        cpu: "2"
        memory: 2Gi
  query:
    replicas: 2
  sampling:
    options:
      default_strategy:
        type: probabilistic
        param: 0.01
      service_strategies:
      - service: payment-service
        type: probabilistic
        param: 0.1
        operation_strategies:
        - operation: /api/checkout
          type: probabilistic
          param: 1.0
      - service: frontend
        type: ratelimiting
        param: 100
```

### OTel Collector 集成

```yaml
apiVersion: opentelemetry.io/v1alpha1
kind: OpenTelemetryCollector
metadata:
  name: otel-collector
  namespace: observability
spec:
  mode: daemonset
  config: |
    receivers:
      otlp:
        protocols:
          grpc:
          http:
    processors:
      batch:
        timeout: 5s
      memory_limiter:
        check_interval: 1s
        limit_mib: 512
    exporters:
      otlp/jaeger:
        endpoint: jaeger-collector-headless.observability.svc:4317
        tls:
          insecure: true
    service:
      pipelines:
        traces:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [otlp/jaeger]
```

## 运维操作

```bash
# 🟢 查看 Jaeger 实例状态
kubectl get jaeger -n observability
kubectl describe jaeger production -n observability

# 🟢 查看 Collector 指标
curl http://jaeger-collector.observability.svc:14269/metrics | grep jaeger_collector

# 🟢 检查 ES 存储健康
curl http://elasticsearch:9200/_cluster/health?pretty

# 🟡 调整采样率
kubectl patch jaeger production -n observability --type=merge -p '{"spec":{"sampling":{"options":{"default_strategy":{"type":"probabilistic","param":0.05}}}}}'

# 🟡 扩容 Collector
kubectl scale deployment production-collector -n observability --replicas=5

# 🔴 清理过期索引（ES 后端）
curl -X DELETE "http://elasticsearch:9200/jaeger-span-$(date -d '-7 days' +%Y.%m.%d)"

# 🔴 删除 Jaeger 实例
kubectl delete jaeger production -n observability
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| UI 无 Trace 数据 | 采样率过低/Collector 异常 | `kubectl logs -l app.kubernetes.io/component=collector -n observability` | 调整采样率或修复 Collector |
| Collector OOM | 流量突增超出内存限制 | `kubectl top pods -n observability` | 增加 memory limits |
| ES 写入超时 | 索引过多/磁盘满 | `curl ES:9200/_cat/indices?v` | 清理旧索引，扩容磁盘 |
| 服务未上报 Span | SDK 配置错误 | 检查 `OTEL_EXPORTER_OTLP_ENDPOINT` 环境变量 | 修正 OTLP endpoint |
| Query 响应慢 | ES 查询超时 | `kubectl logs production-query -n observability` | 优化索引/增加资源 |

```
排查流程:
├── Trace 数据缺失
│   ├── kubectl get pods -n observability → 组件运行状态
│   ├── 检查应用 OTEL_EXPORTER 环境变量 → SDK 配置
│   ├── kubectl logs collector → 接收端错误
│   └── curl ES/_cluster/health → 存储健康
├── 性能问题
│   ├── kubectl top pods → 资源使用
│   ├── ES _nodes/stats → JVM 堆使用率
│   └── jaeger_collector_spans_received_total → 流量指标
└── 存储问题
    ├── ES _cat/indices → 索引数量和大小
    ├── ES _cluster/allocation/explain → 分片分配
    └── 磁盘使用率 → 清理或扩容
```

## 生产案例

### 案例1: 电商大促期间 Trace 数据丢失

- **场景**: 双十一期间订单量激增 10 倍，Jaeger UI 显示大量 Trace 缺失
- **排查**: Collector Pod 频繁 OOMKilled，`jaeger_collector_spans_dropped_total` 指标飙升
- **方案**: 
  1. Collector 扩容至 10 副本，memory limits 提升至 4Gi
  2. 启用 Kafka 缓冲层（strategy: streaming），解耦 Collector 与存储
  3. 非核心服务采样率降至 0.001，核心支付链路保持 1.0
- **效果**: Trace 完整率从 60% 恢复至 99.5%，Collector 零 OOM

### 案例2: ES 索引膨胀导致查询超时

- **场景**: 运行 3 个月后 Jaeger Query 响应时间从 2s 退化至 30s+
- **排查**: ES 集群累积 900+ 索引，总数据量 2TB，JVM GC 频繁
- **方案**:
  1. 部署 Curator 每日清理 7 天前索引
  2. 启用 ILM（Index Lifecycle Management）自动 rollover
  3. 热温冷架构：7天内 SSD，7-30天 HDD
- **效果**: 查询 P99 恢复至 3s，存储成本降低 70%

## 对比

| 特性 | Jaeger | Zipkin | Tempo | Datadog APM |
|------|--------|--------|-------|-------------|
| CNCF 状态 | Graduated | 非 CNCF | Incubating (Grafana) | ❌ 商业 |
| 存储后端 | Cassandra/ES/Kafka | MySQL/ES | 对象存储 | SaaS |
| 火焰图 | ✅ | ⚠️ | ✅ | ✅ |
| OTel 兼容 | ✅ | ✅ | ✅ | ✅ |
| 运维复杂度 | 中 | 低 | 低 | 无 |
| 适用场景 | 大规模生产追踪 | 轻量开发调试 | Grafana 生态 | 全托管企业 |

## 架构定位

在 CNCF 生态中，Jaeger 属于 **Observability** 类别，为云原生应用提供分布式链路追踪能力。

## 参考链接

- [[22-概念/06-可观测性/observability-pillars.md|observability-pillars]]
- [[22-概念/04-存储/storage-model.md|storage-model]]

## Related
- [[22-概念/11-交叉分析/可观测性支柱 × Prometheus-Grafana.md|可观测性支柱 × Prometheus-Grafana]] — 综合

- [[knative]] — Knative
- [[konveyor]] — Konveyor
- [[bfe]] — BFE
- [[score]] — Score
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- jaeger
- [[23-实体/k8s-observability-ecosystem.md|[[可观测性体系：指标、日志、链路追踪与混沌工程|可观测性体系：指标、日志、链路追踪与混沌工程]]]] — Cross-reference
- [[23-实体/15-参考与索引/observability-terms.md|K8s 可观测性术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[22-概念/10-最佳实践/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[22-概念/12-研究/observability-stack-evolution.md|可观测性栈演进]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
