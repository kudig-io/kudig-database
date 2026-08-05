---
title: ResourceQuota 与 LimitRange 设计与治理
description: 面向阿里云/专有云 K8s 的 ResourceQuota 与 LimitRange 治理方案，涵盖命名空间配额设计、默认限制、多租户治理与审计。
summary: 面向阿里云/专有云 K8s 的 ResourceQuota 与 LimitRange 治理方案，涵盖命名空间配额设计、默认限制、多租户治理与审计。
category: reliability
tags:
- k8s
- resourcequota
- limitrange
- governance
- multi-tenant
- cost-management
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 20min
intent_queries:
- ResourceQuota LimitRange 设计
- K8s 命名空间资源配额治理
- 阿里云 K8s 多租户资源限制
trigger_keywords:
- ResourceQuota
- LimitRange
- 资源配额
- 多租户
- 治理
prerequisites:
- kubectl-basics
- rbac-basics
- namespace-basics
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




# ResourceQuota 与 LimitRange 设计与治理

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，讲解 ResourceQuota 与 LimitRange 的设计原则、实施方法与多租户治理。

## 目录

1. [ResourceQuota 与 LimitRange 概述](#resourcequota-与-limitrange-概述)
2. [ResourceQuota 设计](#resourcequota-设计)
3. [LimitRange 设计](#limitrange-设计)
4. [多租户配额模型](#多租户配额模型)
5. [命名空间模板化](#命名空间模板化)
6. [配额监控与告警](#配额监控与告警)
7. [阿里云/专有云场景](#阿里云专有云场景)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. ResourceQuota 与 LimitRange 概述

### 1.1 作用对比

| 对象 | 作用域 | 控制内容 |
|:---|:---|:---|
| ResourceQuota | 命名空间 | 资源总量、对象数量 |
| LimitRange | 命名空间/Pod/容器 | 默认资源、最小/最大限制 |

### 1.2 为什么需要配额治理

- 防止单个命名空间耗尽集群资源
- 控制成本，按团队分配预算
- 保证集群整体稳定性
- 为容量规划提供数据基线

---

## 2. ResourceQuota 设计

### 2.1 生产命名空间配额示例

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: production-quota
  namespace: production
spec:
  hard:
    requests.cpu: "100"
    requests.memory: 200Gi
    limits.cpu: "200"
    limits.memory: 400Gi
    requests.storage: 10Ti
    persistentvolumeclaims: "100"
    pods: "500"
    services: "50"
    services.loadbalancers: "5"
    secrets: "200"
    configmaps: "200"
```

### 2.2 按环境设计配额

| 环境 | CPU requests | 内存 requests | Pod 数 | 说明 |
|:---|---:|---:|---:|:---|
| production | 100 | 200Gi | 500 | 核心生产 |
| staging | 50 | 100Gi | 300 | 预发 |
| development | 20 | 40Gi | 200 | 开发 |
| testing | 30 | 60Gi | 200 | 测试 |

### 2.3 查看配额使用

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 production 命名空间配额使用情况
kubectl describe resourcequota production-quota -n production
```
---

## 3. LimitRange 设计

### 3.1 容器默认资源限制

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: production-limits
  namespace: production
spec:
  limits:
    - type: Container
      default:
        cpu: "500m"
        memory: "512Mi"
      defaultRequest:
        cpu: "100m"
        memory: "128Mi"
      min:
        cpu: "50m"
        memory: "64Mi"
      max:
        cpu: "4"
        memory: "8Gi"
    - type: PersistentVolumeClaim
      min:
        storage: 1Gi
      max:
        storage: 2Ti
```

### 3.2 无资源限制的 Pod 将无法创建

LimitRange 配合 ResourceQuota 可实现：
- 每个容器必须有 requests/limits
- 资源请求不能超过命名空间配额
- PVC 大小在合理范围内

---

## 4. 多租户配额模型

### 4.1 分层配额

```
集群总资源
    │
    ├─ 部门 A (ResourceQuota)
    │    ├─ 团队 A1 (子命名空间配额)
    │    └─ 团队 A2 (子命名空间配额)
    │
    └─ 部门 B (ResourceQuota)
         ├─ 团队 B1
         └─ 团队 B2
```

### 4.2 按团队标签成本分摊

```yaml
# 命名空间标签示例
metadata:
  labels:
    cost-center: "cc1234"
    team: "platform"
    environment: "production"
```

---

## 5. 命名空间模板化

### 5.1 Namespace 模板

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: team-platform-prod
  labels:
    environment: production
    team: platform
    cost-center: cc1234
---
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-platform-prod-quota
  namespace: team-platform-prod
spec:
  hard:
    requests.cpu: "50"
    requests.memory: 100Gi
    limits.cpu: "100"
    limits.memory: 200Gi
    pods: "300"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: team-platform-prod-limits
  namespace: team-platform-prod
spec:
  limits:
    - type: Container
      default:
        cpu: "200m"
        memory: "256Mi"
      defaultRequest:
        cpu: "100m"
        memory: "128Mi"
      max:
        cpu: "2"
        memory: "4Gi"
```

### 5.2 GitOps 管理配额

将命名空间配额纳入 Git 仓库，通过 Argo CD 同步：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 kubectl apply 预览变更
diff -u <(kubectl get resourcequota team-platform-prod-quota -n team-platform-prod -o yaml) team-platform-prod-quota.yaml
```
---

## 6. 配额监控与告警

### 6.1 关键 Prometheus 指标

```bash
# CPU 配额使用率
kube_resourcequota{resource="requests.cpu",type="used"} / kube_resourcequota{resource="requests.cpu",type="hard"}

# 内存配额使用率
kube_resourcequota{resource="requests.memory",type="used"} / kube_resourcequota{resource="requests.memory",type="hard"}
```

### 6.2 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: quota-alerts
  namespace: monitoring
spec:
  groups:
    - name: quota.rules
      rules:
        - alert: ResourceQuotaCpuHigh
          expr: |
            kube_resourcequota{resource="requests.cpu",type="used"}
            / kube_resourcequota{resource="requests.cpu",type="hard"} > 0.85
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "命名空间 {{ $labels.namespace }} CPU 配额使用率超过 85%"
        - alert: ResourceQuotaMemoryHigh
          expr: |
            kube_resourcequota{resource="requests.memory",type="used"}
            / kube_resourcequota{resource="requests.memory",type="hard"} > 0.85
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "命名空间 {{ $labels.namespace }} 内存配额使用率超过 85%"
```

---

## 7. 阿里云/专有云场景

### 7.1 ACK 配额管理

阿里云 ACK 支持：
- 命名空间 ResourceQuota
- 节点池资源上限
- 企业版配额中心

### 7.2 专有云多租户治理

- 按 BU/部门划分命名空间
- 通过 ASO 审批命名空间创建
- 配额变更需走变更流程

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| 所有命名空间配置 ResourceQuota | 生产/测试/开发 | `kubectl get resourcequota -A` |
| 所有命名空间配置 LimitRange | 默认资源限制 | `kubectl get limitrange -A` |
| 配额使用率监控 | 85% 告警 | PrometheusRule |
| 配额变更审批 | GitOps 流程 | 仓库审计 |
| 成本标签 | team/cost-center | 命名空间标签 |
| 超配审计 | 月度检查 | 配额报告 |

---

## LimitRange 与 Pod 安全标准结合

LimitRange 不仅用于资源默认值，也可与 Pod Security Standards 结合，防止未设置资源限制的容器进入生产环境。

### 配额申请与审批流程

```
业务方提交配额申请
  → 平台工程师评估合理性
  → FinOps 审核成本影响
  → 审批通过后创建 ResourceQuota
  → 监控使用情况并定期 review
```

### 命名空间配额模板（团队级）

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
  namespace: team-a
spec:
  hard:
    requests.cpu: "50"
    requests.memory: "128Gi"
    limits.cpu: "100"
    limits.memory: "256Gi"
    pods: "300"
    services: "30"
    persistentvolumeclaims: "100"
    requests.storage: "5Ti"
```

### 常见问题

| 问题 | 可能原因 | 处理建议 |
|:---|:---|:---|
| Pod 无法创建 | 命名空间配额耗尽 | 申请扩容或清理无用资源 |
| LimitRange 导致调度失败 | max 限制过小 | 调整 maxAllowed |
| 资源碎片化 | request 设置过小 | 使用 VPA 推荐值 |
| 成本归属不清 | 缺少 label | 强制命名空间与团队 label |

## 多租户治理进阶

### 按环境分级

| 环境 | ResourceQuota 策略 | LimitRange 策略 |
|:---|:---|:---|
| 开发环境 | 宽松，鼓励实验 | 默认低 request，限制高 limit |
| 测试环境 | 按项目分配 | 默认中等配置 |
| 预发环境 | 与生产对齐 | 与生产一致 |
| 生产环境 | 严格，按业务 SLA | 强制设置 request/limit |

### 成本归因

通过 namespace 与 label 将资源成本归属到团队或项目：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看命名空间资源使用排名
kubectl top pods --all-namespaces | awk '{print $1}' | sort | uniq -c | sort -nr
```
结合 OpenCost 或 ACK 成本分析，生成月度成本报告。

## 典型工单场景与处理

**场景**：用户反馈创建 Pod 时报错 Forbidden: exceeded quota。

处理步骤：
1. 使用 `kubectl describe resourcequota -n <ns>` 查看剩余配额。
2. 确认是 CPU、内存、Pod 数还是 PVC 超限。
3. 协调业务方清理无用资源或申请配额扩容。
4. 如频繁触发，review 配额设置是否合理。

## ResourceQuota 与 LimitRange 的协同

ResourceQuota 控制命名空间资源总量，LimitRange 控制单个 Pod/PVC 的规格。两者结合可实现从宏观到微观的资源治理。

### 协同示例

| 治理目标 | ResourceQuota | LimitRange |
|:---|:---|:---|
| 防止单个命名空间耗尽集群 | 限制 CPU/内存/Pod 总数 | 不直接控制 |
| 防止单个 Pod 占用过多资源 | 不直接控制 | 设置 max CPU/内存 |
| 避免未设置 limit 的 Pod | 不直接控制 | 设置默认值 |
| 控制存储成本 | 限制 PVC 总数与总容量 | 设置单个 PVC 最大容量 |

### 配额告警面板

在 Grafana 中创建面板展示各命名空间配额使用率：

```promql
kube_resourcequota{resource="requests.cpu", type="used"} /
kube_resourcequota{resource="requests.cpu", type="hard"}
```

### 配额治理最佳实践

1. 为每个命名空间设置合理的初始配额，避免过松或过紧。
2. 建立配额申请流程，所有扩容需经过审批。
3. 每月 review 配额使用情况，回收闲置资源。
4. 将配额使用纳入团队考核与成本分摊。

## ResourceQuota 错误排查

当 Pod 创建失败并提示 `exceeded quota` 时，按以下步骤排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看命名空间下所有 ResourceQuota
kubectl get resourcequota -n <namespace>

# 2. 查看具体配额使用情况
kubectl describe resourcequota <quota-name> -n <namespace>

# 3. 查看当前命名空间资源使用总量
kubectl top pods -n <namespace>

# 4. 检查 LimitRange 是否导致 Pod 规格超限
kubectl describe limitrange -n <namespace>
```
### 配额治理案例

某团队频繁触发 CPU 配额告警，经分析发现大量测试 Pod 未设置合理 request。通过 LimitRange 设置默认 request 与 max limit，并在 CI 中增加配额检查，最终将 CPU 请求降低 35%。

## Related

- [[domain-09-reliability-engineering/容量规划/01-capacity-planning-framework.md|容量规划框架]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-11-production-operations/02-governance/05-resource-quota-management|资源配额管理]]

## See Also

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/10-quota-limitrange-troubleshooting|ResourceQuota/LimitRange 故障诊断]]
- [[domain-08-release-change-management/GitOps/01-argo-cd-enterprise-gitops.md|Argo CD 企业级 GitOps]]


<!-- risk-assessed -->
