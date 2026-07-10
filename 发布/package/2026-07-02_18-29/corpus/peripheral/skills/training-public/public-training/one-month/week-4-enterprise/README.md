---
title: 'Week 4: 企业级进阶期 (Days 22-28)'
description: '- Kubernetes 企业级运维'
summary: '- Kubernetes 企业级运维'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- grafana
- helm
- argocd
- opa
- gateway
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- 'Week 4: 企业级进阶期 (Days 22-28) 是什么'
- '如何 Week 4: 企业级进阶期 (Days 22-28)'
trigger_keywords:
- Week
- '4:'
- 企业级进阶期
- Days
- 22-28
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- gpu-scheduling-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Week 4: 企业级进阶期 (Days 22-28)
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[Kubernetes|Kubernetes]] 企业级运维
  - GitOps 持续部署
  - 生产事故响应
  - SRE 能力建设
trigger_keywords:
  - Week 4
  - 企业级
  - GitOps
  - [[ArgoCD|ArgoCD]]
  - 变更管理
  - 事故响应
  - 容量规划
  - SLO
  - 学习路径
reading_level: advanced
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 30min
related_domains:
  - domain-20-enterprise-monitoring-alerting
  - domain-08-release-change-management
  - domain-05-security-compliance
  - domain-11-production-operations
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/README
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p4-gitops-pipeline
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p5-graduation-project
---

# Week 4: 企业级进阶期 (Days 22-28)

## 概述

第四周是整个一个月学习计划的收官阶段，聚焦于企业级场景下的 Kubernetes 运维能力建设。经过前三周的学习，你已经掌握了 K8s 的架构基础、核心技术（工作负载、网络、存储）以及运维安全体系。本周将在此基础上，深入企业级监控告警平台、GitOps 持续部署、云原生安全合规以及生产事故响应等高级主题。

本周的学习目标是帮助你从"能操作"跃迁到"能设计"和"能决策"。在真实的生产环境中，运维工程师面临的挑战远不止于执行 kubectl 命令，而是需要具备系统化的监控体系设计能力、规范化的变更管理流程、以及结构化的问题响应方法论。

### 学习目标

- 掌握企业级监控告警平台（[[Prometheus|Prometheus]] + Thanos + Grafana）的架构设计与部署实践
- 理解 GitOps 理念并能够使用 ArgoCD 实现声明式持续部署
- 深入云原生安全体系，掌握策略引擎（Kyverno）、Secret 管理（Vault）与零信任架构
- 学会运用 FTA（故障树分析）和 FEBM（取证循证方法）解决复杂生产问题
- 建立变更管理、事故响应、容量规划等生产运维标准流程
- **产出**: GitOps 部署流水线 + 生产事故响应 Playbook

---

## 核心概念详解

### 企业级监控体系的演进

在 Week 3 中你学习了 Prometheus 的基础用法。本周将视野扩展到企业级场景。单集群的 Prometheus 可以满足小规模需求，但当企业拥有多个集群、数千个节点时，监控架构需要全面升级。

**Thanos** 是目前最流行的 Prometheus 高可用方案之一。它通过 Sidecar 组件将 Prometheus 的数据上传到对象存储（如 OSS），并通过 Querier 组件实现跨集群查询。这意味着你可以在一个面板上查看所有集群的指标数据。Thanos 的核心组件包括：

- **Sidecar**: 部署在每个 Prometheus 实例旁，负责数据上传和 StoreAPI 暴露
- **Querier**: 统一查询入口，可以同时查询多个 Prometheus 和对象存储中的历史数据
- **Store Gateway**: 从对象存储中读取历史数据
- **Compactor**: 对历史数据进行压缩和降采样，减少存储开销
- **Ruler**: 支持跨集群的告警规则评估

Thanos 部署模式对比：

| 组件 | 部署位置 | 资源需求 | 说明 |
|------|---------|---------|------|
| Sidecar | 每个集群 | 低（100m CPU, 256Mi 内存） | 与 Prometheus 共存 |
| Querier | 中心集群 | 中（500m CPU, 1Gi 内存） | 全局查询入口 |
| Store Gateway | 中心集群 | 中（500m CPU, 1Gi 内存） | 历史数据读取 |
| Compactor | 中心集群 | 中（500m CPU, 1Gi 内存） | 数据压缩降采样 |
| Ruler | 中心集群 | 中（500m CPU, 1Gi 内存） | 全局告警规则 |

**SLO/SLI 体系** 是现代 SRE 工程的基石。SLI（Service Level Indicator）是你用来衡量服务健康度的具体指标，比如可用性（成功请求比例）和延迟（P99 响应时间）。SLO（Service Level Objective）是你为 SLI 设定的目标值，比如"可用性达到 99.9%"。SLA（Service Level Agreement）则是对外承诺的服务等级协议，通常包含法律和财务条款。错误预算（Error Budget）的概念帮助你平衡可靠性和迭代速度——当错误预算耗尽时，团队应优先修复稳定性问题而非发布新功能。

