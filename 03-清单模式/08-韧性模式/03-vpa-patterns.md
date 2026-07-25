---
title: VPA 集成模式
description: VerticalPodAutoscaler 集成配置与最佳实践
summary: VPA 三种模式（Off/Initial/Auto）、资源推荐、与 HPA 协同及 InPlace Pod Resize
category: manifests-patterns
tags:
- k8s
- manifests
- reliability
- vpa
- autoscaling
- resources
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- VPA 如何配置
- VPA 与 HPA 协同
- 资源自动推荐
trigger_keywords:
- vpa
- verticalpodautoscaler
- resources
- recommendation
- in-place-resize
prerequisites:
- k8s-resources-basics
- hpa-basics
authors:
- name: KUDIG Team
  role: contributor
---

# VPA 集成模式

## 1. VPA 三种模式

| 模式 | 行为 | 适用场景 |
|------|------|----------|
| `Off` | 仅推荐，不修改 | 观察分析 |
| `Initial` | 创建 Pod 时设置推荐值 | 不中断运行中 Pod |
| `Auto` | 自动调整（重启 Pod） | 生产推荐 |

## 2. 基础 VPA 配置

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: Auto             # 自动调整（会重启 Pod）
    minReplicas: 3               # 最少 3 副本时才执行
  resourcePolicy:
    containerPolicies:
      - name: app
        minAllowed:
          cpu: 100m
          memory: 128Mi
        maxAllowed:
          cpu: 4
          memory: 8Gi
        controlledResources: ["cpu", "memory"]
        controlledValues: RequestsAndLimits
```

## 3. Off 模式（仅推荐）

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-recommender-only
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Off"            # 只记录推荐，不修改
```

```bash
# 🟢 低风险：查看推荐
kubectl describe vpa app-recommender-only
# 输出示例:
# Recommendation:
#   Container Recommendations:
#     Target: cpu 250m, memory 256Mi
#     Lower Bound: cpu 100m, memory 128Mi
#     Upper Bound: cpu 500m, memory 512Mi
#     Uncapped Target: cpu 300m, memory 280Mi
```

## 4. Initial 模式（创建时设置）

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-initial
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: Initial          # 仅在 Pod 创建时注入推荐值
  resourcePolicy:
    containerPolicies:
      - name: app
        minAllowed:
          cpu: 100m
          memory: 256Mi
        maxAllowed:
          cpu: 2
          memory: 4Gi
```

## 5. VPA + HPA 协同

```yaml
# HPA 管理副本数（基于自定义指标/QPS）
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
spec:
  metrics:
    - type: Pods
      pods:
        metric:
          name: http_requests_per_second
        target:
          type: AverageValue
          averageValue: "100"
    # 注意：不要让 HPA 和 VPA 同时管理 CPU
---
# VPA 管理资源大小（仅内存）
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: Auto
  resourcePolicy:
    containerPolicies:
      - name: app
        controlledResources: ["memory"]  # VPA 只管内存
        minAllowed:
          memory: 128Mi
        maxAllowed:
          memory: 2Gi
```

> ⚠️ **关键规则**：HPA 和 VPA 不能同时控制同一资源维度。通常 HPA 控制 CPU/自定义指标，VPA 只控制内存。

## 6. Auto 模式的重启行为

```
VPA 检测到资源需求变化
  ↓
驱逐旧 Pod（遵循 PDB）
  ↓
创建新 Pod，注入推荐资源值
  ↓
新 Pod 就绪
```

## 7. InPlace Pod Resize（v1.33+ Alpha）

无需重启 Pod 即可调整资源：

```yaml
# 启用 InPlace Pod Resize 特性门控
# kube-apiserver: --feature-gates=InPlacePodVerticalScaling=true
apiVersion: apps/v1
kind: Deployment
metadata:
  name: resizable-app
spec:
  template:
    spec:
      containers:
        - name: app
          image: my-app:v1.0
          resizePolicy:           # 定义 resize 策略
            - resourceName: cpu
              restartPolicy: NotRequired  # CPU 可热调
            - resourceName: memory
              restartPolicy: RestartContainer  # 内存需重启
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: "2"
              memory: 2Gi
```

```yaml
# VPA 配合 InPlace Resize
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: inplace-vpa
spec:
  updatePolicy:
    updateMode: Auto
    # v1.33+ VPA 优先使用 InPlace Resize（不重启）
```

## 8. VPA 组件架构

```
VPA Recommender
  ├── 分析 metrics-server 数据
  ├── 计算 CPU/内存推荐值
  └── 更新 VPA Status.Recommendation

VPA Updater
  ├── 监控 Pod 资源使用
  ├── 检测偏离推荐值的 Pod
  └── 驱逐 Pod（触发重建）

VPA Admission Controller
  ├── 拦截 Pod 创建请求
  ├── 注入推荐的资源值
  └── 覆盖原有 resources
```

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| 先用 Off 模式观察 | 收集 1-2 周数据再切 Auto |
| 设置 minAllowed/maxAllowed | 防止极端值 |
| 与 HPA 配合（错开维度） | HPA 管 CPU，VPA 管内存 |
| 注意 Auto 模式会重启 Pod | 确保有 PDB 保护 |
| 使用 Initial 模式 | 适合不希望中断的应用 |
| 关注 InPlace Resize | 未来方向，减少重启 |

## 10. 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| VPA 不更新 | `updateMode: Off` | 切换为 Auto/Initial |
| Pod 频繁重启 | VPA 频繁调整 | 增加 `minReplicas` 或用 Initial |
| HPA 和 VPA 冲突 | 同时控制 CPU | 错开控制维度 |
| 推荐值不合理 | 样本不足 | 观察 2 周以上 |

## Related

- [[03-清单模式/08-韧性模式/02-hpa-advanced-patterns|HPA 高级模式]]
- [[03-清单模式/08-韧性模式/01-pdb-patterns|PDB 模式]]

## See Also

- [VPA GitHub](https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler)
- [InPlace Pod Resize](https://kubernetes.io/blog/2023/05/12/in-place-pod-resize-alpha/)

<!-- risk-assessed -->
