---
title: K8s Serverless / FaaS 实践指南
description: '# K8s Serverless / FaaS 实践指南'
category: extensions
tags:
- k8s
- extensions
- crd
- operator
- webhook
- helm
- kafka
- hpa
- ingress
- wasm
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 开发工程师
- 架构师
estimated_read_time: 5min
intent_queries:
- K8s Serverless / FaaS 实践指南 是什么
- 如何 K8s Serverless / FaaS 实践指南
- Kubernetes 10 extensions 最佳实践
trigger_keywords:
- K8s
- Serverless
- FaaS
- 实践指南
- extensions
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- kafka-basics
- tls-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-07-platform-engineering/
  label: '相关知识域: domain-07-platform-engineering'
created: "2026-05-23"
---

# K8s Serverless / FaaS 实践指南

> **适用版本**: [[Knative|Knative]] Serving v1.17 / [[OpenFunction|OpenFunction]] v1.2 / [[KEDA|KEDA]] HTTP Addon  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、Serverless on K8s 架构模式](#一serverless-on-k8s-架构模式)
- [二、Knative Serving 部署](#二knative-serving-部署)
- [三、Knative [[Service|Service]] 与自动缩放](#三knative-service-与自动缩放)
- [四、OpenFunction 云原生函数](#四openfunction-云原生函数)
- [五、KEDA + HTTP 无服务器工作负载](#五keda--http-无服务器工作负载)
- [六、事件驱动架构 (Knative Eventing)](#六事件驱动架构-knative-eventing)
- [七、冷启动优化](#七冷启动优化)
- [八、与 WebAssembly 结合](#八与-webassembly-结合)
- [九、选型对比](#九选型对比)

---

<!-- chunk: 一、Serverless on K8s 架构模式 -->
## 一、Serverless on K8s 架构模式

```
K8s Serverless 三种模式

模式 A: Knative Serving (容器级)
├── 基于 Deployment + Service
├── 缩容至 0 (Scale-to-Zero)
├── 自动 HTTPS ( cert-manager 集成)
├── 流量分割 (蓝绿/金丝雀)
└── 适用: 现有容器应用 Serverless 化

模式 B: OpenFunction / Fission (函数级)
├── 函数即代码 (非容器)
├── 多语言运行时 (Go/Node.js/Python/Java)
├── 事件触发 (Kafka/MQTT/HTTP)
└── 适用: 纯函数计算场景

模式 C: KEDA + HPA (混合模式)
├── 标准 Deployment + KEDA Scaler
├── HTTP 触发缩容至 0
├── 与现有 K8s 生态完全兼容
└── 适用: 渐进式 Serverless 采用
```

---

<!-- chunk: 二、Knative Serving 部署 -->
## 二、Knative Serving 部署

### 2.1 安装

```bash
# 安装 Knative Serving
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.17.0/serving-crds.yaml
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.17.0/serving-core.yaml

# 安装网络层 (Kourier 推荐)
kubectl apply -f https://github.com/knative/net-kourier/releases/download/knative-v1.17.0/kourier.yaml

# 配置 Knative 使用 Kourier
kubectl patch configmap/config-network \
  --namespace knative-serving \
  --type merge \
  --patch '{"data":{"ingress-class":"kourier.ingress.networking.knative.dev"}}'

# 安装 HPA 自动缩放
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.17.0/serving-hpa.yaml
```

### 2.2 配置 DNS

```bash
# 使用 Magic DNS (sslip.io)
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.17.0/serving-default-domain.yaml

# 或配置真实域名
kubectl patch configmap/config-domain \
  --namespace knative-serving \
  --type merge \
  --patch '{"data":{"knative.example.com":""}}'
```

---

<!-- chunk: 三、Knative Service 与自动缩放 -->
## 三、Knative Service 与自动缩放

### 3.1 基础 Service

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: hello-knative
  namespace: default
spec:
  template:
    metadata:
      annotations:
        # 自动缩放配置
        autoscaling.knative.dev/minScale: "0"
        autoscaling.knative.dev/maxScale: "10"
        autoscaling.knative.dev/targetBurstCapacity: "200"
        autoscaling.knative.dev/targetUtilizationPercentage: "70"
        # 缩容至 0 的优雅期
        autoscaling.knative.dev/scale-to-zero-pod-retention-period: "5m"
    spec:
      containerConcurrency: 100
      timeoutSeconds: 300
      containers:
      - image: gcr.io/knative-samples/helloworld-go
        ports:
        - containerPort: 8080
        env:
        - name: TARGET
          value: "Knative"
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 256Mi
```

### 3.2 流量管理

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: myapp
  namespace: default
spec:
  template:
    metadata:
      name: myapp-revision-2
    spec:
      containers:
      - image: myregistry/myapp:v2.0.0
  traffic:
  - tag: stable
    revisionName: myapp-revision-1
    percent: 90
  - tag: canary
    revisionName: myapp-revision-2
    percent: 10
  - tag: latest
    latestRevision: true
    percent: 0
```

### 3.3 自动缩放行为

```
请求到达
    |
    ├── 无 Pod 运行 (Scale-to-Zero)
    │   └── Activator 代理请求
    │   └── 启动 Pod (冷启动 2-5s)
    │
    ├── Pod 运行但满并发
    │   └── 自动扩容新 Pod
    │
    └── 请求减少
        └── 缩容至 0 (默认 60s 无请求)
```

---

<!-- chunk: 四、OpenFunction 云原生函数 -->
## 四、OpenFunction 云原生函数

### 4.1 安装

```bash
helm repo add openfunction https://openfunction.github.io/charts/
helm repo update

helm install openfunction openfunction/openfunction \
  --namespace openfunction \
  --create-namespace \
  --set global.ShipWright.enabled=true \
  --set global.KnativeServing.enabled=true \
  --set global.Keda.enabled=true
```

### 4.2 函数定义

```yaml
apiVersion: core.openfunction.io/v1beta2
kind: Function
metadata:
  name: hello-function
  namespace: default
spec:
  version: v1.0.0
  image: myregistry/hello-function:v1
  imageCredentials:
    name: push-secret
  port: 8080
  serving:
    runtime: knative          # knative | async
    template:
      containers:
        - name: function
          imagePullPolicy: IfNotPresent
  build:
    builder: openfunction/builder-go:latest
    srcRepo:
      url: https://github.com/org/hello-function.git
      sourceSubPath: functions/hello
    shipwright:
      strategy:
        name: buildpacks-v3
        kind: ClusterBuildStrategy
```

### 4.3 异步函数 (事件驱动)

```yaml
apiVersion: core.openfunction.io/v1beta2
kind: Function
metadata:
  name: async-processor
spec:
  version: v1.0.0
  image: myregistry/async-processor:v1
  serving:
    runtime: async
    inputs:
      - name: input
        component: kafka-server
    outputs:
      - name: output
        component: kafka-server
    bindings:
      kafka-server:
        type: bindings.kafka
        version: v1
        metadata:
        - name: brokers
          value: kafka:9092
        - name: topics
          value: orders
        - name: consumerGroup
          value: processor-group
  build:
    builder: openfunction/builder-go:latest
    srcRepo:
      url: https://github.com/org/async-functions.git
```

---

<!-- chunk: 五、KEDA + HTTP 无服务器工作负载 -->
## 五、KEDA + HTTP 无服务器工作负载

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: http-app
  namespace: default
spec:
  scaleTargetRef:
    name: http-app
  minReplicaCount: 0
  maxReplicaCount: 100
  triggers:
  - type: metrics-api
    metadata:
      targetValue: "100"
      url: "http://keda-add-ons-http-interceptor.keda.svc.cluster.local:8080/interceptors/default/http-app"
      valueLocation: "value"
```

---

<!-- chunk: 六、事件驱动架构 (Knative Eventing) -->
## 六、事件驱动架构 (Knative Eventing)

```bash
# 安装 Eventing
kubectl apply -f https://github.com/knative/eventing/releases/download/knative-v1.17.0/eventing-crds.yaml
kubectl apply -f https://github.com/knative/eventing/releases/download/knative-v1.17.0/eventing-core.yaml

# 安装 Kafka 源
kubectl apply -f https://github.com/knative-extensions/eventing-kafka-broker/releases/download/knative-v1.17.0/eventing-kafka-controller.yaml
kubectl apply -f https://github.com/knative-extensions/eventing-kafka-broker/releases/download/knative-v1.17.0/eventing-kafka-broker.yaml
```

```yaml
# Broker (事件总线)
apiVersion: eventing.knative.dev/v1
kind: Broker
metadata:
  name: default
  namespace: default
---
# 触发器
apiVersion: eventing.knative.dev/v1
kind: Trigger
metadata:
  name: order-trigger
  namespace: default
spec:
  broker: default
  filter:
    attributes:
      type: order.created
  subscriber:
    ref:
      apiVersion: serving.knative.dev/v1
      kind: Service
      name: order-processor
```

---

<!-- chunk: 七、冷启动优化 -->
## 七、冷启动优化

| 优化手段 | 效果 | 实现 |
|:---|:---|:---|
| 最小副本数 | 避免 Scale-to-Zero | minScale: "1" |
|  readiness 探针 | 快速就绪 | 轻量健康检查 |
| 镜像预热 | 加速拉取 | 节点缓存 / 镜像仓库就近 |
| 精简镜像 | 减少拉取时间 | distroless / scratch |
| CPU 请求 | 加速启动 | 足够 CPU 用于初始化 |
| 初始化容器 | 预处理 | 分离启动逻辑 |

```yaml
spec:
  template:
    metadata:
      annotations:
        autoscaling.knative.dev/minScale: "1"  # 保留 1 个副本
    spec:
      containers:
      - image: gcr.io/distroless/go
        resources:
          requests:
            cpu: 500m  # 足够 CPU 加速启动
```

---

<!-- chunk: 八、与 WebAssembly 结合 -->
## 八、与 WebAssembly 结合

```
Knative + Spin (Wasm)
├── 更小的镜像 (~1MB vs ~50MB)
├── 更快的启动 (<1ms vs 1-2s)
├── 更高的密度 (单节点更多实例)
└── 相同的 Knative 管理体验
```

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: wasm-service
spec:
  template:
    spec:
      runtimeClassName: spin
      containers:
      - image: myregistry/spin-app:latest
```

---

<!-- chunk: 九、选型对比 -->
## 九、选型对比

| 维度 | Knative Serving | OpenFunction | KEDA + HPA | Fission |
|:---|:---|:---|:---|:---|
| **抽象层** | 容器 | 函数 | 容器 | 函数 |
| **缩容至 0** | ✅ | ✅ | ✅ | ❌ |
| **自动 HTTPS** | ✅ | ✅ | ❌ | ❌ |
| **流量分割** | ✅ | ✅ | ❌ | ❌ |
| **事件驱动** | Eventing | 内置 | Scalers | 内置 |
| **冷启动** | 1-5s | 1-5s | 1-5s | 100ms |
| **语言运行时** | 任意容器 | Go/Node/Python/Java | 任意容器 | 多语言 |
| **CNCF 状态** | Graduated | Sandbox | Graduated | - |
| **学习曲线** | 中 | 中 | 低 | 低 |
| **适用场景** | 容器 Serverless 化 | 云原生函数 | 渐进式采用 | 快速函数部署 |

### 选型决策

```
选择 Knative 如果:
  ✅ 现有容器应用需要 Serverless 能力
  ✅ 需要自动 HTTPS 和域名管理
  ✅ 需要流量分割和蓝绿部署
  ✅ CNCF 毕业项目，企业级保障

选择 OpenFunction 如果:
  ✅ 函数即代码 (Function-as-Code)
  ✅ 需要多语言运行时支持
  ✅ 需要内置事件驱动 (Dapr 集成)
  ✅ 需要函数版本管理

选择 KEDA + HPA 如果:
  ✅ 渐进式采用 Serverless
  ✅ 不想引入新的 CRD 体系
  ✅ 已有 K8s 工作负载需要事件驱动

选择 Fission 如果:
  ✅ 极快的函数冷启动
  ✅ 简单的函数部署体验
  ✅ 不需要缩容至 0
```

---

<!-- chunk: 参考链接 -->
## 参考链接

- [Knative 官方文档](https://knative.dev/docs/)
- [OpenFunction 文档](https://openfunction.dev/docs/)
- [KEDA HTTP Add-on](https://github.com/kedacore/http-add-on)
- [Fission 框架](https://fission.io/)
- [CNCF Serverless Landscape](https://landscape.cncf.io/serverless)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-15-specialized-tech MOC
- [[domain-15-specialized-tech/README|Domain-10: Kubernetes 扩展生态]]
- Domain-10 扩展与自定义 — 开源项目索引
- CRD 自定义资源定义开发指南
- 02 - Operator开发模式与控制器实现
- 03 - 准入控制器(Webhook)配置与实现
- Kubernetes API 聚合扩展机制详解
- 包管理与应用分发工具
- 47 - Helm Chart开发与管理
- 129 - Helm 高级运维：复杂部署、CI/CD 集成与安全最佳实践
- CI/CD 管道
- 48 - GitOps工作流

## See Also

- 99-graalvm-native-image-guide
- 99-quarkus-micronaut-cloud-native-java-guide
- 01-crd-development-guide
- 02-operator-development-patterns
