---
title: 'Week 2 Checkpoint: 自测检验 [week-2-security-monitoring]'
description: 'title: Week 2 自测: 安全认证与监控运维'
summary: 'title: Week 2 自测: 安全认证与监控运维'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- coredns
- job
- rbac
- networkpolicy
- operator
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
- 'Week 2 Checkpoint: 自测检验 是什么'
- '如何 Week 2 Checkpoint: 自测检验'
trigger_keywords:
- Week
- 'Checkpoint:'
- 自测检验
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Week 2 Checkpoint: 自测检验

```yaml
---
title: Week 2 自测: 安全认证与监控运维
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "Kubernetes安全自测"
  - "Week2测试题"
  - "RBAC自测"
  - "监控告警测试"
trigger_keywords:
  - "自测"
  - "Week2"
  - "RBAC"
  - "审计"
  - "监控"
  - "配额"
  - "ResourceQuota"
  - "LimitRange"
  - "PSS"
  - "Pod安全"
reading_level: intermediate
audience:
  - sre工程师
  - ops工程师
  - 运维工程师
estimated_read_time: 60min
related_domains:
  - 安全
  - 可观测性
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/inner-training/week-2-security-monitoring/day-8-rbac
  - 生产运维/topic-learn/inner-training/week-2-security-monitoring/day-12-cluster-audit
  - 生产运维/topic-learn/inner-training/week-2-security-monitoring/day-14-quota-license
id: WEEK2-CHECKPOINT
topic: training
type: checkpoint
tags: [week-2, checkpoint, self-test, security, monitoring, k8s, k8s-1.28-1.33]
---
```

> 完成本周学习后，请独立完成以下自测题，不要查阅资料。

---

## 概述

Week 2 聚焦于安全认证与监控运维两大核心领域。安全方面涵盖了 RBAC 权限模型、RAM 账号集成、漏洞管理和审计日志；监控方面包括了集群监控搭建和告警配置。这些是日常运维中最常见的工作内容，掌握这些知识是独立处理安全相关工单的基础。

本自测包含概念理解（5 题）、命令实操（5 题）和场景分析（4 题）三个部分。请独立完成，完成后对照参考答案进行自我评估。

---

## 一、概念理解 (每题 2 分，共 20 分)

### 1. RBAC 中 Role 和 ClusterRole 的区别是什么？RoleBinding 和 ClusterRoleBinding 呢？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**

| 资源 | 作用范围 | 典型场景 |
|------|---------|---------|
| Role | 单个 Namespace | 开发人员只能操作 dev 命名空间 |
| ClusterRole | 整个集群 | 管理节点、PV、查看所有命名空间 |
| RoleBinding | 将角色绑定到 Namespace | 用户 → Role，限制在某个命名空间 |
| ClusterRoleBinding | 将角色绑定到集群 | 管理员 → ClusterRole，全集群权限 |

**重要组合**: ClusterRole + RoleBinding = 复用 ClusterRole 定义，但将权限限制在某个 Namespace

```yaml
# 示例: ClusterRole + RoleBinding 组合
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: pod-reader
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-pod-reader
  namespace: dev
subjects:
- kind: User
  name: developer@company.com
roleRef:
  kind: ClusterRole
  name: pod-reader
  apiGroup: rbac.authorization.k8s.io
```

---

### 2. ACK 的两层权限模型是什么？各自控制什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**

| 层次 | 权限系统 | 控制范围 | 配置位置 |
|------|---------|---------|---------|
| **第一层** | RAM (Resource Access Management) | 云平台操作（创建/删除集群、节点池管理） | RAM 控制台 |
| **第二层** | RBAC (Role-Based Access Control) | 集群内 K8S 资源操作（Pod/Deployment/Secret） | kubectl / ACK 控制台 |

两层权限独立但互补：
- RAM 控制"能否访问 ACK 服务"
- RBAC 控制"能在集群内做什么"
- 必须两层都通过才能操作集群资源

```
用户 → RAM 认证 → ACK API → RBAC 授权 → K8S 资源
         │                        │
         ▼                        ▼
    "你能用 ACK 吗？"      "你能操作哪些 K8S 资源？"
```

---

### 3. K8S 审计日志有哪四个级别？各自记录什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**

