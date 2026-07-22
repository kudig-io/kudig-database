---
title: KEDA (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- orchestration
- keda
- prometheus
- grafana
- jaeger
- helm
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
- KEDA 是什么
- 如何 KEDA
trigger_keywords:
- KEDA
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KEDA

> **CNCF 状态**: Graduated | **类别**: Orchestration | **主要语言**: Go

## 概述

KEDA（Kubernetes Event-Driven Autoscaling）是一个 CNCF 毕业项目，由 Microsoft 和 Red Hat 联合开发。它是 Kubernetes 的事件驱动自动伸缩组件，扩展了 K8s 原生 HPA 的能力，支持基于外部事件源（Kafka 队列长度、Redis 队列深度、Prometheus 指标、AWS CloudWatch 等）的自动伸缩。KEDA 让无状态应用能够根据实际负载（而非 CPU/内存）进行弹性扩缩，特别适合事件驱动和 Serverless 架构。项目于 2023 年正式从 CNCF 毕业。

## Key Features（核心能力）

- **60+ Scalers**：内置支持 Kafka、RabbitMQ、Redis、AWS SQS、Azure Service Bus、Prometheus 等 60+ 事件源
- **Scale-to-Zero**：支持将 Deployment 缩放到零，真正实现 Serverless
- **ScaledObject CRD**：声明式定义伸缩目标和触发器
- **多触发器组合**：支持多个 Scaler 的 AND/OR 组合条件
- **External Scaler**：通过 gRPC 接口实现自定义 Scaler
- **Identity 支持**：支持 Azure Pod Identity、AWS IRSA 等云认证

## 架构与工作原理

KEDA 由三个核心组件构成：Operator（Controller）管理 ScaledObject 和 ScaledJob CRD 的生命周期；Metrics Adapter 作为 Kubernetes API Aggregation Layer 的扩展，将外部指标暴露为 K8s Custom Metrics；External Scaler 通过 gRPC 接口提供自定义指标源。KEDA Controller 监听 ScaledObject CRD，创建对应的 HPA 和触发器配置，Metrics Adapter 将外部指标转化为 HPA 可用的 Custom Metrics。

## K8s 集成

KEDA 深度集成 Kubernetes HPA 机制。ScaledObject CRD 定义目标 Deployment 和触发条件（Scaler），KEDA Controller 自动创建和管理 HPA 资源。Metrics Adapter 注册到 K8s API Server 的 Aggregation Layer，通过 /apis/external.metrics.k8s.io/ 端点提供外部指标。当队列无消息时，KEDA 将 Deployment replicas 设为 0；有新消息时快速从 0 扩展到 1 再按负载扩展。

## 生产用例

- **消息队列消费者伸缩**：根据 Kafka/RabbitMQ 队列深度自动伸缩消费者实例
- **Serverless 容器**：将普通 K8s Deployment 变为可缩放到零的 Serverless 服务
- **基于自定义指标伸缩**：使用 Prometheus 指标（如 QPS）而非 CPU/内存进行伸缩
- **数据库批处理**：根据待处理任务数动态调整 Worker 数量

## 安装与配置

```bash
# 🟢 添加 Helm 仓库
helm repo add kedacore https://kedacore.github.io/charts
helm repo update

# 🟢 安装 KEDA
helm install keda kedacore/keda \
  -n keda-system --create-namespace \
  --set resources.operator.limits.memory=256Mi \
  --set resources.metricServer.limits.memory=256Mi

# 🟢 验证安装
kubectl get pods -n keda-system
kubectl get crd | grep keda.sh

# 🟢 查看可用 Scalers
kubectl get scaledobjects -A
```

### ScaledObject CRD 示例

```yaml
# 基于 Kafka 队列深度伸缩
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: kafka-consumer-scaler
  namespace: production
spec:
  scaleTargetRef:
    name: kafka-consumer
  pollingInterval: 15
  cooldownPeriod: 300
  minReplicaCount: 0
  maxReplicaCount: 100
  triggers:
    - type: kafka
      metadata:
        bootstrapServers: kafka-0.kafka:9092
        consumerGroup: my-consumer-group
        topic: orders
        lagThreshold: "100"
        offsetResetPolicy: latest
---
# 基于 Prometheus 指标伸缩
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: prometheus-scaler
  namespace: production
spec:
  scaleTargetRef:
    name: web-frontend
  minReplicaCount: 2
  maxReplicaCount: 20
  triggers:
    - type: prometheus
      metadata:
        serverAddress: http://prometheus:9090
        metricName: http_requests_per_second
        threshold: "100"
        query: |
          sum(rate(http_requests_total{service="web-frontend"}[2m]))
---
# 基于 Redis 队列伸缩
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: redis-queue-scaler
  namespace: production
spec:
  scaleTargetRef:
    name: redis-worker
  minReplicaCount: 0
  maxReplicaCount: 50
  triggers:
    - type: redis
      metadata:
        address: redis-master:6379
        password: ""
        listName: task-queue
        listLength: "50"
        databaseIndex: "0"
---
# ScaledJob（批处理任务）
apiVersion: keda.sh/v1alpha1
kind: ScaledJob
metadata:
  name: batch-processor
  namespace: production
spec:
  jobTargetRef:
    template:
      spec:
        containers:
          - name: processor
            image: myorg/batch-processor:v1
        restartPolicy: Never
    backoffLimit: 3
  pollingInterval: 30
  maxReplicaCount: 10
  triggers:
    - type: rabbitmq
      metadata:
        host: amqp://rabbitmq:5672
        queueName: batch-tasks
        queueLength: "5"
```

## 运维操作

```bash
# 🟢 查看 ScaledObject 状态
kubectl get scaledobject -A
kubectl describe scaledobject kafka-consumer-scaler -n production

# 🟢 查看 KEDA 创建的 HPA
kubectl get hpa -A | grep keda

# 🟢 查看 KEDA 日志
kubectl logs -n keda-system -l app=keda-operator --tail=100

# 🟡 暂停伸缩
kubectl patch scaledobject kafka-consumer-scaler -n production --type=merge -p \
  '{"spec":{"triggers":[]}}'

# 🟡 调整最大副本数
kubectl patch scaledobject kafka-consumer-scaler -n production --type=merge -p \
  '{"spec":{"maxReplicaCount":200}}'

# 🔴 删除 ScaledObject（会删除对应的 HPA）
kubectl delete scaledobject kafka-consumer-scaler -n production
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 未伸缩 | Scaler 连接失败 | `kubectl describe scaledobject` | 检查事件源连接配置 |
| Scale-to-Zero 失败 | cooldownPeriod 未到 | 查看 ScaledObject events | 等待 cooldown 或调整参数 |
| HPA 未创建 | KEDA Operator 异常 | `kubectl get pods -n keda-system` | 重启 KEDA Operator |
| 指标查询失败 | Prometheus 不可达 | 查看 Metrics Adapter 日志 | 检查 Prometheus 地址 |

```bash
# 排查流程
# 1. 检查 KEDA 组件状态
kubectl get pods -n keda-system
kubectl logs -n keda-system -l app=keda-operator --tail=50

# 2. 检查 ScaledObject 事件
kubectl describe scaledobject <name> -n <ns> | grep -A10 Events

# 3. 检查 HPA 状态
kubectl get hpa -n <ns> | grep keda
kubectl describe hpa keda-hpa-<name> -n <ns>

# 4. 检查外部指标
kubectl get --raw "/apis/external.metrics.k8s.io/v1beta1" | jq .
```

## 生产案例

### 案例1：Kafka 消费者自动伸缩
- **场景**：订单处理服务需要根据 Kafka 队列深度自动伸缩，空闲时缩到 0
- **方案**：KEDA ScaledObject + Kafka Scaler；lagThreshold=100 触发扩容；cooldownPeriod=5min 后缩容；minReplicaCount=0 实现 Serverless
- **效果**：资源成本降低 60%（空闲时零 Pod），峰值处理能力 100 副本

### 案例2：基于 QPS 的 Web 服务伸缩
- **场景**：Web 前端需要根据实际 QPS 而非 CPU 进行伸缩
- **方案**：KEDA + Prometheus Scaler；基于 rate(http_requests_total[2m]) 指标；threshold=100 QPS/副本
- **效果**：伸缩响应时间从 5min(HPA) 缩短到 30s，用户体验显著改善

## 对比替代方案

| 维度 | KEDA | K8s HPA | Knative | 自定义 Operator |
|------|------|---------|---------|---------------|
| 事件源 | 60+ | CPU/Mem/Custom | 有限 | 自定义 |
| Scale-to-Zero | 支持 | 不支持 | 支持 | 自定义 |
| 学习曲线 | 低 | 低 | 中 | 高 |
| 保持 Deployment | 是 | 是 | 否 | 自定义 |
| 批处理任务 | ScaledJob | 无 | 无 | 自定义 |

## 检查清单

- [ ] KEDA 已部署且所有组件 Running
- [ ] ScaledObject 已在测试环境验证
- [ ] 事件源连接已验证（Kafka/Redis/Prometheus）
- [ ] minReplicaCount/maxReplicaCount 已合理设置
- [ ] cooldownPeriod 已配置（避免频繁伸缩）
- [ ] 监控告警已配置（伸缩事件/失败）
- [ ] Scale-to-Zero 场景已验证冷启动时间

## Related

- [[score]] — Score
- [[jaeger]] — Jaeger
- [[helm]] — Helm
- [[cloudevents]] — CloudEvents
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- observability/99-keda-event-driven-autoscaling-guide.md|99-keda-event-driven-autoscaling-guide]]
- keda
- [[实体/cncf-orchestration.md|[[CNCF 编排与应用管理项目全景|CNCF 编排与应用管理项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
