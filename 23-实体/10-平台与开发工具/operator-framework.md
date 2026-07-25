---
title: Operator Framework [entities]
description: '## 概述'
summary: 'Operator Framework 是一个开源工具包，用于以高效、自动化和可扩展的方式管理 Kubernetes 原生应用（Operators）。它提供了构建、测试和分发 Operators 的完整解决方案。'
category: entities
tags:
- k8s
- cncf
- orchestration
- operator-framework
- prometheus
- grafana
- helm
- rbac
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Operator Framework 是什么
- 如何 Operator Framework
trigger_keywords:
- Operator
- Framework
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Operator Framework

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

Operator Framework 是由 Red Hat 开源的工具包，用于构建、测试和分发 Kubernetes 原生应用（Operators），2020 年加入 CNCF Incubating。它提供了完整的 Operator 生命周期管理解决方案，包括 Operator SDK（开发框架）、Operator Lifecycle Manager（OLM，运行时管理）和 OperatorHub.io（发现和分发平台）。Operator Framework 是 Kubernetes 生态中 Operator 模式事实上的标准工具链。

## 核心特性

- **Operator SDK**: 支持 Go、Ansible、Helm 三种方式构建 Operators
- **OLM (Operator Lifecycle Manager)**: 安装、升级、依赖管理和 RBAC 自动化
- **OperatorHub.io**: 类似 App Store 的 Operator 发现和安装平台
- **成熟度模型**: 5 级 Operator 能力模型（Basic Install → Auto Pilot）
- **内置测试**: scorecard 工具评估 Operator 质量
- **Catalog 管理**: 自定义 Operator Catalog 适配企业内部环境

## 架构

Operator Framework 由三个核心组件组成。Operator SDK 是 CLI 工具，提供项目脚手架、API 代码生成、测试框架和打包功能。OLM 在集群中以 Deployment 运行（olm-operator 和 catalog-operator），监听 Subscription 和 ClusterServiceVersion（CSV）CRD，管理 Operator 的安装、升级、依赖解析和 RBAC。OperatorHub.io 是外部 Web 平台，收录社区提交的 Operators。OLM Catalog 以 OCI 镜像或 CatalogSource 形式分发 Operator 列表和元数据。

## Kubernetes 集成

Operator Framework 完全基于 Kubernetes CRD 和 Controller 模式。OLM 通过 ClusterServiceVersion（CSV）描述 Operator 的元数据、安装模式和依赖关系。Subscription CRD 定义用户对某个 Operator 的订阅（频道、更新策略）。InstallPlan 由 OLM 自动生成，列出安装所需的所有资源。OLM 自动管理 RBAC（创建 Role/RoleBinding），确保 Operator 仅获得必要权限。Operator 通过 OLM 安装后，其 CRD 和 Deployment 自动创建。

## 生产使用场景

1. **数据库管理**: 安装 PostgreSQL/Redis Operator，自动化数据库运维
2. **中间件部署**: 通过 OLM 一键安装 Kafka/Elasticsearch Operator
3. **企业内部 Operator**: 构建内部 Operator Catalog，分发公司专有 Operators
4. **自动升级**: 订阅 Operator 更新频道，自动获取安全补丁和新版本

## 安装与配置

```bash
# 安装 Operator SDK
curl -LO https://github.com/operator-framework/operator-sdk/releases/download/v1.33.0/operator-sdk_linux_amd64
chmod +x operator-sdk_linux_amd64 && sudo mv operator-sdk_linux_amd64 /usr/local/bin/operator-sdk

# 安装 OLM
operator-sdk olm install

# 验证 OLM 状态
kubectl get pods -n olm
kubectl get catalogsource -n olm

# 安装 Operator (通过 OLM)
kubectl operator install postgresql \
  --channel stable-v1 \
  --version 1.2.0 \
  --namespace operators

# 使用 SDK 创建新 Operator
operator-sdk init --domain example.com --repo github.com/myorg/my-operator
cd my-operator
operator-sdk create api --group app --version v1 --kind MyApp --resource --controller
make manifests generate
make install
make run
```

```yaml
# Subscription 示例 - 订阅 Operator 更新
apiVersion: operators.coreos.com/v1alpha1
kind: Subscription
metadata:
  name: postgresql-operator
  namespace: operators
spec:
  channel: stable-v1
  name: postgresql
  source: operatorhubio-catalog
  sourceNamespace: olm
  installPlanApproval: Automatic  # 或 Manual
---
# ClusterServiceVersion (CSV) 示例
apiVersion: operators.coreos.com/v1alpha1
kind: ClusterServiceVersion
metadata:
  name: postgresql-operator.v1.2.0
  namespace: operators
spec:
  displayName: PostgreSQL Operator
  version: 1.2.0
  maturity: stable
  install:
    strategy: deployment
    spec:
      deployments:
        - name: postgresql-operator
          spec:
            replicas: 1
            selector:
              matchLabels:
                app: postgresql-operator
  customresourcedefinitions:
    owned:
      - name: postgresqls.database.example.com
        version: v1
        kind: PostgreSQL
```

## 运维操作

