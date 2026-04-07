# Autoscaling Workloads

## 概述
自动扩缩容（Autoscaling）允许工作负载根据资源需求自动调整规模，使集群能够更弹性和高效地响应变化。Kubernetes 支持水平扩缩容（增加/减少副本数）和垂直扩缩容（调整单个 Pod 的资源）。

## 核心概念/原理
- **水平扩缩容（Horizontal Scaling）**：通过增加或减少 Pod 副本数来应对负载变化。
- **垂直扩缩容（Vertical Scaling）**：通过调整现有 Pod 的 CPU/内存 request/limit 来应对资源需求变化。
- **手动扩缩容**：
  - 水平：`kubectl scale` 或修改 `spec.replicas`。
  - 垂直：通过 patch 修改 Pod 或工作负载的资源定义，或使用原地 resize 功能。
- **自动扩缩容**：
  - **HPA（HorizontalPodAutoscaler）**：根据 CPU、内存或自定义指标自动调整副本数。
  - **VPA（VerticalPodAutoscaler）**：根据历史资源使用情况自动调整 Pod 的资源请求和限制。
  - **Cluster Proportional Autoscaler**：根据集群节点数/核心数自动水平扩缩容。
  - **Cluster Proportional Vertical Autoscaler**：根据集群规模自动垂直调整资源请求（Beta）。
  - **KEDA（Kubernetes Event Driven Autoscaler）**：基于事件（如队列消息数）驱动扩缩容。
  - **定时扩缩容**：可通过 KEDA 的 `Cron` scaler 按时间表扩缩容。

## 关键机制或特性
- **HPA**：Kubernetes 核心 API 资源和控制器，周期（默认 15 秒）根据指标调整目标副本数。
- **VPA**：以 CRD 形式提供，需单独安装。包含三个组件：Recommender（分析并生成推荐）、Updater（驱逐 Pod 或原地更新资源）、Admission Controller（在 Pod 创建时注入推荐资源）。
- **原地垂直扩缩容（In-place Pod Vertical Scaling，v1.35 Stable）**：允许在不重新创建 Pod 的情况下调整 CPU 和内存资源；VPA 与原地扩缩容的集成仍在发展中。
- **Metrics Server**：HPA 和 VPA 通常需要 Metrics Server 作为指标来源。

## 使用场景
- 流量波动大的 Web 应用和 API 服务，使用 HPA 自动扩容。
- 资源使用难以预估的应用，使用 VPA 自动 rightsizing。
- 系统级服务（如 DNS）需要根据集群规模自动调整，使用 Cluster Proportional Autoscaler。
- 基于消息队列的批处理任务，使用 KEDA 根据队列深度自动扩容。
- 需要在非高峰时段降本的场景，使用 KEDA Cron scaler 定时缩容。

## 最佳实践/注意事项
- 使用 HPA 时，建议从 Deployment/StatefulSet 清单中移除 `spec.replicas`，避免与声明式应用冲突。
- 部署 VPA 前需确认 Metrics Server 已安装并正常工作。
- HPA 和 VPA 同时作用于同一资源时需谨慎，可能出现冲突；通常建议对同一工作负载不同时启用两者的自动模式。
- 若工作负载级别的扩缩容仍无法满足需求，可进一步考虑节点自动扩缩容（Cluster Autoscaler）。

## 生产 YAML 示例

### HPA 多指标自动扩缩

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-api-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-api
  minReplicas: 3
  maxReplicas: 50
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second     # 自定义指标
      target:
        type: AverageValue
        averageValue: "1000"
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60       # 扩容冷却 1 分钟
      policies:
      - type: Percent
        value: 100                         # 每次最多翻倍
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300      # 缩容冷却 5 分钟
      policies:
      - type: Percent
        value: 10                          # 每次最多缩 10%
        periodSeconds: 60
```

### VPA 资源推荐（Off 模式 + 手动应用）

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-api-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-api
  updatePolicy:
    updateMode: "Off"                      # 仅推荐，不自动修改
  resourcePolicy:
    containerPolicies:
    - containerName: api
      minAllowed:
        cpu: "100m"
        memory: "128Mi"
      maxAllowed:
        cpu: "4"
        memory: "8Gi"
      controlledResources: ["cpu", "memory"]
```

### KEDA 基于消息队列扩缩

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: order-processor
  namespace: production
