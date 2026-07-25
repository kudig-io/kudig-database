---
title: Kubernetes 源码整体架构与目录结构解析
description: 基于 kubernetes-1.36.2 完整源码树的仓库布局、模块划分、staging 机制与构建体系深度解析
summary: 以本地源码树 33-源码/控制平面/kubernetes-1.36.2 为事实来源，解析 cmd/pkg/staging/plugin/api 五大层次的职责边界、go.work 多模块工作区、staging 独立发布机制与源码阅读路径。
category: source-analysis
tags:
- k8s
- source-code
- architecture
- staging
- client-go
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 源码目录结构如何组织
- cmd pkg staging 目录分别放什么
- staging 机制与 k8s.io/client-go 的关系
- 如何开始阅读 Kubernetes 源码
trigger_keywords:
- 源码结构
- source tree
- staging
- go.work
- cmd
- pkg
- vendor
related_domains:
- 集群基础
- 平台工程
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# Kubernetes 源码整体架构与目录结构解析

> **本地源码树**：`33-源码/控制平面/kubernetes-1.36.2/`（314MB，含 vendor）
> 本系列所有代码引用均基于该版本实测验证，函数行号可直接跳转。

## 概述

Kubernetes 是一个约 500 万行 Go 代码的巨型单体仓库（monorepo），但其内部通过清晰的分层将「组件入口、业务逻辑、可复用框架、API 契约」解耦。理解目录结构是源码阅读的第一步——**读错目录层次，就会陷入实现细节而丢失架构主线**。

本文回答三个问题：

1. 各顶层目录的职责边界是什么？
2. staging 机制为何存在，`k8s.io/*` 模块如何从单体仓库发布？
3. 面对一个生产问题，应从哪个目录切入？

---

## 一、顶层目录全景

```
kubernetes-1.36.2/
├── api/          # OpenAPI/Swagger 规范快照（非 Go 代码，是 API 契约的机器可读形式）
├── build/        # 容器化构建脚本（build/run.sh 在容器内编译）
├── cluster/      # 遗留集群部署脚本（gce/ 等，逐步废弃中）
├── cmd/          # ★ 所有二进制组件的 main 入口（薄壳层）
├── hack/         # 开发者脚本（codegen、verify、update-* 系列）
├── pkg/          # ★ 核心业务逻辑（不可被外部导入）
├── plugin/       # ★ 授权/准入等内置插件（RBAC 授权器在此）
├── staging/      # ★ 以 k8s.io/* 名义独立发布的库（client-go 等）
├── test/         # e2e、integration、conformance 测试
├── third_party/  # 第三方非 Go 依赖（protobuf 等）
├── vendor/       # Go 依赖快照（含 staging 的软链接）
├── go.mod        # 主模块 k8s.io/kubernetes
└── go.work       # Go 多模块工作区（聚合 staging 各模块）
```

四个星标目录构成源码阅读的主战场，其依赖方向严格单向：

```
cmd/ ──→ pkg/ ──→ staging/src/k8s.io/*
 │                      ▲
 └── plugin/ ───────────┘

规则：staging 不得反向依赖 pkg/（由 hack/verify-* 脚本强制）
```

---

## 二、cmd/ — 组件入口层

`cmd/` 下每个子目录对应一个可执行文件，遵循统一模式：**main 函数只做一件事——构造 cobra Command 并执行**，全部业务逻辑下沉到 `app/` 子包或 `pkg/`。

| 目录 | 二进制 | 入口链路 | 业务逻辑位置 |
|------|--------|---------|-------------|
| `cmd/kube-apiserver/` | kube-apiserver | `apiserver.go` → `app/server.go` | `pkg/controlplane/` + `staging/.../apiserver` |
| `cmd/kube-controller-manager/` | kube-controller-manager | `controller-manager.go` → `app/controllermanager.go` | `pkg/controller/` |
| `cmd/kube-scheduler/` | kube-scheduler | `scheduler.go` → `app/server.go` | `pkg/scheduler/` |
| `cmd/kubelet/` | kubelet | `kubelet.go` → `app/server.go` | `pkg/kubelet/` |
| `cmd/kube-proxy/` | kube-proxy | `proxy.go` → `app/server.go` | `pkg/proxy/` |
| `cmd/kubeadm/` | kubeadm | `kubeadm.go` → `app/cmd/` | `cmd/kubeadm/app/phases/`（自包含） |
| `cmd/kubectl/` | kubectl | `kubectl.go` | `staging/src/k8s.io/kubectl/` |
| `cmd/cloud-controller-manager/` | CCM 示例 | `main.go` | `staging/src/k8s.io/cloud-provider/` |

