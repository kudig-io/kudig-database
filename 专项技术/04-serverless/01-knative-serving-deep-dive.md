---
title: Knative Serving 深度解析
description: 'Knative Serving 核心机制：自动缩容到零、流量分割、自定义域名与 Concurrency 配置'
summary: 'Knative Serving 核心机制：自动缩容到零、流量分割、自定义域名与 Concurrency 配置'
category: specialized-tech
tags:
- knative
- serverless
- scale-to-zero
- traffic-splitting
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Knative Serving 是什么
- 如何配置 Knative Serving 自动缩容到零
- 如何实现 Knative 流量分割
trigger_keywords:
- knative
- serving
- scale-to-zero
- canary
- blue-green
- traffic splitting
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Knative Serving 深度解析

## 1. 核心架构

Knative Serving 基于 Kubernetes 构建无服务器工作负载，核心组件包括：

| 组件 | 职责 | 默认实现 |
|------|------|----------|
| **Activator** | 接收请求并触发缩容到零的 Pod 拉起 | 共享 Deployment |
| **Autoscaler (KPA)** | 基于并发/RPS 指标决定扩缩容 | Knative Pod Autoscaler |
| **Queue Proxy** | Sidecar，收集指标并限流 | 注入到每个 Pod |
| **DomainMapping** | 自定义域名到 Knative Service 的映射 | Ingress 管理 |

请求流向：

```
Client → Ingress(Kourier) → Activator(冷启动) → Queue Proxy → User Container
                            → Route(热启动)    → Queue Proxy → User Container
```

## 2. Knative Service 定义

### 2.1 基础 Service

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: hello
  namespace: default
spec:
  template:
    metadata:
      annotations:
        # 缩容到零的窗口时间（秒）
        autoscaling.knative.dev/scale-down-delay: "30s"
    spec:
      containers:
        - image: gcr.io/knative-samples/helloworld-go
          ports:
            - containerPort: 8080
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
```

### 2.2 多容器 Pod（Sidecar 模式）

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: app-with-sidecar
spec:
  template:
    spec:
      containers:
        - image: my-app:latest
          name: user-container
          env:
            - name: SIDECAR_URL
              value: "http://localhost:9090"
        - image: my-sidecar:latest
          name: sidecar
          ports:
            - containerPort: 9090
```

## 3. 自动缩容到零（Scale-to-Zero）

### 3.1 KPA 配置参数

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: scale-demo
  annotations:
    # 使用 Knative Pod Autoscaler
    autoscaling.knative.dev/class: "kpa.autoscaling.knative.dev"
    # 缩放模式：stable（平滑）/ rapid（快速）
    autoscaling.knative.dev/metric: "concurrency"
spec:
  template:
    metadata:
      annotations:
        # 目标并发数（soft limit，触发扩缩容）
        autoscaling.knative.dev/target: "100"
        # 硬限制（Queue Proxy 限流上限）
        autoscaling.knative.dev/target-utilization-percentage: "70"
        # 缩容到零的冷却时间
        autoscaling.knative.dev/scale-to-zero-pod-retention-period: "1m"
        # 窗口期（稳定模式下指标平均窗口）
        autoscaling.knative.dev/window: "60s"
        # 最小副本数（0=允许缩容到零）
        autoscaling.knative.dev/min-scale: "0"
        # 最大副本数
        autoscaling.knative.dev/max-scale: "50"
    spec:
      containers:
        - image: my-app:latest
```

### 3.2 Scale-to-Zero 流程

```
正常运行(Pod=3) → 流量降为0 → 等待scale-down-delay → 缩容到0 → Activator接管
     ↑                                                          ↓
     ←←←←←←← 新请求到达 → Activator 拉起 Pod（冷启动） ←←←←←←←←
