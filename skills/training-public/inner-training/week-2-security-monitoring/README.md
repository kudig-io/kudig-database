---
title: 'Week 2: 安全认证与监控运维 (Days 8-14)'
description: '# Week 2: 安全认证与监控运维 (Days 8-14)'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- prometheus
- grafana
- opa
- rbac
- networkpolicy
- operator
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 2: 安全认证与监控运维 (Days 8-14) 是什么'
- '如何 Week 2: 安全认证与监控运维 (Days 8-14)'
trigger_keywords:
- Week
- '2:'
- 安全认证与监控运维
- Days
- 8-14
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- gpu-scheduling-basics
- policy-basics
---

# Week 2: 安全认证与监控运维 (Days 8-14)

```yaml
---
title: Week 2: 安全认证与监控运维
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes安全监控培训"
  - "Week2培训内容"
  - "RBAC权限管理"
  - "审计日志配置"
  - "集群监控搭建"
trigger_keywords:
  - "Week2"
  - "安全"
  - "监控"
  - "RBAC"
  - "审计"
  - "配额"
  - "监控告警"
  - "Prometheus"
  - "Grafana"
  - "安全运维"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 安全工程师
estimated_read_time: 15min
related_domains:
  - domain-05-security-compliance
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/inner-training/week-1-ack-acr-lifecycle
  - domain-11-production-operations/topic-learn/inner-training/week-3-node-workload
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-8-rbac
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-12-cluster-audit
  - domain-11-production-operations/topic-learn/inner-training/week-2-security-monitoring/day-14-quota-license
id: WEEK2-INDEX
topic: training
type: week-index
tags: [week-2, security, monitoring, rbac, audit, k8s, k8s-1.28-1.33]
---
```

## 概述

第二周进入 K8s 集群的安全与监控领域。在第一周中，你掌握了集群的创建、删除、升级等生命周期管理操作。本周将学习如何保护集群安全、识别和防范安全风险、配置审计日志，以及搭建基础监控体系。

安全是生产环境的底线。一个配置不当的 K8s 集群可能面临权限滥用、容器逃逸、数据泄露等严重风险。监控是运维的"眼睛"，没有完善的监控体系，你就无法及时发现问题、更无法在故障发生时快速响应。

### 学习目标

- 深入理解 RBAC 权限模型并能够根据实际需求设计权限方案
- 掌握 RAM（阿里云资源访问管理）与 K8s 权限的集成配置
- 了解 ACK、ACR 和 K8s 常见漏洞类型及其防范措施
- 掌握集群审计日志的配置、采集与分析方法
- 能够搭建基于 Prometheus + Grafana 的基础监控体系
- 了解集群配额管理和 License 管理
- **产出**: 能够配置集群 RBAC 权限、识别安全风险、搭建基础监控

---

## 核心概念详解

### RBAC 权限模型详解

RBAC（Role-Based Access Control）是 Kubernetes 的核心授权机制。理解 RBAC 的关键在于掌握四个核心资源之间的关系：

**Role 和 ClusterRole** 定义了"什么操作可以在什么资源上执行"。Role 是命名空间级别的，只能授权该命名空间内的资源操作。ClusterRole 是集群级别的，可以授权跨命名空间的资源操作或集群级资源（如 Node、PersistentVolume、Namespace）的操作。ClusterRole 还有一个特殊用途：可以通过 RoleBinding 在特定命名空间内引用，实现"定义一次，多处使用"的效果。

**RoleBinding 和 ClusterRoleBinding** 定义了"谁拥有这些权限"。RoleBinding 将 Role（或 ClusterRole）绑定到用户、组或 ServiceAccount，绑定的权限仅在 RoleBinding 所在的命名空间内生效。ClusterRoleBinding 将 ClusterRole 绑定到主体，权限在整个集群范围内生效。

RBAC 四大资源关系:

