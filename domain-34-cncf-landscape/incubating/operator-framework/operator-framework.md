# Operator Framework

> **成熟度**: Incubating | **加入时间**: 2019-07 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://operatorframework.io |
| **GitHub** | https://github.com/operator-framework |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Image Build |

---

## 项目概述

Operator Framework 是一个开源工具包，用于以高效、自动化和可扩展的方式管理 Kubernetes 原生应用（Operators）。它提供了构建、测试和分发 Operators 的完整解决方案。

## 核心特性

- **Operator SDK**: 快速构建 Operators 的开发框架
- **Operator Lifecycle Manager (OLM)**: Operator 安装、升级、RBAC 管理
- **OperatorHub**: Operator 发现和分发平台
- **多语言支持**: Go、Ansible、Helm 三种构建方式
- **成熟度模型**: 5 级能力模型指导 Operator 开发
- **测试框架**: 内置单元测试和 E2E 测试支持

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                   Operator Framework 生态系统                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Operator SDK                          │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────┐   │    │
│  │  │  Go-based   │ │   Ansible   │ │   Helm-based    │   │    │
│  │  │  Operator   │ │  Operator   │ │    Operator     │   │    │
│  │  └─────────────┘ └─────────────┘ └─────────────────┘   │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│                         creates                                  │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Operator Bundle                       │    │
│  │  ┌──────────┐  ┌────────────┐  ┌───────────────────┐   │    │
│  │  │   CRDs   │  │ ClusterSvc │  │  CSV (Metadata)   │   │    │
│  │  │          │  │ Version    │  │                   │   │    │
│  │  └──────────┘  └────────────┘  └───────────────────┘   │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│                        manages                                   │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │            Operator Lifecycle Manager (OLM)              │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────┐    │    │
│  │  │   Catalog    │  │ Subscription │  │ InstallPlan│    │    │
│  │  │   Source     │  │              │  │            │    │    │
│  │  └──────────────┘  └──────────────┘  └────────────┘    │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    OperatorHub.io                        │    │
│  │       公共 Operator 注册中心 / 企业私有 Catalog            │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Operator 成熟度模型

```
Level 5: Auto Pilot         ──── 自动扩缩容、自动调优、异常自愈
Level 4: Deep Insights      ──── 指标、告警、日志分析、容量规划
Level 3: Full Lifecycle     ──── 备份、恢复、故障转移
Level 2: Seamless Upgrades  ──── 版本升级、配置更新、补丁
Level 1: Basic Install      ──── 自动化部署、基本配置
```

---

## 快速开始

### 安装 Operator SDK

```bash
# macOS
brew install operator-sdk

# Linux
export ARCH=$(case $(uname -m) in x86_64) echo -n amd64 ;; aarch64) echo -n arm64 ;; esac)
export OS=$(uname | awk '{print tolower($0)}')
export OPERATOR_SDK_DL_URL=https://github.com/operator-framework/operator-sdk/releases/download/v1.33.0
curl -LO ${OPERATOR_SDK_DL_URL}/operator-sdk_${OS}_${ARCH}
chmod +x operator-sdk_${OS}_${ARCH}
sudo mv operator-sdk_${OS}_${ARCH} /usr/local/bin/operator-sdk
```

### 创建 Go-based Operator

```bash
# 初始化项目
mkdir memcached-operator && cd memcached-operator
operator-sdk init --domain=example.com --repo=github.com/example/memcached-operator

# 创建 API 和 Controller
operator-sdk create api --group cache --version v1alpha1 --kind Memcached --resource --controller

# 项目结构
.
├── api/v1alpha1/          # CRD 定义
│   └── memcached_types.go
├── controllers/           # Reconcile 逻辑
│   └── memcached_controller.go
├── config/
│   ├── crd/              # CRD manifests
│   ├── rbac/             # RBAC 配置
│   └── manager/          # Deployment
├── Dockerfile
└── main.go
```

### 定义 CRD

```go
// api/v1alpha1/memcached_types.go
package v1alpha1

import (
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// MemcachedSpec defines the desired state
type MemcachedSpec struct {
    // Size defines the number of Memcached instances
    // +kubebuilder:validation:Minimum=1
    // +kubebuilder:validation:Maximum=10
    Size int32 `json:"size"`
    
    // ContainerPort defines the port
    // +kubebuilder:default=11211
    ContainerPort int32 `json:"containerPort,omitempty"`
}

// MemcachedStatus defines the observed state
type MemcachedStatus struct {
    // Nodes are the names of the memcached pods
    Nodes []string `json:"nodes,omitempty"`
    
    // Conditions represent the latest observations
    Conditions []metav1.Condition `json:"conditions,omitempty"`
}

// +kubebuilder:object:root=true
// +kubebuilder:subresource:status
// +kubebuilder:printcolumn:name="Size",type="integer",JSONPath=".spec.size"
// +kubebuilder:printcolumn:name="Age",type="date",JSONPath=".metadata.creationTimestamp"
type Memcached struct {
    metav1.TypeMeta   `json:",inline"`
    metav1.ObjectMeta `json:"metadata,omitempty"`

    Spec   MemcachedSpec   `json:"spec,omitempty"`
    Status MemcachedStatus `json:"status,omitempty"`
}
```