| 级别 | 记录内容 | 存储开销 | 适用场景 |
|------|---------|---------|---------|
| **None** | 不记录 | 无 | 低优先级操作 (healthz) |
| **Metadata** | 用户、时间、资源类型、操作类型 | 低 | 大多数操作 |
| **Request** | 元数据 + 请求体内容 | 中 | 需要看到请求参数的操作 |
| **RequestResponse** | 元数据 + 请求体 + 响应体 | 高 | 关键操作（Secret 读取、RBAC 变更） |

---

### 4. ResourceQuota 和 LimitRange 的区别是什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**

| 维度 | ResourceQuota | LimitRange |
|------|--------------|-----------|
| **作用** | 限制 Namespace 下所有资源的总量 | 限制单个 Pod/Container 的资源范围 |
| **粒度** | 宏观（Namespace 级别） | 微观（Container 级别） |
| **功能** | 限制 CPU/内存/Pod数/Service数/PVC数 | 设置默认值、最大值、最小值 |
| **关系** | 两者配合使用效果最佳 | LimitRange 自动添加默认值，ResourceQuota 控制总量 |

```yaml
# LimitRange 示例: 设置默认资源
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
spec:
  limits:
  - type: Container
    default:
      cpu: 200m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi

---
# ResourceQuota 示例: 限制总量
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
spec:
  hard:
    requests.cpu: "4"
    requests.memory: 8Gi
    pods: "20"
```

---

### 5. Pod Securityod Security Standards]] (PSS) 的三个级别分别是什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**

| 级别 | 安全限制 | 允许特权 | 适用场景 |
|------|---------|---------|---------|
| **Privileged** | 无限制 | 特权容器、hostPID、hostNetwork | 系统组件（CNI、CSI） |
| **Baseline** | 基本限制 | 禁止特权容器、禁止 host 命名空间 | 通用应用 |
| **Restricted** | 严格限制 | 非 root、drop ALL capabilities、只读 FS | 安全敏感应用 |

**启用方式**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 为 Namespace 启用 PSS
kubectl label namespace <ns> pod-security.kubernetes.io/enforce=restricted
kubectl label namespace <ns> pod-security.kubernetes.io/audit=baseline
kubectl label namespace <ns> pod-security.kubernetes.io/warn=baseline
```
---

## 二、命令实操 (每题 2 分，共 10 分)

### 6. 如何检查某个 ServiceAccount 是否有权限创建 Deployment？

**你的回答:**
```
# 🟢 低风险：只读/信息收集，通常无副作用
```

**参考答案:**

```bash
# 检查特定 ServiceAccount 的权限
kubectl auth can-i create deployments \
  --as=system:serviceaccount:<namespace>:<sa-name> \
  -n <namespace>

# 预期输出:
# yes

# 列出 ServiceAccount 的所有权限
kubectl auth can-i --list \
  --as=system:serviceaccount:<namespace>:<sa-name> \
  -n <namespace>

# 预期输出:
# Resources   Non-Resource URLs   Resource Names   Verbs
# pods        []                  []               [get list watch]
# deployments []                  []               [get list watch create]
```
---

### 7. 如何查看当前 Namespace 的资源配额使用情况？

**你的回答:**
```
# 🟢 低风险：只读/信息收集，通常无副作用
```

**参考答案:**

```bash
# 查看 ResourceQuota 列表
kubectl get resourcequota -n <namespace>

# 查看详细使用情况
kubectl describe resourcequota -n <namespace>

# 预期输出:
# Name:            compute-quota
# Namespace:       dev
# Resource         Used    Hard
# --------         ----    ----
# limits.cpu       800m    4
# limits.memory    1Gi     8Gi
# pods             4       20
# requests.cpu     400m    4
# requests.memory  512Mi   8Gi
# services         2       10
```
---

### 8. 如何检查集群中是否有特权容器在运行？

**你的回答:**
```
# 🟢 低风险：只读/信息收集，通常无副作用
```

**参考答案:**

```bash
# 检查所有命名空间的特权容器
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.containers[*].securityContext.privileged}{"\n"}{end}' | grep true

# 预期输出:
# kube-system/kube-proxy-xxx: true
# monitoring/prometheus-operator-xxx: true

# 检查 hostPID 和 hostNetwork
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: hostPID={.spec.hostPID} hostNetwork={.spec.hostNetwork}{"\n"}{end}' | grep true