spec:
  scaleTargetRef:
    name: order-processor
  pollingInterval: 15
  cooldownPeriod: 300
  minReplicaCount: 1
  maxReplicaCount: 100
  triggers:
  - type: rabbitmq
    metadata:
      host: amqp://rabbitmq.production.svc:5672
      queueName: orders
      queueLength: "50"          # 每 50 条消息增加 1 个副本
  - type: cron
    metadata:
      timezone: Asia/Shanghai
      start: "0 9 * * *"         # 早 9 点开始高峰预扩容
      end: "0 21 * * *"          # 晚 9 点结束
      desiredReplicas: "10"
```

## 扩缩容方案对比矩阵

| 维度 | HPA | VPA | KEDA | Cluster Proportional |
|------|-----|-----|------|---------------------|
| 扩缩维度 | 水平（副本数） | 垂直（CPU/内存） | 水平 + 事件驱动 | 水平（按集群规模） |
| 指标来源 | Resource/Custom/External | 历史资源使用 | 50+ 事件源 | 节点数/核心数 |
| 是否核心 API | 是 | CRD（需安装） | CRD（需安装） | 独立组件 |
| 是否需要 Metrics Server | 是 | 是 | 取决于 trigger | 否 |
| 零副本缩容 | 否（minReplicas≥1） | N/A | 是（从 0 扩容） | 否 |
| 典型场景 | Web/API 服务 | 资源 rightsizing | 消息队列/事件驱动 | CoreDNS/监控 |
| 与 HPA 共存 | — | 建议分指标 | 替代 HPA | 互不冲突 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| HPA 显示 `<unknown>` 指标 | Metrics Server 未安装或不可用 | `kubectl top pods`；`kubectl get apiservice v1beta1.metrics.k8s.io` |
| 副本数频繁抖动（flapping） | 缺少 stabilizationWindow 或阈值设置不合理 | 增大 `behavior.scaleDown.stabilizationWindowSeconds` |
| VPA 推荐值不更新 | VPA Recommender 组件异常 | `kubectl get vpa -o yaml` 查看 `status.recommendation` |
| KEDA 不触发扩容 | trigger 配置错误或无法连接事件源 | `kubectl get scaledobject -o yaml` 查看 status；检查 KEDA operator logs |
| HPA 和 VPA 冲突 | 两者同时调整同一维度 | VPA 设为 Off 模式仅做推荐；HPA 管理副本数，VPA 管理资源 |
| 扩容后 Pod 长时间 Pending | 集群节点资源不足 | 配合 Cluster Autoscaler/Karpenter 自动扩节点 |

## 生产检查清单

- [ ] Metrics Server 已安装且正常工作
- [ ] HPA 使用时从 Deployment manifest 中移除 `spec.replicas`
- [ ] 配置 `behavior` 控制扩缩速率和冷却窗口
- [ ] VPA 与 HPA 不同时使用 Auto 模式调整同一指标
- [ ] KEDA 的 trigger 连接凭证使用 TriggerAuthentication（非明文）
- [ ] 设置合理的 `minReplicas` 和 `maxReplicas` 防止极端扩缩
- [ ] 监控 HPA 的 `currentReplicas` vs `desiredReplicas` 差异
- [ ] 节点层面配合 Cluster Autoscaler 保障扩容空间

## 命令快速参考

```bash
# 查看 HPA 状态
kubectl get hpa -n production
kubectl describe hpa web-api-hpa -n production

# 查看 VPA 推荐值
kubectl get vpa web-api-vpa -n production -o jsonpath='{.status.recommendation.containerRecommendations}' | jq .

# 手动扩缩
kubectl scale deployment web-api --replicas=10 -n production

# 创建快速 HPA
kubectl autoscale deployment web-api --min=3 --max=20 --cpu-percent=70 -n production

# 查看 Pod 实际资源使用
kubectl top pods -n production --sort-by=cpu

# 查看 KEDA ScaledObject 状态
kubectl get scaledobject -n production -o wide

# 检查 Metrics Server
kubectl get apiservice v1beta1.metrics.k8s.io -o yaml
```

## 交叉引用

- [水平 Pod 自动扩缩](horizontal-pod-autoscaling.md) — HPA 的深入配置和多指标策略
- [垂直 Pod 自动扩缩](vertical-pod-autoscaling.md) — VPA 的模式选择和组件架构
- [工作负载管理](managing-workloads.md) — 手动扩缩容和 kubectl scale 操作
- [Spot 与可抢占工作负载](spot-and-preemptible-workloads.md) — 基于成本的弹性扩缩策略

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/autoscaling/
