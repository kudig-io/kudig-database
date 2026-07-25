---
title: Kubernetes 多集群 Fleet GitOps 运维指南
description: 面向 Kubernetes 生产环境的多集群 Fleet GitOps 运维操作手册，覆盖 ApplicationSet、Karmada/OCM/Cluster API、跨集群 Secret 同步、晋升门控、漂移检测与可观测性。
summary: 面向 Kubernetes 生产环境的多集群 Fleet GitOps 运维操作手册，覆盖 ApplicationSet、Karmada/OCM/Cluster API、跨集群 Secret 同步、晋升门控、漂移检测与可观测性。
category: gitops
tags:
- production
- best-practices
- playbook
- gitops
- fleet
- multi-cluster
- argo-cd
- applicationset
- karmada
- ocm
- cluster-api
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes Fleet GitOps 运维指南是什么
- 如何运维多集群 GitOps
- ApplicationSet 多集群部署最佳实践
- 跨集群 Secret 同步怎么做
- Fleet 漂移检测与回滚
- Karmada OCM Cluster API 选型
trigger_keywords:
- fleet
- gitops
- multi-cluster
- ApplicationSet
- Karmada
- OCM
- Cluster API
- 跨集群
- 多集群发布
- 集群舰队
- 漂移检测
- 晋升门控
prerequisites:
- gitops-basics
- argo-cd-basics
- kubectl-basics
- helm-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 多集群 Fleet GitOps 运维指南

> **适用版本**: Kubernetes v1.28 - v1.33 | **最后更新**: 2026-07
> **文档定位**: 面向生产环境的多集群 Fleet GitOps 运维入口，提供从集群注册、应用分发、Secret 同步、晋升门控到漂移检测的完整操作路径。

本指南聚焦 [[11-发布变更/README.md|发布与变更管理]] 域中的多集群/舰队场景，目标是在数十到数百个 Kubernetes 集群的规模下，保持 Git 作为唯一事实来源、实现安全可控的渐进式发布、可审计的跨集群配置同步，以及可观测的 Fleet 健康状态。内容面向已掌握 Argo CD、Helm/Kustomize 与 kubectl 的 SRE、运维工程师和平台工程师。

---

## 1. 适用场景与范围

### 1.1 适用场景

- **同应用多集群分发**: 同一套平台组件（Ingress Controller、监控 Agent、安全 DaemonSet）需要分发到几十至上百个集群。
- **多环境晋升链路**: 配置从 `dev` → `staging` → `production` 按集群批次晋升，需支持暂停、审批与自动回滚。
- **地域/可用区灾备**: 同一业务在多个地域或云厂商集群运行，要求版本一致、配置一致、Secret 一致。
- **边缘/租户集群管理**: 大量轻量边缘集群或租户专属集群需要统一基线、差量覆盖与生命周期管理。
- **混合云/多云基线治理**: 在自建 IDC、EKS、ACK、GKE、AKS 等多种集群上保持平台组件与策略一致性。

### 1.2 适用范围

本指南覆盖以下技术栈与流程：

- Argo CD ApplicationSet 的 `clusters`、`git`、`list`、`matrix` 生成器模式；
- 集群注册标准化、标签/注解元数据、AppProject 边界隔离；
- 跨集群 Secret 同步（External Secrets Operator / Vault Agent / SOPS + KSOPs）；
- Karmada、Open Cluster Management (OCM)、Cluster API 的核心能力与选型边界；
- 晋升门控（promotion gates）、漂移检测、Fleet 可观测性与应急回滚。

### 1.3 不适用场景

- 单集群 Argo CD 基础安装与配置，请参考 [[11-发布变更/01-GitOps/99-argo-cd-gitops-guide.md|Argo CD 企业级 GitOps 实践指南]]；
- 单个应用的 Helm/Kustomize 模板编写，请参考 [[11-发布变更/01-GitOps/99-helm-production-guide.md|Helm 生产指南]]；
- 集群升级与节点变更，请参考 [[01-集群基础/00-总览/99-production-readiness-operations-guide.md|集群基础域生产就绪运维指南]]；
- 跨集群网络与 Service Mesh，请参考 [[05-网络/00-总览/99-production-readiness-operations-guide.md|网络流量域生产就绪运维指南]]。

---

## 2. 前置条件与工具

### 2.1 必备组件

