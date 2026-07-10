---
title: RBAC 权限不足导致应用无法访问 K8s API
description: 专有云 ACK 集群中 Operator 应用升级后无法 List/Watch CRD，根因为 ServiceAccount 绑定 Role
  缺少 verbs，含诊断、修复与验证。
summary: 专有云 ACK 集群中 Operator 应用升级后无法 List/Watch CRD，根因为 ServiceAccount 绑定 Role 缺少
  verbs，含诊断、修复与验证。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- rbac
- serviceaccount
- role
- rolebinding
- operator
- p1
tier: supporting
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:10:00+08:00'
incident_id: TC-2026-039
priority: P1
severity: high
affected_cluster: ack-zyy-prod-06
affected_namespace: platform-tools
ticket_type: 安全策略故障
skill_ref:
- RBAC 故障诊断
- 最小权限原则
fta_ref:
- 'FTA: RBAC 访问拒绝'
last_updated: 2026-06-26 16:10:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- RBAC 权限不足导致应用无法访问 K8s API 如何处理
trigger_keywords:
- ack
- zyy
- rbac
- serviceaccount
- role
prerequisites:
- kubectl-basics
- k8s-security
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[生产运维/ticket-cases/ticket-case-005-kubelet-cert-expired.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[系统基础/topic-dictionary/security/rbac.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在 ACK 专有云集群 `ack-zyy-prod-06` 的 `platform-tools` 命名空间升级内部 Operator 后，Operator Pod 反复重启，日志中大量 `forbidden` 错误。客户描述如下：

> “我们的 backup-operator 今天升级到 v2.3.0 后启动不起来了，看日志一直在报 `User "system:serviceaccount:platform-tools:backup-operator" cannot list resource "backups" in API group "backup.example.io" at the cluster scope` 这种错误。v2.2 的时候是好的，升级后多了几个 CRD。我们是不是漏配 RBAC 了？这个 Operator 管着我们所有命名空间的备份任务，挺关键的。”

该 Operator 负责全集群备份任务调度，当前因权限不足无法启动，备份任务已暂停。

## 分类与优先级判定

- **工单类型**：安全策略故障 / RBAC 权限不足。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境关键 Operator 因权限不足无法启动，备份任务中断。
2. 报错明确指向 RBAC 规则缺少对新增 CRD 的权限。
3. 需要在 30 分钟内修复 RBAC 并恢复 Operator。

## 诊断步骤

按“先日志、后 ServiceAccount、再 Role/RoleBinding”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Operator Pod 状态与日志
kubectl get pod -n platform-tools -l app=backup-operator
kubectl logs -n platform-tools -l app=backup-operator --tail=200 | grep -i "forbidden|denied|RBAC" | tail -30

# 2. 查看 ServiceAccount
kubectl get sa backup-operator -n platform-tools -o yaml

# 3. 查看 Role/RoleBinding 或 ClusterRole/ClusterRoleBinding
kubectl get role,rolebinding,clusterrole,clusterrolebinding | grep backup-operator
kubectl describe role backup-operator -n platform-tools
kubectl describe rolebinding backup-operator -n platform-tools

# 4. 检查新增 CRD 所需的权限
kubectl get crd | grep backup.example.io
kubectl describe crd backups.backup.example.io | grep -A 5 Names

# 5. 使用 auth can-i 验证权限缺失
kubectl auth can-i list backups.backup.example.io \
  --as=system:serviceaccount:platform-tools:backup-operator \
  -n platform-tools

kubectl auth can-i list backups.backup.example.io \
  --as=system:serviceaccount:platform-tools:backup-operator \
  --all-namespaces

# 6. 检查 Operator 部署清单中的 RBAC 模板
kubectl get deployment backup-operator -n platform-tools -o yaml | grep -A 5 serviceAccountName

# 7. 查看 apiserver 审计日志中 RBAC 拒绝记录（如有审计日志集成）
kubectl logs -n kube-system -l component=kube-apiserver --tail=500 | grep "backup-operator" | grep "forbidden" | tail -20
```
## 根因分析

`backup-operator` v2.3.0 新增了集群级 CRD `backups.backup.example.io`、`restores.backup.example.io` 和 `backuppolicies.backup.example.io`，但部署时只创建了 namespace 级别的 `Role` 和 `RoleBinding`，未授予 Operator 跨命名空间 list/watch 这些 CRD 的权限。

报错信息：

```
User "system:serviceaccount:platform-tools:backup-operator" cannot list resource "backups" in API group "backup.example.io" at the cluster scope
```

根因置信度：**高**。

### 风险与影响评估

- **业务影响：** 备份 Operator 无法启动，全集群定时备份任务暂停，若在此期间发生数据误删或数据库故障，将无法按预期 RPO 恢复，存在数据保护缺口。
- **扩散风险：** 若 Operator 依赖其他新增资源（如 secrets、configmaps 跨命名空间读取）权限也缺失，可能在修复一个错误后继续触发新的 forbidden 日志，需要完整校验所有 verbs。
- **数据风险：** 不涉及数据丢失，但备份窗口延误可能导致下一次全量备份堆积，影响后续备份链路与存储成本。
- **恢复关键：** 必须授予 Operator 对新增 CRD 的集群级权限，同时遵循最小权限原则，避免一次性授予 cluster-admin。

## 修复命令

**第一步：创建 ClusterRole，授予 Operator 对新增 CRD 的集群级权限**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<'EOF' | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: backup-operator-crds
rules:
- apiGroups: ["backup.example.io"]
  resources: ["backups", "restores", "backuppolicies"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: ["backup.example.io"]
  resources: ["backups/status", "restores/status", "backuppolicies/status"]
  verbs: ["get", "update", "patch"]
- apiGroups: [""]
  resources: ["events"]
  verbs: ["create", "patch"]
EOF
```
**第二步：创建 ClusterRoleBinding，将 ClusterRole 绑定到 Operator 的 ServiceAccount**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<'EOF' | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: backup-operator-crds
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: backup-operator-crds
subjects:
- kind: ServiceAccount
  name: backup-operator
  namespace: platform-tools
EOF
```
**第三步：重启 Operator Pod 以重新加载权限**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment backup-operator -n platform-tools
kubectl rollout status deployment backup-operator -n platform-tools --timeout=180s
```
**第四步：验证权限已生效**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl auth can-i list backups.backup.example.io \
  --as=system:serviceaccount:platform-tools:backup-operator \
  --all-namespaces
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. Operator Pod 全部 Running 且重启次数不再增加
kubectl get pod -n platform-tools -l app=backup-operator -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.status.containerStatuses[0].restartCount}{"\n"}{end}'

# 2. Operator 日志无 forbidden 错误
kubectl logs -n platform-tools -l app=backup-operator --tail=100 | grep -i "forbidden|denied" || echo "无权限拒绝日志"

# 3. 验证各新增 CRD 的权限
for resource in backups restores backuppolicies; do
  echo -n "$resource list: "
  kubectl auth can-i list ${resource}.backup.example.io \
    --as=system:serviceaccount:platform-tools:backup-operator \
    --all-namespaces
  echo -n "$resource create: "
  kubectl auth can-i create ${resource}.backup.example.io \
    --as=system:serviceaccount:platform-tools:backup-operator \
    --all-namespaces
done

# 4. 确认 Operator 能正常 list CRD
kubectl exec -n platform-tools deploy/backup-operator -- \
  wget -qO- --header "Authorization: Bearer $(kubectl exec -n platform-tools deploy/backup-operator -- cat /var/run/secrets/kubernetes.io/serviceaccount/token)" \
  --no-check-certificate \
  https://kubernetes.default.svc/apis/backup.example.io/v1/backups 2>/dev/null | head -c 200

# 5. 检查 ClusterRoleBinding 已正确绑定
kubectl get clusterrolebinding backup-operator-crds -o yaml
```
## 回复客户话术

> 您好，工单 TC-2026-039 已处理完成。
>
> **现象确认：** `platform-tools/backup-operator` 升级至 v2.3.0 后反复重启，日志提示对 `backups.backup.example.io` 等新增 CRD 的 list/watch 权限被拒绝。
>
> **根因：** v2.3.0 引入了集群级 CRD（backups/restores/backuppolicies），但现有 RBAC 仅包含 namespace 级的 Role/RoleBinding，未授予 Operator 跨命名空间访问这些新资源的权限，导致启动时无法 list/watch CRD。
>
> **已执行修复：**
> 1. 创建 ClusterRole `backup-operator-crds`，授予新增 CRD 的 get/list/watch/create/update/patch/delete 权限；
> 2. 创建 ClusterRoleBinding 将该 ClusterRole 绑定到 `platform-tools/backup-operator` ServiceAccount；
> 3. 滚动重启 Operator，Pod 已稳定 Running，日志无 forbidden 错误。
>
> **当前状态：** Operator 启动成功，权限校验全部通过，备份任务已恢复调度。
>
> **后续建议：**
> - 在 Operator 升级流程中增加 RBAC 变更检查，确保新增 CRD 与 verbs 同步更新；
> - 将 RBAC 模板纳入 GitOps 管理，变更前通过 `kubectl auth can-i` 做权限基线校验；
> - 遵循 最小权限原则，仅授予 Operator 必需的资源与 verbs，避免过度授权；
> - 对关键 Operator 配置 readiness/liveness probe，及时发现启动期权限问题。
>
> 如有异常请随时联系。

## 复盘与沉淀

本次故障是 Operator 升级过程中 RBAC 同步滞后的典型案例。Operator 通常需要 list/watch 自定义资源，这些资源可能是命名空间级也可能是集群级。当 Operator 新增了对某个 CRD 的集群级 watch 时，原有的 Role/RoleBinding 就无法满足需求，必须使用 ClusterRole/ClusterRoleBinding。很多团队在升级 Operator 时只更新了 Deployment 镜像，却忽略了 CRD 与 RBAC 的配套升级。

排查时，`kubectl auth can-i` 是最直接的验证工具。通过 `--as=system:serviceaccount:<namespace>:<sa>` 可以精确模拟 Operator 的权限，快速定位缺少的 verbs。相比直接阅读 Role YAML，auth can-i 更能反映实际授权结果，尤其是在存在多个 Role/ClusterRole 叠加时。

在安全层面，修复时应避免为了快速恢复而直接给 ServiceAccount 绑定 cluster-admin。本次修复创建了最小权限的 ClusterRole，仅授予新增 CRD 的必要 verbs，并单独授予 events 的 create/patch 权限，符合最小权限原则。后续可以通过 Pod Security Admission 或 OPA Gatekeeper 进一步限制 Operator 的运行时行为。

建议建立以下长效机制：
1. **Operator 升级 checklist：** 明确列出 CRD、RBAC、Webhook、ServiceAccount 等必须同步变更的项；
2. **RBAC 基线测试：** 在 CI 中集成 `kubectl auth can-i` 校验脚本，对关键 ServiceAccount 进行权限回归；
3. **RBAC 版本管理：** 将 ClusterRole/ClusterRoleBinding 与 Operator 部署清单放在同一 Git 仓库，确保同步发布；
4. **审计日志分析：** 定期分析 apiserver 审计日志中的 RBAC 拒绝事件，发现潜在权限缺口。

另外，在专有云 ACK 多租户场景下，RBAC 的正确性直接影响集群安全边界。错误的 RoleBinding 可能让某个命名空间的服务账户获得跨命名空间权限，而过于宽泛的 ClusterRole 则可能被攻击者利用进行横向移动。因此，建议每季度进行一次 RBAC 权限审计，重点检查是否有过期 ServiceAccount、冗余 ClusterRole 以及跨命名空间的不必要绑定。对于备份 Operator 这类关键组件，还应启用令牌轮换与最小权限 ServiceAccount，避免长期使用固定 ServiceAccount Token。

## 是否需要升级及交接信息

- **是否升级**：否（已闭环）。若后续 Operator 升级仍频繁出现 RBAC 遗漏，需升级至 **安全与平台工程团队** 建立 RBAC 变更门禁。
- **交接信息**：
  - 故障单号：`TC-2026-039`
  - 根因：Operator 新增 CRD 缺少集群级 RBAC 权限
  - 影响集群：`ack-zyy-prod-06`
  - 影响命名空间：`platform-tools`
  - 临时修复：创建 ClusterRole 与 ClusterRoleBinding
  - 长期方案：将 RBAC 纳入 Operator 升级 checklist 与 GitOps 门禁
  - 待跟进：确认备份任务调度正常，更新 Operator 升级 SOP

## Related

- 证书过期导致 kubelet 无法连接 apiserver
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- 基于角色的访问控制


<!-- risk-assessed -->
