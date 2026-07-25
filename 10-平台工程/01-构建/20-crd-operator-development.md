---
title: 31 - CRD与Operator开发
description: 'storage: false  # 不是存储版本'
summary: 'storage: false  # 不是存储版本'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- prometheus
- helm
- docker
- rbac
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- CRD与Operator开发 是什么
- 如何 CRD与Operator开发
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- CRD与Operator开发
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
- helm-basics
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: domain
  path: ../专项技术/
  label: '相关知识域: 专项技术'
- type: domain
  path: ../故障诊断/
  label: '相关知识域: 故障诊断'
- type: fta
  path: ../故障诊断/FTA故障树/list/crd-operator-fta.md
  label: '故障树: crd-operator'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 31 - CRD与Operator开发

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].io/docs/concepts/extend-kubernetes/api-extension/custom-resources](https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/custom-resources/)

<!-- chunk: CRD版本规范 -->
## CRD版本规范

| API版本 | 状态 | 功能 | K8s版本 |
|--------|-----|------|--------|
| apiextensions.k8s.io/v1beta1 | 已移除 | 基础CRD | v1.22前 |
| apiextensions.k8s.io/v1 | 稳定 | 完整功能 | v1.16+ |

<!-- chunk: CRD结构定义 -->
## CRD结构定义

| 字段 | 类型 | 必需 | 说明 |
|-----|-----|-----|------|
| `spec.group` | string | ✅ | API组名 |
| `spec.names.kind` | string | ✅ | 资源类型 |
| `spec.names.plural` | string | ✅ | 复数名称 |
| `spec.names.singular` | string | ❌ | 单数名称 |
| `spec.names.shortNames` | []string | ❌ | 短名称 |
| `spec.scope` | Namespaced/Cluster | ✅ | 作用域 |
| `spec.versions` | []Version | ✅ | 版本列表 |
| `spec.conversion` | Conversion | ❌ | 版本转换 |

<!-- chunk: CRD验证规则 -->
## CRD验证规则

| 验证类型 | 字段 | 示例 |
|---------|-----|------|
| 必需字段 | `required` | `required: [name, replicas]` |
| 类型验证 | `type` | `type: string` |
| 枚举值 | `enum` | `enum: [Running, Stopped]` |
| 数值范围 | `minimum/maximum` | `minimum: 1, maximum: 100` |
| 字符串长度 | `minLength/maxLength` | `minLength: 1` |
| 正则匹配 | `pattern` | `pattern: "^[a-z]+$"` |
| 数组长度 | `minItems/maxItems` | `minItems: 1` |
| 默认值 | `default` | `default: 3` |
| CEL验证 | `x-kubernetes-validations` | 自定义验证(v1.25+) |

<!-- chunk: CRD示例 -->
## CRD示例

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: applications.app.example.com
  annotations:
    controller-gen.kubebuilder.io/version: v0.14.0
spec:
  group: app.example.com
  names:
    kind: Application
    plural: applications
    singular: application
    shortNames: [app]
    categories: [all]  # kubectl get all可以看到
  scope: Namespaced
  versions:
  - name: v1
    served: true
    storage: true
    schema:
      openAPIV3Schema:
        type: object
        required: [spec]
        properties:
          spec:
            type: object
            required: [image, replicas]
            properties:
              image:
                type: string
                pattern: "^[a-z0-9.-]+/[a-z0-9.-]+:[a-z0-9.-]+$"
              replicas:
                type: integer
                minimum: 1
                maximum: 100
                default: 1
              ports:
                type: array
                maxItems: 10
                items:
                  type: object
                  required: [port]
                  properties:
                    port:
                      type: integer
                      minimum: 1
                      maximum: 65535
                    protocol:
                      type: string
                      enum: [TCP, UDP]
                      default: TCP
              resources:
                type: object
                properties:
                  cpu:
                    type: string
                    pattern: "^[0-9]+m?$"
                  memory:
                    type: string
                    pattern: "^[0-9]+(Mi|Gi)$"
            # CEL验证规则(v1.25+)
            x-kubernetes-validations:
            - rule: "self.replicas <= 10 || has(self.highAvailability)"
              message: "replicas > 10 requires highAvailability config"
            - rule: "!has(self.resources) || (has(self.resources.cpu) && has(self.resources.memory))"
              message: "if resources specified, both cpu and memory required"
          status:
            type: object
            properties:
              phase:
                type: string
                enum: [Pending, Running, Failed, Succeeded]
              availableReplicas:
                type: integer
              conditions:
                type: array
                items:
                  type: object
                  required: [type, status]
                  properties:
                    type:
                      type: string
                    status:
                      type: string
                      enum: ["True", "False", "Unknown"]
                    reason:
                      type: string
                    message:
                      type: string
                    lastTransitionTime:
                      type: string
                      format: date-time
    subresources:
      status: {}
      scale:
        specReplicasPath: .spec.replicas
        statusReplicasPath: .status.availableReplicas
    additionalPrinterColumns:
    - name: Replicas
      type: integer
      jsonPath: .spec.replicas
    - name: Available
      type: integer
      jsonPath: .status.availableReplicas
    - name: Phase
      type: string
      jsonPath: .status.phase
    - name: Age
      type: date
      jsonPath: .metadata.creationTimestamp
  # 版本转换
  conversion:
    strategy: Webhook
    webhook:
      clientConfig:
        service:
          namespace: system
          name: webhook-service
          path: /convert
      conversionReviewVersions: ["v1"]
