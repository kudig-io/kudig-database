---
title: HPA/VPA/Cluster Autoscaler/Karpenter 联合容量管理
description: 面向阿里云/专有云 K8s 的弹性容量管理方案，讲解 HPA、VPA、Cluster Autoscaler、Karpenter 的协同使用与最佳实践。
summary: 面向阿里云/专有云 K8s 的弹性容量管理方案，讲解 HPA、VPA、Cluster Autoscaler、Karpenter 的协同使用与最佳实践。
category: reliability
tags:
- k8s
- hpa
- vpa
- cluster-autoscaler
- karpenter
- autoscaling
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 25min
intent_queries:
- HPA VPA Cluster Autoscaler Karpenter 联合使用
- K8s 弹性伸缩最佳实践
- 阿里云 K8s 自动扩缩容
trigger_keywords:
- HPA
- VPA
- Cluster Autoscaler
- Karpenter
- 弹性伸缩
- autoscaling
prerequisites:
- kubectl-basics
- hpa-basics
- monitoring-basics
- autoscaler-basics
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




# HPA/VPA/Cluster Autoscaler/Karpenter 联合容量管理

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，系统讲解 HPA、VPA、Cluster Autoscaler、Karpenter 四层弹性方案的组合使用。

## 目录

1. [四层弹性体系](#四层弹性体系)
2. [HPA 水平 Pod 自动伸缩](#hpa-水平-pod-自动伸缩)
3. [VPA 垂直 Pod 自动伸缩](#vpa-垂直-pod-自动伸缩)
4. [Cluster Autoscaler 节点自动伸缩](#cluster-autoscaler-节点自动伸缩)
5. [Karpenter 智能节点供应](#karpenter-智能节点供应)
6. [四层协同场景](#四层协同场景)
7. [阿里云/专有云配置](#阿里云专有云配置)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 四层弹性体系

| 层次 | 组件 | 触发条件 | 作用 |
|:---|:---|:---|:---|
| Pod 水平 | HPA | CPU/内存/自定义指标 | 增减 Pod 副本数 |
| Pod 垂直 | VPA | 历史资源使用 | 调整 requests/limits |
| 节点水平 | Cluster Autoscaler | Pending Pod | 增删节点 |
| 节点供应 | Karpenter | Pending Pod + 约束 | 动态选择实例类型 |

## 2. HPA 水平 Pod 自动伸缩

### 2.1 基于 CPU 的 HPA

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 3
  maxReplicas: 50
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
        - type: Percent
          value: 100
          periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Percent
          value: 10
          periodSeconds: 120
```

### 2.2 基于自定义指标的 HPA

```yaml
metrics:
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"
```

---

## 3. VPA 垂直 Pod 自动伸缩

### 3.1 VPA 部署

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  updatePolicy:
    updateMode: "Off"  # 推荐先使用 Off 模式收集建议
  resourcePolicy:
    containerPolicies:
      - containerName: web-app
        minAllowed:
          cpu: 50m
          memory: 100Mi
        maxAllowed:
          cpu: 2
          memory: 2Gi
        controlledResources: ["cpu", "memory"]
```

### 3.2 查看 VPA 推荐

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe vpa web-vpa -n production
```
---

## 4. Cluster Autoscaler 节点自动伸缩

### 4.1 配置 ConfigMap

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-autoscaler-config
  namespace: kube-system
data:
  cluster-autoscaler.yaml: |
    expander: least-waste
    scale-down-enabled: true
    scale-down-delay-after-add: 10m
    scale-down-unneeded-time: 10m
    scale-down-utilization-threshold: 0.5
    max-node-provision-time: 15m
    skip-nodes-with-system-pods: true
    skip-nodes-with-local-storage: false
```

### 4.2 查看 CA 事件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Cluster Autoscaler 的扩缩容事件
kubectl get events -n kube-system --field-selector source=cluster-autoscaler
```
---

## 5. Karpenter 智能节点供应

### 5.1 Karpenter NodePool

```yaml
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: default
spec:
  template:
    spec:
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot", "on-demand"]
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["ecs.g7.xlarge", "ecs.g7.2xlarge", "ecs.c7.xlarge"]
        - key: topology.kubernetes.io/zone
          operator: In
          values: ["cn-hangzhou-g", "cn-hangzhou-h", "cn-hangzhou-i"]
      nodeClassRef:
        name: default
  limits:
    cpu: 1000
    memory: 4000Gi
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 720h
```

### 5.2 Karpenter NodeClass（阿里云示例）

```yaml
apiVersion: karpenter.k8s.alibabacloud/v1alpha1
kind: ECSNodeClass
metadata:
  name: default
spec:
  region: cn-hangzhou
  imageFamily: AlibabaCloudLinux3
  securityGroupSelectorTerms:
    - tags:
        karpenter.sh/discovery: "true"
  subnetSelectorTerms:
    - tags:
        karpenter.sh/discovery: "true"
  systemDisk:
    category: cloud_essd
    size: 120Gi
```

---

## 6. 四层协同场景

### 6.1 流量突增场景

1. HPA 检测到 CPU 上升，扩容 Pod
2. 新 Pod 因资源不足 Pending
3. Cluster Autoscaler / Karpenter 触发节点扩容
4. Pod 调度到新节点，业务恢复

### 6.2 负载低谷场景

1. HPA 缩容 Pod
2. 节点利用率低于阈值
3. CA 判断节点为空并缩容
4. Karpenter 回收 spot 节点

---

## 7. 阿里云/专有云配置

### 7.1 ACK 使用 Cluster Autoscaler

```bash
# 在 ACK 控制台开启节点自动伸缩
aliyun cs POST /clusters/<cluster-id>/nodes \
  --body '{"count":1,"instance_type":"ecs.g7.xlarge","nodepool_id":"<nodepool-id>"}'
```

### 7.2 专有云限制

- 专有云可能不支持 Spot 实例
- 节点模板需与 ASO 资源池匹配
- 建议先使用 Cluster Autoscaler，条件成熟后引入 Karpenter

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| HPA 配置 | 关键应用均配置 HPA | `kubectl get hpa -A` |
| VPA 建议 | 定期查看并应用 | `kubectl get vpa -A` |
| CA 开启 | 节点自动伸缩正常 | `kubectl get pods -n kube-system | grep autoscaler` |
| Karpenter NodePool | 按需配置 | `kubectl get nodepool` |
| 缩容保护 | 预留缓冲，避免震荡 | CA 配置 |
| 成本标签 | 按团队/项目归属 | Node 标签 |
| 监控告警 | 弹性失败告警 | PrometheusRule |

---

## 阿里云 ACK 自动伸缩实践

阿里云 ACK 提供节点池自动伸缩能力，可与 cluster-autoscaler 配合使用。对于 ACK 托管版，建议通过控制台或 aliyun CLI 开启节点池自动伸缩。

```bash
# 查看节点池列表
aliyun cs nodepool list --ClusterId <cluster-id>

# 开启节点池自动伸缩
aliyun cs nodepool update   --ClusterId <cluster-id>   --NodepoolId <nodepool-id>   --auto-scaling enable   --min-nodes 2   --max-nodes 50
```

### Karpenter 与 Cluster Autoscaler 选型对比

| 维度 | Cluster Autoscaler | Karpenter |
|:---|:---|:---|
| 扩容速度 | 依赖节点池与镜像预热 | 更快，直接选择实例 |
| 实例选择 | 预定义节点池 | 动态选择最佳实例 |
| 配置复杂度 | 低 | 高 |
| 多云支持 | 广泛 | 逐步扩展 |
| 缩容策略 | 基于节点利用率 | 基于 Pod 合并与过期 |

### 常见问题

| 问题 | 可能原因 | 处理建议 |
|:---|:---|:---|
| Pod 持续 Pending | 节点池 max 达到上限 | 调整节点池上限或新增节点池 |
| CA 不缩容 | Pod 分散 / PDB 限制 | 检查 PodDisruptionBudget |
| HPA 频繁抖动 | 指标波动大 | 增大 stabilizationWindowSeconds |
| VPA 推荐未被采用 | Off 模式未切换 | 评估后切换为 Auto 或 Initial |

## 容量管理最佳实践

1. **分层配置**：Pod 层使用 HPA/KEDA，节点层使用 CA/Karpenter，避免单层失效导致雪崩。
2. **缓冲节点**：保留 10%-15% 的缓冲容量，应对突发流量与调度碎片。
3. **多实例规格**：节点池包含多种实例规格，提高调度成功率。
4. **缩容保护**：设置 PDB 与优雅终止时间，避免缩容影响业务。
5. **成本标签**：为不同业务线设置 label，便于成本分摊。

### HPA 与 VPA 协同建议

- 同时使用 HPA 与 VPA 时，VPA 建议使用 Initial 或 Off 模式，避免两者冲突。
- 对批量任务使用 VPA 调整 request，对在线服务使用 HPA 调整副本。

## 典型工单场景与处理

**场景**：应用 CPU 利用率持续 90% 以上，但 HPA 未扩容。

处理步骤：
1. 检查 HPA 当前副本数与 maxReplicas。
2. 查看 metrics-server 是否可用，HPA 是否能获取指标。
3. 确认 Deployment 的 request 设置是否合理。
4. 如为短暂峰值，观察 stabilizationWindowSeconds。
5. 如为持续性负载，提高 request 或调整 HPA 阈值。

## 阿里云 ACK 控制台操作路径

在阿里云 ACK 中，可以通过控制台完成节点池自动伸缩与 HPA 配置：

1. 登录 ACK 控制台 → 选择目标集群。
2. 进入 **节点管理 → 节点池**，开启自动伸缩并设置最小/最大节点数。
3. 进入 **运维管理 → 容器伸缩**，创建 HPA 规则或查看现有规则。
4. 在 **Prometheus 监控** 中配置资源利用率告警。

### 命令速查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 HPA 当前状态
kubectl get hpa -n production

# 查看 CA 日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=50

# 查看 Karpenter 日志
kubectl logs -n karpenter -l app.kubernetes.io/name=karpenter --tail=50
```
### 容量管理常见误区

| 误区 | 正确做法 |
|:---|:---|
| 只设置 HPA 不设置 CA | 必须同时配置节点层扩容 |
| request 设置过大 | 根据实际使用调整，提高利用率 |
| 忽略缩容冷却 | 配置合理的 stabilizationWindowSeconds |
| 所有应用共用同一节点池 | 按业务 SLA 划分节点池 |

## 容量伸缩排障决策树

```
Pod 资源使用高
  │
  ├─ request 设置过低 → 调整 VPA / 手动修改 request
  │
  ├─ 副本数已达 maxReplicas → 提高 maxReplicas 或扩容节点池
  │
  ├─ 指标获取异常 → 检查 metrics-server / Prometheus
  │
  └─ 扩容行为过于激进 → 调整 stabilizationWindowSeconds 与 policies
```

### 阿里云 ACK 节点池扩展示例

```bash
# 创建支持自动伸缩的节点池
aliyun cs nodepool create   --ClusterId <cluster-id>   --Name spot-pool   --InstanceTypes ecs.g7.xlarge,ecs.g7.2xlarge   --ScalingGroupId <sg-id>   --VSwitchIds '["vsw-xxx","vsw-yyy"]'   --MinNodes 0   --MaxNodes 50   --AutoScaling enable
```

## HPA 与 VPA 联合使用模式

生产环境中，HPA 与 VPA 可以按以下模式协同：

| 模式 | 说明 | 适用场景 |
|:---|:---|:---|
| HPA + VPA Off | VPA 仅输出推荐，HPA 负责扩缩容 | 在线服务 |
| HPA + VPA Initial | VPA 设置初始 request，HPA 管理副本 | 新应用上线 |
| VPA Auto + 固定副本 | VPA 调整资源，副本数固定 | 批处理任务 |
| HPA + CA | 标准组合，无需 VPA | 无状态服务 |

### 容量管理关键指标

| 指标 | PromQL | 用途 |
|:---|:---|:---|
| Pod CPU 利用率 | `rate(container_cpu_usage_seconds_total[5m]) / kube_pod_container_resource_requests{resource="cpu"}` | HPA 输入 |
| 节点 CPU 利用率 | `1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m]))` | 触发 CA |
| Pending Pod 数 | `kube_pod_status_phase{phase="Pending"}` | 容量不足信号 |
| 节点池容量 | `sum(kube_node_status_capacity{resource="cpu"})` | 总容量 |

## Related

- [[domain-09-reliability-engineering/容量规划/24-capacity-planning-forecasting.md|容量规划与预测]]
- [[domain-09-reliability-engineering/容量规划/01-capacity-planning-framework.md|容量规划框架]]

## See Also

- [[domain-07-platform-engineering/99-karpenter-node-autoscaling-guide.md|Karpenter 节点自动扩缩容指南]]
- [[domain-10-troubleshooting-diagnostics/资源排障/17-hpa-vpa-troubleshooting.md|HPA/VPA 故障诊断]]


<!-- risk-assessed -->
