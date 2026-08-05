---
title: Kubernetes Operator Development with Kubebuilder
description: K8s Operator 开发深度实践 — Kubebuilder 框架、CRD 设计、Reconcile 循环、Webhook、测试与发布
summary: 从零构建生产级 Kubernetes Operator，涵盖设计模式、代码实现、测试策略、运维实践
category: practice
tags:
- operator
- kubebuilder
- crd
- controller
- reconcile
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: manifests
---
# Kubernetes Operator 开发（Kubebuilder）

> 构建生产级 Kubernetes Operator 的完整实践指南。

## Operator 设计原则

| 原则 | 说明 |
|------|------|
| 声明式 | 用户描述期望状态，Operator 负责收敛 |
| 幂等 | Reconcile 可安全重复执行 |
| 最终一致 | 允许中间状态，最终达到期望 |
| 可观测 | 暴露指标、事件、状态 |
| 可升级 | CRD 版本演进、向后兼容 |

## 项目初始化

```bash
# 安装 Kubebuilder
curl -L -o kubebuilder "https://go.kubebuilder.io/dl/latest/$(go env GOOS)/$(go env GOARCH)"

# 初始化项目
mkdir my-operator && cd my-operator
kubebuilder init --domain example.com --repo github.com/org/my-operator

# 创建 API
kubebuilder create api --group apps --version v1alpha1 --kind MyApp
kubebuilder create webhook --group apps --version v1alpha1 --kind MyApp --defaulting --programmatic-validation
```

## CRD 设计

### API 类型定义

```go
// api/v1alpha1/myapp_types.go
package v1alpha1

import (
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// MyAppSpec 定义期望状态
type MyAppSpec struct {
    // 副本数
    // +kubebuilder:validation:Minimum=1
    // +kubebuilder:validation:Maximum=100
    // +kubebuilder:default=3
    Replicas int32 `json:"replicas"`

    // 镜像
    // +kubebuilder:validation:Required
    Image string `json:"image"`

    // 资源请求
    Resources ResourceRequirements `json:"resources,omitempty"`

    // 数据库配置
    Database DatabaseConfig `json:"database,omitempty"`

    // 自动缩放
    Autoscaling *AutoscalingSpec `json:"autoscaling,omitempty"`
}

type DatabaseConfig struct {
    // +kubebuilder:validation:Enum=postgresql;mysql;mongodb
    Engine string `json:"engine"`
    Size   string `json:"size"`
}

type AutoscalingSpec struct {
    Enabled     bool  `json:"enabled"`
    MinReplicas int32 `json:"minReplicas"`
    MaxReplicas int32 `json:"maxReplicas"`
    TargetCPU   int32 `json:"targetCPU"`
}

// MyAppStatus 定义观测状态
type MyAppStatus struct {
    // +kubebuilder:default="Pending"
    Phase string `json:"phase,omitempty"`

    ReadyReplicas int32 `json:"readyReplicas"`

    // 条件列表（标准模式）
    Conditions []metav1.Condition `json:"conditions,omitempty"`

    // 最后协调时间
    LastReconcileTime *metav1.Time `json:"lastReconcileTime,omitempty"`

    // 观测到的 Generation
    ObservedGeneration int64 `json:"observedGeneration,omitempty"`
}

// +kubebuilder:object:root=true
// +kubebuilder:subresource:status
// +kubebuilder:subresource:scale:specpath=.spec.replicas,statuspath=.status.readyReplicas
// +kubebuilder:printcolumn:name="Phase",type=string,JSONPath=`.status.phase`
// +kubebuilder:printcolumn:name="Ready",type=integer,JSONPath=`.status.readyReplicas`
// +kubebuilder:printcolumn:name="Age",type=date,JSONPath=`.metadata.creationTimestamp`
type MyApp struct {
    metav1.TypeMeta   `json:",inline"`
    metav1.ObjectMeta `json:"metadata,omitempty"`
    Spec   MyAppSpec   `json:"spec,omitempty"`
    Status MyAppStatus `json:"status,omitempty"`
}
```