**阅读技巧**：每个组件的 `app/options/` 包定义了全部启动参数及其默认值，是排查「组件行为与预期不符」时的第一站——先确认参数解析出的 completed config，再追执行逻辑。

### 实测入口示例（kube-apiserver）

```go
// cmd/kube-apiserver/app/server.go:148（实测行号）
func Run(ctx context.Context, opts options.CompletedOptions) error {
    // 构造三层服务器链：AggregatorServer → KubeAPIServer → APIExtensionsServer
    ...
}

// cmd/kube-apiserver/app/server.go:176
func CreateServerChain(config CompletedConfig) (*aggregatorapiserver.APIAggregator, error)
```

详见 [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|kube-apiserver 源码深度剖析]]。

---

## 三、pkg/ — 核心业务逻辑层

`pkg/` 是各组件真正的实现，**导入路径 `k8s.io/kubernetes/pkg/...` 不保证任何 API 稳定性**，外部项目不应依赖（这正是 staging 存在的原因）。

### 关键子包与生产排障映射

| 包 | 职责 | 典型生产问题切入点 |
|----|------|-------------------|
| `pkg/controlplane/` | API Server 实例组装、内置资源 REST 注册 | 某资源 API 不可用 |
| `pkg/registry/` | 各资源的存储策略（Strategy）与 REST 实现 | 字段被静默丢弃/默认值异常 |
| `pkg/controller/` | 全部内置控制器（40+ 个子目录） | 工作负载不收敛、GC 误删 |
| `pkg/scheduler/` | 调度框架、队列、插件 | Pending Pod、调度倾斜 |
| `pkg/kubelet/` | Pod 生命周期、PLEG、驱逐、cgroup | Pod 卡 ContainerCreating、节点 NotReady |
| `pkg/proxy/` | iptables/ipvs/nftables 三种代理模式 | Service 不通、conntrack 泄漏 |
| `pkg/volume/` | 卷插件框架与 CSI 适配 | 挂载失败、卸载卡住 |
| `pkg/features/` | 全部 Feature Gate 定义（`kube_features.go`） | 确认某功能在当前版本的成熟度 |
| `pkg/apis/` | 内部版本（`__internal`）API 类型与转换 | 版本转换/字段裁剪问题 |

**内外部类型的双轨制**：`pkg/apis/core/types.go` 是内部类型（hub），`staging/src/k8s.io/api/core/v1/types.go` 是外部版本（spoke）。API Server 内部统一使用内部类型运算，出入口经 conversion 转换。这是理解「为什么同一个字段在不同版本 API 中表现不同」的关键。

---

## 四、staging/ — 独立发布机制

### 4.1 为什么需要 staging

早期生态项目直接 import `k8s.io/kubernetes`，导致依赖地狱。staging 机制将可复用代码放在 `staging/src/k8s.io/<module>/`，由机器人同步到独立仓库（如 `kubernetes/client-go`），外部以 `k8s.io/client-go v0.36.x` 引用。

本仓库内部通过 `go.work` + `vendor/k8s.io/client-go`（软链接指向 staging）消费同一份代码——**修改 staging 立即对主仓库生效，无需发版**。

### 4.2 核心 staging 模块地图

| 模块 | 职责 | 本系列关联文档 |
|------|------|---------------|
| `k8s.io/api` | 全部外部版本 API 结构体（纯类型，零逻辑） | — |
| `k8s.io/apimachinery` | Scheme/Codec、meta.v1、runtime.Object、watch 接口 | [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|声明式 API 与 Informer 机制]] |
| `k8s.io/client-go` | RESTClient、Informer、WorkQueue、LeaderElection | 同上 |
| `k8s.io/apiserver` | 通用 API Server 框架（handler 链、存储、准入） | [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|kube-apiserver 剖析]] |
| `k8s.io/component-base` | 日志/指标/配置等组件基座 | — |
| `k8s.io/kubectl` | kubectl 全部子命令实现 | — |
| `k8s.io/cri-api` | CRI gRPC 协议定义 | [[14-容器运行时/README.md|容器运行时域]] |
| `k8s.io/csi-translation-lib` | in-tree → CSI 迁移 | [[06-存储/README.md|存储域]] |

