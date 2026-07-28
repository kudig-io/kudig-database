---
title: Vertical Pod Autoscaling
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- hpa
- vpa
- pdb
- statefulset
- daemonset
- crd
- webhook
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Vertical Pod Autoscaling 是什么
- 如何 Vertical Pod Autoscaling
trigger_keywords:
- Vertical
- Pod
- Autoscaling
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Vertical Pod Autoscaling

## 概述
VerticalPodAutoscaler（VPA）自动调整工作负载（如 Deployment、[[statefulset|StatefulSet]]）中 Pod 的资源请求（requests）和限制（limits），以匹配实际资源使用情况。这种垂直缩放也称为 rightsizing 或 autopilot。

## 核心概念/原理
- **VPA 组成**：
  - **Recommender**：分析 Pod 的历史和实时资源使用，生成推荐值（target、lower bound、upper bound）。
  - **Updater**：监控推荐值与当前 Pod 资源的差异，必要时通过驱逐 Pod 或原地更新来应用新资源。
  - **Admission Controller**：以 mutating webhook 形式拦截 Pod 创建请求，将推荐资源注入到新 Pod 中。
- **指标来源**：VPA 需要 Metrics Server（`metrics.k8s.io`）提供资源使用数据。
- **API 版本**：稳定 API 为 `autoscaling.k8s.io/v1`，以 CRD 形式提供，需单独安装。

## 关键机制或特性
- **更新模式（`updateMode`）**：
  - `Off`：仅生成推荐，不自动应用。
  - `Initial`：仅在 Pod 首次创建时应用推荐，不更新运行中的 Pod。
  - `Recreate`：当推荐与当前资源差异超过阈值时，驱逐 Pod 并由控制器重建以应用新资源。
  - `InPlaceOrRecreate`：优先尝试原地更新资源；若不支持则回退到驱逐重建（需集群支持原地 resize）。
  - `Auto`（已弃用，VPA 1.4.0+）：别名等同于 `Recreate`。
- **资源策略（`resourcePolicy`）**：
  - `minAllowed` / `maxAllowed`：为推荐值设置上下限。
  - `controlledResources`：指定 VPA 管理的资源类型（`cpu`、`memory`）。
  - `controlledValues`：
    - `RequestsAndLimits`（默认）：同时调整 request 和 limit，limit 按原始 request-to-limit 比例缩放。
    - `RequestsOnly`：仅调整 request，保持 limit 不变。
- **LimitRange 兼容**：Admission Controller 和 Updater 会确保推荐值符合命名空间中 LimitRange 的约束。
- **PDB 尊重**：Updater 在驱逐 Pod 时会遵守 PodDisruptionBudget，尽量减少服务影响。

## 使用场景
- 难以准确预估资源需求的应用，希望自动优化资源配置。
- 需要避免资源浪费（过度分配）或应用因资源不足而 OOM/Crash 的场景。
- 与 Cluster Autoscaler 配合，通过更准确的资源请求改善节点利用率。

## 最佳实践/注意事项
- 安装 VPA 前确保 Metrics Server 已正常运行。
- 若对同一工作负载同时使用 HPA 和 VPA，需谨慎配置，避免两者冲突。常见做法是：HPA 基于自定义指标缩放，VPA 仅调整资源请求（`RequestsOnly` 模式）。
- 使用 `Recreate` 模式时，注意 Pod 重建会带来的短暂中断；对中断敏感的服务可评估 `InPlaceOrRecreate` 或 `Initial` 模式。
- 使用 `minAllowed` 和 `maxAllowed` 限制推荐范围，防止极端推荐导致应用异常。
- VPA 不适用于 [[daemonset|DaemonSet]]（通常使用 Cluster Proportional Vertical Autoscaler 替代）。

## 实战 YAML 示例

### 基础 VPA：仅推荐模式（安全起步）

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-api-vpa
  namespace: prod
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-api
  updatePolicy:
    updateMode: "Off"                        # 仅生成推荐，不自动调整
  resourcePolicy:
    containerPolicies:
    - containerName: api
      minAllowed:
        cpu: "100m"                          # 最低不低于 100m
        memory: "128Mi"
      maxAllowed:
        cpu: "4000m"                         # 最高不超过 4 核
        memory: "8Gi"
      controlledResources: ["cpu", "memory"]