```

<!-- chunk: CRD版本转换 -->
## CRD版本转换

```yaml
# 多版本CRD示例
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: databases.db.example.com
spec:
  group: db.example.com
  names:
    kind: Database
    plural: databases
  scope: Namespaced
  versions:
  - name: v1
    served: true
    storage: false  # 不是存储版本
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              size:
                type: string  # v1使用string
  - name: v2
    served: true
    storage: true   # v2是存储版本
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              storageSize:
                type: integer  # v2改为integer(Gi)
              storageClass:
                type: string
  conversion:
    strategy: Webhook
    webhook:
      clientConfig:
        service:
          namespace: system
          name: conversion-webhook
          path: /convert
      conversionReviewVersions: ["v1"]
```

<!-- chunk: Operator开发框架对比 -->
## Operator开发框架对比

| 框架 | 语言 | 学习曲线 | 功能 | 社区活跃度 | 适用场景 |
|-----|-----|---------|-----|-----------|---------|
| **Kubebuilder** | Go | 中 | 完整 | ⭐⭐⭐⭐⭐ | 生产级Operator |
| **Operator SDK** | Go/Ansible/Helm | 中 | 完整 | ⭐⭐⭐⭐⭐ | 多语言支持 |
| **controller-runtime** | Go | 高 | 底层 | ⭐⭐⭐⭐⭐ | 高度定制 |
| **KUDO** | YAML | 低 | 基础 | ⭐⭐⭐ | 简单有状态应用 |
| **Metacontroller** | JS/Python | 低 | 基础 | ⭐⭐⭐ | 快速原型 |
| **kopf** | Python | 低 | 中等 | ⭐⭐⭐⭐ | Python生态 |
| **Java Operator SDK** | Java | 中 | 完整 | ⭐⭐⭐⭐ | Java生态 |

<!-- chunk: Kubebuilder开发流程 -->
## Kubebuilder开发流程

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 初始化项目
kubebuilder init --domain example.com --repo github.com/example/app-operator

# 2. 创建API
kubebuilder create api --group app --version v1 --kind Application
# 选择创建Resource和Controller

# 3. 创建Webhook(可选)
kubebuilder create webhook --group app --version v1 --kind Application \
  --defaulting --programmatic-validation

# 4. 编辑类型定义
# api/v1/application_types.go

# 5. 生成代码和清单
make generate    # 生成DeepCopy等
make manifests   # 生成CRD/RBAC/Webhook

# 6. 安装CRD
make install

# 7. 本地运行测试
make run

# 8. 构建和部署
make docker-build docker-push IMG=<registry>/app-operator:v1
make deploy IMG=<registry>/app-operator:v1

# 9. 卸载
make undeploy
make uninstall
```
<!-- chunk: Controller核心代码结构 -->
## Controller核心代码结构

