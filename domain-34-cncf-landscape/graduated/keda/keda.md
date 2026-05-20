---
title: KEDA
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- redis
- kafka
- hpa
- statefulset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KEDA 是什么
- 如何 KEDA
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KEDA
- cncf
- landscape
---


# KEDA

> **成熟度**: Graduated | **加入时间**: 2020-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://keda.sh |
| **GitHub** | https://github.com/kedacore/keda |
| **文档** | https://keda.sh/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Orchestration & Management |

---

## 项目概述

### 简介
KEDA (Kubernetes Event-driven Autoscaling) 是一个轻量级的事件驱动自动扩缩组件，扩展了 Kubernetes HPA 的能力。它支持基于外部事件源（如消息队列、数据库、HTTP 请求等）进行 Pod 自动扩缩，并支持缩容到零。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2019-05 | 由 Microsoft 和 Red Hat 联合发布 |
| 2020-03 | 加入 CNCF Sandbox |
| 2021-08 | 晋升为 CNCF Incubating |
| 2023-08 | 晋升为 CNCF Graduated |

### 核心定位
KEDA 是 Kubernetes 原生的事件驱动扩缩解决方案，使任何容器工作负载都能根据事件负载自动扩缩，是构建事件驱动架构和 Serverless 应用的关键组件。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      KEDA 架构                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Kubernetes Cluster                        ││
│  │                                                              ││
│  │  ┌──────────────────────────────────────────────────────┐   ││
│  │  │                     KEDA                              │   ││
│  │  │  ┌────────────────┐  ┌────────────────┐              │   ││
│  │  │  │  KEDA Operator │  │ Metrics Server │              │   ││
│  │  │  │                │  │ (External      │              │   ││
│  │  │  │ • ScaledObject │  │  Metrics API)  │              │   ││
│  │  │  │ • ScaledJob    │  │                │              │   ││
│  │  │  └───────┬────────┘  └───────┬────────┘              │   ││
│  │  │          │                   │                        │   ││
│  │  │          │    ┌──────────────┘                        │   ││
│  │  │          ▼    ▼                                       │   ││
│  │  │  ┌────────────────┐                                   │   ││
│  │  │  │     HPA        │ ◄── KEDA 创建和管理               │   ││
│  │  │  └───────┬────────┘                                   │   ││
│  │  │          │                                            │   ││
│  │  │          ▼                                            │   ││
│  │  │  ┌────────────────┐  ┌────────────────┐              │   ││
│  │  │  │  Deployment    │  │     Job        │              │   ││
│  │  │  │  (ScaledObject)│  │ (ScaledJob)    │              │   ││
│  │  │  └────────────────┘  └────────────────┘              │   ││
│  │  └──────────────────────────────────────────────────────┘   ││
│  │                         │                                    ││
│  │                         │ Scalers (60+)                      ││
│  │                         ▼                                    ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐           │
│  │  Kafka   │ │ RabbitMQ │ │  Redis   │ │   AWS    │           │
│  │  Queue   │ │  Queue   │ │  Lists   │ │  SQS     │           │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 扩缩流程

```
┌─────────────────────────────────────────────────────────────────┐
│                    KEDA 扩缩决策流程                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  事件源                KEDA               Kubernetes            │
│                                                                  │
│  ┌──────────┐                                                   │
│  │  Kafka   │    1. 查询指标                                    │
│  │ (100 msg)│ ◄────────────────── KEDA Operator                 │
│  └──────────┘                           │                       │
│       │                                 │                       │
│       │ 2. 返回消息数量                 │                       │
│       └────────────────────────────────►│                       │
│                                         │                       │
│                    3. 计算目标副本数    │                       │
│                    100 msgs / 10 = 10   │                       │
│                                         │                       │
│                    4. 更新 HPA          │                       │
│                          │              │                       │
│                          ▼              │                       │
│                    ┌──────────┐         │                       │
│                    │   HPA    │         │                       │
│                    │ target:10│         │                       │
│                    └────┬─────┘         │                       │
│                         │               │                       │
│                    5. 扩缩 Pod          │                       │
│                         │               │                       │
│                         ▼               │                       │
│                    ┌──────────┐         │                       │
│                    │Deployment│         │                       │
│                    │ 10 Pods  │         │                       │
│                    └──────────┘         │                       │
│                                                                  │
│  特殊情况: 缩容到零                                              │
│  当消息数 = 0 时，KEDA 将 Deployment 缩容到 0                   │
│  当消息数 > 0 时，KEDA 将 Deployment 从 0 扩到 minReplicaCount  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 安装部署

```bash
# 使用 Helm 安装
helm repo add kedacore https://kedacore.github.io/charts
helm repo update

helm install keda kedacore/keda \
  --namespace keda \
  --create-namespace

