---
title: Operator 生命周期管理：OLM、升级与 Day-2 运维
description: 面向阿里云 ACK / 专有云 ASO 的 Kubernetes Operator 生命周期管理，涵盖 OLM 安装、订阅升级、Day-2
  运维与故障排查
summary: 面向阿里云 ACK / 专有云 ASO 的 Kubernetes Operator 生命周期管理，涵盖 OLM 安装、订阅升级、Day-2 运维与故障排查
category: domain
tags:
- kubernetes
- operator
- olm
- operator-lifecycle-manager
- crd
- upgrade
- day-2
- ack
- aso
- lifecycle
- management
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- Kubernetes 管理员
estimated_read_time: 20min
intent_queries:
- Operator 生命周期管理是什么
- 如何在 ACK 上安装 OLM
- Kubernetes Operator 升级与回滚最佳实践
trigger_keywords:
- OLM
- Operator Lifecycle Manager
- Operator 升级
- Operator 回滚
- Day-2 运维
- 生命周期管理
prerequisites:
- kubectl-basics
- crd-basics
- helm-basics
- operator-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Operator 生命周期管理：OLM、升级与 Day-2 运维

## 目录

- [1. 为什么需要 OLM](#1-为什么需要-olm)
- [2. OLM 核心概念](#2-olm-核心概念)
- [3. 在 ACK / 专有云 ASO 上安装 OLM](#3-在-ack-专有云-aso-上安装-olm)
- [4. CatalogSource 生命周期管理](#4-catalogsource-生命周期管理)
- [5. 订阅与自动升级](#5-订阅与自动升级)
- [6. Operator 升级与回滚](#6-operator-升级与回滚)
- [7. Day-2 运维](#7-day-2-运维)
- [8. 多租户与权限隔离](#8-多租户与权限隔离)
- [9. OLM 与 Helm 的选型对比](#9-olm-与-helm-的选型对比)
- [10. 常见故障排查](#10-常见故障排查)
- [11. 生产检查清单](#11-生产检查清单)
- [12. Operator 升级风险与回滚演练](#12-operator-升级风险与回滚演练)
- [13. 相关文档](#13-相关文档)
## 1. 为什么需要 OLM

在阿里云 ACK 或专有云 ASO 中，数据库、消息队列、可观测性等能力往往通过 Operator 交付。随着 Operator 数量增长，手动管理 CRD、RBAC、Deployment 的部署与升级会变得脆弱且难以审计。Operator Lifecycle Manager（OLM）提供以下能力：

- **声明式安装**：通过 `Subscription` 声明期望的 Operator 与通道。
- **依赖解析**：自动安装 Operator 所需的依赖包与 CRD。
- **受控升级**：支持自动、手动或指定版本升级。
- **多租户隔离**：通过 `OperatorGroup` 限制 Operator 的作用命名空间。
- **版本约束**：可锁定 `startingCSV`，避免意外升级到不兼容版本。

使用 OLM 后，平台团队可以将 Operator 视为标准软件包，统一进行版本审批、灰度发布与回滚。在阿里云 ACK 多集群场景中，还可以结合 ACK One 的 GitOps 能力，将 Subscription 与 OperatorGroup 以 Git 仓库方式管理，实现跨集群 Operator 的一致交付。

## 2. OLM 核心概念

| 资源 | 作用 | 类比 |
|------|------|------|
| `ClusterServiceVersion (CSV)` | Operator 的发行版本描述，包含 Deployment、RBAC、CRD 等 | RPM / DEB 包 |
| `CatalogSource` | 存放 CSV 与软件包定义的索引源 | YUM / APT 源 |
| `Subscription` | 用户订阅某个 Operator 的通道，触发安装或升级 | 订阅计划 |
| `InstallPlan` | 由 Subscription 生成的安装/升级计划 | 安装脚本 |
| `OperatorGroup` | 定义 Operator 可观测和管理的命名空间范围 | 租户边界 |

```
用户创建 Subscription
       │
       ▼
OLM 解析 CatalogSource → 生成 InstallPlan
       │
       ▼
InstallPlan 创建 CSV、CRD、Deployment、RBAC
       │
       ▼
Operator Pod 运行，开始协调 CR
```

OLM 中的 olm-operator 负责解析依赖并生成 InstallPlan，catalog-operator 负责从 CatalogSource 中提取包信息并创建 Pod 来提供 gRPC 索引服务。理解这两个核心控制器的分工，有助于在 InstallPlan 卡住或 CatalogSource 无法连接时快速定位问题。

## 3. 在 ACK / 专有云 ASO 上安装 OLM

### 3.1 安装 OLM

推荐使用官方 release 脚本安装 OLM 0.x 版本。以下命令适用于标准 Kubernetes 集群，ACK 托管版同样兼容：

```bash
# 下载并安装 OLM 0.28.0，该版本支持 Kubernetes 1.28 - 1.32
export OLM_VERSION=v0.28.0
curl -L https://github.com/operator-framework/operator-lifecycle-manager/releases/download/${OLM_VERSION}/install.sh \
  | bash -s ${OLM_VERSION}
```

安装完成后验证核心组件：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认 olm-operator 与 catalog-operator 已运行
kubectl get pods -n olm

# 查看默认 catalog source
kubectl get catalogsources -n olm
```
### 3.2 在专有云 ASO 中的注意事项

专有云 ASO 通常为离线环境，无法直接访问 Red Hat / OperatorHub 公网源。需要：

1. 在内网 Harbor 或 OSS 中搭建私有 `CatalogSource`。
2. 将 Operator bundle 镜像同步到内网镜像仓库。
3. 在 `CatalogSource` 中指定内网镜像地址与 pullSecret。

```yaml
apiVersion: operators.coreos.com/v1alpha1
kind: CatalogSource
metadata:
  name: internal-operators
  namespace: olm
spec:
  sourceType: grpc
  image: registry.internal.aso/olm/catalog:latest
  displayName: Internal Operator Catalog
  publisher: ASO Platform Team
  secrets:
    - internal-registry-secret
```

在 ASO 环境中，建议将 catalog 镜像构建为 CI/CD 流水线的一部分，每次新增 Operator 时自动更新 catalog 索引并推送到内网仓库。

## 4. CatalogSource 生命周期管理

### 4.1 构建私有 Catalog

在专有云 ASO 离线环境中，私有 Catalog 是 OLM 正常工作的前提。使用 `opm` 工具从 bundle 镜像构建 catalog，并将索引镜像推送到内网镜像仓库，可以确保所有 Operator 安装不依赖外部网络。

```bash
# 初始化并添加 Operator bundle 到私有 catalog
opm index add \
  --bundles registry.internal.aso/operators/cloudnative-pg:v1.22.0 \
  --tag registry.internal.aso/olm/catalog:v20260629 \
  --pull-tool podman

# 推送 catalog 镜像到内网仓库
podman push registry.internal.aso/olm/catalog:v20260629
```

### 4.2 CatalogSource 健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CatalogSource 状态，确保状态为 CONNECTED
kubectl get catalogsource internal-operators -n olm -o jsonpath='{.status.connectionState.lastObservedState}'

# 查看 catalog Pod 日志
kubectl logs -n olm deployment/internal-operators --tail=100
```
## 5. 订阅与自动升级

### 5.1 创建 OperatorGroup 与 Subscription

以下示例在 `database-operators` 命名空间订阅 CloudNativePG Operator，并限制其仅管理同命名空间：

```yaml
apiVersion: operators.coreos.com/v1
kind: OperatorGroup
metadata:
  name: database-operators
  namespace: database-operators
spec:
  targetNamespaces:
    - database-operators
---
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: cloudnative-pg
  namespace: database-operators
spec:
  channel: stable
  name: cloudnative-pg
  source: operatorhubio-catalog
  sourceNamespace: olm
  installPlanApproval: Automatic
```

创建后检查安装计划状态：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 InstallPlan 是否已完成
kubectl get installplan -n database-operators -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}'
```
### 5.2 升级通道选择

| 通道类型 | 适用场景 | 风险 |
|---------|---------|------|
| `stable` | 生产环境 | 低，发布节奏保守 |
| `fast` | 预发/测试 | 中，可快速获得新特性 |
| `candidate` | 灰度验证 | 高，可能包含破坏性变更 |

生产环境建议将 `installPlanApproval` 设置为 `Manual`，由 SRE 审批后再执行升级。自动升级虽然方便，但可能在业务高峰期引入不兼容的 CRD 变更或 webhook 行为变化，导致现有业务异常。Manual 模式配合变更管理流程，可以在低峰期窗口执行升级，并预留回滚时间。

```yaml
spec:
  installPlanApproval: Manual
```

## 6. Operator 升级与回滚

### 6.1 审批手动升级

当 `installPlanApproval: Manual` 时，OLM 会生成 `ApprovalRequired` 状态的 InstallPlan：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看待审批的 InstallPlan
kubectl get installplan -n database-operators

# 批准指定 InstallPlan 执行升级
kubectl patch installplan <plan-name> -n database-operators --type merge \
  -p '{"spec":{"approved":true}}'
```
### 6.2 回滚 Operator

OLM 不直接支持一键回滚 CSV，但可通过重新安装旧版本实现回滚。步骤如下：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 删除当前 Subscription 与 CSV（保留 CRD，避免数据丢失）
kubectl delete subscription cloudnative-pg -n database-operators
kubectl delete csv cloudnative-pg.v1.22.0 -n database-operators

# 2. 使用旧版本重新创建 Subscription
kubectl apply -f - <<EOF
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: cloudnative-pg
  namespace: database-operators
spec:
  channel: stable
  name: cloudnative-pg
  source: operatorhubio-catalog
  sourceNamespace: olm
  startingCSV: cloudnative-pg.v1.21.1
  installPlanApproval: Manual
EOF
```
> 注意：回滚前需确认旧版本 CSV 支持的 CRD 版本与当前 CR 兼容，否则可能导致 CR 被丢弃。建议先在测试命名空间验证旧版本 CSV 能否正常协调现有 CR。

## 7. Day-2 运维

### 7.1 监控 Operator 自身健康

Operator 自身也是 Pod，应纳入集群监控体系：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Operator Pod 资源使用与重启情况
kubectl top pod -n database-operators -l app=cloudnative-pg
kubectl get pods -n database-operators -l app=cloudnative-pg
```
### 7.2 CRD 版本管理

Operator 升级常伴随 CRD 版本升级。升级前需检查 CRD 的 `storedVersions`：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 CRD 当前存储版本与可服务版本
kubectl get crd postgresqls.postgresql.cnpg.io -o jsonpath='{.status.storedVersions}{"\n"}{.status.conditions}'

```
若 `storedVersions` 包含旧版本，应在确认所有 CR 已迁移到新版本后，使用 `kubectl edit crd` 移除旧版本。否则 CRD 中会一直保留旧版本存储格式，影响升级与新特性使用。

### 7.3 备份 Subscription 与 CSV

在重大变更前备份 OLM 资源，便于快速恢复：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出 Subscription 与 CSV 配置
kubectl get subscription -n database-operators -o yaml > backup-subscriptions.yaml
kubectl get csv -n database-operators -o yaml > backup-csvs.yaml
```
建议将备份文件纳入 Git 版本控制，并通过 ACK 配置审计功能追踪变更历史。对于关键 Operator，还应定期演练从备份中恢复 Subscription 与 CSV 的流程，确保在控制平面异常时能够快速恢复。

### 7.4 Webhook 与证书管理

部分 Operator 会注册 admission webhook，OLM 通过 `Operator Lifecycle Manager` 自动注入证书。若 webhook Pod 无法启动，可能导致集群 API 请求被阻塞。排查方法：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 ValidatingWebhookConfiguration 与 MutatingWebhookConfiguration
kubectl get validatingwebhookconfiguration,mutatingwebhookconfiguration
kubectl describe deployment <operator-name> -n <namespace>
```
## 8. 多租户与权限隔离

在多团队共享 ACK 集群时，应通过 `OperatorGroup` 限制每个 Operator 的作用域：

```yaml
apiVersion: operators.coreos.com/v1
kind: OperatorGroup
metadata:
  name: team-a-operators
  namespace: team-a
spec:
  targetNamespaces:
    - team-a
```

避免使用 `AllNamespaces` 安装模式，除非该 Operator 必须管理全集群资源（如网络插件、存储 CSI）。`AllNamespaces` 模式会创建 ClusterRole 并监听所有命名空间，存在较高的权限扩散风险。

## 9. OLM 与 Helm 的选型对比

| 场景 | 推荐工具 | 原因 |
|------|---------|------|
| 需要自动依赖解析与版本约束 | OLM | 内置依赖管理、CSV 版本锁定 |
| 简单应用部署 | Helm | chart 生态成熟，学习成本低 |
| 多租户 Operator 管理 | OLM | OperatorGroup 提供命名空间隔离 |
| 离线环境 | 均可 | OLM 需要自建 catalog，Helm 需要自建 chart 仓库 |
| 需要细粒度升级审批 | OLM | Manual InstallPlan 支持审批流 |

在 ACK 平台工程中，通常将 Helm 用于无状态应用，OLM 用于数据库、中间件、可观测性等复杂 Operator 的生命周期管理。

## 10. 常见故障排查

### 10.1 InstallPlan 卡住

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 InstallPlan 事件与失败原因
kubectl describe installplan <plan-name> -n database-operators
```
常见原因：

- CatalogSource 不可达或镜像拉取失败。
- 依赖的 CRD 版本冲突。
- RBAC 权限不足，无法创建 ClusterRole。

### 10.2 CSV 状态失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CSV 详细状态
kubectl describe csv cloudnative-pg.v1.22.0 -n database-operators
```
重点关注 `Status.Phase` 与 `Status.Message`，常见问题包括：

- `DeploymentNotReady`：Operator Pod 未就绪，检查镜像与资源限制。
- `UnsupportedOperatorGroup`：OperatorGroup 配置与安装模式冲突。
- `CRDConflict`：已存在同名 CRD 且版本不兼容。

### 10.3 Operator 无法管理跨命名空间 CR

当 Operator 采用 `OwnNamespace` 或 `SingleNamespace` 模式时，只能在指定命名空间内响应 CR。若 CR 创建在其他命名空间，Operator 不会协调。此时应检查 OperatorGroup 的 `targetNamespaces` 与 CR 所在命名空间是否一致。

### 10.4 CatalogSource 镜像过期

若 CatalogSource 长时间未更新，可能导致新版本的 CSV 无法被发现。建议：

1. 定期同步 OperatorHub 最新 bundle。
2. 在 CI/CD 中自动构建并推送 catalog 镜像。
3. 为 CatalogSource 配置镜像拉取策略为 `Always`，确保每次启动都拉取最新索引。

## 11. 生产检查清单

- [ ] 已根据环境选择 `Automatic` 或 `Manual` 升级策略。
- [ ] 已创建 `OperatorGroup` 并明确限制 Operator 作用域。
- [ ] CatalogSource 镜像已在专有云内网可用，避免拉取失败。
- [ ] 升级前已备份 Subscription、CSV 与关键 CR。
- [ ] 已检查 CRD `storedVersions`，避免版本碎片化。
- [ ] Operator Pod 已配置资源 limit 与 HPA/VPA。
- [ ] 已设置 Operator 自身健康的监控告警。
- [ ] 已制定回滚 SOP 并在测试环境演练。
- [ ] Webhook 与证书配置已验证，避免 API 阻塞。
- [ ] 已建立 catalog 镜像定期更新机制。

## 12. Operator 升级风险与回滚演练

Operator 升级虽然由 OLM 自动管理，但仍存在 CRD schema 变更、依赖版本冲突与权限扩大等风险。生产环境应在升级前完成风险评估与回滚演练。

### 升级前评审清单

- [ ] 阅读目标 CSV 的 Release Note，确认是否有破坏性变更。
- [ ] 在隔离命名空间或测试集群验证升级路径。
- [ ] 备份当前 CRD、自定义资源与 CSV YAML。
- [ ] 确认依赖项（cert-manager、prometheus-operator 等）版本兼容。
- [ ] 评估新版本的 RBAC 范围是否扩大。

### 回滚演练

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 备份当前 CSV 与 Subscription
kubectl get csv <csv-name> -n <namespace> -o yaml > /backup/csv.yaml
kubectl get subscription <sub-name> -n <namespace> -o yaml > /backup/sub.yaml

# 模拟升级失败后的回滚
kubectl delete csv <new-csv> -n <namespace>
kubectl delete subscription <sub-name> -n <namespace>
kubectl apply -f /backup/sub.yaml
# 将 sub.yaml 中的 startingCSV 指向旧版本
```
> 回滚演练应在非生产环境定期执行，确保升级窗口内有明确的回退方案。

### 权限扩大监控

使用 OPA 或 Kyverno 监控 Operator 部署的 RBAC 变化，防止新版本申请过度权限：

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-operator-rbac
spec:
  validationFailureAction: Audit
  rules:
  - name: check-cluster-admin
    match:
      resources:
        kinds:
        - ClusterRoleBinding
    validate:
      message: "Operator ClusterRoleBinding 不得绑定 cluster-admin"
      pattern:
        roleRef:
          name: "!cluster-admin"
```

## 13. 相关文档

- [[数据库中间件/Operator管理/01-database-operator-patterns.md|数据库 Operator 设计模式]]
- [[数据库中间件/Operator管理/02-operator-comparison-mysql-postgres-redis.md|MySQL/PostgreSQL/Redis Operator 对比]]

```

<!-- risk-assessed -->
