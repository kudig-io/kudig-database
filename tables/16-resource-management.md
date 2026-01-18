# 16 - 资源管理表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/concepts/configuration/manage-resources-containers](https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/)

## 资源类型概览

| 资源名称 | 用途 | API组/版本 | 版本引入 | 稳定版本 | 生产必要性 |
|---------|------|-----------|---------|---------|-----------|
| **ResourceQuota** | 命名空间资源配额 | core/v1 | v1.0 | v1.0 | 多租户必需 |
| **LimitRange** | 默认资源限制 | core/v1 | v1.0 | v1.0 | 推荐 |
| **HorizontalPodAutoscaler** | 水平自动扩缩容 | autoscaling/v2 | v1.1 | v1.23 | 弹性必需 |
| **VerticalPodAutoscaler** | 垂直自动扩缩容 | autoscaling.k8s.io/v1 | 外部项目 | v1.0 | 推荐 |
| **PodDisruptionBudget** | 中断预算 | policy/v1 | v1.4 | v1.21 | 高可用必需 |
| **PriorityClass** | Pod优先级 | scheduling.k8s.io/v1 | v1.8 | v1.14 | 推荐 |

## ResourceQuota完整配置

### 配额类型详解

| 配额类型 | 资源名 | 说明 | 示例值 | 适用场景 |
|---------|-------|------|-------|---------|
| **计算资源** | requests.cpu | CPU请求总量 | 100 | 限制总CPU使用 |
| | requests.memory | 内存请求总量 | 100Gi | 限制总内存使用 |
| | limits.cpu | CPU限制总量 | 200 | 防止CPU超分 |
| | limits.memory | 内存限制总量 | 200Gi | 防止内存超分 |
| **对象计数** | pods | Pod数量 | 100 | 限制Pod密度 |
| | services | Service数量 | 50 | 控制Service数量 |
| | secrets | Secret数量 | 100 | 防止Secret滥用 |
| | configmaps | ConfigMap数量 | 100 | 控制ConfigMap数量 |
| | persistentvolumeclaims | PVC数量 | 50 | 限制存储卷 |
| | services.loadbalancers | LoadBalancer数量 | 5 | 控制SLB成本 |
| | services.nodeports | NodePort数量 | 10 | 限制NodePort |
| **存储** | requests.storage | 存储请求总量 | 500Gi | 限制总存储 |
| | <sc>.storageclass.storage.k8s.io/requests.storage | 特定SC存储配额 | 100Gi | 按存储类型限制 |
| **扩展资源** | requests.nvidia.com/gpu | GPU请求总量 | 8 | 限制GPU使用 |

### 生产级ResourceQuota配置

```yaml
# 生产环境命名空间配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: production-quota
  namespace: production
spec:
  hard:
    # 计算资源
    requests.cpu: "100"
    requests.memory: "200Gi"
    limits.cpu: "200"
    limits.memory: "400Gi"
    
    # 对象计数
    pods: "100"
    services: "50"
    services.loadbalancers: "5"
    services.nodeports: "10"
    secrets: "100"
    configmaps: "100"
    persistentvolumeclaims: "50"
    
    # 存储
    requests.storage: "500Gi"
    alicloud-disk-essd.storageclass.storage.k8s.io/requests.storage: "300Gi"
    alicloud-nas.storageclass.storage.k8s.io/requests.storage: "200Gi"
    
    # GPU资源
    requests.nvidia.com/gpu: "8"
  
  # 作用域选择器(仅限特定优先级)
  scopeSelector:
    matchExpressions:
    - operator: In
      scopeName: PriorityClass
      values: ["high", "medium"]

---
# 开发环境配额(更宽松)
apiVersion: v1
kind: ResourceQuota
metadata:
  name: dev-quota
  namespace: development
spec:
  hard:
    requests.cpu: "50"
    requests.memory: "100Gi"
    limits.cpu: "100"
    limits.memory: "200Gi"
    pods: "200"
    services: "100"
    services.loadbalancers: "2"
```

### 配额查看与监控

```bash
# 查看配额使用情况
kubectl get resourcequota -n production
kubectl describe resourcequota production-quota -n production

# 查看命名空间配额使用率
kubectl get resourcequota -A -o json | jq -r '.items[] | "\(.metadata.namespace): CPU \(.status.used."requests.cpu")/\(.status.hard."requests.cpu")"'

# 监控配额告警(Prometheus)
```

