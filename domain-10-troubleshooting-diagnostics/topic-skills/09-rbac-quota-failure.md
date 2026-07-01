---
title: RBAC 权限与 ResourceQuota 故障诊断 / RBAC & ResourceQuota Troubleshooting
description: '## 1. 概述'
summary: '## 1. 概述'
category: security
tags:
- k8s
- skills
- sop
- runbook
- apiserver
- kubelet
- controller-manager
- prometheus
- istio
- argocd
tier: core
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- RBAC 权限与 ResourceQuota 故障诊断 / RBAC & ResourceQuota Troubleshooting 是什么
- 如何 RBAC 权限与 ResourceQuota 故障诊断 / RBAC & ResourceQuota Troubleshooting
trigger_keywords:
- 403 Forbidden
- RBAC denied
- unauthorized
- ResourceQuota exceeded
- LimitRange conflict
- permission denied
- cannot list
- cannot create
- cannot delete
- quota exceeded
- 权限不足
- 配额超限
- 禁止访问
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- gitops-basics
- tls-basics
- policy-basics
skill_id: SKILL-09_RBAC_QUOTA_FAILURE-001
skill_name: RBAC 权限与 ResourceQuota 故障诊断 / RBAC & ResourceQuota Troubleshooting
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---



---


# RBAC 权限与 ResourceQuota 故障诊断 / RBAC & ResourceQuota Troubleshooting

---

## 1. 概述

RBAC（Role-Based Access Control）和 ResourceQuota 是 [[Kubernetes|Kubernetes]] 中最核心的安全与资源治理机制。RBAC 问题会直接导致用户、ServiceAccount 或控制器无法执行预期操作，严重时可阻断整个 CI/CD 流水线或导致生产服务无法部署。ResourceQuota 和 LimitRange 问题则会导致工作负载无法创建或调度，影响业务扩容和新服务上线。此外，现代 Kubernetes 集群普遍部署 OPA/Gatekeeper 或 [[Kyverno|Kyverno]] 等策略引擎，其 Admission Controller 拦截也会产生类似 RBAC 403 的错误表象。

### 典型触发场景

1. **RBAC 授权问题**: 用户或 ServiceAccount 缺少必要的 Role/RoleBinding，导致 API 调用返回 403 Forbidden。常见于新服务部署、跨 Namespace 访问、CI/CD 权限配置不当
2. **ResourceQuota 配额耗尽**: Namespace 内 CPU/Memory/对象数量达到配额上限，新 Pod/PVC/Service 创建被拒绝。常见于资源紧张的生产环境、批量任务执行、资源泄漏场景
3. **LimitRange 约束冲突**: Pod 资源请求不满足 LimitRange 定义的最小/最大限制，或未设置 requests/limits 导致被 LimitRange 默认值覆盖后超限
4. **Admission Controller 策略拦截**: OPA/Gatekeeper、Kyverno 或其他 ValidatingWebhook 基于安全/合规策略拒绝资源创建
5. **多租户隔离问题**: 跨 Namespace 访问被 [[NetworkPolicy|NetworkPolicy]] 或 RBAC 隔离策略阻止

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 `roles`, `rolebindings`, `clusterroles`, `clusterrolebindings`, `resourcequotas`, `limitranges`, `pods`, `events` 的 `get/list/watch`
  - 修复权限: `roles`, `rolebindings`, `clusterroles`, `clusterrolebindings`, `resourcequotas`, `limitranges` 的 `create/update/delete`
  - 验证命令: `kubectl auth can-i list clusterroles`
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `jq` >= 1.6（可选但推荐）
- **监控系统**: Prometheus + kube-state-metrics >= v2.10（用于 trigger_metrics 匹配）
- **可选**: `kubectl-who-can` 插件、`rbac-lookup` 工具

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | API 调用返回 `403 Forbidden` 错误 / API call returns 403 Forbidden | 执行 kubectl 命令时显示 `Error from server (Forbidden): ... is forbidden: User "xxx" cannot ...` | 0.95 | 错误信息包含 `admission webhook denied`（非 RBAC 问题，应检查 Webhook） |
| SP-02 | ServiceAccount 无法执行操作（cannot list/get/create/delete）/ ServiceAccount cannot perform operations | Pod 日志显示 `cannot list resource "xxx" in API group "yyy"` 或 `Unauthorized` | 0.90 | Token 挂载路径不存在或为空（应检查 Pod spec 中的 serviceAccountName 配置） |
| SP-03 | Pod 创建被拒绝（exceeded quota）/ Pod creation rejected due to quota | `kubectl describe pod` 或 Event 显示 `exceeded quota` 错误 | 0.95 | 错误信息指向 LimitRange 而非 ResourceQuota |
| SP-04 | LimitRange 导致 Pod 资源被自动调整 / LimitRange auto-adjusting Pod resources | Pod 实际 requests/limits 与 spec 定义不同，被 LimitRange defaultRequest/defaultLimit 覆盖 | 0.85 | 用户显式定义了 requests/limits 但仍在 LimitRange 允许范围内 |
| SP-05 | Aggregated ClusterRole 权限未生效 / Aggregated ClusterRole permissions not effective | ClusterRole 使用 aggregationRule 但聚合的子 Role 权限未被包含 | 0.80 | ClusterRole 为非聚合类型（无 aggregationRule 字段） |
| SP-06 | `kubectl auth can-i` 返回 `no` / kubectl auth can-i returns no | `kubectl auth can-i create pods --as=system:serviceaccount:ns:sa` 返回 `no` | 0.95 | 测试使用的主体名称格式错误（User vs ServiceAccount） |
| SP-07 | Webhook admission denied 消息 / Webhook admission denied message | 错误信息包含 `admission webhook "xxx" denied the request` | 0.90 | 非策略拦截的 Webhook 错误（如 MutatingWebhook 配置问题） |
| SP-08 | Namespace 级操作被拒绝（跨 NS 访问）/ Cross-namespace access denied | 尝试访问其他 Namespace 资源时返回 Forbidden | 0.85 | 目标 Namespace 不存在（应为 NotFound 而非 Forbidden） |
| SP-09 | Token 认证失败（ServiceAccount token 过期/无效）/ ServiceAccount token authentication failure | API 调用返回 `Unauthorized` 或 `error: You must be logged in to the server` | 0.80 | kubelet 或 kube-apiserver 连接问题（非 Token 问题） |
| SP-10 | 审计日志中大量 403 记录 / High volume of 403 records in audit logs | `/var/log/kubernetes/audit/audit.log` 中短时间内出现大量 responseStatus.code=403 | 0.85 | 正常的权限探测行为（如 kubectl 的 SelfSubjectAccessReview） |
| SP-11 | PVC/Service/ConfigMap 创建被 quota 拒绝 / PVC/Service/ConfigMap creation rejected by quota | 创建非 Pod 资源时显示 `exceeded quota: xxx, requested: count/xxx=1, used: count/xxx=N, limited: count/xxx=N` | 0.90 | 资源类型不在 ResourceQuota 范围内 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "用户反馈操作被拒绝，显示 403 Forbidden"
- "ServiceAccount 权限不足，Pod 无法访问 API"
- "创建 Pod 失败，提示配额超限"
- "新服务部署失败，权限验证不通过"
- "CI/CD 流水线报错 cannot create resource"
- "多租户场景下跨命名空间访问被拒绝"
- "OPA/Gatekeeper 策略拦截了部署请求"
- "LimitRange 配置导致 Pod 创建失败"
- "Token 无效，认证失败"

**English ticket descriptions**:
- "User getting 403 Forbidden when trying to deploy"
- "ServiceAccount cannot list pods in namespace"
- "Pod creation failed with exceeded quota error"
- "RBAC permission denied for CI/CD pipeline"
- "Cannot create deployment, permission denied"
- "Admission webhook blocking resource creation"
- "ResourceQuota exceeded, cannot create more pods"
- "Token expired or invalid for service account"
- "Kyverno policy rejecting deployment"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 错误信息为 `Unauthorized` 且非 Token 问题，而是 kubeconfig 缺失 | 配置问题 | 用户未正确配置 kubeconfig 或证书过期，参考 SKILL-SEC-001 |
| 错误信息为 NetworkPolicy 阻止网络访问 | SKILL-NET-xxx | 网络层面的隔离问题，非 RBAC 授权问题 |
| Pod Pending 但原因是资源不足（无 quota 错误）| SKILL-POD-002 | 调度问题，节点资源不足而非配额问题 |
| API Server 本身不可用（连接超时）| 控制平面问题 | 超出本 Skill 范围，需排查 apiserver |
| 用户主动删除 RoleBinding 进行权限收回 | 非问题 | 正常的权限管理操作 |
| 证书过期导致的认证失败 | SKILL-SEC-001 | 证书管理问题，非 RBAC 授权问题 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题影响范围：