### 实现 Controller

```go
// controllers/memcached_controller.go
package controllers

import (
    "context"
    "reflect"
    
    appsv1 "k8s.io/api/apps/v1"
    corev1 "k8s.io/api/core/v1"
    "k8s.io/apimachinery/pkg/api/errors"
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/apimachinery/pkg/runtime"
    "k8s.io/apimachinery/pkg/types"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "sigs.k8s.io/controller-runtime/pkg/log"
    
    cachev1alpha1 "github.com/example/memcached-operator/api/v1alpha1"
)

type MemcachedReconciler struct {
    client.Client
    Scheme *runtime.Scheme
}

// +kubebuilder:rbac:groups=cache.example.com,resources=memcacheds,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=cache.example.com,resources=memcacheds/status,verbs=get;update;patch
// +kubebuilder:rbac:groups=apps,resources=deployments,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=core,resources=pods,verbs=get;list;watch

func (r *MemcachedReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    log := log.FromContext(ctx)
    
    // Fetch the Memcached instance
    memcached := &cachev1alpha1.Memcached{}
    err := r.Get(ctx, req.NamespacedName, memcached)
    if err != nil {
        if errors.IsNotFound(err) {
            return ctrl.Result{}, nil
        }
        return ctrl.Result{}, err
    }
    
    // Check if deployment exists
    found := &appsv1.Deployment{}
    err = r.Get(ctx, types.NamespacedName{
        Name:      memcached.Name,
        Namespace: memcached.Namespace,
    }, found)
    
    if err != nil && errors.IsNotFound(err) {
        // Create deployment
        dep := r.deploymentForMemcached(memcached)
        log.Info("Creating Deployment", "Name", dep.Name)
        err = r.Create(ctx, dep)
        if err != nil {
            return ctrl.Result{}, err
        }
        return ctrl.Result{Requeue: true}, nil
    }
    
    // Ensure replica count matches spec
    size := memcached.Spec.Size
    if *found.Spec.Replicas != size {
        found.Spec.Replicas = &size
        err = r.Update(ctx, found)
        if err != nil {
            return ctrl.Result{}, err
        }
        return ctrl.Result{Requeue: true}, nil
    }
    
    // Update status
    podList := &corev1.PodList{}
    listOpts := []client.ListOption{
        client.InNamespace(memcached.Namespace),
        client.MatchingLabels(labelsForMemcached(memcached.Name)),
    }
    if err = r.List(ctx, podList, listOpts...); err != nil {
        return ctrl.Result{}, err
    }
    podNames := getPodNames(podList.Items)
    
    if !reflect.DeepEqual(podNames, memcached.Status.Nodes) {
        memcached.Status.Nodes = podNames
        err := r.Status().Update(ctx, memcached)
        if err != nil {
            return ctrl.Result{}, err
        }
    }
    
    return ctrl.Result{}, nil
}

func (r *MemcachedReconciler) SetupWithManager(mgr ctrl.Manager) error {
    return ctrl.NewControllerManagedBy(mgr).
        For(&cachev1alpha1.Memcached{}).
        Owns(&appsv1.Deployment{}).
        Complete(r)
}
```

---

## OLM 资源

### CatalogSource

```yaml
apiVersion: operators.coreos.com/v1alpha1
kind: CatalogSource
metadata:
  name: my-operators
  namespace: olm
spec:
  sourceType: grpc
  image: example.com/my-operator-index:latest
  displayName: My Operators
  updateStrategy:
    registryPoll:
      interval: 10m
```

### Subscription

```yaml
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: memcached-operator
  namespace: operators
spec:
  channel: stable
  name: memcached-operator
  source: my-operators
  sourceNamespace: olm
  installPlanApproval: Automatic
```

---

## 测试

```bash
# 单元测试
make test

# E2E 测试 (使用 envtest)
make test-e2e

# Scorecard 测试
operator-sdk scorecard bundle --wait-time 60s
```

---

## 最佳实践

1. **Finalizers**: 使用 Finalizers 处理资源清理
2. **Status Conditions**: 遵循 Kubernetes 条件约定
3. **Owner References**: 设置正确的所有者引用
4. **幂等性**: Reconcile 函数必须幂等
5. **错误处理**: 合理使用 Requeue 和错误返回
6. **监控**: 暴露 Prometheus 指标

---

## 参考资源

- [官方文档](https://operatorframework.io/docs)
- [GitHub Repo](https://github.com/operator-framework)
- [Operator SDK 文档](https://sdk.operatorframework.io/)
- [OLM 文档](https://olm.operatorframework.io/)
- [OperatorHub.io](https://operatorhub.io/)

---

**维护者**: Kudig Team | **许可证**: MIT
