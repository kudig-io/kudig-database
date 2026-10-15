---
title: KubeVela 与 OAM 应用交付模型
description: KubeVela 与 OAM（Open Application Model）深度解析 — Application/Component/Trait/Workflow 四概念、CUE 模板引擎与 Addon 生态、Workflow 挂起-审批-通知交付流程、多集群分发、与 Helm/Argo CD/Flux 的组合模式
summary: 以 OAM 理念为核心剖析 KubeVela 的应用交付抽象（Component/Trait/Workflow/Policy）、CUE 定义体系、多环境 Workflow 审批流水线与多集群拓扑分发，并给出与 Helm/Kustomize/GitOps 工具链的分层组合方案与适用边界
category: application-patterns
tags:
- kubevela
- oam
- application-delivery
- cue
- platform-engineering
- gitops
- helm
- multi-cluster
tier: supporting
created: '2026-10-14'
last_updated: '2026-10-14'
difficulty: advanced
audience:
- 平台工程师
- SRE
- 架构师
estimated_read_time: 20min
intent_queries:
- KubeVela 是什么，和 OAM 什么关系
- 如何用 KubeVela 实现 Application/Component/Trait/Workflow 应用交付
- KubeVela workflow 如何做挂起审批与多环境 diff
- KubeVela 如何做多集群应用分发
- KubeVela 与 Helm/Argo CD/Flux 如何组合
trigger_keywords:
- KubeVela
- OAM
- Open Application Model
- Application CRD
- Trait
- Workflow
- CUE
- vela CLI
- Addon
- 多集群分发
prerequisites:
- kubectl-basics
- helm-basics
- gitops-concepts
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# KubeVela 与 OAM 应用交付模型

> **适用版本**: KubeVela v1.11.0（截至 2026-10 GitHub 最新稳定版）；OAM 规范 v0.3.x
> **定位**: 应用层编排与平台工程抽象，不替代 Helm/Kustomize/GitOps，而是站在它们之上

---

## 📋 目录

