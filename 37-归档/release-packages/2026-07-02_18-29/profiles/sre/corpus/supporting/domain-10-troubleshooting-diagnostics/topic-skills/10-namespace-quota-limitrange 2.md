---
title: Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure Diagnosis
  & Remediation
description: '- 运维工程师'
summary: 'Namespace、ResourceQuota 和 LimitRange 是 [[Kubernetes|Kubernetes]] 多租户资源隔离的核心机制。ResourceQuota 限制 Namespace 级别的资源总量，LimitRange 限制单个 Pod/容器的资源范围。'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- apiserver
- prometheus
- statefulset
- daemonset
- job
- cronjob
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 35min
intent_queries:
- Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure Diagnosis
  & Remediation 是什么
- 如何 Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure Diagnosis
  & Remediation
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure Diagnosis
  & Remediation 故障排查
- Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure Diagnosis
  & Remediation 排障步骤
trigger_keywords:
- Namespace
- Quota
- LimitRange
- 故障诊断与修复
- Namespace
- Quota
- LimitRange
- Failure
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
skill_id: SKILL-24_NAMESPACE_QUOTA_LIMITRANGE-001
skill_name: Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure
  Diagnosis & Remediation
version: 1.0.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
skill_id: "SKILL-CONFIG-002"
skill_name: "Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure Diagnosis & Remediation"
version: "1.0"
category: "configuration"
severity_range: "P0-P2"
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
estimated_resolution_time: "5-30min"
risk_level: "medium"
agent_execution_mode: "L2-semi-auto"
trigger_keywords:
  - "quota"
  - "limitrange"
  - "exceeded quota"
  - "resourcequota"
  - "namespace"
  - "配额"
  - "资源配额"
  - "超出配额"
  - "limits exceeded"
  - "minimum requirement"
  - "default request"
  - "terminating namespace"
trigger_events:
  - "FailedCreate"
  - "Forbidden"
  - "OutOfResource"
  - "FailedBinding"
trigger_metrics:
  - 'kube_resourcequota_used_hard_ratio > 0.95'
  - 'kube_resourcequota{type="hard"}'
  - 'kube_namespace_status_phase{phase="Terminating"}'
difficulty: "intermediate"
reading_level: "intermediate"
audience:
  - SRE
  - 运维工程师
  - 平台工程师
estimated_read_time: "10min"
prerequisites:
  - "domain-10-troubleshooting-diagnostics"
  - "kubectl-basics"
  - "namespace-concepts"
related_skills:
  - "SKILL-POD-002"
  - "SKILL-NODE-002"
  - "SKILL-WORK-004"
  - "SKILL-CP-001"
fta_refs:
  - "domain-10-troubleshooting-diagnostics/topic-fta/list/resource-quota-fta.md"
knowledge_refs:
  - "domain-10-troubleshooting-diagnostics/24-quota-limitrange-troubleshooting.md"
  - "domain-01-cluster-fundamentals/"
cross_refs:
  - type: "fta"
    path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/resource-quota-fta.md"
    label: "Quota/LimitRange 故障树分析"
  - type: "domain"
    path: "../domain-10-troubleshooting-diagnostics/24-quota-limitrange-troubleshooting.md"
    label: "Quota/LimitRange 深度排查"
  - type: "[[SKILL|skill]]"
    path: "./23-job-cronjob-failure.md"
    label: "Job/CronJob 故障诊断"
authors:
  - name: KUDIG Team
    role: contributor

tier: peripheral---

# Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure Diagnosis & Remediation

Namespace、ResourceQuota 和 LimitRange 是 [[Kubernetes|Kubernetes]] 多租户资源隔离的核心机制。ResourceQuota 限制 Namespace 级别的资源总量，LimitRange 限制单个 Pod/容器的资源范围。这三者的配置错误或冲突会导致 Pod 创建被拒、资源分配不合理、Namespace 无法正常删除等问题，直接影响应用部署和集群资源管理。

本 Skill 覆盖 ResourceQuota 超限、LimitRange 配置冲突、Namespace Terminating 卡死、配额计算错误、默认值缺失等 10 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| Pod 创建失败，提示 exceeded quota | `kubectl describe pod` 或 Events | 0.95 |
| Pod 创建失败，提示 limits 超出范围 | `kubectl get events --field-selector reason=FailedCreate` | 0.90 |
| Namespace 长期处于 Terminating 状态 | `kubectl get namespace <ns>` | 0.85 |
| 资源使用量显示异常（已删 Pod 仍计占用） | `kubectl get resourcequota -n <ns>` | 0.85 |
| 未设置 resources 的 Pod 被赋予不合理默认值 | `kubectl get pod -o jsonpath='{.spec.containers[0].resources}'` | 0.80 |
| LimitRange 配置导致最小值大于最大值 | `kubectl describe limitrange` | 0.90 |

**排除条件**: 节点资源不足导致调度失败 → SKILL-POD-002; 节点 DiskPressure/MemoryPressure → SKILL-NODE-002; API Server 不可用 → SKILL-CP-001

## 快速分级（2 分钟内完成）

```
影响范围 + 症状类型
├── 生产 Namespace 无法创建 Pod（部署阻断）─────→ P0（立即处理）
├── 多个 Namespace 同时配额超限──────────────────→ P0（30min 内修复）
├── LimitRange 冲突导致 Pod 批量创建失败────────→ P1（1h 内修复）
├── Namespace Terminating 卡死──────────────────→ P1（2h 内修复）
├── 单个非关键 Namespace 配额告警───────────────→ P2（4h 内处理）
└── LimitRange 默认值不合理（性能影响）──────────→ P2（下次维护窗口）
```

**立即升级条件**：
- 核心生产 Namespace（如 kube-system、监控、网关）Pod 创建被阻断
- Namespace Terminating 卡死导致同名 Namespace 无法重建
- 集群级配额错误导致多个 Namespace 同时受影响

