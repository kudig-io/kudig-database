---
title: Knative
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- istio
- kafka
- ingress
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Knative 是什么
- 如何 Knative
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Knative
- cncf
- landscape
---


# Knative

> **成熟度**: Graduated | **加入时间**: 2022-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://knative.dev |
| **GitHub** | https://github.com/knative |
| **文档** | https://knative.dev/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Serverless |

---

## 项目概述

### 简介
Knative 是基于 Kubernetes 的无服务器(Serverless)平台，提供了一套构建、部署和管理现代无服务器工作负载的组件。它抽象了底层基础设施复杂性，让开发者专注于代码。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2018-07 | 由 Google 联合 Pivotal、IBM 等发布 |
| 2019-03 | v0.4 发布，Eventing 成熟 |
| 2021-11 | 捐赠给 CNCF |
| 2022-03 | 成为 CNCF Incubating 项目 |
| 2023-11 | 晋升为 CNCF Graduated |

### 核心定位
Knative 是 Kubernetes 生态中构建 FaaS/Serverless 平台的事实标准，被 Google Cloud Run、Red Hat OpenShift Serverless、VMware Tanzu 等商业产品采用。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      Knative 架构                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Knative Serving                          ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         ││
│  │  │   Service   │  │   Route     │  │Configuration│         ││
│  │  │  (顶层抽象) │  │  (流量路由) │  │  (部署配置) │         ││
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         ││
│  │         │                │                │                 ││
│  │         └────────────────┼────────────────┘                 ││
│  │                          ▼                                  ││
│  │                   ┌─────────────┐                           ││
│  │                   │  Revision   │                           ││
│  │                   │  (不可变快照)│                           ││
│  │                   └─────────────┘                           ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Knative Eventing                         ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         ││
│  │  │   Source    │  │   Broker    │  │   Trigger   │         ││
│  │  │  (事件源)   │──►│  (事件总线) │──►│  (事件过滤) │         ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘         ││
│  │                          │                                  ││
│  │                          ▼                                  ││
│  │                   ┌─────────────┐                           ││
│  │                   │   Sink      │                           ││
│  │                   │  (事件消费) │                           ││
│  │                   └─────────────┘                           ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                     Kubernetes                               ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Serving 组件关系

```
┌─────────────────────────────────────────────────────────────────┐
│                  Knative Service 资源关系                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│                    ┌─────────────────┐                          │
│                    │     Service     │                          │
│                    │  (Knative 服务) │                          │
│                    └────────┬────────┘                          │
│                             │                                    │
│              ┌──────────────┴──────────────┐                    │
│              ▼                              ▼                    │
│    ┌─────────────────┐            ┌─────────────────┐          │
│    │  Configuration  │            │      Route      │          │
│    │   (部署配置)    │            │   (流量路由)    │          │
│    └────────┬────────┘            └────────┬────────┘          │
│             │                              │                     │
│             │ 每次更新创建新 Revision       │                     │
│             ▼                              │                     │
│    ┌─────────────────┐                     │                     │
│    │    Revision     │◄────────────────────┘                    │
│    │   (不可变快照)   │     流量分配到 Revision                   │
│    └────────┬────────┘                                          │
│             │                                                    │
│             ▼                                                    │
│    ┌─────────────────┐                                          │
│    │   Deployment    │                                          │
│    │   (K8s 资源)    │                                          │
│    └─────────────────┘                                          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 核心功能

### Knative Serving

#### Service 定义
```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: hello-world
  namespace: default
spec:
  template:
    metadata:
      annotations:
        # 自动扩缩配置
        autoscaling.knative.dev/min-scale: "0"
        autoscaling.knative.dev/max-scale: "10"
        autoscaling.knative.dev/target: "100"
    spec:
      containers:
        - image: gcr.io/knative-samples/helloworld-go
          ports:
            - containerPort: 8080
          env:
            - name: TARGET
              value: "Knative"
          resources:
            requests:
              memory: "64Mi"
              cpu: "100m"
            limits:
              memory: "128Mi"
              cpu: "500m"
```

#### 流量分割
```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: canary-demo
spec:
  template:
    spec:
      containers:
        - image: myapp:v2
  traffic:
    # 90% 流量到最新版本
    - latestRevision: true
      percent: 90
    # 10% 流量到指定版本 (金丝雀)
    - revisionName: canary-demo-v1
      percent: 10
```

#### 蓝绿部署
```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: blue-green
spec:
  template:
    metadata:
      name: blue-green-v2  # 新版本
    spec:
      containers:
        - image: myapp:v2
  traffic:
    # 100% 流量到 v1 (蓝)
    - revisionName: blue-green-v1
      percent: 100
    # v2 (绿) 可通过特定 URL 访问
    - revisionName: blue-green-v2
      percent: 0
      tag: green
```

### Knative Eventing

#### 事件架构
```
┌─────────────────────────────────────────────────────────────────┐
│                    Knative Eventing 流程                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Event Sources                Broker              Subscribers   │
│  ┌──────────────┐                                               │
│  │ PingSource   │──┐                         ┌──────────────┐   │
│  └──────────────┘  │      ┌──────────┐       │  Service A   │   │
│  ┌──────────────┐  │      │          │       │  (filter:    │   │
│  │ KafkaSource  │──┼─────►│  Broker  │──────►│  type=order) │   │
│  └──────────────┘  │      │          │       └──────────────┘   │
│  ┌──────────────┐  │      └──────────┘       ┌──────────────┐   │
│  │ GitHubSource │──┘           │             │  Service B   │   │
│  └──────────────┘              │             │  (filter:    │   │
│                                └────────────►│  type=user)  │   │
│                                              └──────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

