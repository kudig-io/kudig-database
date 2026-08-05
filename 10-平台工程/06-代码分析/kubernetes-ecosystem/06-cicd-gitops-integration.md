---
title: CI/CD 与 GitOps 集成源码分析
description: 基于 helm-4.2.2、argo-cd-3.4.5 本地源码与 Tekton/Jenkins 机制级分析的 K8s 交付链路剖析：push/pull 两种模式、Helm release 机制、GitOps 调谐与流水线 Pod 化
summary: 对比 push（CI 直推）与 pull（GitOps 控制器）两种交付模式与 K8s 的集成点：Helm 的 release 存储与三方合并、ArgoCD application-controller 的 refresh/compare/sync 调谐循环（两者行号均实测）、Tekton 把流水线翻译为 Pod、Jenkins/GitLab Runner 动态 agent，给出交付链路排障方法。
category: source-analysis
tags:
- k8s
- source-code
- helm
- argocd
- tekton
- jenkins
- gitops
- cicd
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- ArgoCD 如何检测配置漂移
- Helm release 存在哪里
- Tekton 流水线如何变成 Pod
- GitOps 与传统 CI/CD 推送模式区别
trigger_keywords:
- GitOps
- ArgoCD
- Tekton
- Helm
- Jenkins
- GitLab Runner
- OutOfSync
- release
related_domains:
- 发布变更
- 清单模式
- 平台工程
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# CI/CD 与 GitOps 集成源码分析

> **源码基线**：`33-源码/平台工程/{helm-4.2.2,argo-cd-3.4.5}/`（行号实测）；Tekton/Jenkins/GitLab 为机制级分析（源码树待入库，见 [[33-源码/README.md|33-源码 待补充清单]]）
> 本篇属 [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 生态集成系列]]。

## 一、push 与 pull：两种交付模式的集成点差异

| | push（Jenkins/GitLab CI 直推） | pull（ArgoCD/Flux GitOps） |
|---|------------------------------|---------------------------|
| 集成点 | CI runner 持集群凭证调 kubectl/helm | 集群内控制器 watch Git + watch 集群 |
| 凭证方向 | 集群凭证外泄到 CI 系统（攻击面大） | Git 只读凭证进集群，集群凭证不出门 |
| 漂移处理 | 无感知——人工 kubectl 改动永久漂移 | 持续 diff，可自动回正（selfHeal） |
| 审计 | 散落在 CI 日志 | Git history = 完整变更审计 |
| 本质 | 命令式「执行一次」 | 声明式「持续调谐」——即控制器模式（[[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|06 篇]]）在交付域的复刻 |

## 二、Helm：两种模式共用的模板引擎

```go
// helm-4.2.2/pkg/action/（实测行号）
func (i *Install) Run(chrt ci.Charter, vals map[string]any)                       // install.go:275
func (i *Install) RunWithContext(ctx, ch, vals)                                   // install.go:284 渲染→校验→提交→hook
func (u *Upgrade) RunWithContext(ctx, name, ch, vals)                             // upgrade.go:170 三方合并→patch
```

- **release 状态存在集群内**：默认每个 release 版本一个 Secret（`sh.helm.release.v1.<name>.v<N>`，gzip+base64 的完整 manifest）——`helm rollback` 就是取旧 Secret 重放，Secret 被清理策略误删 = release 历史丢失
- **Upgrade 是三方合并**（:170）：old manifest / new manifest / live state 三方对比生成 patch——这让 helm 能容忍集群内的字段级漂移，但也意味着「手工改过的字段在下次 upgrade 时可能被回写」
- **hook 是注解驱动的命令式插层**：`helm.sh/hook: pre-upgrade` 的 Job 由 helm 客户端串行创建并等待——hook 失败即 release 失败，是 CI 卡「upgrade 超时」的高频原因（另一个是 `--wait` 等 Pod ready 而 Pod 起不来）
- ArgoCD 用 Helm 时**只取渲染结果**（`helm template`），release Secret 不存在——`helm list` 看不到 ArgoCD 管的「helm 应用」是设计使然，不是故障

## 三、ArgoCD：GitOps 调谐循环（argo-cd-3.4.5，行号实测）

```
repo-server：git clone + helm/kustomize 渲染 → 目标 manifest
application-controller：
    ① 渲染结果 vs 集群 live state 做 diff（server-side dry-run + 忽略规则归一化）
    ② OutOfSync → 按 sync waves/hooks 顺序 kubectl apply（SSA）
    ③ health 评估（内置 lua 规则：Deployment 看 availableReplicas 等）
```

调谐循环的源码落点：