- [一、概述与核心概念](#一概述与核心概念)
- [二、架构设计与组件关系](#二架构设计与组件关系)
- [三、与 Helm / Kustomize / 纯 YAML 的关系](#三与-helm--kustomize--纯-yaml-的关系)
- [四、生产级配置与操作指南](#四生产级配置与操作指南)
- [五、故障排查与诊断流程](#五故障排查与诊断流程)
- [六、最佳实践与反模式](#六最佳实践与反模式)
- [七、性能调优与监控指标](#七性能调优与监控指标)
- [八、版本兼容性与升级注意事项](#八版本兼容性与升级注意事项)
- [九、参考链接与延伸阅读](#九参考链接与延伸阅读)

---

## 一、概述与核心概念

### 1.1 OAM：应用与平台能力解耦

OAM（Open Application Model）由阿里云与微软于 2019 年联合发起，后来捐入 CNCF 沙箱。它要解决的核心矛盾是：**应用开发者关心"部署什么"，平台团队关心"怎么部署"，而原生 Kubernetes 把两者揉在同一份 YAML 里**。

- 开发者写 `Deployment + Service + Ingress + HPA` 时，被迫理解节点亲和、探针、Ingress Class 等平台细节；
- 平台团队想统一收口 ingress、证书、可观测性注入，却没有统一的挂载点，只能靠 admission webhook 打补丁。

OAM 的解法是把"应用"定义为一等公民 CRD，用**角色分层**取代"所有人写全量 YAML"：

| 角色 | 关心什么 | 在 OAM 中的表达 |
|------|----------|----------------|
| 应用开发者 | 镜像、端口、环境变量 | Component（组件）的 properties |
| 平台团队 | 运维能力如何挂载 | Trait（运维特征）的能力目录 |
| 交付负责人 | 发布到哪、按什么顺序 | Policy + Workflow |
| 所有人合起来 | 一个完整交付单元 | Application |

### 1.2 四大核心概念

| 概念 | API 组 | 职责 | 类比 |
|------|--------|------|------|
| **Application** | `core.oam.dev/v1beta1` | 交付单元：一个业务应用包含的全部组件、策略与流程 | Helm Release 的"应用视角" |
| **Component** | Application 内嵌 | 一个工作负载实例（webservice/worker/helm/task...），只声明"是什么" | Deployment+Service 的抽象壳 |
| **Trait** | 挂在 Component 上 | 一个运维能力（ingress/scaler/labels/gateway/annotations...），只声明"要什么能力" | Kustomize patch / 平台注入 |
| **Workflow** | Application.spec.workflow | 交付流程：步骤编排、挂起审批、多环境推进 | CI/CD pipeline 的交付段 |

两个辅助概念同样重要：

- **Policy（策略）**：交付约束，如 `topology`（发到哪些集群/命名空间）、`override`（差异化配置）、`garbage-collect`、`apply-once`；
- **Definition（定义）**：平台侧的"能力出厂设置"——`ComponentDefinition`/`TraitDefinition`/`WorkflowStepDefinition` 用 CUE（或 Helm/Go 参考）描述参数结构与渲染逻辑，是 Trait 目录的源头。

> **一句话模型**：开发者填 Component，平台在 Definition 目录里暴露 Trait，交付团队用 Workflow + Policy 把应用按流程发到多集群——这就是 OAM 的"应用与平台能力解耦"。

### 1.3 OAM 规范现状（必须知道的边界）

OAM 规范仓库停在 v0.3.x 线，上游 `oam-kubernetes-runtime` 已归档；KubeVela 是事实上的 OAM 唯一活跃实现，并已超出规范本身（Policy/Workflow/CUE 引擎均为 KubeVela 扩展）。选型时应把 KubeVela 视为"OAM 思想 + KubeVela 事实 API"，而不是可互换的多厂商标准。

---

## 二、架构设计与组件关系

### 2.1 整体架构

```
                       ┌─────────────────────────────────────────┐
  vela CLI / VelaUX ──▶│            Application (CR)              │
  （开发者/审批人）      │  components[] + policies[] + workflow[]  │
                       └───────────────────┬─────────────────────┘
                                           │ watch/reconcile
                       ┌───────────────────▼─────────────────────┐
                       │   KubeVela Controller（vela-core）        │
                       │  ┌────────────┐  ┌─────────────────────┐ │
                       │  │ CUE 引擎    │  │ Workflow 引擎        │ │
                       │  │ 渲染参数→K8s│  │ 步骤驱动/挂起/重试    │ │
                       │  └────────────┘  └─────────────────────┘ │
                       │  ┌─────────────────────────────────────┐ │
                       │  │ Definition 运行时                     │ │
                       │  │ ComponentDef / TraitDef / StepDef   │ │
                       │  └─────────────────────────────────────┘ │
                       └──────┬──────────────────┬───────────────┘
                              │ 渲染产物下发       │ cluster-gateway
                    ┌─────────▼────────┐  ┌──────▼──────────────────┐
                    │ 控制集群（local） │  │ 子集群 cluster-1..N      │
                    │ Deployment/Svc/  │  │ 同样的 K8s 原生资源       │
                    │ Ingress/HPA...   │  └─────────────────────────┘
                    └──────────────────┘
```

关键组件：

| 组件 | 说明 |
|------|------|
| **vela-core** | 核心控制器，Deployment 形态运行在 `vela-system`，负责 Application reconcile、CUE 渲染、资源下发与状态回写 |
| **CUE 模板引擎** | Definition 以 CUE 语言编写：`parameter` 暴露给用户、`output/outputs` 渲染成 K8s 资源；支持校验、默认值、条件渲染 |
| **Workflow 引擎** | 内置步骤（deploy/suspend/notification/...）+ 自定义 StepDefinition；支持挂起、终止、重试、超时 |
| **cluster-gateway** | 多集群网关（addon），控制集群经它向子集群写资源，是 `topology` policy 的执行通道 |
| **Addon 生态** | `vela addon` 仓库提供 100+ 能力包（fluxcd、velaux、istio、prometheus-server...），一个 addon 可同时装 CRD + 控制器 + Definition |

### 2.2 一次交付的数据流

1. 用户提交 `Application`（GitOps 场景下由 Argo CD/Flux 同步）；
2. controller 取出每个 Component，按其 `type` 找到 ComponentDefinition，用 CUE 把 `properties` 渲染成工作负载；
3. 逐个渲染 Trait（TraitDefinition），patch/附加到工作负载上（如 ingress → Ingress/Gateway 资源，scaler → HPA）；
4. Workflow 步骤执行：`deploy` 步骤读取 topology policy 决定目标集群集合，把渲染产物经 cluster-gateway 应用到对应集群；
5. 状态聚合回写 `Application.status`，`vela status` / VelaUX 展示每个组件、每个集群的健康度。

### 2.3 多集群分发模型

- **拓扑策略（topology）**：`clusters: ["local","c1"]` 或 `clusterLabelSelector` 圈定目标；
- **差异化策略（override）**：对指定集群覆盖组件参数（如 prod 副本数不同）；
- **健康与垃圾回收**：应用删除时按 `garbage-collect` policy 决定子集群资源是否级联清理。

参考 [[10-平台工程/02-运维/10-multi-cluster-management.md|多集群管理]] 了解集群生命周期管理本身的更多方案。

---

## 三、与 Helm / Kustomize / 纯 YAML 的关系

### 3.1 分层定位

| 工具 | 层次 | 强项 | 弱项 |
|------|------|------|------|
| 纯 YAML | 原始层 | 零依赖、完全透明 | 无参数化、无能力抽象、多环境靠复制粘贴 |
| Kustomize | 渲染层 | 无模板语言的 overlay 差异化 | 只做静态渲染，不懂"应用"、没有流程 |
| Helm | 包管理层 | 生态最大、版本化 chart、仓库分发 | 单 chart 内聚但跨应用流程弱；值文件易膨胀 |
| **KubeVela** | **应用编排层** | 应用模型 + Trait 目录 + Workflow + 多集群 | 学习成本（CUE）；生态远小于 Helm |

KubeVela 不与它们竞争，而是**组合**：

- Component `type: helm` 直接部署 chart（KubeVela 充当上层编排，底层仍用 Helm 渲染）；
- 通过 **fluxcd addon**，`helm` 组件实际由 Helm Controller 落地，获得 drift 修复与 release 管理；
- Kustomize 产物可以封装进自定义 ComponentDefinition（`type: kustomize` 属于 fluxcd addon 提供的组件类型之一）；
- 原生 `webservice`/`worker` 组件渲染出的就是标准 Deployment/Service，Helm/Kustomize 也能管理同一批资源——但应避免双写（见反模式）。

### 3.2 与 Crossplane 的边界

Crossplane 把"云资源"抽象为 Compose 平台 API（见 [[03-清单模式/09-平台模式/01-crossplane-compositions-patterns.md|Crossplane Composition 模式]]）；KubeVela 把"应用交付"抽象为 OAM 模型。两者互补：RDS/OSS 用 Crossplane，应用用 KubeVela，KubeVela Component 甚至可以引用 Crossplane 资源作为依赖。

---

## 四、生产级配置与操作指南

### 4.1 安装与初始化

```bash
# 🟢 添加 Helm 仓库（只读操作）
helm repo add kubevela https://charts.kubevela.net/core
helm repo update

# 🟡 安装 vela-core 到控制集群（创建 vela-system 命名空间与 CRD）
helm install kubevela kubevela/vela-core \
  --version 1.11.0 \
  --namespace vela-system --create-namespace \
  --wait

# 🟡 安装 vela CLI（macOS 示例，其他平台见官方文档）
brew install kubevela/vela/vela

# 🟡 启用 VelaUX 控制台与 fluxcd（Helm 组件能力）addon
vela addon enable velaux
vela addon enable fluxcd
```

```bash
# 🟢 查看平台内置能力目录
vela component list      # webservice/worker/task/helm/...
vela trait list          # ingress/scaler/labels/gateway/...
vela show webservice     # 查看 webservice 参数结构（CUE 生成）
```

### 4.2 完整 Application 示例（2 组件 + Trait + Workflow）

场景：电商平台，`web` 前端（webservice + ingress + scaler）、`order` 后端（helm chart 部署）；流程：先发测试环境 → 通知 → 人工审批 → 发生产双集群。

```yaml
apiVersion: core.oam.dev/v1beta1
kind: Application
metadata:
  name: ecommerce-app
  namespace: ecommerce
spec:
  components:
    # ── 组件 1：前端 Web 服务（KubeVela 原生 webservice 渲染）──
    - name: web
      type: webservice
      properties:
        image: registry.example.com/ecom/web:v1.4.2
        port: 8080
        env:
          - name: ORDER_API
            value: http://order-order-chart:8081
        resources:
          requests:
            cpu: 250m
            memory: 512Mi
        health:
          livenessProbe:
            http:
              path: /healthz
              port: 8080
      traits:
        - type: ingress          # Trait 1：入口与路由
          properties:
            domain: shop.example.com
            http:
              "/api": 8080
        - type: scaler           # Trait 2：弹性副本
          properties:
            min: 3
            max: 10
    # ── 组件 2：订单服务（底层是 Helm chart，上层被 OAM 编排）──
    - name: order
      type: helm
      properties:
        repoType: helm
        url: https://charts.example.com/ecom
        chart: order-chart
        version: "2.3.1"
        values:
          replicaCount: 2
          metrics:
            enabled: true

  policies:
    - name: staging-only
      type: topology
      properties:
        clusters: ["local"]        # 测试环境：仅控制集群
        namespace: ecommerce-staging
    - name: prod-multi-cluster
      type: topology
      properties:
        clusters: ["cn-east", "cn-north"]   # 生产：两个子集群
        namespace: ecommerce-prod
    - name: prod-replica-boost
      type: override
      properties:
        components:
          - name: order
            properties:
              values:
                replicaCount: 4   # 生产环境覆盖 helm values

  workflow:
    steps:
      - name: deploy-staging
        type: deploy
        properties:
          policies: ["staging-only"]
      - name: notify-reviewers
        type: notification
        properties:
          webhook:
            url: https://hooks.example.com/kubevela/approval
            message:
              name: 电商应用等待审批
              header: "staging 已部署，diff 见 VelaUX，请审批后推进 prod"
      - name: manual-approval      # 挂起：vela workflow resume 继续
        type: suspend
        properties:
          duration: 24h            # 超时自动失败，防止永久挂起
      - name: deploy-prod
        type: deploy
        properties:
          policies: ["prod-multi-cluster", "prod-replica-boost"]
```

要点解读：

- **组件解耦**：`web` 用原生 CUE 组件，`order` 用 Helm 组件，Application 层完全一致——开发者不需要知道底层是 chart 还是 CUE；
- **Trait 即目录**：ingress/scaler 由平台在 Definition 里定义，业务侧只填参数；平台可整体替换 ingress → Gateway API 而业务无感；
- **Workflow 编排**：两个 `deploy` 步骤引用不同 topology policy 实现环境推进，`suspend` + `notification` 构成"挂起-审批-通知"闭环。

### 4.3 日常操作（vela CLI）

```bash
# 🟢 渲染预检：不落库，输出将生成的真实资源
vela dry-run -f app.yaml

# 🟢 多环境/线上 diff：对比 Application 与集群实际状态
vela live diff -f app.yaml

# 🟡 提交应用
kubectl apply -f app.yaml -n ecommerce

# 🟢 查看应用状态（含每个集群、每个组件健康度）
vela status ecommerce-app -n ecommerce

# 🟢 查看 workflow 执行记录
vela workflow logs ecommerce-app -n ecommerce

# 🟡 审批通过后恢复挂起的 workflow（推进生产发布）
vela workflow resume ecommerce-app -n ecommerce

# 🔴 终止执行中的 workflow（可能造成跨集群状态不一致，需人工评估）
vela workflow terminate ecommerce-app -n ecommerce

# 🟡 多集群纳管：注册子集群（在子集群 kubeconfig 上下文中执行）
vela cluster add cn-east
vela cluster list   # 🟢
```

### 4.4 与 Argo CD / Flux 的 GitOps 组合

两种成熟组合模式：

1. **GitOps 管同步、KubeVela 管渲染与流程（推荐）**
   - Git 仓库只存 `Application` CR；Argo CD/Flux 把它同步进控制集群；
   - KubeVela controller 负责渲染 + Workflow + 多集群分发；
   - 审批环节交给 `suspend`，GitOps 的"回滚=git revert"与 Workflow 状态需在 runbook 中对齐。
   - 参考 [[11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops.md|Argo CD 企业级 GitOps]] 与 [[11-发布变更/01-GitOps/07-flux-gitops-continuous-delivery.md|Flux GitOps 持续交付]]。

2. **纯 KubeVela 模式**
   - CI 推镜像 → `vela live diff` 卡点 → `vela` CLI/VelaUX 触发；
   - 适合无 GitOps 基础设施、但需要应用抽象与多集群的团队。

注意：不要让 Argo CD 直接管理 KubeVela 渲染出来的子资源（Deployment/Ingress），只管 Application 本身，否则会出现双控制器争抢。渐进发布若需金丝雀，可叠加 [[11-发布变更/01-GitOps/11-argo-rollouts-progressive-delivery.md|Argo Rollouts 渐进式交付]] 或 KubeVela 的 gateway/canary 相关 addon。

---

## 五、故障排查与诊断流程

```bash
# 🟢 第一步：应用整体状态与条件
vela status ecommerce-app -n ecommerce --detail
kubectl get application ecommerce-app -n ecommerce -o yaml | sed -n '/status/,$p'

# 🟢 第二步：定位到具体组件/步骤的错误信息
kubectl get application ecommerce-app -n ecommerce \
  -o jsonpath='{.status.services}' | jq
vela workflow logs ecommerce-app -n ecommerce

# 🟢 第三步：检查渲染产物（控制集群或子集群）
vela status ecommerce-app -n ecommerce --tree   # 资源树
kubectl get deployment,svc,ingress,hpa -n ecommerce-staging

# 🟢 第四步：controller 自身日志
kubectl logs -n vela-system -l app.kubernetes.io/name=kubevela --tail=200
```

| 现象 | 常见根因 | 处理 |
|------|----------|------|
| `CreateComponentDefinition` / 渲染失败 | properties 与 Definition 的 CUE schema 不匹配 | `vela dry-run` 本地复现；对照 `vela show <type>` 修正参数 |
| Trait 不生效 | addon 未启用（如 ingress 依赖 FluxCD/网关 addon） | `vela addon list` 检查并启用 |
| Workflow 停在 suspend | 人工审批未推进 | `vela workflow resume`；超时由 `duration` 控制 |
| 子集群资源未创建 | cluster-gateway 未装/子集群凭证失效 | `vela cluster list`、检查 `cluster-gateway` pod 与 addon |
| 状态 `unhealthy` 但资源存在 | 健康探针/就绪判定未过 | 看 `status.services[].message` 与目标集群 events |
| Application 删除 hang | 子集群不可达，垃圾回收阻塞 | 处理集群连通性；或按 `garbage-collect` policy 调整策略后重试 |

---

## 六、最佳实践与反模式

### 6.1 适用场景

- **平台团队需要"能力目录"**：内部 PaaS、多团队共享集群、统一 ingress/可观测性/安全基线——KubeVela 的 Definition + Addon 体系收益最大；
- **多集群应用分发 + 环境推进流程**：topology + override + workflow 是目前 K8s 生态中少有的"应用级"多集群原语；
- **抽象封装需求强**：把 Helm/Kustomize/云资源封装成少数几个稳定组件类型给业务用。

### 6.2 局限与不适合的场景

- **小团队（3~5 人）无平台化诉求**：引入 CUE + Definition + Addon 是纯负担，Helm + Kustomize + Argo CD 足够；
- **强 GitOps 纯度团队**：Workflow 的运行时状态与 GitOps 声明式哲学存在张力（审批后集群状态与 git 历史不同步）；
- **深度定制渲染**：复杂 CUE 模板调试体验一般，团队需要真正掌握 CUE 语言；
- **OAM 多厂商可移植性**：如前所述，规范停滞，"换一家 OAM 实现"并不现实。

### 6.3 最佳实践

1. **Definition 即 API 契约**：业务可用的参数精简到 10 个以内，其余在 CUE 里给默认值；Definition 变更走评审 + `vela dry-run` 回归；
2. **Addon 管理平台能力，不用裸 CRD**：平台能力统一以 addon 分发（CRD+控制器+Definition 一起装/卸），避免手工碎片化；
3. **Workflow 必设 suspend 超时**（`duration`），防止审批遗漏导致流水线永久挂起；
4. **多集群用 label 圈选而非硬编码集群名**，扩容新集群时 topology policy 不用改；
5. **单一写入源**：要么 KubeVela 要么 Argo CD 管某资源，绝不双写；
6. **override 最小化**：环境差异集中在 values/副本数等少数参数，大量 override 说明组件设计有问题。

### 6.4 反模式

- ❌ 把 Application 当作"更大的 Deployment"，直接在 properties 里透传全部 K8s 字段——抽象名存实亡；
- ❌ 用 CUE 里拼接字符串生成 YAML 片段（`json.Marshal` 黑魔法），应使用结构化 output；
- ❌ 绕过 Trait 手工 patch 子集群资源——下次 reconcile 即被覆盖，且状态混乱；
- ❌ Workflow 步骤里做长计算/数据搬运——Workflow 是交付编排不是数据管道；
- ❌ 未启用 `garbage-collect` 策略就大规模删除 Application——子集群可能残留孤儿资源。

---

## 七、性能调优与监控指标

### 7.1 Controller 调优

| 参数/手段 | 建议 |
|-----------|------|
| `--application-re-sync-period`（requeue 周期） | 大规模应用时可适当调大，减少无效 reconcile |
| vela-core 副本数 | 保持 1（leader election）；扩容 pod 数不提升渲染吞吐 |
| 渲染缓存 | 升级到新版 CUE 引擎收益明显；避免 Definition 里引用大 context 对象 |
| 每集群 Application 数 | 经验值：单 vela-core 舒适区为数百个 Application；超千需评估 controller 负载与 etcd 压力 |

### 7.2 监控指标（vela-core 内置 Prometheus 指标）

| 指标 | 含义 | 告警建议 |
|------|------|----------|
| `vela_application_phase{phase="..."}` | 应用整体阶段（running unhealthy 等） | `workflowSuspending` 挂起超阈值告警 |
| `vela_controller_reconcile_error_count` | reconcile 错误计数 | 持续增长即渲染/下发异常 |
| `vela_workflow_*`（步骤耗时/状态） | workflow 步骤执行情况 | 单步骤失败告警 |
| `vela_resource_apply_duration_seconds` | 资源下发时延 | 多集群下发慢时定位目标集群 |

```bash
# 🟢 快速查看 vela-core 暴露的指标
kubectl -n vela-system exec deploy/kubevela-vela-core \
  -- curl -s localhost:9443/metrics | grep -E '^vela_application_phase' | head
```

### 7.3 VelaUX 与可观测性

启用 `velaux` addon 后可获得应用拓扑、workflow 可视化、多集群状态聚合；再配合 `prometheus-server` addon 或既有监控栈采集上表指标，即构成平台级观测面。

---

## 八、版本兼容性与升级注意事项

> 版本信息核对时间：2026-10-14，`gh api repos/kubevela/kubevela/releases/latest` → `v1.11.0`

- **K8s 版本要求**：KubeVela 1.x 支持主流在维护的 Kubernetes 版本（建议 ≥1.22）；老集群（≤1.20）需停留在旧版 KubeVela，不在本文讨论范围；
- **CRD 升级**：`helm upgrade` 不会自动更新 CRD，升级前必须按官方 release note 手动 `kubectl apply`/`kubectl replace` 新版 CRD（Application/Definition 等 `core.oam.dev`、`standard.oam.dev` 组）；
- **v1.7 之后（含 v1.11）**：Workflow 引擎已稳定为 v1beta1 语义，旧式 `spec.workflow` 内嵌 `suspend` 用法与本文一致；升级前用 `vela dry-run` 全量回归存量 Application；
- **Addon 版本耦合**：fluxcd / velaux 等 addon 与 vela-core 版本存在兼容矩阵，升级顺序：先升 vela-core → 再逐个升级 addon → `vela addon list` 确认无 deprecated；
- **多集群升级**：先升级控制集群 vela-core 与 cluster-gateway，子集群无需安装 vela-core（只需要被纳管的凭证与 RBAC）；
- **回滚预案**：Helm 保留 history，`helm rollback kubevela -n vela-system` 可回滚控制器；已由新版渲染过的资源若 schema 变化，回滚后需 `vela dry-run` 检查兼容性。

---

## 九、参考链接与延伸阅读

- KubeVela 官方文档：https://kubevela.io/zh/docs/（中文站）
- KubeVela GitHub Releases：https://github.com/kubevela/kubevela/releases（v1.11.0）
- OAM 规范仓库：https://github.com/oam-dev/spec（v0.3.x，演进已停滞）
- CUE 语言：https://cuelang.org/
- VelaUX / Addon 仓库：https://github.com/kubevela/catalog
- 本库相关：[[11-发布变更/01-GitOps/19-helm-production-guide.md|Helm 生产实践]]、[[11-发布变更/01-GitOps/16-gitops-2025-argocd-flux-best-practices.md|Argo CD/Flux 最佳实践]]、[[10-平台工程/02-运维/10-multi-cluster-management.md|多集群管理]]、[[22-概念/09-平台与发布/01-cluster-api-deep-dive.md|Cluster API 深度解析]]
