---
title: 'Day 12: K8S 集群审计'
description: '- "Kubernetes审计日志"'
summary: '- "Kubernetes审计日志"'
category: learning
tags:
- k8s
- training
- hands-on
- apiserver
- coredns
- rbac
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 12: K8S 集群审计 是什么'
- '如何 Day 12: K8S 集群审计'
trigger_keywords:
- Day
- '12:'
- K8S
- 集群审计
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 12: K8S 集群审计

```yaml
---
title: Day 12: 集群审计
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes审计日志"
  - "审计日志配置"
  - "SLS日志分析"
  - "API Server审计"
  - "安全审计"
trigger_keywords:
  - "审计"
  - "审计日志"
  - "Audit"
  - "SLS"
  - "日志分析"
  - "API Server"
  - "审计策略"
  - "合规"
reading_level: intermediate
audience:
  - sre工程师
  - 安全工程师
  - 运维工程师
estimated_read_time: 45min
related_domains:
  - 安全
  - 可观测性
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/inner-training/week-2-security-monitoring/day-8-rbac
  - 生产运维/topic-learn/inner-training/week-2-security-monitoring/day-14-quota-license
  - 可观测性/03-logging-architecture
id: WEEK2-DAY12
topic: training
type: hands-on
tags: [week-2, day-12, audit, security, logging, k8s, k8s-1.28-1.33]
---
```

> **学习时间**: 4-5 小时 | **主题**: 审计日志配置与分析方法

---

## 概述

[[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]] 审计（Audit）是集群安全体系中的重要组成部分，它记录了集群中发生的所有 API 操作，包括谁在什么时候对什么资源执行了什么操作。审计日志是安全合规、问题追溯和操作审计的基础，在安全事件响应中发挥着关键作用。

在 ACK（阿里云容器服务）环境中，审计日志默认集成到阿里云 SLS（日志服务），提供了强大的查询和分析能力。本课程将深入讲解 K8S 审计日志的工作机制、ACK 与 SLS 的集成配置、审计日志的查询分析方法，以及审计告警规则的配置。

**学习目标**：
- 理解 K8S 审计日志的作用和工作机制
- 掌握 ACK 审计日志与 SLS 的集成配置
- 能够查询和分析审计日志
- 了解审计策略配置和告警规则

**前置条件**：
- 已完成 Day 8-11 的安全和监控学习
- 有 SLS（日志服务）基本使用经验
- 了解 kubectl 命令行操作

---

## 核心概念

### K8S 审计日志架构

Kubernetes 审计系统在 API Server 层面记录所有 API 请求，包括认证、授权和准入控制阶段的信息。审计日志可以帮助回答以下关键安全问题：

- 谁访问了集群？从哪里访问的？
- 对哪些资源执行了什么操作？
- 操作是否成功？
- 是否有异常的权限提升尝试？

#### 审计日志工作流程

```
用户请求 → API Server
              │
              ├── 1. 认证 (Authentication)
              ├── 2. 授权 (Authorization)
              ├── 3. 准入控制 (Admission Control)
              │
              ├── 4. 审计日志记录 (Audit Logging)
              │       │
              │       ▼
              │   审计策略匹配
              │       │
              │       ▼
              │   按级别记录事件
              │       │
              │       ▼
              │   发送到后端 (日志文件 / Webhook)
              │
              └── 5. 处理请求
```

### 审计级别详解

K8S 定义了四个审计级别，从低到高记录的信息详细程度递增：

| 级别 | 记录内容 | 存储开销 | 适用场景 |
|------|---------|---------|---------|
| **None** | 不记录任何信息 | 无 | 不需要审计的低优先级资源 |
| **Metadata** | 请求元数据（用户、时间、资源、操作） | 低 | 大多数资源操作 |
| **Request** | 元数据 + 请求体 | 中 | 需要看到请求内容的场景 |
| **RequestResponse** | 元数据 + 请求体 + 响应体 | 高 | 需要完整信息的关键操作 |

### 审计日志关键字段