SLO/SLI 示例表：

| SLI 指标 | SLO 目标 | 测量方法 | 错误预算（30天） |
|---------|---------|---------|----------------|
| 可用性 | 99.9% | 成功请求/总请求 | 43.2 分钟 |
| 延迟 P99 | < 500ms | 请求延迟直方图 | N/A |
| 延迟 P95 | < 200ms | 请求延迟直方图 | N/A |
| 错误率 | < 0.1% | 5xx 响应/总响应 | 43.2 分钟 |

### GitOps 与声明式部署

GitOps 的核心理念是"以 Git 仓库作为应用部署状态的唯一真实来源"。这意味着所有的配置变更都通过 Git 提交来完成，部署系统自动检测变更并同步到目标集群。**ArgoCD** 是目前最流行的 GitOps 工具，它通过持续比对 Git 仓库中的期望状态和集群中的实际状态来实现自动化部署。

ArgoCD 的关键概念包括：

- **Application**: 定义了源（Git 仓库中的路径）和目标（集群和命名空间）
- **Sync Policy**: 决定是自动同步还是手动触发
- **Project**: 用于多团队场景下的资源隔离
- **RBAC**: 精细化的权限控制

GitOps 工作流与传统 CI/CD 的区别在于：传统方式中 CI 系统直接将变更推送到集群，而 GitOps 中 CI 只负责构建镜像和更新 Git 仓库，部署由 ArgoCD 在集群内部完成。这种解耦带来了更好的安全性和可审计性。

GitOps 工作流对比：

| 维度 | 传统 CI/CD | GitOps |
|------|-----------|--------|
| 部署触发 | CI Pipeline 推送 | Git 变更自动检测 |
| 集群访问 | CI 需要 kubeconfig | 集群内部 ArgoCD 拉取 |
| 审计能力 | CI 日志 | Git 提交历史 |
| 回滚方式 | 重新运行 Pipeline | git revert |
| 多环境管理 | 多个 Pipeline | 多个 Application |
| 安全性 | CI 需要集群凭证 | 集群内部操作 |

### 云原生安全纵深防御

云原生安全不仅仅是传统的网络安全。它需要在容器镜像、运行时、编排平台和基础设施多个层面实施防护。

**Kyverno** 是一个 Kubernetes 原生的策略引擎。与 OPA Gatekeeper 不同，Kyverno 不需要学习新的策略语言（Rego），而是直接使用 Kubernetes 资源定义策略。它可以执行三种操作：Validate（验证资源是否符合规范）、Mutate（自动修改资源字段）、Generate（自动生成关联资源）。

Kyverno 策略示例：

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resource-limits
spec:
  validationFailureAction: Enforce
  rules:
  - name: validate-resources
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "CPU and memory resource limits are required"
      pattern:
        spec:
          containers:
          - resources:
              limits:
                cpu: "?*"
                memory: "?*"
