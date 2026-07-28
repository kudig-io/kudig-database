---
title: Horizontal Pod Autoscaling
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- controller-manager
- prometheus
- hpa
- vpa
- pdb
- statefulset
- rag
tier: peripheral
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Horizontal Pod Autoscaling 是什么
- 如何 Horizontal Pod Autoscaling
trigger_keywords:
- Horizontal
- Pod
- Autoscaling
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Horizontal Pod Autoscaling

## 概述
HorizontalPodAutoscaler（HPA）是 [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] 的 API 资源和控制器，可根据观察到的指标（如 CPU 利用率、内存利用率或自定义指标）自动调整工作负载（Deployment、[[statefulset|StatefulSet]] 等）的副本数量。

## 核心概念/原理
- **控制循环**：HPA 控制器在 kube-controller-manager 中以固定周期运行（默认 15 秒），查询指标并调整目标副本数。
- **缩放目标**：通过 `scaleTargetRef` 指向支持 `scale` 子资源的工作负载（如 Deployment、StatefulSet、[[replicaset|ReplicaSet]]）。
- **副本数计算**：
  ```
  desiredReplicas = ceil(currentReplicas * currentMetricValue / desiredMetricValue)
  ```
  当比值接近 1.0 时（默认容差 10%），控制器跳过缩放动作。
- **缺失指标与未就绪 Pod 处理**：
  - 缺失指标的 Pod 在缩容时按 100% 利用率假设，在扩容时按 0% 假设，以保守方式 dampen 缩放幅度。
  - 未就绪 Pod 的 CPU  metrics 在初始就绪延迟（默认 30 秒）和 CPU 初始化期（默认 5 分钟）内可能被忽略。

## 关键机制或特性
- **指标类型（autoscaling/v2）**：
  - **Resource metrics**：基于 Pod 级别的 CPU 或内存利用率/原始值。
  - **Container resource metrics**（v1.30 Stable）：基于特定容器的资源使用进行缩放。
  - **Custom metrics**：通过 `custom.metrics.k8s.io` 获取的自定义指标。
  - **External metrics**：通过 `external.metrics.k8s.io` 获取的外部指标。
- **多指标支持**：可配置多个指标，HPA 会计算每个指标对应的期望副本数，最终取最大值。
- **行为配置（`behavior`）**：
  - `scaleUp` / `scaleDown`：分别配置扩容和缩容行为。
  - `policies`：定义缩放速率（按 Pods 数或百分比）。
  - `stabilizationWindowSeconds`：缩容稳定窗口，默认 300 秒，用于平滑副本波动。
  - `selectPolicy`：`Max`（默认，允许最大变化）、`Min`（最小变化）、`Disabled`（禁用该方向缩放）。
  - `tolerance`（v1.35 Beta）：指标波动容差，默认 10%；例如目标 100MiB、容差 5%，则仅在超过 105MiB 时才扩容。
- **Pod 就绪与启动**：
  - `--horizontal-pod-autoscaler-initial-readiness-delay`（默认 30s）
  - `--horizontal-pod-autoscaler-cpu-initialization-period`（默认 5m）

## 使用场景
- 流量波动明显的无状态 Web 服务和 API。
- 需要基于队列长度、请求延迟等自定义指标自动扩容的场景。
- 在滚动更新期间保持应用可用性并自动调整容量。

## 最佳实践/注意事项
- 使用 `autoscaling/v2` API 以利用多指标、行为配置和容器级资源指标。
- 确保 Metrics Server（resource metrics）或相应的 custom/external metrics adapter 已部署。
- 使用 HPA 时，建议从目标工作负载的 manifest 中移除 `spec.replicas`，避免 `kubectl apply` 引起副本数抖动（thrashing）。
- 对于启动期 CPU 突增的应用，配置合适的 `startupProbe` 或 `readinessProbe`，并确保 `cpu-initialization-period` 覆盖启动时长。
- 滚动更新期间，Deployment 控制器与 HPA 协同管理 ReplicaSet 副本数；StatefulSet 则由 StatefulSet 控制器直接处理。

## 实战 YAML 示例

### 基于 CPU 的 HPA（基础配置）

```yaml
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
  minReplicas: 3                             # 最小副本数
  maxReplicas: 20                            # 最大副本数
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70               # 目标 CPU 平均利用率 70%
```