| 字段 | 说明 | 示例 |
|------|------|------|
| `requestReceivedTimestamp` | 请求接收时间 | 2026-05-18T10:30:00.123456Z |
| `verb` | 操作类型 | get, list, create, update, delete, patch |
| `user.username` | 操作用户 | ram-user@1234567890 |
| `user.groups` | 用户组 | [system:authenticated, dev-team] |
| `sourceIPs` | 来源 IP | ["192.168.1.100"] |
| `objectRef.resource` | 资源类型 | [[Pods|pods]], [[Deployments|deployments]], secrets |
| `objectRef.namespace` | 命名空间 | default, kube-system |
| `objectRef.name` | 资源名称 | my-app-xxx |
| `responseStatus.code` | 响应状态码 | 200, 201, 403, 404 |
| `requestObject` | 请求体（Request级别） | Pod YAML 定义 |
| `responseObject` | 响应体（RequestResponse级别） | 完整资源对象 |

---

## 实战演练

### 任务 1: 审计日志配置 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看 ACK 集群审计配置
aliyun cs GET /clusters/<cluster_id> | jq '.meta_data' | jq -r . | jq '.AuditProjectName'

# 预期输出:
# "k8s-log-c1234567890abcdef"

# Step 2: 在 SLS 控制台查看审计日志
# 控制台路径: 日志服务 → Project: k8s-log-<cluster_id> → Logstore: apiserver-audit-log

# Step 3: 查看 SLS Project 信息
aliyun sls GetProject --projectName=k8s-log-<cluster_id>

# Step 4: 查看 Logstore 配置
aliyun sls GetLogstore --projectName=k8s-log-<cluster_id> --logstoreName=apiserver-audit-log

# Step 5: 通过 kubectl 查看事件（作为审计日志的补充）
kubectl get events -A --sort-by='.lastTimestamp' | tail -20

# 预期输出:
# NAMESPACE   LAST SEEN   TYPE      REASON     OBJECT              MESSAGE
# default     2m          Normal    Pulled     pod/my-app-xxx      Successfully pulled image
# default     1m          Warning   Failed     pod/my-app-xxx      Error: ImagePullBackOff
# kube-system 30s        Normal    Started    pod/coredns-xxx     Started container coredns

# Step 6: 检查 API Server 审计配置
kubectl get pods -n kube-system -l component=kube-apiserver -o yaml | grep -A 10 "audit"
# 在 ACK 托管版中，审计配置由阿里云管理
```
### 任务 2: 审计日志查询 (45min)

```bash
# === 在 SLS 控制台查询审计日志 ===

# 查询 1: 查看所有删除操作
# SLS 查询语句:
# verb: delete | select * from log order by requestReceivedTimestamp desc limit 50

# 查询 2: 查看特定用户的操作
# user.username: "ram-user@1234567890" | select * from log order by requestReceivedTimestamp desc limit 50

# 查询 3: 查看 Secret 相关操作
# objectRef.resource: secrets | select * from log order by requestReceivedTimestamp desc limit 50

# 查询 4: 查看失败的请求
# responseStatus.code >= 400 | select * from log order by requestReceivedTimestamp desc limit 50

# 查询 5: 查看 RBAC 相关操作
# objectRef.resource: roles OR objectRef.resource: clusterroles OR objectRef.resource: rolebindings OR objectRef.resource: clusterrolebindings

# 查询 6: 统计过去 1 小时的操作分布
# * | select verb, count(*) as cnt from log group by verb order by cnt desc limit 20

# 预期输出:
# verb      | cnt
# --------- | -----
# get       | 15234
# list      | 8756
# watch     | 4321
# create    | 567
# update    | 234
# delete    | 89

# 查询 7: 查看创建 Pod 的操作详情
# verb: create AND objectRef.resource: pods | select requestReceivedTimestamp, user.username, objectRef.namespace, objectRef.name, responseStatus.code from log order by requestReceivedTimestamp desc limit 20
```

### 任务 3: 审计告警配置 (30min)

```bash
# === 在 SLS 中配置审计告警 ===

# 告警规则 1: 高权限操作告警 - ClusterRoleBinding 创建
# 查询: verb: create AND objectRef.resource: clusterrolebindings
# 触发条件: count > 0
# 通知: 钉钉/企微群 + Email

# 告警规则 2: 异常删除操作 - Deployment 被删除
# 查询: verb: delete AND objectRef.resource: deployments
# 触发条件: count > 0
# 通知: 钉钉/企微群 + 电话（生产命名空间）

# 告警规则 3: 密钥访问监控 - Secret 被读取
# 查询: verb: get AND objectRef.resource: secrets AND objectRef.namespace != kube-system
# 触发条件: count > 10（5分钟内超过10次访问）
# 通知: 安全团队

