---
title: 自动扩缩容最佳实践
description: HPA / VPA / Cluster Autoscaler / Karpenter 协同工作的最佳实践与避坑指南
summary: 四类 Autoscaler 职责划分 + 协同配置 + 冷启动优化 + 常见冲突排查
category: reliability
tags:
- slo
- sli
- reliability
- autoscaling
- hpa
- vpa
- karpenter
- capacity
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 自动扩缩容最佳实践

> **核心原则**：扩缩容不是"一个 HPA 解决一切"。HPA 管副本数、VPA 管单副本资源、Cluster Autoscaler/Karpenter 管节点——**四者职责不同，必须分层协同**。让任何一个越界都会导致震荡、成本失控或扩容失败。

## 四层 Autoscaler 职责矩阵

```
┌─────────────────────────────────────────────┐
│ Karpenter / Cluster Autoscaler              │  节点层：Pod Pending → 加节点
├─────────────────────────────────────────────┤
│ HPA   (Horizontal)                          │  副本层：负载高 → 加副本
├─────────────────────────────────────────────┤
│ VPA   (Vertical)                            │  容器层：OOM/CPU → 调大 requests
├─────────────────────────────────────────────┤
│ Application (内置限流/降级)                   │  应用层：自保
└─────────────────────────────────────────────┘
```

| 扩缩容器 | 响应时间 | 适用场景 | 风险 |
|---------|---------|---------|------|
| HPA | 秒–分钟 | 流量型扩容 | 节点不够会卡 Pending |
| VPA | 分钟–小时 | 资源配比优化 | 重启 Pod（Live 模式） |
| Cluster Autoscaler | 分钟 | 节点补足 | 冷启动慢（2–5 分钟） |
| Karpenter | 秒级 | 节点补足（快） | 需 Spot 管理 |

## 1. HPA：基于自定义指标 + 多策略

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata: { name: api }
spec:
  scaleTargetRef: { apiVersion: apps/v1, kind: Deployment, name: api }
  minReplicas: 3
  maxReplicas: 50
  metrics:
  - type: Resource
    resource: { name: cpu, target: { type: Utilization, averageUtilization: 70 } }
  - type: Pods                      # 自定义指标（RPS）
    pods:
      metric: { name: http_requests_per_second }
      target: { type: AverageValue, averageValue: "500" }
  behavior:                         # ★ 防震荡的关键
    scaleUp:
      stabilizationWindowSeconds: 0     # 扩容立即响应
      policies:
      - { type: Percent, value: 100, periodSeconds: 30 }   # 30s 翻倍
      selectPolicy: Max
    scaleDown:
      stabilizationWindowSeconds: 300   # ★ 缩容慢，至少稳定 5 分钟才缩
      policies:
      - { type: Percent, value: 10, periodSeconds: 60 }    # 每分钟最多缩 10%
      selectPolicy: Min
```

**要点**：
- `scaleUp` 快、`scaleDown` 慢——缩容保守是防震荡的金科玉律。
- `maxReplicas` 必须有上限，否则一个指标错误能把你扩到破产。

## 2. VPA：仅推荐模式起步

🟡 **中危**：VPA `Auto` 模式会重启 Pod，生产首次启用必须用 `Off`（仅推荐）观察 2 周。

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata: { name: api }
spec:
  targetRef: { apiVersion: apps/v1, kind: Deployment, name: api }
  updatePolicy: { updateMode: "Off" }   # ★ 起步用 Off，只出建议不改 Pod
  resourcePolicy:
    containerPolicies:
    - containerName: '*'
      controlledResources: ["cpu", "memory"]
      maxAllowed: { cpu: 4, memory: 8Gi }   # ★ 设上限防失控
```

⚠️ **HPA 与 VPA 冲突铁律**：不要在同一 Deployment 上同时用 HPA（基于 CPU 利用率）和 VPA（Auto 模式）——两者会互相打架。要么 HPA 用自定义指标 + VPA Off，要么 VPA 不动 CPU。

## 3. Karpenter：秒级节点供给

```yaml
# NodePool：定义"要什么样的节点"
apiVersion: karpenter.sh/v1
kind: NodePool
metadata: { name: default }
spec:
  template:
    spec:
      requirements:
      - key: kubernetes.io/arch
        operator: In
        values: ["amd64"]
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["on-demand", "spot"]   # ★ Spot 省钱但需应用层容忍
      - key: node.kubernetes.io/instance-type
        operator: In
        values: ["m6i.large","m6i.xlarge","c6i.large"]
      expireAfter: 720h                  # ★ 节点 30 天强制轮换，防漂移
  limits:
    cpu: 1000
    memory: 4000Gi
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized
    consolidateAfter: 30s
```