## 执行流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
工单/告警触发
    │
    ▼
┌──────────────┐    Step: D1.1-D1.5
│ Phase 1      │    内容: kubectl 快速检查（只读，零风险）
│ 快速检查      │
└──────┬───────┘
       │ 无法确认根因
       ▼
┌──────────────┐    Step: D2.1-D2.6
│ Phase 2      │    内容: 深度分析（只读，零风险）
│ 深度检查      │
└──────┬───────┘
       │ 需主动探测/修复
       ▼
┌──────────────┐    Step: D3.1-D3.3
│ Phase 3      │    内容: 主动探测（低风险，可能需审批）
│ 主动探测      │
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    RC-001~010
│ 根因匹配      │
└──────┬───────┘
       │
       ▼
┌──────────────┐    REM-001~008
│ 修复操作      │    风险: LOW → MEDIUM → HIGH → CRITICAL
└──────┬───────┘
       │
       ▼
┌──────────────┐    V1~V6
│ 验证确认      │
└──────────────┘
```
## 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Pod 创建失败，Event 提示 exceeded quota | `kubectl get events --field-selector reason=FailedCreate` | 0.95 | 节点资源不足 |
| S2 | Pod 创建失败，提示超出 LimitRange 限制 | `kubectl describe pod` 查看 Event | 0.90 | 节点资源不足 |
| S3 | Namespace 处于 Terminating 状态超 10 分钟 | `kubectl get namespace <ns>` | 0.85 | 正常删除流程 |
| S4 | ResourceQuota used 大于实际运行 Pod 数量 | `kubectl get resourcequota -n <ns>` | 0.85 | 存在 Terminating Pod |
| S5 | 未设置 resources 的 Pod 获得异常默认值 | `kubectl get pod -o jsonpath='{.spec.containers[*].resources}'` | 0.80 | 手动设置了 resources |
| S6 | LimitRange 显示 min > max 配置错误 | `kubectl describe limitrange -n <ns>` | 0.90 | 无 |
| S7 | 多个 ResourceQuota 在同一 Namespace 冲突 | `kubectl get resourcequota -n <ns>` | 0.80 | 无 |
| S8 | Namespace 无 ResourceQuota 但集群要求配额 | `kubectl get resourcequota -n <ns>` | 0.75 | 集群未启用准入控制 |

### 2.2 工单关键词映射

- "无法创建 Pod，提示 exceeded quota"
- "Pod 创建失败，limits 超出范围"
- "Namespace 删不掉，一直 Terminating"
- "ResourceQuota 使用量显示不对"
- "Pod 没设 resources，但被分配了默认值"
- "LimitRange 配置冲突"
- "Namespace 没有配额限制"
- "配额计算有问题，已删除的 Pod 还算在内"

### 2.3 排除标准

- 节点资源不足导致 Pending → 使用 SKILL-POD-002
- 节点 DiskPressure/MemoryPressure 导致驱逐 → 使用 SKILL-NODE-002
- API Server 不可用 → 使用 SKILL-CP-001
- RBAC 权限不足导致 Forbidden → 使用 SKILL-SEC-001
- 应用业务逻辑导致的 Pod 失败 → 使用 SKILL-POD-001

## 快速分级（2 分钟内完成）

### 3.1 影响评估

**Step T1**: 检查受影响 Namespace 列表及规模
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 ResourceQuota 使用率
kubectl get resourcequota --all-namespaces -o jsonpath='{
  range .items[*]
}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{.status.used.pods}{"/"}{.status.hard.pods}{"\t"}{.status.used."requests.cpu"}{"/"}{.status.hard."requests.cpu"}{"\n"}{end}'

# 查看 Terminating 的 Namespace
kubectl get namespaces --field-selector status.phase=Terminating

# 统计受影响的 Pod 创建事件
kubectl get events --all-namespaces --field-selector reason=FailedCreate -o jsonpath='{
  range .items[*]
}{.metadata.namespace}{"\t"}{.message}{"\n"}{end}' | grep -i "quota|limit"
```
> **判断规则**: 
> - 如果存在 Terminating Namespace 超过 10 分钟 → 影响范围为 Namespace 生命周期管理
> - 如果 FailedCreate 事件涉及 >3 个 Namespace → 影响范围为集群级
> - 如果仅单个 Namespace 且为非生产 → 影响范围有限

**Step T2**: 确定是否为核心系统受影响
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 kube-system 等核心 Namespace 的配额状态
kubectl get resourcequota -n kube-system
kubectl get events -n kube-system --field-selector reason=FailedCreate

# 检查是否有 Pod 因配额被驱逐
kubectl get events --all-namespaces --field-selector reason=Evicted | grep -i quota
```
> **判断规则**: 如果 kube-system、[[Ingress|ingress]]、monitoring 等核心 Namespace 受影响 → 升级为 P0

**Step T3**: 检查 LimitRange 冲突范围
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 LimitRange
kubectl get limitrange --all-namespaces

# 检查是否有 Namespace 同时配置了冲突的 LimitRange
for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  count=$(kubectl get limitrange -n $ns --no-headers 2>/dev/null | wc -l)
  if [ $count -gt 1 ]; then
    echo "Namespace $ns has $count LimitRange objects"
  fi
done
```
### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| 核心 Namespace（kube-system/ingress/monitoring）Pod 创建被阻断 | P0 | 直接影响集群核心功能 |
| 多个生产 Namespace 同时配额超限 | P0 | 影响面扩大 |
| Namespace Terminating 卡死且需要重建同名 Namespace | P1 | 影响应用重新部署 |
| 单个生产 Namespace 配额超限 | P1 | 影响该 Namespace 内所有新部署 |
| LimitRange 配置导致部分 Pod 无法创建 | P2 | 影响特定规格的工作负载 |
| 配额默认值不合理，存在资源浪费 | P2 | 性能/成本影响 |
| 非生产 Namespace 配额告警 | P3 | 预防性处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工**：
- kube-system Namespace 配额超限导致核心组件无法扩容
- 多个 Namespace 同时出现配额错误，怀疑集群级准入控制器问题
- Namespace Terminating 卡死导致业务连续性受影响且需要紧急重建
- ResourceQuota 配置变更后导致大规模 Pod 驱逐