```

冷启动延迟分析：

| 阶段 | 耗时 | 优化方法 |
|------|------|----------|
| 请求排队 | ~10ms | Activator 预热 |
| Pod 调度 | 50-200ms | 节点预置、PriorityClass |
| 镜像拉取 | 1-30s | 预拉取、小镜像 |
| 容器启动 | 应用相关 | 优化启动流程 |
| Readiness 检查 | 1-5s | 快速 health check |

### 3.3 零冷启动优化

```yaml
# 方法 1: 保持至少 1 个副本（禁用 scale-to-zero）
autoscaling.knative.dev/min-scale: "1"

# 方法 2: 使用 Scale-to-Zero Pod Retention 延长保留
autoscaling.knative.dev/scale-to-zero-pod-retention-period: "5m"

# 方法 3: 使用 activator 数量优化并发瓶颈
# ConfigMap: config-autoscaler
apiVersion: v1
kind: ConfigMap
metadata:
  name: config-autoscaler
  namespace: knative-serving
data:
  activator-capacity: "200"       # 每个 activator 处理的最大并发
  max-scale-up-rate: "1000"       # 最大扩容倍率
  container-concurrency-target-default: "100"
```

## 4. 流量分割（Traffic Splitting）

### 4.1 金丝雀发布

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: my-app
spec:
  template:
    metadata:
      name: my-app-v2    # Revision 名称
    spec:
      containers:
        - image: my-app:v2
  traffic:
    - revisionName: my-app-v1    # 稳定版本
      percent: 90
      latestRevision: false
    - revisionName: my-app-v2    # 金丝雀版本
      percent: 10
      latestRevision: false
```

### 4.2 蓝绿发布

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: my-app
spec:
  template:
    metadata:
      name: my-app-green
    spec:
      containers:
        - image: my-app:green
  traffic:
    - tag: blue
      revisionName: my-app-blue
      percent: 100
    - tag: green
      revisionName: my-app-green
      percent: 0    # 预部署，通过 URL 预览
```

蓝绿切换通过修改 percent 完成：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 预览 green 版本
curl https://green-my-app.example.com

# 切换流量到 green
kubectl ksvc update my-app --traffic blue=0,green=100
```
### 4.3 基于 Header 的路由

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: my-app
spec:
  traffic:
    - tag: stable
      revisionName: my-app-v1
      percent: 100
    - tag: preview
      revisionName: my-app-v2
      percent: 0
      # 通过 Knative Route 规则实现 Header 路由
```

```bash
# 通过指定 Header 访问 preview 版本
curl -H "Knative-Serving-Tag: preview" https://my-app.example.com
```

## 5. 自定义域名与 HTTPS

### 5.1 DomainMapping

```yaml
apiVersion: serving.knative.dev/v1beta1
kind: DomainMapping
metadata:
  name: app.example.com
  namespace: production
spec:
  ref:
    name: my-app
    kind: Service
    apiVersion: serving.knative.dev/v1
```

### 5.2 自动 HTTPS（cert-manager 集成）

```yaml
# 安装 cert-manager 后配置 Knative 使用
apiVersion: v1
kind: ConfigMap
metadata:
  name: config-network
  namespace: knative-serving
data:
  auto-tls: "Enabled"
  http-protocol: "Redirected"    # HTTP 自动跳转 HTTPS
```

```yaml
# ClusterIssuer 配置
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    privateKeySecretRef:
      name: letsencrypt-prod-key
    solvers:
      - http01:
          ingress:
            class: kourier
```

### 5.3 全局域名配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: config-domain
  namespace: knative-serving
data:
  # 默认域名后缀
  example.com: ""
  # 特定标签使用不同域名
  staging.example.com: |
    selector:
      app: staging
```

## 6. Concurrency 配置

### 6.1 并发模型

```yaml
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: concurrent-app
spec:
  template:
    metadata:
      annotations:
        # 硬限制：Queue Proxy 会限流
        autoscaling.knative.dev/min-scale: "1"
    spec:
      # 容器级并发限制
      containerConcurrency: 100    # 硬限制，0=无限制
      containers:
        - image: my-app:latest
          env:
            - name: GOMAXPROCS
              value: "2"
```

