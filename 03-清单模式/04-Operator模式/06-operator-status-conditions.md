---
title: Operator Status Conditions 设计
description: Status Conditions 标准设计模式、类型定义与消费者最佳实践
summary: 使用 metav1.Condition 标准化 Status 设计，包含条件类型命名、Reason/Message 规范及使用模式
category: manifests-patterns
tags:
- k8s
- manifests
- operator
- status
- conditions
- api-design
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 平台工程师
- 开发工程师
- SRE
estimated_read_time: 10min
intent_queries:
- Status Conditions 如何设计
- CRD status conditions 最佳实践
- Operator 状态报告
trigger_keywords:
- conditions
- status
- ready
- available
- reconciled
prerequisites:
- operator-basics
- crd-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Operator Status Conditions 设计

## 1. Conditions vs Phase

| 模式 | 优点 | 缺点 |
|------|------|------|
| **Phase** (单一字符串) | 简单直观 | 无法表示多维度状态 |
| **Conditions** (条件列表) | 多维度独立状态 | 略复杂 |
| **Phase + Conditions** | 兼顾可读性与精确性 | 最佳实践 |

## 2. 标准 Condition 结构

```yaml
status:
  observedGeneration: 3
  conditions:
    - type: Ready               # 聚合条件：所有子条件都 True
      status: "False"
      lastTransitionTime: "2026-07-11T08:00:00Z"
      reason: DeploymentNotReady
      message: "Deployment 有 2/3 副本就绪"
    - type: DeploymentReady
      status: "False"
      lastTransitionTime: "2026-07-11T08:00:05Z"
      reason: Progressing
      message: " ReplicaSet 正在滚动更新"
    - type: ServiceReady
      status: "True"
      lastTransitionTime: "2026-07-11T07:59:00Z"
      reason: ServiceAvailable
      message: "Service 已创建并分配 ClusterIP"
    - type: IngressReady
      status: "True"
      lastTransitionTime: "2026-07-11T07:59:05Z"
      reason: IngressConfigured
      message: "Ingress 路由已配置"
```

## 3. CRD Schema 定义

```yaml
properties:
  status:
    type: object
    properties:
      observedGeneration:
        type: integer
        format: int64
      readyReplicas:
        type: integer
      phase:
        type: string
        enum: [Pending, Running, Failed, Degraded]
      conditions:
        type: array
        x-kubernetes-list-type: map
        x-kubernetes-list-map-keys: [type]
        items:
          type: object
          required: [type, status, lastTransitionTime, reason, message]
          properties:
            type:
              type: string
              description: "条件类型，如 Ready, Available, Progressing"
            status:
              type: string
              enum: ["True", "False", "Unknown"]
            observedGeneration:
              type: integer
              format: int64
            lastTransitionTime:
              type: string
              format: date-time
            reason:
              type: string
              description: "驼峰式原因码，如 DeploymentNotReady"
            message:
              type: string
              description: "人类可读的详细信息"
```

## 4. Go 实现（使用 metav1.Condition）

```go
import (
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

func (r *WebAppReconciler) setCondition(ctx context.Context, webapp *platformv1.WebApp) error {
    conditions := []metav1.Condition{}

    // Deployment 条件
    deployReady := r.checkDeploymentReady(ctx, webapp)
    conditions = append(conditions, metav1.Condition{
        Type:               "DeploymentReady",
        Status:             boolToConditionStatus(deployReady),
        ObservedGeneration: webapp.Generation,
        LastTransitionTime: metav1.Now(),
        Reason:             ternary(deployReady, "DeploymentAvailable", "DeploymentNotReady"),
        Message:            ternary(deployReady, "Deployment 副本全部就绪", "Deployment 副本不足"),
    })

    // Service 条件
    svcReady := r.checkServiceReady(ctx, webapp)
    conditions = append(conditions, metav1.Condition{
        Type:               "ServiceReady",
        Status:             boolToConditionStatus(svcReady),
        ObservedGeneration: webapp.Generation,
        LastTransitionTime: metav1.Now(),
        Reason:             ternary(svcReady, "ServiceAvailable", "ServiceNotFound"),
        Message:            ternary(svcReady, "Service 已创建", "Service 不存在"),
    })

    // 聚合 Ready 条件
    allReady := deployReady && svcReady
    conditions = append(conditions, metav1.Condition{
        Type:               "Ready",
        Status:             boolToConditionStatus(allReady),
        ObservedGeneration: webapp.Generation,
        LastTransitionTime: metav1.Now(),
        Reason:             ternary(allReady, "AllResourcesReady", "ResourcesNotReady"),
        Message:            ternary(allReady, "所有资源已就绪", "部分资源未就绪"),
    })

    return r.patchStatus(ctx, webapp, conditions)
}
```

## 5. 常见 Condition 类型约定

| 类型 | 含义 | 常见 Reason |
|------|------|-------------|
| `Ready` | 聚合就绪状态 | `AllResourcesReady`, `ResourcesNotReady` |
| `Available` | 服务可用 | `Available`, `Unavailable` |
| `Progressing` | 正在变更 | `ReconciliationInProgress`, `Reconciled` |
| `Degraded` | 功能降级 | `Degraded`, `NotDegraded` |
| `Synced` | 已同步到期望状态 | `Synced`, `OutOfSync` |

## 6. 消费者最佳实践

```bash
# 🟢 低风险：只读查询
# 使用 fieldSelector 按 condition 查询
kubectl get webapps -o json \
  | jq '.items[] | select(.status.conditions[] | select(.type=="Ready" and .status=="False"))'

# 检查未同步的资源
kubectl get webapps -o json \
  | jq '.items[] | select(.status.observedGeneration < .metadata.generation)'
```

## 7. 生产实践

- **总是包含 `Ready` 聚合条件**：方便 kubectl 和自动化工具快速判断状态
- **设置 `observedGeneration`**：消费者可判断控制器是否已处理最新 spec
- **只在状态变化时更新 `lastTransitionTime`**：避免每次 Reconcile 都刷新
- **使用 `x-kubernetes-list-type: map`**：确保按 `type` 键去重
- **Reason 使用驼峰命名**：如 `DeploymentNotReady`，便于程序化判断

## Related

- [[03-清单模式/04-Operator模式/01-operator-cr-design-patterns|CRD 设计模式]]
- [[03-清单模式/04-Operator模式/07-operator-metrics-observability|Metrics 可观测性]]

## See Also

- [Kubernetes Condition 类型约定](https://github.com/kubernetes/community/blob/master/contributors/devel/sig-architecture/api-conventions.md#typical-status-properties)
- [ metav1.Condition 文档](https://pkg.go.dev/k8s.io/apimachinery/pkg/apis/meta/v1#Condition)

<!-- risk-assessed -->
