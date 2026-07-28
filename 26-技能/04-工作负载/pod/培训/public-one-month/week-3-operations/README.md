---
title: 'Week 3: 运维作战能力期 (Days 15-21)'
description: 'title: Week 3: 运维作战能力期 (Days 15-21)'
summary: 'title: Week 3: 运维作战能力期 (Days 15-21)'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- prometheus
- grafana
- helm
- elasticsearch
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
- 'Week 3: 运维作战能力期 (Days 15-21) 是什么'
- '如何 Week 3: 运维作战能力期 (Days 15-21)'
trigger_keywords:
- Week
- '3:'
- 运维作战能力期
- Days
- 15-21
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- gpu-scheduling-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Week 3: 运维作战能力期 (Days 15-21)
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[kubernetes|kubernetes]] Week 3 学习路径
  - K8s 运维能力建设
  - 安全可观测性排障学习
  - 故障排查方法论入门
trigger_keywords:
  - Week 3
  - 安全体系
  - 可观测性
  - 故障排查
  - FTA
  - FEBM
  - 运维
  - 学习路径
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 30min
related_domains:
  - 安全
  - 可观测性
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/public-training/one-month/week-2-core-technologies/README
  - 生产运维/topic-learn/public-training/one-month/week-3-operations/day-15-security-1
  - 生产运维/topic-learn/public-training/one-month/week-4-enterprise/README
---

# Week 3: 运维作战能力期 (Days 15-21)

## 概述

第三周是从"学习知识"向"实战能力"转变的关键阶段。前两周你建立了 K8s 架构认知和核心技术基础，本周将聚焦于三大运维核心能力：**安全合规**、**可观测性**和**故障排查**。

在生产环境中，运维工程师的日常工作不仅仅是部署应用，更重要的是确保系统的安全、稳定和可观测。安全体系是底线——一次安全事件可能导致数据泄露和业务中断；可观测性是眼睛——没有监控和日志，你就像在黑暗中操作；故障排查能力是手术刀——当问题发生时，需要快速定位和解决。

### 学习目标

- 建立完整的 K8s 安全合规体系认知（认证、授权、Pod 安全标准、密钥管理）
- 构建覆盖 Metrics、Logs、Traces、Alerting 的完整可观测性体系
- 掌握基于 FTA/FEBM 方法的结构化故障排查方法论
- **产出**: 监控告警配置 + 故障排查手册

---

## 核心概念详解

### K8s 安全体系全景

Kubernetes 的安全模型建立在三个核心机制之上：**认证（Authentication）**、**授权（Authorization）** 和 **准入控制（Admission Control）**。

**认证** 回答"你是谁"的问题。K8s 支持多种认证方式：X.509 客户端证书、ServiceAccount Token、OIDC（OpenID Connect）、Webhook Token 认证等。在 ACK 集群中，常用的是客户端证书（kubeconfig 中内置的证书）和 ServiceAccount Token（Pod 中自动挂载的 Token）。从 Kubernetes 1.24 开始，ServiceAccount 不再自动创建 Secret，而是通过 TokenRequest API 动态生成短期 Token。

K8s 认证方式对比:

| 认证方式 | 安全等级 | 适用场景 | 配置复杂度 |
|----------|---------|---------|-----------|
| X.509 证书 | 高 | kubectl、系统组件 | 中 |
| ServiceAccount Token | 中 | Pod 内部调用 | 低 |
| OIDC | 高 | 企业 SSO 集成 | 高 |
| Webhook | 高 | 自定义认证 | 高 |
| Bootstrap Token | 低 | 节点加入集群 | 低 |

**授权** 回答"你能做什么"的问题。RBAC（Role-Based Access Control）是 K8s 最常用的授权模式。它通过四个核心资源实现精细化权限控制：

- **Role**: 命名空间级别的权限定义，规定了在某个命名空间内可以对哪些资源执行哪些操作
- **ClusterRole**: 集群级别的权限定义，可以授权跨命名空间的资源访问或集群级资源（如 Node、PV）的访问
- **RoleBinding**: 将 Role 绑定到用户、组或 ServiceAccount
- **ClusterRoleBinding**: 将 ClusterRole 绑定到主体

RBAC 最佳实践:

