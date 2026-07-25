---
title: Perses (entities)
description: '## 概述'
summary: 'Perses 是一个云原生的 Dashboard 即代码 (Dashboard-as-Code) 可视化平台，用于创建和管理可观测性仪表板。'
category: entities
tags:
- k8s
- cncf
- observability
- perses
- prometheus
- grafana
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Perses 是什么
- 如何 Perses
trigger_keywords:
- Perses
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Perses

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: Go, TypeScript

## 概述

Perses 是一个云原生的 Dashboard 即代码（Dashboard-as-Code）可视化平台，由 Perses 社区开发（核心贡献者来自 Grafana Labs），2023 年加入 CNCF 沙箱。它旨在成为 Grafana 的开源替代方案之一，提供标准化的 Dashboard 定义规范，支持将仪表板作为代码进行版本控制和 GitOps 管理。与 Grafana 使用数据库存储 Dashboard JSON 不同，Perses 原生使用 JSON/YAML 文件存储 Dashboard 定义，使得仪表板可以纳入 Git 版本控制，通过 ArgoCD/Flux 等 GitOps 工具管理。Perses 原生支持 Prometheus 和 Loki 数据源，并提供与 Grafana 兼容的 Panel 类型（折线图、柱状图、热力图等）。

## 核心能力

- **Dashboard 即代码**: Dashboard 定义为原生 JSON/YAML 文件，可纳入 Git 版本控制
- **CRD 集成**: PersesDashboard CRD 支持在 Kubernetes 中以 GitOps 方式管理仪表板
- **标准化规范**: 统一的 Dashboard 定义格式，支持变量、布局和面板复用
- **多数据源**: 原生支持 Prometheus、Loki、Tempo 等可观测性后端
- **RBAC**: 内置基于角色的访问控制
- **导入兼容**: 支持从 Grafana JSON 导入 Dashboard

## 架构

Perses 采用简洁的云原生架构：

- **Perses Server**: 后端服务，提供 RESTful API 和 Dashboard 渲染
- **Perses UI**: 基于 React 的前端界面，支持 Dashboard 可视化编辑和浏览
- **Dashboard Spec**: JSON/YAML 格式的仪表板定义，包含布局、面板和查询
- **Datasource Plugin**: 可扩展的数据源适配器（Prometheus、Loki 等）
- **CRD Controller**: 在 Kubernetes 中监听 PersesDashboard CRD，同步到 Perses Server
- **File Storage**: Dashboard 定义存储在文件系统或 Git 中（而非数据库）

GitOps 流程：`Dashboard YAML → Git → ArgoCD → PersesDashboard CRD → Perses Server → UI 渲染`

## K8s 集成

Perses 通过 Perses Operator 和 CRD 实现与 Kubernetes 的深度集成。`PersesDashboard` CRD 定义 Dashboard 资源，`PersesDatasource` CRD 定义数据源。Perses Operator 监听这些 CRD，自动将配置同步到 Perses Server。Dashboard 定义以 YAML 存储在 Git 中，通过 ArgoCD/Flux 自动同步到集群中的 PersesDashboard CRD。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 GitOps 工作流完全兼容，实现了 Dashboard 的声明式管理。

## 生产场景

1. **GitOps 仪表板管理**: 所有 Dashboard 纳入 Git 版本控制，通过 GitOps 工具自动部署
2. **多环境监控**: 在 dev/staging/prod 集群中统一部署标准化 Dashboard
3. **团队 Dashboard 共享**: 通过 Git 分享 Dashboard 定义，团队成员可复用和修改
4. **监控即代码**: 在 CI/CD 流水线中自动生成和部署监控仪表板

## 安装与配置

```bash
# Helm 安装 Perses
helm repo add perses https://perses.github.io/helm-charts
helm install perses perses/perses -n perses --create-namespace \
  --set config.database.type=file \
  --set config.database.file.folder=/data/perses

# 安装 Perses Operator
helm install perses-operator perses/perses-operator -n perses

# 等待就绪
kubectl wait --for=condition=available deployment/perses -n perses --timeout=120s

# 访问 Perses UI
kubectl port-forward svc/perses 8080:8080 -n perses
# 打开 http://localhost:8080
```

```yaml
# Prometheus 数据源 CRD
apiVersion: perses.dev/v1alpha1
kind: PersesDatasource
metadata:
  name: prometheus
  namespace: perses
spec:
  default: true
  plugin:
    kind: PrometheusDatasource
    spec:
      directUrl: http://prometheus-server.monitoring.svc:9090
---
# Dashboard CRD 示例
apiVersion: perses.dev/v1alpha1
kind: PersesDashboard
metadata:
  name: k8s-cluster-overview
  namespace: monitoring
spec:
  display:
    name: K8s Cluster Overview
  variables:
  - name: namespace
    type: ListVariable
    spec:
      plugin:
        kind: PrometheusLabelValuesVariable
        spec:
          labelName: namespace
  panels:
    cpu-usage:
      spec:
        display:
          name: CPU Usage by Namespace
        plugin:
          kind: TimeSeriesChart
          spec:
            queries:
            - spec:
                plugin:
                  kind: PrometheusTimeSeriesQuery
                  spec:
                    query: sum(rate(container_cpu_usage_seconds_total[5m])) by (namespace)
  layouts:
  - kind: Grid
    spec:
      items:
      - x: 0
        y: 0
        width: 12
        height: 8
        content: $ref: '#/spec/panels/cpu-usage'
```