```go
// internal/controller/application_controller.go
package controller

import (
    "context"
    "fmt"
    "time"

    appsv1 "k8s.io/api/apps/v1"
    corev1 "k8s.io/api/core/v1"
    "k8s.io/apimachinery/pkg/api/errors"
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/apimachinery/pkg/runtime"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "sigs.k8s.io/controller-runtime/pkg/controller/controllerutil"
    "sigs.k8s.io/controller-runtime/pkg/log"

    appv1 "github.com/example/app-operator/api/v1"
)

const applicationFinalizer = "app.example.com/finalizer"

type ApplicationReconciler struct {
    client.Client
    Scheme *runtime.Scheme
}

// +kubebuilder:rbac:groups=app.example.com,resources=applications,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=app.example.com,resources=applications/status,verbs=get;update;patch
// +kubebuilder:rbac:groups=app.example.com,resources=applications/finalizers,verbs=update
// +kubebuilder:rbac:groups=apps,resources=deployments,verbs=get;list;watch;create;update;patch;delete

func (r *ApplicationReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    logger := log.FromContext(ctx)

    // 1. 获取CR
    app := &appv1.Application{}
    if err := r.Get(ctx, req.NamespacedName, app); err != nil {
        if errors.IsNotFound(err) {
            return ctrl.Result{}, nil
        }
        return ctrl.Result{}, err
    }

    // 2. 处理删除
    if !app.DeletionTimestamp.IsZero() {
        if controllerutil.ContainsFinalizer(app, applicationFinalizer) {
            // 执行清理逻辑
            if err := r.cleanup(ctx, app); err != nil {
                return ctrl.Result{}, err
            }
            // 移除finalizer
            controllerutil.RemoveFinalizer(app, applicationFinalizer)
            if err := r.Update(ctx, app); err != nil {
                return ctrl.Result{}, err
            }
        }
        return ctrl.Result{}, nil
    }

    // 3. 添加finalizer
    if !controllerutil.ContainsFinalizer(app, applicationFinalizer) {
        controllerutil.AddFinalizer(app, applicationFinalizer)
        if err := r.Update(ctx, app); err != nil {
            return ctrl.Result{}, err
        }
    }

    // 4. 同步Deployment
    deployment := r.constructDeployment(app)
    if err := controllerutil.SetControllerReference(app, deployment, r.Scheme); err != nil {
        return ctrl.Result{}, err
    }

    found := &appsv1.Deployment{}
    err := r.Get(ctx, client.ObjectKeyFromObject(deployment), found)
    if err != nil && errors.IsNotFound(err) {
        logger.Info("Creating Deployment", "name", deployment.Name)
        if err := r.Create(ctx, deployment); err != nil {
            return ctrl.Result{}, err
        }
    } else if err == nil {
        // 更新Deployment
        if err := r.Update(ctx, deployment); err != nil {
            return ctrl.Result{}, err
        }
    } else {
        return ctrl.Result{}, err
    }

    // 5. 更新状态
    app.Status.Phase = "Running"
    app.Status.AvailableReplicas = found.Status.AvailableReplicas
    if err := r.Status().Update(ctx, app); err != nil {
        return ctrl.Result{}, err
    }

    // 6. 返回结果
    return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
}

func (r *ApplicationReconciler) SetupWithManager(mgr ctrl.Manager) error {
    return ctrl.NewControllerManagedBy(mgr).
        For(&appv1.Application{}).
        Owns(&appsv1.Deployment{}).
        Complete(r)
}
```

<!-- chunk: Reconcile模式深度解析 -->
## Reconcile模式深度解析

### Reconcile触发模式对比

| 模式 | 说明 | 触发条件 | 适用场景 | 优缺点 |
|-----|------|---------|---------|--------|
| **Level-triggered** | 基于期望状态与实际状态的差值 | Watch事件/Requeue | 大多数场景（推荐默认） | ✅天然幂等 ❌可能多余计算 |
| **Edge-triggered** | 仅在状态变更边缘触发 | 事件过滤Predicate | 高频事件但少量需处理 | ✅减少无效调谐 ❌可能丢失状态 |
| **定时同步** | 周期性强制对账 | RequeueAfter定时器 | 外部资源同步/漂移检测 | ✅防漂移 ❌增加API压力 |
| **混合模式** | 事件驱动+周期性兜底 | Watch+定时器 | 生产级高可靠场景 | ✅最高可靠性 ❌复杂度高 |

### Level-triggered vs Edge-triggered

```
┌─────────────── Level-triggered (推荐) ───────────────┐
│                                                       │
│  期望状态(Spec) ─────┐                                │
│                       ├─→ Diff → Apply → 实际状态     │
│  实际状态(Status) ───┘                                │
│                                                       │
│  特点: 每次Reconcile都完整对比期望和实际状态           │
│  优势: 天然支持幂等，错过事件也能自愈                  │
│  劣势: 每次都需要完整的状态读取和比较                  │
└───────────────────────────────────────────────────────┘

┌─────────────── Edge-triggered ───────────────────────┐
│                                                       │
│  Event(Create/Update/Delete) → Filter → Reconcile     │
│                                                       │
│  特点: 仅在特定事件发生时触发Reconcile                │
│  优势: 减少不必要的处理，适合高频事件场景              │
│  劣势: 可能因事件丢失导致状态不一致                    │
│  补救: 必须配合定时Requeue兜底                         │
└───────────────────────────────────────────────────────┘
```

### Predicate事件过滤（Edge-triggered实现）