**Step T1**: 确认 403 错误的主体和范围（10s）
```bash
# 从错误信息中提取主体（User/ServiceAccount/Group）
# 示例错误: User "system:serviceaccount:production:cicd-sa" cannot create ...
# 检查是否为关键 ServiceAccount
kubectl get sa -A | grep -E "(cicd|jenkins|argocd|flux|crossplane)"
```
> **判断规则**:
> - 受影响主体为 CI/CD 系统 ServiceAccount → **P0**（阻断部署流水线）
> - 受影响主体为关键控制器（如 [[cert-manager|cert-manager]]、external-dns）→ **P1**
> - 受影响主体为普通用户 → **P2**

**Step T2**: 检查 ResourceQuota 使用情况（30s）
```bash
# 查看所有 Namespace 的配额使用情况
kubectl get resourcequota -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,USED_CPU:.status.used.requests\\.cpu,HARD_CPU:.status.hard.requests\\.cpu,USED_MEM:.status.used.requests\\.memory,HARD_MEM:.status.hard.requests\\.memory
```
> **判断规则**:
> - 生产 Namespace 配额使用率 > 90% → **P1**
> - 多个 Namespace 配额同时耗尽 → **P0**
> - 单个非生产 Namespace 配额紧张 → **P2**

**Step T3**: 检查 RoleBinding/ClusterRoleBinding 状态（60s）
```bash
# 查找与问题主体相关的绑定
SA_NAME="<serviceaccount-name>"
NS="<namespace>"
kubectl get rolebinding,clusterrolebinding -A -o wide | grep -E "${SA_NAME}|${NS}"
# 如果无结果，说明可能缺少绑定
```
> **判断规则**:
> - 找不到任何绑定 → 可能为 RC-001（RoleBinding 缺失）
> - 存在绑定但权限不足 → 可能为 RC-002（Role 规则不完整）

**Step T4**: 检查 Admission Webhook 状态（30s）
```bash
# 查看 ValidatingWebhookConfiguration 是否存在拦截
kubectl get validatingwebhookconfiguration -o name | wc -l
kubectl get events -A --field-selector reason=FailedCreate | grep -i "denied|rejected|forbidden" | head -10
```
> **判断规则**:
> - 存在多个 ValidatingWebhook 且 Event 中有 denied 记录 → 可能为 RC-007（策略拦截）

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| CI/CD 流水线 ServiceAccount 被阻断 **或** 多个生产 Namespace 配额耗尽 | **P0** | 直接影响生产部署能力，业务无法发布或扩容 | 立即响应，15min 内确认根因 |
| 关键控制器（cert-manager/argocd/external-dns）权限异常 **或** 单个生产 Namespace 配额紧张 | **P1** | 影响集群运维自动化或部分业务 | 15min 内响应，30min 内修复 |
| 普通开发者权限问题 **或** 开发/测试 Namespace 配额问题 | **P2** | 影响开发效率但不影响生产 | 30min 内响应，2h 内修复 |
| 新建 ServiceAccount 权限配置不当 **或** 低优先级 Namespace 的 LimitRange 问题 | **P3** | 影响范围有限，不影响现有服务 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **生产发布阻断**: 生产环境 CI/CD 流水线完全无法执行任何部署操作
- **大规模权限异常**: 超过 5 个 Namespace 同时报告 RBAC 权限问题
- **控制器级联问题**: 集群核心控制器（如 kube-controller-manager 使用的 SA）权限异常
- **安全事件**: 怀疑存在权限提升攻击或未授权访问尝试
- **配额全局耗尽**: 整个集群资源配额耗尽，无法进行任何资源创建

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: RBAC 快速诊断（只读，零风险）

> **目标**: 快速确认 RBAC 权限配置问题的根因类型
> **预计耗时**: 2-5 分钟

**Step D1.1**: 权限验证 - 列出主体所有权限
- **命令**:
  ```bash
  # 替换 NS 和 SA 为实际值
  kubectl auth can-i --list --as=system:serviceaccount:NS:SA
  # 或针对特定操作验证
  kubectl auth can-i create pods --as=system:serviceaccount:NS:SA -n TARGET_NS
  ```
- **超时**: 10s
- **预期输出模式**: 权限列表或 `yes`/`no` 响应
- **判断规则**:
  - 输出为空或仅有极少权限 → 可能为 RC-001（RoleBinding 缺失）
  - 有部分权限但缺少特定操作 → 可能为 RC-002（Role 规则不完整）
  - 所有权限正常 → 问题可能不在 RBAC，继续 Phase 2 或 Phase 3
- **版本差异**: 无

**Step D1.2**: RoleBinding 查询 - 查找主体关联的绑定
- **命令**:
  ```bash
  # 查找 ServiceAccount 的所有绑定
  SA_NAME="<sa-name>"
  kubectl get rolebinding -A -o wide | grep "$SA_NAME"
  kubectl get clusterrolebinding -o wide | grep "$SA_NAME"
  ```
- **超时**: 15s
- **预期输出模式**: RoleBinding/ClusterRoleBinding 列表
- **判断规则**:
  - 无任何输出 → RC-001（RoleBinding 缺失）
  - 存在绑定但绑定的 Role 不正确 → 继续 D1.3 检查 Role 内容
  - 绑定存在且看似正确 → 检查 subjects 配置是否匹配
- **版本差异**: 无

**Step D1.3**: Role/ClusterRole 规则检查
- **命令**:
  ```bash
  # 查看 Role 详细规则
  kubectl get role ROLE_NAME -n NS -o yaml
  # 或 ClusterRole
  kubectl get clusterrole CLUSTERROLE_NAME -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: Role/ClusterRole YAML 定义
- **判断规则**:
  - `rules` 字段中缺少所需的 `verbs`（如需要 create 但只有 get/list）→ RC-002
  - `rules` 字段中缺少所需的 `resources`（如需要 deployments 但只有 pods）→ RC-002
  - `rules` 字段中 `apiGroups` 不正确（如需要 `apps` 但只有 `""`）→ RC-011
  - 规则看似完整 → 检查是否为 Aggregated ClusterRole 问题（D1.5）
- **版本差异**:
  - **[v1.28+]**: ClusterRole 聚合使用 `aggregationRule.clusterRoleSelectors`

**Step D1.4**: ServiceAccount 状态检查
- **命令**:
  ```bash
  kubectl get sa SA_NAME -n NS -o yaml
  # 检查 secrets 和 automountServiceAccountToken
  ```
- **超时**: 5s
- **预期输出模式**: ServiceAccount YAML 定义
- **判断规则**:
  - `automountServiceAccountToken: false` 但 Pod 需要访问 API → Token 未挂载
  - SA 不存在 → 需要创建 ServiceAccount
  - SA 存在但无关联 secret（v1.24 之前）→ 检查 token controller
- **版本差异**:
  - **[v1.24+]**: ServiceAccount 不再自动创建 Secret，使用 projected token

**Step D1.5**: Token 有效性检查
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查 Pod 中的 token 挂载
  kubectl exec -it POD_NAME -n NS -- cat /var/run/secrets/kubernetes.io/serviceaccount/token
  # 验证 token（需要 jq）
  kubectl exec -it POD_NAME -n NS -- cat /var/run/secrets/kubernetes.io/serviceaccount/token | cut -d'.' -f2 | base64 -d 2>/dev/null | jq .
  ```
- **超时**: 15s
- **预期输出模式**: Token 内容和 JWT payload
- **判断规则**:
  - Token 文件不存在 → `automountServiceAccountToken: false` 或 volumeMount 缺失
  - Token 存在但 JWT exp 字段已过期 → RC-005（Token 过期）
  - Token 中的 sub 字段与预期 SA 不匹配 → 配置错误