Karpenter vs Cluster Autoscaler：
- **Karpenter** 更快（秒级）、更省（直接选最便宜机型）、更智能（主动整合）。
- **Cluster Autoscaler** 更成熟、与各大云厂 ASG 深度集成、运维心智负担低。
- 新集群建议 Karpenter；老集群迁移前充分验证。

## 4. 冷启动优化

节点扩容要 2–5 分钟（申机器→启动→注册→调度），这期间 SLO 会破。对策：

1. **预留 buffer**：HPA `minReplicas` 设成能扛 1.5 倍日常峰值的副本数。
2. **OverProvisioning**：用低优先级的"占位 Pod"提前把节点拉起来，流量来时被高优先级 Pod 抢占：

```yaml
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata: { name: overprovisioning }
value: -1
```

3. **预热连接池**：Pod 启动后 readiness probe 之前先建好 DB/缓存连接。

## 排查 Checklist

- [ ] HPA 显示 `AbleToScale=False`？→ 检查 maxReplicas / 节点资源
- [ ] Pod 卡 Pending？→ `kubectl describe pod` 看 scheduler 事件，查 CA/Karpenter 日志
- [ ] 扩缩容震荡？→ 检查 `stabilizationWindowSeconds` 与指标抖动
- [ ] VPA 不工作？→ 确认 `updateMode` 不是 Off，且没与 HPA 争 CPU

## 监控与告警

### PrometheusRule 扩缩容告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: autoscaling-alerts
  namespace: monitoring
spec:
  groups:
    - name: autoscaling.rules
      rules:
        # HPA 达到最大副本数
        - alert: HPAMaxedOut
          expr: |
            kube_horizontalpodautoscaler_status_current_replicas
            ==
            kube_horizontalpodautoscaler_spec_max_replicas
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "HPA {{ $labels.horizontalpodautoscaler }} 已达到最大副本数"

        # HPA 无法扩容
        - alert: HPAUnableToScale
          expr: |
            kube_horizontalpodautoscaler_status_condition{condition="AbleToScale", status="false"} == 1
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "HPA {{ $labels.horizontalpodautoscaler }} 无法扩容"

        # Pod Pending 过多
        - alert: TooManyPendingPods
          expr: |
            count(kube_pod_status_phase{phase="Pending"} == 1) > 10
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "集群中有 {{ $value }} 个 Pending Pod"

        # 节点资源不足
        - alert: NodeResourcePressure
          expr: |
            (sum(kube_pod_container_resource_requests{resource="cpu"}) by (node)
            /
            sum(kube_node_status_allocatable{resource="cpu"}) by (node)) > 0.9
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} CPU 请求率超过 90%"

        # Karpenter 节点供给失败
        - alert: KarpenterProvisioningFailed
          expr: |
            rate(karpenter_provisioner_scheduling_duration_seconds_count{result="error"}[5m]) > 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Karpenter 节点供给失败"
```

### Grafana Dashboard 面板

| 面板 | PromQL | 用途 |
|-----|--------|------|
| HPA 副本数趋势 | `kube_horizontalpodautoscaler_status_current_replicas` | 观察扩缩容行为 |
| HPA 目标达成率 | `current_replicas / desired_replicas` | 判断是否卡住 |
| Pod Pending 数量 | `kube_pod_status_phase{phase="Pending"}` | 节点资源不足信号 |
| 节点资源利用率 | `instance:node_cpu_utilisation:rate5m` | 容量规划依据 |
| Karpenter 节点数 | `karpenter_nodes_count` | 节点供给趋势 |

## 成本优化

### Spot 实例策略

```yaml
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: spot-workers
spec:
  template:
    spec:
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot"]  # 仅使用 Spot
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["m6i.large", "m6i.xlarge", "c6i.large", "c6i.xlarge"]
      taints:
        - key: spot-instance
          value: "true"
          effect: PreferNoSchedule
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized
    consolidateAfter: 30s
  limits:
    cpu: 500
    memory: 2000Gi
```

### 成本监控

```bash
# 🟢 低风险：查看节点成本分布
kubectl cost namespace --show-cpu --show-memory

# 🟢 低风险：查看 Spot 节点比例
kubectl get nodes -l karpenter.sh/capacity-type=spot --no-headers | wc -l
kubectl get nodes --no-headers | wc -l

# 🟢 低风险：查看资源利用率
kubectl top nodes --sort-by=cpu
kubectl top pods -A --sort-by=cpu | head -20
```

## 多集群扩缩容

### 联邦 HPA 配置

```yaml
# 使用 KEDA 实现基于外部指标的扩缩容
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: api-scaledobject
  namespace: production