```go
// 自定义Predicate过滤器
func (r *ApplicationReconciler) SetupWithManager(mgr ctrl.Manager) error {
    return ctrl.NewControllerManagedBy(mgr).
        For(&appv1.Application{}, builder.WithPredicates(
            predicate.Or(
                predicate.GenerationChangedPredicate{}, // 仅Spec变更触发
                predicate.AnnotationChangedPredicate{}, // 注解变更触发
            ),
        )).
        Owns(&appsv1.Deployment{}, builder.WithPredicates(
            predicate.Funcs{
                CreateFunc: func(e event.CreateEvent) bool { return true },
                UpdateFunc: func(e event.UpdateEvent) bool {
                    // 仅在Ready副本数变化时触发
                    oldDep := e.ObjectOld.(*appsv1.Deployment)
                    newDep := e.ObjectNew.(*appsv1.Deployment)
                    return oldDep.Status.ReadyReplicas != newDep.Status.ReadyReplicas
                },
                DeleteFunc:  func(e event.DeleteEvent) bool { return true },
                GenericFunc: func(e event.GenericEvent) bool { return false },
            },
        )).
        // 监听外部资源变化（如ConfigMap变更触发关联CR重新调谐）
        Watches(
            &corev1.ConfigMap{},
            handler.EnqueueRequestsFromMapFunc(r.findApplicationsForConfigMap),
            builder.WithPredicates(predicate.ResourceVersionChangedPredicate{}),
        ).
        WithOptions(controller.Options{
            MaxConcurrentReconciles: 5,
            RateLimiter: workqueue.NewMaxOfRateLimiter(
                workqueue.NewItemExponentialFailureRateLimiter(200*time.Millisecond, 1000*time.Second),
                &workqueue.BucketRateLimiter{Limiter: rate.NewLimiter(rate.Limit(10), 100)},
            ),
        }).
        Complete(r)
}

// 跨资源关联查找
func (r *ApplicationReconciler) findApplicationsForConfigMap(
    ctx context.Context, obj client.Object,
) []reconcile.Request {
    configMap := obj.(*corev1.ConfigMap)
    var apps appv1.ApplicationList
    if err := r.List(ctx, &apps, client.InNamespace(configMap.Namespace),
        client.MatchingLabels{"config-ref": configMap.Name}); err != nil {
        return nil
    }
    requests := make([]reconcile.Request, len(apps.Items))
    for i, app := range apps.Items {
        requests[i] = reconcile.Request{
            NamespacedName: types.NamespacedName{Name: app.Name, Namespace: app.Namespace},
        }
    }
    return requests
}
```

### 定时同步与混合模式

```go
func (r *ApplicationReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    // ... 核心调谐逻辑 ...

    // 混合模式: 事件驱动 + 定时兜底
    // 正常情况下30秒后重新入队做漂移检测
    // 如果有错误，使用指数退避重试
    if reconcileErr != nil {
        // 错误重试: 不设置RequeueAfter，让RateLimiter控制退避间隔
        return ctrl.Result{}, reconcileErr
    }
    // 成功后定时重新对账（防止外部系统漂移）
    return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
}
```

<!-- chunk: Operator生产级最佳实践 -->
## Operator生产级最佳实践

### 核心实践速查表

| 实践 | 级别 | 说明 | 关键实现 |
|-----|------|------|----------|
| **幂等性** | 🔴必须 | Reconcile必须幂等，多次执行结果一致 | CreateOrUpdate / SSA |
| **状态管理** | 🔴必须 | 使用Status子资源，分离spec/status更新 | `r.Status().Update()` |
| **所有权管理** | 🔴必须 | 设置OwnerReferences实现级联删除 | `SetControllerReference()` |
| **[[Finalizers|Finalizers]]** | 🔴必须 | 删除前清理外部资源 | 添加/移除Finalizer |
| **条件状态** | 🔴必须 | 使用Conditions标准化状态报告 | `meta.SetStatusCondition()` |
| **事件记录** | 🟡推荐 | 发送K8s Events记录关键操作 | `recorder.Eventf()` |
| **重试策略** | 🟡推荐 | 指数退避重试失败操作 | RateLimiter配置 |
| **并发控制** | 🟡推荐 | 限制并发Reconcile数量 | `MaxConcurrentReconciles` |
| **监控指标** | 🟡推荐 | 暴露Prometheus自定义指标 | controller-runtime metrics |
| **优雅终止** | 🟡推荐 | Leader选举+优雅退出 | `LeaderElection: true` |
| **SSA补丁** | 🟢建议 | Server-Side Apply减少冲突 | `Patch(SSA)` |
| **缓存优化** | 🟢建议 | 按需缓存减少内存 | `cache.ByObject` |