## 诊断工作流

### Phase 1: 快速检查（只读，零风险）

**Step D1.1**: 检查 ResourceQuota 基本状态
- **命令**:
  ```bash
  kubectl get resourcequota --all-namespaces
  kubectl get resourcequota -n <namespace> -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: 显示 `used` 和 `hard` 字段
- **判断规则**:
  - 如果 `used` 某项 >= `hard` 对应项 → RC-001（配额超限）
  - 如果 `used` 数值明显大于实际运行 Pod 数 → RC-002（配额计算错误）
  - 如果 Namespace 无 ResourceQuota 但 Pod 创建失败 → 继续 D1.2
- **版本差异**: **[v1.28+]** 支持 `services.nodeports` 和 `services.loadbalancers` 独立配额

**Step D1.2**: 检查 LimitRange 配置
- **命令**:
  ```bash
  kubectl get limitrange -n <namespace>
  kubectl describe limitrange -n <namespace>
  kubectl get limitrange <name> -n <namespace> -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: 显示 `min`、`max`、`default`、`defaultRequest` 字段
- **判断规则**:
  - 如果 `min` > `max` → RC-003（配置冲突）
  - 如果 `default` 不在 `min`~`max` 范围内 → RC-004（默认值越界）
  - 如果无 LimitRange 且 Pod 未设置 resources → RC-005（默认值缺失）

**Step D1.3**: 检查 Namespace 状态
- **命令**:
  ```bash
  kubectl get namespace <namespace> -o yaml
  kubectl get namespace <namespace> -o jsonpath='{.status.phase}'
  ```
- **超时**: 10s
- **预期输出模式**: `Active` 或 `Terminating`
- **判断规则**:
  - 如果状态为 `Terminating` 且持续超过 10 分钟 → RC-010（Namespace 卡死）
  - 如果状态为 `Terminating` 但刚删除不久 → 正常流程，继续监控

**Step D1.4**: 检查 FailedCreate 事件详情
- **命令**:
  ```bash
  kubectl get events -n <namespace> --field-selector reason=FailedCreate --sort-by='.lastTimestamp'
  kubectl get events -n <namespace> --field-selector reason=FailedCreate -o jsonpath='{
    range .items[*]
  }{.message}{"\n"}{end}' | grep -iE "quota|limit|exceed"
  ```
- **超时**: 10s
- **预期输出模式**: 包含 "exceeded quota" 或 "minimum requirement" 等关键字
- **判断规则**:
  - 消息包含 "exceeded quota" → RC-001
  - 消息包含 "minimum cpu|memory requirement" → RC-003
  - 消息包含 "must be <= limit" → RC-008

**Step D1.5**: 检查同一 Namespace 中多个配额对象
- **命令**:
  ```bash
  kubectl get resourcequota -n <namespace> --no-headers | wc -l
  kubectl get resourcequota -n <namespace> -o jsonpath='{
    range .items[*]
  }{.metadata.name}{"\t"}{range $k,$v := .spec.hard}{$k}{":"}{$v}{","}{end}{"\n"}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  - 如果存在多个 ResourceQuota 且硬限制相互矛盾 → RC-006（多配额冲突）

### Phase 2: 深度检查（只读，零风险）

**Step D2.1**: 分析 ResourceQuota 使用量与实际资源的差异
- **命令**:
  ```bash
  # 获取 ResourceQuota 统计的 Pod 数
  QUOTA_PODS=$(kubectl get resourcequota -n <namespace> -o jsonpath='{.items[0].status.used.pods}')
  
  # 获取实际 Running Pod 数
  ACTUAL_PODS=$(kubectl get pods -n <namespace> --field-selector status.phase=Running --no-headers | wc -l)
  
  # 获取 Terminating Pod 数
  TERM_PODS=$(kubectl get pods -n <namespace> --field-selector status.phase!=Running,status.phase!=Pending,status.phase!=Succeeded --no-headers 2>/dev/null | wc -l)
  
  echo "Quota counted pods: $QUOTA_PODS"
  echo "Actual Running pods: $ACTUAL_PODS"
  echo "Non-Running pods: $TERM_PODS"
  ```
- **超时**: 15s
- **判断规则**:
  - 如果 QUOTA_PODS > ACTUAL_PODS + TERM_PODS → RC-002（计算错误）
  - 如果 QUOTA_PODS == ACTUAL_PODS + TERM_PODS 但 TERM_PODS > 0 → 配额正常，等待 Pod 完全终止

**Step D2.2**: 检查 LimitRange 详细约束
- **命令**:
  ```bash
  kubectl get limitrange -n <namespace> -o jsonpath='{
    range .items[*].spec.limits[*]
  }{"Type: "}{.type}{"\n"}{"Min: "}{.min}{"\n"}{"Max: "}{.max}{"\n"}{"Default: "}{.default}{"\n"}{"DefaultRequest: "}{.defaultRequest}{"\n"}{"MaxLimitRequestRatio: "}{.maxLimitRequestRatio}{"\n---\n"}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  - 检查 `min.cpu` > `max.cpu` 或 `min.memory` > `max.memory` → RC-003
  - 检查 `default.cpu` > `max.cpu` 或 `default.cpu` < `min.cpu` → RC-004
  - 检查 `maxLimitRequestRatio` 是否过于严格 → RC-008

