---
title: Kubernetes 多集群 Fleet GitOps 运维指南
description: 覆盖 Argo CD ApplicationSet 模式、Karmada/OCM/Cluster API 概述、跨集群 Secret 同步、发布门控、漂移检测与 Fleet 可观测性的生产级运维手册
summary: 覆盖 Argo CD ApplicationSet 模式、Karmada/OCM/Cluster API 概述、跨集群 Secret 同步、发布门控、漂移检测与 Fleet 可观测性的生产级运维手册
category: release-change-management
tags:
- production
- best-practices
- playbook
- gitops
- fleet
- multi-cluster
- applicationset
- karmada
- ocm
- cluster-api
- secret-sync
- drift-detection
- observability
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
- Kubernetes 多集群 Fleet GitOps 运维指南 是什么
- ApplicationSet 怎么用
- Karmada OCM Cluster API 区别
- 跨集群 Secret 同步方案
- GitOps 发布门控怎么实现
- Fleet 漂移检测怎么做
trigger_keywords:
- fleet
- multi-cluster
- applicationset
- karmada
- ocm
- cluster-api
- secret sync
- promotion gate
- drift detection
- gitops
prerequisites:
- argocd-basics
- gitops-basics
- kubernetes-concepts
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

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产运维 Runbook

本指南面向管理 10+ 集群的 SRE 与平台工程师，系统阐述如何以 GitOps 方式运营 Kubernetes Fleet。内容覆盖 Argo CD ApplicationSet 的多集群分发模式、Karmada/OCM/Cluster API 的定位与选型、跨集群 Secret 同步的安全实践、发布门控与漂移检测，以及 Fleet 级别的可观测性。目标是在保持单集群自治的同时，实现全局一致性、可审计的变更与可量化的发布风险。Fleet 规模下的核心挑战不是“如何部署到一个集群”，而是“如何在数百个集群上安全、可预测、可回滚地推进变更”，这要求将 GitOps 与治理、观测、门控深度集成。本指南提供的模式与命令均来自生产实践，可直接作为平台团队建设 Fleet 运维体系的起点。

---

## 1. 适用场景与范围

- **Fleet 定义**：由同一平台团队管理、共享身份/网络/可观测基线的多 Kubernetes 集群集合，通常按环境（dev/staging/prod）、区域、租户或业务线划分。
- **Argo CD ApplicationSet**：负责将应用声明以声明式方式分发到多个目标集群。
- **多集群调度层**：Karmada、Open Cluster Management（OCM）、Cluster API（CAPI）分别解决应用调度、策略治理、集群生命周期问题。
- **跨集群 Secret 同步**：在保持 Secret 不落地仓库的前提下，将证书、镜像拉取凭证、外部凭据同步到多个集群。
- **发布门控**：基于 SLO、测试、人工审批的跨环境/跨区域自动推进。
- **漂移检测**：识别并纠正集群实际状态与 Git 声明之间的差异，确保 Fleet 长期一致性。

### 1.1 Fleet GitOps 分层架构

典型的 Fleet GitOps 架构分为四层：

- **配置层**：Git 仓库保存应用声明、环境覆盖、策略与模板。仓库结构应清晰区分全局配置、环境配置与租户配置。
- **分发层**：Argo CD ApplicationSet 根据集群元数据（标签、区域、环境）生成 Application，并下发到各集群。
- **调度/治理层**：Karmada 负责跨集群应用调度，OCM 负责集群注册、策略下发与批量运维，Cluster API 负责集群生命周期。
- **观测层**：Prometheus + Thanos/Mimir 聚合多集群指标，Loki/Vector 聚合日志，Jaeger/Tempo 聚合链路。

四层之间通过声明式 API 解耦，使得每一层可以独立演进。例如，新增一个集群只需在 OCM 注册并打上标签，ApplicationSet 会自动为其生成应用；删除集群则只需移除注册，GitOps 会自动清理相关 Application。

---

## 2. 前置条件与工具

### 2.1 基础设施前提

- 每个集群已部署 Argo CD Agent 或具备网络可达的 Argo CD Server。
- 中央 Git 仓库采用 monorepo 或多 repo 结构，明确分支/目录对应环境。
- 具备跨集群身份体系：OIDC、SPIFFE/SPIRE 或云厂商 RAM/IRSA。
- 对象存储或 Prometheus Remote Write 用于聚合多集群指标。
- 具备跨集群事件与告警聚合能力，能够将多个集群的告警统一路由到同一值班平台。
- 建立 GitOps 变更审计日志，记录每次同步的提交者、版本、目标集群与结果。
- 建立标准化的应用目录结构与命名规范，确保 ApplicationSet 模板可复用。