# 使用 trivy 扫描
trivy k8s --report summary cluster
```
---

### 9. 如何查看集群节点的 CPU 和内存使用率？

**你的回答:**
```
# 🟢 低风险：只读/信息收集，通常无副作用
```

**参考答案:**

```bash
# 查看节点资源使用
kubectl top nodes

# 预期输出:
# NAME            CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node-worker-1   800m         20%    4096Mi          25%
# node-worker-2   650m         16%    3584Mi          22%
# node-worker-3   720m         18%    3840Mi          24%

# 查看 Pod 资源使用
kubectl top pods -A --sort-by=memory | head -10

# 预期输出:
# NAMESPACE     NAME                         CPU(cores)   MEMORY(bytes)
# monitoring    prometheus-k8s-0             120m         2048Mi
# kube-system   coredns-66f5b8f7f5-abc12     10m          64Mi
# default       my-app-7d9f8b6c4-xyz12       50m          256Mi

# 注意: 需要 metrics-server 已安装
kubectl get pods -n kube-system -l k8s-app=metrics-server
```
---

### 10. 如何为 Namespace 启用 PSS baseline 级别？

**你的回答:**
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
```

**参考答案:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 启用 enforce + audit + warn 三个维度
kubectl label namespace <namespace> pod-security.kubernetes.io/enforce=baseline
kubectl label namespace <namespace> pod-security.kubernetes.io/audit=baseline
kubectl label namespace <namespace> pod-security.kubernetes.io/warn=baseline

# 验证标签
kubectl get namespace <namespace> --show-labels | grep pod-security

# 预期输出:
# <namespace>   Active   ...pod-security.kubernetes.io/audit=baseline,pod-security.kubernetes.io/enforce=baseline,pod-security.kubernetes.io/warn=baseline

# 测试: 创建不合规的 Pod
kubectl run test-privileged --image=nginx -n <namespace> --overrides='{"spec":{"containers":[{"name":"app","image":"nginx","securityContext":{"privileged":true}}]}}'
# 预期: 被 PSS 拒绝
```
---

## 三、场景分析 (每题 5 分，共 20 分)

### 11. 用户报告无法在某个 Namespace 创建 Pod，但在其他 Namespace 可以，排查思路？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点 - 完整排查流程:**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 尝试创建 Pod，查看错误信息
kubectl run test --image=nginx -n <problematic-namespace>
# 记录错误信息

# Step 2: 检查 ResourceQuota
kubectl describe resourcequota -n <problematic-namespace>
# 关注 Used vs Hard 是否已满

# Step 3: 检查 LimitRange
kubectl describe limitrange -n <problematic-namespace>
# 检查 min/max 限制是否合理

# Step 4: 检查 PSS
kubectl get namespace <problematic-namespace> --show-labels | grep pod-security
# PSS 可能拒绝了不合规的 Pod

# Step 5: 检查 RBAC
kubectl auth can-i create pods --as=<user> -n <problematic-namespace>
# 确认用户在该命名空间有权限

# Step 6: 查看事件
kubectl get events -n <problematic-namespace> --sort-by='.lastTimestamp'
```
**常见原因和解决方案:**

| 原因 | 症状 | 解决方案 |
|------|------|---------|
| ResourceQuota 已满 | "exceeded quota" | 清理资源或增加配额 |
| LimitRange 限制过严 | "must have resources" | 调整 LimitRange 的 min |
| PSS 拒绝 | "violates PodSecurity" | 修复 Pod 安全配置或调整 PSS 级别 |
| RBAC 无权限 | "forbidden" | 添加 RoleBinding |

---

### 12. 如何设计一个安全的多团队集群权限方案？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**

```yaml
# 多团队集群安全权限方案

# 1. 每个团队独立 Namespace
kubectl create namespace team-a
kubectl create namespace team-b

# 2. PSS 级别
kubectl label namespace team-a pod-security.kubernetes.io/enforce=baseline
kubectl label namespace team-b pod-security.kubernetes.io/enforce=restricted

# 3. RBAC: 团队只能操作自己的 Namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: team-developer
rules:
- apiGroups: ["", "apps", "batch"]
  resources: ["pods", "deployments", "services", "configmaps", "jobs"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-a-developer
  namespace: team-a
subjects:
- kind: Group
  name: team-a@company.com
roleRef:
  kind: ClusterRole
  name: team-developer
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-b-developer
  namespace: team-b
subjects:
- kind: Group
  name: team-b@company.com
roleRef:
  kind: ClusterRole
  name: team-developer

# 4. ResourceQuota 限制各团队资源
# 5. NetworkPolicy 隔离团队间网络
# 6. RAM 用户映射到 ACK 角色
```