## 运维操作

```bash
# 🟢 查看 Dashboard 列表
kubectl get persesdashboard -A

# 🟢 查看数据源配置
kubectl get persesdatasource -A

# 🟡 更新 Dashboard（通过 GitOps 或直接 apply）
kubectl apply -f dashboards/k8s-overview.yaml

# 🟢 查看 Perses Server 日志
kubectl logs -n perses -l app=perses --tail=50

# 🟡 重启 Perses Server
kubectl rollout restart deployment/perses -n perses

# 🟢 导出 Dashboard 为 JSON
curl -s http://perses:8080/api/v1/projects/default/dashboards/k8s-overview | jq .

# 🔴 删除 Dashboard
kubectl delete persesdashboard k8s-cluster-overview -n monitoring
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Dashboard 不显示 | CRD 未同步或 Perses Server 异常 | `kubectl get persesdashboard` | 检查 Operator 日志 |
| 数据源连接失败 | Prometheus 不可达 | `kubectl logs -n perses` | 检查数据源 URL 和网络 |
| 图表无数据 | PromQL 查询错误或无匹配指标 | 在 Perses UI 中测试查询 | 修正 PromQL 查询 |
| Operator 未同步 | CRD 版本不匹配或 RBAC 问题 | `kubectl logs -n perses -l app=perses-operator` | 检查 CRD 版本和 RBAC |
| GitOps 同步失败 | ArgoCD 未配置或 CRD 未注册 | `kubectl get crd persesdashboards.perses.dev` | 确认 CRD 已安装 |

```
排查流程：
├── Dashboard 不显示
│   ├── kubectl get persesdashboard 确认 CRD 存在
│   ├── 检查 Perses Operator 日志
│   ├── 确认 Perses Server 正常运行
│   └── 检查 Dashboard YAML 格式
├── 数据源问题
│   ├── 检查 PersesDatasource CRD 配置
│   ├── 确认 Prometheus/Loki 服务可达
│   ├── 在 Perses UI 中测试数据源连接
│   └── 检查 RBAC 和 ServiceAccount
└── GitOps 同步问题
    ├── 确认 ArgoCD/Flux 已配置
    ├── 检查 CRD 是否已注册
    └── 查看 GitOps 工具同步日志
```

## 生产案例

### 案例 1：GitOps 监控仪表板管理

- **场景**：50+ 微服务的监控 Dashboard 分散在 Grafana 中，无版本控制，变更无审计
- **排查**：Dashboard 变更无记录，误删后无法恢复，多环境 Dashboard 不一致
- **方案**：迁移到 Perses，所有 Dashboard 以 YAML 存储在 Git，通过 ArgoCD 自动同步
- **效果**：Dashboard 变更可追溯，多环境一致性 100%，误删可从 Git 恢复

### 案例 2：平台工程标准化监控

- **场景**：平台团队为 20+ 业务团队提供标准化监控模板，但 Grafana 模板维护困难
- **排查**：各团队自行修改 Dashboard，标准化模板被破坏，无法统一升级
- **方案**：Perses CRD 定义标准化模板，业务团队只能覆盖变量不能修改结构
- **效果**：标准化模板统一维护，业务团队自助修改变量，模板升级一键同步

## 对比

| 特性 | Perses | Grafana | Kibana | Apache Superset | 适用场景 |
|------|--------|---------|--------|-----------------|----------|
| Dashboard 即代码 | ✅ 原生 | ⚠️ 需插件 | ❌ | ⚠️ 有限 | GitOps 管理 |
| GitOps | ✅ CRD | ⚠️ 需插件 | ❌ | ❌ | K8s 原生工作流 |
| K8s 原生 | ✅ | ⚠️ | ❌ | ❌ | 云原生环境 |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | 非 CNCF | 开源生态 |
| 生产成熟度 | 中（新项目） | 高 | 高 | 高 | 稳定性要求 |

## 架构定位

在 CNCF 生态中，Perses 属于 **Observability** 类别，为云原生应用提供 Dashboard 即代码的可视化能力。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/09-平台与发布/gitops-principles.md|gitops-principles]]
- [[pod-lifecycle]]

## Related

- [[kaito]] — KAITO
- [[youki]] — youki
- [[easegress]] — Easegress
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- perses
- [[23-实体/15-参考与索引/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