### 多指标 HPA（CPU + 自定义指标 + 行为控制）

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-api-hpa-advanced
  namespace: prod
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-api
  minReplicas: 3
  maxReplicas: 50
  metrics:
  # 指标 1: CPU 利用率
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  # 指标 2: 内存利用率
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  # 指标 3: 自定义指标（每秒请求数）
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"                 # 每个 Pod 目标 1000 QPS
  # 指标 4: 外部指标（消息队列深度）
  - type: External
    external:
      metric:
        name: queue_messages_ready
        selector:
          matchLabels:
            queue: "orders"
      target:
        type: Value
        value: "500"                         # 队列中消息 < 500 时缩容
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60         # 扩容稳定窗口 1 分钟
      policies:
      - type: Percent
        value: 100                           # 每次最多扩容 100%
        periodSeconds: 60
      - type: Pods
        value: 10                            # 或每次最多加 10 个 Pod
        periodSeconds: 60
      selectPolicy: Max                      # 取两个策略中更大的变化
    scaleDown:
      stabilizationWindowSeconds: 300        # 缩容稳定窗口 5 分钟（防止抖动）
      policies:
      - type: Percent
        value: 10                            # 每次最多缩容 10%
        periodSeconds: 60
      selectPolicy: Min                      # 取更保守的策略
```

## 故障排查

### HPA 不触发扩容
- **症状**: 负载升高但副本数不变，HPA 的 `TARGETS` 显示 `<unknown>`。
- **常见原因**: Metrics Server 未安装或不可用；Pod 未设置 `resources.requests`。
- **诊断命令**:
  ```bash
  # 查看 HPA 状态和当前指标
  kubectl get hpa web-api-hpa -n prod
  
  # 查看 HPA 详细条件和事件
  kubectl describe hpa web-api-hpa -n prod
  
  # 验证 Metrics Server 是否正常
  kubectl top pods -n prod
  kubectl get apiservice v1beta1.metrics.k8s.io -o yaml
  
  # 检查 Pod 是否设置了 resources.requests
  kubectl get pod -n prod -l app=web-api -o jsonpath='{.items[0].spec.containers[0].resources.requests}'
  ```
- **解决方案**: 安装/修复 Metrics Server；为 Pod 设置 `resources.requests`。

### HPA 频繁扩缩容（抖动/Thrashing）
- **症状**: 副本数短时间内反复增减。
- **常见原因**: 目标利用率设置过于敏感；`stabilizationWindowSeconds` 过短；应用指标波动大。
- **诊断命令**:
  ```bash
  # 查看 HPA 事件历史
  kubectl describe hpa web-api-hpa -n prod | grep -A 20 "Events"
  
  # 查看副本数变化趋势
  kubectl get hpa web-api-hpa -n prod -w

  ```
- **解决方案**: 增大 `scaleDown.stabilizationWindowSeconds`（建议 300-600 秒）；调整目标利用率留出更大缓冲。

### HPA 与 kubectl apply 冲突
- **症状**: `kubectl apply` 后 HPA 设置的副本数被覆盖。
- **常见原因**: Deployment manifest 中硬编码了 `spec.replicas`。
- **解决方案**: 从 manifest 中移除 `spec.replicas` 字段，让 HPA 全权管理副本数。

## 生产就绪检查清单

- [ ] 使用 `autoscaling/v2` API（而非 v1）
- [ ] Metrics Server 已部署且健康运行
- [ ] 目标工作负载的所有容器都设置了 `resources.requests`
- [ ] `minReplicas` >= 2（保障高可用）
- [ ] `maxReplicas` 考虑了集群资源容量上限
- [ ] `behavior.scaleDown.stabilizationWindowSeconds` >= 300 秒
- [ ] Deployment manifest 中已移除 `spec.replicas`
- [ ] 配合 PDB 使用，防止缩容时不可用副本过多
- [ ] 自定义指标场景下已部署 Prometheus Adapter 或相应的 metrics adapter

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 HPA 状态（含当前/目标指标值）
kubectl get hpa -n prod

# 查看 HPA 详细条件和事件
kubectl describe hpa <hpa-name> -n prod

# 实时监控 HPA 变化
kubectl get hpa <hpa-name> -n prod -w

# 快速创建基于 CPU 的 HPA
kubectl autoscale deployment web-api -n prod --min=3 --max=20 --cpu-percent=70

# 查看 Metrics Server 是否正常
kubectl top pods -n prod
kubectl top nodes
```
## 交叉引用

- [VPA 垂直自动扩缩](./vertical-pod-autoscaling.md)
- [自动扩缩容概览](../../../../17-%E7%B3%BB%E7%BB%9F%E5%9F%BA%E7%A1%80/06-%E7%9F%A5%E8%AF%86%E5%AD%97%E5%85%B8/workloads/autoscaling-workloads.md)
- [HPA 故障树分析 (FTA)](../../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/06-FTA%E6%95%85%E9%9A%9C%E6%A0%91/list/hpa-fta.md)
- [工作负载监控与告警](../../../../02-%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/01-%E6%A0%B8%E5%BF%83%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/06-workload-monitoring-alerting.md)
- [Deployments](./deployments.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/autoscaling/horizontal-pod-autoscale/

## Related

- [[17-系统基础/06-知识字典/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[17-系统基础/06-知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[17-系统基础/06-知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]

```

<!-- risk-assessed -->