### 1. 幂等性保证

```go
// ✅ 正确: 使用CreateOrUpdate保证幂等
func (r *ApplicationReconciler) reconcileDeployment(
    ctx context.Context, app *appv1.Application,
) error {
    deployment := &appsv1.Deployment{
        ObjectMeta: metav1.ObjectMeta{
            Name:      app.Name,
            Namespace: app.Namespace,
        },
    }
    
    op, err := controllerutil.CreateOrUpdate(ctx, r.Client, deployment, func() error {
        // mutate函数: 仅设置期望状态，不管当前状态
        deployment.Spec.Replicas = &app.Spec.Replicas
        deployment.Spec.Selector = &metav1.LabelSelector{
            MatchLabels: map[string]string{"app": app.Name},
        }
        deployment.Spec.Template = corev1.PodTemplateSpec{
            ObjectMeta: metav1.ObjectMeta{
                Labels: map[string]string{"app": app.Name},
            },
            Spec: corev1.PodSpec{
                Containers: []corev1.Container{{
                    Name:  "main",
                    Image: app.Spec.Image,
                }},
            },
        }
        return controllerutil.SetControllerReference(app, deployment, r.Scheme)
    })
    if err != nil {
        return fmt.Errorf("CreateOrUpdate Deployment failed: %w", err)
    }
    
    log.FromContext(ctx).Info("Deployment reconciled", "operation", op)
    return nil
}

// ✅ 推荐: 使用Server-Side Apply (SSA) 实现无冲突幂等
func (r *ApplicationReconciler) reconcileDeploymentSSA(
    ctx context.Context, app *appv1.Application,
) error {
    deployment := &appsv1.Deployment{
        TypeMeta: metav1.TypeMeta{APIVersion: "apps/v1", Kind: "Deployment"},
        ObjectMeta: metav1.ObjectMeta{
            Name:      app.Name,
            Namespace: app.Namespace,
        },
        Spec: appsv1.DeploymentSpec{
            Replicas: &app.Spec.Replicas,
            // ... 完整的期望状态
        },
    }
    // SSA: 由fieldManager标识的字段归该控制器管理
    return r.Patch(ctx, deployment, client.Apply,
        client.FieldOwner("application-controller"),
        client.ForceOwnership,
    )
}

// ❌ 错误: 非幂等的实现
// func reconcile() {
//     deployment.Spec.Replicas++ // 每次调用都递增，非幂等！
// }
```

### 2. 状态管理与Conditions

```go
import "k8s.io/apimachinery/pkg/api/meta"

// 标准化Condition更新
func (r *ApplicationReconciler) updateCondition(
    ctx context.Context, app *appv1.Application,
    condType string, status metav1.ConditionStatus,
    reason, message string,
) error {
    meta.SetStatusCondition(&app.Status.Conditions, metav1.Condition{
        Type:               condType,
        Status:             status,
        ObservedGeneration: app.Generation,  // 关键: 记录观察到的Generation
        Reason:             reason,
        Message:            message,
        LastTransitionTime: metav1.Now(),
    })
    return r.Status().Update(ctx, app)
}

// 完整的Reconcile状态管理流程
func (r *ApplicationReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    app := &appv1.Application{}
    if err := r.Get(ctx, req.NamespacedName, app); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }
    
    // 使用defer确保状态始终更新（即使出错）
    var reconcileErr error
    defer func() {
        if reconcileErr != nil {
            r.updateCondition(ctx, app, "Ready",
                metav1.ConditionFalse, "ReconcileFailed", reconcileErr.Error())
            r.updateCondition(ctx, app, "Progressing",
                metav1.ConditionFalse, "Error", reconcileErr.Error())
        } else {
            r.updateCondition(ctx, app, "Ready",
                metav1.ConditionTrue, "ReconcileSuccess", "All resources synced")
            r.updateCondition(ctx, app, "Progressing",
                metav1.ConditionFalse, "Synced", "Desired state achieved")
        }
    }()
    
    // 核心调谐逻辑...
    reconcileErr = r.reconcileResources(ctx, app)
    if reconcileErr != nil {
        return ctrl.Result{}, reconcileErr
    }
    return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
}
```

### 3. Finalizer生产级实现