# 验证安装
kubectl get pods -n keda
```

---

## 核心资源

### ScaledObject (用于 Deployment/StatefulSet)

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: kafka-consumer-scaler
  namespace: default
spec:
  # 目标工作负载
  scaleTargetRef:
    name: kafka-consumer
    kind: Deployment
  
  # 副本数范围
  minReplicaCount: 0   # 支持缩容到零
  maxReplicaCount: 50
  
  # 冷却时间
  cooldownPeriod: 60   # 缩容前等待时间 (秒)
  
  # 扩缩触发器
  triggers:
    - type: kafka
      metadata:
        bootstrapServers: kafka.default.svc:9092
        consumerGroup: my-consumer-group
        topic: my-topic
        lagThreshold: "100"     # 每 100 条消息一个 Pod
        activationLagThreshold: "0"  # 激活阈值
```

### ScaledJob (用于 Job)

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledJob
metadata:
  name: batch-processor
spec:
  jobTargetRef:
    parallelism: 1
    completions: 1
    backoffLimit: 3
    template:
      spec:
        containers:
          - name: processor
            image: batch-processor:latest
        restartPolicy: Never
  
  pollingInterval: 30
  maxReplicaCount: 100
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 3
  
  triggers:
    - type: aws-sqs-queue
      metadata:
        queueURL: https://sqs.us-west-2.amazonaws.com/xxx/my-queue
        queueLength: "5"
        awsRegion: us-west-2
```

---

## 常用 Scalers

### Kafka

```yaml
triggers:
  - type: kafka
    metadata:
      bootstrapServers: kafka:9092
      consumerGroup: my-group
      topic: orders
      lagThreshold: "50"
      # 认证 (可选)
      sasl: plaintext
      username: user
    authenticationRef:
      name: kafka-auth
```

### RabbitMQ

```yaml
triggers:
  - type: rabbitmq
    metadata:
      host: amqp://guest:guest@rabbitmq:5672/
      queueName: tasks
      queueLength: "10"
      mode: QueueLength  # 或 MessageRate
```

### Redis Lists

```yaml
triggers:
  - type: redis
    metadata:
      address: redis:6379
      listName: jobs
      listLength: "5"
      databaseIndex: "0"
```

### Prometheus

```yaml
triggers:
  - type: prometheus
    metadata:
      serverAddress: http://prometheus:9090
      metricName: http_requests_total
      query: sum(rate(http_requests_total{service="api"}[2m]))
      threshold: "100"
```

### Cron

```yaml
triggers:
  - type: cron
    metadata:
      timezone: Asia/Shanghai
      start: 0 6 * * *      # 每天 6:00
      end: 0 20 * * *       # 每天 20:00
      desiredReplicas: "10"
```

### HTTP (基于请求数)

```yaml
triggers:
  - type: prometheus
    metadata:
      serverAddress: http://prometheus:9090
      query: sum(rate(nginx_ingress_controller_requests{service="my-app"}[1m]))
      threshold: "100"  # 每秒 100 请求一个 Pod
```

---

## 高级功能

### TriggerAuthentication

```yaml
# 认证配置
apiVersion: keda.sh/v1alpha1
kind: TriggerAuthentication
metadata:
  name: kafka-auth
spec:
  secretTargetRef:
    - parameter: username
      name: kafka-credentials
      key: username
    - parameter: password
      name: kafka-credentials
      key: password

---
# 在 ScaledObject 中引用
spec:
  triggers:
    - type: kafka
      authenticationRef:
        name: kafka-auth
```

### 多触发器组合

```yaml
spec:
  triggers:
    # 消息队列触发
    - type: kafka
      metadata:
        topic: orders
        lagThreshold: "100"
    
    # CPU 触发
    - type: cpu
      metricType: Utilization
      metadata:
        value: "70"
    
    # 定时触发
    - type: cron
      metadata:
        timezone: UTC
        start: 0 9 * * 1-5
        end: 0 18 * * 1-5
        desiredReplicas: "5"
```

### 暂停扩缩

```yaml
metadata:
  annotations:
    # 暂停自动扩缩
    autoscaling.keda.sh/paused: "true"
    # 暂停时的副本数
    autoscaling.keda.sh/paused-replicas: "3"
```

---

## 监控

```yaml
# KEDA 暴露 Prometheus 指标
# keda_scaler_active - 活跃的 Scaler 数
# keda_scaler_metrics_value - 当前指标值
# keda_scaled_object_errors - 错误计数

# ServiceMonitor
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: keda-metrics
spec:
  selector:
    matchLabels:
      app: keda-operator
  endpoints:
    - port: metricsservice
```

---

## 参考资源

- [官方文档](https://keda.sh/docs)
- [GitHub Repo](https://github.com/kedacore/keda)
- [CNCF 项目页面](https://www.cncf.io/projects/keda/)
- [Scalers 列表](https://keda.sh/docs/scalers/)
- [示例](https://github.com/kedacore/samples)

---

**维护者**: Kudig Team | **许可证**: MIT