**Step D2.3**: 检查 Pod 的资源配置与 LimitRange 匹配情况
- **命令**:
  ```bash
  kubectl get pods -n <namespace> -o jsonpath='{
    range .items[*]
  }{.metadata.name}{"\t"}{.spec.containers[0].resources.requests}{"\t"}{.spec.containers[0].resources.limits}{"\n"}{end}'
  ```
- **超时**: 15s
- **判断规则**:
  - 如果有 Pod 的 `limits` 与 `requests` 比例超过 LimitRange 的 `maxLimitRequestRatio` → RC-008
  - 如果有 Pod 未设置 `resources` 且 LimitRange 也未配置 default → RC-005

**Step D2.4**: 检查 Namespace Terminating 的阻塞原因
- **命令**:
  ```bash
  # 查看 Namespace 的 finalizers
  kubectl get namespace <namespace> -o jsonpath='{.metadata.finalizers}'
  
  # 查看 Namespace 中残留的资源
  kubectl api-resources --verbs=list --namespaced -o name | xargs -n1 -I{} sh -c 'echo "--- {} ---"; kubectl get {} -n <namespace> --ignore-not-found'
  
  # 查看 Namespace 的 Event
  kubectl get events --all-namespaces --field-selector involvedObject.name=<namespace>
  ```
- **超时**: 30s
- **判断规则**:
  - 如果存在无法删除的自定义资源（CRD）→ 需手动清理 CR
  - 如果 finalizer 包含云厂商特定值 → 需云厂商控制台配合删除
  - 如果 APIService 不可用 → 需修复 APIService

**Step D2.5**: 检查集群级准入控制配置
- **命令**:
  ```bash
  # 检查是否启用了 ResourceQuota 准入控制器
  kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{
    .items[0].spec.containers[0].command[*]
  }' | grep -o 'ResourceQuota'
  
  # 检查 LimitRanger 准入控制器
  kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{
    .items[0].spec.containers[0].command[*]
  }' | grep -o 'LimitRanger'
  ```
- **超时**: 10s
- **判断规则**:
  - 如果未启用 ResourceQuota 但用户期望有配额限制 → 需配置准入控制器
  - 如果未启用 LimitRanger 但配置了 LimitRange → LimitRange 不会生效

**Step D2.6**: 检查配额使用的 Prometheus 指标（如可访问）
- **命令**:
  ```bash
  # 查询配额使用率
  curl -s 'http://prometheus:9090/api/v1/query?query=kube_resourcequota_used_hard_ratio>0.8' | jq '.data.result[] | {namespace: .metric.namespace, resource: .metric.resource, value: .value[1]}'
  
  # 查询接近超限的配额
  curl -s 'http://prometheus:9090/api/v1/query?query=kube_resourcequota{type="hard"}' | jq '.data.result[] | {namespace: .metric.namespace, resource: .metric.resource, hard: .value[1]}'
  ```
- **超时**: 15s
- **判断规则**: 指标显示使用率 >95% 但 kubectl 显示未超限 → 可能存在 metrics-server 与 API Server 数据不一致

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 测试 Pod 创建以确认具体限制条件
- **目的**: 确定是哪个具体配额项或 LimitRange 约束导致创建失败
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 测试最小资源 Pod 创建
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: Pod
  metadata:
    name: quota-test-minimal
    namespace: <namespace>
  spec:
    containers:
    - name: test
      image: busybox:1.36
      command: ["sh", "-c", "sleep 10"]
      resources:
        requests:
          cpu: "1m"
          memory: "4Mi"
        limits:
          cpu: "10m"
          memory: "16Mi"
  EOF
  
  # 查看创建结果
  kubectl get pod quota-test-minimal -n <namespace>
  kubectl describe pod quota-test-minimal -n <namespace> | grep -A5 "Events"
  ```
- **超时**: 30s
- **风险级别**: 🟢 低风险
- **判断规则**:
  - 如果最小 Pod 也无法创建 → 说明是对象数量配额（如 pods）已超限
  - 如果提示超出 LimitRange 最小值 → LimitRange min 配置过高
- **回滚**: `kubectl delete pod quota-test-minimal -n <namespace>`

**Step D3.2**: 验证 LimitRange 默认值注入行为
- **目的**: 确认 LimitRange 是否按预期注入默认资源
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 创建不指定 resources 的测试 Pod
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: Pod
  metadata:
    name: quota-test-defaults
    namespace: <namespace>
  spec:
    containers:
    - name: test
      image: busybox:1.36
      command: ["sh", "-c", "sleep 10"]
  EOF
  
  # 检查注入的资源值
  kubectl get pod quota-test-defaults -n <namespace> -o jsonpath='{
    .spec.containers[0].resources
  }'
  ```
- **超时**: 30s
- **风险级别**: 🟢 低风险
- **判断规则**:
  - 如果未注入任何默认值 → LimitRange 未配置 default 或准入控制器未启用
  - 如果注入的值不合理（过大或过小）→ RC-004
- **回滚**: `kubectl delete pod quota-test-defaults -n <namespace>`