- **版本差异**:
  - **[v1.30+]**: Bound ServiceAccount Token 生命周期管理增强
  - **[v1.32+]**: Token cleanup controller GA，过期 token 会被自动清理

---

### Phase 2: Quota/LimitRange 诊断（只读，零风险）

> **目标**: 检查 ResourceQuota 和 LimitRange 配置及使用情况
> **预计耗时**: 3-5 分钟

**Step D2.1**: 配额使用情况检查
- **命令**:
  ```bash
  # 查看 Namespace 内所有配额
  kubectl describe quota -n NS
  # 或获取结构化输出
  kubectl get resourcequota -n NS -o jsonpath='{range .items[*]}{"Name: "}{.metadata.name}{"\n"}{range $k,$v := .status.hard}{"  "}{$k}{": used="}{index $.status.used $k}{"/"}{$v}{"\n"}{end}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: ResourceQuota 详情，包含 used 和 hard 值
- **判断规则**:
  - `used` 等于 `hard` → RC-003（配额耗尽）
  - `used` 接近 `hard`（>90%）→ 配额紧张，需要关注
  - 无 ResourceQuota 输出 → Namespace 无配额限制，问题可能在 LimitRange
- **版本差异**: 无

**Step D2.2**: LimitRange 配置检查
- **命令**:
  ```bash
  kubectl describe limitrange -n NS
  ```
- **超时**: 10s
- **预期输出模式**: LimitRange 详情，包含 min/max/default/defaultRequest
- **判断规则**:
  - Pod 请求的资源 < `min` → RC-004（低于最小限制）
  - Pod 请求的资源 > `max` → RC-004（超过最大限制）
  - Pod 未设置 requests/limits，LimitRange 的 `default` 值导致超 quota → RC-004
- **版本差异**: 无

**Step D2.3**: 资源使用统计
- **命令**:
  ```bash
  # 查看当前 Pod 资源使用情况
  kubectl top pods -n NS --sort-by=memory
  # 计算总资源请求
  kubectl get pods -n NS -o jsonpath='{range .items[*]}{.metadata.name}{" CPU: "}{.spec.containers[*].resources.requests.cpu}{" Memory: "}{.spec.containers[*].resources.requests.memory}{"\n"}{end}'
  ```
- **超时**: 15s
- **预期输出模式**: Pod 资源使用数据
- **判断规则**:
  - 存在大量 Completed/Failed Pod 占用配额 → 建议清理（REM-003）
  - 单个 Pod 请求过大占用大部分配额 → 需要优化资源请求
- **版本差异**: 无

**Step D2.4**: 跨 Namespace 配额对比
- **命令**:
  ```bash
  # 对比多个 Namespace 的配额使用
  for ns in production staging development; do
    echo "=== $ns ==="
    kubectl describe quota -n $ns 2>/dev/null | grep -E "^(Name:|cpu|memory|pods)" || echo "No quota"
  done
  ```
- **超时**: 20s
- **预期输出模式**: 多 Namespace 配额摘要
- **判断规则**:
  - 生产环境配额紧张而其他环境空闲 → 资源分配不均
  - 所有环境配额都紧张 → 集群整体资源不足
- **版本差异**: 无

**Step D2.5**: PriorityClass 对配额的影响
- **命令**:
  ```bash
  # 检查是否有基于 PriorityClass 的配额作用域
  kubectl get resourcequota -n NS -o yaml | grep -A10 "scopeSelector|scopes"
  # 查看 PriorityClass 定义
  kubectl get priorityclass
  ```
- **超时**: 10s
- **预期输出模式**: 配额作用域和 PriorityClass 列表
- **判断规则**:
  - 配额有 `scopeSelector.matchExpressions` 限制特定 PriorityClass → 高优先级 Pod 可能有独立配额
  - 使用 `NotTerminating` 或 `Terminating` scope → 区分长期运行和批处理配额
- **版本差异**:
  - **[v1.28+]**: 增强的 scopeSelector 支持更复杂的配额规则

---

### Phase 3: Admission Controller 诊断（只读，零风险）

> **目标**: 检查 OPA/Gatekeeper、Kyverno 等策略引擎的拦截行为
> **预计耗时**: 3-5 分钟

**Step D3.1**: ValidatingWebhookConfiguration 列表
- **命令**:
  ```bash
  kubectl get validatingwebhookconfiguration -o wide
  # 查看具体配置
  kubectl get validatingwebhookconfiguration -o yaml | grep -A20 "name:|rules:|failurePolicy:"
  ```
- **超时**: 10s
- **预期输出模式**: Webhook 配置列表
- **判断规则**:
  - 存在 `gatekeeper-*` 或 `kyverno-*` webhook → 可能被策略拦截
  - `failurePolicy: Fail` 且 webhook 不可用 → 所有请求都会被拒绝
  - 无 ValidatingWebhook → 问题不在 Admission Controller
- **版本差异**: 无

**Step D3.2**: MutatingWebhookConfiguration 列表
- **命令**:
  ```bash
  kubectl get mutatingwebhookconfiguration -o wide
  ```
- **超时**: 10s
- **预期输出模式**: MutatingWebhook 配置列表
- **判断规则**:
  - MutatingWebhook 可能修改资源导致后续 ValidatingWebhook 拒绝
  - 检查 webhook 顺序（通过 `reinvocationPolicy`）
- **版本差异**: 无

**Step D3.3**: OPA/Gatekeeper 约束检查
- **命令**:
  ```bash
  # 列出所有约束
  kubectl get constraints
  # 查看约束详情和违规情况
  kubectl get constraints -o jsonpath='{range .items[*]}{.kind}{"/"}{.metadata.name}{": violations="}{.status.totalViolations}{"\n"}{end}'
  # 查看具体约束规则
  kubectl get constrainttemplate -o yaml | grep -A30 "kind: K8s"
  ```
- **超时**: 15s
- **预期输出模式**: Constraint 列表和违规统计
- **判断规则**:
  - 存在 violations > 0 的约束 → 检查是否包含被拒绝的资源
  - 约束的 `enforcementAction: deny` → 会阻止资源创建（RC-007）
  - `enforcementAction: dryrun` → 仅记录不拦截
- **版本差异**:
  - **[v1.29+]**: Gatekeeper v3.14+ 支持 Mutation

**Step D3.4**: Kyverno 策略检查
- **命令**:
  ```bash
  # 列出所有策略
  kubectl get clusterpolicy,policy -A
  # 查看策略违规报告
  kubectl get policyreport,clusterpolicyreport -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{": "}{.summary}{"\n"}{end}'
  # 查看具体策略规则
  kubectl get clusterpolicy -o yaml | grep -A20 "name:|validate:|match:"
  ```
- **超时**: 15s
- **预期输出模式**: Policy 列表和违规报告
- **判断规则**:
  - 存在 `validationFailureAction: Enforce` 的策略 → 会阻止资源创建（RC-007）
  - PolicyReport 中有 fail 状态 → 检查具体失败原因
  - `validationFailureAction: Audit` → 仅审计不拦截
- **版本差异**:
  - **[v1.28+]**: Kyverno 1.10+ 支持 ValidatingAdmissionPolicy 集成

**Step D3.5**: Webhook 日志分析
- **命令**:
  ```bash
  # Gatekeeper audit 控制器日志
  kubectl logs -n gatekeeper-system -l control-plane=audit-controller --tail=50 | grep -i "denied|violation"
  # Kyverno admission 控制器日志
  kubectl logs -n kyverno -l app=kyverno --tail=50 | grep -i "denied|blocked|failed"
  # 通用 webhook Pod 日志
  kubectl get pods -A -l app.kubernetes.io/component=webhook -o name | head -1 | xargs kubectl logs --tail=30
  ```
- **超时**: 20s
- **预期输出模式**: Webhook 日志中的拒绝记录
- **判断规则**:
  - 日志中包含具体的拒绝原因 → 确认策略拦截（RC-007 或 RC-012）
  - 日志中显示策略冲突 → RC-012（策略冲突导致死锁）
  - 无拒绝日志 → 问题可能不在策略层面
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | **RoleBinding 缺失** — ServiceAccount/User 没有关联任何 RoleBinding 或 ClusterRoleBinding，导致完全无权限 | ~22% | D1.2 无任何绑定输出；D1.1 权限列表为空 | rbac-fta: BE-missing-binding |
| RC-002 | **Role 规则不完整** — Role/ClusterRole 存在但缺少必要的 verbs、resources 或 apiGroups | ~18% | D1.3 规则中缺少所需 verb/resource；D1.1 缺少特定权限 | rbac-fta: BE-incomplete-rules |
| RC-003 | **ResourceQuota CPU/Memory 耗尽** — Namespace 的计算资源配额已达上限，无法创建新 Pod | ~15% | D2.1 显示 used=hard；Event 包含 exceeded quota | quota-fta: BE-compute-quota-exhausted |
| RC-004 | **LimitRange defaultRequest/defaultLimit 冲突** — Pod 未设置 requests/limits，LimitRange 默认值应用后超出 quota 或不满足 min/max 约束 | ~8% | D2.2 LimitRange 有 default 值；Pod 实际资源与 spec 不同 | quota-fta: BE-limitrange-conflict |
| RC-005 | **ServiceAccount Token 无效/过期** — Bound token 已过期或被删除，API 认证失败 | ~7% | D1.5 Token 不存在或 JWT exp 已过期；返回 Unauthorized | rbac-fta: BE-token-invalid |
| RC-006 | **Aggregated ClusterRole label selector 不匹配** — 聚合 ClusterRole 的 selector 无法匹配到子 Role，导致权限未聚合 | ~6% | D1.3 aggregationRule 存在但 rules 为空；子 Role 标签不匹配 | rbac-fta: BE-aggregation-mismatch |
| RC-007 | **OPA/Gatekeeper 策略误拒** — Constraint 配置过于严格，合法资源被拦截 | ~5% | D3.3 存在 violations；D3.5 日志显示 denied | policy-fta: BE-policy-too-strict |
| RC-008 | **Namespace 级隔离配置导致跨 NS 访问失败** — Role/RoleBinding 仅在单一 Namespace 生效，跨 NS 访问被拒绝 | ~5% | D1.2 RoleBinding 存在于其他 NS；跨 NS 操作返回 Forbidden | rbac-fta: BE-namespace-isolation |
| RC-009 | **Object count quota 耗尽** — pods/services/configmaps/secrets 等对象数量达到上限 | ~4% | D2.1 count/xxx 类型配额 used=hard | quota-fta: BE-object-count-exhausted |
| RC-010 | **RBAC 规则继承与覆盖冲突** — 多个 RoleBinding 绑定不同 Role，权限组合导致意外行为 | ~4% | D1.2 存在多个绑定；D1.1 权限与预期不符 | rbac-fta: BE-rule-conflict |
| RC-011 | **API Group 版本不匹配** — Role 中 apiGroups 配置错误，如使用 `extensions/v1beta1` 而非 `apps/v1` | ~3% | D1.3 apiGroups 与目标资源 API 不匹配 | rbac-fta: BE-apigroup-mismatch |
| RC-012 | **Kyverno/OPA 策略冲突导致死锁** — 多个策略相互冲突，任何配置都无法通过验证 | ~3% | D3.3/D3.4 多个策略 violations；D3.5 日志显示策略冲突 | policy-fta: BE-policy-deadlock |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 创建/修复 RoleBinding
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 确认 ServiceAccount 存在
  kubectl get sa SA_NAME -n NS
  # 确认目标 Role/ClusterRole 存在
  kubectl get clusterrole ROLE_NAME
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 创建 RoleBinding（Namespace 级权限）
  kubectl create rolebinding BINDING_NAME \
    --clusterrole=ROLE_NAME \
    --serviceaccount=NS:SA_NAME \
    -n TARGET_NS
  
  # 或创建 ClusterRoleBinding（集群级权限）
  kubectl create clusterrolebinding BINDING_NAME \
    --clusterrole=ROLE_NAME \
    --serviceaccount=NS:SA_NAME
  ```