```

### 生产 VPA：自动调整 + 资源边界

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-api-vpa-auto
  namespace: prod
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-api
  updatePolicy:
    updateMode: "Recreate"                   # 驱逐重建应用新资源
  resourcePolicy:
    containerPolicies:
    - containerName: api
      minAllowed:
        cpu: "200m"
        memory: "256Mi"
      maxAllowed:
        cpu: "2000m"
        memory: "4Gi"
      controlledResources: ["cpu", "memory"]
      controlledValues: RequestsAndLimits    # 同时调整 request 和 limit
    # Sidecar 容器使用固定资源，不受 VPA 管理
    - containerName: fluent-bit
      mode: "Off"                            # 禁止 VPA 调整此容器
```

### VPA 与 HPA 共存方案

```yaml
# VPA: 仅调整 request（不影响 HPA 的副本数决策）
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: web-api-vpa
  namespace: prod
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-api
  updatePolicy:
    updateMode: "Initial"                    # 仅在 Pod 创建时应用，减少中断
  resourcePolicy:
    containerPolicies:
    - containerName: api
      controlledValues: RequestsOnly         # 仅调整 request，limit 不变
      minAllowed:
        cpu: "100m"
        memory: "128Mi"
      maxAllowed:
        cpu: "2000m"
        memory: "4Gi"
---
# HPA: 基于自定义指标（非 CPU/Memory）进行水平扩缩
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-api-hpa
  namespace: prod
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-api
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"
```

## 故障排查

### VPA 推荐值不出现
- **症状**: `kubectl describe vpa` 中 `recommendation` 为空。
- **常见原因**: Metrics Server 未安装；VPA Recommender 未运行；Pod 运行时间太短（需要几分钟的数据）。
- **诊断命令**:
  ```bash
  # 查看 VPA 推荐值
  kubectl describe vpa web-api-vpa -n prod
  
  # 检查 VPA 组件是否健康
  kubectl get pods -n kube-system -l app=vpa-recommender
  kubectl get pods -n kube-system -l app=vpa-updater
  kubectl get pods -n kube-system -l app=vpa-admission-controller
  
  # 验证 Metrics Server
  kubectl top pods -n prod
  ```

### VPA 频繁驱逐 Pod
- **症状**: Pod 被反复驱逐重建，导致服务不稳定。
- **常见原因**: `minAllowed`/`maxAllowed` 范围过大，推荐值波动剧烈。
- **解决方案**: 缩小 `minAllowed`/`maxAllowed` 范围；切换到 `Initial` 模式减少中断。

### VPA 与 HPA 冲突
- **症状**: 副本数和资源同时变化，导致不可预测的行为。
- **解决方案**: VPA 使用 `controlledValues: RequestsOnly`；HPA 基于自定义指标（非 CPU/Memory）扩缩。

## 生产检查清单

- [ ] VPA CRD 和三个组件（Recommender、Updater、Admission Controller）已安装
- [ ] Metrics Server 健康运行
- [ ] 初次部署使用 `Off` 模式观察推荐值，验证合理后再启用自动模式
- [ ] `minAllowed`/`maxAllowed` 设置了合理的资源边界
- [ ] 如有 HPA 共存，VPA 使用 `RequestsOnly` 模式
- [ ] PDB 已配置，防止 VPA 驱逐导致服务不可用
- [ ] Sidecar 等辅助容器通过 `mode: "Off"` 排除 VPA 管理

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VPA 推荐值
kubectl describe vpa <vpa-name> -n prod

# 查看 VPA 列表
kubectl get vpa -n prod

# 查看 VPA 推荐的资源值（JSON 格式）
kubectl get vpa <vpa-name> -n prod -o jsonpath='{.status.recommendation.containerRecommendations}'

# 查看 VPA 组件状态
kubectl get pods -n kube-system -l 'app in (vpa-recommender,vpa-updater,vpa-admission-controller)'
```
## 交叉引用

- [HPA 水平自动扩缩](./horizontal-pod-autoscaling.md)
- [自动扩缩容概览](./autoscaling-workloads.md)
- [VPA 故障树分析 (FTA)](../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/06-FTA%E6%95%85%E9%9A%9C%E6%A0%91/list/vpa-fta.md)
- [Pod QoS 等级](./pod-quality-of-service-classes.md)
- [工作负载监控与告警](../../../02-%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/01-%E6%A0%B8%E5%BF%83%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/06-workload-monitoring-alerting.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/autoscaling/vertical-pod-autoscale/

## Related

- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]


<!-- risk-assessed -->