**Step D3.3**: 测试 Namespace 资源释放情况
- **目的**: 确认删除资源后配额是否正确释放
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 记录当前配额使用
  BEFORE=$(kubectl get resourcequota -n <namespace> -o jsonpath='{.items[0].status.used.pods}')
  
  # 删除一个 Pod
  kubectl delete pod <pod-name> -n <namespace>
  
  # 等待并检查配额释放
  sleep 5
  AFTER=$(kubectl get resourcequota -n <namespace> -o jsonpath='{.items[0].status.used.pods}')
  echo "Before: $BEFORE, After: $AFTER"
  ```
- **超时**: 30s
- **风险级别**: 🟡 中风险（删除生产 Pod）
- **审批提示**: "建议删除 Pod <pod-name> 以测试配额释放，是否批准？"
- **判断规则**: 如果删除后配额未减少 → RC-002（配额计算错误）

## 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | ResourceQuota 硬限制配置过低 | 高 | D1.1: used >= hard; D1.4: "exceeded quota" | FTA-QUOTA-001 |
| RC-002 | 配额计算错误，已终止资源仍计入 used | 中 | D2.1: quota pods > actual + terminating; D3.3: 删除后未释放 | FTA-QUOTA-002 |
| RC-003 | LimitRange min > max 配置冲突 | 中 | D1.2: min.cpu > max.cpu; D1.4: "minimum requirement" | FTA-LIMIT-001 |
| RC-004 | LimitRange 默认值超出 min/max 范围 | 中 | D1.2: default > max 或 default < min; D3.2: 注入值异常 | FTA-LIMIT-002 |
| RC-005 | LimitRange 未配置默认值，导致 QoS 为 BestEffort | 中 | D1.2: 无 default 配置; D3.2: 未注入 resources | FTA-LIMIT-003 |
| RC-006 | 同一 Namespace 多个 ResourceQuota 规则冲突 | 低 | D1.5: 多个 ResourceQuota 对象 | FTA-QUOTA-003 |
| RC-007 | 配额仅配置 requests.* 但 Pod 设置了 limits.* | 中 | D2.2: 硬限制无 limits.cpu/memory; D1.4: 创建失败 | FTA-QUOTA-004 |
| RC-008 | maxLimitRequestRatio 过于严格 | 低 | D2.2: maxLimitRequestRatio < 实际比例; D2.3: Pod limits/requests 比例超限 | FTA-LIMIT-004 |
| RC-009 | Namespace 无 ResourceQuota 但集群级策略要求 | 低 | D2.5: 准入控制器启用; D1.1: 无 ResourceQuota 对象 | FTA-QUOTA-005 |
| RC-010 | Namespace Terminating 卡死，finalizer 或资源未清理 | 中 | D1.3: phase=Terminating; D2.4: 存在残留资源或 finalizer | FTA-NS-001 |

## 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 调整 ResourceQuota 硬限制
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 确认当前使用量和硬限制
  kubectl get resourcequota <quota-name> -n <namespace> -o jsonpath='{
    "used": .status.used,
    "hard": .status.hard
  }'
  
  # 确认增加后的值不会导致节点资源不足
  kubectl top nodes
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方式 1: Patch 增加特定资源配额
  kubectl patch resourcequota <quota-name> -n <namespace> --type merge -p '{
    "spec": {
      "hard": {
        "pods": "<new-value>",
        "requests.cpu": "<new-value>",
        "requests.memory": "<new-value>",
        "limits.cpu": "<new-value>",
        "limits.memory": "<new-value>"
      }
    }
  }'
  
  # 方式 2: 编辑完整配置
  kubectl edit resourcequota <quota-name> -n <namespace>
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 验证新配额已生效
  kubectl get resourcequota <quota-name> -n <namespace>
  
  # 尝试创建之前失败的 Pod
  kubectl apply -f <previously-failed-pod.yaml>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch resourcequota <quota-name> -n <namespace> --type merge -p '{
    "spec": {
      "hard": <original-values>
    }
  }'
  ```

#### REM-002: 清理已终止资源释放配额
- **适用根因**: RC-002
- **前置检查**:
  ```bash
  # 查看 Terminating 或 Failed 的 Pod
  kubectl get pods -n <namespace> --field-selector status.phase!=Running,status.phase!=Pending,status.phase!=Succeeded
  
  # 查看 Completed 的 Job
  kubectl get jobs -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 强制删除卡住的 Terminating Pod
  kubectl delete pods -n <namespace> --field-selector status.phase=Unknown --force --grace-period=0
  
  # 删除已完成的 Job（及其 Pod）
  kubectl delete jobs -n <namespace> --field-selector status.succeeded=1
  
  # 清理 Evicted Pod
  kubectl delete pods -n <namespace> --field-selector status.phase=Failed
  ```
- **后置验证**:
  ```bash
  # 验证配额已释放
  kubectl get resourcequota -n <namespace>
  
  # 确认 used 值下降
  kubectl get resourcequota <quota-name> -n <namespace> -o jsonpath='{
    .status.used.pods}{"/"}{.status.hard.pods}
  }'
  ```
- **回滚命令**: 无法回滚已删除资源，需确保删除前已备份必要日志

#### REM-003: 修复 LimitRange min/max 冲突
- **适用根因**: RC-003, RC-004
- **前置检查**:
  ```bash
  # 备份当前 LimitRange
  kubectl get limitrange <limitrange-name> -n <namespace> -o yaml > /tmp/backup-limitrange.yaml
  
  # 确认冲突项
  kubectl get limitrange <limitrange-name> -n <namespace> -o jsonpath='{
    range .spec.limits[*]
  }{ .type }{" min="}{.min}{" max="}{.max}{" default="}{.default}{"\n"}{end}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方式 1: Patch 修复冲突值
  kubectl patch limitrange <limitrange-name> -n <namespace> --type merge -p '{
    "spec": {
      "limits": [
        {
          "type": "Container",
          "min": {
            "cpu": "10m",
            "memory": "32Mi"
          },
          "max": {
            "cpu": "4",
            "memory": "8Gi"
          },
          "default": {
            "cpu": "100m",
            "memory": "256Mi"
          },
          "defaultRequest": {
            "cpu": "50m",
            "memory": "128Mi"
          }
        }
      ]
    }
  }'
  
  # 方式 2: 应用修复后的 YAML
  kubectl apply -f corrected-limitrange.yaml
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 确认 LimitRange 已更新
  kubectl describe limitrange -n <namespace>
  
  # 测试 Pod 创建
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: Pod
  metadata:
    name: limitrange-verify
    namespace: <namespace>
  spec:
    containers:
    - name: test
      image: busybox:1.36
      command: ["sh", "-c", "sleep 5"]
      resources:
        requests:
          cpu: "50m"
          memory: "128Mi"
        limits:
          cpu: "100m"
          memory: "256Mi"
  EOF
  kubectl wait --for=condition=Ready pod/limitrange-verify -n <namespace> --timeout=30s
  kubectl delete pod limitrange-verify -n <namespace>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f /tmp/backup-limitrange.yaml
  ```