- **后置验证**:
  ```bash
  kubectl auth can-i VERB RESOURCE --as=system:serviceaccount:NS:SA_NAME -n TARGET_NS
  # 预期: yes
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete rolebinding BINDING_NAME -n TARGET_NS
  # 或
  kubectl delete clusterrolebinding BINDING_NAME
  ```

#### REM-002: 补充 Role 规则
- **适用根因**: RC-002, RC-011
- **前置检查**:
  ```bash
  # 查看当前 Role 规则
  kubectl get role ROLE_NAME -n NS -o yaml
  # 确认需要添加的权限
  kubectl api-resources | grep RESOURCE_NAME
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 使用 kubectl patch 添加规则
  kubectl patch role ROLE_NAME -n NS --type='json' -p='[
    {"op": "add", "path": "/rules/-", "value": {
      "apiGroups": ["apps"],
      "resources": ["deployments"],
      "verbs": ["get", "list", "watch", "create", "update", "patch", "delete"]
    }}
  ]'
  
  # 或导出-编辑-应用
  kubectl get role ROLE_NAME -n NS -o yaml > role-backup.yaml
  # 编辑后
  kubectl apply -f role-updated.yaml
  ```
- **后置验证**:
  ```bash
  kubectl auth can-i create deployments --as=system:serviceaccount:NS:SA_NAME -n NS
  # 预期: yes
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f role-backup.yaml
  ```

#### REM-003: 调整 ResourceQuota 限额
- **适用根因**: RC-003, RC-009
- **前置检查**:
  ```bash
  # 查看当前配额使用情况
  kubectl describe quota -n NS
  # 确认集群有足够资源
  kubectl top nodes
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案一：增加配额上限
  kubectl patch resourcequota QUOTA_NAME -n NS --type='merge' -p='{"spec":{"hard":{"requests.cpu":"10","requests.memory":"20Gi","pods":"50"}}}'
  
  # 方案二：清理已完成的 Pod 释放配额
  kubectl delete pods -n NS --field-selector=status.phase=Succeeded
  kubectl delete pods -n NS --field-selector=status.phase=Failed
  ```
- **后置验证**:
  ```bash
  kubectl describe quota QUOTA_NAME -n NS
  # 预期: used < hard
  # 尝试创建资源
  kubectl run test-pod --image=nginx -n NS --dry-run=server
  # 预期: pod/test-pod created (dry run)
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 恢复原配额值
  kubectl patch resourcequota QUOTA_NAME -n NS --type='merge' -p='{"spec":{"hard":{"requests.cpu":"ORIGINAL_VALUE"}}}'
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-004: 修正 LimitRange 配置
- **适用根因**: RC-004
- **影响说明**: 修改 LimitRange 会影响该 Namespace 内所有新创建的 Pod 的默认资源配置。已运行的 Pod 不受影响。
- **审批提示**: "建议修改 Namespace `<NS>` 的 LimitRange 配置。该操作会影响后续创建的所有 Pod 的默认资源限制。是否批准？"
- **前置检查**:
  ```bash
  kubectl describe limitrange -n NS
  kubectl get pods -n NS -o jsonpath='{range .items[*]}{.metadata.name}{": CPU="}{.spec.containers[*].resources.requests.cpu}{", Memory="}{.spec.containers[*].resources.requests.memory}{"\n"}{end}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 备份当前配置
  kubectl get limitrange -n NS -o yaml > limitrange-backup.yaml
  
  # 更新 LimitRange
  kubectl patch limitrange LIMITRANGE_NAME -n NS --type='merge' -p='
  {
    "spec": {
      "limits": [{
        "type": "Container",
        "default": {"cpu": "500m", "memory": "512Mi"},
        "defaultRequest": {"cpu": "100m", "memory": "128Mi"},
        "max": {"cpu": "2", "memory": "4Gi"},
        "min": {"cpu": "50m", "memory": "64Mi"}
      }]
    }
  }'
  ```
- **后置验证**:
  ```bash
  kubectl describe limitrange -n NS
  # 尝试创建符合新限制的 Pod
  kubectl run test-pod --image=nginx -n NS --dry-run=server
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f limitrange-backup.yaml
  ```

#### REM-005: 清理过期 ServiceAccount Token
- **适用根因**: RC-005
- **影响说明**: 重建 Pod 会导致短暂中断，但会获得新的有效 Token。对于长期运行的 Pod，需要评估重启影响。
- **审批提示**: "建议重启 Pod `<POD_NAME>` 以获取新的 ServiceAccount Token。该操作会导致 Pod 短暂中断。是否批准？"
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 确认 Token 确实无效
  kubectl exec -it POD_NAME -n NS -- cat /var/run/secrets/kubernetes.io/serviceaccount/token | cut -d'.' -f2 | base64 -d 2>/dev/null | jq -r '.exp | . - now | . < 0'
  # 检查 Pod 的重启策略
  kubectl get pod POD_NAME -n NS -o jsonpath='{.spec.restartPolicy}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 方案一：删除并重建 Pod（适用于 Deployment/StatefulSet 管理的 Pod）
  kubectl delete pod POD_NAME -n NS
  
  # 方案二：手动创建新 Token（K8s 1.24+）
  kubectl create token SA_NAME -n NS --duration=3600s > new-token.txt
  # 通过应用程序配置使用新 Token
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查新 Pod 的 Token 有效性
  kubectl exec -it NEW_POD_NAME -n NS -- curl -sk \
    -H "Authorization: Bearer $(cat /var/run/secrets/kubernetes.io/serviceaccount/token)" \
    https://kubernetes.default.svc/api/v1/namespaces
  # 预期: 返回 Namespace 列表而非 Unauthorized
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # Pod 删除后会自动重建（如果由控制器管理）
  # 如果是手动创建的 Pod，需要从备份恢复
  kubectl apply -f pod-backup.yaml
  ```