## Reconcile 循环

```go
// internal/controller/myapp_controller.go
func (r *MyAppReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    log := r.Log.WithValues("myapp", req.NamespacedName)

    // 1. 获取 CR
    var app v1alpha1.MyApp
    if err := r.Get(ctx, req.NamespacedName, &app); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }

    // 2. 处理删除（Finalizer）
    if !app.DeletionTimestamp.IsZero() {
        return r.handleDeletion(ctx, &app)
    }

    // 3. 确保 Finalizer
    if !controllerutil.ContainsFinalizer(&app, finalizerName) {
        controllerutil.AddFinalizer(&app, finalizerName)
        return ctrl.Result{}, r.Update(ctx, &app)
    }

    // 4. 协调 Deployment
    if err := r.reconcileDeployment(ctx, &app); err != nil {
        r.setCondition(&app, "DeploymentReady", metav1.ConditionFalse, "ReconcileError", err.Error())
        return ctrl.Result{}, err
    }

    // 5. 协调 Service
    if err := r.reconcileService(ctx, &app); err != nil {
        return ctrl.Result{}, err
    }

    // 6. 协调 HPA（如果启用自动缩放）
    if app.Spec.Autoscaling != nil && app.Spec.Autoscaling.Enabled {
        if err := r.reconcileHPA(ctx, &app); err != nil {
            return ctrl.Result{}, err
        }
    }

    // 7. 更新状态
    app.Status.Phase = "Running"
    app.Status.ObservedGeneration = app.Generation
    app.Status.LastReconcileTime = &metav1.Time{Time: time.Now()}
    r.setCondition(&app, "Ready", metav1.ConditionTrue, "AllResourcesReady", "")

    if err := r.Status().Update(ctx, &app); err != nil {
        return ctrl.Result{}, err
    }

    // 8. 重新入队（定期检查）
    return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
}

func (r *MyAppReconciler) reconcileDeployment(ctx context.Context, app *v1alpha1.MyApp) error {
    desired := &appsv1.Deployment{
        ObjectMeta: metav1.ObjectMeta{
            Name:      app.Name,
            Namespace: app.Namespace,
            Labels:    app.Labels,
        },
        Spec: appsv1.DeploymentSpec{
            Replicas: &app.Spec.Replicas,
            Selector: &metav1.LabelSelector{
                MatchLabels: map[string]string{"app": app.Name},
            },
            Template: corev1.PodTemplateSpec{
                ObjectMeta: metav1.ObjectMeta{
                    Labels: map[string]string{"app": app.Name},
                },
                Spec: corev1.PodSpec{
                    Containers: []corev1.Container{{
                        Name:  "app",
                        Image: app.Spec.Image,
                    }},
                },
            },
        },
    }

    // 设置 Owner Reference
    if err := controllerutil.SetControllerReference(app, desired, r.Scheme); err != nil {
        return err
    }

    // Create or Update
    var existing appsv1.Deployment
    err := r.Get(ctx, client.ObjectKeyFromObject(desired), &existing)
    if errors.IsNotFound(err) {
        return r.Create(ctx, desired)
    }
    if err != nil {
        return err
    }

    // 更新（保留不可变字段）
    existing.Spec.Replicas = desired.Spec.Replicas
    existing.Spec.Template = desired.Spec.Template
    return r.Update(ctx, &existing)
}
```

## Admission Webhook

