---
title: Operator Finalizer 清理模式
description: Finalizer 机制详解、删除流程、级联清理与常见陷阱
summary: Finalizer 实现资源删除前的清理逻辑，包括外部资源回收、有序删除及常见反模式
category: manifests-patterns
tags:
- k8s
- manifests
- operator
- finalizer
- cleanup
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- 开发工程师
- SRE
estimated_read_time: 10min
intent_queries:
- Finalizer 如何工作
- Operator 删除清理
- CR 删除前外部资源回收
trigger_keywords:
- finalizer
- deletion
- cleanup
- garbage-collection
prerequisites:
- operator-basics
- crd-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Operator Finalizer 清理模式

## 1. Finalizer 机制

Finalizer 是元数据中的字符串列表，当存在时 Kubernetes **不会真正删除对象**。控制器在清理完成后移除 finalizer，对象才被垃圾回收。

```yaml
apiVersion: platform.example.com/v1
kind: Database
metadata:
  name: my-db
  finalizers:
    - platform.example.com/db-cleanup
spec:
  engine: postgresql
  instanceId: aws-rds-prod-12345
```

## 2. 删除流程

```
用户执行 kubectl delete
      ↓
API Server 检查 metadata.deletionTimestamp
      ↓
deletionTimestamp == nil ?
  ├── 是 → 设置 deletionTimestamp，对象进入 "Terminating" 状态
  │        （此时对象仍可读，但不能再更新 spec）
  └── 否 → 对象已有 deletionTimestamp，进入清理流程
      ↓
Reconcile 检测到 deletionTimestamp != nil
      ↓
执行清理逻辑（删除外部资源）
      ↓
移除 finalizer（patch metadata.finalizers）
      ↓
对象被 GC 真正删除
```

## 3. Reconcile 中的 Finalizer 逻辑

```go
const finalizerName = "platform.example.com/db-cleanup"

func (r *DatabaseReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    var db platformv1.Database
    if err := r.Get(ctx, req.NamespacedName, &db); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }

    // === 正在删除 ===
    if !db.ObjectMeta.DeletionTimestamp.IsZero() {
        if controllerutil.ContainsFinalizer(&db, finalizerName) {
            if err := r.cleanupExternalResources(ctx, &db); err != nil {
                return ctrl.Result{}, err // 清理失败会重试
            }
            controllerutil.RemoveFinalizer(&db, finalizerName)
            if err := r.Update(ctx, &db); err != nil {
                return ctrl.Result{}, err
            }
        }
        return ctrl.Result{}, nil
    }

    // === 正常创建/更新 ===
    if !controllerutil.ContainsFinalizer(&db, finalizerName) {
        controllerutil.AddFinalizer(&db, finalizerName)
        if err := r.Update(ctx, &db); err != nil {
            return ctrl.Result{}, err
        }
        return ctrl.Result{Requeue: true}, nil // 确认 finalizer 已添加
    }

    return r.reconcileNormal(ctx, &db)
}
```

## 4. 外部资源清理

```go
func (r *DatabaseReconciler) cleanupExternalResources(ctx context.Context, db *platformv1.Database) error {
    // 1. 删除云数据库实例
    if err := r.cloudClient.DeleteDBInstance(ctx, db.Spec.InstanceId); err != nil {
        if !isNotFound(err) {
            return fmt.Errorf("删除 RDS 实例失败: %w", err)
        }
    }

    // 2. 删除 DNS 记录
    if err := r.dnsClient.DeleteRecord(ctx, fmt.Sprintf("%s.internal", db.Name)); err != nil {
        return fmt.Errorf("删除 DNS 记录失败: %w", err)
    }

    // 3. 删除监控告警
    if err := r.monitoringClient.DeleteAlerts(ctx, db.Name); err != nil {
        return fmt.Errorf("删除告警规则失败: %w", err)
    }

    return nil
}
```

## 5. 多 Finalizer 顺序删除

```yaml
metadata:
  finalizers:
    - platform.example.com/cleanup-secrets   # 先清理 Secret
    - platform.example.com/cleanup-dns       # 再清理 DNS
    - platform.example.com/cleanup-infra     # 最后删除基础设施
```

Finalizer **没有保证执行顺序**，控制器需自行处理依赖。

## 6. 常见陷阱

| 陷阱 | 说明 | 解决方案 |
|------|------|----------|
| Finalizer 永不移除 | 清理逻辑失败但不返回错误 | 清理失败必须 return error |
| Finalizer 名称冲突 | 多控制器使用相同前缀 | 使用 `group/resource` 格式 |
| 卡在 Terminating | 控制器已下线 | 手动 `kubectl patch` 移除 |
| 嵌套删除死锁 | A 的 finalizer 等 B 删除，B 的 finalizer 等 A | 破除循环依赖 |

## 7. 手动移除卡住的 Finalizer

> ⚠️ **🔴 高危操作** — 仅在控制器已永久下线时使用，会导致外部资源泄漏

```bash
# 🔴 高风险：手动移除 finalizer，可能导致外部资源残留
kubectl patch database my-db --type='json' \
  -p='[{"op":"remove","path":"/metadata/finalizers"}]'
```

## Related

- [[03-清单模式/04-Operator模式/02-operator-reconciliation-patterns|调谐循环模式]]
- [[03-清单模式/04-Operator模式/06-operator-status-conditions|Status Conditions 设计]]

## See Also

- [Kubernetes Finalizer 文档](https://kubernetes.io/docs/concepts/overview/working-with-objects/finalizers/)
- [controllerutil Finalizer helper](https://pkg.go.dev/sigs.k8s.io/controller-runtime/pkg/controller/controllerutil)

<!-- risk-assessed -->