#### REM-006: 修复 Aggregated ClusterRole
- **适用根因**: RC-006
- **影响说明**: 修改 ClusterRole 标签会影响聚合规则的匹配，可能改变其他依赖该 ClusterRole 的权限配置。
- **审批提示**: "建议修复 Aggregated ClusterRole `<ROLE_NAME>` 的标签选择器。该操作可能影响其他依赖此 Role 的权限绑定。是否批准？"
- **前置检查**:
  ```bash
  # 查看聚合规则
  kubectl get clusterrole PARENT_ROLE -o yaml | grep -A10 aggregationRule
  # 查看子 Role 的标签
  kubectl get clusterrole -l rbac.authorization.k8s.io/aggregate-to-edit=true
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # 方案一：为子 Role 添加正确标签
  kubectl label clusterrole CHILD_ROLE rbac.authorization.k8s.io/aggregate-to-PARENT=true
  
  # 方案二：修改聚合规则的选择器
  kubectl patch clusterrole PARENT_ROLE --type='merge' -p='
  {
    "aggregationRule": {
      "clusterRoleSelectors": [{
        "matchLabels": {
          "rbac.authorization.k8s.io/aggregate-to-PARENT": "true"
        }
      }]
    }
  }'
  ```
- **后置验证**:
  ```bash
  # 检查聚合后的规则
  kubectl get clusterrole PARENT_ROLE -o yaml | grep -A50 "rules:"
  # 验证权限
  kubectl auth can-i --list --as=system:serviceaccount:NS:SA
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  kubectl label clusterrole CHILD_ROLE rbac.authorization.k8s.io/aggregate-to-PARENT-
  ```

#### REM-007: 调整 OPA/Gatekeeper 约束
- **适用根因**: RC-007
- **影响说明**: 修改策略约束会影响整个集群的安全合规检查，可能允许之前被阻止的资源创建。
- **审批提示**: "建议调整 Gatekeeper 约束 `<CONSTRAINT_NAME>` 以允许资源 `<RESOURCE>` 创建。该操作会放宽安全策略。是否批准？"
- **前置检查**:
  ```bash
  # 查看约束详情
  kubectl get constraint CONSTRAINT_NAME -o yaml
  # 查看违规详情
  kubectl get constraint CONSTRAINT_NAME -o jsonpath='{.status.violations}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案一：添加例外（排除特定 Namespace）
  kubectl patch constraint CONSTRAINT_NAME --type='merge' -p='
  {
    "spec": {
      "match": {
        "excludedNamespaces": ["NAMESPACE_TO_EXCLUDE"]
      }
    }
  }'
  
  # 方案二：将约束改为审计模式
  kubectl patch constraint CONSTRAINT_NAME --type='merge' -p='
  {
    "spec": {
      "enforcementAction": "dryrun"
    }
  }'
  
  # 方案三：调整约束参数
  kubectl patch constraint CONSTRAINT_NAME --type='merge' -p='
  {
    "spec": {
      "parameters": {
        "allowedRegistries": ["docker.io", "gcr.io", "your-registry.com"]
      }
    }
  }'
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 重新尝试创建资源
  kubectl apply -f RESOURCE.yaml --dry-run=server
  # 检查约束状态
  kubectl get constraint CONSTRAINT_NAME -o jsonpath='{.status.totalViolations}'
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 恢复原约束配置
  kubectl apply -f constraint-backup.yaml
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-008: 紧急临时赋予 cluster-admin（带审计）
- **适用根因**: RC-001, RC-002, RC-008（紧急情况）
- **影响说明**: 赋予 cluster-admin 权限等同于超级管理员，可以执行任何操作。这是一个**高风险的临时措施**，必须在问题解决后立即撤销。
- **操作步骤**:
  1. **创建审计记录**:
     ```bash
     # 记录操作原因和时间
     echo "$(date): Emergency cluster-admin granted to SA_NAME by OPERATOR for REASON" >> /var/log/rbac-emergency.log
     ```
  2. **创建临时 ClusterRoleBinding**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

     ```bash
     kubectl create clusterrolebinding emergency-admin-SA_NAME \
       --clusterrole=cluster-admin \
       --serviceaccount=NS:SA_NAME \
       --dry-run=client -o yaml > emergency-binding.yaml
     # 添加注释说明
     kubectl annotate -f emergency-binding.yaml \
       "emergency.k8s.io/reason=REASON" \
       "emergency.k8s.io/operator=OPERATOR_NAME" \
       "emergency.k8s.io/expires=$(date -d '+2 hours' +%Y-%m-%dT%H:%M:%SZ)" \
       --local -o yaml > emergency-binding-annotated.yaml
     kubectl apply -f emergency-binding-annotated.yaml
     ```
  3. **设置自动过期提醒**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 2小时后提醒撤销
     echo "kubectl delete clusterrolebinding emergency-admin-SA_NAME" | at now + 2 hours
     ```
  4. **验证并执行紧急操作**
  5. **问题解决后立即撤销**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     kubectl delete clusterrolebinding emergency-admin-SA_NAME
     ```
- **安全检查**:
  - 确保操作被完整记录到审计日志
  - 通知安全团队此次紧急授权
  - 设置 2 小时自动过期提醒
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete clusterrolebinding emergency-admin-SA_NAME
  ```

#### REM-009: 批量 Quota 重置与资源清理
- **适用根因**: RC-003, RC-009
- **影响说明**: 批量删除资源可能导致服务中断，需要评估被删除资源的影响范围。
- **操作步骤**:
  1. **评估影响范围**:
     ```bash
     # 列出将被清理的资源
     kubectl get pods -n NS --field-selector=status.phase=Succeeded -o name
     kubectl get pods -n NS --field-selector=status.phase=Failed -o name
     kubectl get pods -n NS -o jsonpath='{range .items[?(@.status.phase=="Evicted")]}{.metadata.name}{"\n"}{end}'
     ```
  2. **备份重要信息**:
     ```bash
     kubectl get pods -n NS -o yaml > pods-backup.yaml
     kubectl get events -n NS > events-backup.txt
     ```
  3. **执行清理**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 清理已完成的 Pod
     kubectl delete pods -n NS --field-selector=status.phase=Succeeded
     # 清理失败的 Pod
     kubectl delete pods -n NS --field-selector=status.phase=Failed
     # 清理被驱逐的 Pod
     kubectl get pods -n NS -o json | jq -r '.items[] | select(.status.phase=="Evicted") | .metadata.name' | xargs -r kubectl delete pod -n NS
     ```
  4. **重置配额（如需要）**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     kubectl delete resourcequota QUOTA_NAME -n NS
     kubectl apply -f new-quota.yaml
     ```
- **安全检查**:
  - 确认被删除的 Pod 确实不再需要
  - 评估是否有 Job/CronJob 正在运行
  - 检查 PVC 是否会随 Pod 删除