```
ClusterRole ──→ ClusterRoleBinding ──→ User/Group/SA  (集群级权限)
     │
     └──────→ RoleBinding ──→ User/Group/SA  (命名空间级权限)

Role ──→ RoleBinding ──→ User/Group/SA  (命名空间级权限)
```

权限设计的最佳实践：

- **最小权限原则**: 只授予完成任务所需的最少权限。避免使用通配符（`*`）匹配所有资源或操作
- **命名空间隔离**: 不同团队或环境使用不同的命名空间，通过 RoleBinding 限制访问范围
- **ServiceAccount 最小化**: 每个 Pod 应使用专属的 ServiceAccount，而非共享 default ServiceAccount
- **定期审计**: 定期检查 RBAC 配置，清理不再需要的权限授予

RBAC 规则中的 verbs（操作动词）对应 K8s API 的操作类型：

| Verb | 说明 | 对应 HTTP 方法 |
|------|------|--------------|
| get | 获取单个资源 | GET /api/.../name |
| list | 列出资源集合 | GET /api/... |
| watch | 监听资源变化 | GET /api/...?watch=true |
| create | 创建资源 | POST /api/... |
| update | 更新资源 | PUT /api/.../name |
| patch | 部分更新 | PATCH /api/.../name |
| delete | 删除资源 | DELETE /api/.../name |
| deletecollection | 删除集合 | DELETE /api/... |

### RAM 与 K8s 权限集成

在阿里云 ACK 中，集群的权限管理有两个层面：RAM 层面和 K8s RBAC 层面。

**RAM（Resource Access Management）** 控制的是阿里云资源层面的访问权限，包括：是否可以查看集群列表、是否可以创建/删除集群、是否可以查看节点信息等。RAM 通过策略（Policy）来定义权限，策略可以附加到 RAM 用户、RAM 角色或 RAM 用户组。

**K8s RBAC** 控制的是集群内部的 API 访问权限，包括：是否可以查看/创建/删除 Pod、Service、Deployment 等资源。ACK 提供了预定义的 RBAC 角色（cluster-admin、admin、operator、readonly、dev）以简化权限配置。

ACK 预定义 RBAC 角色:

| 角色 | 权限范围 | 适用人员 |
|------|---------|---------|
| cluster-admin | 所有命名空间完全控制 | 集群管理员 |
| admin | 指定命名空间完全控制 | 团队负责人 |
| operator | 指定命名空间运维操作 | 运维工程师 |
| dev | 指定命名空间开发操作 | 开发工程师 |
| readonly | 指定命名空间只读 | 产品经理/审计 |

两者的映射关系：RAM 用户或角色首先需要通过 RAM 策略获得集群的访问权限，然后通过 RBAC 配置获得集群内部的具体操作权限。在 ACK 控制台中，可以通过"授权管理"页面同时配置这两个层面的权限。

### K8s 安全漏洞与风险防范

K8s 安全漏洞可以从以下几个层面来理解：

**容器镜像安全**: 使用了包含已知漏洞的基础镜像。防范措施：定期扫描镜像漏洞（使用 ACR 的安全扫描功能）、使用最小化基础镜像（如 distroless、alpine）、及时更新基础镜像版本。

**配置安全**: 不安全的 K8s 配置，如：运行特权容器（privileged: true）、挂载宿主机路径（hostPath）、使用 hostNetwork、以 root 用户运行容器等。防范措施：实施 Pod 安全标准（Pod Security Standards）、使用策略引擎（如 OPA/Kyverno）强制安全策略。

**网络安全**: Pod 之间默认可以互相通信，攻击者可以利用这一点进行横向移动。防范措施：配置 NetworkPolicy 限制 Pod 间通信、使用网络加密（如 WireGuard）、限制出站流量。

**密钥管理**: 将 Secret 明文存储在 etcd 中（仅 Base64 编码）。防范措施：启用 etcd 加密、使用外部密钥管理工具（如 Vault）、使用 Sealed Secrets 加密存储。

安全风险等级与优先级:

