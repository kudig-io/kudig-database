---
title: CNCF Incubating Projects
description: CNCF 孵化项目参考 — 正在走向成熟的高潜力云原生项目深度解析
summary: CNCF 孵化阶段项目全景，涵盖 Dapr、Backstage、OpenTelemetry、Flux、KubeEdge 等
category: reference
tags:
- cncf
- incubating
- dapr
- backstage
- flux
- opentelemetry
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
---
# CNCF 孵化项目 Incubating

> CNCF 孵化阶段项目——已证明生产价值，正在走向毕业。

## 孵化项目全景

| 项目 | 类别 | 核心能力 | 生产采用度 |
|------|------|----------|-----------|
| OpenTelemetry | 可观测性 | 统一遥测数据采集（Traces/Metrics/Logs） | 极高 |
| Flux | 持续交付 | GitOps 持续交付 | 高 |
| Dapr | 应用运行时 | 分布式应用构建块 | 高 |
| Backstage | 平台工程 | 开发者门户/服务目录 | 高 |
| KubeEdge | 边缘计算 | 云边协同 | 中-高 |
| Knative | Serverless | 事件驱动 Serverless | 中-高 |
| OpenKruise | 应用定义 | 增强工作负载控制器 | 中 |
| KEDA | 自动缩放 | 事件驱动自动缩放 | 高 |
| Crossplane | 基础设施 | 云基础设施即 K8s API | 中-高 |
| Litmus | 混沌工程 | 云原生混沌工程 | 中 |
| Open Policy Agent | 安全 | 通用策略引擎 | 高 |
| Thanos | 可观测性 | Prometheus 长期存储与联邦 | 高 |
| Cortex | 可观测性 | Prometheus 水平扩展 | 中-高 |
| Strimzi | 流处理 | Kafka on K8s Operator | 中 |
| TiKV | 存储 | 分布式事务 KV 存储 | 中 |
| Rook | 存储 | 存储编排（Ceph） | 中-高 |
| Longhorn | 存储 | 轻量分布式块存储 | 中 |
| Volcano | 批处理 | 高性能批处理调度 | 中 |
| KubeVirt | 虚拟化 | VM on K8s | 中 |
| Cert-Manager | 安全 | TLS 证书自动化管理 | 极高 |

## 重点项目深度解析

### OpenTelemetry

```yaml
# OTel Collector 部署模式
apiVersion: opentelemetry.io/v1beta1
kind: OpenTelemetryCollector
metadata:
  name: otel-collector
  namespace: observability
spec:
  mode: daemonset
  config:
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318
    processors:
      batch:
        timeout: 5s
        send_batch_size: 1024
      memory_limiter:
        check_interval: 1s
        limit_mib: 512
    exporters:
      prometheus:
        endpoint: 0.0.0.0:8889
      otlp/tempo:
        endpoint: tempo:4317
        tls:
          insecure: true
    service:
      pipelines:
        traces:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [otlp/tempo]
        metrics:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [prometheus]
```

### Flux GitOps

```yaml
# Flux Kustomization 声明式部署
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: apps
  namespace: flux-system
spec:
  interval: 10m
  path: ./apps/production
  prune: true
  sourceRef:
    kind: GitRepository
    name: flux-system
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: backend
      namespace: production
  postBuild:
    substitute:
      ENVIRONMENT: production
```

### KEDA 事件驱动缩放

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: consumer-scaler
spec:
  scaleTargetRef:
    name: event-consumer
  minReplicaCount: 1
  maxReplicaCount: 100
  triggers:
    - type: kafka
      metadata:
        bootstrapServers: kafka:9092
        consumerGroup: my-group
        topic: events
        lagThreshold: "50"
    - type: prometheus
      metadata:
        serverAddress: http://prometheus:9090
        metricName: http_requests_total
        query: sum(rate(http_requests_total{deployment="my-deployment"}[2m]))
        threshold: "100"
```

### Crossplane 基础设施即代码

```yaml
apiVersion: ec2.aws.upbound.io/v1beta1
kind: Instance
metadata:
  name: web-server
  annotations:
    crossplane.io/external-name: i-1234567890
spec:
  forProvider:
    region: us-east-1
    instanceType: t3.medium
    ami: ami-0c55b159cbfafe1f0
    subnetIdRef:
      name: my-subnet
  providerConfigRef:
    name: aws-provider
---
apiVersion: rds.aws.upbound.io/v1beta1
kind: Instance
metadata:
  name: postgres
spec:
  forProvider:
    region: us-east-1
    instanceClass: db.t3.medium
    engine: postgres
    engineVersion: "15"
    masterUsername: admin
    allocatedStorage: 100
  providerConfigRef:
    name: aws-provider
```

## 孵化→毕业路径

| 阶段 | 要求 |
|------|------|
| 进入孵化 | 有生产采用、治理完善、安全审计通过 |
| 毕业准备 | 广泛生产采用、社区多样性、持续维护 |
| 毕业 | TOC 投票通过、满足所有毕业标准 |

## 选型建议

- **可观测性统一** → OpenTelemetry（事实标准）
- **GitOps 交付** → Flux 或 ArgoCD（均已毕业/孵化）
- **开发者门户** → Backstage（插件生态丰富）
- **事件驱动缩放** → KEDA（轻量、多触发器）
- **多云基础设施** → Crossplane（K8s 原生 IaC）
- **边缘计算** → KubeEdge（华为主导、国内生态好）

## Related

- [[生态参考/CNCF全景/graduated/index.md|毕业项目]]
- [[生态参考/CNCF全景/sandbox/index.md|沙箱项目]]
- [[生态参考/index.md|生态参考总索引]]