#### REM-004: 配置 LimitRange 默认资源值
- **适用根因**: RC-005
- **前置检查**:
  ```bash
  # 确认当前无 default 配置
  kubectl get limitrange -n <namespace> -o jsonpath='{
    .items[*].spec.limits[?(@.type=="Container")].default}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: LimitRange
  metadata:
    name: default-resources
    namespace: <namespace>
  spec:
    limits:
    - type: Container
      default:
        cpu: "200m"
        memory: "256Mi"
      defaultRequest:
        cpu: "50m"
        memory: "128Mi"
      min:
        cpu: "10m"
        memory: "32Mi"
      max:
        cpu: "4"
        memory: "8Gi"
  EOF
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 创建测试 Pod 验证默认值注入
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: Pod
  metadata:
    name: default-test
    namespace: <namespace>
  spec:
    containers:
    - name: test
      image: busybox:1.36
      command: ["sh", "-c", "sleep 5"]
  EOF
  
  kubectl get pod default-test -n <namespace> -o jsonpath='{
    .spec.containers[0].resources
  }'
  kubectl delete pod default-test -n <namespace>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete limitrange default-resources -n <namespace>
  ```

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-005: 合并或删除冲突的 ResourceQuota
- **适用根因**: RC-006
- **影响说明**: 合并 ResourceQuota 可能影响资源分配策略，需确认合并后的配额值
- **审批提示**: "建议合并 Namespace <namespace> 中的多个 ResourceQuota，是否批准？"
- **前置检查**:
  ```bash
  # 列出所有 ResourceQuota 及其限制
  kubectl get resourcequota -n <namespace> -o jsonpath='{
    range .items[*]
  }{ "Name: " }{ .metadata.name }{ "\nHard: " }{ .spec.hard }{ "\n---\n" }{ end }'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 备份原有配额
  kubectl get resourcequota -n <namespace> -o yaml > /tmp/backup-quotas.yaml
  
  # 创建合并后的统一配额
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: ResourceQuota
  metadata:
    name: unified-quota
    namespace: <namespace>
  spec:
    hard:
      pods: "100"
      requests.cpu: "20"
      requests.memory: "40Gi"
      limits.cpu: "40"
      limits.memory: "80Gi"
      persistentvolumeclaims: "20"
      services: "20"
  EOF
  
  # 删除旧的冲突配额
  kubectl delete resourcequota <old-quota-1> <old-quota-2> -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get resourcequota -n <namespace>
  kubectl describe resourcequota unified-quota -n <namespace>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete resourcequota unified-quota -n <namespace>
  kubectl apply -f /tmp/backup-quotas.yaml
  ```

#### REM-006: 补充 ResourceQuota 的 limits 限制项
- **适用根因**: RC-007
- **影响说明**: 添加 limits 限制后，已运行 Pod 不受影响，但新 Pod 必须遵守
- **审批提示**: "建议在 ResourceQuota 中增加 limits.cpu 和 limits.memory，是否批准？"
- **前置检查**:
  ```bash
  # 确认当前硬限制中缺少 limits.*
  kubectl get resourcequota <quota-name> -n <namespace> -o jsonpath='{.spec.hard}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch resourcequota <quota-name> -n <namespace> --type merge -p '{
    "spec": {
      "hard": {
        "limits.cpu": "40",
        "limits.memory": "80Gi"
      }
    }
  }'
  ```
- **后置验证**:
  ```bash
  kubectl get resourcequota <quota-name> -n <namespace> -o jsonpath='{.spec.hard}'
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch resourcequota <quota-name> -n <namespace> --type json -p='[
    {"op": "remove", "path": "/spec/hard/limits.cpu"},
    {"op": "remove", "path": "/spec/hard/limits.memory"}
  ]'
  ```

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-007: 调整 maxLimitRequestRatio
- **适用根因**: RC-008
- **影响说明**: 放宽 maxLimitRequestRatio 可能导致资源超售风险增加
- **操作步骤**:
  1. 备份当前 LimitRange 配置
  2. 分析当前工作负载的 limits/requests 比例分布
  3. 计算合理的 maxLimitRequestRatio（建议 CPU <= 10, Memory <= 4）
  4. 更新 LimitRange
  5. 验证关键工作负载不受影响
- **安全检查**:
  ```bash
  # 检查当前所有 Pod 的比例分布
  kubectl get pods -n <namespace> -o jsonpath='{
    range .items[*]
  }{ .metadata.name }{ "\t" }
  { .spec.containers[0].resources.limits.cpu }{ "/" }
  { .spec.containers[0].resources.requests.cpu }{ "\t" }
  { .spec.containers[0].resources.limits.memory }{ "/" }
  { .spec.containers[0].resources.requests.memory }{ "\n" }
  { end }'
  ```
