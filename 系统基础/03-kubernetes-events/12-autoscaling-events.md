---
title: 12 - 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)
description: '- [HPA 事件详解](#hpa-事件详解)'
summary: '- [HPA 事件详解](#hpa-事件详解)'
category: kubernetes-events
tags:
- k8s
- events
- troubleshooting
- apiserver
- kubelet
- prometheus
- hpa
- vpa
- pdb
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler) 是什么
- 如何 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)
- Kubernetes 33 kubernetes events 最佳实践
trigger_keywords:
- 自动扩缩容事件
- HPA
- VPA
- Cluster
- Autoscaler
- kubernetes
- events
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- gpu-scheduling-basics
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




# 12 - 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档详细记录 HPA、VPA 和 Cluster Autoscaler 产生的所有自动扩缩容相关事件。**

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [事件总览](#事件总览)
- [HPA 事件详解](#hpa-事件详解)
- [VPA 事件详解](#vpa-事件详解)
- [Cluster Autoscaler 事件详解](#cluster-autoscaler-事件详解)
- [HPA 决策算法](#hpa-决策算法)
- [VPA 工作模式](#vpa-工作模式)
- [CA 扩缩容决策逻辑](#ca-扩缩容决策逻辑)
- [Behavior 行为配置](#behavior-行为配置)
- [故障排查场景](#故障排查场景)
- [最佳实践](#最佳实践)

---

<!-- chunk: 事件总览 -->## 事件总览

## 事件统计表

| 组件 | 事件类型 | 事件数量 | 主要用途 |
|------|---------|---------|---------|
| **HPA** | Normal | 5 | 扩缩容成功、计算完成 |
| | Warning | 11 | 指标获取失败、计算错误 |
| **VPA** | Normal | 3 | 驱逐 Pod、提供建议、更新检查点 |
| | Warning | 1 | 更新失败 |
| **Cluster Autoscaler** | Normal | 5 | 节点扩缩容成功 |
| | Warning | 3 | 扩缩容失败 |
| **总计** | - | **28** | 自动扩缩容全生命周期 |

## 事件频率分级

| 频率级别 | 事件数量 | 代表事件 |
|---------|---------|---------|
| **高频** (每次扩缩容) | 3 | SuccessfulRescale, DesiredReplicasComputed, AbleToScale |
| **中频** (定期触发) | 7 | FailedGetResourceMetric, ReadyForNewScale, TriggeredScaleUp |
| **低频** (异常/特殊) | 13 | FailedRescale, EvictedByVPA, ScaleDownFailed |
| **罕见** (配置错误) | 5 | InvalidMetricSourceType, InvalidSelector |

---

<!-- chunk: HPA 事件详解 -->## HPA 事件详解

## 1. SuccessfulRescale

**基本信息**
```yaml
事件名称: SuccessfulRescale
类型: Normal
引入版本: v1.1+
组件: horizontal-pod-autoscaler
频率: 高频 (每次扩缩容成功)
```

**事件格式**
```
Type: Normal
Reason: SuccessfulRescale
Message: New size: 5; reason: cpu resource utilization (percentage of request) above target
Age: 30s
From: horizontal-pod-autoscaler
```

**触发条件**
- HPA 成功更改 Deployment/ReplicaSet 的副本数
- 指标满足扩缩容条件
- Scale 操作执行成功

**示例场景**

**场景 1: CPU 使用率超过目标触发扩容**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 HPA 事件
kubectl describe hpa web-app

Events:
  Type    Reason             Age   Message
  ----    ------             ----  -------
  Normal  SuccessfulRescale  45s   New size: 5; reason: cpu resource utilization (percentage of request) above target
```
**场景 2: 自定义指标触发扩容**
```bash
Events:
  Normal  SuccessfulRescale  1m    New size: 8; reason: http_requests_per_second above target
```

**排查建议**
✅ **正常事件** - 确认扩缩容符合预期
- 检查新副本数是否合理
- 验证指标趋势是否稳定
- 监控资源使用情况

**相关配置**
```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

---

## 2. FailedRescale

**基本信息**
```yaml
事件名称: FailedRescale
类型: Warning
引入版本: v1.1+
组件: horizontal-pod-autoscaler
频率: 低频 (扩缩容失败)
```

**事件格式**
```
Type: Warning
Reason: FailedRescale
Message: New size: 8; reason: failed to update deployment.apps/web-app scale: Operation cannot be fulfilled
Age: 15s
From: horizontal-pod-autoscaler
```

**触发条件**
- HPA 计算出新副本数，但 Scale 操作失败
- API Server 返回错误
- 资源冲突或限制

**常见原因**

**原因 1: 资源配额限制**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 ResourceQuota
kubectl get resourcequota -n production

NAME            CREATED AT
compute-quota   2026-02-01T10:00:00Z

kubectl describe resourcequota compute-quota

Status:
  Hard:
    requests.cpu: 100
    requests.memory: 200Gi
    pods: 50  # 达到上限
  Used:
    pods: 50
```
**原因 2: PodDisruptionBudget 阻止缩容**
```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-app-pdb
spec:
  minAvailable: 5
  selector:
    matchLabels:
      app: web-app
# 如果当前有 5 个 Pod，HPA 无法缩容到 3
```

**原因 3: Deployment 并发更新冲突**
```bash
Events:
  Warning  FailedRescale  10s  the object has been modified; please apply your changes to the latest version
```

**排查步骤**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 HPA 状态
kubectl get hpa web-app -o yaml | grep -A 10 conditions

# 2. 检查目标资源状态
kubectl get deployment web-app -o yaml | grep -A 5 status

# 3. 查看 API Server 日志
kubectl logs -n kube-system kube-apiserver-master-1 | grep "web-app"

# 4. 检查 ResourceQuota
kubectl describe resourcequota -n production
```
**解决方案**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 方案 1: 调整 ResourceQuota
kubectl edit resourcequota compute-quota
# 增加 pods 限制

# 方案 2: 调整 PDB
kubectl edit pdb web-app-pdb
# 降低 minAvailable

# 方案 3: 等待并发冲突解决
# HPA 会自动重试
```
---

## 3. DesiredReplicasComputed

**基本信息**
```yaml
事件名称: DesiredReplicasComputed
类型: Normal
引入版本: v1.1+
组件: horizontal-pod-autoscaler
频率: 高频 (每个评估周期)
```

**事件格式**
```
Type: Normal
Reason: DesiredReplicasComputed
Message: Computed desired replicas: 8 (from current 5, based on cpu utilization: 85%)
Age: 1m
From: horizontal-pod-autoscaler
```

**触发条件**
- HPA 完成一次指标评估
- 计算出期望副本数
- 无论是否执行扩缩容

**算法说明**
```
desiredReplicas = ceil[currentReplicas * (currentMetricValue / targetMetricValue)]

示例:
currentReplicas = 5
currentMetricValue = 85% (CPU 使用率)
targetMetricValue = 70%

desiredReplicas = ceil[5 * (85 / 70)] = ceil[6.07] = 7
```

**示例场景**

**场景 1: 计算后无需扩缩容**
```bash
Events:
  Normal  DesiredReplicasComputed  30s  Computed desired replicas: 5 (current 5, cpu: 68%)
  # 68% < 70%，无需扩容
```

**场景 2: 计算后需要扩容**
```bash
Events:
  Normal  DesiredReplicasComputed  45s  Computed desired replicas: 8 (current 5, cpu: 95%)
  Normal  AbleToScale              44s  Recommended replicas: 8
  Normal  SuccessfulRescale        43s  New size: 8
```

**调试信息**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看详细计算过程
kubectl get hpa web-app -o yaml

status:
  currentMetrics:
  - type: Resource
    resource:
      name: cpu
      current:
        averageUtilization: 85
        averageValue: 850m
  desiredReplicas: 8
  currentReplicas: 5
```
---

## 4. FailedGetResourceMetric

**基本信息**
```yaml
事件名称: FailedGetResourceMetric
类型: Warning
引入版本: v1.6+
组件: horizontal-pod-autoscaler
频率: 中频 (指标获取失败)
```

**事件格式**
```
Type: Warning
Reason: FailedGetResourceMetric
Message: failed to get cpu utilization: unable to get metrics for resource cpu: no metrics returned from resource metrics API
Age: 30s
From: horizontal-pod-autoscaler
```

**触发条件**
- 无法从 Metrics Server 获取资源指标
- Metrics Server 不可用
- Pod 未定义 resource requests

**常见原因**

**原因 1: Metrics Server 未安装或不可用**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Metrics Server
kubectl get deployment metrics-server -n kube-system

Error: deployments.apps "metrics-server" not found

# 安装 Metrics Server
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml
```
**原因 2: Pod 未定义 resources.requests**
```yaml
# 错误配置 - 缺少 resources.requests
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  template:
    spec:
      containers:
      - name: app
        image: nginx:1.21
        # 缺少 resources 定义！

---
# 正确配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  template:
    spec:
      containers:
      - name: app
        image: nginx:1.21
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
```

**原因 3: Metrics Server API 异常**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Metrics Server 日志
kubectl logs -n kube-system deployment/metrics-server

E0210 10:15:30.123456       1 manager.go:111] unable to fully collect metrics: [unable to fully scrape metrics from source kubelet_summary:node1: unable to fetch metrics from Kubelet node1 (node1): Get "https://node1:10250/stats/summary?only_cpu_and_memory=true": x509: certificate signed by unknown authority]
```
**排查步骤**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Metrics Server 状态
kubectl get apiservice v1beta1.metrics.k8s.io
NAME                     SERVICE                      AVAILABLE   AGE
v1beta1.metrics.k8s.io   kube-system/metrics-server   True        30d

# 2. 测试指标获取
kubectl top nodes
kubectl top pods -n production

# 3. 检查 Pod resources 定义
kubectl get deployment web-app -o yaml | grep -A 10 resources

# 4. 查看 HPA 状态
kubectl get hpa web-app -o yaml | grep -A 20 conditions
```
**解决方案**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 方案 1: 修复 Metrics Server
kubectl edit deployment metrics-server -n kube-system
# 添加 --kubelet-insecure-tls 参数

# 方案 2: 添加 resources.requests
kubectl set resources deployment web-app --requests=cpu=100m,memory=128Mi

# 方案 3: 重启 Metrics Server
kubectl rollout restart deployment metrics-server -n kube-system
```
---

## 5. FailedComputeMetricsReplicas

**基本信息**
```yaml
事件名称: FailedComputeMetricsReplicas
类型: Warning
引入版本: v1.6+
组件: horizontal-pod-autoscaler
频率: 低频 (计算错误)
```

**事件格式**
```
Type: Warning
Reason: FailedComputeMetricsReplicas
Message: failed to compute desired number of replicas based on listed metrics for Deployment/web-app: invalid metrics (1 invalid out of 2); first error is: failed to get cpu utilization: missing request for cpu
Age: 1m
From: horizontal-pod-autoscaler
```

**触发条件**
- 多个指标中部分失败
- 指标数据不完整或无效
- 计算过程出现错误

**常见场景**
```yaml
# 场景: 多指标 HPA，部分指标失败
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app
spec:
  metrics:
  - type: Resource
    resource:
      name: cpu  # 成功
      target:
        type: Utilization
        averageUtilization: 70
  - type: Pods
    pods:
      metric:
        name: http_requests  # 失败 - 指标不存在
      target:
        type: AverageValue
        averageValue: "1000"
```

**排查建议**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查每个指标源
kubectl get --raw /apis/metrics.k8s.io/v1beta1/namespaces/production/pods
kubectl get --raw /apis/custom.metrics.k8s.io/v1beta1/namespaces/production/pods/*/http_requests
```
---

## 6-9. 指标获取失败事件

**FailedGetExternalMetric (v1.10+)**
```
Type: Warning
Reason: FailedGetExternalMetric
Message: failed to get external metric cloudwatch-sqs-queue-depth: unable to fetch metrics from external metrics API
```

**FailedGetObjectMetric (v1.6+)**
```
Type: Warning
Reason: FailedGetObjectMetric
Message: failed to get object metric: unable to get metric ingress_requests_per_second for Ingress/web-ingress
```

**FailedGetPodsMetric (v1.6+)**
```
Type: Warning
Reason: FailedGetPodsMetric
Message: failed to get pods metric: unable to get metric http_requests for selector app=web
```

**排查重点**
- 检查 Custom Metrics API / External Metrics API 是否部署
- 验证 [[Prometheus|Prometheus]] Adapter / Datadog Cluster Agent 等适配器配置
- 确认指标名称和选择器正确

---

## 10-13. 配置错误事件

**InvalidMetricSourceType (v1.6+)**
```
Type: Warning
Reason: InvalidMetricSourceType
Message: invalid metric source type: Object
```

**InvalidSelector (v1.6+)**
```
Type: Warning
Reason: InvalidSelector
Message: invalid selector: unable to parse selector
```

**FailedUpdateStatus (v1.1+)**
```
Type: Warning
Reason: FailedUpdateStatus
Message: failed to update status: the server could not find the requested resource
```

**FailedGetScale (v1.1+)**
```
Type: Warning
Reason: FailedGetScale
Message: failed to get scale subresource: deployments.apps "web-app" not found
```

---

## 14. AbleToScale

**基本信息**
```yaml
事件名称: AbleToScale
类型: Normal
引入版本: v1.23+
组件: horizontal-pod-autoscaler
频率: 高频 (每次准备扩缩容)
```

**事件格式**
```
Type: Normal
Reason: AbleToScale
Message: the HPA controller was able to get the target's current scale
Age: 30s
From: horizontal-pod-autoscaler
```

**触发条件**
- HPA 成功获取目标资源的 scale 子资源
- 准备执行扩缩容评估

**示例**
```bash
Events:
  Normal  AbleToScale              1m   the HPA controller was able to get the target's current scale
  Normal  DesiredReplicasComputed  1m   Computed desired replicas: 8
  Normal  SuccessfulRescale        59s  New size: 8
```

---

## 15. ReadyForNewScale

**基本信息**
```yaml
事件名称: ReadyForNewScale
类型: Normal
引入版本: v1.23+
组件: horizontal-pod-autoscaler
频率: 中频 (冷却期后)
```

**事件格式**
```
Type: Normal
Reason: ReadyForNewScale
Message: recommended size matches current size
Age: 2m
From: horizontal-pod-autoscaler
```

**触发条件**
- 上次扩缩容的冷却期已过
- HPA 可以执行新的扩缩容操作

**冷却期说明**
```yaml
# v1.23+ behavior 配置
spec:
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容冷却期 5 分钟
    scaleUp:
      stabilizationWindowSeconds: 0    # 扩容无冷却期
```

---

## 16. ScaleDownStabilized

**基本信息**
```yaml
事件名称: ScaleDownStabilized
类型: Normal
引入版本: v1.17+
组件: horizontal-pod-autoscaler
频率: 中频 (缩容稳定期)
```

**事件格式**
```
Type: Normal
Reason: ScaleDownStabilized
Message: recent recommendations were higher than current one, skipping the scale down
Age: 1m
From: horizontal-pod-autoscaler
```

**触发条件**
- 计算出的副本数小于当前值（需要缩容）
- 但在稳定窗口期内有更高的建议值
- 为避免频繁波动，跳过本次缩容

**稳定窗口机制**
```yaml
spec:
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 5 分钟窗口
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
```

**示例场景**
```
时间线:
10:00 - 计算: 需要 10 副本
10:01 - 计算: 需要 8 副本
10:02 - 计算: 需要 6 副本 (但 5 分钟内最高是 10，跳过缩容)
10:03 - 计算: 需要 7 副本
10:05 - 计算: 需要 6 副本 (稳定窗口内最高是 8，跳过缩容)
10:06 - 计算: 需要 5 副本 (稳定窗口内最高是 7，执行缩容到 7)
```

---

<!-- chunk: VPA 事件详解 -->## VPA 事件详解

## 17. EvictedByVPA

**基本信息**
```yaml
事件名称: EvictedByVPA
类型: Normal
引入版本: VPA addon
组件: vpa-updater
频率: 低频 (VPA 驱逐 Pod)
```

**事件格式**
```
Type: Normal
Reason: EvictedByVPA
Message: Pod evicted by VPA Updater to apply resource recommendation
Age: 30s
From: vpa-updater
```

**触发条件**
- VPA 模式为 `Auto` 或 `Recreate`
- Pod 实际资源与推荐值差异超过阈值
- VPA 触发驱逐以应用新的资源请求

**VPA 配置示例**
```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  updatePolicy:
    updateMode: Auto  # 或 Recreate
  resourcePolicy:
    containerPolicies:
    - containerName: '*'
      minAllowed:
        cpu: 100m
        memory: 128Mi
      maxAllowed:
        cpu: 2
        memory: 2Gi
```

**排查建议**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VPA 推荐值
kubectl describe vpa web-app-vpa

Recommendation:
  Container Recommendations:
    Container Name:  app
    Lower Bound:
      Cpu:     150m
      Memory:  262144k
    Target:
      Cpu:     200m  # 推荐值
      Memory:  300Mi
    Upper Bound:
      Cpu:     500m
      Memory:  500Mi
```
---

## 18. RecommendationProvided

**基本信息**
```yaml
事件名称: RecommendationProvided
类型: Normal
引入版本: VPA addon
组件: vpa-recommender
频率: 中频 (定期更新)
```

**事件格式**
```
Type: Normal
Reason: RecommendationProvided
Message: VPA recommender provided new resource recommendation: cpu=200m, memory=300Mi
Age: 5m
From: vpa-recommender
```

**触发条件**
- VPA Recommender 完成分析
- 生成新的资源推荐值
- 所有 updateMode 都会产生此事件

---

## 19. UpdateFailed

**基本信息**
```yaml
事件名称: UpdateFailed
类型: Warning
引入版本: VPA addon
组件: vpa-updater
频率: 低频 (更新失败)
```

**事件格式**
```
Type: Warning
Reason: UpdateFailed
Message: failed to evict pod: PodDisruptionBudget violation
Age: 2m
From: vpa-updater
```

**常见原因**
- PodDisruptionBudget 限制驱逐
- Pod 标记为不可驱逐 (`cluster-autoscaler.kubernetes.io/safe-to-evict: "false"`)
- 资源配额不足

---

## 20. CheckpointUpdated

**基本信息**
```yaml
事件名称: CheckpointUpdated
类型: Normal
引入版本: VPA addon
组件: vpa-recommender
频率: 低频 (定期检查点)
```

**事件格式**
```
Type: Normal
Reason: CheckpointUpdated
Message: VPA checkpoint updated with new resource usage data
Age: 10m
From: vpa-recommender
```

**说明**
- VPA Recommender 定期保存历史数据到 checkpoint
- 用于重启后恢复推荐状态

---

<!-- chunk: Cluster Autoscaler 事件详解 -->## Cluster Autoscaler 事件详解

## 21. ScaledUpGroup

**基本信息**
```yaml
事件名称: ScaledUpGroup
类型: Normal
引入版本: CA addon
组件: cluster-autoscaler
频率: 中频 (节点扩容)
```

**事件格式**
```
Type: Normal
Reason: ScaledUpGroup
Message: Scale-up: group node-group-1 size increased from 3 to 5
Age: 2m
From: cluster-autoscaler
```

**触发条件**
- 有 Pod 因资源不足处于 Pending 状态
- CA 评估可以通过增加节点调度这些 Pod
- 成功向云供应商请求增加节点

**示例场景**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. Pod Pending
kubectl get pods

NAME                READY   STATUS    RESTARTS   AGE
web-app-1           1/1     Running   0          10m
web-app-2           0/1     Pending   0          1m  # 资源不足

# 2. 查看 Pending 原因
kubectl describe pod web-app-2

Events:
  Warning  FailedScheduling  1m  0/3 nodes are available: 3 Insufficient cpu.

# 3. CA 触发扩容
kubectl get events --field-selector involvedObject.kind=Node

Type    Reason          Message
Normal  ScaledUpGroup   Scale-up: group node-group-1 size increased from 3 to 5

# 4. 新节点加入
kubectl get nodes

NAME      STATUS   ROLES    AGE
node-1    Ready    worker   10d
node-2    Ready    worker   10d
node-3    Ready    worker   10d
node-4    Ready    worker   2m   # 新节点
node-5    Ready    worker   2m   # 新节点
```
**配置参数**
```yaml
# CA Deployment 配置
spec:
  containers:
  - command:
    - ./cluster-autoscaler
    - --cloud-provider=aws
    - --nodes=1:10:node-group-1  # min:max:name
    - --scale-down-enabled=true
    - --scale-down-delay-after-add=10m
    - --scale-down-unneeded-time=10m
```

---

## 22. ScaleDown

**基本信息**
```yaml
事件名称: ScaleDown
类型: Normal
引入版本: CA addon
组件: cluster-autoscaler
频率: 中频 (节点缩容)
```

**事件格式**
```
Type: Normal
Reason: ScaleDown
Message: Scale-down: node node-4 removed from group node-group-1
Age: 5m
From: cluster-autoscaler
```

**触发条件**
- 节点上 Pod 总资源请求低于阈值（默认 50%）
- 节点上的 Pod 可以调度到其他节点
- 满足缩容等待时间（默认 10 分钟）

**缩容决策条件**
```
节点可以缩容的条件（所有条件必须满足）:

1. 节点利用率低于阈值 (--scale-down-utilization-threshold=0.5)
2. 节点空闲时间超过阈值 (--scale-down-unneeded-time=10m)
3. 节点上所有 Pod 满足以下之一:
   - 可以被驱逐 (无 PDB 限制)
   - 可以调度到其他节点
   - 是 DaemonSet Pod
   - 有 local storage 但可以容忍数据丢失
4. 节点没有缩容保护注解:
   - cluster-autoscaler.kubernetes.io/scale-down-disabled: "true"
5. 节点上没有系统 Pod (除 DaemonSet 和 kube-system)
```

---

## 23. ScaleDownEmpty

**基本信息**
```yaml
事件名称: ScaleDownEmpty
类型: Normal
引入版本: CA addon
组件: cluster-autoscaler
频率: 中频 (空节点缩容)
```

**事件格式**
```
Type: Normal
Reason: ScaleDownEmpty
Message: Scale-down: empty node node-5 removed
Age: 1m
From: cluster-autoscaler
```

**触发条件**
- 节点上没有任何 Pod（除 DaemonSet）
- 满足空节点缩容等待时间（默认 10 分钟）

**配置参数**
```bash
--scale-down-unneeded-time=10m          # 普通节点缩容等待时间
--scale-down-unready-time=20m           # 未就绪节点缩容等待时间
--scale-down-delay-after-add=10m        # 扩容后延迟缩容时间
--scale-down-delay-after-delete=0s      # 删除节点后延迟时间
--scale-down-delay-after-failure=3m     # 缩容失败后延迟时间
```

---

## 24. ScaleDownFailed

**基本信息**
```yaml
事件名称: ScaleDownFailed
类型: Warning
引入版本: CA addon
组件: cluster-autoscaler
频率: 低频 (缩容失败)
```

**事件格式**
```
Type: Warning
Reason: ScaleDownFailed
Message: Scale-down: failed to delete node node-4: failed to terminate instance i-abc123
Age: 2m
From: cluster-autoscaler
```

**常见原因**

**原因 1: 云供应商 API 失败**
```
Message: failed to delete node: RequestLimitExceeded: Request limit exceeded
```

**原因 2: Pod 驱逐失败**
```
Message: failed to evict pod web-app-1: PodDisruptionBudget violation
```

**原因 3: 节点有保护注解**
```yaml
apiVersion: v1
kind: Node
metadata:
  annotations:
    cluster-autoscaler.kubernetes.io/scale-down-disabled: "true"
```

---

## 25. NotTriggerScaleUp

**基本信息**
```yaml
事件名称: NotTriggerScaleUp
类型: Warning
引入版本: CA addon
组件: cluster-autoscaler
频率: 低频 (扩容条件不满足)
```

**事件格式**
```
Type: Warning
Reason: NotTriggerScaleUp
Message: pod didn't trigger scale-up: 2 max node group size reached
Age: 30s
From: cluster-autoscaler
```

**常见原因**

**原因 1: 节点组达到最大值**
```bash
--nodes=1:10:node-group-1  # 当前已有 10 个节点
```

**原因 2: Pod 资源请求超过节点规格**
```yaml
# Pod 请求
resources:
  requests:
    cpu: 32    # 超过节点规格
    memory: 128Gi

# 节点规格: 16 CPU, 64Gi 内存
```

**原因 3: 节点选择器/亲和性无法满足**
```yaml
nodeSelector:
  gpu: "true"  # 但节点组没有 GPU 标签
```

---

## 26. TriggeredScaleUp

**基本信息**
```yaml
事件名称: TriggeredScaleUp
类型: Normal
引入版本: CA addon
组件: cluster-autoscaler
频率: 中频 (触发扩容)
```

**事件格式**
```
Type: Normal
Reason: TriggeredScaleUp
Message: pod triggered scale-up: [{node-group-1 3->5 (max: 10)}]
Age: 1m
From: cluster-autoscaler
```

**触发条件**
- Pod 处于 Pending 状态
- 调度器无法在现有节点上调度
- CA 评估增加节点可以调度该 Pod

**示例**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Pod 事件
kubectl describe pod web-app-pending

Events:
  Type     Reason            Age   Message
  ----     ------            ----  -------
  Warning  FailedScheduling  2m    0/3 nodes available: 3 Insufficient cpu
  Normal   TriggeredScaleUp  1m    pod triggered scale-up

# 节点事件
kubectl get events --field-selector reason=ScaledUpGroup

Type    Reason         Message
Normal  ScaledUpGroup  Scale-up: group node-group-1 size increased from 3 to 5
```
---

## 27. ScaleDownDisabledAnnotation

**基本信息**
```yaml
事件名称: ScaleDownDisabledAnnotation
类型: Normal
引入版本: CA addon
组件: cluster-autoscaler
频率: 低频 (注解保护)
```

**事件格式**
```
Type: Normal
Reason: ScaleDownDisabledAnnotation
Message: scale-down disabled by annotation on node node-4
Age: 5m
From: cluster-autoscaler
```

**触发条件**
- 节点有缩容保护注解
- CA 跳过该节点的缩容评估

**保护注解**
```yaml
# 节点级别保护
apiVersion: v1
kind: Node
metadata:
  annotations:
    cluster-autoscaler.kubernetes.io/scale-down-disabled: "true"

# Pod 级别保护（防止节点被缩容）
apiVersion: v1
kind: Pod
metadata:
  annotations:
    cluster-autoscaler.kubernetes.io/safe-to-evict: "false"
```

---

## 28. FailedToScaleUpGroup

**基本信息**
```yaml
事件名称: FailedToScaleUpGroup
类型: Warning
引入版本: CA addon
组件: cluster-autoscaler
频率: 低频 (扩容失败)
```

**事件格式**
```
Type: Warning
Reason: FailedToScaleUpGroup
Message: failed to increase node group size: rate limit exceeded
Age: 1m
From: cluster-autoscaler
```

**常见原因**
- 云供应商 API 限流
- 资源配额不足（vCPU、IP 地址等）
- 节点组配置错误
- 网络、安全组问题

**排查步骤**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 CA 日志
kubectl logs -n kube-system deployment/cluster-autoscaler

# 2. 检查云供应商配额
aws ec2 describe-account-attributes --attribute-names max-instances

# 3. 验证节点组配置
aws autoscaling describe-auto-scaling-groups --auto-scaling-group-names node-group-1
```
---

<!-- chunk: HPA 决策算法 -->## HPA 决策算法

## 基本计算公式

```
desiredReplicas = ceil[currentReplicas * (currentMetricValue / targetMetricValue)]

其中:
- currentReplicas: 当前副本数
- currentMetricValue: 当前指标值（所有 Pod 平均值）
- targetMetricValue: 目标指标值
- ceil: 向上取整
```

## 详细计算流程

**步骤 1: 获取 Pod 指标**
```go
// 获取所有 Ready 状态的 Pod 指标
readyPods := getReadyPods(deployment)
currentMetricValue := sum(metrics) / len(readyPods)
```

**步骤 2: 计算期望副本数**
```go
if currentMetricValue > targetMetricValue {
    // 扩容
    desiredReplicas = ceil(currentReplicas * (currentMetricValue / targetMetricValue))
} else if currentMetricValue < targetMetricValue {
    // 缩容
    desiredReplicas = floor(currentReplicas * (currentMetricValue / targetMetricValue))
}
```

**步骤 3: 应用边界限制**
```go
if desiredReplicas < minReplicas {
    desiredReplicas = minReplicas
}
if desiredReplicas > maxReplicas {
    desiredReplicas = maxReplicas
}
```

**步骤 4: 应用容忍度（Tolerance）**
```go
tolerance := 0.1  // 默认 10%
if abs((currentMetricValue - targetMetricValue) / targetMetricValue) < tolerance {
    // 在容忍范围内，不执行扩缩容
    desiredReplicas = currentReplicas
}
```

## 实际示例

**示例 1: CPU 使用率扩容**
```
当前状态:
- currentReplicas: 5
- CPU requests: 100m per pod
- CPU usage: 平均 85m per pod
- targetUtilization: 70%

计算过程:
currentMetricValue = (85m / 100m) * 100% = 85%
targetMetricValue = 70%

desiredReplicas = ceil[5 * (85 / 70)]
                = ceil[5 * 1.214]
                = ceil[6.07]
                = 7

结果: 扩容到 7 个副本
```

**示例 2: 自定义指标扩容**
```
当前状态:
- currentReplicas: 3
- 指标: http_requests_per_second
- 当前值: 1500 (每个 Pod 500 请求/秒)
- 目标值: 300 请求/秒 per pod

计算过程:
currentMetricValue = 1500 / 3 = 500
targetMetricValue = 300

desiredReplicas = ceil[3 * (500 / 300)]
                = ceil[3 * 1.667]
                = ceil[5]
                = 5

结果: 扩容到 5 个副本
```

## 多指标决策

**并行计算所有指标，取最大值**
```yaml
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
```

**计算逻辑**
```go
// 计算每个指标的期望副本数
cpuDesiredReplicas := calculateReplicas(cpuMetric)      // 例如: 7
memoryDesiredReplicas := calculateReplicas(memoryMetric) // 例如: 5

// 取最大值（保守策略，确保满足所有指标）
finalDesiredReplicas := max(cpuDesiredReplicas, memoryDesiredReplicas)
// 结果: 7
```

## 特殊场景处理

**场景 1: Pod 启动中（未就绪）**
```go
// 忽略未就绪的 Pod
readyPods := filterReady(allPods)
currentMetricValue := sum(metricsOfReadyPods) / len(readyPods)
```

**场景 2: 缺失指标的 Pod**
```go
// 如果 Pod 指标缺失（如刚启动）
if missingMetrics > 0 {
    // 假设缺失指标的 Pod 使用目标值
    estimatedValue := (sumExistingMetrics + missingMetrics * targetValue) / totalPods
}
```

**场景 3: 容忍度阈值**
```yaml
# 默认容忍度 10%
# 如果 currentValue 在 [targetValue * 0.9, targetValue * 1.1] 范围内
# 不触发扩缩容

示例:
targetValue = 70%
容忍范围 = [63%, 77%]
如果 currentValue = 72%，不扩缩容
```

---

<!-- chunk: VPA 工作模式 -->## VPA 工作模式

## 四种模式对比

| 模式 | 行为 | 适用场景 | 风险 |
|------|------|---------|------|
| **Off** | 仅提供建议，不更新 Pod | 观察测试阶段 | 无 |
| **Initial** | 仅在 Pod 创建时应用建议 | 新部署的应用 | 低 |
| **Auto** | 自动驱逐并重建 Pod | 无状态应用 | 中 |
| **Recreate** | 手动重启时应用建议 | 有状态应用、需要控制重启时间 | 低 |

## 模式详解

**1. Off 模式 - 仅观察**
```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  updatePolicy:
    updateMode: "Off"  # 仅提供建议
```

**行为**
- VPA Recommender 持续分析资源使用情况
- 生成推荐值并记录到 VPA status
- **不会**修改任何 Pod 的 resources
- **不会**驱逐或重启 Pod

**查看建议**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe vpa web-app-vpa

Status:
  Recommendation:
    Container Recommendations:
      Container Name:  app
      Lower Bound:
        Cpu:     100m
        Memory:  128Mi
      Target:      # 推荐应用的值
        Cpu:     200m
        Memory:  256Mi
      Upper Bound:
        Cpu:     500m
        Memory:  512Mi
```
**适用场景**
- 初次部署 VPA，评估推荐值是否合理
- 生产环境观察期
- 不想自动更改资源的应用

---

**2. Initial 模式 - 创建时应用**
```yaml
updatePolicy:
  updateMode: "Initial"
```

**行为**
- 对**新创建**的 Pod，应用 VPA 推荐的 resources
- 对**已存在**的 Pod，不做任何更改
- 不会驱逐现有 Pod

**示例**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 初始 Deployment
apiVersion: apps/v1
kind: Deployment
spec:
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: 100m      # 初始配置
            memory: 128Mi

# 创建 VPA（Initial 模式）后
# 现有 Pod: 保持 100m CPU, 128Mi 内存
# 新建 Pod: 使用 VPA 推荐值（如 200m CPU, 256Mi 内存）

# 扩容后的新 Pod
kubectl get pod web-app-new -o yaml | grep -A 5 resources

resources:
  requests:
    cpu: 200m      # VPA 推荐值
    memory: 256Mi
```
**适用场景**
- 新部署的应用，想逐步应用 VPA 推荐
- 避免现有 Pod 被驱逐，但希望新 Pod 使用优化配置
- 滚动更新时自动应用新配置

---

**3. Auto 模式 - 自动更新**
```yaml
updatePolicy:
  updateMode: "Auto"
```

**行为**
- VPA Updater 主动驱逐 Pod（调用 Eviction API）
- Deployment/ReplicaSet 重建 Pod
- 新 Pod 使用 VPA 推荐的 resources

**驱逐条件**
```go
// VPA 决定驱逐的条件（任一满足）:
1. Pod 当前 CPU request 与推荐值相差 > 10%
2. Pod 当前 memory request 与推荐值相差 > 10%
3. Pod 资源使用超出 limits（风险优化）
```

**事件流**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. VPA 生成新推荐值
Events:
  Normal  RecommendationProvided  5m  new recommendation: cpu=400m, memory=512Mi

# 2. VPA Updater 驱逐 Pod
Events:
  Normal  EvictedByVPA  4m  Pod evicted to apply resource recommendation

# 3. Deployment 重建 Pod
Events:
  Normal  SuccessfulCreate  3m  Created pod: web-app-new

# 4. 新 Pod 使用推荐值
kubectl get pod web-app-new -o yaml | grep -A 5 resources

resources:
  requests:
    cpu: 400m      # VPA 更新后的值
    memory: 512Mi
```
**限制和保护**
```yaml
# 1. PodDisruptionBudget 保护
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-app-pdb
spec:
  minAvailable: 2  # VPA 会遵守 PDB，确保最少 2 个 Pod 可用

# 2. resourcePolicy 限制
spec:
  resourcePolicy:
    containerPolicies:
    - containerName: app
      minAllowed:
        cpu: 100m
        memory: 128Mi
      maxAllowed:
        cpu: 2          # VPA 不会推荐超过 2 CPU
        memory: 4Gi
      controlledResources:
      - cpu
      - memory
```

**适用场景**
- 无状态应用（如 Web 服务）
- 可以容忍 Pod 重启
- 希望自动优化资源使用

**风险**
- Pod 重启导致服务中断（需配合 PDB）
- 频繁驱逐可能影响稳定性

---

**4. Recreate 模式 - 手动控制更新**
```yaml
updatePolicy:
  updateMode: "Recreate"
```

**行为**
- VPA 更新 Deployment/StatefulSet 的 resources 定义
- **不会**自动驱逐 Pod
- 需要手动触发 Pod 重建（如滚动更新）

**工作流程**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. VPA 更新 Deployment spec
kubectl get deployment web-app -o yaml | grep -A 5 resources

spec:
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: 300m      # VPA 更新的值
            memory: 384Mi

# 2. 但 Pod 不会自动重建
kubectl get pods

NAME         STATUS    RESTARTS   AGE
web-app-1    Running   0          10d  # 仍使用旧配置

# 3. 手动触发滚动更新
kubectl rollout restart deployment web-app

# 4. 新 Pod 使用 VPA 推荐值
kubectl get pods

NAME         STATUS    RESTARTS   AGE
web-app-new  Running   0          1m   # 使用新配置
```
**适用场景**
- 有状态应用（[[StatefulSet|StatefulSet]]）
- 需要控制重启时间窗口
- 生产环境需要审批流程
- 关键服务不能随意重启

**优势**
- 完全控制何时应用资源更改
- 避免意外重启
- 可以配合维护窗口

---

## VPA 与 HPA 共存

**推荐配置**
```yaml
# VPA: 仅管理 memory
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  updatePolicy:
    updateMode: "Auto"
  resourcePolicy:
    containerPolicies:
    - containerName: app
      controlledResources:
      - memory  # 仅管理内存

---
# HPA: 仅管理 cpu
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  metrics:
  - type: Resource
    resource:
      name: cpu  # 仅基于 CPU 扩缩容
      target:
        type: Utilization
        averageUtilization: 70
```

**避免冲突的原则**
1. VPA 管理 memory，HPA 基于 CPU 扩缩容
2. 不要让 VPA 和 HPA 同时管理相同的资源指标
3. 优先使用 HPA（横向扩展），VPA 作为补充（纵向优化）

---

<!-- chunk: CA 扩缩容决策逻辑 -->## CA 扩缩容决策逻辑

## 扩容决策流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────┐
│ 1. 检测 Pending Pods                     │
│    kubectl get pods --field-selector=   │
│    status.phase=Pending                  │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 2. 分析 Pending 原因                     │
│    - Insufficient cpu/memory/gpu         │
│    - Node affinity/selector 不匹配       │
│    - Taints/Tolerations 不匹配           │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 3. 模拟调度到新节点                      │
│    - 为每个节点组模拟添加节点            │
│    - 运行调度器算法检查 Pod 是否可调度   │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 4. 选择最佳节点组                        │
│    - 优先级: 资源匹配度                  │
│    - 成本（如配置了 Expander）            │
│    - 节点组当前大小                       │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 5. 检查约束条件                          │
│    - 节点组未达到 max 限制               │
│    - 云资源配额充足                      │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 6. 触发扩容                              │
│    - 调用云供应商 API 增加节点           │
│    - 生成 TriggeredScaleUp 事件          │
└─────────────────────────────────────────┘
```
## 缩容决策流程

```
┌─────────────────────────────────────────┐
│ 1. 评估所有节点利用率                    │
│    utilization = (requests / capacity)   │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 2. 识别低利用率节点                      │
│    - utilization < threshold (默认 50%)  │
│    - 持续时间 > unneeded-time (默认 10m) │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 3. 检查节点上的 Pods                     │
│    对于每个 Pod，检查:                   │
│    - 是否可以被驱逐 (PDB)                │
│    - 是否可以调度到其他节点              │
│    - 是否有 local storage                │
│    - 是否有 scale-down-disabled 注解     │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 4. 模拟驱逐和重新调度                    │
│    - 将节点上的 Pod 模拟调度到其他节点   │
│    - 检查资源是否充足                    │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 5. 检查缩容保护                          │
│    - 节点是否有保护注解                  │
│    - 是否在延迟保护期内                  │
│    - 节点组是否达到 min 限制             │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ 6. 执行缩容                              │
│    - 驱逐节点上的 Pods                   │
│    - 调用云供应商 API 删除节点           │
│    - 生成 ScaleDown 事件                 │
└─────────────────────────────────────────┘
```

## 关键参数详解

**扩容参数**
```bash
# 节点组配置
--nodes=<min>:<max>:<node-group-name>
# 示例: --nodes=1:10:node-group-1
#       最少 1 个节点，最多 10 个节点

# 扩容延迟
--scale-up-from-zero=true          # 允许从 0 扩容
--max-nodes-total=100              # 集群节点总数上限
--max-cores-total=320              # 集群总 CPU 核数上限
--max-memory-total=1280            # 集群总内存上限 (GiB)

# 扩容后延迟缩容（避免抖动）
--scale-down-delay-after-add=10m
```

**缩容参数**
```bash
# 缩容开关
--scale-down-enabled=true

# 缩容阈值
--scale-down-utilization-threshold=0.5  # 节点利用率低于 50% 才考虑缩容

# 缩容等待时间
--scale-down-unneeded-time=10m          # 节点低利用率持续 10 分钟
--scale-down-unready-time=20m           # 未就绪节点持续 20 分钟

# 缩容延迟（避免频繁操作）
--scale-down-delay-after-add=10m        # 扩容后 10 分钟内不缩容
--scale-down-delay-after-delete=0s      # 删除节点后立即评估其他节点
--scale-down-delay-after-failure=3m     # 缩容失败后 3 分钟内不重试
```

## Expander 策略

**选择节点组的策略（当多个节点组都满足条件时）**

**1. random (默认)**
```bash
--expander=random
# 随机选择一个节点组
```

**2. most-pods**
```bash
--expander=most-pods
# 选择能调度最多 Pending Pods 的节点组
```

**3. least-waste**
```bash
--expander=least-waste
# 选择添加节点后资源浪费最少的节点组
# 计算公式: waste = (node_capacity - pod_requests) / node_capacity
```

**4. price**
```bash
--expander=price
# 选择成本最低的节点组（需要云供应商支持）
```

**5. priority**
```bash
--expander=priority
# 根据配置的优先级 ConfigMap 选择节点组

# ConfigMap 示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-autoscaler-priority-expander
  namespace: kube-system
data:
  priorities: |-
    10:
      - .*-spot-.*   # 优先选择 Spot 实例节点组
    50:
      - .*-gpu-.*    # 其次选择 GPU 节点组
    100:
      - .*           # 最后选择其他节点组
```

---

<!-- chunk: Behavior 行为配置 -->## Behavior 行为配置

## v1.18+ Behavior 字段

**引入版本**: Kubernetes v1.18+  
**稳定版本**: v1.23+

**作用**: 精细控制 HPA 的扩缩容行为，避免频繁波动。

## 完整配置示例

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app-advanced
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 2
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 0  # 扩容无稳定窗口
      policies:
      - type: Percent
        value: 100       # 每次最多增加 100% (翻倍)
        periodSeconds: 15
      - type: Pods
        value: 4         # 每次最多增加 4 个 Pod
        periodSeconds: 15
      selectPolicy: Max  # 取两个策略的最大值
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容稳定窗口 5 分钟
      policies:
      - type: Percent
        value: 50        # 每分钟最多减少 50%
        periodSeconds: 60
      - type: Pods
        value: 2         # 每分钟最多减少 2 个 Pod
        periodSeconds: 60
      selectPolicy: Min  # 取两个策略的最小值（保守缩容）
```

## 字段详解

**1. stabilizationWindowSeconds**

**作用**: 稳定窗口期，避免指标抖动导致频繁扩缩容。

**扩容窗口**
```yaml
scaleUp:
  stabilizationWindowSeconds: 0  # 默认 0，立即扩容
```
- 设为 0: 一旦指标超过阈值，立即扩容
- 设为 N: 在过去 N 秒内，取推荐副本数的最大值

**缩容窗口**
```yaml
scaleDown:
  stabilizationWindowSeconds: 300  # 默认 300 秒（5 分钟）
```
- 在过去 300 秒内，取推荐副本数的最小值
- 避免流量短暂下降就立即缩容

**示例**
```
时间线 (缩容场景):
10:00 - 推荐副本数: 10
10:01 - 推荐副本数: 8
10:02 - 推荐副本数: 6  # 流量短暂下降
10:03 - 推荐副本数: 9  # 流量恢复
10:04 - 推荐副本数: 8
10:05 - 推荐副本数: 8

实际缩容决策（stabilizationWindowSeconds=300）:
10:05 - 取过去 5 分钟最小值 = 6
      - 但由于窗口内有更高值（10），不会立即缩容到 6
      - 保守缩容，避免频繁波动
```

---

**2. policies**

**类型**: Percent 或 Pods

**Percent 类型**
```yaml
policies:
- type: Percent
  value: 50
  periodSeconds: 60
```
- 含义: 每 60 秒，最多增加/减少 50% 的副本数
- 计算: maxChange = ceil(currentReplicas * 0.5)

**示例**
```
当前副本数: 10
Percent=50, periodSeconds=60

最大变化量 = ceil(10 * 0.5) = 5
因此，每分钟最多增加/减少 5 个副本
```

**Pods 类型**
```yaml
policies:
- type: Pods
  value: 4
  periodSeconds: 60
```
- 含义: 每 60 秒，最多增加/减少 4 个 Pod

---

**3. selectPolicy**

**可选值**: Max, Min, Disabled

**Max** (扩容常用)
```yaml
scaleUp:
  policies:
  - type: Percent
    value: 100       # 策略 1: 最多翻倍
    periodSeconds: 15
  - type: Pods
    value: 4         # 策略 2: 最多增加 4 个
    periodSeconds: 15
  selectPolicy: Max  # 取最大值
```

**示例**
```
当前副本数: 10

策略 1: 10 * 100% = 10 (可增加 10 个，变为 20)
策略 2: 4 (可增加 4 个，变为 14)

selectPolicy=Max: 取 max(10, 4) = 10
结果: 扩容到 20 个副本
```

**Min** (缩容常用)
```yaml
scaleDown:
  policies:
  - type: Percent
    value: 50        # 策略 1: 最多减少 50%
    periodSeconds: 60
  - type: Pods
    value: 2         # 策略 2: 最多减少 2 个
    periodSeconds: 60
  selectPolicy: Min  # 取最小值（保守缩容）
```

**示例**
```
当前副本数: 10

策略 1: 10 * 50% = 5 (可减少 5 个，变为 5)
策略 2: 2 (可减少 2 个，变为 8)

selectPolicy=Min: 取 min(5, 2) = 2
结果: 缩容到 8 个副本（更保守）
```

---

## 常见配置场景

**场景 1: 快速扩容，慢速缩容**
```yaml
behavior:
  scaleUp:
    stabilizationWindowSeconds: 0
    policies:
    - type: Percent
      value: 100
      periodSeconds: 15  # 每 15 秒可翻倍
    selectPolicy: Max
  scaleDown:
    stabilizationWindowSeconds: 300
    policies:
    - type: Pods
      value: 1           # 每分钟仅减少 1 个
      periodSeconds: 60
    selectPolicy: Min
```

**场景 2: 禁止缩容（仅扩容）**
```yaml
behavior:
  scaleDown:
    selectPolicy: Disabled  # 完全禁止缩容
```

**场景 3: 平滑扩缩容**
```yaml
behavior:
  scaleUp:
    stabilizationWindowSeconds: 60  # 扩容也有稳定期
    policies:
    - type: Pods
      value: 2
      periodSeconds: 60
  scaleDown:
    stabilizationWindowSeconds: 300
    policies:
    - type: Pods
      value: 1
      periodSeconds: 60
```

---

<!-- chunk: 故障排查场景 -->## 故障排查场景

## 场景 1: HPA 无法获取指标

**症状**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get hpa

NAME      REFERENCE          TARGETS         MINPODS   MAXPODS   REPLICAS   AGE
web-app   Deployment/web-app <unknown>/70%   2         10        2          5m
```
**事件**
```
Type: Warning
Reason: FailedGetResourceMetric
Message: failed to get cpu utilization: unable to get metrics for resource cpu
```

**排查步骤**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Metrics Server
kubectl get apiservice v1beta1.metrics.k8s.io
NAME                     SERVICE                      AVAILABLE
v1beta1.metrics.k8s.io   kube-system/metrics-server   False  # 不可用!

kubectl logs -n kube-system deployment/metrics-server

# 2. 检查 Pod 是否定义 resources.requests
kubectl get deployment web-app -o yaml | grep -A 5 resources

# 3. 测试指标获取
kubectl top pods -n production
```
**解决方案**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装/修复 Metrics Server
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

# 添加 resources.requests
kubectl set resources deployment web-app --requests=cpu=100m,memory=128Mi
```
---

## 场景 2: HPA 频繁扩缩容

**症状**
```bash
# 每分钟都在扩缩容
Events:
  Normal  SuccessfulRescale  5m   New size: 8
  Normal  SuccessfulRescale  4m   New size: 6
  Normal  SuccessfulRescale  3m   New size: 9
  Normal  SuccessfulRescale  2m   New size: 7
```

**原因分析**
- 指标波动大
- 目标值设置不合理
- 缺少稳定窗口和行为配置

**解决方案**
```yaml
# 添加 behavior 配置
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
spec:
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 5 分钟稳定窗口
      policies:
      - type: Pods
        value: 1
        periodSeconds: 60  # 每分钟最多减少 1 个
    scaleUp:
      stabilizationWindowSeconds: 60   # 扩容也添加稳定期
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
```

---

## 场景 3: Cluster Autoscaler 不扩容

**症状**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods

NAME         STATUS    RESTARTS   AGE
web-app-1    Pending   0          10m  # 一直 Pending
```
**事件**
```
Type: Warning
Reason: NotTriggerScaleUp
Message: pod didn't trigger scale-up: 2 max node group size reached
```

**排查步骤**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查节点组配置
kubectl logs -n kube-system deployment/cluster-autoscaler | grep "max node group size"

# 2. 检查 Pod 资源请求
kubectl describe pod web-app-1 | grep -A 5 "Requests"

# 3. 检查节点组状态
aws autoscaling describe-auto-scaling-groups --auto-scaling-group-names node-group-1
```
**解决方案**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 增加节点组最大值
kubectl edit deployment cluster-autoscaler -n kube-system
# 修改 --nodes=1:10:node-group-1 为 --nodes=1:20:node-group-1
```
---

## 场景 4: VPA 驱逐 Pod 失败

**症状**
```
Type: Warning
Reason: UpdateFailed
Message: failed to evict pod: PodDisruptionBudget violation
```

**原因**
- PodDisruptionBudget 限制
- 没有足够的可用副本

**排查步骤**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 PDB
kubectl get pdb

NAME          MIN AVAILABLE   MAX UNAVAILABLE   ALLOWED DISRUPTIONS   AGE
web-app-pdb   5               N/A               0                     10d
```
**解决方案**
```yaml
# 调整 PDB
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-app-pdb
spec:
  minAvailable: 3  # 降低最小可用数
  selector:
    matchLabels:
      app: web-app
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

## HPA 最佳实践

**1. 合理设置目标值**
```yaml
# ❌ 不推荐: 目标值过高
metrics:
- type: Resource
  resource:
    name: cpu
    target:
      averageUtilization: 90  # 容易 OOM、延迟增加

# ✅ 推荐: 留有余量
metrics:
- type: Resource
  resource:
    name: cpu
    target:
      averageUtilization: 70  # 30% 缓冲空间
```

**2. 配置 behavior 避免抖动**
```yaml
behavior:
  scaleUp:
    stabilizationWindowSeconds: 0
    policies:
    - type: Percent
      value: 100
      periodSeconds: 15
  scaleDown:
    stabilizationWindowSeconds: 300
    policies:
    - type: Pods
      value: 1
      periodSeconds: 60
```

**3. 多指标组合**
```yaml
metrics:
- type: Resource
  resource:
    name: cpu
    target:
      averageUtilization: 70
- type: Resource
  resource:
    name: memory
    target:
      averageUtilization: 80
- type: Pods
  pods:
    metric:
      name: http_requests_per_second
    target:
      type: AverageValue
      averageValue: "1000"
```

---

## VPA 最佳实践

**1. 分阶段部署**
```
Phase 1: Off 模式 - 观察推荐值 (1-2 周)
Phase 2: Initial 模式 - 新 Pod 应用推荐 (1 周)
Phase 3: Auto 模式 - 自动更新 (生产环境)
```

**2. 设置合理边界**
```yaml
resourcePolicy:
  containerPolicies:
  - containerName: app
    minAllowed:
      cpu: 100m      # 防止过低
      memory: 128Mi
    maxAllowed:
      cpu: 4         # 防止过高
      memory: 8Gi
```

**3. 配合 PDB 使用**
```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
spec:
  minAvailable: 50%  # 确保 VPA 驱逐时服务可用
```

---

## Cluster Autoscaler 最佳实践

**1. 合理配置节点组**
```bash
# 多个节点组满足不同工作负载
--nodes=1:10:general-purpose-nodes  # 通用工作负载
--nodes=0:5:gpu-nodes               # GPU 工作负载
--nodes=0:20:spot-nodes             # Spot 实例（成本优化）
```

**2. 设置节点保护**
```yaml
# 关键节点保护
apiVersion: v1
kind: Node
metadata:
  annotations:
    cluster-autoscaler.kubernetes.io/scale-down-disabled: "true"

# Pod 级别保护
apiVersion: v1
kind: Pod
metadata:
  annotations:
    cluster-autoscaler.kubernetes.io/safe-to-evict: "false"
```

**3. 优化缩容参数**
```bash
--scale-down-delay-after-add=10m        # 扩容后 10 分钟不缩容
--scale-down-unneeded-time=10m          # 节点空闲 10 分钟才缩容
--scale-down-utilization-threshold=0.5  # 利用率低于 50% 才考虑缩容
```

---

<!-- chunk: 交叉引用 -->## 交叉引用

## 相关文档

| 文档 | 描述 |
|------|------|
| **系统基础/01-pod-lifecycle-events.md** | Pod 生命周期事件，包括 Pending 状态 |
| **系统基础/02-scheduling-events.md** | 调度事件，CA 扩容后的调度过程 |
| **系统基础/05-resource-events.md** | 资源配额事件，HPA/CA 受限场景 |
| **系统基础/11-metrics-monitoring-events.md** | Metrics Server 事件，HPA 指标源 |
| **网络/30-service-mesh-deep-dive.md** | Service Mesh 环境下的自动扩缩容 |
| **故障诊断/topic-structural-trouble-shooting/05-workloads/02-deployment-troubleshooting.md** | Deployment 故障排查 |

## 相关命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# HPA
kubectl get hpa
kubectl describe hpa <name>
kubectl top pods

# VPA
kubectl get vpa
kubectl describe vpa <name>

# Cluster Autoscaler
kubectl logs -n kube-system deployment/cluster-autoscaler
kubectl get nodes
kubectl describe node <name>

# 事件查询
kubectl get events --sort-by='.lastTimestamp' | grep -E 'HorizontalPodAutoscaler|VPA|cluster-autoscaler'
```
---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 12/15

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-33-kubernetes-events MOC
- [[系统基础/README.md|Domain-33: Kubernetes Events 全域事件大全]]
- Domain-33 K8s 事件 — 开源项目索引
- 01 - Kubernetes 事件系统架构与 API 参考
- 02 - Pod 与容器生命周期事件
- 03 - 镜像拉取事件
- 04 - 探针与健康检查事件
- 05 - 调度与抢占事件
- 06 - 节点生命周期与状态事件
- 07 - Deployment 与 ReplicaSet 控制器事件
- 08 - StatefulSet 与 DaemonSet 控制器事件
- 09 - Job 与 CronJob 批处理事件

## See Also

- 10-service-networking-events
- 11-storage-volume-events
- 13-security-admission-rbac-events
- 14-namespace-resource-gc-events

## Related

- [[生态参考/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]
- [[生态参考/topic-index/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