### 2.2 必备工具

| 工具 | 用途 | 推荐版本 |
|------|------|----------|
| Argo CD | GitOps 持续交付 | v2.12+ |
| ApplicationSet | 多集群应用分发 | 随 Argo CD |
| Karmada | 跨集群应用调度 | v1.11+ |
| OCM | 集群注册、Work、Policy、Placement | v0.15+ |
| Cluster API | 声明式集群生命周期 | v1.8+ |
| External Secrets Operator | 跨集群 Secret 同步 | v0.10+ |
| Prometheus + Thanos/Mimir | 全局指标聚合 | v2.55+ / v2.15+ |

---

## 3. 标准操作流程

### 3.1 Argo CD ApplicationSet 分发模式

ApplicationSet 是 Fleet GitOps 的核心控制器，它根据生成器（Generator）动态创建多个 Argo CD Application。与手动维护大量 Application 不同，ApplicationSet 能够根据集群标签、Git 目录、外部 SCM 或矩阵组合自动生成应用，大幅降低运维复杂度。选择生成器时应兼顾灵活性与可维护性，避免为了覆盖所有边缘场景而使模板过于复杂。

#### 推荐 Git 仓库结构

```
fleet-gitops/
├── apps/
│   ├── payment/
│   │   ├── base/
│   │   └── overlays/
│   │       ├── dev/
│   │       ├── staging/
│   │       └── production/
│   └── order/
├── platform/
│   ├── monitoring/
│   ├── ingress/
│   └── cert-manager/
├── policies/
│   ├── kyverno/
│   └── opa-gatekeeper/
├── clusters/
│   ├── beijing-prod/
│   └── shanghai-prod/
└── applicationsets/
    ├── platform-base.yaml
    └── apps-matrix.yaml
```

该结构将应用、平台组件、策略与集群配置分离，便于权限控制与审计。环境覆盖放在应用目录内，集群特定配置放在 `clusters/` 目录。

#### Cluster Generator（按集群列表）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: platform-base
  namespace: argocd
spec:
  generators:
  - clusters:
      selector:
        matchLabels:
          env: production
  template:
    metadata:
      name: '{{name}}-platform-base'
    spec:
      project: platform
      source:
        repoURL: https://github.com/kudig/fleet-gitops.git
        targetRevision: HEAD
        path: platform-base/overlays/{{metadata.labels.env}}
      destination:
        server: '{{server}}'
        namespace: kube-system
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
        - CreateNamespace=true