- **回滚方案**: 应用备份的 LimitRange YAML

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-008: 强制删除 Terminating Namespace
- **适用根因**: RC-010
- **审批要求**: 需集群管理员或高级 SRE 审批，确认 Namespace 中无重要数据
- **数据备份**: 备份 Namespace 中所有资源的 YAML
- **操作步骤**:
  1. 导出 Namespace 中所有资源（用于审计）
  2. 移除 Namespace 的 finalizers
  3. 如果仍有资源残留，手动清理 APIService 或 CRD 资源
  4. 验证 Namespace 已删除
  5. 如需重建，重新创建 Namespace 和配额配置
- **执行命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 步骤 1: 备份资源
  kubectl api-resources --verbs=list --namespaced -o name | xargs -n1 -I{} sh -c 'kubectl get {} -n <namespace> -o yaml > /tmp/backup-<namespace>-{}.yaml 2>/dev/null || true'
  
  # 步骤 2: 移除 finalizers
  kubectl get namespace <namespace> -o json | jq '.spec.finalizers = []' | kubectl replace --raw "/api/v1/namespaces/<namespace>/finalize" -f -
  
  # 步骤 3: 如果 Namespace 仍存在，强制清理
  kubectl delete namespace <namespace> --force --grace-period=0  # ⚠️ 不可逆：永久删除命名空间及全部资源
  
  # 备选方案：直接通过 API 删除 finalizers
  kubectl patch namespace <namespace> --type json -p='[{"op": "remove", "path": "/metadata/finalizers"}]'
  ```
- **回滚方案**: 使用步骤 1 的备份 YAML 重建 Namespace 及资源

## 验证确认

### 7.1 即时验证（修复后 1 分钟内）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# V1: 验证 ResourceQuota 状态正常
kubectl get resourcequota -n <namespace> -o jsonpath='{
  .items[*].status.used}{"\n"}{.items[*].status.hard
}'
# 预期: used 各项均小于 hard

# V2: 验证 Pod 可以正常创建
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: validation-pod
  namespace: <namespace>
spec:
  containers:
  - name: test
    image: busybox:1.36
    command: ["sh", "-c", "sleep 30"]
    resources:
      requests:
        cpu: "50m"
        memory: "128Mi"
      limits:
        cpu: "100m"
        memory: "256Mi"
EOF
kubectl wait --for=condition=Ready pod/validation-pod -n <namespace> --timeout=60s
# 预期: Pod 状态变为 Running

# V3: 验证 LimitRange 配置正确
kubectl describe limitrange -n <namespace>
# 预期: min <= default <= max，无冲突提示

# V4: 验证 Namespace 状态正常
kubectl get namespace <namespace> -o jsonpath='{.status.phase}'
# 预期: Active（如适用）
```
### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| 配额使用率 | `kube_resourcequota_used_hard_ratio` | 下降并稳定在 <80% | >95% |
| Pod 创建成功率 | `kubectl get events --field-selector reason=FailedCreate` | 无新 FailedCreate 事件 | 任何新事件 |
| Namespace 状态 | `kubectl get namespace <ns>` | 保持 Active | Terminating |
| LimitRange 注入值 | 新建 Pod 的 resources 字段 | 符合 LimitRange 配置 | 注入异常值 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：
- [ ] ResourceQuota 的 `used` < `hard`（所有资源项）
- [ ] LimitRange 的 `min` <= `default` <= `max`（所有类型）
- [ ] 测试 Pod 可以在 Namespace 中正常创建并运行
- [ ] Namespace 状态为 Active（非 Terminating）
- [ ] 无新的 FailedCreate 配额相关事件

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 配额使用率趋势 | Prometheus `kube_resourcequota_used_hard_ratio` | 每 4 小时 | 如果持续增长，需扩容配额 |
| Pod 创建失败事件 | `kubectl get events --field-selector reason=FailedCreate` | 每 8 小时 | 如有新事件，重新诊断 |
| LimitRange 变更 | GitOps/配置审计日志 | 每次变更 | 验证变更合理性 |
| Namespace 删除操作 | 审计日志 | 每次删除 | 关注 Terminating 卡死 |

## 升级协议

### 8.1 自动升级条件

| 条件 | 说明 |
|------|------|
| 诊断超时 | 诊断工作流执行超过 20 分钟未确认根因 |
| 修复失败 | 同一修复操作执行 2 次仍未通过验证 |
| 严重性升级 | 初始分级为 P2 但影响面扩大到 P0 级别（如多个 Namespace 同时问题） |
| 未知根因 | 诊断完成但无法匹配任何已知根因 |
| 集群级问题 | 怀疑 API Server 准入控制器异常 |

### 8.2 升级消息模板

```
【{severity}】{skill_name} - {cluster_name}
- 问题概述: Namespace {namespace} 的 {resource} 配额/限制异常
- 影响范围: {affected_namespaces} 个 Namespace，{affected_pods} 个 Pod 创建受阻
- 已完成诊断: {completed_steps}
- 初步发现: {root_cause_candidate}
- 需要: {action_needed}
- 工单编号: {ticket_id}
```

### 8.3 交接信息包

升级时，Agent 需准备以下信息：
1. 完整诊断路径和每步输出
2. ResourceQuota 和 LimitRange 的当前 YAML 配置
3. 受影响的 Namespace 列表和 Pod 创建失败事件
4. 已排除的根因及原因
5. 可能的根因假设
6. 最近 30 分钟的关键事件时间线