| 原则 | 说明 | 反面示例 |
|------|------|---------|
| 最小权限 | 只授予必要权限 | `verbs: ["*"]` |
| 命名空间隔离 | 用 RoleBinding 限制范围 | 滥用 ClusterRoleBinding |
| 专属 SA | 每个 Pod 用独立 ServiceAccount | 共享 default SA |
| 定期审计 | 清理过期权限 | 永不审查 RBAC |


**准入控制** 是请求通过认证和授权后的最后一道关卡。它可以在资源被持久化到 etcd 之前进行拦截和修改。常见的准入控制器包括：NamespaceLifecycle（防止在正在删除的命名空间中创建资源）、LimitRanger（确保资源请求在 LimitRange 范围内）、ResourceQuota（确保命名空间的资源配额不被超出）、PodSecurity（替代已废弃的 PodSecurityPolicy，实施 Pod 安全标准）。

准入控制阶段:

```
请求 → 认证 → 授权 → 准入控制 → etcd
                         │
                    ┌────┴────┐
                    │ 变更 Webhook │ (Mutating)
                    │ 校验 Webhook │ (Validating)
                    │ 内置控制器   │ (ResourceQuota等)
                    └─────────┘
```

**Pod 安全标准（Pod Security Standards）** 定义了三种安全策略级别：

- **Privileged**: 不受限制，适用于系统和特权级应用
- **Baseline**: 最小限制，禁止已知的危险权限提升
- **Restricted**: 严格限制，遵循 Pod 加固最佳实践

PSS 三级详细对比:

| 控制项 | Privileged | Baseline | Restricted |
|--------|-----------|----------|------------|
| 特权容器 | 允许 | 禁止 | 禁止 |
| hostPath | 允许 | 禁止 | 禁止 |
| hostNetwork | 允许 | 禁止 | 禁止 |
| hostPort | 允许 | 禁止 | 禁止 |
| capabilities 添加 | 允许 | 禁止 | 禁止 |
| runAsNonRoot | 不要求 | 不要求 | 必须 |
| readOnlyRootFilesystem | 不要求 | 不要求 | 必须 |
| drop ALL capabilities | 不要求 | 不要求 | 必须 |

### 可观测性三大支柱

可观测性（Observability）是现代运维的核心能力，它由三大支柱组成：

**Metrics（指标）** 是系统运行状态的数字化表示。Prometheus 是 K8s 生态中最流行的指标采集和存储系统。它采用拉取（Pull）模式，通过 HTTP 端点从目标采集指标数据。Prometheus 的数据模型基于时间序列（Time Series），每个时间序列由指标名称和一组键值对标签（Labels）唯一标识。PromQL 是 Prometheus 的查询语言，支持丰富的聚合、计算和过滤操作。

关键指标类型包括：

- **Counter（计数器）**: 只增不减的累计值，如请求总数、错误总数。通常配合 `rate()` 函数计算速率
- **Gauge（仪表盘）**: 可增可减的当前值，如 CPU 使用率、内存使用量、当前连接数
- **Histogram（直方图）**: 对观测值进行采样并统计分布，用于计算分位数（如 P50、P95、P99 延迟）
- **Summary（摘要）**: 类似 Histogram，但在客户端计算分位数

PromQL 常用函数:

| 函数 | 用途 | 示例 |
|------|------|------|
| rate() | 计算速率 | rate(http_requests_total[5m]) |
| irate() | 瞬时速率 | irate(http_requests_total[5m]) |
| histogram_quantile() | 计算分位数 | histogram_quantile(0.99, rate(latency_bucket[5m])) |
| topk() | 取 Top N | topk(10, cpu_usage) |
| sum by | 按标签聚合 | sum(rate(http_requests[5m])) by (method) |
| avg by | 按标签平均 | avg(cpu_usage) by (instance) |

**Logs（日志）** 记录了系统中的离散事件。在 K8s 中，日志通常分为三个层面：容器标准输出/标准错误（由容器运行时管理）、应用自定义日志文件、集群组件日志（如 kube-apiserver、etcd 的日志）。Loki 是 Grafana Labs 出品的轻量级日志聚合系统，与 Prometheus 共享标签模型，可以在 Grafana 中实现指标与日志的关联查询。

日志采集架构:

