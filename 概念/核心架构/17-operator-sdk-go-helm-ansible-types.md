---
sources:
- "专项技术/扩展机制/17-operator-sdk-go-helm-ansible-types.md"
title: Operator SDK 三种类型 (Go/Helm/Ansible)
summary: 解析 Operator SDK 的 Go、Helm、Ansible 三类 operator 的开发流程、生成物差异与选型决策。
category: concepts
tags:
- operator-sdk
- operator
- go
- helm
- ansible
- kubebuilder
- controller-runtime
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- Operator 开发者
- 平台工程师
- 架构师
estimated_read_time: 22min
intent_queries:
- Operator SDK 是什么
- Go Helm Ansible operator 区别
- 如何选择 operator 类型
- Helm operator 如何工作
trigger_keywords:
- Operator SDK
- Go Operator
- Helm Operator
- Ansible Operator
- kubebuilder
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维与开发命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证；镜像/Helm chart/Ansible Galaxy 来源是否可信。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集或本地脚手架生成，无副作用）。

# Operator SDK 三种类型 (Go / Helm / Ansible)

> **适用版本**: Kubernetes v1.28 - v1.33 | **Operator SDK**: v1.30+ / v1.40+ | **最后更新**: 2026-07 | **参考**: [sdk.operatorframework.io](https://sdk.operatorframework.io/) | [operator-framework/operator-sdk](https://github.com/operator-framework/operator-sdk)

<!-- chunk: 概述 -->
## 概述

`02-operator-development-patterns.md` 用一张对比表带过了 Operator SDK，本篇做**补强专题**：聚焦 Operator SDK 提供的**三种 operator 类型**——Go、Helm、Ansible——的开发流程、生成物差异、工作机制与选型决策。

**核心一句话**：Operator SDK 是 [Operator Framework](https://operatorframework.io/) 的开发工具链，它让你不必手写 controller boilerplate，而是通过 `init` + `create api` 两步脚手架出一个完整 operator；其中：

- **Go Operator**：全功能编程，底层基于 kubebuilder + controller-runtime，**生产级首选**。
- **Helm Operator**：无代码，复用已有 Helm chart，把 CR 变化映射为 `helm install/upgrade`。
- **Ansible Operator**：声明式，把 CR 变化映射为 Ansible role/playbook 执行，**运维团队无需学 Go**。

> 💡 **定位**：本文不是「Operator 模式入门」，也不是「controller-runtime 全集」，而是**回答「面对一个新需求，我该用 SDK 的哪一种类型？」**。入门请看 [[概念/operator-pattern.md|Operator 模式]]，深度 reconcile 实现请看 [[专项技术/扩展机制/02-operator-development-patterns.md|Operator 开发模式]]。

<!-- chunk: Operator SDK 是什么 -->
## Operator SDK 是什么

### Operator Framework 四件套

Operator Framework 不是单一工具，而是一整套 Operator 生命周期方案，通常拆为四块：

| 组件 | 定位 | 解决什么问题 |
|------|------|--------------|
| **SDK** | 开发态 | 脚手架 + 构建 + 打包，快速产出 operator 二进制与 OCI 镜像 |
| **OLM** (Operator Lifecycle Manager) | 运行态 | operator 的安装、升级、依赖解析、按 namespace 隔离、CRD 版本演进 |
| **Registry** | 元数据 | 存放 operator 的 catalog（bundle / catalog image），供 OLM 拉取 |
| **RukPak** | 新一代打包 | （社区演进中）统一的 provisioner，替代部分 OLM 安装路径 |

```
┌─────────────────────────────────────────────────────────────┐
│                   Operator Framework                        │
│                                                              │
│   开发 (SDK)        发布 (Registry)      运行 (OLM)          │
│  ┌──────────┐      ┌────────────┐      ┌──────────────┐     │
│  │ init     │      │ bundle image│      │ install      │     │
│  │ create   │ ───▶ │ catalog    │ ───▶ │ upgrade      │     │
│  │ build    │      │ semver     │      │ dependency   │     │
│  │ run      │      │ signing    │      │ RBAC gate    │     │
│  └──────────┘      └────────────┘      └──────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### SDK 三大核心命令

SDK 的日常开发几乎只围绕三条命令展开：

| 命令 | 作用 | 触发时机 |
|------|------|----------|
| `operator-sdk init` | 初始化项目骨架（go.mod / Makefile / Dockerfile / PROJECT） | 项目第一次创建 |
| `operator-sdk create api` | 生成一组 CRD types + controller（或 chart / role） | 每新增一个 GVK |
| `operator-sdk run` | 本地或集群内运行；`run bundle` 走 OLM 测试 | 联调 / 验收 |

这三条命令背后由 **plugin** 决定生成什么样的代码：`go/v3`、`helm/v1`、`ansible/v1` 三套 plugin 决定了 operator 的类型。

### 与 kubebuilder 的关系

这是被问得最多、也最容易被讲混的一点：

| 维度 | kubebuilder | Operator SDK |
|------|-------------|--------------|
| 出身 | Kubernetes SIG（controller-tools / controller-runtime 维护方） | Red Hat 主导，CNCF 项目 |
| 语言 | 仅 Go | Go / Helm / Ansible |
| 关系 | **被 SDK 复用**：SDK 的 Go plugin 直接调用 kubebuilder 的 scaffold 逻辑 | SDK 是**超集** |
| 版本对齐 | 自走版本号 | SDK v3+ 与 kubebuilder 的 `go/v3` plugin 共享同一套 controller-runtime scaffold |
| 打包 | 不负责 OLM bundle | 自带 `operator-sdk generate kustomize manifests` + `bundle` 命令，原生对接 OLM |

> ⚠️ **关键事实**：写 **Go Operator 时，SDK ≈ kubebuilder + OLM 打包脚手架**。两者在 Go 路径上几乎等价，差异只在「是否需要 Helm/Ansible」和「是否要 OLM bundle 自动化」。如果你永远只写 Go operator，用 kubebuilder 也没问题；一旦需要 Helm/Ansible 或 OLM 一条龙，SDK 是唯一选择。

历史脉络：

```
2018 ── kubebuilder 诞生（K8s SIG）
2018 ── Operator SDK 诞生（Red Hat），初版有 Go/Helm/Ansible
2019 ── SDK v0.x 与 kubebuilder v1 脚手架分歧
2020 ── SDK v1+ 把 Go 路径迁回 kubebuilder scaffold（共用 controller-runtime）
2021 ── SDK v3：Go/Helm/Ansible 三 plugin 统一在 v3 脚手架下，OLM v0.x 成熟
2024+ ── SDK 与 kubebuilder 持续同步 controller-runtime 版本；OLM v1 + RukPak 演进
```

<!-- chunk: 三种 operator 类型对比 -->
## 三种 Operator 类型对比（核心）

这是本篇的「重心」。请把这张表当作后续选型的基准。

| 维度 | 🟦 Go Operator | 🟩 Helm Operator | 🟧 Ansible Operator |
|------|----------------|------------------|----------------------|
| **开发语言** | Go（编程） | Helm chart（YAML 模板） | Ansible playbook / role（YAML） |
| **学习/开发成本** | 高（需 Go + controller-runtime） | 低（几乎无代码） | 中（需 Ansible + k8s 模块） |
| **灵活度** | 最高（任意逻辑、任意副作用） | 受限（等价于 `helm install/upgrade`） | 中（Ansible 模块生态覆盖广） |
| **典型生成物** | Go 二进制 + manager Deployment + CRD YAML | `helm-operator` 二进制 + 内嵌 chart + watches.yaml | `ansible-operator` 二进制 + roles/ + molecule/ |
| **底层运行时** | controller-runtime（共享缓存、workqueue） | helm.sh 库 + controller-runtime 事件桥 | ansible-runner 子进程，每次 reconcile 起一个 play |
| **Reconcile 逻辑** | 你手写 `Reconcile()` | SDK 替你写：调 helm 引擎 | SDK 替你写：调 ansible-runner |
| **状态管理** | 手写 Status 子资源 | Helm release 状态（secret 存储） | Ansible 幂等 + operator 写回 Status |
| **性能** | 最优（编译型、常驻、共享 informer 缓存） | 中（helm 渲染开销） | 较重（每次 reconcile 拉起 ansible-runner，秒级冷启动） |
| **测试方式** | `envtest`（controller-runtime 自带） + Ginkgo | `helm unittest` + `helm template` diff | `molecule`（Ansible 生态标准测试框架） |
| **OLM bundle 友好度** | 好（官方最佳实践齐全） | 好（chart 可直接打包） | 好（roles 打包进镜像） |
| **是否可调外部系统** | 任意（SDK、HTTP、gRPC、数据库…） | 受限（只能通过 helm hooks + 额外 sidecar） | 好（Ansible 模块可调任意 API/CLI） |
| **生产级长期维护** | ⭐⭐⭐⭐⭐ 首选 | ⭐⭐⭐ 看场景 | ⭐⭐⭐⭐ 看团队技能栈 |
| **典型代表项目** | Prometheus Operator、Cert-Manager、Argo CD operator | bitnami 众多 chart 包成 operator | cert-manager 早期 ansible 变体、社区中间件 operator |

### 一句话区分

- **Go**：要写代码、要做复杂状态机、要极致性能 → 你自己 reconcile。
- **Helm**：你已经有一份成熟 Helm chart，只想把它包成 CR 驱动的 operator，**一行代码都不想写**。
- **Ansible**：你的运维团队是 Ansible 原生、不懂 Go、需求中等复杂度，希望**声明式**编排 K8s 资源与外部系统。

### 同一需求、三种实现的味道差异

假设需求是「用户创建 `RedisCluster` CR，operator 拉起一主两从 Redis」：

| 步骤 | Go Operator | Helm Operator | Ansible Operator |
|------|-------------|---------------|------------------|
| 写法 | 在 `Reconcile` 里 `CreateOrUpdate` StatefulSet / Service / ConfigMap | 写/复用 Redis Helm chart，CR 字段透传为 values | 在 role 里 `k8s` 模块应用这些资源，或 `k8s` + `template` |
| 升级时 | 自己 diff、改 patch、控制滚动节奏 | `helm upgrade` 一次到位 | 重新跑一遍 play（幂等覆盖） |
| 扩展外部逻辑（如注册到 Consul） | 直接在 reconcile 里 `consul.Put(...)` | 需 helm hook + sidecar，麻烦 | `consul_kv` 模块一行搞定 |
| 失败处理 | 自己 retry、写 Conditions | helm rollback | Ansible `retries`/`until` + `failed_when` |

<!-- chunk: Go Operator 详解 -->
## Go Operator 详解

### 脚手架流程

```bash
# 🟢 低风险：本地生成脚手架，不触碰集群
operator-sdk init \
  --domain example.com \
  --repo github.com/example/app-operator \
  --license apache2 \
  --owner "Platform Team"

# 🟢 低风险：生成一组 API + Controller
operator-sdk create api \
  --group apps \
  --version v1alpha1 \
  --kind App \
  --resource \
  --controller
```

`init` 之后项目长这样（与 kubebuilder 几乎一致）：

```
app-operator/
├── api/v1alpha1/         # CRD Go types + kubebuilder marker
│   ├── app_types.go
│   ├── groupversion_info.go
│   └── zz_generated.deepcopy.go
├── internal/controller/  # Reconciler 实现
│   └── app_controller.go
├── cmd/main/             # manager 入口
│   └── main.go
├── config/               # kustomize 各层 overlay
│   ├── crd/
│   ├── default/
│   ├── manager/
│   ├── manifests/        # OLM bundle 基础
│   ├── prometheus/       # ServiceMonitor
│   ├── rbac/             # ClusterRole（由 marker 生成）
│   └── samples/
├── Dockerfile
├── Makefile              # make install / run / docker-build / bundle
├── go.mod
├── PROJECT               # SDK 项目元数据，plugin 据此增量生成
└── .dockerignore
```

> 🔑 **关键约定**：`api/v1alpha1/app_types.go` 里的 `//+kubebuilder:...` 注解是「真理之源」。RBAC (`//+kubebuilder:rbac:groups=...`)、CRD 校验 (`//+kubebuilder:validation:...`)、打印机列 (`//+kubebuilder:printcolumn`) 全部由 marker 驱动 `make manifests` 生成。

### 生成物的演进：marker 驱动

```bash
# 🟢 低风险：本地代码生成（controller-gen），无副作用
make manifests   # 生成 config/crd/bases/*.yaml
make generate    # 生成 zz_generated.deepcopy.go
```

- **CRD 定义**：`api/v1alpha1/app_types.go` 里的 `AppSpec` / `AppStatus` struct。
- **RBAC**：由 `app_controller.go` 顶部的 `//+kubebuilder:rbac` marker 生成 `config/rbac/role.yaml`。
- **Webhook**：`create webhook` 子命令单独生成 `api/v1alpha1/*_webhook.go` 与 `internal/webhook/`。
- **main.go**：组装 `manager`、注册 `Scheme`、挂 controller、挂 webhook、挂 metrics。

### controller-runtime：manager / client / reconcile

Go Operator 的运行时是 [controller-runtime](https://pkg.go.dev/sigs.k8s.io/controller-runtime)，三个核心概念：

| 概念 | 角色 | 通俗类比 |
|------|------|----------|
| **Manager** | 容器，持有 shared cache + client + healthz / metrics server，管理一组 controller | 进程骨架 |
| **Client** | 对 apiserver 读写，读写分离（cache 读、直连写） | 数据访问层 |
| **Reconciler** | 你实现的 `Reconcile(ctx, req) (Result, error)`，幂等地把世界推向期望状态 | 业务逻辑 |

#### Reconcile 模板（教科书级）

```go
package controller

import (
	"context"
	appsv1 "github.com/example/app-operator/api/v1alpha1"
	appsv "k8s.io/api/apps/v1"
	corev1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/runtime"
	ctrl "sigs.k8s.io/controller-runtime"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/controller/controllerutil"
)

type AppReconciler struct {
	client.Client
	Scheme *runtime.Scheme
}

// +kubebuilder:rbac:groups=apps.example.com,resources=apps,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=apps.example.com,resources=apps/status,verbs=get;update;patch
// +kubebuilder:rbac:groups=apps,resources=deployments,verbs=get;list;watch;create;update;patch;delete
func (r *AppReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
	// 1) 取期望状态（CR）
	var app appsv1.App
	if err := r.Get(ctx, req.NamespacedName, &app); err != nil {
		if errors.IsNotFound(err) {
			return ctrl.Result{}, nil // 已删除，无需处理
		}
		return ctrl.Result{}, err
	}

	// 2) 计算期望的实际资源
	deploy := desiredDeployment(&app)
	if err := controllerutil.SetControllerReference(&app, deploy, r.Scheme); err != nil {
		return ctrl.Result{}, err
	}

	// 3) CreateOrUpdate：幂等地让世界向期望对齐
	if _, err := controllerutil.CreateOrUpdate(ctx, r.Client, deploy, func() error {
		deploy.Spec.Replicas = app.Spec.Replicas
		// ...合并其它字段，注意保留运行时字段（如 ClusterIP）
		return nil
	}); err != nil {
		return ctrl.Result{}, err
	}

	// 4) 回写 Status（注意 resourceVersion 冲突，建议用 retry）
	app.Status.Ready = deploy.Status.ReadyReplicas == *deploy.Spec.Replicas
	_ = r.Status().Update(ctx, &app) // 生产里请加 retry on conflict

	// 5) 不 ready 就 requeue，否则交给 watch 自然触发
	if !app.Status.Ready {
		return ctrl.Result{RequeueAfter: 10 * time.Second}, nil
	}
	return ctrl.Result{}, nil
}

func (r *AppReconciler) SetupWithManager(mgr ctrl.Manager) error {
	return ctrl.NewControllerManagedBy(mgr).
		For(&appsv1.App{}).
		Owns(&appsv.Deployment{}). // 监听属主为该 CR 的 Deployment
		Complete(r)
}
```

这套写法与 kubebuilder 项目**完全一致**——这正是「SDK 的 Go 路径 ≈ kubebuilder」的具体含义。

### Go Operator 的核心优势

1. **完全控制**：任意时序、任意条件、任意副作用（调外部 API、跑 job、写 DB）。
2. **生产级能力齐全**：leader election、metrics、webhook、finalizer、status conditions、conversion webhook 都有现成范式。
3. **性能最优**：编译为单二进制，常驻进程，共享 informer cache，单实例可 watch 上万对象。
4. **生态最厚**：cert-manager、prometheus-operator、argo-rollouts、cluster-api 都是这条路。

### Go Operator 的代价

- 需要 Go 工程能力（go.mod、接口、context、错误处理）。
- 需要理解 controller-runtime 的缓存语义（cache vs live client、rate-limited workqueue、requeue 语义）。
- 升级 controller-runtime 大版本时 reconcile 签名 / 接口会变，需迁移。

<!-- chunk: Helm Operator 详解 -->
## Helm Operator 详解

### 脚手架流程

```bash
# 🟢 低风险：用 helm plugin 初始化项目
operator-sdk init \
  --plugins=helm \
  --domain example.com \
  --repo github.com/example/app-helm-operator

# 🟢 低风险：把一个已有 chart 绑定为 CR
# 方式 A：直接指向本地 chart 目录
operator-sdk create api \
  --group apps \
  --version v1alpha1 \
  --kind App \
  --helm-chart=./charts/app

# 🟢 低风险：方式 B：从远程仓库拉 chart 自动绑定
operator-sdk create api \
  --group apps \
  --version v1alpha1 \
  --kind App \
  --helm-chart=bitnami/redis
```

生成物结构与 Go Operator **完全不同**：

```
app-helm-operator/
├── helm-charts/              # 内嵌 chart 副本（init 时复制进来）
│   └── app/
│       ├── Chart.yaml
│       ├── values.yaml
│       └── templates/
├── watches.yaml              # CR GVK → chart 映射（核心）
├── config/
│   ├── crd/                  # 由 chart values schema 反推 CRD
│   ├── manager/
│   └── manifests/            # OLM bundle
├── Dockerfile                # 用 helm-operator 基础镜像
└── Makefile
```

注意：**没有 `api/`、没有 `controllers/`、没有 Go 文件**——这就是「无代码 operator」。

### watches.yaml：CR 到 chart 的桥梁

`watches.yaml` 是 Helm Operator 的灵魂，它告诉 operator「这个 GVK 的 CR 变化时，跑哪个 chart」：

```yaml
# 列表语法：每一行一组映射
- version: v1alpha1
  group: apps.example.com
  kind: App
  chart: helm-charts/app
  # 可选：把 CR 的某些字段映射到 helm values
  overrideValues:
    image.repository: $RELATED_IMAGE_APP   # 来自 OLM 注入的环境变量
  # 可选：在 release 名字里加前缀/后缀，避免多实例冲突
  releaseName: "{{ .metadata.name }}"
  # 可选：限制 CR 字段透传范围
  watchDependentResources: true
  maxConcurrentReconciles: 1
```

- **group + version + kind**：决定监听的 CR GVK。
- **chart**：相对项目根的 chart 路径，打包时打进镜像。
- **overrideValues**：把环境变量、CR spec 中的特定字段注入为 helm values。
- **watchDependentResources**：true 时 helm operator 还会监听该 release 下的资源变化，自动 reconcile。

### 工作机制（端到端）

```
  用户 apply App CR (spec: replicas=3, image=...)
            │
            ▼
   apiserver → informer → controller-runtime 事件桥
            │
            ▼
   helm-operator 读取 CR，构造 values = CR.spec（含 overrideValues）
            │
            ▼
   helm.Install / helm.Upgrade（chart + values）  ◀── 复用 helm 引擎
            │
            ▼
   生成/更新一组 K8s 资源（Deployment、Service、ConfigMap…）
            │
            ▼
   helm release 状态（pending/ deployed / failed）回写为 CR.status
```

关键点：**Helm Operator 不重新发明 reconcile，它就是把 CR 变化翻译为 helm 操作**。理解了 helm 的 install/upgrade/rollback/release 模型，就理解了 Helm Operator 的全部行为。

### Helm Operator 的适用与限制

**适用**

- 已经有成熟、有测试的 Helm chart，想低成本把它「升级」为 CR 驱动、可被 OLM 管理。
- 简单的「CR → 一组资源」映射，不需要复杂状态机。
- 团队 Helm 能力 > Go 能力。

**限制**

- **复杂业务逻辑难写**：要靠 helm 的 `pre-install`/`post-upgrade` hook + sidecar 容器拼凑，远不如 Go 里几行代码直接。
- **变量管理完全依赖 values**：所有可变点必须先在 chart 里开成 value。
- **精细 reconcile 难做**：helm 是「整体渲染 + 三方合并」，对单个字段的精细 diff 控制不如 Go。
- **多 CR 共享 release**：helm release 名字唯一，一个 namespace 内一个 chart 一个 release，跨 CR 共享需要 overrideValues 与 releaseName 巧妙设计。

### Helm Operator 的生产注意

- **CR 删除 = helm uninstall**：除非配置 `--enable-leader-election` 之外，请确认 finalizer 行为；默认会随 CR 删除把 release 一起删掉。如需保留，需用 `uninstallCRDs: false` 之外的策略，或自建 finalizer。
- **release 命名冲突**：在多 namespace 部署同 chart 时，务必用 `releaseName` 模板加入 namespace 区分。
- **values schema 必填**：没有 schema 时，CRD 会很宽松；建议给 chart 配 `values.schema.json`，让 SDK 反推出严格的 CRD 校验。

<!-- chunk: Ansible Operator 详解 -->
## Ansible Operator 详解

### 脚手架流程

```bash
# 🟢 低风险：用 ansible plugin 初始化项目
operator-sdk init \
  --plugins=ansible \
  --domain example.com \
  --repo github.com/example/app-ansible-operator

# 🟢 低风险：生成一组 CR + Ansible role
operator-sdk create api \
  --group apps \
  --version v1alpha1 \
  --kind App
```

生成物与 Helm/Go 都不同：

```
app-ansible-operator/
├── roles/
│   └── app/                 # 标准 Ansible role
│       ├── tasks/main.yml
│       ├── defaults/main.yml
│       └── files/ templates/
├── playbooks/
│   └── app.yml              # 入口 playbook（调用 role）
├── watches.yaml             # CR GVK → playbook 映射
├── molecule/                # Ansible 标准测试框架
│   └── default/
│       ├── converge.yml
│       └── verify.yml
├── config/
├── Dockerfile               # 用 ansible-operator 基础镜像
└── requirements.yml         # 外部 Galaxy roles
```

同样**没有 Go 代码**——operator 的核心逻辑就是 `roles/app/tasks/main.yml`。

### watches.yaml 与 playbook

```yaml
- version: v1alpha1
  group: apps.example.com
  kind: App
  role: roles/app            # 方式 A：直接指向 role
  # 或者：
  # playbook: playbooks/app.yml   # 方式 B：指向 playbook
  manageStatus: true         # operator 自动回写 status.conditions
  watchEnvironment: true
  maxRunnerArtifacts: 20     # ansible-runner 保留的运行产物数
  vars:                      # 注入到每个 play 的全局变量
    reconciliation_interval: 30
```

### role：声明式幂等编排

一个典型 `roles/app/tasks/main.yml`：

```yaml
---
# Ansible 的 k8s 模块直接 apply 资源，幂等
- name: Ensure namespace exists
  kubernetes.core.k8s:
    state: present
    definition:
      apiVersion: v1
      kind: Namespace
      metadata:
        name: "{{ ansible_operator_meta.namespace }}-runtime"

- name: Render and apply workload
  kubernetes.core.k8s:
    state: present
    template: app-deployment.yaml.j2   # roles/app/templates/

- name: Wait for rollout
  kubernetes.core.k8s_info:
    kind: Deployment
    namespace: "{{ ansible_operator_meta.namespace }}"
    name: "{{ ansible_operator_meta.name }}"
  register: dep
  until: dep.resources[0].status.readyReplicas | default(0) == dep.resources[0].spec.replicas
  retries: 30
  delay: 5

- name: Talk to external system (e.g. register to a CMDB)
  ansible.builtin.uri:
    url: "https://cmdb.internal/api/v1/services"
    method: POST
    body_format: json
    body: { name: "{{ ansible_operator_meta.name }}" }
```

注意几个 operator 专有变量：

- `ansible_operator_meta.name` / `ansible_operator_meta.namespace`：当前 CR 的元数据。
- `{{ lookup('env', 'POD_NAME') }}`：operator pod 自身。
- operator 自动注入 `ownerReferences`，role 创建的资源会成为 CR 的下属，CR 删除时自动级联。

### 工作机制

```
  用户 apply App CR
        │
        ▼
 ansible-operator 监听到事件 → 构造 playbook 变量（含 CR spec）
        │
        ▼
 ansible-runner 子进程：执行 playbooks/app.yml（或 role: app）
        │
        ▼
 k8s 模块幂等地 apply 资源 / 外部模块调任意 API
        │
        ▼
 playbook 输出 / failed_when / changed → operator 回写 status.conditions
```

关键差异：**每次 reconcile 都拉起一次 ansible-runner 子进程**，所以延迟和资源开销显著高于 Go Operator。这是「声明式、低代码」的代价。

### Ansible Operator 的优势

1. **运维团队无需学 Go**：用熟悉的 Ansible 表达意图。
2. **声明式 + 幂等**：Ansible 天然声明式，`state: present` 即等价于 `CreateOrUpdate`。
3. **外部系统集成强**：Ansible 模块生态（数百个 collection）覆盖网络设备、云 API、数据库、消息队列。
4. **可测试**：`molecule` 提供完整的「起 kind → converge → verify → destroy」测试链。

### Ansible Operator 的限制

- **性能开销**：每次 reconcile 启动 ansible-runner，秒级冷启动；高并发场景需谨慎。
- **复杂状态机难写**：Ansible 是「一次性执行」语义，跨 reconcile 的状态机要靠 `status` 字段 + `when` 条件手工拼。
- **错误处理弱于代码**：`retries/until` 比 Go 的 `requeue` 笨重。
- **依赖 ansible-runner 的运行产物**：artifact 多了会占磁盘，需调 `maxRunnerArtifacts`。
- **secret 管理**：Ansible 历史上对敏感变量可见性有过坑，注意 `no_log: true`。

<!-- chunk: 选型决策 -->
## 选型决策

### 决策树

```
                    新需求要做 operator?
                            │
              ┌─────────────┴──────────────┐
              ▼                            ▼
        已有成熟 Helm chart?         需要复杂状态机 / 高并发
              │                            │
            是│ 是                          是│ 是
              ▼                            ▼
        Helm Operator              Go Operator（生产首选）
              │                            │
              否                           否
              ▼                            ▼
      运维团队是 Ansible 栈?         需要调外部系统但不复杂?
              │                            │
            是│                            │
              ▼                            ▼
        Ansible Operator               倾向 Go Operator
                                       （保持长期可维护）
```

### 按场景对照表

| 你的场景 | 推荐类型 | 理由 |
|----------|----------|------|
| 中间件生命周期管理（MySQL/Redis/Kafka 主从切换） | **Go** | 状态机复杂、性能敏感 |
| 把公司内部一份成熟 chart 包成「服务化 CR」 | **Helm** | 零代码、即包即用 |
| 运维团队熟悉 Ansible，要做跨 K8s + 物理机的编排 | **Ansible** | 模块生态覆盖广、团队成本低 |
| 需要每秒 reconcile 上百个对象 | **Go** | 共享 informer、编译型性能 |
| 需要在 CR 变化时调一个非 K8s 的 REST API | **Go / Ansible** | Go 直接 SDK，Ansible 用 `uri` 模块 |
| 短期内 PoC，先跑通流程 | **Helm** | 最快出活 |
| 长期生产、多版本 CRD 演进、需要 conversion webhook | **Go** | 生态最厚、迁移成本最低 |

### 一句话结论

> **长期生产首选 Go；已有 chart 选 Helm；运维原生选 Ansible。** 三者不是互斥对立，同一组织内完全可以并存——核心服务用 Go，周边打包用 Helm，跨域编排用 Ansible。

<!-- chunk: 生产实践 -->
## 生产实践

### 通用：OLM 打包与分发

无论哪种类型，最终生产部署首选 OLM（Operator Lifecycle Manager）。SDK 自带 bundle 流程：

```bash
# 🟡 中风险：本地构建镜像并推送，会改镜像仓库
make docker-build docker-push IMG=ghcr.io/example/app-operator:v0.1.0

# 🟢 低风险：本地生成 OLM bundle 目录
make bundle IMG=ghcr.io/example/app-operator:v0.1.0

# 🟢 低风险：构建 bundle 镜像
make bundle-build bundle-push BUNDLE_IMG=ghcr.io/example/app-operator-bundle:v0.1.0

# 🟡 中风险：在集群里通过 OLM 安装该 bundle（会创建 subscription）
operator-sdk run bundle ghcr.io/example/app-operator-bundle:v0.1.0 \
  --namespace operators --service-account default

# 🟢 低风险：清理 OLM 测试
operator-sdk cleanup app-operator -n operators
```

OLM 的价值在于：依赖解析、按 namespace 隔离、CRD 版本升级策略（`spec.customresourcedefinitions.owned`）、自动 RBAC gate。

### Go Operator 生产要点

```bash
# 🟡 中风险：开启 leader election 部署（多副本互斥 reconcile）
make deploy IMG=ghcr.io/example/app-operator:v0.1.0
# 在 config/manager/manager.yaml 里加 args:
#   - --leader-elect
#   - --leader-election-id=app-operator.example.com

# 🟢 低风险：本地跑 envtest（controller-runtime 自带 apiserver+etcd）
make install
make run ENABLE_WEBHOOKS=false
# 或 envtest：
make test
```

生产必备清单（Go）：

- **Leader election**：避免多副本重复 reconcile。
- **Metrics + Prometheus ServiceMonitor**：`config/prometheus/` 已生成。
- **Webhook**：conversion（多版本 CRD）、validating、mutating 三类，用 `create webhook` 生成。
- **Status Conditions**：用 `meta.SetStatusCondition` 规范化，便于上层告警。
- **Finalizer**：删除前的清理（外部系统注销、卷回收）。
- **Pprof / healthz / readyz**：manager 自动暴露，记得在 Deployment 加 liveness/readiness。
- **Resource requests/limits**：controller 默认不限内存，长期跑会泄漏 informer cache，务必设。

### Helm Operator 生产要点

```bash
# 🟢 低风险：本地渲染验证（不 apply，只看产出）
helm template helm-charts/app -f values.yaml

# 🟢 低风险：在集群里看 release 与 CR 的对应关系
helm list -A | grep app-

# 🟡 中风险：升级 chart 后让 operator 自动滚 release
# 修改 helm-charts/app/Chart.yaml 的 version，重新 make docker-build
```

生产清单（Helm）：

- **values.schema.json**：把 chart 的可变字段文档化并让 CRD 严格校验。
- **release 命名**：多 namespace 部署时用 `releaseName: "{{ .metadata.namespace }}-{{ .metadata.name }}"`。
- **依赖 chart 版本锁死**：`Chart.lock` + 版本号固定，避免 helm 拉到不兼容版本。
- **生命周期对齐**：CR 删除默认 `helm uninstall`，若需保留资源请设计 finalizer 或 `keep` annotation。
- **hook 用量克制**：`pre-install`/`post-upgrade` hook 多了会放大调试难度。

### Ansible Operator 生产要点

```bash
# 🟢 低风险：molecule 本地测试（起 kind → converge → verify）
molecule test

# 🟡 中风险：本地直接以 ansible 方式跑一次 play，快速验证 role
cd roles/app && ansible-playbook -i localhost, ../../playbooks/app.yml \
  -e "ansible_operator_meta={name=app-demo,namespace=default}"

# 🟢 低风险：查看 ansible-runner 产物（在 operator pod 里）
kubectl exec -n operators deploy/app-ansible-operator-controller-manager \
  -- ls /tmp/ansible-operator/runner
```

生产清单（Ansible）：

- **`maxRunnerArtifacts` 控盘**：artifact 会积累，调小（如 5）并定期清理。
- **`no_log: true`**：涉及 secret 的 task 一定要加，否则输出会泄漏。
- **`async` + `poll`**：长时间 task（如数据迁移）用异步，避免 reconcile 卡死。
- **`retries/until`** 替代 requeue：Ansible 侧轮询比交给 operator requeue 更直观。
- **inventory 与目标集群**：operator 已注入 in-cluster kubeconfig，role 里直接用 `kubernetes.core.k8s` 即可，无需写 host。
- **molecule 必备**：每个 role 至少一个 `molecule/default` 场景，CI 里卡住。

### 三类 Operator 的测试栈对照

| 类型 | 单元测试 | 集成测试 | 端到端 |
|------|----------|----------|--------|
| **Go** | Ginkgo / Gomega（controller-runtime 默认） | `envtest`（本地起 apiserver+etcd） | kind 集群 + OLM `run bundle` |
| **Helm** | `helm unittest`（chart 字段断言） | `helm template` diff / `helm install --dry-run` | kind + operator 部署 |
| **Ansible** | `ansible-test sanity`（语法/lint） | `molecule`（kind converge+verify） | kind + OLM |

<!-- chunk: 排障 -->
## 排障

### 通用：先看 operator 日志

```bash
# 🟢 低风险：查看 operator reconcile 日志
kubectl logs -n operators deploy/app-operator-controller-manager -c manager --tail=100

# 🟢 低风险：跟随日志
kubectl logs -n operators deploy/app-operator-controller-manager -c manager -f

# 🟢 低风险：看 manager 是否 leader（Go）
kubectl get lease -n operators
kubectl get lease app-operator.example.com -n operators -o yaml
```

### 各类型特征性故障

#### Go Operator

| 现象 | 可能原因 | 排查 |
|------|----------|------|
| CR 一直不 ready，controller 没动作 | RBAC 权限不足，workqueue 一直 backoff | `kubectl describe clusterrolebinding <operator>`；看日志里的 `forbidden` |
| reconcile 报错循环（日志刷屏） | reconcile 里 panic 或 requeue 立即返回 | 看日志 stack trace；检查 `Get` 后是否处理 NotFound |
| Status 不更新 | 没注册 status subresource，或 resourceVersion 冲突 | `make manifests` 确认 CRD 有 `status`；用 `retry.RetryOnConflict` 更新 |
| webhook 不生效 | webhook cert 没挂上，或 ca-injection 缺失 | `kubectl get validatingwebhookconfigurations`；检查 `cert-manager.io/inject-ca-from` |

```bash
# 🟢 低风险：查看 CRD 是否注册了 status subresource
kubectl get crd apps.example.com -o jsonpath='{.spec.versions[*].subresources}'

# 🟢 低风险：reconcile 错误指标（Prometheus）
kubectl exec -n operators deploy/prometheus -- \
  promtool query instant http://localhost:9090 \
  'rate(controller_runtime_reconcile_errors_total[5m])'
```

#### Helm Operator

| 现象 | 可能原因 | 排查 |
|------|----------|------|
| CR status 一直 `pending-install` | helm 渲染失败、缺 value | 看 operator 日志里的 `helm: ... error`；`helm template` 复现 |
| release 被反复 upgrade | `watchDependentResources: true` 下子资源抖动 | 调整 chart 让资源幂等；临时关 `watchDependentResources` |
| CR 删除后资源没删 | chart 用了 `resources: []` 或 `lookup` 跳过 | 检查 chart 模板的 `if` 分支；确认 release owner 链 |
| 多 namespace 同 chart 冲突 | release 名字撞 | `watches.yaml` 里加 `releaseName` 模板 |

```bash
# 🟢 低风险：列出所有 helm release（A=所有 namespace）
helm list -A | grep app-

# 🟢 低风险：看某个 release 的状态
helm status app-demo -n default

# 🟡 中风险：release 卡住时回滚（会改集群状态）
helm rollback app-demo 1 -n default
```

#### Ansible Operator

| 现象 | 可能原因 | 排查 |
|------|----------|------|
| reconcile 延迟高 | ansible-runner 冷启动 + 大 inventory | 调 `maxConcurrentReconciles`；精简 role |
| play 输出里有 secret 明文 | task 缺 `no_log` | 给涉 secret 的 task 全部加 `no_log: true` |
| role 失败但 status 没更新 | `manageStatus: false` 或 playbook 没写回 | 看 `watches.yaml`；在 role 末尾用 `operator_sdk.util` collection 写 conditions |
| artifact 把磁盘打满 | `maxRunnerArtifacts` 过大 | 调小并定期 `kubectl exec` 清理 |

```bash
# 🟢 低风险：operator 日志含完整 playbook 输出
kubectl logs -n operators deploy/app-ansible-operator-controller-manager -c manager -f

# 🟢 低风险：进 pod 看 ansible-runner 运行产物
kubectl exec -n operators deploy/app-ansible-operator-controller-manager -- \
  ls -la /tmp/ansible-operator/runner/apps.example.com/v1alpha1/App/

# 🟢 低风险：单独跑一次 play 复现
kubectl exec -n operators deploy/app-ansible-operator-controller-manager -- \
  ansible-playbook /opt/ansible/playbooks/app.yml \
  -e "ansible_operator_meta={name=app-demo,namespace=default}"
```

### 共性问题速查

| 现象 | 三类通杀的排查 |
|------|----------------|
| CR 创建无反应 | ① CRD 是否安装 `kubectl get crd`；② operator pod 是否 Running；③ RBAC `kubectl auth can-i` |
| reconcile 报错循环 | 日志看 root cause；Go 看 stack trace，Helm 看 helm error，Ansible 看 play failed task |
| CR 删除卡住 | finalizer 未完成；`kubectl patch <cr> -p '{"metadata":{"finalizers":[]}}' --type=merge` 可强制（🔴 高风险） |
| OLM 安装失败 | `operator-sdk run bundle` 日志；`kubectl get installplan -n operators -o yaml` |

```bash
# 🔴 高风险：强制删除卡住的 CR（去掉 finalizer，可能留下孤儿资源）
kubectl patch app.apps.example.com app-demo --type=merge \
  -p '{"metadata":{"finalizers":[]}}'

# 🟢 低风险：检查 operator 的 serviceaccount 是否具备 CR 权限
kubectl auth can-i create apps.example.com --as=system:serviceaccount:operators:default
```

<!-- chunk: 版本兼容性 -->
## 版本兼容性与演进

### SDK / kubebuilder / controller-runtime 版本对齐

| Operator SDK | kubebuilder plugin | controller-runtime | Go Operator 行为 |
|--------------|--------------------|--------------------|------------------|
| v1.30 | `go/v4` | v0.16+ | `internal/controller/` 布局、Reconcile(ctx, req) |
| v1.33+ | `go/v4` | v0.17+ | controller-runtime `ctrl.Builder` API 收敛 |
| v1.40+ | `go/v4` | v0.18+ | cache 选项细化、webhook 证书注入改进 |

> 📌 **升级建议**：跨大版本（如 v1.2x → v1.3x）升级前，先读 SDK RELEASENOTES 里 `controller-runtime` 与 `scaffold` 的 breaking changes，再跑 `operator-sdk init` 在临时目录生成新骨架，与现有代码 diff。

### Helm / Ansible Operator 的稳定性

- **Helm Operator**：自 SDK v1.0 起 `watches.yaml` schema 稳定，`overrideValues`、`watchDependentResources` 均向后兼容。
- **Ansible Operator**：基于 `ansible-runner`，operator 基础镜像版本与 Ansible 版本耦合（2.14+）；升级时注意 `kubernetes.core` collection 版本。

### OLM v1 与 RukPak 演进

OLM v0（当前主流）→ OLM v1 + RukPak 是社区演进方向。OLM v1 更轻量、更强的 provisioner 抽象，但对 Operator SDK 产出的 bundle 仍然兼容，短期无需改造。

<!-- chunk: 反模式 -->
## 常见反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| 用 Helm Operator 做复杂状态机 | hook 越加越多，难维护 | 早期就上 Go Operator |
| Ansible Operator 里 `shell` 模块滥用 | 幂等性失控、性能差 | 用 `k8s`/`uri` 等声明式模块 |
| Go reconcile 里 `time.Sleep` 阻塞 | 阻塞 workqueue，影响其它 CR | 用 `RequeueAfter` 返回 |
| 不写 Status Conditions | 上层告警无据可依 | 规范化 `conditions` 字段 |
| Helm chart 改 values 不改 Chart 版本 | release 升级幂等检测失效 | values 改动同步改 `Chart.yaml` patch 版本 |
| Ansible role 不写 molecule | CI 通过但线上炸 | 每个 role 至少一个 molecule 场景 |
| 三个类型混用一份 CRD | 字段语义对不上 | 一个 GVK 对应一个类型，明确边界 |

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[专项技术/扩展机制/02-operator-development-patterns.md|Operator 开发模式]]
- [[专项技术/扩展机制/01-crd-development-guide.md|CRD 开发指南]]
- [[集群基础/设计原则/12-operator-development-guide.md|Operator 开发指南]]
- [[概念/operator-pattern.md|Operator 模式]]
- [[发布变更/GitOps/13-helm-production-patterns.md|Helm 生产模式]]
- [[专项技术/扩展机制/03-admission-webhook-configuration.md|准入 Webhook 配置]]
- [[专项技术/扩展机制/05-package-management-tools.md|包管理工具]]

## See Also

- [[概念/operator-pattern.md|Operator 模式]]
- [[集群基础/设计原则/03-controller-pattern.md|控制器模式]]
- [[集群基础/设计原则/05-informer-workqueue.md|Informer 与 WorkQueue]]
- [[发布变更/GitOps/01-argo-cd-enterprise-gitops.md|ArgoCD 企业 GitOps]]

## 参考链接

- [Operator SDK 官方文档](https://sdk.operatorframework.io/docs/building-operators/)
- [Operator Framework GitHub](https://github.com/operator-framework)
- [controller-runtime](https://github.com/kubernetes-sigs/controller-runtime)
- [kubebuilder book](https://book.kubebuilder.io/)
- [Helm Operator 工作原理](https://sdk.operatorframework.io/docs/building-operators/helm/)
- [Ansible Operator 工作原理](https://sdk.operatorframework.io/docs/building-operators/ansible/)
- [OLM (Operator Lifecycle Manager)](https://olm.operatorframework.io/)

<!-- risk-assessed -->