| 风险等级 | 典型威胁 | 修复优先级 | 修复时间要求 |
|----------|---------|-----------|------------|
| 高危 | 特权容器、API Server 暴露 | P0 | 24 小时内 |
| 中高危 | 无 NetworkPolicy、Secret 明文 | P1 | 1 周内 |
| 中危 | 未启用审计、default SA 权限过大 | P2 | 1 月内 |
| 低危 | 未配置 PSS、SA Token 自动挂载 | P3 | 按计划修复 |

### 审计日志

K8s 审计日志记录了集群中所有的 API 调用，是安全合规和故障追溯的重要工具。审计日志可以帮助你回答以下问题：

- 谁在什么时间执行了什么操作？
- 操作的对象是什么？结果如何？
- 是否有异常的权限提升行为？

在 ACK 集群中，审计日志默认会采集到阿里云 SLS（日志服务）。你可以通过 SLS 控制台查询和分析审计日志。审计策略（Audit Policy）定义了哪些事件需要记录以及记录的级别（Metadata、Request、RequestResponse）。

审计级别说明:

| 级别 | 记录内容 | 存储开销 | 适用场景 |
|------|---------|---------|---------|
| None | 不记录 | 无 | 不关注的操作 |
| Metadata | 仅请求元数据 | 小 | 大部分操作 |
| Request | 元数据 + 请求体 | 中 | 敏感操作 |
| RequestResponse | 元数据 + 请求体 + 响应体 | 大 | 密码/证书操作 |

### 监控体系搭建

基于 Prometheus + Grafana 的监控体系是 K8s 社区的标准方案。

**Prometheus** 负责数据采集和存储。它通过 Service Discovery 自动发现 K8s 中的监控目标，通过 HTTP 拉取指标数据。Prometheus 支持 PromQL 查询语言，可以进行丰富的数据聚合和计算。

**Grafana** 负责数据可视化和告警。它从 Prometheus 读取数据并以图表的形式展示。Grafana 支持丰富的面板类型（时间序列图、仪表盘、热力图等）和灵活的告警规则。

监控体系的关键指标包括：

- **节点级指标**: CPU 使用率、内存使用率、磁盘 IO、网络流量、文件系统使用率
- **Pod 级指标**: CPU/内存请求量和限制量、容器重启次数、OOMKilled 事件
- **应用级指标**: HTTP 请求率、错误率、延迟分布（由应用通过 /metrics 端点暴露）
- **集群级指标**: 调度失败次数、资源使用率、Pod 驱逐次数

### 集群配额与资源管理

**ResourceQuota** 限制了命名空间的资源使用总量，包括：CPU/内存的请求和限制、Pod 数量、Service 数量、PVC 数量等。它帮助防止某个团队或应用占用过多资源。

**LimitRange** 定义了 Pod/Container 的默认资源请求和限制，以及资源的最小/最大值。当 Pod 未指定资源请求时，LimitRange 的默认值会自动填充。

**ACK 集群配额和 License** 管理了集群级别的资源上限。ACK 不同版本有不同的配额限制（如托管版默认最多 500 个节点）。了解这些限制有助于在规划集群时做出合理的决策。

---

## 实战演练

### 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 8 | K8S 集群 RBAC | [day-8-rbac.md](./day-8-rbac.md) |
| Day 9 | RAM 账号管理 | [day-9-ram-integration.md](./day-9-ram-integration.md) |
| Day 10 | ACK/ACR/K8S 漏洞 | [day-10-vulnerability.md](./day-10-vulnerability.md) |
| Day 11 | 风险点识别与防范 | [day-11-risk-prevention.md](./day-11-risk-prevention.md) |
| Day 12 | K8S 集群审计 | [day-12-cluster-audit.md](./day-12-cluster-audit.md) |
| Day 13 | K8S 集群监控 | [day-13-cluster-monitoring.md](./day-13-cluster-monitoring.md) |
| Day 14 | K8S 集群配额 & License | [day-14-quota-license.md](./day-14-quota-license.md) |