# 告警规则 4: 权限提升尝试 - 403 错误
# 查询: responseStatus.code: 403
# 触发条件: count > 5（5分钟内超过5次未授权尝试）
# 通知: 安全团队

# 告警规则 5: 特权容器创建
# 查询: verb: create AND objectRef.resource: pods AND requestObject: "privileged.*true"
# 触发条件: count > 0
# 通知: 安全团队

# SLS 告警配置示例 (通过 API)
cat > sls-alert-config.json << 'EOF'
{
  "alertName": "k8s-audit-privilege-escalation",
  "projectName": "k8s-log-<cluster_id>",
  "displayName": "K8s 审计: 高权限操作检测",
  "condition": "verb: create AND objectRef.resource: clusterrolebindings | select count(*) as cnt from log",
  "threshold": "cnt > 0",
  "notification": {
    "type": "dingtalk",
    "webhook": "https://oapi.dingtalk.com/robot/send?access_token=xxx"
  }
}
EOF
```

### 任务 4: 审计日志分析实践 (30min)

```bash
# === 场景: 排查"谁删除了某个 Deployment" ===

# Step 1: 在 SLS 中查询删除操作
# 查询语句:
# verb: delete AND objectRef.resource: deployments AND objectRef.name: <deployment-name> AND objectRef.namespace: <namespace>

# Step 2: 关注以下关键字段:
# - user.username: 操作者身份
# - sourceIPs: 来源 IP 地址
# - requestReceivedTimestamp: 操作时间
# - objectRef.namespace: 命名空间
# - objectRef.name: 资源名称
# - responseStatus.code: 操作是否成功 (200=成功)

# Step 3: 预期查询结果示例:
# requestReceivedTimestamp     | user.username         | sourceIPs       | responseStatus.code
# 2026-05-18T10:25:30.123456Z  | developer@1234567890  | 192.168.1.100   | 200
# (成功删除，操作者 developer@1234567890，来源 IP 192.168.1.100)

# Step 4: 扩展查询 - 该用户的其他操作
# user.username: "developer@1234567890" | select requestReceivedTimestamp, verb, objectRef.resource, objectRef.name from log order by requestReceivedTimestamp desc limit 50

# Step 5: 形成审计报告:
# ============================================
# K8S 集群审计报告
# ============================================
# 事件: Deployment <name> 被删除
# 操作者: developer@1234567890
# 来源 IP: 192.168.1.100
# 操作时间: 2026-05-18 10:25:30 UTC
# 命名空间: production
# 操作结果: 成功 (HTTP 200)
# ============================================
# 建议措施:
# 1. 确认该操作是否为授权操作
# 2. 检查 RBAC 权限是否过于宽松
# 3. 考虑启用 Deployment 删除的二次确认机制
# ============================================
```

---

## 配置参考

### 审计策略配置示例

```yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
- level: None
  resources:
  - group: ""
    resources: ["endpoints"]
  nonResourceURLs:
  - /healthz
  - /livez
  - /readyz
  omitStages:
  - RequestReceived

- level: Metadata
  resources:
  - group: ""
    resources: ["pods", "services", "configmaps"]

- level: Request
  resources:
  - group: ""
    resources: ["secrets"]
  - group: "rbac.authorization.k8s.io"
    resources: ["roles", "clusterroles", "rolebindings", "clusterrolebindings"]

- level: RequestResponse
  resources:
  - group: ""
    resources: ["namespaces"]
  - group: "rbac.authorization.k8s.io"
    resources: ["clusterrolebindings"]
  verbs: ["create", "delete"]

- level: Metadata
  omitStages:
  - RequestReceived