## 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| ResourceQuota 优先级抢占 | 支持 | 支持 | 支持 | 支持 | 支持 |
| LimitRange 默认存储类 | 支持 | 支持 | 支持 | 支持 | 支持 |
| 跨命名空间配额（ClusterResourceQuota） | 需 OpenShift | 需 OpenShift | 需 OpenShift | 需 OpenShift | 需 OpenShift |
| ResourceQuota 跟踪终止中 Pod | 支持 | 支持 | 支持 | 支持 | 支持 |
| LimitRange 对 Init Container 限制 | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get resourcequota` | 标准输出 | 标准输出 | 标准输出 | 标准输出 | 标准输出 |
| `--field-selector status.phase=Terminating` | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| ResourceQuota | v1 | v1 | v1 | v1 | v1 |
| LimitRange | v1 | v1 | v1 | v1 | v1 |
| Namespace | v1 | v1 | v1 | v1 | v1 |

## 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 将节点资源不足误诊为配额超限 | Pod Pending，Event 提示 insufficient resource | 节点 CPU/Memory 不足 | 检查 `kubectl describe node` 的 Allocatable 和 Requested |
| 将 RBAC Forbidden 误诊为配额超限 | Pod 创建失败，提示 Forbidden | RBAC 权限不足 | 检查用户是否有 create pod 权限 |
| 将正常配额限制误诊为问题 | 非生产 Namespace 配额告警 | 预期行为，防止资源滥用 | 区分告警阈值和实际问题 |
| 将 Terminating Pod 计入配额异常 | ResourceQuota used > Running Pod 数 | Terminating Pod 仍占用配额 | 检查 Pod 是否真正完成终止 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：
- ResourceQuota 控制器工作原理 → `domain-01-cluster-fundamentals/resource-quota-controller.md`
- LimitRange 准入控制器机制 → `domain-01-cluster-fundamentals/limit-range-admission.md`
- Namespace 生命周期和 Finalizer → `domain-01-cluster-fundamentals/namespace-lifecycle.md`
- 多租户资源隔离最佳实践 → `domain-11-production-operations/topic-best-practices/multi-tenant-resource-isolation.md`

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-05 | v1.0 | 初始版本 | 覆盖 Namespace/Quota/LimitRange 故障诊断 |

## 云厂商特异性

| 平台 | 差异 | 诊断命令 | 备注 |
|------|------|---------|------|
| ACK | Namespace 删除时可能受云资源（SLB/磁盘）Finalizer 阻塞 | `aliyun cs GET /k8s/{cluster-id}/namespaces/{ns}` | 需先清理关联云资源 |
| EKS | Fargate Profile 有自己的资源配额逻辑 | `aws eks describe-fargate-profile` | Fargate Pod 不计入节点配额 |
| GKE | Autopilot 模式强制要求设置 resources | `gcloud container clusters describe` | 未设置 resources 的 Pod 会被拒绝 |
| AKS | 支持 ClusterResourceQuota（通过 AKS 扩展） | `az aks show` | 需注意集群级配额叠加 |

## 自动化集成接口

### 12.1 脚本入口

- **diagnose-quick.sh**: Phase 1 快速诊断脚本入口
  - 调用约定: `./scripts/diagnose-quick.sh --namespace <NS>`
  - 输出: ResourceQuota 状态、LimitRange 配置、FailedCreate 事件
- **diagnose-deep.sh**: Phase 2 深度诊断脚本入口
  - 调用约定: `./scripts/diagnose-deep.sh --namespace <NS>`
  - 输出: 配额使用量分析、LimitRange 冲突检测、Namespace 终止原因
- **verify.sh**: 修复后验证脚本入口
  - 调用约定: `./scripts/verify.sh --namespace <NS>`
  - 输出: 验证 Pod 创建测试、配额状态确认

### 12.2 Webhook 回调

- **告警路由**: 从 AlertManager/Prometheus 告警自动触发 Skill
- **回调格式**: JSON payload 含 skill_id、trigger_source、context

### 12.3 输出规范

| 脚本 | 用途 | 示例调用 |
|------|------|----------|
| diagnose-quick.sh | Phase 1 快速检查 | `./scripts/diagnose-quick.sh --namespace prod-app` |
| diagnose-deep.sh | Phase 2 深度检查 | `./scripts/diagnose-deep.sh --namespace prod-app` |
| verify.sh | 修复后验证 | `./scripts/verify.sh --namespace prod-app` |

### 12.4 Webhook 配置示例

```yaml
# AlertManager Webhook 示例
receivers:
- name: skill-trigger
  webhook_configs:
  - url: 'http://agent-gateway/skill/SKILL-CONFIG-002'
    send_resolved: true
```

### 12.5 输出 JSON Schema

```json
{
  "skill_id": "SKILL-CONFIG-002",
  "findings": [
    { "step": "D1.1", "result": "used.pods >= hard.pods", "severity": "critical" }
  ],
  "root_cause_candidates": [
    { "rc_id": "RC-001", "confidence": 0.95, "evidence": ["D1.1", "D1.4"] }
  ],
  "recommended_action": {
    "rem_id": "REM-001",
    "risk_level": "low",
    "command": "kubectl patch resourcequota <name> -n <ns> --type merge -p '{...}'",
    "rollback": "kubectl patch resourcequota <name> -n <ns> --type merge -p '{...}'"
  }
}
```

---

## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/技能体系/23-job-cronjob-failure.md|SKILL-WORK-004 Job/CronJob 故障诊断]]
- [[domain-10-troubleshooting-diagnostics/技能体系/19-node-resource-pressure.md|SKILL-NODE-002 节点资源压力诊断]]
- [[domain-10-troubleshooting-diagnostics/技能体系/20-networkpolicy-connectivity.md|SKILL-NET-004 NetworkPolicy 连通性问题]]
- [[domain-10-troubleshooting-diagnostics/技能体系/21-statefulset-failure.md|SKILL-WORK-002 StatefulSet 故障诊断]]
- [[domain-10-troubleshooting-diagnostics/技能体系/22-daemonset-failure.md|SKILL-WORK-003 DaemonSet 故障诊断]]
- [[domain-10-troubleshooting-diagnostics/资源排障/24-quota-limitrange-troubleshooting.md|Quota/LimitRange 深度排查]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/resource-quota-fta.md|Quota/LimitRange 故障树分析]]

```

<!-- risk-assessed -->