## LimitRange完整配置

### LimitRange类型

| 限制类型 | 适用对象 | 参数 | 说明 |
|---------|---------|------|------|
| **Container** | 容器 | default | 默认limits |
| | | defaultRequest | 默认requests |
| | | max | 最大限制 |
| | | min | 最小请求 |
| | | maxLimitRequestRatio | 最大limits/requests比率 |
| **Pod** | Pod | max | Pod级最大 |
| | | min | Pod级最小 |
| **PersistentVolumeClaim** | PVC | max | 最大存储 |
| | | min | 最小存储 |

### 生产级LimitRange配置

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: production-limits
  namespace: production
spec:
  limits:
  # 容器级限制
  - type: Container
    default:  # 默认limits
      cpu: "1"
      memory: "1Gi"
    defaultRequest:  # 默认requests
      cpu: "100m"
      memory: "128Mi"
    max:  # 最大值
      cpu: "8"
      memory: "16Gi"
    min:  # 最小值
      cpu: "50m"
      memory: "64Mi"
    maxLimitRequestRatio:  # 最大比率
      cpu: "10"
      memory: "4"
  
  # Pod级限制
  - type: Pod
    max:
      cpu: "16"
      memory: "32Gi"
    min:
      cpu: "100m"
      memory: "128Mi"
  
  # PVC限制
  - type: PersistentVolumeClaim
    max:
      storage: "500Gi"
    min:
      storage: "1Gi"
```

## HPA完整配置

### HPA v2指标类型

| 指标类型 | 来源 | 配置方式 | 版本支持 | 适用场景 |
|---------|------|---------|---------|---------|
| **Resource** | Metrics Server | CPU/Memory利用率 | v1.23+ GA | 基于资源扩缩 |
| **Pods** | Custom Metrics API | Pod级自定义指标 | v1.23+ | 业务指标 |
| **Object** | Custom Metrics API | K8S对象指标 | v1.23+ | 基于对象状态 |
| **External** | External Metrics API | 外部系统指标 | v1.23+ | 云监控指标 |
| **ContainerResource** | Metrics Server | 容器级资源 | v1.27+ | 精细控制 |

### 生产级HPA配置

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 3  # 最小副本数
  maxReplicas: 100  # 最大副本数
  
  metrics:
  # 1. CPU利用率
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70  # 目标70%
  
  # 2. 内存利用率
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80  # 目标80%
  
  # 3. Pod级指标: 每秒请求数
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"  # 每个Pod处理1000 QPS
  
  # 4. 外部指标: SLB队列长度
  - type: External
    external:
      metric:
        name: sls_queue_length
        selector:
          matchLabels:
            queue: web-app
      target:
        type: AverageValue
        averageValue: "30"  # 队列长度<30
  
  # 5. 对象指标: Ingress请求数
  - type: Object
    object:
      metric:
        name: requests_per_second
      describedObject:
        apiVersion: networking.k8s.io/v1
        kind: Ingress
        name: web-app-ingress
      target:
        type: Value
        value: "10000"  # Ingress总QPS
  
  # 扩缩容行为配置
  behavior:
    # 缩容行为
    scaleDown:
      stabilizationWindowSeconds: 300  # 稳定窗口5分钟
      policies:
      - type: Percent
        value: 10  # 每次缩容10%
        periodSeconds: 60
      - type: Pods
        value: 5  # 每次最多缩5个Pod
        periodSeconds: 60
      selectPolicy: Min  # 选择最保守策略
    
    # 扩容行为
    scaleUp:
      stabilizationWindowSeconds: 0  # 立即扩容
      policies:
      - type: Percent
        value: 100  # 每次扩容100%(翻倍)
        periodSeconds: 15
      - type: Pods
        value: 10  # 每次最多扩10个Pod
        periodSeconds: 15
      selectPolicy: Max  # 选择最激进策略
```

### HPA监控与调试

```bash
# 查看HPA状态
kubectl get hpa -n production
kubectl describe hpa web-app-hpa -n production

# 查看HPA事件
kubectl get events --field-selector involvedObject.name=web-app-hpa -n production

# 查看当前指标
kubectl get hpa web-app-hpa -n production -o yaml | grep -A 10 currentMetrics

# 模拟负载测试HPA
kubectl run -it --rm load-generator --image=busybox /bin/sh
while true; do wget -q -O- http://web-app.production.svc.cluster.local; done
```

