---
title: Pod Security Policy 咨询与迁移 — 远程顾问对话脚本
summary: Pod Security Policy 咨询与迁移 — 远程顾问对话脚本：kubectl version --short
category: dialogue
tags:
- dialogue
- remote-consultant
- troubleshooting
- visibility/public
tier: supporting
created: 2026-05-21
updated: 2026-05-21
dialogue_id: DIALOGUE-PSP-001
skill_id: SKILL-PSP-001
role: remote-consultant
language: zh
severity: medium
status: reviewed
last_updated: 2026-05-21
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Security Policy 咨询与迁移 — 远程顾问对话脚本

> 对应概念：[[concepts/pod-security-policy.md|Pod Security Policy]]  
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。  
> **⚠️ 重要提示**：Pod Security Policy 已于 Kubernetes v1.21 弃用，v1.25 正式移除。本脚本同时覆盖遗留环境排查和迁移指导。

---

## 对话入口

**工程师**：我们集群之前使用了 PSP，现在有些 Pod 创建失败，想确认是否是 PSP 导致，以及如何迁移。

**顾问回应**：收到。请先确认：当前集群的 Kubernetes 版本是多少？

---

### 步骤 1: 确认 Kubernetes 版本

**顾问**：请执行以下命令确认版本：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl version --short
```
> **如果无法执行**：请通过集群管理控制台查看版本信息，或提供 `kube-apiserver` 的镜像标签。

**预期用户回复**：版本为 v1.24.x 或 v1.25+，或更早版本。

**下一步判断**：
- 若 v1.25+ → 进入步骤 2（PSA 检查，PSP 已不可用）
- 若 v1.21–v1.24 → 进入步骤 3（PSP 排查）
- 若 <v1.21 → 进入步骤 3（PSP 仍可用但已弃用）

---

### 步骤 2: 检查 Pod Security Admission（v1.25+）

**顾问**：v1.25+ 已移除 PSP，请检查是否启用了 PSA：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ns -o yaml | grep -A 5 pod-security
```
> **如果无法执行**：请逐个检查关键命名空间：`kubectl get ns <name> -o yaml | grep pod-security`

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ns --show-labels
```
> **如果无法执行**：请提供命名空间列表和标签信息。

**预期用户回复**：命名空间带有 `pod-security.kubernetes.io/enforce: restricted` 等标签，或无任何 PSA 标签。

**下一步判断**：
- 若 PSA enforce=restricted 导致 Pod 失败 → 进入步骤 6 修复方案（调整 PSA 级别）
- 若未配置 PSA → 提示检查 Admission Webhook 或其他安全策略
- 若 <v1.25 → 进入步骤 3

---

### 步骤 3: 检查 PSP 资源（v1.24 及以下）

**顾问**：请检查集群中现有的 PSP 资源：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get psp
```
> **如果无法执行**：如果 kubectl 提示 `psp` 资源类型不存在，说明当前环境不支持 PSP（可能已移除或未启用）。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe psp <psp-name>
```
> **如果无法执行**：请提供 `kubectl get psp -o yaml` 的输出。

**预期用户回复**：PSP 存在，且某些策略字段（如 privileged=false、hostNetwork=false）阻止了 Pod 创建。

**下一步判断**：
- 若 PSP 阻止 Pod → 进入步骤 4（检查绑定关系）
- 若 PSP 不存在 → 进入步骤 2（检查 PSA 或其他机制）

---

### 步骤 4: 检查 PSP 绑定关系

**顾问**：请确认 ServiceAccount 是否绑定了正确的 PSP：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get rolebinding,clusterrolebinding --all-namespaces | grep psp
```
> **如果无法执行**：请检查目标命名空间下的 RoleBinding 和 ClusterRoleBinding。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl auth can-i use psp/<psp-name> --as=system:serviceaccount:<ns>:<sa>
```
> **如果无法执行**：请确认 ServiceAccount 名称和命名空间，替换后重试。

**预期用户回复**：ServiceAccount 未绑定任何 PSP，或绑定的 PSP 权限过于严格。

**下一步判断**：
- 若未绑定 → 进入步骤 6 修复方案（创建绑定）
- 若绑定但权限不足 → 进入步骤 5（评估现有 Pod 安全上下文）

---

### 步骤 5: 评估现有 Pod 的 securityContext

**顾问**：请检查失败 Pod 的安全上下文配置：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 20 securityContext
```
> **如果无法执行**：请提供 Pod 的 YAML 配置文件中 `spec.containers[].securityContext` 和 `spec.securityContext` 部分。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get events -n <namespace> | grep Forbidden
```
> **如果无法执行**：请查看该命名空间下最近的事件，寻找 `unable to validate against any pod security policy` 或 `Forbidden` 关键字。

**预期用户回复**：Pod 设置了 `privileged: true`、`hostNetwork: true` 或 `runAsRoot`，被 PSP/PSA 拒绝。

**下一步判断**：
- 若 Pod 需要特权但策略禁止 → 进入步骤 6 修复方案（调整策略或 Pod 配置）
- 若 Pod 无特权需求但被误拦截 → 进入步骤 6 修复方案（修正策略匹配）

---

### 步骤 6: 提供修复与迁移方案

**顾问**：根据以上排查，请按对应场景执行：

#### 方案 A：v1.25+ 调整 PSA 级别（推荐）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl label ns <namespace> pod-security.kubernetes.io/enforce=baseline --overwrite
```
> **如果无法执行**：请使用 `kubectl edit ns <namespace>` 手动修改标签。可选值：`privileged`（最宽松）、`baseline`（标准）、`restricted`（最严格）。

验证：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get ns <namespace> --show-labels
```
#### 方案 B：v1.24 及以下创建 PSP 绑定

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create clusterrolebinding psp-<name> --clusterrole=psp:<psp-name> --serviceaccount=<ns>:<sa>
```
> **如果无法执行**：请准备 ClusterRoleBinding YAML 文件并通过 `kubectl apply -f` 创建。

#### 方案 C：创建兼容的 Pod Security Policy

```yaml
apiVersion: policy/v1beta1
kind: PodSecurityPolicy
metadata:
  name: restricted-psp
spec:
  privileged: false
  runAsUser:
    rule: MustRunAsNonRoot
  seLinux:
    rule: RunAsAny
  fsGroup:
    rule: RunAsAny
  volumes:
    - 'configMap'
    - 'emptyDir'
    - 'projected'
    - 'secret'
    - 'downwardAPI'
    - 'persistentVolumeClaim'

```

> **如果无法执行**：请根据实际需求调整 `volumes` 和 `runAsUser` 规则，然后通过 `kubectl apply -f psp.yaml` 创建。

#### 方案 D：迁移到 Pod Security Admission（长期方案）

1. 审计现有命名空间的安全需求
2. 为每个命名空间设置合适的 PSA 标签：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

   ```bash
   kubectl label ns <ns> pod-security.kubernetes.io/enforce=baseline
   kubectl label ns <ns> pod-security.kubernetes.io/warn=restricted
   kubectl label ns <ns> pod-security.kubernetes.io/audit=restricted
   ```
3. 更新 Pod 的 `securityContext` 以满足 restricted 要求
4. 在 v1.25+ 集群上验证 Pod 可正常创建

---

## 相关概念

- [[concepts/pod-security-policy.md|Pod Security Policy]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-gitops-devops/index|安全合规索引]]

```

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