- **回滚方案**:
  ```bash
  # Pod 删除后无法直接恢复，需要重新创建
  # 如果是控制器管理的 Pod，会自动重建
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-010: 多租户 RBAC 完整重构
- **适用根因**: RC-008, RC-010, RC-012（结构性问题）
- **审批要求**: 需要安全团队 + 平台 Team Lead 审批
- **数据备份**: 
  ```bash
  # 完整备份现有 RBAC 配置
  kubectl get roles,rolebindings,clusterroles,clusterrolebindings -A -o yaml > rbac-full-backup.yaml
  ```
- **操作步骤**:
  1. **分析现有权限结构**:
     ```bash
     # 生成权限矩阵报告
     kubectl get rolebindings -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{.subjects[*].name}{"\t"}{.roleRef.name}{"\n"}{end}' > rbac-matrix.txt
     ```
  2. **设计新的权限模型**:
     - 定义租户隔离边界
     - 设计 Role 层级结构
     - 确定聚合规则
  3. **逐步迁移**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # 创建新的 Role/ClusterRole
     kubectl apply -f new-roles/
     # 创建新的 Binding（保留旧 Binding）
     kubectl apply -f new-bindings/
     # 验证新权限
     kubectl auth can-i --list --as=system:serviceaccount:NS:SA
     # 确认无误后删除旧 Binding
     kubectl delete -f old-bindings/
     ```
  4. **验证与监控**:
     ```bash
     # 监控 403 错误
     kubectl get events -A --field-selector reason=Forbidden --watch
     ```
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f rbac-full-backup.yaml
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# V1: 确认权限已生效
kubectl auth can-i VERB RESOURCE --as=system:serviceaccount:NS:SA -n TARGET_NS
# 预期: yes

# V2: 确认资源可以创建
kubectl run test-verify --image=nginx -n TARGET_NS --dry-run=server
# 预期: pod/test-verify created (dry run)

# V3: 确认配额有剩余空间
kubectl describe quota -n TARGET_NS | grep -E "^(Name:|cpu|memory|pods)"
# 预期: used < hard

# V4: 确认无新的 403 错误
kubectl get events -n TARGET_NS --field-selector reason=Forbidden --sort-by=.lastTimestamp | tail -5
# 预期: 无新增 Forbidden 事件

# V5: 确认 ServiceAccount Token 有效
kubectl exec -it POD_NAME -n NS -- curl -sk \
  -H "Authorization: Bearer $(cat /var/run/secrets/kubernetes.io/serviceaccount/token)" \
  https://kubernetes.default.svc/api/v1/namespaces/NS
# 预期: 返回 Namespace 详情
```

### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| 403 错误率 | `apiserver_request_total{code="403"}` | 下降并稳定 | 修复后 403 错误持续增加 |
| 配额使用率 | `kube_resourcequota{type="used"} / kube_resourcequota{type="hard"}` | 低于 90% | 使用率持续攀升并再次接近 100% |
| Pod 创建成功率 | `kubectl get events --field-selector reason=FailedCreate` | 无新增 | 出现新的 FailedCreate 事件 |
| Webhook 拒绝率 | `apiserver_admission_webhook_rejection_count` | 下降或稳定 | 拒绝数持续增加 |
| ServiceAccount 认证 | `apiserver_authentication_attempts{result="success"}` | 稳定或上升 | success 率下降 |
| 策略违规数 | `kubectl get constraints -o jsonpath='{.items[*].status.totalViolations}'` | 不增加 | violations 数量增加 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] `kubectl auth can-i` 对受影响主体返回 `yes`
- [ ] 资源创建操作成功执行（非 dry-run）
- [ ] ResourceQuota 使用率低于 90%
- [ ] 无新增 403 Forbidden 事件
- [ ] 无新增 admission denied 事件
- [ ] CI/CD 流水线（如受影响）恢复正常执行
- [ ] 审计日志中无新的权限拒绝记录
- [ ] 根因已明确记录并采取了预防措施

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| RBAC 权限稳定性 | 监控 403 错误趋势 | 每小时 | 如果 403 错误复现 → 重新进入本 Skill 诊断流程 |
| 配额使用趋势 | `kube_resourcequota` 指标趋势图 | 每 4 小时 | 使用率线性增长 → 排查资源泄漏或配额规划问题 |
| Token 有效性 | 检查 ServiceAccount token 过期时间 | 每日 | Token 即将过期 → 提前规划 Pod 滚动更新 |
| 策略变更 | 监控 Constraint/Policy 变更事件 | 每 4 小时 | 新策略导致现有资源不合规 → 评估影响 |
| 新增 RoleBinding | `kubectl get rolebinding -A --sort-by=.metadata.creationTimestamp` | 每日 | 异常的新 Binding → 检查是否为授权操作 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 3 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V5 验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（如更多 Namespace 受影响） | 诊断过程中发现问题范围扩大 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因（RC-001 至 RC-012） | 所有诊断步骤均无明确异常发现 |
| **策略死锁** | 多个策略相互冲突，无法找到满足所有约束的配置 | D3.3-D3.5 显示策略冲突 |
| **安全疑虑** | 诊断过程中发现可疑权限提升尝试或未授权访问模式 | 审计日志中发现异常行为 |

### 8.2 升级消息模板

```
【{severity}】RBAC 权限与 ResourceQuota 故障诊断 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {principal_type} {principal_name} 在 {namespace} 中执行 {operation} 操作失败
- 错误类型: {error_type} (403 Forbidden / Quota Exceeded / Admission Denied)
- 影响范围: 
  - 受影响主体: {affected_principals}
  - 受影响 Namespace: {affected_namespaces}
  - 是否影响 CI/CD: {cicd_affected}
