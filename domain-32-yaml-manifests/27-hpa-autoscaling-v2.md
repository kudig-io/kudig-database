# 27 - HorizontalPodAutoscaler v2 YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02  
> **主题**: HPA v2 水平自动扩缩容、多指标支持、扩缩容行为控制

## 目录

- [概述](#概述)
- [完整字段说明](#完整字段说明)
- [指标类型详解](#指标类型详解)
- [扩缩容行为控制](#扩缩容行为控制)
- [VPA 垂直扩缩容参考](#vpa-垂直扩缩容参考)
- [内部原理](#内部原理)
- [生产案例](#生产案例)
- [最佳实践](#最佳实践)
- [常见问题](#常见问题)

---

## 概述

### HorizontalPodAutoscaler v2

HPA v2 是 Kubernetes 核心自动扩缩容机制，支持多种指标类型和精细化行为控制。

**核心能力**:
- **多指标支持**: CPU、Memory、自定义指标、外部指标
- **行为控制**: 扩缩容速率、稳定窗口、策略选择
- **容器级资源指标**: v1.27+ 支持 `ContainerResource`
- **动态算法**: 基于当前副本数和目标值自动计算期望副本数

**版本兼容性**:
- v1.23+: HPA v2 正式 GA（替代 v2beta2）
- v1.25+: 推荐使用 `autoscaling/v2`
- v1.27+: 新增 `ContainerResource` 指标类型
- v1.32: 稳定版本，所有特性成熟

---

## 完整字段说明

### 基础结构

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: myapp-hpa
  namespace: default
spec:
  # ========== 必需字段 ==========
  
  # 目标资源引用 (Deployment, ReplicaSet, StatefulSet)
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: myapp
  
  # 副本数范围
  minReplicas: 2    # 最小副本数 (默认 1)
  maxReplicas: 10   # 最大副本数 (必需)
  
  # 指标配置 (至少一个)
  metrics:
  - type: Resource  # 资源指标 (CPU/Memory)
    resource:
      name: cpu
      target:
        type: Utilization       # 使用率类型
        averageUtilization: 70  # 目标 70%
  
  # ========== 可选字段 ==========
  
  # 扩缩容行为控制 (v1.23+ GA)
  behavior:
    scaleUp:       # 扩容行为
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
    scaleDown:     # 缩容行为
      stabilizationWindowSeconds: 300
      policies:
      - type: Pods
        value: 1
        periodSeconds: 60

# ========== 状态字段 (status) ==========
status:
  # 当前副本数
  currentReplicas: 3
  # 期望副本数
  desiredReplicas: 5
  # 当前指标值
  currentMetrics:
  - type: Resource
    resource:
      name: cpu
      current:
        averageUtilization: 85
        averageValue: "850m"
  # 最后扩缩容时间
  lastScaleTime: "2026-02-10T10:30:00Z"
  # 条件状态
  conditions:
  - type: AbleToScale
    status: "True"
    reason: ReadyForNewScale
  - type: ScalingActive
    status: "True"
    reason: ValidMetricFound
  - type: ScalingLimited
    status: "False"
    reason: DesiredWithinRange
```

---

## 指标类型详解

### 1. Resource 指标 (CPU/Memory)

#### CPU 使用率扩缩容

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: cpu-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  # CPU 使用率 (基于 Pod 的 resources.requests.cpu)
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization         # 百分比类型
        averageUtilization: 70    # 所有 Pod 平均 CPU 使用率 70%
```

**计算公式**:
```
averageUtilization = (当前 CPU 使用量 / CPU Request) * 100
目标值: 所有 Pod 的平均 CPU 使用率不超过 70%
```

#### Memory 绝对值扩缩容

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: memory-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: cache-service
  minReplicas: 3
  maxReplicas: 20
  metrics:
  # Memory 绝对值
  - type: Resource
    resource:
      name: memory
      target:
        type: AverageValue        # 绝对值类型
        averageValue: "500Mi"     # 每个 Pod 平均内存 500Mi
```

**适用场景**:
- `Utilization`: 适合 CPU（通常设置 60-80%）
- `AverageValue`: 适合 Memory（避免触碰 Limit 导致 OOMKilled）

### 2. ContainerResource 指标 (v1.27+)

针对特定容器的资源指标（适用于 Sidecar 场景）。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: container-cpu-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: istio-app
  minReplicas: 2
  maxReplicas: 15
  metrics:
  # 仅监控主容器 CPU (忽略 Istio Sidecar)
  - type: ContainerResource
    containerResource:
      name: cpu                  # 资源名称
      container: application     # 容器名称 (spec.containers[].name)
      target:
        type: Utilization
        averageUtilization: 75
```

**使用场景**:
- **Service Mesh**: 避免 Sidecar CPU 影响扩缩容决策
- **多容器 Pod**: 仅基于主容器指标扩缩容
- **Daemonset + Sidecar**: 精确监控业务容器

### 3. Pods 自定义指标

基于 Pod 自身暴露的业务指标（需配合 Metrics Adapter）。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: custom-qps-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-gateway
  minReplicas: 5
  maxReplicas: 50
  metrics:
  # 每个 Pod 的 QPS 指标
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second  # 自定义指标名称
        selector:                        # 可选: 指标标签选择器
          matchLabels:
            verb: GET
      target:
        type: AverageValue
        averageValue: "1000"  # 每个 Pod 处理 1000 QPS
```

**指标来源**:
- **Prometheus Adapter**: 从 Prometheus 查询指标
- **Datadog/Custom Metrics API**: 第三方监控系统

**配置示例** (Prometheus Adapter):
```yaml
# ConfigMap 配置
rules:
- seriesQuery: 'http_requests_total{namespace!="",pod!=""}'
  resources:
    overrides:
      namespace: {resource: "namespace"}
      pod: {resource: "pod"}
  name:
    matches: "^(.*)_total$"
    as: "${1}_per_second"
  metricsQuery: 'rate(<<.Series>>{<<.LabelMatchers>>}[1m])'
```

### 4. Object 指标

基于集群中其他对象的指标（如 Ingress、Service）。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: ingress-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: frontend
  minReplicas: 3
  maxReplicas: 30
  metrics:
  # 基于 Ingress 对象的请求数
  - type: Object
    object:
      metric:
        name: requests-per-second
        selector:
          matchLabels:
            verb: GET
      describedObject:
        apiVersion: networking.k8s.io/v1
        kind: Ingress
        name: main-ingress
      target:
        type: Value
        value: "10000"  # Ingress 总 QPS 10000
```

**计算逻辑**:
```
desiredReplicas = currentReplicas * (currentMetricValue / targetValue)
例如: currentReplicas=5, currentMetricValue=15000, targetValue=10000
desiredReplicas = 5 * (15000 / 10000) = 7.5 → 向上取整 8
```

### 5. External 指标

基于集群外部系统的指标（如云监控、消息队列）。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: sqs-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: worker
  minReplicas: 2
  maxReplicas: 100
  metrics:
  # AWS SQS 队列消息数
  - type: External
    external:
      metric:
        name: sqs_queue_messages_visible
        selector:
          matchLabels:
            queue_name: "task-queue"
      target:
        type: AverageValue
        averageValue: "30"  # 每个 Pod 处理 30 条消息
```

**常见外部指标**:
- **AWS CloudWatch**: SQS 队列长度、ALB 请求数
- **Azure Monitor**: Service Bus 消息数
- **RabbitMQ**: 队列深度
- **Redis**: Key 数量、内存使用

### 6. 多指标组合

多个指标同时生效，取**最大副本数**。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: multi-metric-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-server
  minReplicas: 5
  maxReplicas: 50
  metrics:
  # 指标1: CPU 使用率
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  
  # 指标2: Memory 绝对值
  - type: Resource
    resource:
      name: memory
      target:
        type: AverageValue
        averageValue: "1Gi"
  
  # 指标3: 自定义 QPS
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "500"
  
  # 指标4: 外部 Kafka Lag
  - type: External
    external:
      metric:
        name: kafka_consumer_lag
        selector:
          matchLabels:
            topic: "events"
      target:
        type: AverageValue
        averageValue: "1000"
```

**决策逻辑**:
```
每个指标计算期望副本数:
- CPU 指标 → 8 个副本
- Memory 指标 → 6 个副本
- QPS 指标 → 12 个副本
- Kafka Lag → 10 个副本

最终副本数 = max(8, 6, 12, 10) = 12
```

---

## 扩缩容行为控制

### behavior 字段结构

> **Feature Status**: v1.18 Alpha → v1.23 GA

```yaml
spec:
  behavior:
    # 扩容行为
    scaleUp:
      # 稳定窗口: 在此时间内不会再次扩容 (秒)
      stabilizationWindowSeconds: 0  # 默认 0 (立即扩容)
      
      # 策略选择模式
      selectPolicy: Max  # Max (最激进) | Min (最保守) | Disabled (禁用)
      
      # 扩容策略列表
      policies:
      - type: Percent        # 百分比策略
        value: 100           # 每次扩容 100% (翻倍)
        periodSeconds: 15    # 15 秒内允许一次
      - type: Pods           # 绝对值策略
        value: 4             # 每次最多增加 4 个 Pod
        periodSeconds: 15
    
    # 缩容行为
    scaleDown:
      # 稳定窗口: 5 分钟内指标稳定才缩容
      stabilizationWindowSeconds: 300  # 默认 300 秒
      
      selectPolicy: Min  # 选择最保守的策略
      
      policies:
      - type: Percent
        value: 50          # 每次最多缩容 50%
        periodSeconds: 60  # 1 分钟内允许一次
      - type: Pods
        value: 2           # 每次最多减少 2 个 Pod
        periodSeconds: 60
```

### 扩容行为配置

#### 快速扩容策略（处理流量突增）

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: fast-scale-up
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-server
  minReplicas: 2
  maxReplicas: 100
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 60
  
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 0  # 立即扩容,不等待
      selectPolicy: Max              # 选择最激进的策略
      policies:
      # 策略1: 初期快速翻倍
      - type: Percent
        value: 100           # 翻倍
        periodSeconds: 15    # 15 秒一次
      # 策略2: 每次至少加 10 个 Pod
      - type: Pods
        value: 10
        periodSeconds: 15
      # 策略3: 第一分钟后限制增速
      - type: Percent
        value: 50
        periodSeconds: 60
```

**效果**:
```
初始 2 个 Pod, CPU 使用率 90%:
0s:   2 → 4 (翻倍, +100%)
15s:  4 → 8 (翻倍, +100%)
30s:  8 → 16 (翻倍, +100%)
45s:  16 → 26 (max(+10 Pods, +100%) = +10)
60s:  26 → 36 (限制为 +50% 或 +10, 取 Max = +10)
```

#### 渐进式扩容（避免过度扩容）

```yaml
behavior:
  scaleUp:
    stabilizationWindowSeconds: 60  # 1 分钟稳定窗口
    selectPolicy: Min               # 选择最保守的策略
    policies:
    - type: Percent
      value: 10           # 每次最多增加 10%
      periodSeconds: 60
    - type: Pods
      value: 2            # 或增加 2 个 Pod
      periodSeconds: 30
```

### 缩容行为配置

#### 保守缩容（避免抖动）

```yaml
behavior:
  scaleDown:
    stabilizationWindowSeconds: 600  # 10 分钟稳定窗口
    selectPolicy: Min                # 最保守策略
    policies:
    # 策略1: 每 5 分钟最多缩 10%
    - type: Percent
      value: 10
      periodSeconds: 300
    # 策略2: 每次最多减 1 个 Pod
    - type: Pods
      value: 1
      periodSeconds: 120
```

**效果**:
- 指标低于目标值后，等待 10 分钟确认趋势
- 缩容时每 2 分钟最多减少 1 个 Pod
- 避免因短暂流量下降导致的频繁缩容

#### 禁用缩容（仅扩容不缩容）

```yaml
behavior:
  scaleDown:
    selectPolicy: Disabled  # 完全禁用自动缩容
  scaleUp:
    stabilizationWindowSeconds: 0
    policies:
    - type: Percent
      value: 100
      periodSeconds: 15
```

**使用场景**:
- 流量预测场景（提前扩容，手动缩容）
- 防止缓存失效（Pod 缩容导致缓存丢失）
- 成本优化窗口（仅在特定时间允许缩容）

### 不同时间段差异化策略

```yaml
# 工作时间: 快速扩容 + 保守缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: business-hours-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-service
  minReplicas: 10   # 工作时间最少 10 个
  maxReplicas: 100
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
    scaleDown:
      stabilizationWindowSeconds: 600  # 10 分钟
      policies:
      - type: Pods
        value: 1
        periodSeconds: 300  # 5 分钟减 1 个
---
# 非工作时间: 通过修改 minReplicas 实现
# 实际生产中通常使用 CronJob 自动切换 HPA 配置
```

---

## VPA 垂直扩缩容参考

> **注意**: VPA 是社区项目，非 Kubernetes 核心组件

### VPA vs HPA 对比

| 维度           | HPA (水平扩缩容)          | VPA (垂直扩缩容)          |
|----------------|---------------------------|---------------------------|
| **调整对象**   | Pod 副本数                | Pod 资源 requests/limits  |
| **适用场景**   | 无状态应用、可水平扩展    | 有状态应用、数据库        |
| **扩容速度**   | 快 (秒级)                 | 慢 (需重启 Pod)           |
| **状态影响**   | 无影响 (新 Pod)           | 会重启 Pod (状态丢失)     |
| **成本优化**   | 中等                      | 高 (精确资源分配)         |
| **兼容性**     | 原生支持                  | 需安装 VPA 组件           |

### VPA 基础配置

**安装 VPA** (社区项目):
```bash
# 克隆仓库
git clone https://github.com/kubernetes/autoscaler.git
cd autoscaler/vertical-pod-autoscaler

# 部署 VPA
./hack/vpa-up.sh

# 验证
kubectl get crd | grep verticalpodautoscaler
```

**VPA 配置示例**:

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: mysql-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: StatefulSet
    name: mysql
  
  # 更新策略
  updatePolicy:
    updateMode: "Auto"  # Auto | Initial | Recreate | Off
  
  # 资源策略
  resourcePolicy:
    containerPolicies:
    - containerName: mysql
      # 资源范围限制
      minAllowed:
        cpu: "500m"
        memory: "1Gi"
      maxAllowed:
        cpu: "4"
        memory: "16Gi"
      # 控制模式
      controlledResources: ["cpu", "memory"]
      # 控制值类型
      controlledValues: RequestsAndLimits  # RequestsOnly | RequestsAndLimits
```

**updateMode 说明**:
- **Off**: 仅提供建议，不自动修改
- **Initial**: 仅在 Pod 创建时设置资源
- **Recreate**: 主动重启 Pod 更新资源
- **Auto**: 在 Pod 重启时自动更新（推荐）

### HPA + VPA 联合使用

> **警告**: HPA 和 VPA 同时作用于 CPU/Memory 会冲突！

**安全组合方式**:

```yaml
# HPA: 基于自定义指标扩缩容
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 2
  maxReplicas: 20
  metrics:
  # 仅使用自定义指标 (避免与 VPA 冲突)
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"
---
# VPA: 调整 CPU/Memory
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  updatePolicy:
    updateMode: Auto
  resourcePolicy:
    containerPolicies:
    - containerName: app
      # VPA 仅管理 CPU/Memory
      controlledResources: ["cpu", "memory"]
      minAllowed:
        cpu: "100m"
        memory: "128Mi"
      maxAllowed:
        cpu: "2"
        memory: "4Gi"
```

---

## 内部原理

### HPA 控制循环

```
┌─────────────────────────────────────────────────────────┐
│          HPA Controller (kube-controller-manager)       │
└────────────────────┬────────────────────────────────────┘
                     │ 每 15 秒一次 (--horizontal-pod-autoscaler-sync-period)
                     ▼
┌─────────────────────────────────────────────────────────┐
│ 1. 获取 HPA 对象和目标 Deployment                        │
│ 2. 查询指标 API (Metrics Server / Custom Metrics API)  │
│ 3. 计算期望副本数                                        │
│ 4. 检查 behavior 约束和稳定窗口                         │
│ 5. 更新 Deployment.spec.replicas                       │
└─────────────────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Deployment Controller                      │
│  创建/删除 Pod 以达到期望副本数                          │
└─────────────────────────────────────────────────────────┘
```

### 计算算法

#### 基础公式

```
desiredReplicas = ceil[currentReplicas * (currentMetricValue / targetMetricValue)]
```

**示例计算**:
```yaml
# 当前状态:
currentReplicas: 3
currentMetricValue: 900m  # 当前 CPU 使用量
targetMetricValue: 500m   # 目标 CPU 使用量

# 计算:
desiredReplicas = ceil[3 * (900 / 500)]
                = ceil[3 * 1.8]
                = ceil[5.4]
                = 6  # 需要 6 个副本
```

#### 容忍度机制

避免微小波动导致频繁扩缩容:

```go
// HPA 内置容忍度 (--horizontal-pod-autoscaler-tolerance, 默认 0.1)
tolerance := 0.1

if abs(currentMetricValue - targetMetricValue) / targetMetricValue < tolerance {
  // 不触发扩缩容 (在 ±10% 范围内)
  return currentReplicas
}
```

**例子**:
```
目标 CPU: 70%
容忍范围: 63% - 77%
- CPU 65%: 不触发缩容
- CPU 75%: 不触发扩容
- CPU 60%: 触发缩容
- CPU 80%: 触发扩容
```

#### 多指标聚合

```go
var maxReplicas int32

for _, metric := range hpa.Spec.Metrics {
  replicas := calculateReplicasForMetric(metric)
  if replicas > maxReplicas {
    maxReplicas = replicas
  }
}

return min(maxReplicas, hpa.Spec.MaxReplicas)
```

### 指标采集链路

#### 架构图

```
┌───────────────┐
│  Application  │ (暴露 /metrics)
└───────┬───────┘
        │
        ▼
┌───────────────┐
│  Prometheus   │ (采集并存储指标)
└───────┬───────┘
        │
        ▼
┌───────────────────────────┐
│  Prometheus Adapter       │ (实现 Custom Metrics API)
└───────┬───────────────────┘
        │
        ▼
┌───────────────────────────┐
│  Custom Metrics API       │ (apis/custom.metrics.k8s.io)
└───────┬───────────────────┘
        │
        ▼
┌───────────────────────────┐
│  HPA Controller           │ (读取指标并计算副本数)
└───────────────────────────┘
```

#### Metrics API 分层

```yaml
# 1. Resource Metrics API (核心指标)
# - 提供者: Metrics Server
# - 指标: CPU, Memory
GET /apis/metrics.k8s.io/v1beta1/pods

# 2. Custom Metrics API (自定义指标)
# - 提供者: Prometheus Adapter, Datadog, etc.
# - 指标: 业务指标 (QPS, Latency, etc.)
GET /apis/custom.metrics.k8s.io/v1beta1/namespaces/default/pods/*/http_requests_per_second

# 3. External Metrics API (外部指标)
# - 提供者: Cloud Provider Adapter
# - 指标: 云监控指标 (SQS, CloudWatch, etc.)
GET /apis/external.metrics.k8s.io/v1beta1/namespaces/default/sqs_queue_messages_visible
```

### 关键参数调优

```yaml
# kube-controller-manager 参数
--horizontal-pod-autoscaler-sync-period=15s           # HPA 评估周期
--horizontal-pod-autoscaler-tolerance=0.1             # 容忍度 10%
--horizontal-pod-autoscaler-downscale-stabilization=5m  # 缩容稳定窗口 (已废弃, 使用 behavior)
--horizontal-pod-autoscaler-cpu-initialization-period=5m  # Pod 启动后忽略 CPU 指标的时间
--horizontal-pod-autoscaler-initial-readiness-delay=30s   # Pod 就绪后开始采集指标的延迟
```

---

## 生产案例

### 案例1: 电商平台 - CPU 扩缩容

**场景**: 电商 API 服务，处理用户请求，CPU 密集型。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: ecommerce-api-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: ecommerce-api
  
  # 最小 10 个副本保证基础可用性
  minReplicas: 10
  # 最大 100 个副本应对大促
  maxReplicas: 100
  
  metrics:
  # CPU 使用率目标 60%
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 60
  
  behavior:
    scaleUp:
      # 快速扩容应对突发流量
      stabilizationWindowSeconds: 0
      selectPolicy: Max
      policies:
      - type: Percent
        value: 100    # 翻倍
        periodSeconds: 30
      - type: Pods
        value: 10     # 或每次加 10 个
        periodSeconds: 30
    
    scaleDown:
      # 保守缩容避免抖动
      stabilizationWindowSeconds: 300  # 5 分钟
      selectPolicy: Min
      policies:
      - type: Percent
        value: 10     # 每次最多缩 10%
        periodSeconds: 60
      - type: Pods
        value: 5      # 或每次减 5 个
        periodSeconds: 60
```

**Deployment 配置**:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ecommerce-api
spec:
  replicas: 10  # HPA 会覆盖此值
  template:
    spec:
      containers:
      - name: api
        image: ecommerce-api:v2.1
        resources:
          requests:
            cpu: "1"       # HPA 基于此值计算使用率
            memory: "2Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
```

**监控验证**:

```bash
# 实时查看 HPA 状态
kubectl get hpa ecommerce-api-hpa --watch

# 输出示例:
# NAME                 REFERENCE                   TARGETS   MINPODS   MAXPODS   REPLICAS   AGE
# ecommerce-api-hpa    Deployment/ecommerce-api    65%/60%   10        100       12         5m
#                                                  ↑ 当前超目标, 需扩容

# 详细信息
kubectl describe hpa ecommerce-api-hpa
```

### 案例2: 消息队列消费者 - 外部指标扩缩容

**场景**: Kafka 消费者服务，根据消费延迟动态扩缩容。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: kafka-consumer-hpa
  namespace: data-pipeline
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: event-processor
  
  minReplicas: 3
  maxReplicas: 50
  
  metrics:
  # 外部指标: Kafka Consumer Lag
  - type: External
    external:
      metric:
        name: kafka_consumer_lag_sum
        selector:
          matchLabels:
            topic: "user-events"
            consumer_group: "processor-group"
      target:
        type: AverageValue
        # 每个 Pod 处理 1000 条 Lag
        averageValue: "1000"
  
  behavior:
    scaleUp:
      # 积压消息时快速扩容
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 60
    scaleDown:
      # 消息消费完毕后缓慢缩容
      stabilizationWindowSeconds: 600
      policies:
      - type: Pods
        value: 1
        periodSeconds: 120
```

**Prometheus Adapter 配置**:

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: adapter-config
  namespace: monitoring
data:
  config.yaml: |
    rules:
    # 配置 Kafka Lag 指标
    - seriesQuery: 'kafka_consumer_lag{topic="user-events"}'
      resources:
        overrides:
          namespace: {resource: "namespace"}
      name:
        as: "kafka_consumer_lag_sum"
      metricsQuery: 'sum(kafka_consumer_lag{<<.LabelMatchers>>}) by (namespace)'
```

**效果演示**:

```bash
# 模拟消息积压
# Lag: 50000 条, 期望每个 Pod 处理 1000 条
# desiredReplicas = 50000 / 1000 = 50 (达到 maxReplicas)

# HPA 状态
kubectl get hpa kafka-consumer-hpa
# TARGETS        REPLICAS
# 5000/1000      50/50  (已扩到最大)

# 消息消费完毕后
# Lag: 500 条
# desiredReplicas = 500 / 1000 = 0.5 → 3 (minReplicas)
```

### 案例3: 在线视频服务 - 多指标混合扩缩容

**场景**: 视频转码服务，同时考虑 CPU、内存、队列长度。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: video-transcoder-hpa
  namespace: media
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: transcoder
  
  minReplicas: 5
  maxReplicas: 200
  
  metrics:
  # 指标1: CPU 使用率 (转码 CPU 密集)
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 80  # 转码允许高 CPU
  
  # 指标2: Memory 绝对值 (避免 OOM)
  - type: Resource
    resource:
      name: memory
      target:
        type: AverageValue
        averageValue: "6Gi"  # 每个 Pod 最多 6Gi
  
  # 指标3: 转码队列长度 (Redis)
  - type: External
    external:
      metric:
        name: redis_list_length
        selector:
          matchLabels:
            list_key: "transcode:queue"
      target:
        type: AverageValue
        averageValue: "10"  # 每个 Pod 处理 10 个任务
  
  # 指标4: 任务处理速度 (自定义指标)
  - type: Pods
    pods:
      metric:
        name: transcode_duration_seconds
        selector:
          matchLabels:
            quantile: "0.99"
      target:
        type: AverageValue
        averageValue: "120"  # P99 延迟不超过 2 分钟
  
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 30
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 600
      policies:
      - type: Pods
        value: 2
        periodSeconds: 120
```

**决策示例**:

```bash
# 当前状态: 20 个副本
# 指标计算:
# - CPU 90% → 需要 20 * (90/80) = 22.5 → 23 个副本
# - Memory 5.5Gi → 需要 20 * (5.5/6) = 18 个副本
# - 队列长度 300 → 需要 300 / 10 = 30 个副本
# - P99 延迟 180s → 需要 20 * (180/120) = 30 个副本

# 最终决策: max(23, 18, 30, 30) = 30 个副本
```

---

## 最佳实践

### 1. 合理设置资源请求

**错误示例** (未设置 requests):

```yaml
# ❌ HPA 无法工作
containers:
- name: app
  resources:
    limits:
      cpu: "2"
      memory: "4Gi"
  # 缺少 requests!
```

**正确示例**:

```yaml
# ✅ HPA 基于 requests 计算使用率
containers:
- name: app
  resources:
    requests:
      cpu: "1"       # 必需, HPA 依赖此值
      memory: "2Gi"  # 必需
    limits:
      cpu: "2"       # 可选, 防止资源耗尽
      memory: "4Gi"
```

### 2. 避免 HPA 与手动扩缩容冲突

**问题场景**:

```bash
# 手动修改副本数
kubectl scale deployment myapp --replicas=20

# HPA 会在下一个周期 (15s) 覆盖此值!
```

**解决方案**:

```yaml
# 方式1: 临时禁用 HPA
kubectl delete hpa myapp-hpa

# 方式2: 调整 minReplicas
kubectl patch hpa myapp-hpa -p '{"spec":{"minReplicas":20}}'

# 方式3: 使用 behavior 控制 (不允许缩容)
behavior:
  scaleDown:
    selectPolicy: Disabled
```

### 3. 配置 PDB 防止过度缩容

**避免缩容导致服务不可用**:

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: myapp-pdb
spec:
  minAvailable: 3  # 始终保持至少 3 个 Pod 可用
  selector:
    matchLabels:
      app: myapp
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: myapp-hpa
spec:
  minReplicas: 5  # minReplicas 应 ≥ PDB.minAvailable
  maxReplicas: 50
```

### 4. 监控和告警

**关键指标监控**:

```yaml
# Prometheus 告警规则
groups:
- name: hpa-alerts
  rules:
  # HPA 达到最大副本数
  - alert: HPAMaxedOut
    expr: |
      kube_horizontalpodautoscaler_status_current_replicas{job="kube-state-metrics"}
        >=
      kube_horizontalpodautoscaler_spec_max_replicas{job="kube-state-metrics"}
    for: 15m
    annotations:
      summary: "HPA {{ $labels.namespace }}/{{ $labels.horizontalpodautoscaler }} 已达到最大副本数"
  
  # HPA 无法获取指标
  - alert: HPAMetricUnavailable
    expr: |
      kube_horizontalpodautoscaler_status_condition{condition="ScalingActive",status="false"} == 1
    for: 5m
    annotations:
      summary: "HPA 无法获取指标"
```

**查看 HPA 事件**:

```bash
# 查看 HPA 决策历史
kubectl describe hpa myapp-hpa

# Events:
#   Type     Reason             Message
#   Normal   SuccessfulRescale  New size: 8; reason: cpu resource utilization (percentage of request) above target
```

### 5. 指标选择建议

| 应用类型       | 推荐指标                          | 目标值建议        |
|----------------|-----------------------------------|-------------------|
| 无状态 API     | CPU Utilization                   | 60-70%            |
| 内存密集型     | Memory AverageValue               | 80% of Limit      |
| 队列消费者     | External (Queue Length)           | 每 Pod 100-1000   |
| Web 前端       | Pods (QPS)                        | 每 Pod 500-2000   |
| 数据库 (只读)  | ContainerResource (db CPU)        | 70%               |
| 批处理任务     | Object (Job Completion Time)      | < 30 分钟         |

### 6. 测试和验证

**负载测试**:

```bash
# 使用 hey 压测
hey -z 5m -c 100 -q 10 http://myapp.example.com/api

# 观察 HPA 行为
watch kubectl get hpa myapp-hpa

# 验证扩缩容时间
kubectl get events --sort-by='.lastTimestamp' | grep myapp
```

---

## 常见问题

### Q1: HPA 不工作,状态显示 `<unknown>`?

**症状**:

```bash
kubectl get hpa
# TARGETS        REPLICAS
# <unknown>/70%  3/10
```

**排查步骤**:

```bash
# 1. 检查 Metrics Server 是否运行
kubectl get deployment metrics-server -n kube-system
kubectl logs -n kube-system deployment/metrics-server

# 2. 验证指标 API 可访问
kubectl get --raw /apis/metrics.k8s.io/v1beta1/pods

# 3. 检查 Pod 是否有 resources.requests
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].resources}'

# 4. 查看 HPA 详细信息
kubectl describe hpa myapp-hpa
# Conditions:
#   Type: ScalingActive
#   Status: False
#   Reason: FailedGetResourceMetric
#   Message: missing request for cpu
```

**解决方案**:
- 确保 Metrics Server 正常运行
- Pod 必须设置 `resources.requests`

### Q2: HPA 频繁扩缩容 (抖动)?

**原因**: 指标波动 + 稳定窗口过短

**解决方案**:

```yaml
behavior:
  scaleUp:
    # 增加扩容稳定窗口
    stabilizationWindowSeconds: 60  # 从 0 改为 60
  scaleDown:
    # 增加缩容稳定窗口
    stabilizationWindowSeconds: 600  # 从 300 改为 600
```

### Q3: 自定义指标 HPA 不生效?

**排查 Custom Metrics API**:

```bash
# 1. 检查 API 注册
kubectl get apiservices | grep custom.metrics

# 2. 查询自定义指标
kubectl get --raw "/apis/custom.metrics.k8s.io/v1beta1/namespaces/default/pods/*/http_requests_per_second" | jq .

# 3. 检查 Prometheus Adapter 配置
kubectl get cm adapter-config -n monitoring -o yaml

# 4. 查看 Adapter 日志
kubectl logs -n monitoring deployment/prometheus-adapter
```

### Q4: HPA 达到 maxReplicas 但仍超负载?

**临时解决**:

```bash
# 提高 maxReplicas
kubectl patch hpa myapp-hpa -p '{"spec":{"maxReplicas":200}}'
```

**长期优化**:
- 检查资源配置是否合理
- 优化应用性能 (减少 CPU/Memory 消耗)
- 考虑分片架构

### Q5: HPA 与 Cluster Autoscaler 冲突?

**场景**: HPA 扩容但节点资源不足

**解决方案**:

```yaml
# 1. 确保 Cluster Autoscaler 正常运行
kubectl logs -n kube-system deployment/cluster-autoscaler

# 2. 配置合理的 Pod 优先级
spec:
  priorityClassName: high-priority

# 3. 监控节点扩容事件
kubectl get events --field-selector reason=TriggeredScaleUp
```

---

## 版本兼容性对照

| 功能                    | v1.25 | v1.26 | v1.27 | v1.28 | v1.30 | v1.32 |
|-------------------------|-------|-------|-------|-------|-------|-------|
| HPA v2 GA               | ✅    | ✅    | ✅    | ✅    | ✅    | ✅    |
| behavior 字段 GA        | ✅    | ✅    | ✅    | ✅    | ✅    | ✅    |
| ContainerResource       | ❌    | ❌    | ✅    | ✅    | ✅    | ✅    |
| External Metrics API    | ✅    | ✅    | ✅    | ✅    | ✅    | ✅    |

---

## 参考资料

- [HPA 官方文档](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/)
- [HPA Walkthrough](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale-walkthrough/)
- [Custom Metrics API](https://github.com/kubernetes/metrics)
- [Prometheus Adapter](https://github.com/kubernetes-sigs/prometheus-adapter)
- [VPA 社区项目](https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler)

---

**文档维护**: 建议每季度更新，关注 ContainerResource 和新指标类型的演进。