```go
// controller/appcontroller.go
func NewApplicationController(...)                                  // :152  控制器装配（informer/队列/缓存）
func (ctrl *ApplicationController) processAppRefreshQueueItem()     // :1693 refresh 主循环：取渲染+比对+写状态
func (ctrl *ApplicationController) autoSync(app, syncStatus, ...)   // :2186 自动同步决策（prune/selfHeal 在此把门）
func (ctrl *ApplicationController) processRequestedAppOperation(app)// :1430 执行 sync operation（手动/自动同一入口）

// controller/state.go
func (m *appStateManager) CompareAppState(app, project, revisions, ...)  // :559 核心 diff：目标 manifest vs live state

// controller/sync.go
func (m *appStateManager) SyncAppState(app, project, state)         // :107 把 diff 结果交给 gitops-engine 按 wave 顺序 apply

// reposerver/repository/repository.go
func (s *Service) GenerateManifest(ctx, q)                          // :593 repo-server 渲染入口（helm template/kustomize build）
```

两个结构性事实：

1. **refresh 与 sync 是两条队列**：:1693 只算状态（OutOfSync/Synced），真正写集群的是 :1430；手动点 Sync 和 autoSync（:2186）最终都落到同一个 operation 执行入口——排查「sync 卡住」看 operation state 而非 refresh 日志
2. **apply 能力下沉到 gitops-engine**（仓内 `gitops-engine/` 子模块，go.mod replace 指向本地）：SyncAppState（:107）只编排 wave/hook 顺序，SSA patch、prune、健康等待由 engine 完成，Flux 的同类能力也源自相同抽象

与 K8s 的源码级接触面全部经 apiserver：watch 用集群级 informer 缓存全部托管资源（大集群下 controller 内存与 `--app-resync` 周期是主要调优点）；写入用 Server-Side Apply（[[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|02 篇]] managedFields 机制）——**永久 OutOfSync 的第一大根因就是字段属主冲突**：HPA 改 replicas、webhook 注入 sidecar、控制器回写默认值，都会让 live 与 Git 永远不等，解法是 `ignoreDifferences` 而非硬 sync。

sync waves（`argocd.argoproj.io/sync-wave` 注解）解决资源顺序：CRD/Namespace 在前、依赖它们的 CR 在后——等价于 Helm hook 的声明式重写。

## 四、Tekton 与动态 agent：把 CI 算力搬进集群

**Tekton**：纯 CRD + 控制器架构（K8s 原生度最高的 CI）：

- Pipeline/Task 是模板，PipelineRun/TaskRun 是执行实例；控制器把**每个 TaskRun 翻译成一个 Pod**，Task 内的 steps 映射为容器列表
- steps 需要顺序执行，而 Pod 内容器是并发启动的——Tekton 用 entrypoint 包装器让后一个 step 等前一个的完成信号，这是「step 卡住但 Pod Running」的排障关键
- workspace = PVC/emptyDir 在 steps 间共享数据；镜像构建在集群内用 Kaniko/BuildKit（无 Docker daemon）

**Jenkins kubernetes plugin / GitLab Runner kubernetes executor**：同一模式——每个 job 动态创建 agent Pod，完成即删。集成点是 podTemplate（资源、镜像、SA）；生产痛点集中在：agent 镜像拉取慢拖累构建时长、requests 设置不当挤占业务资源（建议独立 node pool + taint）、Pod 创建风暴打 apiserver。

## 五、生产排障速查

| 症状 | 链路定位 | 检查手段 |
|------|---------|---------|
| helm upgrade 超时 | hook Job 或 --wait | `kubectl get job -l "helm.sh/hook"`、目标 Pod 事件 |
| helm 历史丢失/回滚失败 | release Secret 被删 | `kubectl get secret -l owner=helm`、Secret 清理策略 |
| ArgoCD 永久 OutOfSync | 字段属主冲突（CompareAppState:559） | `argocd app diff` 看具体字段、ignoreDifferences 规则 |
| ArgoCD sync 卡住 | operation 执行链（processRequestedAppOperation:1430） | `argocd app get` 看 operation state、hook/wave 阻塞资源 |
| ArgoCD sync 后应用 Degraded | health 评估 | 资源自身事件（sync 成功≠应用健康）、自定义 health lua |
| Tekton step 卡住 Pod Running | entrypoint 等待链 | 上一 step 退出码、`kubectl logs <pod> -c step-<name>` |
| CI agent Pod 起不来 | podTemplate/配额 | ResourceQuota、镜像拉取、ServiceAccount 权限 |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|kubernetes-core 06 - 声明式 API 与 Informer 机制]]（GitOps 的机制原型）
- [[10-平台工程/06-代码分析/kubernetes-core/02-kube-apiserver-deep-dive.md|kubernetes-core 02 - kube-apiserver 源码深度剖析]]（SSA/managedFields）
- [[03-清单模式/05-GitOps模式/index.md|清单模式域：GitOps 模式]]
- [[03-清单模式/03-Helm值模式/index.md|清单模式域：Helm 值模式]]
- [[11-发布变更/README.md|发布变更域]]
- [[02-工作负载/02-Java-on-K8s/05-java-cicd-tekton-argocd.md|工作负载域：Java CI/CD（Tekton+ArgoCD）实践]]