```go
const applicationFinalizer = "app.example.com/cleanup"

func (r *ApplicationReconciler) handleFinalizerAndDeletion(
    ctx context.Context, app *appv1.Application,
) (ctrl.Result, bool, error) {
    logger := log.FromContext(ctx)
    
    // 资源正在被删除
    if !app.DeletionTimestamp.IsZero() {
        if controllerutil.ContainsFinalizer(app, applicationFinalizer) {
            logger.Info("Executing finalizer cleanup")
            
            // 设置状态为Terminating
            r.updateCondition(ctx, app, "Ready",
                metav1.ConditionFalse, "Terminating", "Cleaning up external resources")
            
            // 清理外部资源（带超时保护）
            cleanupCtx, cancel := context.WithTimeout(ctx, 2*time.Minute)
            defer cancel()
            
            if err := r.cleanupExternalResources(cleanupCtx, app); err != nil {
                logger.Error(err, "Cleanup failed, will retry")
                r.Recorder.Eventf(app, corev1.EventTypeWarning,
                    "CleanupFailed", "External resource cleanup failed: %v", err)
                // 返回错误触发重试，但不要无限重试
                return ctrl.Result{RequeueAfter: 10 * time.Second}, true, err
            }
            
            r.Recorder.Event(app, corev1.EventTypeNormal,
                "CleanupComplete", "External resources cleaned up")
            
            // 移除Finalizer
            controllerutil.RemoveFinalizer(app, applicationFinalizer)
            if err := r.Update(ctx, app); err != nil {
                return ctrl.Result{}, true, err
            }
        }
        return ctrl.Result{}, true, nil // isDeleting=true
    }
    
    // 确保Finalizer存在
    if !controllerutil.ContainsFinalizer(app, applicationFinalizer) {
        controllerutil.AddFinalizer(app, applicationFinalizer)
        if err := r.Update(ctx, app); err != nil {
            return ctrl.Result{}, false, err
        }
    }
    
    return ctrl.Result{}, false, nil // isDeleting=false
}

// 外部资源清理（示例：删除云资源、DNS记录等）
func (r *ApplicationReconciler) cleanupExternalResources(
    ctx context.Context, app *appv1.Application,
) error {
    // 1. 删除外部负载均衡器
    if err := r.ExternalLBClient.Delete(ctx, app.Status.ExternalLBID); err != nil {
        return fmt.Errorf("delete external LB: %w", err)
    }
    // 2. 清理DNS记录
    if err := r.DNSClient.DeleteRecord(ctx, app.Spec.Domain); err != nil {
        return fmt.Errorf("delete DNS record: %w", err)
    }
    // 3. 释放IP地址
    if err := r.IPAMClient.Release(ctx, app.Status.AllocatedIP); err != nil {
        return fmt.Errorf("release IP: %w", err)
    }
    return nil
}
```

### 4. 事件记录规范

```go
type ApplicationReconciler struct {
    client.Client
    Scheme   *runtime.Scheme
    Recorder record.EventRecorder  // 事件记录器
}

// 事件记录最佳实践
func (r *ApplicationReconciler) reconcileWithEvents(
    ctx context.Context, app *appv1.Application,
) error {
    // ✅ 记录正常操作
    r.Recorder.Event(app, corev1.EventTypeNormal,
        "Reconciling", "Starting reconciliation")
    
    // ✅ 记录重要状态变更
    if oldReplicas != app.Spec.Replicas {
        r.Recorder.Eventf(app, corev1.EventTypeNormal,
            "ScalingDeployment", "Scaling from %d to %d replicas",
            oldReplicas, app.Spec.Replicas)
    }
    
    // ✅ 记录警告事件
    if app.Spec.Replicas > 50 {
        r.Recorder.Eventf(app, corev1.EventTypeWarning,
            "HighReplicaCount", "Replica count %d exceeds recommended maximum 50",
            app.Spec.Replicas)
    }
    
    // ✅ 记录错误事件
    if err := r.reconcileDeployment(ctx, app); err != nil {
        r.Recorder.Eventf(app, corev1.EventTypeWarning,
            "ReconcileFailed", "Failed to reconcile Deployment: %v", err)
        return err
    }
    
    return nil
}

// SetupWithManager中注册EventRecorder
func (r *ApplicationReconciler) SetupWithManager(mgr ctrl.Manager) error {
    r.Recorder = mgr.GetEventRecorderFor("application-controller")
    return ctrl.NewControllerManagedBy(mgr).
        For(&appv1.Application{}).
        Owns(&appsv1.Deployment{}).
        Complete(r)
}
```

### 5. 重试策略与速率限制

| 策略 | 配置 | 说明 |
|-----|------|------|
| 指数退避 | `base=200ms, max=1000s` | 失败重试间隔指数增长 |
| 令牌桶 | `rate=10/s, burst=100` | 全局限制Reconcile速率 |
| 每对象退避 | `per-item failure tracker` | 单个对象失败不影响其他 |
| Requeue延迟 | `RequeueAfter=30s` | 成功后定时重新对账 |