spec:
  scaleTargetRef:
    name: api
  minReplicaCount: 3
  maxReplicaCount: 100
  triggers:
    # 基于 Prometheus 指标
    - type: prometheus
      metadata:
        serverAddress: http://prometheus:9090
        metricName: http_requests_per_second
        query: |
          sum(rate(http_requests_total{job="api"}[1m]))
        threshold: "1000"
    # 基于队列长度
    - type: prometheus
      metadata:
        serverAddress: http://prometheus:9090
        metricName: queue_length
        query: |
          sum(job_queue_length{job="api"})
        threshold: "100"
```

### 跨集群流量调度

```yaml
# 使用 Istio 实现跨集群流量调度
apiVersion: networking.istio.io/v1beta1
kind: ServiceEntry
metadata:
  name: api-remote
  namespace: production
spec:
  hosts:
    - api.remote.cluster.local
  location: MESH_EXTERNAL
  ports:
    - number: 80
      name: http
      protocol: HTTP
  resolution: DNS
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: api
  namespace: production
spec:
  hosts:
    - api
  http:
    - route:
        - destination:
            host: api
            subset: local
          weight: 80
        - destination:
            host: api.remote.cluster.local
            subset: remote
          weight: 20
```

## 故障排查详解

### HPA 不工作排查流程

```
[HPA 不工作]
    │
    ├── [检查 HPA 状态]
    │   kubectl describe hpa <name>
    │       │
    │       ├── AbleToScale=False → 检查 RBAC / maxReplicas
    │       │
    │       ├── ScalingActive=False → 检查指标源
    │       │   - metrics-server 是否正常?
    │       │   - 自定义指标适配器是否正常?
    │       │
    │       └── 指标获取失败 → 检查 Prometheus Adapter
    │
    ├── [检查指标]
    │   kubectl get --raw "/apis/external.metrics.k8s.io/v1beta1"
    │       │
    │       └── 指标不存在 → 检查 KEDA / Prometheus Adapter
    │
    └── [检查目标]
        kubectl get deployment <name> -o yaml
            │
            └── replicas 被手动修改 → HPA 会覆盖手动修改
```

### Pod Pending 排查

```bash
# 🟢 低风险：查看 Pending Pod 原因
kubectl get pods -A --field-selector=status.phase=Pending

# 🟢 低风险：查看调度失败事件
kubectl describe pod <pod-name> -n <namespace> | grep -A10 Events

# 常见原因:
# 1. Insufficient cpu/memory → 扩容节点或调整 requests
# 2. node(s) didn't match node selector → 检查 nodeSelector
# 3. node(s) had taint → 检查 tolerations
# 4. pod has unbound PVC → 检查 PV/PVC
```

### 扩缩容震荡排查

```bash
# 🟢 低风险：查看 HPA 事件
kubectl describe hpa <name> | grep -A20 Events

# 震荡特征:
# - 频繁 scale up/down
# - 副本数在 min/max 之间波动

# 解决方案:
# 1. 增加 stabilizationWindowSeconds
# 2. 调整指标阈值 (避免在阈值附近波动)
# 3. 使用 behavior.policies 限制扩缩速度
```

## 生产配置模板

### 完整 HPA 配置

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: api-production
  namespace: production
  labels:
    app: api
    tier: critical
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api
  minReplicas: 5
  maxReplicas: 100
  metrics:
    # CPU 利用率
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 65
    # 内存利用率
    - type: Resource
      resource:
        name: memory
        target:
          type: Utilization
          averageUtilization: 75
    # 自定义指标: RPS
    - type: Pods
      pods:
        metric:
          name: http_requests_per_second
        target:
          type: AverageValue
          averageValue: "500"
    # 外部指标: 队列长度
    - type: External
      external:
        metric:
          name: queue_length
          selector:
            matchLabels:
              queue: api-queue
        target:
          type: AverageValue
          averageValue: "50"
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
        - type: Percent
          value: 100
          periodSeconds: 30
        - type: Pods
          value: 10
          periodSeconds: 30
      selectPolicy: Max
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Percent
          value: 10
          periodSeconds: 60
      selectPolicy: Min
```

## 相关

- [[可靠性/容量规划/02-hpa-vpa-cluster-autoscaler-karpenter.md|02 hpa vpa cluster autoscaler karpenter]]
- [[可靠性/容量规划/07-resource-right-sizing-guide.md|07 resource right sizing guide]]
- [[可靠性/容量规划/01-capacity-planning-framework.md|01 capacity planning framework]]

<!-- risk-assessed -->