## VPA完整配置

### VPA更新模式

| 更新模式 | 行为 | Pod重启 | 适用场景 |
|---------|------|--------|---------|
| **Off** | 仅推荐 | 否 | 观察分析 |
| **Initial** | 仅新Pod | 否 | 保守模式 |
| **Recreate** | 自动重建Pod | 是 | 主动优化 |
| **Auto** | 自动更新 | 是 | 生产使用 |

### 生产级VPA配置

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-app-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  
  updatePolicy:
    updateMode: "Auto"  # 自动更新
  
  resourcePolicy:
    containerPolicies:
    - containerName: web-app
      minAllowed:
        cpu: 100m
        memory: 128Mi
      maxAllowed:
        cpu: 8
        memory: 16Gi
      controlledResources: ["cpu", "memory"]
      controlledValues: RequestsAndLimits  # 同时更新requests和limits
      
    - containerName: sidecar
      mode: "Off"  # 不调整sidecar资源
```

### VPA与HPA共存

```yaml
# 推荐配置: VPA调整CPU/Memory，HPA基于自定义指标
---
# VPA
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app
  updatePolicy:
    updateMode: "Auto"
  resourcePolicy:
    containerPolicies:
    - containerName: "*"
      controlledResources: ["memory"]  # 仅调整内存

---
# HPA
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app
  minReplicas: 2
  maxReplicas: 100
  metrics:
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"
  # 不使用CPU/Memory指标，避免与VPA冲突
```

## PodDisruptionBudget完整配置

### PDB策略

| 参数 | 说明 | 示例值 | 注意事项 |
|-----|------|-------|---------|
| **minAvailable** | 最少可用数 | 2 或 50% | 与maxUnavailable互斥 |
| **maxUnavailable** | 最多不可用数 | 1 或 25% | 与minAvailable互斥 |
| **unhealthyPodEvictionPolicy** | 不健康Pod驱逐策略 | IfHealthyBudget | v1.27+ |

### 生产级PDB配置

```yaml
# 关键服务PDB (至少保持2个副本可用)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-app-pdb
  namespace: production
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: critical-app
      tier: frontend
  unhealthyPodEvictionPolicy: IfHealthyBudget  # v1.27+

---
# 一般服务PDB (最多1个副本不可用)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-app-pdb
  namespace: production
spec:
  maxUnavailable: 1
  selector:
    matchLabels:
      app: web-app

---
# 百分比PDB (保持80%可用)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: worker-pdb
  namespace: production
spec:
  minAvailable: 80%
  selector:
    matchLabels:
      app: worker
```

## PriorityClass完整配置

### 优先级规划

| 优先级 | 值范围 | 用途 | 抢占策略 |
|-------|--------|------|---------|
| **系统级** | 1,000,000,000+ | K8S系统组件 | PreemptLowerPriority |
| **业务关键** | 100,000-999,999 | 核心业务 | PreemptLowerPriority |
| **高优先级** | 10,000-99,999 | 重要业务 | PreemptLowerPriority |
| **普通** | 1,000-9,999 | 一般业务 | PreemptLowerPriority |
| **低优先级** | 1-999 | 批处理 | Never |
| **默认** | 0 | 未指定 | PreemptLowerPriority |

### 生产级PriorityClass配置

```yaml
# 业务关键优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: business-critical
value: 100000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "业务关键服务，可抢占低优先级Pod"

---
# 高优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 10000
preemptionPolicy: PreemptLowerPriority
description: "高优先级服务"

---
# 普通优先级(默认)
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: normal-priority
value: 1000
globalDefault: true
preemptionPolicy: PreemptLowerPriority
description: "默认优先级"

---
# 低优先级(批处理)
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: low-priority-batch
value: 100
preemptionPolicy: Never  # 不触发抢占
description: "批处理任务，不抢占其他Pod"
```

### 使用PriorityClass

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: critical-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: critical-service
  template:
    metadata:
      labels:
        app: critical-service
    spec:
      priorityClassName: business-critical
      containers:
      - name: app
        image: critical-service:latest
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
```

## 资源请求与限制最佳实践

### QoS等级详解