- 已完成诊断:
  - Phase 1 RBAC 诊断: {phase1_summary}
  - Phase 2 Quota 诊断: {phase2_summary}
  - Phase 3 Admission 诊断: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-SEC-002 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤（D1.1 → D1.2 → ... → D3.5）及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
   - 例: "RC-003 已排除 — D2.1 显示配额使用率 42%，低于阈值"
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
4. **关键资源快照**:
   ```bash
   # RBAC 相关
   kubectl get rolebinding,clusterrolebinding -A -o wide | grep PRINCIPAL > rbac-bindings.txt
   kubectl get role,clusterrole -A | grep ROLE_NAME > roles.txt
   # Quota 相关
   kubectl describe quota -n NS > quota-status.txt
   kubectl describe limitrange -n NS > limitrange-status.txt
   # 策略相关
   kubectl get constraints -o yaml > constraints.txt
   kubectl get clusterpolicy,policy -A -o yaml > policies.txt
   # 事件和审计
   kubectl get events -A --field-selector reason=Forbidden > forbidden-events.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| Bound ServiceAccount Tokens | GA（默认） | GA | GA | GA | GA |
| ServiceAccount Token 自动挂载 | 默认 true | 默认 true | 默认 true | 默认 true | 默认 true |
| Token Request API | GA | GA | GA | GA | GA |
| LegacyServiceAccountTokenNoAutoGeneration | beta（默认启用） | GA | GA | GA | GA |
| ServiceAccountTokenNodeBindingValidation | alpha | beta | beta | GA | GA |
| Token Cleanup Controller | alpha | beta | beta | GA | GA |
| ValidatingAdmissionPolicy | beta | beta | GA | GA | GA |
| Aggregated ClusterRole 改进 | 基础 | 改进 | 改进 | 稳定 | 稳定 |
| ResourceQuota scopeSelector | GA | GA | GA | GA | GA |
| Kyverno ValidatingAdmissionPolicy 支持 | 1.10 | 1.11 | 1.12 | 1.13 | 1.13+ |
| Gatekeeper Mutation 支持 | 3.13 | 3.14 | 3.15 | 3.16 | 3.17 |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl auth can-i --list` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl auth whoami` | beta | GA | GA | GA | GA |
| `kubectl create token` | GA | GA | GA | GA | GA |
| `kubectl get constraints` (Gatekeeper) | 需安装 | 需安装 | 需安装 | 需安装 | 需安装 |
| `kubectl get clusterpolicy` (Kyverno) | 需安装 | 需安装 | 需安装 | 需安装 | 需安装 |
| ValidatingAdmissionPolicy 调试 | 有限 | 改进 | GA 级支持 | 增强 | 增强 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Role/ClusterRole | rbac.authorization.k8s.io/v1 | v1 | v1 | v1 | v1 |
| RoleBinding/ClusterRoleBinding | rbac.authorization.k8s.io/v1 | v1 | v1 | v1 | v1 |
| ResourceQuota | v1 (core) | v1 | v1 | v1 | v1 |
| LimitRange | v1 (core) | v1 | v1 | v1 | v1 |
| ServiceAccount | v1 (core) | v1 | v1 | v1 | v1 |
| ValidatingAdmissionPolicy | admissionregistration.k8s.io/v1beta1 | v1beta1 | v1 | v1 | v1 |
| TokenRequest | authentication.k8s.io/v1 | v1 | v1 | v1 | v1 |

### 9.4 版本相关的诊断注意事项

- **[v1.24+]**: ServiceAccount 不再自动创建长期 Secret。使用 projected token（bound token），默认 1 小时有效期，自动轮转。诊断时注意检查 token 挂载方式：
  ```bash
  kubectl get pod POD -o yaml | grep -A20 "serviceAccountToken"
  ```

- **[v1.30+]**: Token Cleanup Controller 进入 beta，会自动清理过期的 ServiceAccount tokens。如果 Pod 长期运行（>1小时），可能需要重启获取新 token。

- **[v1.30+]**: ValidatingAdmissionPolicy GA，可能替代部分 Webhook 功能。诊断时需同时检查：
  ```bash
  kubectl get validatingadmissionpolicy
  kubectl get validatingadmissionpolicybinding
  ```

- **[v1.31+]**: ServiceAccountTokenNodeBindingValidation GA，token 绑定到特定 Node，跨节点使用会失败。

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 Admission Webhook 拒绝误判为 RBAC 问题** | 错误信息显示 Forbidden，初步判断为权限不足 | 实际是 OPA/Gatekeeper/Kyverno 策略拦截，错误信息中包含 "admission webhook denied" | 仔细阅读完整错误信息，检查是否包含 "webhook" 关键字。先执行 D3.1-D3.5 排除策略拦截 |
| **将 Namespace 隔离问题误判为 Role 缺失** | 用户在 NS-A 有权限但在 NS-B 返回 Forbidden | RoleBinding 仅存在于 NS-A，用户试图跨 Namespace 访问 | 确认操作的目标 Namespace，检查该 NS 是否有对应的 RoleBinding |
| **将配额问题误判为权限问题** | Pod 创建失败显示 Forbidden | ResourceQuota 耗尽时也会返回 Forbidden | 检查完整错误信息中是否包含 "exceeded quota"，执行 D2.1 检查配额 |
| **将 LimitRange 默认值影响误判为 Quota 问题** | Pod 实际资源超出预期导致 quota 不足 | LimitRange 的 defaultRequest/defaultLimit 被应用 | 对比 Pod spec 与实际 Pod 的资源配置，检查 LimitRange 默认值 |
| **将 API Group 不匹配误判为权限缺失** | Role 中有 deployments 权限但仍返回 Forbidden | Role 中 apiGroups 为 `""` 但 deployments 在 `apps` 组 | 使用 `kubectl api-resources` 确认资源所属 API Group |
| **将 Token 过期问题误判为 Role 问题** | ServiceAccount 有权限但 Pod 内访问 API 失败 | Bound token 已过期（默认 1 小时） | 检查 Pod 启动时间和 token 有效期，考虑重启 Pod |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| RBAC 架构与权限模型 | `domain-05-security-compliance/07-rbac-matrix-configuration.md` | 理解 Role、ClusterRole、Binding 的完整设计 |
| ResourceQuota 与 LimitRange | `domain-10-troubleshooting-diagnostics/12-rbac-quota-troubleshooting.md` | 深度配额问题排查 |
| OPA/Gatekeeper 策略引擎 | `domain-05-security-compliance/14-policy-engines-opa-kyverno.md` | 理解策略引擎的工作原理 |
| ServiceAccount Token 机制 | `domain-05-security-compliance/01-authentication-authorization-system.md` | Token 生命周期和轮转机制 |
| 审计日志分析 | `domain-05-security-compliance/04-audit-logging-compliance.md` | 分析 403 错误的审计日志 |
| 多租户安全架构 | `domain-05-security-compliance/21-multicluster-security.md` | 跨租户隔离问题 |
| 控制平面 API Server | `domain-01-cluster-fundamentals/` | API Server 认证授权流程 |
| Pod 安全标准 | `domain-05-security-compliance/06-pod-security-standards.md` | PSA 与 RBAC 的交互 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、10 个修复操作 | 基于 RBAC/Quota 相关工单分析，确定为安全类高优先级场景 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **ValidatingAdmissionPolicy 深度诊断**: v1.30+ 原生策略引擎的诊断流程
2. **跨集群 RBAC 联邦**: 多集群环境下的权限同步问题
3. **动态准入控制器调试**: 复杂 Webhook 链的诊断方法
4. **OIDC/LDAP 集成问题**: 外部身份提供商导致的认证失败
5. **审计日志深度分析**: 基于审计日志的权限问题根因定位
6. **Istio AuthorizationPolicy**: 服务网格层面的 RBAC 问题

---

## 附录 A: 常用诊断命令速查

### A.1 RBAC 诊断命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# === 权限检查 ===
# 检查特定操作权限
kubectl auth can-i create pods --as=system:serviceaccount:NS:SA -n TARGET_NS

# 列出所有权限
kubectl auth can-i --list --as=system:serviceaccount:NS:SA

# 检查集群管理员权限
kubectl auth can-i '*' '*' --as=USER

# 查看当前身份
kubectl auth whoami

# === RoleBinding 查询 ===
# 查找 ServiceAccount 的所有绑定
kubectl get rolebinding,clusterrolebinding -A -o wide | grep SA_NAME

# 查看绑定详情
kubectl describe rolebinding BINDING_NAME -n NS

# 查找绑定特定 Role 的所有 Binding
kubectl get rolebinding,clusterrolebinding -A -o yaml | grep -B10 "name: ROLE_NAME"

# === Role/ClusterRole 查询 ===
# 查看 Role 规则
kubectl get role ROLE_NAME -n NS -o yaml

# 查看 ClusterRole 规则
kubectl get clusterrole ROLE_NAME -o yaml

# 列出所有聚合 ClusterRole
kubectl get clusterrole -o yaml | grep -B5 "aggregationRule:"

# === ServiceAccount 诊断 ===
# 查看 ServiceAccount 详情
kubectl get sa SA_NAME -n NS -o yaml

# 检查 Pod 使用的 ServiceAccount
kubectl get pod POD_NAME -n NS -o jsonpath='{.spec.serviceAccountName}'

# 创建临时 Token（K8s 1.24+）
kubectl create token SA_NAME -n NS --duration=3600s
```

### A.2 Quota/LimitRange 诊断命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# === ResourceQuota 检查 ===
# 查看命名空间配额
kubectl get resourcequota -n NS
kubectl describe resourcequota -n NS

# 计算配额使用率
kubectl get resourcequota -n NS -o jsonpath='{range .items[*]}{.metadata.name}{": "}{.status.used.requests\.cpu}{"/"}{.status.hard.requests\.cpu}{"\n"}{end}'

# 查看所有命名空间配额摘要
kubectl get resourcequota -A -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,USED_CPU:.status.used.requests\.cpu,HARD_CPU:.status.hard.requests\.cpu

# === LimitRange 检查 ===
# 查看 LimitRange 配置
kubectl describe limitrange -n NS

# 查看 LimitRange YAML
kubectl get limitrange -n NS -o yaml

# === 资源使用统计 ===
# 查看 Pod 资源使用
kubectl top pods -n NS --sort-by=memory

# 统计 Pod 资源请求
kubectl get pods -n NS -o jsonpath='{range .items[*]}{.metadata.name}{" CPU: "}{.spec.containers[*].resources.requests.cpu}{" Memory: "}{.spec.containers[*].resources.requests.memory}{"\n"}{end}'

