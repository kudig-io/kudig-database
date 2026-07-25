---
title: Crossplane 平台工程实践指南
description: '# Crossplane 平台工程实践指南'
summary: 'helm repo add crossplane-stable https://charts.crossplane.io/stable'
category: infrastructure-as-code
tags:
- k8s
- iac
- terraform
- pulumi
- helm
- opa
- postgresql
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
- DevOps 工程师
estimated_read_time: 5min
intent_queries:
- Crossplane 平台工程实践指南 是什么
- 如何 Crossplane 平台工程实践指南
- Kubernetes 24 infrastructure as code 最佳实践
trigger_keywords:
- Crossplane
- 平台工程实践指南
- infrastructure
- as
- code
prerequisites:
- kubectl-basics
- gitops-basics
- helm-basics
- iac-basics
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Crossplane|Crossplane]] 平台工程实践指南

> **适用版本**: Crossplane v1.19.0  
> **最后更新**: 2026-04-24  
> **难度**: 高级

---

## 📋 目录

- [一、核心概念](#一核心概念)
- [二、安装部署](#二安装部署)
- [三、Provider 配置](#三provider-配置)
- [四、Composition 构建平台 API](#四composition-构建平台-api)
- [五、GitOps 集成](#五gitops-集成)
- [六、多租户与治理](#六多租户与治理)
- [七、与 Terraform 对比](#七与-terraform-对比)

---

## 一、核心概念

```
# 🟢 低风险：只读/信息收集，通常无副作用
开发者视角 (Claim)
  ├── DatabaseClaim ──► 平台工程师预定义
  │
  平台工程师视角
    ├── CompositeResourceDefinition (XRD): 模式定义
    ├── Composition: 实现映射 (AWS RDS / GCP CloudSQL / Azure PG)
    └── Provider: 云厂商插件
        └── Managed Resource (MR): 底层云资源
```
---

## 二、安装部署

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add crossplane-stable https://charts.crossplane.io/stable
helm install crossplane crossplane-stable/crossplane \
  --namespace crossplane-system \
  --create-namespace \
  --version 1.19.0

# 安装 CLI
 curl -sL https://raw.githubusercontent.com/crossplane/crossplane/master/install.sh | sh
 sudo mv crossplane /usr/local/bin/
```
---

## 三、Provider 配置

```yaml
# AWS Provider
apiVersion: pkg.crossplane.io/v1
kind: Provider
metadata:
  name: provider-aws-rds
spec:
  package: xpkg.upbound.io/upbound/provider-aws-rds:v1.21.0
---
# ProviderConfig (凭证)
apiVersion: aws.upbound.io/v1beta1
kind: ProviderConfig
metadata:
  name: default
spec:
  credentials:
    source: Secret
    secretRef:
      namespace: crossplane-system
      name: aws-creds
      key: creds
```

---

## 四、Composition 构建平台 API

```yaml
# XRD: 平台工程师定义 API
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xpostgresqls.database.example.org
spec:
  group: database.example.org
  names:
    kind: XPostgreSQL
    plural: xpostgresqls
  claimNames:
    kind: PostgreSQL
    plural: postgresqls
  versions:
  - name: v1alpha1
    served: true
    referenceable: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              parameters:
                type: object
                properties:
                  region:
                    type: string
                    enum: ["us-east-1", "eu-west-1"]
                  storageGB:
                    type: integer
                    default: 20
                  version:
                    type: string
                    enum: ["14", "15", "16"]
                    default: "16"
---
# Composition: AWS RDS 实现
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: aws-rds-postgresql
  labels:
    provider: aws
    db: postgresql
spec:
  compositeTypeRef:
    apiVersion: database.example.org/v1alpha1
    kind: XPostgreSQL
  resources:
  - name: rds-instance
    base:
      apiVersion: rds.aws.upbound.io/v1beta1
      kind: Instance
      spec:
        forProvider:
          engine: postgres
          instanceClass: db.t3.micro
          allocatedStorage: 20
          region: us-east-1
    patches:
    - fromFieldPath: spec.parameters.region
      toFieldPath: spec.forProvider.region
    - fromFieldPath: spec.parameters.storageGB
      toFieldPath: spec.forProvider.allocatedStorage
    - fromFieldPath: spec.parameters.version
      toFieldPath: spec.forProvider.engineVersion
```

---

## 五、GitOps 集成

```yaml
# 开发者提交 Claim
apiVersion: database.example.org/v1alpha1
kind: PostgreSQL
metadata:
  name: myapp-db
  namespace: dev-team
spec:
  parameters:
    region: us-east-1
    storageGB: 50
    version: "16"
```

**与 [[Argo|Argo]] CD 集成**
- Crossplane 资源为原生 K8s YAML
- 直接通过 Argo CD 管理
- 利用 Argo CD 的 drift detection 检测云资源漂移

---

## 六、多租户与治理

```yaml
# 配额与治理 (通过 XRD validation 或 OPA Gatekeeper)
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: require-cost-center
spec:
  match:
    kinds:
    - apiGroups: ["database.example.org"]
      kinds: ["PostgreSQL"]
  parameters:
    labels:
    - key: cost-center
```

---

## 七、与 Terraform 对比

| 维度 | Crossplane | Terraform |
|:---|:---|:---|
| 控制平面 | K8s 原生 | 外部 CLI |
| GitOps | 原生支持 | 需 wrapper |
| 漂移检测 | 自动调和 | 需手动 plan |
| 组合抽象 | XRD + Composition | Module |
| 多租户 | 命名空间隔离 | 需额外管理 |
| 云资源状态 | K8s CR 实时反映 | state 文件 |
| 团队分工 | 平台/开发解耦 | 通常集中管理 |

---

## 参考链接

- [Crossplane 官方文档](https://docs.crossplane.io/)
- [Upbound Marketplace](https://marketplace.upbound.io/)
- [Crossplane 架构](https://docs.crossplane.io/latest/concepts/)

---

## Obsidian 相关文档

- domain-24-infrastructure-as-code MOC
- [[11-发布变更/README.md|Domain 08: 基础设施即代码 (Infrastructure as Code)]]
- Domain-24 基础设施即代码 — 开源项目索引
- Terraform企业级基础设施即代码实践
- Ansible企业级自动化运维深度实践
- Pulumi Enterprise Infrastructure as Code Platform
- Azure Resource Manager (ARM) Enterprise 深度实践
- Crossplane Enterprise Infrastructure Orchestration 深度实践

## 故障排查表

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| Composite Resource 一直 Pending | Composition 未匹配或 XR spec 字段错误 | `kubectl describe xr <name>` | 检查 Composition claimNames 与 XR apiVersion 匹配 |
| Managed Resource 创建失败 | Provider 凭证过期或权限不足 | `kubectl get events -n crossplane-system` | 更新 ProviderConfig Secret，检查云 API 权限 |
| Provider Pod CrashLoopBackOff | 资源不足或版本不兼容 | `kubectl logs -n crossplane-system -l pkg.crossplane.io/provider` | 增加 memory limit，降级 Provider 版本 |
| Composition 更新后 XR 未同步 | Revision 未激活或 watch 断开 | `kubectl get compositionrevision` | 删除旧 Revision，重启 crossplane Pod |
| 资源漂移未自动修复 | drift-detection 间隔过长或 disabled | `kubectl get providerconfig -o yaml` | 设置 `spec.pollInterval: 10m` |
| 删除 XR 后云资源残留 | finalizer 卡住或云 API 超时 | `kubectl get xr -o jsonpath='{.items[*].metadata.finalizers}'` | 手动移除 finalizer 后在云端清理 |

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| Provider 版本 | 锁定版本 + 定期升级 | 避免 `latest` tag，使用 `spec.package: xpkg.upbound.io/...:v0.42.0` |
| Composition 设计 | 一个 Composition 对应一个平台能力 | 避免过度抽象，保持 1:1 映射 |
| 资源配额 | 为 crossplane-system 设置 PriorityClass | 确保控制面资源不被驱逐 |
| 多集群 | Hub-Spoke 模式，Hub 运行 Crossplane | Spoke 集群通过 GitOps 同步 XR |
| 安全 | ProviderConfig 使用 IRSA/Workload Identity | 避免静态 AK/SK |
| 可观测性 | 启用 crossplane_metrics_port | 接入 Prometheus 监控 reconcile 延迟 |
| 灾难恢复 | 定期导出 XR + Composition 到 Git | etcd 备份 + Git 双保险 |
| 升级策略 | 先升级 Provider CRD，再升级 Provider Pod | 避免 CRD 不兼容导致 reconcile 失败 |

## 关键指标与告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: crossplane-alerts
  namespace: crossplane-system
spec:
  groups:
  - name: crossplane.rules
    rules:
    - alert: CrossplaneReconcileErrors
      expr: rate(crossplane_reconcile_errors_total[5m]) > 0.1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Crossplane reconcile 错误率过高"
    - alert: CrossplaneManagedResourceNotReady
      expr: crossplane_managed_resource_ready == 0
      for: 30m
      labels:
        severity: critical
      annotations:
        summary: "Managed Resource {{ $labels.name }} 30min 未就绪"
    - alert: CrossplaneProviderHealthFalse
      expr: crossplane_provider_healthy == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Provider {{ $labels.name }} 不健康"
```

## 相关工具

| 工具 | 用途 | 地址 |
|------|------|------|
| crossplane CLI | 本地 XR 渲染与验证 | github.com/crossplane/crossplane-cli |
| upjet | 从 Terraform Provider 生成 Crossplane Provider | github.com/crossplane/upjet |
| provider-family-aws | AWS 全量 Provider 家族包 | marketplace.upbound.io |
| crossplane-contrib/provider-kubernetes | 管理集群内 K8s 资源 | github.com/crossplane-contrib |
| function-patch-and-transform | Composition 函数式转换 | github.com/crossplane-contrib |

## See Also

- 04-azure-resource-manager-enterprise
- 05-crossplane-enterprise-orchestration
- 01-terraform-enterprise-iac
- 02-ansible-enterprise-automation

```

<!-- risk-assessed -->