| QoS等级 | 条件 | 驱逐优先级 | 适用场景 | CPU限制建议 |
|--------|------|-----------|---------|-----------|
| **Guaranteed** | requests=limits(CPU和Memory) | 最低(最后驱逐) | 关键数据库 | 设置limits |
| **Burstable** | requests<limits或部分设置 | 中等 | 一般服务 | 可不设limits |
| **BestEffort** | 无requests和limits | 最高(最先驱逐) | 批处理 | 不设置 |

### 资源配置模板

```yaml
# Guaranteed QoS (关键服务)
apiVersion: v1
kind: Pod
metadata:
  name: guaranteed-pod
spec:
  containers:
  - name: app
    image: critical-app:latest
    resources:
      requests:
        cpu: "2"
        memory: "4Gi"
      limits:
        cpu: "2"      # 与requests相等
        memory: "4Gi"  # 与requests相等

---
# Burstable QoS (一般服务)
apiVersion: v1
kind: Pod
metadata:
  name: burstable-pod
spec:
  containers:
  - name: app
    image: web-app:latest
    resources:
      requests:
        cpu: "500m"
        memory: "1Gi"
      limits:
        # CPU不设限制，允许突发
        memory: "2Gi"  # Memory必须设限制

---
# BestEffort QoS (批处理)
apiVersion: v1
kind: Pod
metadata:
  name: besteffort-pod
spec:
  containers:
  - name: batch-job
    image: batch-processor:latest
    # 不设置resources
```

### 资源配置推荐值

| 应用类型 | CPU Request | CPU Limit | Memory Request | Memory Limit | 说明 |
|---------|------------|-----------|----------------|-------------|------|
| **Web前端** | 100m | 不设置 | 128Mi | 512Mi | 允许CPU突发 |
| **API服务** | 500m | 2核 | 512Mi | 2Gi | 限制CPU防止雪崩 |
| **数据库** | 2核 | 2核 | 4Gi | 4Gi | Guaranteed QoS |
| **缓存** | 500m | 不设置 | 1Gi | 1Gi | 允许CPU突发 |
| **批处理** | 不设置 | 不设置 | 不设置 | 不设置 | 使用剩余资源 |
| **Sidecar** | 50m | 200m | 64Mi | 256Mi | 限制辅助容器 |

## 资源监控与告警

### Prometheus监控指标

```yaml
groups:
- name: resource_alerts
  rules:
  # ResourceQuota使用率告警
  - alert: NamespaceQuotaExceeding
    expr: |
      kube_resourcequota{resource="requests.cpu", type="used"} / 
      kube_resourcequota{resource="requests.cpu", type="hard"} > 0.85
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Namespace {{ $labels.namespace }} quota exceeding 85%"
  
  # HPA达到最大值
  - alert: HPAMaxedOut
    expr: |
      kube_horizontalpodautoscaler_status_current_replicas == 
      kube_horizontalpodautoscaler_spec_max_replicas
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "HPA {{ $labels.namespace }}/{{ $labels.horizontalpodautoscaler }} at max replicas"
  
  # PDB违规
  - alert: PodDisruptionBudgetViolation
    expr: |
      kube_poddisruptionbudget_status_current_healthy < 
      kube_poddisruptionbudget_status_desired_healthy
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "PDB {{ $labels.namespace }}/{{ $labels.poddisruptionbudget }} violated"
  
  # 容器CPU限流
  - alert: ContainerCPUThrottling
    expr: |
      rate(container_cpu_cfs_throttled_seconds_total[5m]) > 0.3
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Container {{ $labels.namespace }}/{{ $labels.pod }}/{{ $labels.container }} CPU throttling > 30%"
  
  # 容器内存使用率高
  - alert: ContainerMemoryHigh
    expr: |
      container_memory_working_set_bytes / container_spec_memory_limit_bytes > 0.9
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Container {{ $labels.namespace }}/{{ $labels.pod }} memory usage > 90%"
```

### 成本优化监控

```bash
# 查看各命名空间资源使用成本
kubectl top nodes
kubectl top pods -A --sort-by=cpu
kubectl top pods -A --sort-by=memory

# 识别过度配置的Pod (requests >> usage)
kubectl get pods -A -o json | jq -r '
  .items[] | 
  select(.spec.containers[].resources.requests.memory) |
  "\(.metadata.namespace)/\(.metadata.name)"
'

# 使用Kubecost进行成本分析
kubectl port-forward -n kubecost svc/kubecost-cost-analyzer 9090
```

---

**资源管理原则**: 设置合理配额，启用自动扩缩容，监控资源使用，持续优化成本

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)
