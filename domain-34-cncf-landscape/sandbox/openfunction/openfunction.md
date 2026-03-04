# OpenFunction

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://openfunction.dev/ |
| **GitHub** | https://github.com/OpenFunction/OpenFunction |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

OpenFunction 是一个云原生 FaaS (Function as a Service) 平台，使开发者能够专注于业务逻辑。它集成了 Knative、KEDA、Dapr、Shipwright 等云原生项目，提供从源码构建到函数运行的完整生命周期管理，支持同步和异步函数运行时。

### 核心特性

- **多运行时**: 同步函数 (Knative Serving) 和异步函数 (KEDA + Dapr)
- **源码构建**: 集成 Shipwright/Buildpacks 实现 Source-to-Image
- **事件驱动**: 基于 KEDA 的事件源触发，支持 Kafka、Cron、HTTP 等
- **Dapr 集成**: 利用 Dapr 的 Binding 和 Pub/Sub 实现异步消息处理
- **多语言 SDK**: Go, Node.js, Python, Java 函数 SDK
- **自动伸缩**: 同步函数从 0 缩放，异步函数基于事件积压伸缩
- **函数框架**: OpenFunction Functions Framework 统一编程模型

---

## 架构设计

```
┌────────────────────────────────────────────────────┐
│                  OpenFunction                       │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │           OpenFunction Controller             │   │
│  │                                               │   │
│  │  Function CRD ──► Build ──► Serving           │   │
│  └──────────┬──────────┬──────────┬─────────────┘   │
│             │          │          │                   │
│  ┌──────────┴───┐ ┌───┴──────┐ ┌┴──────────────┐   │
│  │ Build Phase   │ │ Sync     │ │ Async         │   │
│  │              │ │ Serving  │ │ Serving       │   │
│  │ Shipwright   │ │          │ │               │   │
│  │ Buildpacks   │ │ Knative  │ │ KEDA + Dapr   │   │
│  │ Dockerfile   │ │ Serving  │ │               │   │
│  └──────────────┘ └──────────┘ └───────────────┘   │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │           OpenFunction Gateway                │   │
│  │      (Kubernetes Gateway API)                 │   │
│  └──────────────────────────────────────────────┘   │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │           Event Sources                       │   │
│  │  Kafka │ Cron │ HTTP │ MQTT │ Redis │ ...    │   │
│  └──────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────┘
```

### 函数类型对比

| 特性 | 同步函数 | 异步函数 |
|:---|:---|:---|
| **运行时** | Knative Serving | KEDA + Dapr |
| **触发方式** | HTTP 请求 | 事件源 (Kafka, Cron 等) |
| **伸缩方式** | 基于并发请求数 | 基于事件积压量 |
| **缩放到零** | 支持 | 支持 |
| **响应模式** | 请求-响应 | 异步处理 |
| **适用场景** | API、Webhook | 数据处理、ETL |

---

## 快速开始

### 安装 OpenFunction

```bash
# 使用 Helm 安装
helm repo add openfunction https://openfunction.github.io/charts/
helm repo update

helm install openfunction openfunction/openfunction \
  --namespace openfunction \
  --create-namespace \
  --set global.Knative.enabled=true \
  --set global.Keda.enabled=true \
  --set global.Dapr.enabled=true \
  --set global.Shipwright.enabled=true \
  --set global.TektonPipelines.enabled=true
```

### 编写同步函数 (Go)

```go
package hello

import (
    "fmt"
    "net/http"

    ofctx "github.com/OpenFunction/functions-framework-go/context"
)

func Hello(ctx ofctx.Context, w http.ResponseWriter, r *http.Request) error {
    name := r.URL.Query().Get("name")
    if name == "" {
        name = "World"
    }
    fmt.Fprintf(w, "Hello, %s!", name)
    return nil
}
```

### 部署同步函数

```yaml
apiVersion: core.openfunction.io/v1beta2
kind: Function
metadata:
  name: hello-function
spec:
  version: "v1.0.0"
  image: "registry.example.com/hello-function:latest"
  imageCredentials:
    name: registry-secret
  build:
    builder: openfunction/builder-go:latest
    env:
      FUNC_NAME: "Hello"
      FUNC_CLEAR_SOURCE: "true"
    srcRepo:
      url: "https://github.com/my-org/hello-function.git"
      sourceSubPath: "functions/hello"
      revision: "main"
  serving:
    runtime: knative
    template:
      containers:
        - name: function
          imagePullPolicy: Always
          resources:
            limits:
              cpu: "500m"
              memory: "256Mi"
    scaleOptions:
      minReplicas: 0
      maxReplicas: 10
      knative:
        autoscaling.knative.dev/target: "100"
```

---

## 配置详解

### 异步函数 - Kafka 消费者