```go
// 生产级RateLimiter配置
import (
    "golang.org/x/time/rate"
    "k8s.io/client-go/util/workqueue"
)

func rateLimiter() workqueue.RateLimiter {
    return workqueue.NewMaxOfRateLimiter(
        // 每个对象的指数退避: 200ms → 400ms → 800ms → ... → 最大1000s
        workqueue.NewItemExponentialFailureRateLimiter(200*time.Millisecond, 1000*time.Second),
        // 全局速率限制: 10 QPS, burst 100
        &workqueue.BucketRateLimiter{Limiter: rate.NewLimiter(rate.Limit(10), 100)},
    )
}

// Reconcile中的重试返回策略
func (r *ApplicationReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    // 暂时性错误 → 返回error让RateLimiter处理退避
    if isTransientError(err) {
        return ctrl.Result{}, err  // 自动指数退避
    }
    // 需要等待的场景 → 固定延迟重试
    if isWaitingForDependency(app) {
        return ctrl.Result{RequeueAfter: 15 * time.Second}, nil
    }
    // 永久性错误 → 不重试，仅更新状态
    if isPermanentError(err) {
        r.updateCondition(ctx, app, "Ready", metav1.ConditionFalse, "PermanentError", err.Error())
        return ctrl.Result{}, nil  // 不返回error，不重试
    }
    // 成功 → 定时重新对账
    return ctrl.Result{RequeueAfter: 5 * time.Minute}, nil
}
```

### 6. 监控指标暴露

```go
import (
    "github.com/prometheus/client_golang/prometheus"
    "sigs.k8s.io/controller-runtime/pkg/metrics"
)

var (
    reconcileTotal = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "application_reconcile_total",
            Help: "Total number of reconciliations per controller",
        },
        []string{"controller", "result"},  // result: success/error/requeue
    )
    reconcileDuration = prometheus.NewHistogramVec(
        prometheus.HistogramOpts{
            Name:    "application_reconcile_duration_seconds",
            Help:    "Duration of reconciliation per controller",
            Buckets: []float64{0.01, 0.05, 0.1, 0.5, 1, 5, 10, 30},
        },
        []string{"controller"},
    )
    resourceCount = prometheus.NewGaugeVec(
        prometheus.GaugeOpts{
            Name: "application_managed_resources",
            Help: "Number of managed Application resources",
        },
        []string{"namespace", "phase"},
    )
)

func init() {
    metrics.Registry.MustRegister(reconcileTotal, reconcileDuration, resourceCount)
}

// 在Reconcile中记录指标
func (r *ApplicationReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    startTime := time.Now()
    defer func() {
        reconcileDuration.WithLabelValues("application").Observe(time.Since(startTime).Seconds())
    }()
    
    result, err := r.doReconcile(ctx, req)
    if err != nil {
        reconcileTotal.WithLabelValues("application", "error").Inc()
    } else if result.Requeue || result.RequeueAfter > 0 {
        reconcileTotal.WithLabelValues("application", "requeue").Inc()
    } else {
        reconcileTotal.WithLabelValues("application", "success").Inc()
    }
    return result, err
}
```

<!-- chunk: Operator最佳实践速查 -->
## Operator最佳实践速查

| 实践 | 说明 | 示例 |
|-----|------|------|
| **幂等性** | Reconcile必须幂等 | 使用CreateOrUpdate/SSA |
| **状态管理** | 使用Status子资源 | 分离spec和status更新 |
| **所有权** | 设置OwnerReferences | 级联删除子资源 |
| **事件记录** | 发送K8s Events | 记录关键操作 |
| **重试策略** | 指数退避重试 | RateLimiter配置 |
| **资源限制** | 设置并发和速率限制 | MaxConcurrentReconciles |
| **监控指标** | 暴露Prometheus指标 | controller-runtime metrics |
| **优雅终止** | 处理终止信号 | LeaderElection graceful |
| **Finalizers** | 清理外部资源 | 删除前执行清理 |
| **条件状态** | 使用Conditions | 标准化状态报告 |

<!-- chunk: Controller配置 -->
## Controller配置

```go
// main.go
func main() {
    mgr, err := ctrl.NewManager(ctrl.GetConfigOrDie(), ctrl.Options{
        Scheme:                 scheme,
        MetricsBindAddress:     ":8080",
        HealthProbeBindAddress: ":8081",
        LeaderElection:         true,
        LeaderElectionID:       "app-operator.example.com",
        // 并发控制
        Controller: config.Controller{
            GroupKindConcurrency: map[string]int{
                "Application.app.example.com": 10,  // 最多10个并发
            },
        },
    })

    if err := (&controller.ApplicationReconciler{
        Client: mgr.GetClient(),
        Scheme: mgr.GetScheme(),
    }).SetupWithManager(mgr); err != nil {
        setupLog.Error(err, "unable to create controller")
        os.Exit(1)
    }
}
```