```go
// internal/webhook/v1alpha1/myapp_webhook.go

// +kubebuilder:webhook:path=/mutate-apps-example-com-v1alpha1-myapp,mutating=true,failurePolicy=fail,sideEffects=None,groups=apps.example.com,resources=myapps,verbs=create;update,versions=v1alpha1,name=mmyapp.kb.io,admissionReviewVersions=v1

func (w *MyAppWebhook) Default(ctx context.Context, obj runtime.Object) error {
    app, ok := obj.(*v1alpha1.MyApp)
    if !ok {
        return fmt.Errorf("expected MyApp, got %T", obj)
    }
    // 默认值
    if app.Spec.Replicas == 0 {
        app.Spec.Replicas = 3
    }
    return nil
}

// +kubebuilder:webhook:path=/validate-apps-example-com-v1alpha1-myapp,mutating=false,failurePolicy=fail,sideEffects=None,groups=apps.example.com,resources=myapps,verbs=create;update,versions=v1alpha1,name=vmyapp.kb.io,admissionReviewVersions=v1

func (w *MyAppWebhook) ValidateCreate(ctx context.Context, obj runtime.Object) (admission.Warnings, error) {
    app := obj.(*v1alpha1.MyApp)
    var errs field.ErrorList

    if app.Spec.Replicas > 50 && app.Spec.Resources.CPU < "2" {
        errs = append(errs, field.Forbidden(
            field.NewPath("spec", "replicas"),
            "high replicas require adequate resources",
        ))
    }

    if len(errs) > 0 {
        return nil, apierrors.NewInvalid(v1alpha1.GroupVersion.WithKind("MyApp").GroupKind(), app.Name, errs)
    }
    return nil, nil
}
```

## 测试策略

### 单元测试（envtest）

```go
func TestMyAppReconciler(t *testing.T) {
    testEnv := &envtest.Environment{
        CRDDirectoryPaths: []string{"../config/crd/bases"},
        WebhookInstallOptions: envtest.WebhookInstallOptions{
            Paths: []string{"../config/webhook"},
        },
    }
    cfg, err := testEnv.Start()
    require.NoError(t, err)
    defer testEnv.Stop()

    k8sClient, _ := client.New(cfg, client.Options{})
    reconciler := &MyAppReconciler{Client: k8sClient, Scheme: scheme}

    // 创建测试 CR
    app := &v1alpha1.MyApp{
        ObjectMeta: metav1.ObjectMeta{Name: "test", Namespace: "default"},
        Spec: v1alpha1.MyAppSpec{Replicas: 3, Image: "nginx:latest"},
    }
    require.NoError(t, k8sClient.Create(ctx, app))

    // 执行 Reconcile
    result, err := reconciler.Reconcile(ctx, ctrl.Request{
        NamespacedName: types.NamespacedName{Name: "test", Namespace: "default"},
    })
    require.NoError(t, err)
    assert.Equal(t, ctrl.Result{RequeueAfter: 30 * time.Second}, result)

    // 验证 Deployment 创建
    var deploy appsv1.Deployment
    require.NoError(t, k8sClient.Get(ctx, types.NamespacedName{Name: "test", Namespace: "default"}, &deploy))
    assert.Equal(t, int32(3), *deploy.Spec.Replicas)
}
```

## 发布与运维

### 多阶段构建 Dockerfile

```dockerfile
FROM golang:1.22 AS builder
WORKDIR /workspace
COPY go.mod go.sum ./
RUN go mod download
COPY . .
RUN CGO_ENABLED=0 GOOS=linux go build -a -o manager cmd/main.go

FROM gcr.io/distroless/static:nonroot
WORKDIR /
COPY --from=builder /workspace/manager .
USER 65532:65532
ENTRYPOINT ["/manager"]
```

### 生产部署检查清单

- [ ] Leader Election 启用（多副本安全）
- [ ] 健康检查端点（/healthz, /readyz）
- [ ] Prometheus 指标暴露（/metrics）
- [ ] 资源限制设置（避免 OOM）
- [ ] PodDisruptionBudget 配置
- [ ] CRD 版本策略（v1alpha1 → v1beta1 → v1）
- [ ] 转换 Webhook（多版本共存）
- [ ] 优雅关闭（处理 SIGTERM）

## Related

- [[03-清单模式/index.md|清单模式总索引]]
- [[03-清单模式/02-Kustomize模式/index.md|Kustomize 模式]]
- [[10-平台工程/index.md|平台工程]]