# 清理已完成 Pod
kubectl delete pods -n NS --field-selector=status.phase=Succeeded
kubectl delete pods -n NS --field-selector=status.phase=Failed
```

### A.3 Admission Controller 诊断命令

```bash
# === Webhook 配置检查 ===
# 列出 ValidatingWebhook
kubectl get validatingwebhookconfiguration

# 列出 MutatingWebhook
kubectl get mutatingwebhookconfiguration

# 查看 Webhook 详情
kubectl describe validatingwebhookconfiguration WEBHOOK_NAME

# === OPA/Gatekeeper 检查 ===
# 列出所有约束
kubectl get constraints

# 查看约束违规情况
kubectl get constraints -o jsonpath='{range .items[*]}{.kind}{"/"}{.metadata.name}{": violations="}{.status.totalViolations}{"\n"}{end}'

# 查看约束模板
kubectl get constrainttemplate

# 查看 Gatekeeper 审计日志
kubectl logs -n gatekeeper-system -l control-plane=audit-controller --tail=100

# === Kyverno 检查 ===
# 列出所有策略
kubectl get clusterpolicy,policy -A

# 查看策略报告
kubectl get policyreport,clusterpolicyreport -A

# 查看 Kyverno 日志
kubectl logs -n kyverno -l app=kyverno --tail=100

# === ValidatingAdmissionPolicy (v1.30+) ===
kubectl get validatingadmissionpolicy
kubectl get validatingadmissionpolicybinding
```

---

## 附录 B: RBAC 配置模板

### B.1 只读权限 Role

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: production
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log", "pods/status"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["services", "endpoints", "configmaps"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets", "statefulsets"]
  verbs: ["get", "list", "watch"]
```

### B.2 开发者权限 Role

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: developer
  namespace: development
rules:
- apiGroups: ["", "apps", "batch"]
  resources: ["*"]
  verbs: ["*"]
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses", "networkpolicies"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
- apiGroups: ["autoscaling"]
  resources: ["horizontalpodautoscalers"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
```

### B.3 CI/CD ServiceAccount 配置

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cicd-deployer
  namespace: production
automountServiceAccountToken: true
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: deployer
  namespace: production
rules:
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["configmaps", "secrets", "services"]
  verbs: ["get", "list", "watch", "create", "update", "patch"]
- apiGroups: ["batch"]
  resources: ["jobs"]
  verbs: ["get", "list", "watch", "create", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: cicd-deployer-binding
  namespace: production
subjects:
- kind: ServiceAccount
  name: cicd-deployer
  namespace: production
roleRef:
  kind: Role
  name: deployer
  apiGroup: rbac.authorization.k8s.io
```

### B.4 多租户 Namespace 隔离配置

```yaml
# 为每个租户创建独立的 ResourceQuota
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-quota
  namespace: tenant-a
spec:
  hard:
    requests.cpu: "10"
    requests.memory: "20Gi"
    limits.cpu: "20"
    limits.memory: "40Gi"
    pods: "50"
    services: "20"
    persistentvolumeclaims: "10"
    secrets: "50"
    configmaps: "50"
---
# 配套的 LimitRange
apiVersion: v1
kind: LimitRange
metadata:
  name: tenant-limits
  namespace: tenant-a
spec:
  limits:
  - type: Container
    default:
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:
      cpu: "100m"
      memory: "128Mi"
    max:
      cpu: "2"
      memory: "4Gi"
    min:
      cpu: "50m"
      memory: "64Mi"
  - type: Pod
    max:
      cpu: "4"
      memory: "8Gi"
```

---

## 附录 C: 故障诊断脚本

### C.1 RBAC 权限审计脚本

```bash
#!/bin/bash
# rbac_audit.sh - RBAC 权限审计工具

echo "=== RBAC 权限审计报告 ==="
echo "生成时间: $(date)"
echo ""

# 1. 检查 cluster-admin 绑定
echo "--- 1. Cluster-Admin 绑定检查 ---"
kubectl get clusterrolebinding -o jsonpath='{range .items[?(@.roleRef.name=="cluster-admin")]}{.metadata.name}{"\t"}{.subjects[*].name}{"\n"}{end}'
echo ""

# 2. 检查过度授权的 Role
echo "--- 2. 过度授权检查（verbs: *）---"
kubectl get role,clusterrole -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\n"}{range .rules[*]}{"  verbs: "}{.verbs}{" resources: "}{.resources}{"\n"}{end}{end}' | grep -E "\*"
echo ""

# 3. 检查未使用的 RoleBinding
echo "--- 3. ServiceAccount 绑定统计 ---"
kubectl get rolebinding,clusterrolebinding -A -o jsonpath='{range .items[*]}{range .subjects[?(@.kind=="ServiceAccount")]}{.namespace}{"/"}{.name}{"\n"}{end}{end}' | sort | uniq -c | sort -rn | head -10
echo ""

# 4. 检查无 Binding 的 ServiceAccount
echo "--- 4. 无权限绑定的 ServiceAccount ---"
for sa in $(kubectl get sa -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\n"}{end}' | grep -v "default" | head -20); do
  ns=$(echo $sa | cut -d'/' -f1)
  name=$(echo $sa | cut -d'/' -f2)
  bindings=$(kubectl get rolebinding,clusterrolebinding -A -o jsonpath="{range .items[*]}{range .subjects[?(@.name==\"$name\")]}{.name}{end}{end}" 2>/dev/null)
  if [ -z "$bindings" ]; then
    echo "  $sa - 无绑定"
  fi
done
```

### C.2 Quota 使用率监控脚本

```bash
#!/bin/bash
# quota_monitor.sh - ResourceQuota 使用率监控

THRESHOLD=${1:-80}  # 默认告警阈值 80%

echo "=== ResourceQuota 使用率报告 ==="
echo "告警阈值: ${THRESHOLD}%"
echo ""

kubectl get resourcequota -A -o json | jq -r '
  .items[] | 
  .metadata.namespace as $ns |
  .metadata.name as $name |
  .status.hard as $hard |
  .status.used as $used |
  ($hard | to_entries[]) as $h |
  ($used[$h.key] // "0") as $u |
  {
    namespace: $ns,
    quota: $name,
    resource: $h.key,
    used: $u,
    hard: $h.value
  } | 
  "\(.namespace)/\(.quota): \(.resource) = \(.used)/\(.hard)"
' | while read line; do
  used=$(echo $line | grep -oP '\d+(?=/)' | tail -1)
  hard=$(echo $line | grep -oP '(?<=/)[\d\.]+' | tail -1)
  if [ -n "$used" ] && [ -n "$hard" ] && [ "$hard" != "0" ]; then
    pct=$((used * 100 / ${hard%.*}))
    if [ $pct -ge $THRESHOLD ]; then
      echo "⚠️  $line ($pct%)"
    else
      echo "✅ $line ($pct%)"
    fi
  fi
done
```

### C.3 策略冲突检测脚本

```bash
#!/bin/bash
# policy_conflict_check.sh - 策略冲突检测

echo "=== 策略冲突检测报告 ==="
echo ""

# 检查 Gatekeeper 约束
if kubectl get constraints &>/dev/null; then
  echo "--- Gatekeeper 约束状态 ---"
  kubectl get constraints -o jsonpath='{range .items[*]}{.kind}{"/"}{.metadata.name}{" enforcement="}{.spec.enforcementAction}{" violations="}{.status.totalViolations}{"\n"}{end}'
  echo ""
  
  # 检查高违规约束
  echo "--- 高违规约束（>10）---"
  kubectl get constraints -o json | jq -r '.items[] | select(.status.totalViolations > 10) | "\(.kind)/\(.metadata.name): \(.status.totalViolations) violations"'
fi

# 检查 Kyverno 策略
if kubectl get clusterpolicy &>/dev/null; then
  echo ""
  echo "--- Kyverno 策略状态 ---"
  kubectl get clusterpolicy -o jsonpath='{range .items[*]}{.metadata.name}{" action="}{.spec.validationFailureAction}{" ready="}{.status.ready}{"\n"}{end}'
  
  # 检查策略报告中的失败
  echo ""
  echo "--- Kyverno 策略违规 ---"
  kubectl get policyreport -A -o json 2>/dev/null | jq -r '.items[] | .results[]? | select(.result=="fail") | "\(.policy): \(.message)"' | head -10
fi

echo ""
echo "检测完成"
```

## Related

- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]

```