### 4.3 staging 同步约束

```bash
# staging 代码修改后需执行（hack/ 目录）
hack/update-vendor.sh          # 重建 vendor 软链
hack/verify-staging-meta-files.sh
# CI 通过 hack/verify-import-boss.sh 阻止 staging → pkg 的反向依赖
```

---

## 五、plugin/ — 内置策略插件

容易被忽略但生产价值极高的目录：

```
plugin/pkg/
├── auth/authorizer/rbac/     # RBAC 授权器实现（授权判定逻辑在这里，不在 pkg/auth）
└── admission/                # 内置准入插件
    ├── noderestriction/      # NodeRestriction（kubelet 越权防护）
    ├── podsecurity/          # Pod Security Admission
    ├── resourcequota/        # 配额准入
    └── serviceaccount/       # SA token 自动注入
```

排查「RBAC 明明配了却 403」时，真正的判定代码在 `plugin/pkg/auth/authorizer/rbac/rbac.go`，而规则求解在 `pkg/registry/rbac/validation/rule.go`。

---

## 六、构建与代码生成体系

| 机制 | 位置 | 说明 |
|------|------|------|
| Makefile | 根目录 | `make WHAT=cmd/kubelet` 单组件编译 |
| 容器化构建 | `build/run.sh` | 保证与 CI 一致的工具链 |
| deepcopy/defaulter/conversion 生成 | `hack/update-codegen.sh` | `zz_generated.*.go` 均为生成物，勿手改 |
| OpenAPI 生成 | `api/openapi-spec/` | `kubectl explain` 的数据源 |
| Feature Gate 清单 | `pkg/features/kube_features.go` | 每版本审计新增/毕业/移除的开关 |

**识别生成代码**：文件名前缀 `zz_generated.` 或文件头 `// Code generated by ... DO NOT EDIT.`。阅读源码时可整体跳过，但要知道 deepcopy 的存在解释了「为什么 Informer 缓存对象不能直接修改」——修改前必须 `DeepCopy()`。

---

## 七、源码阅读路径建议

按「先抽象、后实现」的顺序，推荐四条渐进路线：

```
路线 A（机制基础，必读）:
  apimachinery/runtime → client-go/tools/cache (Reflector/DeltaFIFO/Informer)
  → client-go/util/workqueue → 任一简单控制器 (pkg/controller/replicaset)

路线 B（请求生命周期）:
  cmd/kube-apiserver/app/server.go → staging/.../apiserver/pkg/server/config.go
  (DefaultBuildHandlerChain) → pkg/registry/... → staging/.../storage/etcd3

路线 C（调度）:
  pkg/scheduler/scheduler.go → schedule_one.go → framework/runtime/framework.go
  → backend/queue/scheduling_queue.go

路线 D（节点侧）:
  pkg/kubelet/kubelet.go (syncLoop:2620) → pod_workers.go → kuberuntime/
  → CRI (staging/src/k8s.io/cri-api)
```

每条路线对应的深度剖析见本系列后续文档。

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|02 - kube-apiserver 源码深度剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/03-kube-controller-manager-deep-dive.md|03 - kube-controller-manager 源码深度剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/04-kube-scheduler-deep-dive.md|04 - kube-scheduler 源码深度剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/05-etcd-storage-deep-dive.md|05 - etcd 与存储链路源码剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 - 声明式 API 与 Informer 机制源码剖析]]
- [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 - 组件交互关系与数据流向]]
- [[01-集群基础/01-架构总览/04-source-code-structure.md|集群基础：源码结构深度解析]]（宏观表格版）
- [[01-集群基础/02-设计原则/09-source-code-walkthrough.md|设计原则：源码阅读指南]]