| 组件 | 用途 | 推荐版本/说明 |
|:---|:---|:---|
| Argo CD | GitOps 控制面，管理 Application 与 ApplicationSet | v2.11+，启用 ApplicationSet 控制器 |
| ApplicationSet Controller | 多集群/多环境 Application 生成 | 随 Argo CD 部署，确认 `applicationset-controller` Pod 运行 |
| External Secrets Operator (ESO) | 跨集群 Secret 同步 | v0.9+，配合 Vault/AWS SM/GCP SM 使用 |
| cert-manager | 集群入口/控制面证书自动化 | v1.14+ |
| Prometheus/Grafana | Fleet 指标聚合与告警 | 建议使用 Thanos/Mimir 实现跨集群长期存储 |
| OpenTelemetry / Fluent Bit | 跨集群日志与追踪汇聚 | 按地域或业务域设置 Collector 层级 |

### 2.2 可选的集群联邦/编排层

| 工具 | 定位 | 典型场景 |
|:---|:---|:---|
| **Karmada** | Kubernetes 原生多集群应用编排 | 需要在多个集群间做跨集群调度、故障迁移、多集群 Service 发现 |
| **Open Cluster Management (OCM)** | 以 Hub-Spoke 为中心的集群生命周期与治理 | 大规模集群注册、策略下发、工作负载分发、可观测性汇聚 |
| **Cluster API** | 声明式集群创建与生命周期 | 需要按需创建/销毁/升级 Kubernetes 集群（常与 Argo CD + OCM 组合） |

> **选型建议**: 若核心诉求是"用 GitOps 把同一套应用分发到多个集群"，优先使用 Argo CD ApplicationSet；若需要跨集群调度、故障转移或多集群 Service，则引入 Karmada 或 OCM；若需要自动化集群创建/扩缩容，则引入 Cluster API。三者并非互斥，常见组合为：Cluster API 管理集群生命周期 → OCM 注册与治理 → Argo CD ApplicationSet 分发工作负载。

### 2.3 CLI 与权限

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 必备 CLI
argocd version          # Argo CD CLI v2.11+
kubectl version         # v1.28+
helm version            # v3.13+
clusteradm version      # OCM 管理 CLI（如使用 OCM）
karmadactl version      # Karmada 管理 CLI（如使用 Karmada）

# 权限要求
# - Argo CD 管理员权限：管理 AppProject、ApplicationSet、集群 Secret
# - 各目标集群 cluster-admin 或具备创建/更新目标 Namespace 资源的 RBAC
# - Secret 管理后端（Vault/AWS SM 等）读取权限
```
---

## 3. 标准操作流程

### 3.1 Step 1 — 集群注册与元数据标准化

所有目标集群必须在 Argo CD 中以统一方式注册，并携带标准化标签与注解，供 ApplicationSet 选择。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1.1 添加集群并打标签（推荐通过 CLI 一次性完成）
argocd cluster add production-beijing-01 \
  --name prod-bj-01 \
  --label environment=production \
  --label region=cn-beijing \
  --label cloud=alicloud \
  --label tier=platform \
  --label criticality=p1

# 1.2 验证集群注册与标签
argocd cluster list -o wide
kubectl get secret -n argocd -l argocd.argoproj.io/secret-type=cluster \
  -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels.environment}{"/"}{.metadata.labels.region}{"\n"}{end}'

# 1.3 在 Git 中管理集群 Secret（可选，适合 Cluster API 或 OCM 自动生成场景）
# 将 argocd cluster add 生成的 Secret 导出后存入 Git（注意剔除 token 等敏感字段，
# 或配合 External Secrets Operator 从 Vault 动态注入 bearerToken）。
```
**元数据规范示例**:

| 标签 | 用途 | 示例 |
|:---|:---|:---|
| `environment` | 环境隔离 | `dev`, `staging`, `production` |
| `region` | 地域/可用区路由 | `cn-beijing`, `us-east-1` |
| `cloud` | 云厂商标识 | `alicloud`, `aws`, `gcp`, `azure`, `idc` |
| `tier` | 集群用途 | `platform`, `business`, `edge`, `ai-ml` |
| `criticality` | 变更窗口与审批级别 | `p0`, `p1`, `p2` |
| `fleet-group` | 分批发布组 | `wave-1`, `wave-2`, `wave-3` |

### 3.2 Step 2 — AppProject 与 RBAC 边界

在多集群 Fleet 中，必须防止应用被部署到错误环境。通过 AppProject 限定 `sourceRepos`、`destinations`、`clusterResourceWhitelist` 与 `namespaceResourceBlacklist`。

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AppProject
metadata:
  name: production-platform
  namespace: argocd
