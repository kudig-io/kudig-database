---
title: Operator 调谐循环模式
description: Reconcile Loop 设计模式、幂等性、错误处理与重试策略
summary: 深入讲解 Reconcile 循环的幂等设计、水平触发 vs 边沿触发、指数退避重试及生产级调谐模式
category: manifests-patterns
tags:
- k8s
- manifests
- operator
- reconcile
- controller
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- 开发工程师
estimated_read_time: 14min
intent_queries:
- Reconcile 循环如何设计
- Operator 幂等性
- controller-runtime reconcile
trigger_keywords:
- reconcile
- controller
- idempotent
- operator
prerequisites:
- controller-runtime
- crd-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Operator 调谐循环模式

## 1. 水平触发 vs 边沿触发

Operator 必须遵循 **水平触发（level-triggered）** 原则：基于当前状态而非事件做决策。Reconcile 函数接收一个名字（namespace/name），自行获取最新状态，而不是依赖事件数据。

```go
// ✅ 正确：水平触发 — 每次从缓存获取最新状态
func (r *WebAppReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    var webapp platformv1.WebApp
    if err := r.Get(ctx, req.NamespacedName, &webapp); err != nil {
        if errors.IsNotFound(err) {
            return ctrl.Result{}, nil // 资源已删除，无需处理
        }
        return ctrl.Result{}, err
    }
    // 基于当前 spec 状态执行逻辑
    return r.reconcile(ctx, &webapp)
}
```

## 2. 幂等性设计

Reconcile 必须可安全重复执行，以下操作均幂等：

```go
func (r *WebAppReconciler) reconcile(ctx context.Context, webapp *platformv1.WebApp) (ctrl.Result, error) {
    // 1. 获取或创建 Deployment
    var deploy appsv1.Deployment
    err := r.Get(ctx, client.ObjectKeyFromObject(webapp), &deploy)
    if errors.IsNotFound(err) {
        deploy = r.buildDeployment(webapp)
        if err := r.Create(ctx, &deploy); err != nil {
            return ctrl.Result{}, err
        }
    } else if err != nil {
        return ctrl.Result{}, err
    }

    // 2. 比较期望状态 vs 实际状态
    expectedImage := webapp.Spec.Image
    if deploy.Spec.Template.Spec.Containers[0].Image != expectedImage {
        deploy.Spec.Template.Spec.Containers[0].Image = expectedImage
        if err := r.Update(ctx, &deploy); err != nil {
            return ctrl.Result{}, err
        }
    }
    return ctrl.Result{}, nil
}
```

## 3. Status 更新模式

使用 `patch` 而非 `update` 来更新 status，减少冲突：

```go
func (r *WebAppReconciler) updateStatus(ctx context.Context, webapp *platformv1.WebApp) error {
    patch := client.MergeFrom(webapp.DeepCopy())
    webapp.Status.ReadyReplicas = r.countReadyPods(ctx, webapp)
    webapp.Status.ObservedGeneration = webapp.Generation
    webapp.Status.Phase = "Running"
    return r.Status().Patch(ctx, webapp, patch)
}
```

## 4. 指数退避重试

对临时错误使用延迟重试：

```go
func (r *WebAppReconciler) reconcile(ctx context.Context, webapp *platformv1.WebApp) (ctrl.Result, error) {
    err := r.createExternalResource(ctx, webapp)
    if err != nil {
        // 临时错误：延迟重试
        return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
    }
    // 正常完成，不主动 requeue（由 Watch 事件驱动）
    return ctrl.Result{}, nil
}
```

## 5. Owner Reference 自动垃圾回收

```go
func (r *WebAppReconciler) buildDeployment(webapp *platformv1.WebApp) *appsv1.Deployment {
    return &appsv1.Deployment{
        ObjectMeta: metav1.ObjectMeta{
            Name:      webapp.Name,
            Namespace: webapp.Namespace,
            Labels: map[string]string{
                "app.kubernetes.io/managed-by": "webapp-operator",
            },
        },
        Spec: appsv1.DeploymentSpec{
            Replicas: ptr.To(int32(webapp.Spec.Replicas)),
            Selector: &metav1.LabelSelector{
                MatchLabels: map[string]string{"app": webapp.Name},
            },
            Template: corev1.PodTemplateSpec{
                ObjectMeta: metav1.ObjectMeta{
                    Labels: map[string]string{"app": webapp.Name},
                },
                Spec: corev1.PodSpec{
                    Containers: []corev1.Container{{
                        Name:  "app",
                        Image: webapp.Spec.Image,
                    }},
                },
            },
        },
    }
}

// 使用 SetControllerReference 设置 Owner Reference
ctrl.SetControllerReference(webapp, deploy, r.Scheme)
```

## 6. Reconcile 生命周期

```
Event (Create/Update/Delete)
      ↓
Work Queue (去重 + 限速)
      ↓
Reconcile(req)
  ├── Get CR → 不存在? return (已删除)
  ├── 检查 DeletionTimestamp → 有? 执行 Finalizer 清理
  ├── 对比 desired vs actual
  ├── Create/Update 子资源
  ├── Update Status
  └── return Result{RequeueAfter?}
```

## 7. 生产实践

| 模式 | 说明 |
|------|------|
| 单一 Reconcile 入口 | 不要在 Reconcile 中启动 goroutine |
| 使用 client-go 限速队列 | 避免对 API Server 产生压力 |
| 区分可重试错误与永久错误 | 永久错误不应 requeue |
| `ObservedGeneration` 追踪 | status.observedGeneration == metadata.generation 表示已同步 |
| 条件更新（patch） | 避免无条件写 status 导致热循环 |

## Related

- [[03-清单模式/04-Operator模式/01-operator-cr-design-patterns|CRD 设计模式]]
- [[03-清单模式/04-Operator模式/04-operator-finalizer-cleanup|Finalizer 清理模式]]

## See Also

- [controller-runtime 最佳实践](https://pkg.go.dev/sigs.k8s.io/controller-runtime)
- [Kubernetes 控制器模式](https://kubernetes.io/docs/concepts/architecture/controller/)

<!-- risk-assessed -->