```

#### Git Generator（按目录/文件矩阵）

```yaml
spec:
  generators:
  - matrix:
      generators:
      - git:
          repoURL: https://github.com/kudig/fleet-gitops.git
          revision: HEAD
          directories:
          - path: apps/*/overlays/production
      - clusters:
          selector:
            matchLabels:
              env: production
```

#### SCM Provider Generator（动态发现租户仓库）

适用于 SaaS 多租户场景，自动为每个租户仓库创建 Application。通过扫描 GitHub/GitLab 组织下的仓库列表，ApplicationSet 可为每个租户生成独立应用，避免手动维护大量 Application。该模式要求租户仓库目录结构标准化，例如 `tenants/<tenant>/overlays/<env>`。

#### 生成器选型建议

| 场景 | 推荐生成器 | 说明 |
|------|------------|------|
| 固定集群集合 | Cluster Generator | 简单直观，适合环境稳定的 Fleet |
| 应用数量多、目录结构统一 | Git Generator | 按目录自动发现应用 |
| 环境 × 集群矩阵 | Matrix Generator | 组合 Git 与 Cluster 生成器 |
| 多租户 SaaS | SCM Provider Generator | 动态发现租户仓库 |

选择生成器时应考虑可维护性。过于复杂的 Matrix 组合会导致 Application 数量难以预测，建议定期审计 ApplicationSet 生成的 Application 总数。

### 3.2 Karmada / OCM / Cluster API 定位

选择合适的调度与治理技术，取决于 Fleet 的规模与业务需求。下表对三种主流技术进行了定位对比：

| 技术 | 核心能力 | 适用场景 | 与 Argo CD 关系 |
|------|----------|----------|-----------------|
| **Karmada** | 跨集群 Deployment/Service/Ingress 调度、OverridePolicy、PropagationPolicy | 应用层多活、灰度发布、故障迁移 | Argo CD 管理 Karmada 控制面上的 CRD |
| **OCM** | 集群注册（ManagedCluster）、Work（资源下发）、Placement、Policy（安全/合规） | 集群联邦治理、策略即代码、批量运维 | Argo CD 可将 Policy/Placement 同步到 Hub |
| **Cluster API** | 声明式创建、升级、销毁 Kubernetes 集群 | 集群生命周期自动化、节点池弹性 | Argo CD 管理 CAPI 的 Cluster/MachineDeployment |

推荐组合：
- **轻量 Fleet**：Argo CD ApplicationSet + OCM 集群注册。适合 10–50 个集群、应用无需跨集群调度的场景。
- **应用多活 Fleet**：Argo CD + Karmada + OCM Policy。适合需要在多个区域同时运行同一应用、并根据流量或故障自动迁移的场景。
- **全生命周期自动化**：Argo CD + CAPI + OCM + Karmada。适合需要频繁创建/销毁集群、节点池弹性要求高的超大规模 Fleet。

#### Cluster API 集群创建示例

```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: cluster-shanghai
  namespace: default
spec:
  clusterNetwork:
    pods:
      cidrBlocks:
      - 192.168.0.0/16
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: cluster-shanghai-cp
  infrastructureRef:
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
    kind: <Provider>Cluster
    name: cluster-shanghai
```

Argo CD 管理上述 Cluster 对象后，CAPI 会自动在云厂商上创建对应的 Kubernetes 集群。集群创建完成后，OCM 自动将其注册到 Hub，ApplicationSet 随即开始下发应用。

#### Karmada 典型工作流

```yaml
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: nginx-propagation
  namespace: default
spec:
  resourceSelectors:
  - apiVersion: apps/v1
    kind: Deployment
    name: nginx
  placement:
    clusterAffinity:
      clusterNames:
      - cluster-beijing
      - cluster-shanghai
    replicaScheduling:
      replicaSchedulingType: Divided
      replicaDivisionPreference: Weighted
      weightPreference:
        staticWeightList:
        - targetCluster:
            clusterNames: [cluster-beijing]
          weight: 70
        - targetCluster:
            clusterNames: [cluster-shanghai]
          weight: 30
```

Karmada 会根据 PropagationPolicy 将 Deployment 分发到多个集群，并按权重分配副本。当某个集群不可用时，Karmada 可自动将副本漂移到健康集群，实现应用层多活。

#### OCM 集群注册示例

```bash
# Hub 集群
clusteradm init --wait
clusteradm get token

# 受管集群
clusteradm join --hub-token <token> --hub-apiserver <hub-api-server> --cluster-name cluster-beijing

# Hub 集群接受注册
clusteradm accept --clusters cluster-beijing
```

注册后，Hub 集群可通过 ManifestWork 向受管集群下发资源，也可通过 Policy 强制执行安全基线。

### 3.3 跨集群 Secret 同步

Secret 绝不应以明文提交到 Git。在 Fleet 中， Secret 需要在多个集群间同步，但同步过程必须保证安全、可控、可审计。推荐采用 External Secrets Operator（ESO）+ 中央 Vault/AWS Secrets Manager：

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: vault-backend
spec:
  provider:
    vault:
      server: https://vault.internal:8200
      path: secret
      version: v2
      auth:
        kubernetes:
          mountPath: kubernetes
          role: external-secrets
---
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-db-creds
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    kind: ClusterSecretStore
    name: vault-backend
  target:
    name: db-creds
  data:
  - secretKey: username
    remoteRef:
      key: production/app/db
      property: username
```

通过 ApplicationSet 将 ExternalSecret 下发到所有目标集群，各集群 ESO 独立拉取并创建本地 Secret。该模式的优势在于：Secret 值不落地 Git，各集群独立拉取避免了单点泄露导致全网风险，且可以按集群设置不同的 refreshInterval。

对于需要跨集群共享的 CA bundle 或镜像拉取凭证，可使用 `ClusterSecretStore` 配合命名空间级 `ExternalSecret`。对于高敏感凭据（如云厂商 AK/SK），建议直接采用云厂商工作负载身份（如 IRSA、GKE Workload Identity）替代静态 Secret。

### 3.4 发布门控

发布门控防止未经测试或不符合 SLO 的变更进入下游环境。在 Fleet 场景下，门控不仅要考虑单一集群的健康状况，还要综合考虑多个区域、多个环境的指标与审批状态。典型阶段：

```
Git PR → CI 测试 → Merge → Dev 同步 → 自动化验收 → Staging 同步 → 人工审批 → Production 同步 → SLO 验证
```

实现方式：

1. **分支门控**：`main` → `release/staging` → `release/production`，通过 PR + CODEOWNERS 控制。每个环境对应一个 Git 分支或标签，只有经过验证的变更才能进入下游分支。
2. **Argo CD ApplicationSet 条件**：
   ```yaml
   template:
     spec:
       source:
         targetRevision: release/production
   ```
   通过修改 ApplicationSet 的 `targetRevision` 控制生产环境接收的 Git 版本。
3. **SLO 门控**：基于 Prometheus 查询结果，当错误预算充足时才允许同步生产环境。例如，当 1 小时错误预算消耗超过 5% 时，自动阻断生产同步。
4. **人工审批**：Argo CD 启用 `syncWindow` 或外部 Webhook 在 Production 同步前暂停等待审批。审批记录应保存到审计日志，便于事后追溯。

#### 金丝雀发布与 Fleet

对于跨多个区域的生产环境，推荐先向一个区域发布金丝雀版本，观察 30 分钟至 1 小时 SLO 指标后再推广到其他区域。Argo CD ApplicationSet 可通过集群标签 `canary: true` 控制首批目标：

```yaml
generators:
- clusters:
    selector:
      matchLabels:
        env: production
        canary: "true"
```

### 3.5 漂移检测与自我修复

漂移（drift）是指集群实际状态与 Git 声明不一致。漂移可能由运维人员手动修改、控制器冲突、外部系统变更或同步失败引起。漂移若长期存在，会降低 GitOps 的可审计性与可回滚能力。

```yaml
spec:
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
```

`selfHeal: true` 会在检测到漂移后自动同步 Git 声明。生产环境建议结合 Argo CD 的 `Application` 历史记录功能，保留最近的同步版本，便于快速回滚。

对于无法自动修复的 drift（如某些控制器会反复改写 annotations），应在 Git 声明中设置 `ignoreDifferences`，避免无限同步循环。

同时配置告警：

```yaml
# PrometheusRule 示例
- alert: ArgoCDApplicationOutOfSync
  expr: argocd_app_info{sync_status!="Synced"} == 1
  for: 10m
  labels:
    severity: warning
  annotations:
    summary: "Argo CD 应用 {{ $labels.name }} 处于漂移状态"
```

### 3.6 Fleet 可观测性

Fleet 可观测性不仅是要看到每个集群的状态，更要能横向比较、快速定位异常集群与异常应用。由于 Fleet 中集群数量多、分布广，单一集群的告警很容易被淹没，因此需要建立分级告警机制：集群级告警由本地值班处理，跨集群或平台级告警由平台 SRE 处理。

#### 指标聚合

在每个集群部署 Prometheus，通过 Remote Write 或 Thanos Sidecar 将指标发送到全局 Thanos Query/Mimir。必须统一指标标签，至少包含 `cluster`、`region`、`environment`、`team`。标签不一致会导致全局查询结果缺失或重复。

```yaml
# Thanos Query 查询示例
sum by (cluster) (rate(container_cpu_usage_seconds_total{namespace="production"}[5m]))
```

对于边缘或网络受限集群，可先在本地聚合后再上传，减少跨地域带宽消耗。

#### 日志与链路

边缘集群日志量可能很大，建议在集群侧部署 Fluent Bit/Vector 做过滤与压缩，只将 ERROR 级别与关键业务日志上传到中心 Loki。链路追踪同样可采样上传，降低带宽压力。

#### 关键看板

- 各集群 Application Sync 状态热力图
- 跨集群 Deployment 副本分布与差异
- Secret 同步成功率
- 各集群资源水位与成本

### 3.7 变更审计与合规

Fleet GitOps 的优势之一是天然具备变更审计能力。每次 Git 提交都对应一次潜在变更，Argo CD 会记录谁、在何时、将哪个版本同步到了哪个集群。审计要求较高的组织应将这些记录导出到 SIEM 或对象存储，保存期限不少于 180 天。

此外，可通过 OPA/Kyverno Policy 禁止在集群内直接修改 GitOps 管理的资源：

```yaml
validationFailureAction: Enforce
rules:
- name: protect-gitops-resources
  match:
    resources:
      annotations:
        argocd.argoproj.io/instance: "?*"
  validate:
    message: "GitOps 管理的资源不允许手动修改"
    deny:
      conditions:
      - key: "{{ request.operation }}"
        operator: Equals
        value: UPDATE
```

---

## 4. 关键检查点与验证命令

| 检查项 | 命令 | 合格标准 |
|--------|------|----------|
| ApplicationSet 生成应用 | `kubectl get applicationset -n argocd` | 所有生成器无错误 |
| 应用同步状态 | `argocd app list` | Synced/Healthy |
| 集群注册状态 | `kubectl get managedcluster` | True/Available |
| Karmada 资源分发 | `kubectl get propagationpolicy -n <ns>` | 所有目标集群已绑定 |
| Secret 同步 | `kubectl get externalsecret -A` | Ready=True |
| 漂移告警 | Prometheus alert | 无持续 10 分钟以上 OutOfSync |
| 全局指标聚合 | Thanos Query | 可查询所有集群指标 |

---

## 5. 回滚/应急方案

- **单次发布故障**：在 Argo CD 中回滚到上一个 Git commit 或已同步的 Revision。
  ```bash
  argocd app rollback <app> <revision>
  ```
  回滚后应持续观察业务指标，确认问题已修复后再继续后续发布。
- **全集群配置漂移**：启用 `selfHeal: true` 并手动触发同步；若 drift 由外部运维手动修改导致，需审计并补充 GitOps 策略，避免重复发生。
- **Secret 泄露**：立即轮换 Vault/Secrets Manager 中的密钥，ESO 会在 `refreshInterval` 后自动同步新值；必要时手动触发同步。
  ```bash
  kubectl annotate externalsecret <name> -n <ns> force-sync=$(date +%s)
  ```
- **区域级集群失联**：将 ApplicationSet 生成器中的目标集群临时注释或调整 label selector，将流量切换至健康区域。同时检查全局 DNS 与负载均衡器是否已自动切换。
- **大规模同步失败**：若 ApplicationSet 生成大量 Application 后同步失败，先暂停自动同步，排查模板错误，修复后分批重新同步，避免所有集群同时进入异常状态。

---

## 6. 风险与注意事项

1. **避免 ApplicationSet 过度矩阵化**：Git + Cluster + List 多重生成器易导致应用数量爆炸，建议每个 ApplicationSet 管理的应用数 ≤ 200。应定期审计生成的 Application 数量，删除废弃模板。
2. **Secret 不可落入 Git**：任何含 base64 编码 Secret 的 PR 必须通过 CI 扫描（如 `gitleaks`）阻断。同时应在 Git 仓库启用分支保护，禁止直接推送到环境分支。
3. **跨集群网络策略**：Fleet 内集群间通信应通过 PrivateLink/Cloud Interconnect/VPN，禁止公网暴露 API Server。OCM 与 Karmada 的 Hub 集群是高价值目标，必须加固访问控制。
4. **版本一致性**：Argo CD、Karmada、OCM 的版本需与目标 Kubernetes 版本兼容，升级前查阅官方 matrix。建议先在非生产 Fleet 验证后再推广。
5. **drift 自动修复风险**：生产环境建议关闭 `prune` 的自动执行，由人工确认后再删除资源。误删 StatefulSet 或 Namespace 可能导致数据丢失。
6. **Git 仓库成为单点故障**：应配置仓库高可用、异地镜像与只读副本，避免 Git 平台故障导致无法同步。
7. **权限最小化**：不同团队应对不同 Application 拥有只读或读写权限，避免跨租户误操作。Argo CD Projects 可用于隔离权限。

---

## 7. 相关 Runbook / 推荐阅读

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-08-release-change-management/02-production-readiness-operations-guide|发布变更管理 生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-08-release-change-management/02-production-readiness-operations-guide|平台工程 生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-08-release-change-management/01-gitops/05-argo-cd-gitops-guide|Argo CD GitOps 指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-08-release-change-management/01-gitops/07-flux-gitops-guide|Flux GitOps 指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-08-release-change-management/01-gitops/05-gitops-security-compliance|GitOps 安全合规]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-09-reliability-engineering/02-slo-sli/02-slo-implementation-guide|SLO 设定与实施指南]]


<!-- risk-assessed -->
