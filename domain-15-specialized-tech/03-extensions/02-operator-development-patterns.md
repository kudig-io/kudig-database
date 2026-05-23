---
title: 02 - Operator开发模式与控制器实现
description: '## Operator核心架构模式'
category: extensions
tags:
- k8s
- extensions
- crd
- operator
- webhook
- controller-manager
- prometheus
- docker
- opa
- mysql
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 开发工程师
- 架构师
estimated_read_time: 5min
intent_queries:
- Operator开发模式与控制器实现 是什么
- 如何 Operator开发模式与控制器实现
- Kubernetes 10 extensions 最佳实践
trigger_keywords:
- Operator开发模式与控制器实现
- extensions
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- gitops-basics
- mysql-basics
- tls-basics
- policy-basics
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
  path: ../domain-07-platform-engineering/
  label: '相关知识域: domain-07-platform-engineering'
created: "2026-05-23"
---

# 02 - Operator开发模式与控制器实现

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [operatorframework.io](https://operatorframework.io/) | [book.kubebuilder.io](https://book.kubebuilder.io/)

<!-- chunk: Operator核心架构模式 -->
## Operator核心架构模式

### 控制器模式原理

```
┌─────────────────────────────────────────────────────────────────┐
│                     Reconcile循环                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────────┐    │
│  │  观察变化    │───▶│  计算期望状态  │───▶│  执行变更操作    │   │
│  │ (Watch/Informer)│  (Reconcile)   │    │ (Client CRUD)   │   │
│  └─────────────┘    └──────────────┘    └─────────────────┘    │
│         │                    │                     │             │
│         ▼                    ▼                     ▼             │
│  ┌─────────────┐    ┌──────────────┐    ┌─────────────────┐    │
│  │ 事件队列     │    │  期望状态     │    │  实际状态        │   │
│  │ (WorkQueue) │    │ (Spec)       │    │ (Status)        │   │
│  └─────────────┘    └──────────────┘    └─────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Operator开发框架对比

| 框架 | 语言 | 学习曲线 | 生态成熟度 | 适用场景 |
|------|------|----------|------------|----------|
| **Kubebuilder** | Go | 中 | ⭐⭐⭐⭐⭐ | 企业级Operator |
| **Operator SDK** | Go/Multi | 中 | ⭐⭐⭐⭐⭐ | 全功能开发 |
| **KUDO** | YAML | 低 | ⭐⭐⭐ | 声明式Operator |
| **Metacontroller** | 多语言 | 低 | ⭐⭐⭐ | 简单场景 |
| **[[Crossplane|Crossplane]]** | Go | 高 | ⭐⭐⭐⭐ | 基础设施即代码 |

<!-- chunk: Kubebuilder开发实践 -->
## Kubebuilder开发实践

### 1. 项目初始化

```bash
# 安装kubebuilder
curl -L -o kubebuilder https://go.kubebuilder.io/dl/latest/$(go env GOOS)/$(go env GOARCH)
chmod +x kubebuilder && sudo mv kubebuilder /usr/local/bin/

# 创建项目
mkdir mysql-operator && cd mysql-operator
kubebuilder init --domain example.com --repo github.com/example/mysql-operator

# 创建API
kubebuilder create api --group database --version v1beta1 --kind MySQLCluster

# 创建控制器
kubebuilder create controller --group database --version v1beta1 --kind MySQLCluster
```

### 2. API定义 (api/v1beta1/mysqlcluster_types.go)

```go
/*
Copyright 2024 The MySQL Operator Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package v1beta1

import (
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// EDIT THIS FILE!  THIS IS SCAFFOLDING FOR YOU TO OWN!
// NOTE: json tags are required.  Any new fields you add must have json tags for the fields to be serialized.

// MySQLClusterSpec defines the desired state of MySQLCluster
type MySQLClusterSpec struct {
    // INSERT ADDITIONAL SPEC FIELDS - desired state of cluster
    // Important: Run "make" to regenerate code after modifying this file

    // Replicas is the number of instances in the cluster
    // +kubebuilder:validation:Minimum=1
    // +kubebuilder:validation:Maximum=10
    // +kubebuilder:default=1
    Replicas int32 `json:"replicas,omitempty"`

    // Version specifies the MySQL version to use
    // +kubebuilder:validation:Enum=5.7;8.0
    // +kubebuilder:default="8.0"
    Version string `json:"version,omitempty"`

    // Storage configuration
    Storage StorageSpec `json:"storage"`

    // Resources configuration
    // +optional
    Resources *ResourcesSpec `json:"resources,omitempty"`

    // Backup configuration
    // +optional
    Backup *BackupSpec `json:"backup,omitempty"`
}

// StorageSpec defines storage requirements
type StorageSpec struct {
    // Size of the persistent volume
    // +kubebuilder:validation:Pattern=`^[0-9]+Gi$`
    Size string `json:"size"`

    // Storage class name
    // +optional
    Class string `json:"class,omitempty"`
}

// ResourcesSpec defines compute resources
type ResourcesSpec struct {
    // CPU request
    // +optional
    CPU string `json:"cpu,omitempty"`

    // Memory request
    // +optional
    Memory string `json:"memory,omitempty"`

    // CPU limit
    // +optional
    CPULimit string `json:"cpuLimit,omitempty"`

    // Memory limit
    // +optional
    MemoryLimit string `json:"memoryLimit,omitempty"`
}

// BackupSpec defines backup configuration
type BackupSpec struct {
    // Enable automatic backup
    Enabled bool `json:"enabled"`

    // Cron schedule for backup
    // +optional
    Schedule string `json:"schedule,omitempty"`

    // Retention period
    // +optional
    Retention string `json:"retention,omitempty"`
}

// MySQLClusterStatus defines the observed state of MySQLCluster
type MySQLClusterStatus struct {
    // INSERT ADDITIONAL STATUS FIELD - define observed state of cluster
    // Important: Run "make" to regenerate code after modifying this file

    // Phase represents the current phase of cluster
    // +optional
    Phase ClusterPhase `json:"phase,omitempty"`

    // Replicas is the number of actual running instances
    // +optional
    Replicas int32 `json:"replicas,omitempty"`

    // Conditions represent the latest available observations of an object's state
    // +optional
    Conditions []metav1.Condition `json:"conditions,omitempty"`

    // ReadyReplicas is the number of ready instances
    // +optional
    ReadyReplicas int32 `json:"readyReplicas,omitempty"`
}

// ClusterPhase represents the phase of MySQL cluster
// +kubebuilder:validation:Enum=Pending;Creating;Running;Failed;Deleting
type ClusterPhase string

const (
    ClusterPending  ClusterPhase = "Pending"
    ClusterCreating ClusterPhase = "Creating"
    ClusterRunning  ClusterPhase = "Running"
    ClusterFailed   ClusterPhase = "Failed"
    ClusterDeleting ClusterPhase = "Deleting"
)

//+kubebuilder:object:root=true
//+kubebuilder:subresource:status
//+kubebuilder:printcolumn:name="Replicas",type="integer",JSONPath=".spec.replicas"
//+kubebuilder:printcolumn:name="Status",type="string",JSONPath=".status.phase"
//+kubebuilder:printcolumn:name="Ready",type="integer",JSONPath=".status.readyReplicas"
//+kubebuilder:printcolumn:name="Age",type="date",JSONPath=".metadata.creationTimestamp"

// MySQLCluster is the Schema for the mysqlclusters API
type MySQLCluster struct {
    metav1.TypeMeta   `json:",inline"`
    metav1.ObjectMeta `json:"metadata,omitempty"`

    Spec   MySQLClusterSpec   `json:"spec,omitempty"`
    Status MySQLClusterStatus `json:"status,omitempty"`
}

//+kubebuilder:object:root=true

// MySQLClusterList contains a list of MySQLCluster
type MySQLClusterList struct {
    metav1.TypeMeta `json:",inline"`
    metav1.ListMeta `json:"metadata,omitempty"`
    Items           []MySQLCluster `json:"items"`
}

func init() {
    SchemeBuilder.Register(&MySQLCluster{}, &MySQLClusterList{})
}
```

### 3. 控制器实现 (controllers/mysqlcluster_controller.go)

```go
/*
Copyright 2024 The MySQL Operator Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package controllers

import (
    "context"
    "fmt"
    "time"

    "github.com/go-logr/logr"
    appsv1 "k8s.io/api/apps/v1"
    corev1 "k8s.io/api/core/v1"
    apierrors "k8s.io/apimachinery/pkg/api/errors"
    "k8s.io/apimachinery/pkg/api/resource"
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/apimachinery/pkg/runtime"
    "k8s.io/apimachinery/pkg/types"
    "k8s.io/client-go/tools/record"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "sigs.k8s.io/controller-runtime/pkg/controller/controllerutil"
    "sigs.k8s.io/controller-runtime/pkg/log"

    databasev1beta1 "github.com/example/mysql-operator/api/v1beta1"
)

// MySQLClusterReconciler reconciles a MySQLCluster object
type MySQLClusterReconciler struct {
    client.Client
    Scheme   *runtime.Scheme
    Recorder record.EventRecorder
    Log      logr.Logger
}

//+kubebuilder:rbac:groups=database.example.com,resources=mysqlclusters,verbs=get;list;watch;create;update;patch;delete
//+kubebuilder:rbac:groups=database.example.com,resources=mysqlclusters/status,verbs=get;update;patch
//+kubebuilder:rbac:groups=database.example.com,resources=mysqlclusters/finalizers,verbs=update
//+kubebuilder:rbac:groups=apps,resources=statefulsets,verbs=get;list;watch;create;update;patch;delete
//+kubebuilder:rbac:groups=core,resources=services;persistentvolumeclaims,verbs=get;list;watch;create;update;patch;delete
//+kubebuilder:rbac:groups=core,resources=events,verbs=create;patch

// Reconcile is part of the main kubernetes reconciliation loop which aims to
// move the current state of the cluster closer to the desired state.
func (r *MySQLClusterReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    log := log.FromContext(ctx)

    // Fetch the MySQLCluster instance
    cluster := &databasev1beta1.MySQLCluster{}
    if err := r.Get(ctx, req.NamespacedName, cluster); err != nil {
        if apierrors.IsNotFound(err) {
            // Object not found, return. Created objects are automatically garbage collected.
            log.Info("MySQLCluster resource not found. Ignoring since object must be deleted")
            return ctrl.Result{}, nil
        }
        // Error reading the object - requeue the request.
        log.Error(err, "Failed to get MySQLCluster")
        return ctrl.Result{}, err
    }

    // Handle deletion
    if cluster.DeletionTimestamp != nil {
        return r.handleDeletion(ctx, cluster)
    }

    // Add finalizer if not present
    if !controllerutil.ContainsFinalizer(cluster, "mysqlcluster.finalizers.example.com") {
        controllerutil.AddFinalizer(cluster, "mysqlcluster.finalizers.example.com")
        if err := r.Update(ctx, cluster); err != nil {
            return ctrl.Result{}, err
        }
    }

    // Set initial status
    if cluster.Status.Phase == "" {
        cluster.Status.Phase = databasev1beta1.ClusterPending
        if err := r.Status().Update(ctx, cluster); err != nil {
            return ctrl.Result{}, err
        }
    }

    // Reconcile StatefulSet
    sts, err := r.reconcileStatefulSet(ctx, cluster)
    if err != nil {
        log.Error(err, "Failed to reconcile StatefulSet")
        r.Recorder.Event(cluster, "Warning", "FailedCreate", fmt.Sprintf("Failed to create StatefulSet: %v", err))
        return ctrl.Result{RequeueAfter: 30 * time.Second}, err
    }

    // Reconcile Services
    if err := r.reconcileServices(ctx, cluster); err != nil {
        log.Error(err, "Failed to reconcile Services")
        return ctrl.Result{RequeueAfter: 30 * time.Second}, err
    }

    // Update status
    return r.updateStatus(ctx, cluster, sts)
}

func (r *MySQLClusterReconciler) reconcileStatefulSet(ctx context.Context, cluster *databasev1beta1.MySQLCluster) (*appsv1.StatefulSet, error) {
    log := log.FromContext(ctx)

    // Define StatefulSet
    sts := &appsv1.StatefulSet{
        ObjectMeta: metav1.ObjectMeta{
            Name:      cluster.Name + "-mysql",
            Namespace: cluster.Namespace,
        },
        Spec: appsv1.StatefulSetSpec{
            Replicas: &cluster.Spec.Replicas,
            Selector: &metav1.LabelSelector{
                MatchLabels: map[string]string{
                    "app":     "mysql",
                    "cluster": cluster.Name,
                },
            },
            Template: corev1.PodTemplateSpec{
                ObjectMeta: metav1.ObjectMeta{
                    Labels: map[string]string{
                        "app":     "mysql",
                        "cluster": cluster.Name,
                    },
                },
                Spec: corev1.PodSpec{
                    Containers: []corev1.Container{
                        {
                            Name:  "mysql",
                            Image: fmt.Sprintf("mysql:%s", cluster.Spec.Version),
                            Ports: []corev1.ContainerPort{
                                {
                                    ContainerPort: 3306,
                                    Name:          "mysql",
                                },
                            },
                            Env: []corev1.EnvVar{
                                {
                                    Name:  "MYSQL_ROOT_PASSWORD",
                                    Value: "rootpassword", // In production, use Secret
                                },
                            },
                            VolumeMounts: []corev1.VolumeMount{
                                {
                                    Name:      "data",
                                    MountPath: "/var/lib/mysql",
                                },
                            },
                        },
                    },
                    Volumes: []corev1.Volume{
                        {
                            Name: "data",
                            VolumeSource: corev1.VolumeSource{
                                PersistentVolumeClaim: &corev1.PersistentVolumeClaimVolumeSource{
                                    ClaimName: cluster.Name + "-mysql",
                                },
                            },
                        },
                    },
                },
            },
            VolumeClaimTemplates: []corev1.PersistentVolumeClaim{
                {
                    ObjectMeta: metav1.ObjectMeta{
                        Name: "data",
                    },
                    Spec: corev1.PersistentVolumeClaimSpec{
                        AccessModes: []corev1.PersistentVolumeAccessMode{
                            corev1.ReadWriteOnce,
                        },
                        Resources: corev1.ResourceRequirements{
                            Requests: corev1.ResourceList{
                                corev1.ResourceStorage: resource.MustParse(cluster.Spec.Storage.Size),
                            },
                        },
                        StorageClassName: &cluster.Spec.Storage.Class,
                    },
                },
            },
        },
    }

    // Set MySQLCluster instance as the owner and controller
    if err := ctrl.SetControllerReference(cluster, sts, r.Scheme); err != nil {
        return nil, err
    }

    // Create or update StatefulSet
    result, err := controllerutil.CreateOrUpdate(ctx, r.Client, sts, func() error {
        // Update existing StatefulSet
        sts.Spec.Replicas = &cluster.Spec.Replicas
        sts.Spec.Template.Spec.Containers[0].Image = fmt.Sprintf("mysql:%s", cluster.Spec.Version)
        sts.Spec.VolumeClaimTemplates[0].Spec.Resources.Requests[corev1.ResourceStorage] = resource.MustParse(cluster.Spec.Storage.Size)
        if cluster.Spec.Storage.Class != "" {
            sts.Spec.VolumeClaimTemplates[0].Spec.StorageClassName = &cluster.Spec.Storage.Class
        }
        return nil
    })
    if err != nil {
        return nil, err
    }

    if result != controllerutil.OperationResultNone {
        log.Info("StatefulSet updated", "operation", result)
        r.Recorder.Event(cluster, "Normal", "Updated", fmt.Sprintf("StatefulSet %s", result))
    }

    return sts, nil
}

func (r *MySQLClusterReconciler) reconcileServices(ctx context.Context, cluster *databasev1beta1.MySQLCluster) error {
    // Headless service for StatefulSet
    headlessSvc := &corev1.Service{
        ObjectMeta: metav1.ObjectMeta{
            Name:      cluster.Name + "-mysql-headless",
            Namespace: cluster.Namespace,
        },
        Spec: corev1.ServiceSpec{
            ClusterIP: "None",
            Selector: map[string]string{
                "app":     "mysql",
                "cluster": cluster.Name,
            },
            Ports: []corev1.ServicePort{
                {
                    Port: 3306,
                    Name: "mysql",
                },
            },
        },
    }

    if err := ctrl.SetControllerReference(cluster, headlessSvc, r.Scheme); err != nil {
        return err
    }

    if _, err := controllerutil.CreateOrUpdate(ctx, r.Client, headlessSvc, func() error {
        return nil
    }); err != nil {
        return err
    }

    // Client service
    clientSvc := &corev1.Service{
        ObjectMeta: metav1.ObjectMeta{
            Name:      cluster.Name + "-mysql",
            Namespace: cluster.Namespace,
        },
        Spec: corev1.ServiceSpec{
            Selector: map[string]string{
                "app":     "mysql",
                "cluster": cluster.Name,
            },
            Ports: []corev1.ServicePort{
                {
                    Port: 3306,
                    Name: "mysql",
                },
            },
        },
    }

    if err := ctrl.SetControllerReference(cluster, clientSvc, r.Scheme); err != nil {
        return err
    }

    if _, err := controllerutil.CreateOrUpdate(ctx, r.Client, clientSvc, func() error {
        return nil
    }); err != nil {
        return err
    }

    return nil
}

func (r *MySQLClusterReconciler) updateStatus(ctx context.Context, cluster *databasev1beta1.MySQLCluster, sts *appsv1.StatefulSet) (ctrl.Result, error) {
    log := log.FromContext(ctx)

    // Get actual state
    readyReplicas := sts.Status.ReadyReplicas
    currentReplicas := sts.Status.Replicas

    // Update status
    cluster.Status.Replicas = currentReplicas
    cluster.Status.ReadyReplicas = readyReplicas

    // Determine phase
    if readyReplicas == 0 && currentReplicas == 0 {
        cluster.Status.Phase = databasev1beta1.ClusterPending
    } else if readyReplicas < currentReplicas {
        cluster.Status.Phase = databasev1beta1.ClusterCreating
    } else if readyReplicas == currentReplicas && currentReplicas > 0 {
        cluster.Status.Phase = databasev1beta1.ClusterRunning
        // Add ready condition
        r.setCondition(cluster, "Ready", metav1.ConditionTrue, "ClusterReady", "MySQL cluster is ready")
    } else {
        cluster.Status.Phase = databasev1beta1.ClusterFailed
        r.setCondition(cluster, "Ready", metav1.ConditionFalse, "ClusterFailed", "MySQL cluster failed")
    }

    if err := r.Status().Update(ctx, cluster); err != nil {
        log.Error(err, "Failed to update MySQLCluster status")
        return ctrl.Result{}, err
    }

    // Requeue if not ready
    if cluster.Status.Phase != databasev1beta1.ClusterRunning {
        return ctrl.Result{RequeueAfter: 10 * time.Second}, nil
    }

    return ctrl.Result{}, nil
}

func (r *MySQLClusterReconciler) setCondition(cluster *databasev1beta1.MySQLCluster, conditionType string, status metav1.ConditionStatus, reason, message string) {
    // Implementation for setting conditions
}

func (r *MySQLClusterReconciler) handleDeletion(ctx context.Context, cluster *databasev1beta1.MySQLCluster) (ctrl.Result, error) {
    // Cleanup logic here
    if controllerutil.ContainsFinalizer(cluster, "mysqlcluster.finalizers.example.com") {
        // Perform cleanup
        controllerutil.RemoveFinalizer(cluster, "mysqlcluster.finalizers.example.com")
        if err := r.Update(ctx, cluster); err != nil {
            return ctrl.Result{}, err
        }
    }
    return ctrl.Result{}, nil
}

// SetupWithManager sets up the controller with the Manager.
func (r *MySQLClusterReconciler) SetupWithManager(mgr ctrl.Manager) error {
    return ctrl.NewControllerManagedBy(mgr).
        For(&databasev1beta1.MySQLCluster{}).
        Owns(&appsv1.StatefulSet{}).
        Owns(&corev1.Service{}).
        Complete(r)
}
```

### 4. Makefile配置

```makefile
# Image URL to use all building/pushing image targets
IMG ?= controller:latest
# ENVTEST_K8S_VERSION refers to the version of kubebuilder assets to be downloaded by envtest binary.
ENVTEST_K8S_VERSION = 1.28.0

# Get the currently used golang install path (in GOPATH/bin, unless GOBIN is set)
ifeq (,$(shell go env GOBIN))
GOBIN=$(shell go env GOPATH)/bin
else
GOBIN=$(shell go env GOBIN)
endif

# CONTAINER_TOOL defines the container tool to be used for building images.
# Be aware that the target commands are only tested with Docker which is
# scaffolded by default. However, you might want to replace it to use other
# tools. (i.e. podman)
CONTAINER_TOOL ?= docker

# Setting SHELL to bash allows bash commands to be executed by recipes.
# Options are set to exit when a recipe line exits non-zero or a piped command fails.
SHELL = /usr/bin/env bash -o pipefail
.SHELLFLAGS = -ec

.PHONY: all
all: build

##@ General

# The help target prints out all targets with their descriptions organized
# beneath their categories. The categories are represented by '##@' and the
# target descriptions by '##'. The awk command is responsible for reading the
# entire set of makefiles included in this invocation, looking for lines of the
# file as xyz: ## something, and then pretty-format the target and help. Then,
# if there's a line with ##@ something, that gets pretty-printed as a category.
# More info on the usage of ANSI control characters for terminal formatting:
# https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_parameters
# More info on the awk command:
# http://linuxcommand.org/lc3_adv_awk.php

.PHONY: help
help: ## Display this help.
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make \033[36m<target>\033[0m\n"} /^[a-zA-Z_0-9-]+:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

##@ Development

.PHONY: manifests
manifests: controller-gen ## Generate WebhookConfiguration, ClusterRole and CustomResourceDefinition objects.
	$(CONTROLLER_GEN) rbac:roleName=manager-role crd webhook paths="./..." output:crd:artifacts:config=config/crd/bases

.PHONY: generate
generate: controller-gen ## Generate code containing DeepCopy, DeepCopyInto, and DeepCopyObject method implementations.
	$(CONTROLLER_GEN) object:headerFile="hack/boilerplate.go.txt" paths="./..."

.PHONY: fmt
fmt: ## Run go fmt against code.
	go fmt ./...

.PHONY: vet
vet: ## Run go vet against code.
	go vet ./...

.PHONY: test
test: manifests generate fmt vet envtest ## Run tests.
	KUBEBUILDER_ASSETS="$(shell $(ENVTEST) use $(ENVTEST_K8S_VERSION) --bin-dir $(LOCALBIN) -p path)" go test ./... -coverprofile cover.out

##@ Build

.PHONY: build
build: manifests generate fmt vet ## Build manager binary.
	go build -o bin/manager main.go

.PHONY: run
run: manifests generate fmt vet ## Run a controller from your host.
	go run ./main.go

# If you wish to build the manager image targeting other platforms you can use the --platform flag.
# (i.e. docker build --platform linux/arm64). However, you must enable docker buildKit for it.
# More info: https://docs.docker.com/develop/develop-images/build_enhancements/
.PHONY: docker-build
docker-build: test ## Build docker image with the manager.
	$(CONTAINER_TOOL) build -t ${IMG} .

.PHONY: docker-push
docker-push: ## Push docker image with the manager.
	$(CONTAINER_TOOL) push ${IMG}

# PLATFORMS defines the target platforms for the manager image be built to provide support to multiple
# architectures. (i.e. make docker-buildx IMG=myregistry/mypoperator:latest). To use this option you need to:
# - be able to use docker buildx. More info: https://docs.docker.com/build/buildx/
# - have enabled BuildKit. More info: https://docs.docker.com/develop/develop-images/build_enhancements/
# - be able to push the image to your registry (i.e. if you do not set a valid value via IMG=<myregistry/image:<tag>> then the export will fail)
# To adequately provide solutions that are compatible with multiple platforms, you should consider using this option.
PLATFORMS ?= linux/arm64,linux/amd64,linux/s390x,linux/ppc64le
.PHONY: docker-buildx
docker-buildx: test ## Build and push docker image for the manager for cross-platform support
	# copy existing Dockerfile and insert --platform=${BUILDPLATFORM} into Dockerfile.cross, and preserve the original Dockerfile
	sed -e '1 s/\(^FROM\)/FROM --platform=\$$\{BUILDPLATFORM\}/; t' -e ' 1,// s//FROM --platform=\$$\{BUILDPLATFORM\}/' Dockerfile > Dockerfile.cross
	- $(CONTAINER_TOOL) buildx create --name project-v3-builder
	$(CONTAINER_TOOL) buildx use project-v3-builder
	- $(CONTAINER_TOOL) buildx build --push --platform=$(PLATFORMS) --tag ${IMG} -f Dockerfile.cross .
	- $(CONTAINER_TOOL) buildx rm project-v3-builder
	rm Dockerfile.cross

##@ Deployment

ifndef ignore-not-found
  ignore-not-found = false
endif

.PHONY: install
install: manifests kustomize ## Install CRDs into the K8s cluster specified in ~/.kube/config.
	$(KUSTOMIZE) build config/crd | kubectl apply -f -

.PHONY: uninstall
uninstall: manifests kustomize ## Uninstall CRDs from the K8s cluster specified in ~/.kube/config. Call with ignore-not-found=true to ignore resource not found errors during deletion.
	$(KUSTOMIZE) build config/crd | kubectl delete --ignore-not-found=$(ignore-not-found) -f -

.PHONY: deploy
deploy: manifests kustomize ## Deploy controller to the K8s cluster specified in ~/.kube/config.
	cd config/manager && $(KUSTOMIZE) edit set image controller=${IMG}
	$(KUSTOMIZE) build config/default | kubectl apply -f -

.PHONY: undeploy
undeploy: ## Undeploy controller from the K8s cluster specified in ~/.kube/config. Call with ignore-not-found=true to ignore resource not found errors during deletion.
	$(KUSTOMIZE) build config/default | kubectl delete --ignore-not-found=$(ignore-not-found) -f -

##@ Build Dependencies

<!-- chunk: Location to install dependencies to -->
## Location to install dependencies to
LOCALBIN ?= $(shell pwd)/bin
$(LOCALBIN):
	mkdir -p $(LOCALBIN)

<!-- chunk: Tool Binaries -->
## Tool Binaries
KUSTOMIZE ?= $(LOCALBIN)/kustomize
CONTROLLER_GEN ?= $(LOCALBIN)/controller-gen
ENVTEST ?= $(LOCALBIN)/setup-envtest

<!-- chunk: Tool Versions -->
## Tool Versions
KUSTOMIZE_VERSION ?= v5.0.1
CONTROLLER_TOOLS_VERSION ?= v0.12.0

.PHONY: kustomize
kustomize: $(KUSTOMIZE) ## Download kustomize locally if necessary. If wrong version is installed, it will be removed before downloading.
$(KUSTOMIZE): $(LOCALBIN)
	@if test -x $(LOCALBIN)/kustomize && ! $(LOCALBIN)/kustomize version | grep -q $(KUSTOMIZE_VERSION); then \
		echo "$(LOCALBIN)/kustomize version is not expected $(KUSTOMIZE_VERSION). Removing it before installing."; \
		rm -rf $(LOCALBIN)/kustomize; \
	fi
	test -s $(LOCALBIN)/kustomize || GOBIN=$(LOCALBIN) GO111MODULE=on go install sigs.k8s.io/kustomize/kustomize/v5@$(KUSTOMIZE_VERSION)

.PHONY: controller-gen
controller-gen: $(CONTROLLER_GEN) ## Download controller-gen locally if necessary. If wrong version is installed, it will be overwritten.
$(CONTROLLER_GEN): $(LOCALBIN)
	test -s $(LOCALBIN)/controller-gen && $(LOCALBIN)/controller-gen --version | grep -q $(CONTROLLER_TOOLS_VERSION) || \
	GOBIN=$(LOCALBIN) go install sigs.k8s.io/controller-tools/cmd/controller-gen@$(CONTROLLER_TOOLS_VERSION)

.PHONY: envtest
envtest: $(ENVTEST) ## Download envtest-setup locally if necessary.
$(ENVTEST): $(LOCALBIN)
	test -s $(LOCALBIN)/setup-envtest || GOBIN=$(LOCALBIN) go install sigs.k8s.io/controller-runtime/tools/setup-envtest@latest
```

<!-- chunk: Operator部署配置 -->
## Operator部署配置

### 1. RBAC配置 (config/rbac/role.yaml)

```yaml
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  creationTimestamp: null
  name: manager-role
rules:
- apiGroups:
  - apps
  resources:
  - statefulsets
  verbs:
  - create
  - delete
  - get
  - list
  - patch
  - update
  - watch
- apiGroups:
  - ""
  resources:
  - events
  verbs:
  - create
  - patch
- apiGroups:
  - ""
  resources:
  - persistentvolumeclaims
  - services
  verbs:
  - create
  - delete
  - get
  - list
  - patch
  - update
  - watch
- apiGroups:
  - database.example.com
  resources:
  - mysqlclusters
  verbs:
  - create
  - delete
  - get
  - list
  - patch
  - update
  - watch
- apiGroups:
  - database.example.com
  resources:
  - mysqlclusters/finalizers
  verbs:
  - update
- apiGroups:
  - database.example.com
  resources:
  - mysqlclusters/status
  verbs:
  - get
  - patch
  - update
```

### 2. 部署清单 (config/default/kustomization.yaml)

```yaml
# Adds namespace to all resources.
namespace: mysql-operator-system

# Value of this field is prepended to the
# names of all resources, e.g. a deployment named
# "wordpress" becomes "alices-wordpress".
# Note that it should also match with the prefix (text before '-') of the namespace
# field above.
namePrefix: mysql-operator-

# Labels to add to all resources and selectors.
#labels:
#- includeSelectors: true
#  pairs:
#    someName: someValue

resources:
- ../crd
- ../rbac
- ../manager
# [WEBHOOK] To enable webhook, uncomment all the sections with [WEBHOOK] prefix including the one in
# crd/kustomization.yaml
#- ../webhook
# [CERTMANAGER] To enable cert-manager, uncomment all sections with 'CERTMANAGER'. 'WEBHOOK' components are required.
#- ../certmanager
# [PROMETHEUS] To enable prometheus monitor, uncomment all sections with 'PROMETHEUS'.
#- ../prometheus

patchesStrategicMerge:
# Protect the /metrics endpoint by putting it behind auth.
# If you want your controller-manager to expose the /metrics
# endpoint w/o any authn/z, please comment the following line.
- manager_auth_proxy_patch.yaml



# [WEBHOOK] To enable webhook, uncomment all the sections with [WEBHOOK] prefix including the one in
# crd/kustomization.yaml
#- manager_webhook_patch.yaml

# [CERTMANAGER] To enable cert-manager, uncomment all sections with 'CERTMANAGER'.
# Uncomment 'CERTMANAGER' sections in crd/kustomization.yaml to enable the CA injection in the admission webhooks.
# 'CERTMANAGER' needs to be enabled to use ca injection
#- webhookcainjection_patch.yaml

# [CERTMANAGER] To enable cert-manager, uncomment all sections with 'CERTMANAGER' above
# Uncomment the following replacements to add the cert-manager CA injection annotations
#replacements:
#  - source: # Add cert-manager annotation to ValidatingWebhookConfiguration, MutatingWebhookConfiguration and CRDs
#      kind: Certificate
#      group: cert-manager.io
#      version: v1
#      name: serving-cert # this name should match the one in certificate.yaml
#      fieldPath: .metadata.namespace # namespace of the certificate CR
#    targets:
#      - select:
#          kind: ValidatingWebhookConfiguration
#        fieldPaths:
#          - .metadata.annotations.[cert-manager.io/inject-ca-from]
#        options:
#          delimiter: '/'
#          index: 0
#          create: true
#      - select:
#          kind: MutatingWebhookConfiguration
#        fieldPaths:
#          - .metadata.annotations.[cert-manager.io/inject-ca-from]
#        options:
#          delimiter: '/'
#          index: 0
#          create: true
#      - select:
#          kind: CustomResourceDefinition
#        fieldPaths:
#          - .metadata.annotations.[cert-manager.io/inject-ca-from]
#        options:
#          delimiter: '/'
#          index: 0
#          create: true
#  - source:
#      kind: Certificate
#      group: cert-manager.io
#      version: v1
#      name: serving-cert # this name should match the one in certificate.yaml
#      fieldPath: .metadata.name
#    targets:
#      - select:
#          kind: ValidatingWebhookConfiguration
#        fieldPaths:
#          - .metadata.annotations.[cert-manager.io/inject-ca-from]
#        options:
#          delimiter: '/'
#          index: 1
#          create: true
#      - select:
#          kind: MutatingWebhookConfiguration
#        fieldPaths:
#          - .metadata.annotations.[cert-manager.io/inject-ca-from]
#        options:
#          delimiter: '/'
#          index: 1
#          create: true
#      - select:
#          kind: CustomResourceDefinition
#        fieldPaths:
#          - .metadata.annotations.[cert-manager.io/inject-ca-from]
#        options:
#          delimiter: '/'
#          index: 1
#          create: true
#  - source: # Add cert-manager annotation to the webhook Service
#      kind: Service
#      version: v1
#      name: webhook-service
#      fieldPath: .metadata.name # namespace of the service
#    targets:
#      - select:
#          kind: Certificate
#          group: cert-manager.io
#          version: v1
#        fieldPaths:
#          - .spec.dnsNames.0
#          - .spec.dnsNames.1
#        options:
#          delimiter: '.'
#          index: 0
#          create: true
#  - source:
#      kind: Service
#      version: v1
#      name: webhook-service
#      fieldPath: .metadata.namespace # namespace of the service
#    targets:
#      - select:
#          kind: Certificate
#          group: cert-manager.io
#          version: v1
#        fieldPaths:
#          - .spec.dnsNames.0
#          - .spec.dnsNames.1
#        options:
#          delimiter: '.'
#          index: 1
#          create: true
```

<!-- chunk: Operator监控与运维 -->
## Operator监控与运维

### 1. 监控指标

```go
// metrics.go
package metrics

import (
    "github.com/prometheus/client_golang/prometheus"
    "sigs.k8s.io/controller-runtime/pkg/metrics"
)

var (
    ClusterReconcileTotal = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "mysql_operator_cluster_reconcile_total",
            Help: "Total number of cluster reconciliations",
        },
        []string{"result"},
    )
    
    ClusterReconcileDuration = prometheus.NewHistogramVec(
        prometheus.HistogramOpts{
            Name: "mysql_operator_cluster_reconcile_duration_seconds",
            Help: "Duration of cluster reconciliations",
            Buckets: []float64{0.1, 0.5, 1, 5, 10, 30},
        },
        []string{},
    )
    
    ClusterStatus = prometheus.NewGaugeVec(
        prometheus.GaugeOpts{
            Name: "mysql_operator_cluster_status",
            Help: "Current status of MySQL clusters",
        },
        []string{"namespace", "name", "phase"},
    )
)

func init() {
    // Register custom metrics with the global prometheus registry
    metrics.Registry.MustRegister(
        ClusterReconcileTotal,
        ClusterReconcileDuration,
        ClusterStatus,
    )
}
```

### 2. 健康检查端点

```go
// health_check.go
package health

import (
    "context"
    "net/http"
    "time"

    "sigs.k8s.io/controller-runtime/pkg/healthz"
    "sigs.k8s.io/controller-runtime/pkg/log"
)

// CustomHealthCheck implements custom health check logic
type CustomHealthCheck struct {
    // Add dependencies here
}

func (c *CustomHealthCheck) Check(req *http.Request) error {
    ctx, cancel := context.WithTimeout(req.Context(), 5*time.Second)
    defer cancel()

    // Implement custom health check logic
    // For example: check database connectivity, external API availability, etc.
    
    log := log.FromContext(ctx)
    log.Info("Health check passed")
    return nil
}

// SetupHealthChecks configures health check endpoints
func SetupHealthChecks(mgr ctrl.Manager) error {
    // Add readiness check
    if err := mgr.AddHealthzCheck("ready", healthz.Ping); err != nil {
        return err
    }

    // Add liveness check
    if err := mgr.AddHealthzCheck("live", &CustomHealthCheck{}); err != nil {
        return err
    }

    return nil
}
```

<!-- chunk: Operator测试策略 -->
## Operator测试策略

### 1. 单元测试

```go
// controllers/mysqlcluster_controller_test.go
package controllers

import (
    "context"
    "time"

    . "github.com/onsi/ginkgo/v2"
    . "github.com/onsi/gomega"
    appsv1 "k8s.io/api/apps/v1"
    corev1 "k8s.io/api/core/v1"
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/apimachinery/pkg/types"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "sigs.k8s.io/controller-runtime/pkg/reconcile"

    databasev1beta1 "github.com/example/mysql-operator/api/v1beta1"
)

var _ = Describe("MySQLCluster Controller", func() {
    const (
        MySQLClusterName      = "test-mysql"
        MySQLClusterNamespace = "default"
    )

    Context("When creating MySQLCluster", func() {
        It("Should create StatefulSet and Services", func() {
            By("Creating a new MySQLCluster")
            ctx := context.Background()
            cluster := &databasev1beta1.MySQLCluster{
                TypeMeta: metav1.TypeMeta{
                    APIVersion: "database.example.com/v1beta1",
                    Kind:       "MySQLCluster",
                },
                ObjectMeta: metav1.ObjectMeta{
                    Name:      MySQLClusterName,
                    Namespace: MySQLClusterNamespace,
                },
                Spec: databasev1beta1.MySQLClusterSpec{
                    Replicas: 1,
                    Version:  "8.0",
                    Storage: databasev1beta1.StorageSpec{
                        Size: "10Gi",
                    },
                },
            }
            Expect(k8sClient.Create(ctx, cluster)).Should(Succeed())

            By("Checking if StatefulSet was created")
            stsLookupKey := types.NamespacedName{Name: MySQLClusterName + "-mysql", Namespace: MySQLClusterNamespace}
            createdSts := &appsv1.StatefulSet{}
            eventuallyConsistentGet(ctx, stsLookupKey, createdSts)

            By("Checking if Services were created")
            svcLookupKey := types.NamespacedName{Name: MySQLClusterName + "-mysql", Namespace: MySQLClusterNamespace}
            createdSvc := &corev1.Service{}
            eventuallyConsistentGet(ctx, svcLookupKey, createdSvc)
        })
    })
})

func eventuallyConsistentGet(ctx context.Context, key client.ObjectKey, obj client.Object) {
    Eventually(func() bool {
        err := k8sClient.Get(ctx, key, obj)
        return err == nil
    }, time.Minute, time.Second).Should(BeTrue())
}
```

### 2. 集成测试

```bash
#!/bin/bash
# integration-test.sh

set -e

echo "🔧 准备测试环境..."
kind create cluster --name mysql-operator-test || true
kubectl cluster-info --context kind-mysql-operator-test

echo "🏗️  构建测试镜像..."
make docker-build IMG=test/mysql-operator:test

echo "📦 部署CRD..."
make install

echo "🚀 部署Operator..."
make deploy IMG=test/mysql-operator:test

echo "⏱️  等待Operator就绪..."
kubectl wait --for=condition=available deployment/mysql-operator-controller-manager -n mysql-operator-system --timeout=300s

echo "🧪 运行集成测试..."
go test -v ./test/integration/... -timeout 300s

echo "🧹 清理测试环境..."
kind delete cluster --name mysql-operator-test

echo "✅ 集成测试完成!"
```

<!-- chunk: Operator企业级生产实践 -->
## Operator企业级生产实践

### 1. 高可用部署架构

```yaml
# 高可用Operator部署配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mysql-operator-controller-manager
  namespace: mysql-operator-system
spec:
  replicas: 3  # 多副本实现高可用
  selector:
    matchLabels:
      control-plane: controller-manager
  template:
    metadata:
      labels:
        control-plane: controller-manager
    spec:
      serviceAccountName: mysql-operator-controller-manager
      terminationGracePeriodSeconds: 10
      containers:
      - name: manager
        image: mysql-operator:v1.0.0
        imagePullPolicy: IfNotPresent
        command:
        - /manager
        args:
        - --leader-elect  # 启用领导者选举
        - --leader-election-id=mysql-operator
        - --health-probe-bind-address=:8081
        - --metrics-bind-address=:8080
        - --zap-devel=true
        - --zap-encoder=console
        - --zap-stacktrace-level=error
        ports:
        - containerPort: 8080
          name: metrics
          protocol: TCP
        - containerPort: 8081
          name: health
          protocol: TCP
        env:
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8081
          initialDelaySeconds: 15
          periodSeconds: 20
        readinessProbe:
          httpGet:
            path: /readyz
            port: 8081
          initialDelaySeconds: 5
          periodSeconds: 10
        resources:
          limits:
            cpu: 500m
            memory: 512Mi
          requests:
            cpu: 100m
            memory: 128Mi
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            drop:
            - ALL
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          runAsUser: 1000

---
# 领导者选举配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: mysql-operator-config
  namespace: mysql-operator-system
data:
  controller_manager_config.yaml: |
    apiVersion: controller-runtime.sigs.k8s.io/v1alpha1
    kind: ControllerManagerConfig
    health:
      healthProbeBindAddress: :8081
    metrics:
      bindAddress: :8080
    leaderElection:
      leaderElect: true
      resourceName: mysql-operator-lock
      resourceNamespace: mysql-operator-system
    webhook:
      port: 9443
```

### 2. 生产级监控与告警

```go
// metrics.go - 生产级指标定义
package metrics

import (
    "github.com/prometheus/client_golang/prometheus"
    "sigs.k8s.io/controller-runtime/pkg/metrics"
)

var (
    // Operator核心指标
    OperatorReconcileTotal = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "mysql_operator_reconcile_total",
            Help: "Total number of reconciliations per controller",
        },
        []string{"controller", "result"},
    )
    
    OperatorReconcileErrors = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "mysql_operator_reconcile_errors_total",
            Help: "Total number of reconciliation errors per controller",
        },
        []string{"controller"},
    )
    
    OperatorReconcileDuration = prometheus.NewHistogramVec(
        prometheus.HistogramOpts{
            Name:    "mysql_operator_reconcile_duration_seconds",
            Help:    "Reconciliation duration in seconds",
            Buckets: prometheus.ExponentialBuckets(0.001, 2, 15),
        },
        []string{"controller"},
    )
    
    // 业务指标
    MySQLClustersManaged = prometheus.NewGauge(
        prometheus.GaugeOpts{
            Name: "mysql_clusters_managed_total",
            Help: "Total number of MySQL clusters managed by the operator",
        },
    )
    
    MySQLClusterStatus = prometheus.NewGaugeVec(
        prometheus.GaugeOpts{
            Name: "mysql_cluster_status",
            Help: "Current status of MySQL clusters",
        },
        []string{"cluster", "namespace", "status"},
    )
    
    MySQLBackupSuccessRate = prometheus.NewGaugeVec(
        prometheus.GaugeOpts{
            Name: "mysql_backup_success_rate",
            Help: "Backup success rate for MySQL clusters",
        },
        []string{"cluster", "namespace"},
    )
)

func init() {
    // 注册指标
    metrics.Registry.MustRegister(
        OperatorReconcileTotal,
        OperatorReconcileErrors,
        OperatorReconcileDuration,
        MySQLClustersManaged,
        MySQLClusterStatus,
        MySQLBackupSuccessRate,
    )
}

// RecordReconcileMetrics 记录协调指标
func RecordReconcileMetrics(controller string, err error, duration float64) {
    result := "success"
    if err != nil {
        result = "error"
        OperatorReconcileErrors.WithLabelValues(controller).Inc()
    }
    
    OperatorReconcileTotal.WithLabelValues(controller, result).Inc()
    OperatorReconcileDuration.WithLabelValues(controller).Observe(duration)
}
```

### 3. 安全加固配置

```yaml
# 安全强化的RBAC配置
apiVersion: v1
kind: ServiceAccount
metadata:
  name: mysql-operator-controller-manager
  namespace: mysql-operator-system
automountServiceAccountToken: false  # 禁用自动挂载

---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: mysql-operator-leader-election-role
  namespace: mysql-operator-system
rules:
- apiGroups:
  - ""
  resources:
  - configmaps
  verbs:
  - get
  - list
  - watch
  - create
  - update
  - patch
  - delete
- apiGroups:
  - coordination.k8s.io
  resources:
  - leases
  verbs:
  - get
  - list
  - watch
  - create
  - update
  - patch
  - delete
- apiGroups:
  - ""
  resources:
  - events
  verbs:
  - create
  - patch

---
# 最小权限的ClusterRole
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: mysql-operator-manager-role
rules:
# CRD资源权限
- apiGroups:
  - database.example.com
  resources:
  - mysqlclusters
  verbs:
  - create
  - delete
  - get
  - list
  - patch
  - update
  - watch
- apiGroups:
  - database.example.com
  resources:
  - mysqlclusters/finalizers
  verbs:
  - update
- apiGroups:
  - database.example.com
  resources:
  - mysqlclusters/status
  verbs:
  - get
  - patch
  - update

# 核心Kubernetes资源权限（最小化）
- apiGroups:
  - apps
  resources:
  - statefulsets
  verbs:
  - create
  - delete
  - get
  - list
  - patch
  - update
  - watch
- apiGroups:
  - ""
  resources:
  - services
  - configmaps
  - secrets
  - persistentvolumeclaims
  verbs:
  - create
  - delete
  - get
  - list
  - patch
  - update
  - watch
- apiGroups:
  - ""
  resources:
  - pods
  verbs:
  - get
  - list
  - watch
- apiGroups:
  - ""
  resources:
  - events
  verbs:
  - create
  - patch
```

### 4. 故障自愈与弹性设计

```go
// resilience.go - 弹性设计和故障自愈
package resilience

import (
    "context"
    "fmt"
    "time"
    
    "k8s.io/apimachinery/pkg/api/errors"
    "k8s.io/apimachinery/pkg/runtime"
    "k8s.io/client-go/util/retry"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "sigs.k8s.io/controller-runtime/pkg/log"
)

// CircuitBreaker 熔断器模式
type CircuitBreaker struct {
    failureThreshold int
    timeout          time.Duration
    lastFailure      time.Time
    failureCount     int
    isOpen           bool
}

func NewCircuitBreaker(threshold int, timeout time.Duration) *CircuitBreaker {
    return &CircuitBreaker{
        failureThreshold: threshold,
        timeout:          timeout,
        isOpen:           false,
    }
}

func (cb *CircuitBreaker) Execute(fn func() error) error {
    if cb.isOpen && time.Since(cb.lastFailure) < cb.timeout {
        return fmt.Errorf("circuit breaker is open")
    }
    
    err := fn()
    if err != nil {
        cb.failureCount++
        cb.lastFailure = time.Now()
        if cb.failureCount >= cb.failureThreshold {
            cb.isOpen = true
        }
        return err
    }
    
    // 成功执行，重置熔断器
    cb.failureCount = 0
    cb.isOpen = false
    return nil
}

// RetryableReconciler 支持重试的协调器
type RetryableReconciler struct {
    client.Client
    Scheme *runtime.Scheme
    circuitBreaker *CircuitBreaker
}

func (r *RetryableReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    log := log.FromContext(ctx)
    
    // 使用熔断器保护关键操作
    err := r.circuitBreaker.Execute(func() error {
        return r.performReconciliation(ctx, req)
    })
    
    if err != nil {
        // 记录错误并安排重试
        log.Error(err, "Reconciliation failed")
        
        // 对于瞬时错误，安排快速重试
        if r.isTransientError(err) {
            return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
        }
        
        // 对于永久错误，记录但不重试
        if r.isPermanentError(err) {
            log.Error(err, "Permanent error, will not retry")
            return ctrl.Result{}, nil
        }
        
        // 其他错误，使用指数退避重试
        return ctrl.Result{RequeueAfter: time.Minute}, nil
    }
    
    return ctrl.Result{}, nil
}

func (r *RetryableReconciler) performReconciliation(ctx context.Context, req ctrl.Request) error {
    // 使用retry包处理乐观锁冲突
    return retry.RetryOnConflict(retry.DefaultRetry, func() error {
        // 获取最新资源状态
        instance := &databasev1beta1.MySQLCluster{}
        if err := r.Get(ctx, req.NamespacedName, instance); err != nil {
            if errors.IsNotFound(err) {
                return nil // 资源已删除
            }
            return err
        }
        
        // 执行业务逻辑
        return r.reconcileInstance(ctx, instance)
    })
}

func (r *RetryableReconciler) isTransientError(err error) bool {
    // 定义瞬时错误类型
    transientErrors := []string{
        "connection refused",
        "timeout",
        "temporary failure",
    }
    
    for _, msg := range transientErrors {
        if strings.Contains(err.Error(), msg) {
            return true
        }
    }
    return false
}

func (r *RetryableReconciler) isPermanentError(err error) bool {
    // 定义永久错误类型
    return errors.IsInvalid(err) || errors.IsBadRequest(err)
}
```

### 5. 生产环境部署脚本

```bash
#!/bin/bash
# production-operator-deploy.sh

set -euo pipefail

OPERATOR_NAME="mysql-operator"
NAMESPACE="mysql-operator-system"
VERSION="v1.0.0"
REGISTRY="myregistry.com/operators"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log() {
    echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')] $1${NC}"
}

warn() {
    echo -e "${YELLOW}[$(date +'%Y-%m-%d %H:%M:%S')] $1${NC}"
}

error() {
    echo -e "${RED}[$(date +'%Y-%m-%d %H:%M:%S')] $1${NC}"
}

# 1. 环境预检
preflight_check() {
    log "🔍 执行环境预检..."
    
    # 检查kubectl
    if ! command -v kubectl &> /dev/null; then
        error "kubectl 未安装"
        exit 1
    fi
    
    # 检查集群连接
    if ! kubectl cluster-info &> /dev/null; then
        error "无法连接到Kubernetes集群"
        exit 1
    fi
    
    # 检查权限
    if ! kubectl auth can-i create namespace &> /dev/null; then
        error "缺少创建命名空间权限"
        exit 1
    fi
    
    log "✅ 环境预检通过"
}

# 2. 准备部署环境
prepare_environment() {
    log "🔧 准备部署环境..."
    
    # 创建命名空间
    kubectl create namespace ${NAMESPACE} --dry-run=client -o yaml | kubectl apply -f -
    
    # 创建必要的Secrets
    if [ ! -f "secrets.yaml" ]; then
        warn "未找到secrets.yaml文件，跳过密钥创建"
    else
        kubectl apply -f secrets.yaml -n ${NAMESPACE}
    fi
    
    # 创建ConfigMaps
    if [ ! -f "config.yaml" ]; then
        warn "未找到config.yaml文件，使用默认配置"
    else
        kubectl create configmap ${OPERATOR_NAME}-config \
            --from-file=config.yaml \
            -n ${NAMESPACE} \
            --dry-run=client -o yaml | kubectl apply -f -
    fi
    
    log "✅ 环境准备完成"
}

# 3. 部署CRD
deploy_crds() {
    log "📦 部署CRD..."
    
    # 备份现有CRD
    if kubectl get crd mysqlclusters.database.example.com &> /dev/null; then
        log "💾 备份现有CRD..."
        kubectl get crd mysqlclusters.database.example.com -o yaml > backup-crd-$(date +%Y%m%d-%H%M%S).yaml
    fi
    
    # 部署新CRD
    kubectl apply -f config/crd/bases/
    
    # 等待CRD就绪
    log "⏳ 等待CRD注册完成..."
    until kubectl get crd mysqlclusters.database.example.com &> /dev/null; do
        sleep 2
    done
    
    log "✅ CRD部署完成"
}

# 4. 部署Operator
deploy_operator() {
    log "🚀 部署Operator..."
    
    # 构建和推送镜像
    if [ "${SKIP_BUILD:-false}" = "false" ]; then
        log "🏗️  构建Operator镜像..."
        make docker-build IMG=${REGISTRY}/${OPERATOR_NAME}:${VERSION}
        make docker-push IMG=${REGISTRY}/${OPERATOR_NAME}:${VERSION}
    fi
    
    # 生成部署清单
    log "📋 生成部署清单..."
    kustomize build config/default > deploy.yaml
    
    # 替换镜像地址
    sed -i.bak "s|controller:latest|${REGISTRY}/${OPERATOR_NAME}:${VERSION}|g" deploy.yaml
    
    # 部署Operator
    kubectl apply -f deploy.yaml
    
    # 等待部署就绪
    log "⏳ 等待Operator就绪..."
    kubectl wait --for=condition=available \
        deployment/${OPERATOR_NAME}-controller-manager \
        -n ${NAMESPACE} \
        --timeout=300s
    
    log "✅ Operator部署完成"
}

# 5. 健康检查
health_check() {
    log "🏥 执行健康检查..."
    
    # 检查Pod状态
    kubectl get pods -n ${NAMESPACE}
    
    # 检查Leader选举
    kubectl get leases -n ${NAMESPACE}
    
    # 检查Metrics端点
    kubectl port-forward service/${OPERATOR_NAME}-controller-manager-metrics-service 8080:8080 -n ${NAMESPACE} &
    PORT_FORWARD_PID=$!
    sleep 5
    
    if curl -s http://localhost:8080/metrics | grep -q "mysql_operator"; then
        log "✅ Metrics端点正常"
    else
        warn "❌ Metrics端点异常"
    fi
    
    # 清理端口转发
    kill ${PORT_FORWARD_PID} 2>/dev/null || true
    
    # 测试CRD功能
    log "🧪 测试CRD功能..."
    cat <<EOF | kubectl apply -f -
apiVersion: database.example.com/v1beta1
kind: MySQLCluster
metadata:
  name: test-cluster
  namespace: ${NAMESPACE}
spec:
  replicas: 1
  storage:
    size: "5Gi"
EOF
    
    sleep 30
    if kubectl get mysqlcluster test-cluster -n ${NAMESPACE} &> /dev/null; then
        log "✅ CRD功能测试通过"
        kubectl delete mysqlcluster test-cluster -n ${NAMESPACE}
    else
        warn "❌ CRD功能测试失败"
    fi
    
    log "✅ 健康检查完成"
}

# 6. 设置监控告警
setup_monitoring() {
    log "📊 设置监控告警..."
    
    # 创建ServiceMonitor
    cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: ${OPERATOR_NAME}
  namespace: ${NAMESPACE}
spec:
  selector:
    matchLabels:
      control-plane: controller-manager
  endpoints:
  - port: metrics
    interval: 30s
EOF
    
    # 创建告警规则
    cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ${OPERATOR_NAME}-alerts
  namespace: ${NAMESPACE}
spec:
  groups:
  - name: ${OPERATOR_NAME}.rules
    rules:
    - alert: OperatorDown
      expr: absent(up{job="${OPERATOR_NAME}-controller-manager"} == 1)
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Operator不可用"
        
    - alert: HighReconciliationErrors
      expr: rate(mysql_operator_reconcile_errors_total[5m]) > 0.1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Operator协调错误率过高"
EOF
    
    log "✅ 监控告警设置完成"
}

# 主函数
main() {
    log "🚀 开始${OPERATOR_NAME}生产环境部署"
    log "版本: ${VERSION}"
    log "命名空间: ${NAMESPACE}"
    log "镜像仓库: ${REGISTRY}"
    echo "========================================"
    
    preflight_check
    prepare_environment
    deploy_crds
    deploy_operator
    health_check
    setup_monitoring
    
    log "🎉 ${OPERATOR_NAME}生产环境部署完成!"
    log "部署时间: $(date)"
}

# 错误处理
trap 'error "部署过程中发生错误，请检查日志"' ERR

# 执行主函数
main "$@"
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-15-specialized-tech KUDIG Database — Global MOC
- [[domain-15-specialized-tech/README|Domain-10: Kubernetes 扩展生态]]
- Domain-10 扩展与自定义 — 开源项目索引
- CRD 自定义资源定义开发指南
- 03 - 准入控制器(Webhook)配置与实现
- Kubernetes API 聚合扩展机制详解
- 包管理与应用分发工具
- 47 - Helm Chart开发与管理
- 129 - Helm 高级运维：复杂部署、CI/CD 集成与安全最佳实践
- CI/CD 管道
- 48 - GitOps工作流
- 103 - 容器镜像构建工具 (Container Image Build)

## See Also

- 99-serverless-faas-guide
- 01-crd-development-guide
- 03-admission-webhook-configuration
- 04-api-aggregation-extension