```
容器 stdout/stderr → 容器运行时 → /var/log/containers/
                                         ↓
                                    Promtail/Filebeat
                                         ↓
                                    Loki / Elasticsearch
                                         ↓
                                      Grafana / Kibana
```

**Traces（分布式追踪）** 记录了一个请求在分布式系统中经过的完整路径。当请求经过多个微服务时，分布式追踪帮助你定位延迟瓶颈和错误来源。OpenTelemetry 是 CNCF 的可观测性标准，统一了 Metrics、Logs 和 Traces 的数据采集。

### 故障排查方法论

故障排查是运维工程师最考验综合能力的技能。传统的"试错法"效率低下且容易引入新问题。本周将学习两种结构化的故障排查方法论：

**FTA（故障树分析，Fault Tree Analysis）** 是一种自顶向下的分析方法。它从一个问题现象（顶事件）出发，通过布尔逻辑（AND/OR）将可能的问题原因层层分解，最终构建一棵故障树。当问题发生时，你沿着故障树从上到下逐层排查，可以避免遗漏关键排查路径。FTA 特别适合复杂系统的故障分析，因为它强制你系统化地思考所有可能的问题路径。

FTA 核心元素:

```
顶事件 (Top Event): 要分析的问题现象
├── 中间事件 (Intermediate): 进一步分解的子问题
│   ├── 基本事件 (Basic): 不可再分的根因
│   └── 基本事件
└── 中间事件
    ├── 基本事件
    └── 基本事件

逻辑门:
- AND 门: 所有子事件都发生 → 父事件发生
- OR 门: 任一子事件发生 → 父事件发生
```

**FEBM（取证循证方法，Forensic Evidence-Based Method）** 强调"以证据驱动决策"。它的核心流程是：收集证据 → 形成假设 → 验证假设 → 得出结论。FEBM 的关键原则是：每一个推理步骤都必须有数据支撑，不做无根据的猜测。在 K8s 故障排查中，"证据"包括：事件日志（kubectl describe）、资源状态（kubectl get）、指标数据（Prometheus）、应用日志（Loki/ELK）等。

FEBM 流程:

```
# 🟢 低风险：只读/信息收集，通常无副作用
1. 收集证据 → kubectl describe, logs, events, Prometheus
2. 分析证据 → 时间线重建、因果关系、排除法
3. 形成假设 → 基于证据推理可能的根因
4. 验证假设 → 设计实验验证
5. 记录结论 → 根因、修复、预防
```
这两种方法并非互斥，而是互补的：FTA 帮助你构建全面的排查框架，FEBM 帮助你在框架内的每一步做出准确的判断。

---

## 实战演练

### 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 15 | 安全体系: RBAC + 认证授权 | [day-15-security-1.md](./day-15-security-1.md) |
| Day 16 | 安全体系: Pod 安全 + 密钥管理 | [day-16-security-2.md](./day-16-security-2.md) |
| Day 17 | 可观测性: 监控 + Prometheus | [day-17-observability-1.md](./day-17-observability-1.md) |
| Day 18 | 可观测性: 日志 + 分布式追踪 | [day-18-observability-2.md](./day-18-observability-2.md) |
| Day 19 | 故障排查方法论 (关键日) | [day-19-troubleshooting-methodology.md](./day-19-troubleshooting-methodology.md) |
| Day 20 | 故障排查实战 | [day-20-troubleshooting-practice.md](./day-20-troubleshooting-practice.md) |
| Day 21 | 平台运维 + 综合实践 | [day-21-platform-ops.md](./day-21-platform-ops.md) |

### 本周实操环境准备

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确保集群正常运行
kubectl get nodes
kubectl get pods -A

# 创建练习用的命名空间
kubectl create namespace security-lab
kubectl create namespace observability-lab
kubectl create namespace troubleshooting-lab

# 确认 Helm 可用
helm version

# 添加本周需要的 Helm 仓库
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add grafana https://grafana.github.io/helm-charts
helm repo update
```
### 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

### 本周实践项目

**项目 P3**: [可观测性体系搭建 + 故障演练](../projects/p3-observability-fault-drill.md)

---

## 常见问题

### Q1: RBAC 配置错误导致无法操作集群怎么办？

如果你不小心删除了关键的 ClusterRoleBinding，可能导致所有人都无法操作集群。预防措施：始终保留至少一个具有 cluster-admin 权限的 kubeconfig 文件。在 ACK 集群中，可以通过控制台重新获取 kubeconfig。恢复方法：使用 SSH 登录到 Master 节点（专有版），或通过 ACK 控制台执行紧急命令（托管版）。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 预防: 保存 emergency kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config | jq -r '.config' > ~/.kube/config-emergency

# 验证权限
kubectl auth can-i '*' '*' --kubeconfig ~/.kube/config-emergency
```
### Q2: Prometheus 指标太多，存储空间不够怎么办？