```

### 审计策略参数说明

| 参数 | 说明 | 示例 |
|------|------|------|
| `level` | 审计级别 | None, Metadata, Request, RequestResponse |
| `resources` | 匹配的资源类型 | pods, deployments, secrets |
| `verbs` | 匹配的操作类型 | get, list, create, update, delete |
| `nonResourceURLs` | 匹配的非资源 URL | /healthz, /metrics |
| `omitStages` | 忽略的阶段 | RequestReceived |
| `user` | 匹配的用户 | system:kube-proxy |

### SLS 查询语法参考

| 语法 | 说明 | 示例 |
|------|------|------|
| `field: value` | 精确匹配 | `verb: delete` |
| `field >= value` | 数值比较 | `responseStatus.code >= 400` |
| `AND` | 逻辑与 | `verb: delete AND objectRef.resource: pods` |
| `OR` | 逻辑或 | `objectRef.resource: roles OR objectRef.resource: clusterroles` |
| `*` | 查询所有 | `* | select count(*) from log` |
| `| select` | SQL 分析 | `* | select verb, count(*) from log group by verb` |
| `order by` | 排序 | `order by requestReceivedTimestamp desc` |
| `limit N` | 限制条数 | `limit 50` |

---

## 常见问题

### Q1: K8S 审计日志记录了什么信息？有什么价值？

**A**: 审计日志记录了所有经过 API Server 的请求信息，包括：
- **操作者**: 谁执行的（用户名、IP 地址）
- **操作内容**: 对什么资源执行了什么操作
- **操作结果**: 是否成功（HTTP 状态码）
- **时间**: 操作发生的精确时间

**价值**:
1. **安全合规**: 满足安全审计要求，证明操作可追溯
2. **问题追溯**: 确认"谁删除了某个 Deployment"
3. **异常检测**: 发现未授权的访问尝试
4. **行为分析**: 了解集群使用模式，优化权限配置

### Q2: ACK 审计日志如何与 SLS 集成？

**A**: ACK 托管版集群默认集成 SLS：
1. 创建集群时自动创建 SLS Project（`k8s-log-<cluster_id>`）
2. 审计日志自动写入 `apiserver-audit-log` Logstore
3. 可在 ACK 控制台直接查看，也可在 SLS 控制台高级查询
4. 日志保留天数和存储容量可在 SLS 控制台配置
5. 开启路径: ACK 控制台 → 集群详情 → 集群审计 → 开启

### Q3: 审计日志存储成本如何控制？

**A**: 控制审计日志存储成本的方法：
1. **合理设置审计级别**: 大多数资源用 Metadata 级别，仅关键资源用 RequestResponse
2. **设置日志保留天数**: 根据合规要求设置，通常 30-90 天
3. **使用冷热分层**: 热数据 7 天，温数据 30 天，冷数据 90 天
4. **过滤低价值日志**: 健康检查、metrics 采集等高频低价值操作设为 None 级别
5. **使用 SLS 降低存储副本数**: 非关键场景可减少副本数

### Q4: 审计日志和 Events 有什么区别？

**A**: 两者记录的信息不同：
- **审计日志**: 记录 API 操作（谁对什么资源做了什么），面向安全和合规
- **Events**: 记录集群内的事件通知（Pod 调度、拉取镜像等），面向排障
- **建议**: 安全审计用审计日志，日常排障用 Events

### Q5: 如何通过审计日志发现安全威胁？

**A**: 重点关注以下场景：
1. **频繁的 403 错误**: 可能有人在尝试未授权访问
2. **非预期的 ClusterRoleBinding 创建**: 可能是权限提升攻击
3. **大量 Secret 读取**: 可能是凭证窃取
4. **来自异常 IP 的操作**: 可能是账号泄露
5. **凌晨/周末的高权限操作**: 可能是非授权操作
6. **删除关键资源**: 如删除 Namespace、Deployment

---

## 要点总结

- **审计日志**记录所有 API 操作，是安全合规和问题追溯的基础
- **四个审计级别**: None < Metadata < Request < RequestResponse
- **ACK + SLS** 提供开箱即用的审计日志查询和分析能力
- **审计告警**可以实时检测异常操作，如权限提升、未授权访问
- **关键查询场景**: 删除操作、Secret 访问、RBAC 变更、403 错误
- 合理配置 **审计级别** 可以平衡安全需求与存储成本

---

## 延伸阅读

- [Kubernetes 审计官方文档](https://kubernetes.io/docs/tasks/debug/debug-cluster/audit/)
- [阿里云 SLS 文档](https://help.aliyun.com/product/28958.html)
- [ACK 审计日志文档](https://help.aliyun.com/document_detail/155212.html)
- [文件: `../../../可观测性/01-observability-architecture-overview.md`](../../../可观测性/01-observability-architecture-overview.md)
- [文件: `../../../可观测性/03-logging-architecture.md`](../../../可观测性/03-logging-architecture.md)

---

## 明日预告

Day 13 将学习集群监控体系搭建与告警配置。


<!-- risk-assessed -->