```

**Vault** 是 HashiCorp 出品的 Secret 管理工具。它解决了 Kubernetes 原生 Secret 的几个痛点：Base64 编码而非加密、缺乏轮转机制、审计能力不足。Vault 通过 Kubernetes Auth Method 实现与集群的集成，Pod 可以通过 ServiceAccount 自动获取 Secret 而无需硬编码。

**零信任安全架构** 的核心理念是"永不信任，始终验证"。在 K8s 环境中，这意味着：Pod 之间不应默认互信，每个请求都需要经过身份验证和授权，网络策略应限制 Pod 间的通信，所有敏感数据在传输和存储时都应加密。

### 生产运维标准流程

生产环境的运维不仅需要技术能力，更需要规范化的流程。

**变更管理** 是生产稳定性的第一道防线。所有生产变更应经过评估、审批、执行、验证四个阶段。变更前应确认回滚方案，变更后应监控关键指标。推荐使用 GitOps 管理变更——每次变更都有 Git 记录，可以轻松回滚。

**事故响应流程** 定义了从发现问题到恢复服务的标准步骤。一个成熟的事故响应流程包括：检测（通过告警发现异常）、评估（判断影响范围和严重等级）、缓解（采取措施恢复服务）、根因分析（找到问题根因）、改进（防止同类问题再次发生）。FTA 和 FEBM 方法论将在 Day 26 深入讲解。

**容量规划** 帮助你提前发现资源瓶颈。通过分析历史指标趋势，预测未来的资源需求，并提前进行扩容或优化。关键指标包括 CPU/内存利用率、Pod 调度失败率、存储使用率等。

---

## 实战演练

### 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 22 | 企业监控: Prometheus 企业级 + Grafana | [day-22-enterprise-monitoring.md](./day-22-enterprise-monitoring.md) |
| Day 23 | 企业日志 + GitOps | [day-23-logging-gitops.md](./day-23-logging-gitops.md) |
| Day 24 | 云原生安全 + 合规 | [day-24-security-compliance.md](./day-24-security-compliance.md) |
| Day 25 | 生产运维最佳实践 | [day-25-production-best-practices.md](./day-25-production-best-practices.md) |
| Day 26 | FTA/FEBM 专题深化 | [day-26-fta-febm-deep.md](./day-26-fta-febm-deep.md) |
| Day 27 | 扩展生态 + 高级主题 | [day-27-extensions.md](./day-27-extensions.md) |
| Day 28 | 综合复习 + 毕业项目 | [day-28-final-project.md](./day-28-final-project.md) |

### 本周实操环境准备

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确保你的集群正常运行
kubectl get nodes
# 预期输出: 所有节点 Ready

# 创建本周使用的命名空间
kubectl create namespace enterprise-demo
kubectl create namespace monitoring
kubectl create namespace security-demo
kubectl create namespace gitops-demo

# 安装 Helm（如果尚未安装）
# macOS: brew install helm
# Linux: https://helm.sh/docs/intro/install/

# 验证 Helm 版本
helm version
# 预期输出: version.BuildInfo{Version:"v3.13.0",...}

# 添加常用的 Helm 仓库
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add argo https://argoproj.github.io/argo-helm
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo update
# 预期输出:
# Hang tight while we grab the latest from your chart repositories...
# ...Successfully got an update from the "prometheus-community" chart repository
# ...Successfully got an update from the "argo" chart repository
# ...Successfully got an update from the "bitnami" chart repository
# Update Complete. ⎈Happy Helming!⎈

# 安装 kube-prometheus-stack（如果尚未安装）
helm install monitoring prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --set grafana.adminPassword=admin123 \
  --set prometheus.prometheusSpec.retention=7d
# 预期输出:
# NAME: monitoring
# LAST DEPLOYED: ...
# NAMESPACE: monitoring
# STATUS: deployed
```
### 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的终极自测题。

### 本周实践项目

**项目 P4**: [GitOps 流水线](../projects/p4-gitops-pipeline.md) — 搭建从代码提交到自动部署的完整流水线
**项目 P5**: [毕业综合实践项目](../projects/p5-graduation-project.md) — 综合运用一个月所学完成生产级 K8s 平台搭建

---

## 配置示例

### Thanos 部署配置

```yaml
# Thanos Sidecar（与 Prometheus 共存）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: thanos-sidecar
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: thanos-sidecar
  template:
    metadata:
      labels:
        app: thanos-sidecar
    spec:
      containers:
      - name: thanos-sidecar
        image: thanosio/thanos:v0.32.0
        args:
        - sidecar
        - --tsdb.path=/prometheus
        - --objstore.config-file=/etc/thanos/objstore.yml
        - --http-address=0.0.0.0:10902
        - --grpc-address=0.0.0.0:10901
        - --prometheus.url=http://prometheus:9090
        ports:
        - name: http
          containerPort: 10902
        - name: grpc
          containerPort: 10901
        volumeMounts:
        - name: prometheus-data
          mountPath: /prometheus
        - name: thanos-config
          mountPath: /etc/thanos
      volumes:
      - name: prometheus-data
        persistentVolumeClaim:
          claimName: prometheus-pvc
      - name: thanos-config
        configMap:
          name: thanos-objstore-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: thanos-objstore-config
  namespace: monitoring
data:
  objstore.yml: |
    type: OSS
    config:
      bucket: "thanos-storage"
      endpoint: "oss-cn-hangzhou.aliyuncs.com"
      access_key_id: "<access-key>"
      secret_access_key: "<secret-key>"
```

### SLO 告警规则示例

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: slo-alerts
  namespace: monitoring
spec:
  groups:
  - name: slo-burn-rate
    rules:
    - alert: HighErrorBudgetBurnRate
      expr: |
        (
          1 - (sum(rate(http_requests_total{code=~"2.."}[1h]))
               / sum(rate(http_requests_total[1h])))
        ) > (1 - 0.999) * 14.4
      for: 5m
      labels:
        severity: critical
        slo: availability
      annotations:
        summary: "Error budget burn rate is too high"
        description: "The 1-hour error rate is 14.4x the monthly SLO threshold"