```go
package consumer

import (
    ofctx "github.com/OpenFunction/functions-framework-go/context"
)

func HandleMessage(ctx ofctx.Context, in []byte) (ofctx.Out, error) {
    // in 是 Kafka 消息内容
    ctx.GetLogger().Info("Received message", "payload", string(in))
    
    // 处理消息并输出到另一个 topic
    out, err := ctx.Send("output", in)
    if err != nil {
        return ctx.ReturnOnInternalError(), err
    }
    return out, nil
}
```

```yaml
apiVersion: core.openfunction.io/v1beta2
kind: Function
metadata:
  name: kafka-consumer
spec:
  version: "v1.0.0"
  image: "registry.example.com/kafka-consumer:latest"
  build:
    builder: openfunction/builder-go:latest
    env:
      FUNC_NAME: "HandleMessage"
    srcRepo:
      url: "https://github.com/my-org/functions.git"
      sourceSubPath: "kafka-consumer"
  serving:
    runtime: async
    scaleOptions:
      minReplicas: 0
      maxReplicas: 20
      keda:
        scaledObject:
          pollingInterval: 15
          cooldownPeriod: 60
          advanced:
            horizontalPodAutoscalerConfig:
              behavior:
                scaleDown:
                  stabilizationWindowSeconds: 45
        triggers:
          - type: kafka
            metadata:
              bootstrapServers: "kafka.default.svc:9092"
              consumerGroup: "my-consumer-group"
              topic: "input-topic"
              lagThreshold: "10"
    bindings:
      kafka-input:
        type: bindings.kafka
        version: v1
        metadata:
          - name: brokers
            value: "kafka.default.svc:9092"
          - name: topics
            value: "input-topic"
          - name: consumerGroup
            value: "my-consumer-group"
    outputs:
      - dapr:
          name: kafka-output
          type: bindings.kafka
          version: v1
          metadata:
            - name: brokers
              value: "kafka.default.svc:9092"
            - name: topics
              value: "output-topic"
            - name: publishTopic
              value: "output-topic"
```

### 事件触发 - Cron 定时任务

```yaml
apiVersion: core.openfunction.io/v1beta2
kind: Function
metadata:
  name: cron-job
spec:
  version: "v1.0.0"
  image: "registry.example.com/cron-job:latest"
  serving:
    runtime: async
    scaleOptions:
      minReplicas: 0
      maxReplicas: 1
      keda:
        triggers:
          - type: cron
            metadata:
              timezone: "Asia/Shanghai"
              start: "0 8 * * *"
              end: "0 9 * * *"
              desiredReplicas: "1"
    bindings:
      cron:
        type: bindings.cron
        version: v1
        metadata:
          - name: schedule
            value: "0 */1 * * *"
```

### 函数 Pub/Sub 模式

```yaml
apiVersion: core.openfunction.io/v1beta2
kind: Function
metadata:
  name: subscriber
spec:
  serving:
    runtime: async
    pubsub:
      redis-pubsub:
        type: pubsub.redis
        version: v1
        metadata:
          - name: redisHost
            value: "redis.default.svc:6379"
    inputs:
      - dapr:
          name: redis-pubsub
          topic: "notifications"
    outputs:
      - dapr:
          name: redis-pubsub
          topic: "processed-notifications"
```

---

## Gateway 配置

```yaml
# 使用 Kubernetes Gateway API 暴露函数
apiVersion: networking.openfunction.io/v1alpha1
kind: Gateway
metadata:
  name: openfunction-gateway
  namespace: openfunction
spec:
  domain: "functions.example.com"
  clusterDomain: "cluster.local"
  hostTemplate: "{{.Name}}.{{.Namespace}}.{{.Domain}}"
  pathTemplate: "{{.Namespace}}/{{.Name}}"
  httpRouteLabelKey: "app.kubernetes.io/managed-by"
  gatewayRef:
    name: external-gateway
    namespace: openfunction
  gatewaySpec:
    listeners:
      - name: ofn-http
        protocol: HTTP
        port: 80
        allowedRoutes:
          namespaces:
            from: All
```

---

## 监控

```bash
# 查看函数状态
kubectl get functions -A

# 查看构建状态
kubectl get builders -A

# 查看 Serving 状态
kubectl get servings -A

# 查看 KEDA ScaledObject
kubectl get scaledobject -A
```

---

## 最佳实践

1. **运行时选择**: HTTP API 使用同步函数 (Knative)，消息处理使用异步函数 (KEDA+Dapr)
2. **构建缓存**: 配置 BuildPacks 缓存加速重复构建
3. **资源限制**: 为函数设置合理的 CPU/内存限制，避免资源争抢
4. **冷启动优化**: 对延迟敏感的函数设置 `minReplicas: 1` 避免冷启动
5. **事件去重**: 异步函数实现幂等处理，应对消息重复投递
6. **监控告警**: 监控函数错误率和延迟，配置 KEDA 的 fallback 策略

---

## 参考资源

- [OpenFunction 官方文档](https://openfunction.dev/docs/)
- [OpenFunction GitHub](https://github.com/OpenFunction/OpenFunction)
- [Functions Framework Go](https://github.com/OpenFunction/functions-framework-go)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