#### Broker 和 Trigger
```yaml
# 创建 Broker
apiVersion: eventing.knative.dev/v1
kind: Broker
metadata:
  name: default
  namespace: event-demo

---
# 事件源：定时触发
apiVersion: sources.knative.dev/v1
kind: PingSource
metadata:
  name: ping-source
spec:
  schedule: "*/1 * * * *"  # 每分钟
  contentType: "application/json"
  data: '{"message": "Hello from PingSource"}'
  sink:
    ref:
      apiVersion: eventing.knative.dev/v1
      kind: Broker
      name: default

---
# Trigger：过滤并路由事件
apiVersion: eventing.knative.dev/v1
kind: Trigger
metadata:
  name: my-trigger
spec:
  broker: default
  filter:
    attributes:
      type: dev.knative.sources.ping
  subscriber:
    ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: event-display
```

---

## 自动扩缩

### 扩缩策略

```
请求数变化                        Pod 数量
    │                                │
100 ┤ ████████████                   │     ████████
    │             ████               │ ████        ████
 50 ┤                 ████           │             
    │                     ████       │                 ████
  0 ┤─────────────────────────────►  │─────────────────────────────►
    0   1   2   3   4   5   时间(分) 0   1   2   3   4   5   时间(分)
    
    缩容到零 (Scale to Zero)          Pod 从 0 快速扩展
```

### 配置选项

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: autoscale-demo
spec:
  template:
    metadata:
      annotations:
        # KPA (Knative Pod Autoscaler) 配置
        autoscaling.knative.dev/class: "kpa.autoscaling.knative.dev"
        
        # 扩缩指标：concurrency 或 rps
        autoscaling.knative.dev/metric: "concurrency"
        
        # 目标并发数
        autoscaling.knative.dev/target: "100"
        
        # 缩容到零
        autoscaling.knative.dev/min-scale: "0"
        
        # 最大实例数
        autoscaling.knative.dev/max-scale: "50"
        
        # 缩容延迟 (秒)
        autoscaling.knative.dev/scale-down-delay: "30s"
        
        # 初始实例数
        autoscaling.knative.dev/initial-scale: "1"
```

---

## 安装部署

### 使用 YAML 安装

```bash
# 安装 Knative Serving
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.12.0/serving-crds.yaml
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.12.0/serving-core.yaml

# 安装网络层 (选择一个)
# Kourier (轻量级)
kubectl apply -f https://github.com/knative/net-kourier/releases/download/knative-v1.12.0/kourier.yaml

# 配置 Kourier 为默认网络
kubectl patch configmap/config-network \
  --namespace knative-serving \
  --type merge \
  --patch '{"data":{"ingress-class":"kourier.ingress.networking.knative.dev"}}'

# 安装 Knative Eventing
kubectl apply -f https://github.com/knative/eventing/releases/download/knative-v1.12.0/eventing-crds.yaml
kubectl apply -f https://github.com/knative/eventing/releases/download/knative-v1.12.0/eventing-core.yaml
```

### 使用 Knative Operator

```yaml
apiVersion: operator.knative.dev/v1beta1
kind: KnativeServing
metadata:
  name: knative-serving
  namespace: knative-serving
spec:
  version: "1.12.0"
  ingress:
    kourier:
      enabled: true
  config:
    network:
      ingress-class: "kourier.ingress.networking.knative.dev"
    autoscaler:
      enable-scale-to-zero: "true"
      scale-to-zero-grace-period: "30s"
```

---

## 使用场景

### 1. HTTP 服务自动扩缩
```yaml
# 自动扩缩的 Web 服务
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: web-api
spec:
  template:
    spec:
      containers:
        - image: myregistry/web-api:latest
          ports:
            - containerPort: 8080
```

### 2. 事件驱动处理
```yaml
# Kafka 事件消费
apiVersion: sources.knative.dev/v1beta1
kind: KafkaSource
metadata:
  name: kafka-source
spec:
  bootstrapServers:
    - my-kafka:9092
  topics:
    - orders
  consumerGroup: knative-group
  sink:
    ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: order-processor
```

### 3. 定时任务
```yaml
# Cron 定时任务
apiVersion: sources.knative.dev/v1
kind: PingSource
metadata:
  name: daily-report
spec:
  schedule: "0 9 * * *"  # 每天 9 点
  data: '{"task": "generate-report"}'
  sink:
    ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: report-generator
```

---

## 生态集成

| 组件 | 说明 |
|:---|:---|
| **Istio** | 服务网格集成 |
| **Contour** | Ingress 控制器 |
| **Kourier** | 轻量级 Ingress |
| **Kafka** | 事件源 |
| **Prometheus** | 监控指标 |
| **Tekton** | CI/CD 流水线 |

---

## 参考资源

- [官方文档](https://knative.dev/docs)
- [GitHub Repo](https://github.com/knative)
- [CNCF 项目页面](https://www.cncf.io/projects/knative/)
- [Knative Samples](https://github.com/knative/docs/tree/main/code-samples)
- [Google Cloud Run 文档](https://cloud.google.com/run/docs)

---

**维护者**: Kudig Team | **许可证**: MIT