```

---

## 常见问题

### Q1: Thanos 和 Cortex 有什么区别？应该选哪个？

Thanos 和 Cortex 都解决 Prometheus 的高可用和长期存储问题，但设计思路不同。Thanos 采用"侧车"模式，与现有 Prometheus 实例配合使用，侵入性低，适合已经部署 Prometheus 的场景。Cortex 则是一个完全分布式的时序数据库，支持多租户，适合大规模 SaaS 平台。如果你的集群数量在 10 个以内，推荐 Thanos；如果需要多租户和极大规模，考虑 Cortex。

### Q2: GitOps 适合所有场景吗？

GitOps 最适合应用部署和配置管理场景，但对于一些需要即时响应的操作（如紧急扩容、手动故障恢复），传统方式可能更快。生产环境推荐采用"GitOps 为主，手动操作为辅"的混合模式，但手动操作后应及时将状态同步回 Git 仓库。

### Q3: Kyverno 和 OPA Gatekeeper 如何选择？

Kyverno 的优势在于使用 Kubernetes 原生资源定义策略，学习曲线低，与 kubectl 生态无缝集成。OPA Gatekeeper 使用 Rego 语言，功能更强大但学习成本更高。如果你刚开始接触策略引擎，推荐从 Kyverno 入手。如果团队已有 Rego 经验或有复杂的策略逻辑需求，OPA Gatekeeper 可能更适合。

### Q4: 本周学习时间不够怎么办？

本周内容的优先级排序如下：
1. **必做**: Day 22（企业监控）+ Day 25（生产最佳实践）+ Day 28（毕业项目）
2. **推荐**: Day 23（GitOps）+ Day 24（安全）
3. **扩展**: Day 26（FTA/FEBM 专题）+ Day 27（扩展生态）

### Q5: 如何在生产环境中实施 SLO？

从以下步骤开始：1）选择 1-2 个最关键的服务；2）定义 SLI（如可用性 = 成功请求/总请求）；3）设定 SLO（如 99.9%）；4）在 Prometheus 中配置 SLO 告警规则（burn rate 方式）；5）创建 Grafana Dashboard 展示错误预算消耗情况；6）将 SLO 纳入事故响应流程。

### Q6: Vault 与 K8s 原生 Secret 应该如何选择？

对于简单的场景（少量 Secret、不需要频繁轮转），K8s 原生 Secret 配合 RBAC 限制访问就足够了。当出现以下需求时考虑使用 Vault：Secret 需要频繁轮转（如数据库密码每 30 天更换）、需要审计 Secret 的访问记录、多个 K8s 集群或非 K8s 应用共享 Secret、需要动态 Secret（如临时数据库凭证）。

---

## 要点总结

本周是整个学习计划中综合性最强的一周，涵盖了以下核心能力：

| 能力域 | 关键技术 | 学习日 |
|--------|----------|--------|
| 企业级监控 | Thanos, SLO/SLI, Grafana | Day 22 |
| 持续部署 | GitOps, ArgoCD | Day 23 |
| 安全合规 | Kyverno, Vault, 零信任 | Day 24 |
| 生产运维 | 变更管理, 事故响应, 容量规划 | Day 25 |
| 故障分析 | FTA, FEBM | Day 26 |
| 扩展生态 | CRD, Helm, Operator | Day 27 |
| 综合检验 | 毕业项目 | Day 28 |

本周学习完成后，你应该能够独立设计和管理一个生产级的 Kubernetes 平台。

---

## 延伸阅读

- [Prometheus 企业级监控](../../domain-06-observability/01-prometheus-enterprise-monitoring.md)
- [ArgoCD 企业级 GitOps](../../domain-08-release-change-management/01-argo-cd-enterprise-gitops.md)
- [Kyverno 企业策略管理](../../domain-05-security-compliance/04-kyverno-enterprise-policy-management.md)
- [Vault 企业 Secret 管理](../../domain-05-security-compliance/05-vault-enterprise-secrets-management.md)
- [FTA 故障树分析](../../../domain-10-troubleshooting-diagnostics/FTA故障树/04-fta-core-principles.md)
- [FEBM 取证循证方法](../../../domain-10-troubleshooting-diagnostics/FEBM方法论/01-febm-theory-foundations.md)
- [SLO/SLI 体系](../../domain-06-observability/18-slo-sli-system.md)
- [CRD 开发指南](../../domain-15-specialized-tech/01-crd-development-guide.md)
- [Helm Charts 管理](../../domain-15-specialized-tech/06-helm-charts-management.md)
- [生产架构设计原则](../../domain-11-production-operations/01-production-architecture-design-principles.md)
- [零信任安全架构](../../domain-11-production-operations/07-zero-trust-security-architecture.md)
- [变更管理流程](../../domain-11-production-operations/22-change-management-process.md)
- [事故响应处理](../../domain-11-production-operations/23-incident-response-handling.md)
- [容量规划预测](../../domain-11-production-operations/24-capacity-planning-forecasting.md)

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