```bash
# 🟢 检查 OLM 状态
kubectl get pods -n olm
kubectl get catalogsource -n olm
kubectl get subscriptions -A
kubectl get clusterserviceversions -A

# 🟢 检查已安装的 Operator
kubectl operator list
kubectl get csv -n operators

# 🟢 检查 Operator 资源
kubectl get postgresql -A  # 示例 CRD
kubectl describe csv postgresql-operator.v1.2.0 -n operators

# 🟢 检查 InstallPlan
kubectl get installplans -n operators
kubectl describe installplan <name> -n operators

# 🟡 升级 Operator
kubectl operator upgrade postgresql --channel stable-v2 -n operators

# 🟡 卸载 Operator
kubectl operator uninstall postgresql -n operators

# 🟢 运行 scorecard 测试
operator-sdk scorecard ./bundle --output json

# 🟢 构建 Operator Bundle
make bundle
make bundle-build BUNDLE_IMG=registry.example.com/my-operator-bundle:v1.0.0
```

## Operator 成熟度模型

| 级别 | 名称 | 能力 | 说明 |
|------|------|------|------|
| Level 1 | Basic Install | 自动化安装 | 通过 OLM 安装 CRD 和 Deployment |
| Level 2 | Seamless Upgrades | 无缝升级 | 支持 Operator 和 CR 版本升级 |
| Level 3 | Full Lifecycle | 完整生命周期 | 备份、恢复、扩缩容 |
| Level 4 | Deep Insights | 深度洞察 | 指标、日志、告警集成 |
| Level 5 | Auto Pilot | 自动驾驶 | 自动调优、异常自愈、智能扩缩 |

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Operator 安装失败 | CSV 未就绪 | `kubectl get csv -n operators` | 检查 CSV 状态/事件 |
| Subscription 未解析 | CatalogSource 不可用 | `kubectl get catalogsource -n olm` | 检查 Catalog Pod |
| InstallPlan 失败 | RBAC 权限不足 | `kubectl describe installplan` | 检查权限配置 |
| CR 未调谐 | Operator Pod 崩溃 | `kubectl logs -n operators` | 检查 Operator 日志 |
| 升级卡住 | 依赖冲突 | 检查 CSV 依赖 | 解决依赖冲突 |
| scorecard 失败 | CRD 定义不完整 | `operator-sdk scorecard` | 修复 CRD/示例 |

### 排查流程

```
Operator 异常
├── 安装失败
│   ├── kubectl get csv → 检查 CSV 状态
│   ├── kubectl describe csv → 查看事件
│   ├── kubectl get installplan → 检查安装计划
│   └── kubectl logs -n olm → 检查 OLM 日志
├── CR 未调谐
│   ├── kubectl get pods -n operators → Operator Pod 状态
│   ├── kubectl logs <operator-pod> → 查看调谐日志
│   ├── kubectl describe cr <name> → 检查 CR 事件
│   └── 检查 RBAC 权限
└── 升级问题
    ├── 检查 Subscription 频道配置
    ├── 检查 CSV 依赖关系
    └── 检查 InstallPlan 审批状态
```

## 生产案例

### 案例 1: 数据库 Operator 自动化运维

- **场景**: 20+ PostgreSQL 实例需要手动备份、升级、故障恢复
- **排查**: 手动运维耗时且容易出错；故障恢复时间 >30 分钟
- **方案**: 安装 PostgreSQL Operator (Level 3)；自动化备份/恢复/升级；自定义 CR 定义实例规格
- **效果**: 运维时间减少 90%；故障恢复 <5 分钟；零数据丢失

### 案例 2: 企业内部 Operator Catalog

- **场景**: 公司开发了 5 个内部 Operator，分发和版本管理混乱
- **排查**: 各团队手动安装 Operator；版本不一致；升级困难
- **方案**: 构建内部 Operator Catalog (OCI 镜像)；OLM 统一管理和升级；Subscription 自动更新
- **效果**: Operator 分发标准化；版本一致性 100%；升级一键完成

## 对比与替代方案

| 维度 | Operator Framework | Kubebuilder | Metacontroller | Helm |
|------|-------------------|-------------|----------------|------|
| 开发框架 | ✅ SDK | ✅ 轻量 | ✅ Webhook | ❌ |
| 生命周期管理 | ✅ OLM | ❌ | ❌ | 部分 |
| 依赖管理 | ✅ | ❌ | ❌ | ✅ |
| 自动升级 | ✅ | ❌ | ❌ | ❌ |
| 发现平台 | ✅ OperatorHub | ❌ | ❌ | Artifact Hub |
| 复杂度 | 高 | 中 | 低 | 低 |
| 适用场景 | 企业级 Operator | 简单 Operator | 轻量 Controller | 应用打包 |

## 检查清单

- [ ] OLM 已安装且健康
- [ ] CatalogSource 可访问
- [ ] Operator CSV 状态 Succeeded
- [ ] CRD 已正确安装
- [ ] RBAC 权限配置正确
- [ ] Subscription 频道配置正确
- [ ] scorecard 测试通过
- [ ] Bundle 镜像已推送到 Registry

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]

## Related

- [[kubeclipper]] — KubeClipper
- [[runme-notebooks]] — Runme
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[helm]] — Helm
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference

<!-- risk-assessed -->