---

### 13. 收到安全漏洞公告 (CVE)，处理流程是什么？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**

```
# 🟢 低风险：只读/信息收集，通常无副作用
CVE 处理标准流程:

Phase 1: 评估 (0-2h)
├── 确认漏洞影响范围 (哪些版本受影响)
├── 查看CVSS评分和严重等级
├── 检查集群版本: kubectl version
├── 检查运行镜像: kubectl get pods -A -o jsonpath='{...image}'
└── 判断: 集群是否受影响？

Phase 2: 缓解 (2-24h)
├── 启用 NetworkPolicy 限制攻击面
├── 禁用受影响的功能/参数
├── 加强监控告警规则
└── 通知相关团队

Phase 3: 修复 (24-72h)
├── 升级 K8S 版本到修复版本
├── 升级受影响的组件
├── 更新镜像到安全版本
└── 选择变更窗口执行

Phase 4: 验证
├── 确认漏洞已修复
├── 回归测试业务功能
├── 更新安全基线文档
└── 复盘总结
```
---

### 14. 如何通过审计日志排查"某个 Deployment 被误删"的问题？

**你的回答:**
```
(在此写下你的答案)
```

**参考要点:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 在 SLS 中查询删除操作
# 查询语句:
# verb: delete AND objectRef.resource: deployments AND objectRef.name: <deployment-name> AND objectRef.namespace: <namespace>

# Step 2: 关注关键字段
# - user.username: 操作者
# - sourceIPs: 来源 IP
# - requestReceivedTimestamp: 操作时间
# - responseStatus.code: 是否成功 (200=成功)
# - userAgent: 使用了什么工具 (kubectl/控制台/CI-CD)

# Step 3: 扩展查询 - 该操作者的其他操作
# user.username: "<username>" | select requestReceivedTimestamp, verb, objectRef.resource, objectRef.name from log order by requestReceivedTimestamp desc limit 50

# Step 4: 形成审计报告
# ============================================
# 审计报告: Deployment 误删追溯
# ============================================
# 资源: <namespace>/<deployment-name>
# 操作: DELETE
# 操作者: developer@1234567890
# 来源 IP: 192.168.1.100
# 时间: 2026-05-18 10:25:30 UTC
# 工具: kubectl/v1.30.1
# 结果: 成功 (200)
# ============================================

# Step 5: 预防措施
# - 收窄 RBAC 权限 (禁止 delete)
# - 配置审计告警 (删除 Deployment 时告警)
# - 启用 Admission Webhook (删除前二次确认)
```
---

## 四、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 概念理解 | __ | 20 |
| 命令实操 | __ | 10 |
| 场景分析 | __ | 20 |
| **总分** | __ | **50** |

### 评估标准

- **45-50 分**: 优秀，完全掌握本周内容
- **35-44 分**: 良好，基本掌握，部分细节需加强
- **25-34 分**: 及格，核心概念理解，需要复习
- **< 25 分**: 不及格，建议重新学习本周内容

---

## 五、薄弱点记录

```
1.


2.


3.

```

---

## 要点总结

- **RBAC 四种资源**: Role/ClusterRole 定义权限，RoleBinding/ClusterRoleBinding 绑定到用户
- **ACK 两层权限**: RAM (云平台) + RBAC (集群内)，两层独立但互补
- **审计四个级别**: None < Metadata < Request < RequestResponse
- **ResourceQuota** 控制总量，**LimitRange** 控制单个容器
- **PSS 三个级别**: Privileged → Baseline → Restricted
- **CVE 处理**: 评估 → 缓解 → 修复 → 验证

---

## 下周计划调整

```
需要加强的领域:

下周额外复习:
```

---

## 延伸阅读

- [RBAC 文档](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
- [Pod Security Standards](https://kubernetes.io/docs/concepts/security/pod-security-standards/)
- [审计日志](https://kubernetes.io/docs/tasks/debug/debug-cluster/audit/)
- [ACK 安全最佳实践](https://help.aliyun.com/document_detail/2627792.html)

## Related

- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