<!-- chunk: Webhook开发 -->
## Webhook开发

```go
// api/v1/application_webhook.go

// +kubebuilder:webhook:path=/mutate-app-example-com-v1-application,mutating=true,failurePolicy=fail,sideEffects=None,groups=app.example.com,resources=applications,verbs=create;update,versions=v1,name=mapplication.kb.io,admissionReviewVersions=v1

var _ webhook.Defaulter = &Application{}

func (r *Application) Default() {
    if r.Spec.Replicas == 0 {
        r.Spec.Replicas = 1
    }
}

// +kubebuilder:webhook:path=/validate-app-example-com-v1-application,mutating=false,failurePolicy=fail,sideEffects=None,groups=app.example.com,resources=applications,verbs=create;update,versions=v1,name=vapplication.kb.io,admissionReviewVersions=v1

var _ webhook.Validator = &Application{}

func (r *Application) ValidateCreate() (admission.Warnings, error) {
    if r.Spec.Replicas > 100 {
        return nil, fmt.Errorf("replicas cannot exceed 100")
    }
    return nil, nil
}

func (r *Application) ValidateUpdate(old runtime.Object) (admission.Warnings, error) {
    oldApp := old.(*Application)
    if r.Spec.Image != oldApp.Spec.Image {
        // 记录镜像变更
    }
    return r.ValidateCreate()
}

func (r *Application) ValidateDelete() (admission.Warnings, error) {
    return nil, nil
}
```

<!-- chunk: Operator测试 -->
## Operator测试

```go
// internal/controller/application_controller_test.go
var _ = Describe("Application Controller", func() {
    Context("When reconciling a resource", func() {
        const resourceName = "test-application"

        ctx := context.Background()

        typeNamespacedName := types.NamespacedName{
            Name:      resourceName,
            Namespace: "default",
        }
        application := &appv1.Application{}

        BeforeEach(func() {
            By("creating the custom resource")
            err := k8sClient.Get(ctx, typeNamespacedName, application)
            if err != nil && errors.IsNotFound(err) {
                resource := &appv1.Application{
                    ObjectMeta: metav1.ObjectMeta{
                        Name:      resourceName,
                        Namespace: "default",
                    },
                    Spec: appv1.ApplicationSpec{
                        Image:    "nginx:1.25",
                        Replicas: 3,
                    },
                }
                Expect(k8sClient.Create(ctx, resource)).To(Succeed())
            }
        })

        It("should create Deployment", func() {
            By("Reconciling the created resource")
            controllerReconciler := &ApplicationReconciler{
                Client: k8sClient,
                Scheme: k8sClient.Scheme(),
            }

            _, err := controllerReconciler.Reconcile(ctx, reconcile.Request{
                NamespacedName: typeNamespacedName,
            })
            Expect(err).NotTo(HaveOccurred())

            deployment := &appsv1.Deployment{}
            Eventually(func() error {
                return k8sClient.Get(ctx, typeNamespacedName, deployment)
            }).Should(Succeed())
            Expect(*deployment.Spec.Replicas).To(Equal(int32(3)))
        })
    })
})
```

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 | 影响 |
|------|---------|------|
| v1.25 | CRD验证规则CEL支持GA | 内置复杂验证 |
| v1.26 | SelectableFields Alpha | 自定义字段选择器 |
| v1.27 | CRD验证Ratcheting Beta | 渐进式验证 |
| v1.28 | ValidatingAdmissionPolicy CRD集成 | 简化webhook |
| v1.29 | CRD SelectableFields Beta | 字段选择更稳定 |
| v1.30 | CEL cost估算改进 | 性能优化 |
| v1.31 | CRD元数据验证增强 | 更严格的验证 |
| v1.32 | SelectableFields GA | 生产可用 |

---

**Operator开发原则**: 幂等Reconcile + OwnerReference级联删除 + Finalizer清理外部资源 + Status子资源更新状态 + 完善的测试覆盖

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 平台工程 MOC
- [[10-平台工程/README.md|Platform Ops Domain (平台运维领域)]]
- Domain-9 平台运维 — 开源项目索引
- 平台运维概述
- 集群生命周期管理
- 容量规划与资源评估 (Capacity Planning & Resource Assessment)
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)

## See Also

- 18-platform-observability-practice
- 19-lease-leader-election
- 21-api-aggregation
- 22-client-libraries


<!-- risk-assessed -->