Prometheus 的存储占用与指标基数（Cardinality）直接相关。高基数指标（如包含用户 ID 的标签）会指数级增加存储需求。优化策略：使用 recording rules 预聚合高频查询、删除不需要的指标（通过 metric_relabel_configs）、设置合理的保留期（retention）。对于长期存储需求，考虑使用 Thanos 或 Cortex。

```yaml
# 删除高基数指标示例
metric_relabel_configs:
- source_labels: [__name__]
  regex: 'go_.*'
  action: drop

```

### Q3: 如何快速定位一个 Pod 的问题原因？

遵循以下排查步骤：
1. `kubectl get pod <name>` 查看状态
2. `kubectl describe pod <name>` 查看 Events 和 Conditions
3. `kubectl logs <name>` 查看容器日志
4. `kubectl logs <name> --previous` 查看上次崩溃的日志
5. 如果以上都无法定位，使用 `kubectl exec` 进入容器检查

### Q4: 本周学习重点应该放在哪里？

Day 19（故障排查方法论）是本周最关键的一天。FTA 和 FEBM 方法论将贯穿你整个运维生涯。建议至少分配 5-6 小时给这一天。Day 17-18 的可观测性内容也非常重要，因为监控和日志是故障排查的"眼睛"。

---

## 要点总结

| 能力域 | 关键知识点 | 学习日 |
|--------|-----------|--------|
| 安全体系 | 认证/授权/RBAC/ServiceAccount | Day 15 |
| 安全加固 | Pod 安全标准/Secret 管理/网络策略 | Day 16 |
| 监控体系 | Prometheus/PromQL/Grafana | Day 17 |
| 日志追踪 | Loki/Distributed Tracing | Day 18 |
| 排障方法论 | FTA/FEBM | Day 19 |
| 排障实战 | 实际问题场景演练 | Day 20 |
| 综合实践 | 平台运维全流程 | Day 21 |

---

## 延伸阅读

- [认证授权系统](../../../../../../08-%E5%AE%89%E5%85%A8/01-%E8%BA%AB%E4%BB%BD%E4%B8%8E%E8%AE%BF%E9%97%AE/01-authentication-authorization-system.md)
- [RBAC 矩阵配置](../../../../../../08-%E5%AE%89%E5%85%A8/01-%E8%BA%AB%E4%BB%BD%E4%B8%8E%E8%AE%BF%E9%97%AE/07-rbac-matrix-configuration.md)
- [Pod 安全标准](../../../%E5%AE%89%E5%85%A8/06-pod-security-standards.md)
- [可观测性架构总览](../../../../../../09-%E5%8F%AF%E8%A7%82%E6%B5%8B%E6%80%A7/01-%E6%80%BB%E8%A7%88/01-observability-architecture-overview.md)
- [Prometheus 监控](../../../../../../09-%E5%8F%AF%E8%A7%82%E6%B5%8B%E6%80%A7/02-%E6%8C%87%E6%A0%87/10-monitoring-metrics-prometheus.md)
- [FTA 故障树分析](../../../../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/06-FTA%E6%95%85%E9%9A%9C%E6%A0%91/04-fta-core-principles.md)
- [FEBM 取证循证方法](../../../../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/07-FEBM%E6%96%B9%E6%B3%95%E8%AE%BA/01-febm-theory-foundations.md)
- [Pod 综合排障](../../../%E8%AF%8A%E6%96%AD%E6%8E%92%E9%9A%9C/08-pod-comprehensive-troubleshooting.md)
- [Node NotReady 诊断](../../../../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/01-%E6%A0%B8%E5%BF%83%E6%8E%92%E9%9A%9C/06-node-notready-diagnosis.md)

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[10-平台工程/02-运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[23-实体/15-参考与索引/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[21-生态参考/03-领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]

```

<!-- risk-assessed -->