### 6.2 并发参数调优矩阵

| 场景 | target | containerConcurrency | window | 模式 |
|------|--------|---------------------|--------|------|
| 延迟敏感 API | 50 | 100 | 6s | rapid |
| 批量处理 | 200 | 0 | 120s | stable |
| 突发流量 | 80 | 200 | 30s | rapid |
| 成本敏感 | 150 | 0 | 300s | stable |

## 7. Kourier / Ingress 集成

### 7.1 Kourier 安装与配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Kourier（轻量级 Knative Ingress）
kubectl apply -f https://github.com/knative/net-kourier/releases/latest/download/kourier.yaml

# 配置 Knative Serving 使用 Kourier
kubectl patch configmap/config-network \
  --namespace knative-serving \
  --type merge \
  --patch '{"data":{"ingress-class":"kourier.ingress.networking.knative.dev"}}'
```
### 7.2 Kourier 高级配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kourier-config
  namespace: kourier-system
data:
  # 启用 PROXY 协议
  enable-proxy-protocol: "false"
  # 空闲连接超时
  idle-timeout: "300s"
  # 最大连接数
  max-connections: "1024"
```

### 7.3 与 Nginx Ingress 并存

```yaml
# 配置特定 Service 使用 Nginx Ingress
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: nginx-routed-app
  annotations:
    networking.knative.dev/ingress.class: kourier.ingress.networking.knative.dev
spec:
  template:
    spec:
      containers:
        - image: my-app:latest
```

## 8. 监控与排障

### 8.1 关键指标

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Knative Service 状态
kubectl ksvc get my-app

# 查看 Revision 列表
kubectl get revisions -l serving.knative.dev/service=my-app

# 查看 Pod 缩容状态
kubectl get pods -l serving.knative.dev/service=my-app

# 查看 Autoscaler 指标
kubectl -n knative-serving port-forward svc/autoscaler 9090:9090
```
### 8.2 常见问题排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 问题 1: 冷启动超时
# 检查 Activator 日志
kubectl -n knative-serving logs -l app=activator -f

# 问题 2: 流量分割不生效
# 检查 Route 状态
kubectl get route my-app -o yaml

# 问题 3: 缩容到零后请求失败
# 检查 Activator 到 Activator 的连接
kubectl -n knative-serving get endpoints activator-service
```
## 9. 生产最佳实践

```yaml
# 生产级 Knative Service 模板
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: production-app
  annotations:
    autoscaling.knative.dev/class: "kpa.autoscaling.knative.dev"
    autoscaling.knative.dev/metric: "rps"
spec:
  template:
    metadata:
      annotations:
        autoscaling.knative.dev/target: "1000"
        autoscaling.knative.dev/min-scale: "2"
        autoscaling.knative.dev/max-scale: "100"
        autoscaling.knative.dev/scale-to-zero-pod-retention-period: "5m"
        autoscaling.knative.dev/window: "60s"
    spec:
      containerConcurrency: 200
      containers:
        - image: my-app:latest
          readinessProbe:
            httpGet:
              path: /healthz
            periodSeconds: 3
          resources:
            requests:
              cpu: 250m
              memory: 256Mi
            limits:
              cpu: "1"
              memory: 512Mi
  traffic:
    - latestRevision: true
      percent: 100
```

---

## Related

- [[专项技术/04-serverless/02-knative-eventing-patterns|Knative Eventing 事件驱动模式]]
- [[专项技术/04-serverless/03-openfaas-serverless-functions|OpenFaaS 无服务器函数]]

## See Also

- [Knative Serving 官方文档](https://knative.dev/docs/serving/)
- [Knative Autoscaling](https://knative.dev/docs/serving/autoscaling/)


<!-- risk-assessed -->