spec:
  description: 生产平台组件分发项目
  sourceRepos:
    - "https://github.com/org/gitops-fleet.git"
  destinations:
    - server: https://kubernetes.default.svc
      namespace: kube-system
    - server: https://prod-bj-01.example.com
      namespace: kube-system
    - server: https://prod-sh-01.example.com
      namespace: kube-system
  clusterResourceWhitelist:
    - group: ''
      kind: Namespace
    - group: rbac.authorization.k8s.io
      kind: ClusterRole
    - group: rbac.authorization.k8s.io
      kind: ClusterRoleBinding
  namespaceResourceBlacklist:
    - group: ''
      kind: ResourceQuota
  roles:
    - name: platform-sre
      description: 平台 SRE 可同步与回滚
      policies:
        - p, proj:production-platform:platform-sre, applications, sync, production-platform/*, allow
        - p, proj:production-platform:platform-sre, applications, rollback, production-platform/*, allow
      groups:
        - platform-sre@example.com
```

> **关键原则**: 一个 AppProject 不要同时包含 `production` 与 `non-production` 目标；生产环境的目标集群应通过白名单精确列出，避免使用通配符 `*`。

### 3.3 Step 3 — ApplicationSet 多集群应用生成

#### 3.3.1 Clusters 生成器（按标签选择集群）

适用于"同一套基线组件分发到所有匹配标签的集群"。

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: platform-addons
  namespace: argocd
spec:
  generators:
    - clusters:
        selector:
          matchLabels:
            environment: production
            tier: platform
        values:
          chartVersion: "1.4.2"
          replicas: "3"
  template:
    metadata:
      name: '{{nameNormalized}}-ingress-nginx'
      labels:
        fleet-app: ingress-nginx
        environment: '{{metadata.labels.environment}}'
    spec:
      project: production-platform
      source:
        repoURL: https://github.com/org/gitops-fleet.git
        targetRevision: main
        path: addons/ingress-nginx/overlays/{{metadata.labels.region}}
        helm:
          valueFiles:
            - values-{{metadata.labels.environment}}.yaml
          parameters:
            - name: controller.replicaCount
              value: '{{values.replicas}}'
      destination:
        server: '{{server}}'
        namespace: ingress-nginx
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
          - PrunePropagationPolicy=foreground
          - PruneLast=true
```

#### 3.3.2 Git 生成器（按目录自动发现应用）

适用于"每个应用在 Git 中有独立目录，自动发现后分发到指定环境"。

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: business-apps
  namespace: argocd
spec:
  generators:
    - matrix:
        generators:
          - git:
              repoURL: https://github.com/org/business-apps.git
              revision: main
              directories:
                - path: apps/*
          - clusters:
              selector:
                matchLabels:
                  environment: production
                  tier: business
  template:
    metadata:
      name: '{{path.basename}}-{{nameNormalized}}'
    spec:
      project: production-business
      source:
        repoURL: https://github.com/org/business-apps.git
        targetRevision: main
        path: '{{path}}'
      destination:
        server: '{{server}}'
        namespace: '{{path.basename}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

#### 3.3.3 List 生成器（精确控制分批）

适用于"需要显式定义每批集群与版本"的金丝雀发布。

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: payment-service-canary
  namespace: argocd
spec:
  generators:
    - list:
        elements:
          - cluster: prod-bj-01
            wave: wave-1
            version: v2.3.1-rc1
          - cluster: prod-sh-01
            wave: wave-2
            version: v2.3.1-rc1
          - cluster: prod-sz-01
            wave: wave-3
            version: v2.3.1
  template:
    metadata:
      name: 'payment-{{cluster}}'
      annotations:
        wave: '{{wave}}'
    spec:
      project: production-business
      source:
        repoURL: https://github.com/org/business-apps.git
        targetRevision: main
        path: apps/payment-service
        helm:
          parameters:
            - name: image.tag
              value: '{{version}}'
      destination:
        name: '{{cluster}}'
        namespace: payment
      syncPolicy:
        syncOptions:
          - CreateNamespace=true
```

#### 3.3.4 Matrix 生成器组合模式（目录 × 集群 × 版本）

当应用目录、目标集群与版本矩阵都需要组合时，使用 `matrix` 生成器将多个生成器嵌套，避免为每个组合维护独立 Application。

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: saas-tenants
  namespace: argocd
spec:
  generators:
    - matrix:
        generators:
          - git:
              repoURL: https://github.com/org/tenant-configs.git
              revision: main
              directories:
                - path: tenants/*
          - list:
              elements:
                - region: cn-beijing
                  cluster: prod-bj-01
                  replicas: "6"
                - region: cn-shanghai
                  cluster: prod-sh-01
                  replicas: "4"
  template:
    metadata:
      name: '{{path.basename}}-{{cluster}}'
      labels:
        tenant: '{{path.basename}}'
        region: '{{region}}'
    spec:
      project: saas-tenants
      source:
        repoURL: https://github.com/org/tenant-configs.git
        targetRevision: main
        path: '{{path}}/overlays/{{region}}'
        helm:
          parameters:
            - name: replicaCount
              value: '{{replicas}}'
      destination:
        name: '{{cluster}}'
        namespace: '{{path.basename}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
```

**使用建议**: 当组合数量超过 200 个时，建议拆分为多个 ApplicationSet，并为每个 ApplicationSet 设置独立的 `requeueAfterSeconds`，避免控制器在同一时间触发大量同步。

#### 3.3.5 生成器选择速查

| 生成器 | 数据输入 | 最佳场景 | 注意事项 |
|:---|:---|:---|:---|
| `clusters` | Argo CD 中注册的集群 Secret | 按环境/地域批量分发平台组件 | 依赖集群标签规范，标签错误会导致误部署 |
| `git` (directories) | Git 仓库目录结构 | 应用仓库自动发现，每个目录对应一个应用 | 目录命名需稳定，删除目录会触发资源裁剪 |
| `git` (files) | Git 仓库 JSON/YAML 文件 | 需要显式定义参数矩阵，如版本、副本数 | 文件内容变更会重新生成 Application |
| `list` | 内联元素列表 | 精确控制小批量集群与版本，如金丝雀 | 不适合动态扩容场景，需手动维护列表 |
| `matrix` | 两个生成器的笛卡尔积 | 目录 × 集群 × 版本等多维组合 | 组合爆炸时需拆分 ApplicationSet |
| `scmProvider` | GitHub/GitLab 组织/仓库 | 多仓库多租户场景 | 需要 SCM token 与 API 限流控制 |

### 3.4 Step 4 — 跨集群 Secret 同步

Secret 不应直接存入 Git。推荐通过 External Secrets Operator 从集中式 Secret 后端同步到各集群。

#### 3.4.1 集中式 ClusterSecretStore

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: vault-fleet-backend
spec:
  provider:
    vault:
      server: https://vault.internal.example.com
      path: secret
      version: v2
      auth:
        kubernetes:
          mountPath: kubernetes
          role: fleet-secret-reader
          serviceAccountRef:
            name: external-secrets
            namespace: external-secrets
```

#### 3.4.2 跨集群 ExternalSecret

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: payment-db-credentials
  namespace: payment
spec:
  refreshInterval: 1h
  secretStoreRef:
    kind: ClusterSecretStore
    name: vault-fleet-backend
  target:
    name: payment-db-credentials
    creationPolicy: Owner
    deletionPolicy: Retain
    template:
      type: Opaque
      data:
        DATABASE_URL: "postgresql://{{ .username }}:{{ .password }}@{{ .host }}/payment"
  dataFrom:
    - extract:
        key: secret/data/production/payment/db
```

#### 3.4.3 验证 Secret 同步状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 ExternalSecret 状态
kubectl get externalsecrets -A

# 检查目标 Secret 是否生成并一致
for cluster in prod-bj-01 prod-sh-01 prod-sz-01; do
  echo "=== $cluster ==="
  kubectl --context=$cluster get secret payment-db-credentials -n payment \
    -o jsonpath='{.data.DATABASE_URL}' | base64 -d | sed 's/:[^:@]*@/:****@/'
  echo
done

# 强制刷新（轮换后）
kubectl annotate externalsecret payment-db-credentials -n payment force-sync="$(date +%s)" --overwrite
```
### 3.5 Step 5 — 晋升门控（Promotion Gates）

在多集群 Fleet 中，晋升门控用于控制版本从一批集群滚动到下一批集群，避免一次性全量升级。

#### 3.5.1 Git 分支/Tag 门控

```yaml
# ApplicationSet 中使用 targetRevision 指向环境分支
targetRevision: '{{metadata.labels.environment}}'   # production 分支仅包含已验证版本
```

通过 CI/CD 流水线将验证通过的版本合并到 `production` 分支，ApplicationSet 自动触发同步。

#### 3.5.2 Argo Rollouts + Analysis 门控（业务应用）

对于业务应用，可在每个集群内使用 Argo Rollouts 的金丝雀分析作为批次内门控：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: payment-service
  namespace: payment
spec:
  replicas: 10
  strategy:
    canary:
      steps:
        - setWeight: 10
        - pause: { duration: 10m }
        - analysis:
            templates:
              - templateName: success-rate
            args:
              - name: service-name
                value: payment-service
        - setWeight: 50
        - pause: { duration: 10m }
        - setWeight: 100
```

#### 3.5.3 人工审批门控

对于高风险变更，在 ApplicationSet 生成的 Application 上设置 `syncWindows` 或在 Git 中使用 PR/MR 审批，要求人工批准后才合并到目标分支。

```bash
# 临时暂停某集群自动同步（用于人工观察）
argocd app set payment-prod-bj-01 --sync-policy none

# 确认无误后恢复
argocd app set payment-prod-bj-01 --sync-policy automated --self-heal
```

### 3.6 Step 6 — 漂移检测

漂移是指集群实际状态与 Git 声明状态不一致，可能由人工 `kubectl edit`、控制器侧写或同步失败引起。

#### 3.6.1 启用自动自愈

```yaml
syncPolicy:
  automated:
    selfHeal: true
    prune: true
```

#### 3.6.2 周期性漂移扫描

```bash
# 批量检查所有 Fleet 应用同步状态
argocd app list -l fleet-app=ingress-nginx -o json | \
  jq -r '.[] | select(.sync_status != "Synced") | [.metadata.name, .sync_status, .health_status] | @tsv'

# 查看具体差异
argocd app diff payment-prod-bj-01

# 强制刷新后重新比对
argocd app get payment-prod-bj-01 --refresh --hard
```

#### 3.6.3 Prometheus 漂移告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: argocd-fleet-drift
  namespace: monitoring
spec:
  groups:
    - name: fleet.drift
      rules:
        - alert: ArgoCDApplicationOutOfSync
          expr: argocd_app_info{sync_status="OutOfSync"} == 1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Argo CD 应用存在漂移"
            description: "应用 {{ $labels.name }} 在集群 {{ $labels.dest_server }} 上处于 OutOfSync 状态超过 5 分钟。"
        - alert: ArgoCDApplicationSyncFailed
          expr: increase(argocd_app_sync_total{phase="Error"}[10m]) > 0
          labels:
            severity: critical
          annotations:
            summary: "Argo CD 应用同步失败"
            description: "应用 {{ $labels.name }} 近 10 分钟同步失败次数增加。"
```

### 3.7 Step 7 — Fleet 可观测性

#### 3.7.1 应用级指标

```bash
# 按集群统计应用健康状态
argocd app list -o json | jq -r '
  group_by(.server)[] |
  {server: .[0].server, total: length, healthy: map(select(.health_status=="Healthy")) | length, out_of_sync: map(select(.sync_status=="OutOfSync")) | length}
'
```

#### 3.7.2 集群级指标

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 多集群节点状态汇总（需配置多个 kubeconfig context）
for ctx in prod-bj-01 prod-sh-01 prod-sz-01; do
  echo "=== $ctx ==="
  kubectl --context=$ctx top nodes 2>/dev/null || echo "metrics unavailable"
  kubectl --context=$ctx get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[-1].type}{"="}{.status.conditions[-1].status}{"\n"}{end}'
done
```
#### 3.7.3 集中式 Dashboard 建议

- **Argo CD 大盘**: 应用总数、Sync 状态、Health 状态、同步耗时、错误率；
- **多集群资源大盘**: 按 `cluster`、`region`、`environment` 聚合的 CPU/Memory/磁盘/网络；
- **发布门控大盘**: 各 wave 当前版本、批次进度、金丝雀分析结果；
- **Secret 同步大盘**: ExternalSecret 同步成功率、SecretStore 可用性、刷新延迟。

### 3.8 Step 8 — 大规模 Fleet 性能优化与分片

当 Argo CD 管理的 Application 数量超过 500 或目标集群超过 50 时，默认单实例控制器容易出现同步延迟、内存占用高、repo-server 排队等问题。

#### 3.8.1 控制器参数调优

```yaml
# argocd-cmd-params-cm.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-cmd-params-cm
  namespace: argocd
data:
  controller.status.processors: "50"
  controller.operation.processors: "25"
  controller.repo.server.timeout.seconds: "120"
  controller.app.resync.timeout: "300"
  server.repo.server.timeout.seconds: "120"
```

#### 3.8.2 应用分片（Sharding）

通过为 Application 添加分片标签，让多个 application-controller 副本各自处理不同分片：

```yaml
# ApplicationSet template 中添加分片标签
metadata:
  labels:
    shard: '{{metadata.labels.region}}'
```

```yaml
# argocd-cmd-params-cm 启用分片
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-cmd-params-cm
  namespace: argocd
data:
  controller.sharding.algorithm: "legacy"   # 或 "round-robin"
```

#### 3.8.3 Repo Server 横向扩展

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 增加 repo-server 副本数并启用水平扩展
kubectl scale deployment argocd-repo-server -n argocd --replicas=5

# 启用 repo-server 缓存（默认启用，检查 PVC 状态）
kubectl get pvc -n argocd | grep argocd-repo-server
```
#### 3.8.4 拆分 Argo CD 实例

当 Fleet 规模达到数百集群、数千应用时，建议按业务域或地域拆分独立 Argo CD 实例：

- **平台实例**: 管理 ingress-nginx、cert-manager、monitoring 等平台基线；
- **业务实例**: 管理业务应用，按业务线进一步拆分；
- **边缘实例**: 管理边缘集群，采用 pull-based 或 OCM Agent 模式。

每个实例独立部署、独立升级、独立灾难恢复，降低单点故障影响面。

### 3.9 Step 9 — Karmada / OCM / Cluster API 选型与集成

在纯 ApplicationSet 无法满足的场景下，需要引入集群联邦或声明式集群管理工具。

#### 3.9.1 选型对照

| 场景 | 推荐工具 | 原因 |
|:---|:---|:---|
| 跨集群调度与故障迁移 | Karmada | 原生 Workload/Policy/OverridePolicy 模型，支持 PropagationPolicy 与 Failover |
| 大规模集群注册、策略与治理 | OCM | Hub-Spoke 架构，ManifestWork、Policy、Placement 适合统一基线 |
| 自动化集群创建/扩缩容/升级 | Cluster API | 声明式 Cluster/MachineDeployment，支持多云基础设施 |
| 仅多集群应用分发 | Argo CD ApplicationSet | 最轻量，与 GitOps 流程无缝集成 |

#### 3.9.2 典型组合架构

```
GitOps 仓库
    │
    ▼
Argo CD (Hub 集群)
    │
    ├──► ApplicationSet → 分发平台基线到各成员集群
    │
    ├──► OCM Hub → 注册成员集群、下发 Policy、收集可观测性
    │
    └──► Cluster API → 按需创建/销毁 Kubernetes 集群
```

#### 3.9.3 OCM 快速接入验证

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 在 Hub 集群安装 OCM
clusteradm init --wait

# 获取 join 命令并在成员集群执行
clusteradm get token
clusteradm join --hub-token <token> --hub-apiserver <hub-api-server> --cluster-name prod-bj-01

# 验证集群注册
kubectl get managedclusters

# 通过 ManifestWork 下发基线 Namespace
kubectl apply -f - <<EOF
apiVersion: work.open-cluster-management.io/v1
kind: ManifestWork
metadata:
  name: baseline-namespace
  namespace: prod-bj-01
spec:
  workload:
    manifests:
      - apiVersion: v1
        kind: Namespace
        metadata:
          name: fleet-baseline
EOF
```
#### 3.9.4 Karmada 快速接入验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 Karmada 控制面
karmadactl init --kubeconfig=/root/.kube/config

# 添加成员集群
karmadactl join prod-bj-01 --cluster-kubeconfig=/root/.kube/prod-bj-01.config

# 分发 Deployment
kubectl --context=karmada-apiserver apply -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: sample-app
  namespace: default
spec:
  replicas: 3
  selector:
    matchLabels:
      app: sample-app
  template:
    metadata:
      labels:
        app: sample-app
    spec:
      containers:
        - name: app
          image: nginx:1.25
---
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: sample-app-propagation
  namespace: default
spec:
  resourceSelectors:
    - apiVersion: apps/v1
      kind: Deployment
      name: sample-app
  placement:
    clusterAffinity:
      clusterNames:
        - prod-bj-01
        - prod-sh-01
EOF
```
> **集成建议**: 对于已经使用 Argo CD 的团队，可先用 ApplicationSet 覆盖 80% 的多集群分发场景；当出现跨集群调度、故障迁移、动态扩集群需求时，再叠加 Karmada 或 OCM，避免过度工程化。

### 3.10 Step 10 — 边缘与弱网集群的 GitOps 策略

边缘集群通常具备带宽受限、网络不稳定、与控制面间歇性断连的特点，push-based Argo CD 模式可能无法保证一致性。

#### 3.10.1 Pull-Based 模式

在边缘场景中使用 Flux 或 Argo CD Agent 模式，让边缘集群主动从 Git 拉取配置：

```bash
# Flux 在边缘集群安装并监听上游 Git
flux bootstrap github \
  --owner=org \
  --repository=edge-gitops \
  --branch=main \
  --path=clusters/prod-edge-01 \
  --personal=false
```

#### 3.10.2 离线自治与缓存

- 在边缘节点预置镜像缓存与 Helm chart 缓存，降低断网时启动失败概率；
- 配置 `syncOptions` 中的 `RespectIgnoreDifferences=true`，避免控制面侧字段差异触发持续同步；
- 使用 OCM ManifestWork 的 `DeleteOption` 与 `UpdateStrategy` 控制边缘集群在断网期间的资源保留策略。

#### 3.10.3 带宽优化

- 对边缘应用使用单镜像、精简 Helm chart，避免大体积 ConfigMap；
- 设置 `requeueAfterSeconds: 3600` 降低 ApplicationSet 对 Git 的轮询频率；
- 在边缘侧使用本地 Git 镜像或对象存储作为配置缓存层。

---

## 4. 关键检查点与验证命令

### 4.1 每日巡检清单

| 检查项 | 通过标准 | 验证命令 |
|:---|:---|:---|
| 集群注册完整 | 所有生产集群已在 Argo CD 注册 | `argocd cluster list` |
| 标签规范一致 | 每个集群携带 environment/region/tier/criticality 标签 | `kubectl get secret -n argocd -l argocd.argoproj.io/secret-type=cluster -o json` |
| 应用全部 Synced | 无 OutOfSync 应用 | `argocd app list` |
| 应用全部 Healthy | 无 Degraded/Progressing 超时 | `argocd app list` |
| Secret 同步正常 | ExternalSecret 状态为 Ready | `kubectl get externalsecrets -A` |
| 无持续同步失败 | 近 1 小时无 Error 阶段同步 | `kubectl logs -n argocd deploy/argocd-application-controller` |

### 4.2 变更前检查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 渲染目标清单
argocd app manifests payment-prod-bj-01 --source-live | head -n 100

# 2. 检查目标集群是否存在且可访问
argocd cluster get prod-bj-01

# 3. 确认 AppProject 允许该目标
kubectl get appproject production-business -n argocd -o yaml | grep -A 20 destinations

# 4. 验证 ApplicationSet 生成结果（不实际同步）
kubectl apply --dry-run=client -f applicationset-payment.yaml
```
### 4.3 变更后验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 等待并验证同步完成
argocd app wait payment-prod-bj-01 --health --sync

# 验证目标资源版本
kubectl --context=prod-bj-01 get deployment payment-service -n payment -o jsonpath='{.spec.template.spec.containers[0].image}'

# 验证多集群版本一致性
for ctx in prod-bj-01 prod-sh-01 prod-sz-01; do
  echo -n "$ctx: "
  kubectl --context=$ctx get deployment payment-service -n payment -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
done
```
---

## 5. 回滚/应急方案

### 5.1 ApplicationSet 级回滚

ApplicationSet 本身不直接回滚，通过回退 Git 中 ApplicationSet 引用的 `targetRevision` 或参数，触发所有目标 Application 回滚。

```bash
# 方式 1: 回退 Git 分支
# 将 production 分支 revert 到上一稳定 commit，Argo CD 自动收敛

# 方式 2: 临时固定 ApplicationSet 参数
argocd app set payment-prod-bj-01 --parameter image.tag=v2.2.9
```

### 5.2 单集群回滚

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看历史 revision
argocd app history payment-prod-bj-01

# 回滚到指定 revision
argocd app rollback payment-prod-bj-01 42

# 或使用 kubectl
kubectl rollout undo deployment/payment-service -n payment --context=prod-bj-01
```
### 5.3 全局应急暂停

当发现致命缺陷时，立即暂停所有 Fleet 自动同步：

```bash
# 暂停 ApplicationSet 下所有应用
argocd app list -l fleet-app=payment-service -o name | xargs -I {} argocd app set {} --sync-policy none

# 确认已暂停
argocd app list -l fleet-app=payment-service -o json | jq -r '.[] | [.metadata.name, .spec.syncPolicy.automated] | @tsv'

# 修复后批量恢复
argocd app list -l fleet-app=payment-service -o name | xargs -I {} argocd app set {} --sync-policy automated --self-heal
```

### 5.4 Secret 泄露/轮换应急

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 立即撤销 Vault/AWS SM 中旧凭据
# 2. 更新 ExternalSecret 引用的 key 版本
kubectl patch externalsecret payment-db-credentials -n payment --type merge \
  -p '{"spec":{"dataFrom":[{"extract":{"key":"secret/data/production/payment/db"}}]}}'

# 3. 强制刷新
kubectl annotate externalsecret payment-db-credentials -n payment force-sync="$(date +%s)" --overwrite

# 4. 验证所有集群 Secret 已更新
for ctx in prod-bj-01 prod-sh-01 prod-sz-01; do
  kubectl --context=$ctx get secret payment-db-credentials -n payment -o jsonpath='{.metadata.resourceVersion}'
  echo
done
```
---

## 6. 风险与注意事项

### 6.1 标签误配导致误部署

**风险**: 错误的 `matchLabels` 可能将生产组件部署到 staging 集群，或反之。

**缓解**:
- 禁止在 ApplicationSet 中使用单一标签做环境判断，必须组合 `environment` + `tier` + `region`；
- AppProject 的 `destinations` 显式列出允许的服务器，不使用 `*`；
- 变更 ApplicationSet 前必须通过 `argocd app manifests` 预览生成结果。

### 6.2 Secret 跨集群泄露

**风险**: 同一 ExternalSecret 被分发到多集群后，若某个集群被攻破，可能导致 Secret 横向扩散。

**缓解**:
- 按集群/环境拆分 Vault path，避免所有集群读取同一 key；
- 启用 Secret 的 `deletionPolicy: Retain` 并配合定期轮换；
- 对敏感 Secret 使用按集群独立的 ExternalSecret 模板。

### 6.3 控制器性能瓶颈

**风险**: 当 ApplicationSet 生成数百个 Application 时，Argo CD application-controller 与 repo-server 可能成为瓶颈。

**缓解**:
- 增加 `controller.status.processors` 与 `--repo-server-timeout-seconds`；
- 按业务域拆分多个 Argo CD 实例或 shard；
- 对大规模 Fleet 使用 OCM/Karmada 作为分发层，Argo CD 仅管理高层策略。

### 6.4 网络分区导致同步不一致

**风险**: 目标集群与 Argo CD 控制面网络中断，导致 Git 更新无法同步。

**缓解**:
- 为 Argo CD 控制面配置多可用区部署；
- 对边缘集群使用 pull-based GitOps（如 Flux）或 OCM Agent 模式；
- 设置 `argocd_app_info{sync_status="Unknown"}` 告警。

### 6.5 ApplicationSet 删除级联

**风险**: 删除 ApplicationSet 默认会级联删除其生成的所有 Application 及关联资源。

**缓解**:
- 使用 `spec.syncPolicy.preserveResourcesOnDeletion: true` 保留资源；
- 删除前执行 `argocd app list -l app.kubernetes.io/instance=<appset-name>` 确认影响范围。

---

## 7. 相关 Runbook / 推荐阅读

### 本域资料

- [[11-发布变更/00-总览/99-production-readiness-operations-guide.md|发布与变更管理 生产就绪运维指南]]
- [[11-发布变更/01-GitOps/99-argo-cd-gitops-guide.md|Argo CD 企业级 GitOps 实践指南]]
- [[11-发布变更/01-GitOps/99-helm-production-guide.md|Helm 生产指南]]
- [[11-发布变更/01-GitOps/07-gitops-security-compliance.md|GitOps 安全合规实践]]
- [[11-发布变更/04-变更管理/03-change-rollback-playbook.md|变更回滚操作手册]]

### 多集群与联邦资料

- [[18-云厂商/07-多云混合/08-multicloud-federation-karmada.md|Karmada 多云联邦实践]]
- [[17-系统基础/06-知识字典/platform-engineering/cluster-api-and-fleet-management.md|Cluster API 与 Fleet 管理术语]]
- [[17-系统基础/06-知识字典/platform-engineering/karmada.md|Karmada 术语]]

### 相关域生产就绪指南

- [[18-云厂商/00-总览/99-production-readiness-operations-guide.md|云厂商 生产就绪运维指南]]
- [[12-可靠性/00-总览/99-production-readiness-operations-guide.md|可靠性工程 生产就绪运维指南]]
- [[13-生产运维/00-总览/99-production-readiness-operations-guide.md|生产运维 生产就绪运维指南]]
- [[03-清单模式/00-总览/99-production-readiness-operations-guide.md|清单与模式 生产就绪运维指南]]
- [[08-安全/00-总览/99-production-readiness-operations-guide.md|安全合规 生产就绪运维指南]]
- [[09-可观测性/01-总览/99-production-readiness-operations-guide.md|可观测性 生产就绪运维指南]]

### 安全与网络

- [[08-安全/02-网络安全/21-multicluster-security.md|多集群安全]]
- [[05-网络/00-总览/99-production-readiness-operations-guide.md|网络流量 生产就绪运维指南]]

---

## 8. 总结

多集群 Fleet GitOps 的核心不在于把单集群 Argo CD 简单放大，而在于建立四层控制：

1. **元数据与边界控制**: 统一集群标签、AppProject 目标限定、RBAC 最小权限；
2. **声明式分发控制**: 通过 ApplicationSet 的 `clusters`/`git`/`list`/`matrix` 生成器实现"一次定义，多集群收敛"；
3. **变更风险控制**: 跨集群 Secret 同步、分批晋升门控、金丝雀分析、人工审批；
4. **运行时控制**: 漂移检测、可观测大盘、自动化告警、分级回滚。

在生产环境中，建议每季度复核一次集群注册状态、ApplicationSet 生成规则、AppProject 边界与 Secret 权限，确保 Fleet 规模扩大后仍能维持一致、安全、可回滚的 GitOps 运维能力。

---

*本指南作为 发布变更 的多集群 Fleet GitOps 入口文档，应随集群规模、工具链与组织流程的演进每季度复核一次。*


<!-- risk-assessed -->