### 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

### 本周实践项目

**项目 P2**: [安全认证与监控体系搭建](../projects/p2-security-monitoring-setup.md)

---

## 常见问题

### Q1: RBAC 权限配置后不生效怎么办？

权限配置不生效的常见原因：1) RoleBinding/ClusterRoleBinding 中的 subjects 字段配置错误（如 ServiceAccount 的命名空间不匹配）；2) Role 中的 apiGroups 或 resources 名称错误；3) 用户连接集群时使用了不同的认证身份。排查命令：`kubectl auth can-i <verb> <resource> --as=<user>` 可以模拟验证权限。

### Q2: 如何在不影响业务的情况下启用审计日志？

启用审计日志不会影响集群中正在运行的业务。审计日志的采集是在 API Server 层面进行的，对业务流量没有影响。但需要注意：审计日志量可能很大，建议只记录必要的事件级别，并配置合理的日志保留策略。

### Q3: Prometheus 监控数据丢失怎么办？

Prometheus 数据丢失的常见原因：1) Prometheus Pod 重启导致本地存储数据丢失（使用 PersistentVolume 可以避免）；2) 存储空间不足导致写入失败（监控 Prometheus 的存储使用率并设置告警）；3) Target 采集超时或网络不通（检查 Prometheus 的 Targets 页面）。

### Q4: 如何平衡安全性和便利性？

安全与便利性确实存在矛盾。推荐的平衡策略：开发环境可以适当放宽安全限制以提高效率；预发和生产环境必须严格执行安全策略。通过 CI/CD 自动化安全检查（如镜像漏洞扫描、配置合规检查），可以在不影响开发效率的前提下保障安全。

---

## 要点总结

| 主题 | 关键知识点 | 学习日 |
|------|-----------|--------|
| RBAC | Role/ClusterRole/Binding 设计 | Day 8 |
| RAM 集成 | RAM 用户/角色/策略与 K8s 映射 | Day 9 |
| 漏洞安全 | CVE、镜像安全、配置安全 | Day 10 |
| 风险防范 | 安全基线、风险评估 | Day 11 |
| 审计日志 | 审计策略、SLS 集成、日志分析 | Day 12 |
| 监控体系 | Prometheus + Grafana | Day 13 |
| 配额管理 | ResourceQuota、LimitRange | Day 14 |

---

## 延伸阅读

- [认证授权系统](../../domain-05-security-compliance/01-authentication-authorization-system.md)
- [RBAC 矩阵配置](../../domain-05-security-compliance/07-rbac-matrix-configuration.md)
- [Pod 安全标准](../../domain-05-security-compliance/06-pod-security-standards.md)
- [证书管理](../../domain-05-security-compliance/10-certificate-management.md)
- [Secret 管理工具](../../domain-05-security-compliance/11-secret-management-tools.md)
- [可观测性架构总览](../../domain-06-observability/01-observability-architecture-overview.md)
- [Prometheus 监控](../../domain-06-observability/10-monitoring-metrics-prometheus.md)

## Related

- [[domain-19-landscape-references/98-merged-indexes/README-from-domain-19-landscape-references|Domain-34: CNCF Landscape 开源项目]] — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[domain-03-networking-traffic/98-merged-indexes/MOC-from-domain-03-networking-traffic|domain-03-networking-traffic MOC]] — Cross-reference
- [[domain-20-application-patterns/98-merged-indexes/README-from-domain-20-application-patterns|Topic 应用层架构设计最佳实践]] — Cross-reference
- [[domain-20-application-patterns/98-merged-indexes/MOC-from-domain-20-application-patterns|topic-application-architecture MOC]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- [[domain-08-release-change-management/98-merged-indexes/MOC-from-domain-08-release-change-management|domain-08-release-change-management MOC]] — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system|监控告警体系]] — Cross-reference
- [[domain-09-reliability-engineering/98-merged-indexes/README-from-domain-09-reliability-engineering|Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity)]] — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
