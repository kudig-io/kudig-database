---
dialogue_id: "DIALOGUE-RBAC-001"
skill_id: "SKILL-RBAC-001"
role: "remote-consultant"
language: "zh"
severity: "medium"
status: "reviewed"
created: 2026-05-21
updated: 2026-05-21
---

# ServiceAccount 权限不足，无法创建 Pod — 远程顾问对话脚本

> 对应概念：[[concepts/rbac-authorization|RBAC 权限模型]]
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

---

## 对话入口

**工程师**：ServiceAccount 权限不足，应用无法创建 Pod，报错 `Forbidden`。

**顾问回应**：收到。请先确认：该 ServiceAccount 的名称和所在命名空间是什么？以及报错的完整错误信息是什么？

---

### 步骤 1: 确认身份和权限

**顾问**：请使用该 ServiceAccount 的身份验证权限：

```bash
kubectl auth can-i create pods --as=system:serviceaccount:<ns>:<sa> -n <ns>
```

> **如果无法执行**：请执行 `kubectl auth can-i create pods --as=system:serviceaccount:<ns>:<sa> --all-namespaces` 查看集群级别权限。

```bash
kubectl auth can-i --list --as=system:serviceaccount:<ns>:<sa> -n <ns>
```

> **如果无法执行**：请确认当前 kubectl 用户具有 `impersonate` 权限，否则请提供报错信息。

**预期用户回复**：权限检查结果为 `no`，或 `--list` 输出中缺少 `create pods` 权限。

**下一步判断**：
- 若权限为 no → 进入步骤 2 检查绑定
- 若权限为 yes 但操作仍失败 → 提示检查 admission webhook 或 PSP/PSA 限制

---

### 步骤 2: 检查 RoleBinding 和 ClusterRoleBinding

**顾问**：请检查该 ServiceAccount 绑定的角色：

```bash
kubectl get rolebinding,clusterrolebinding --all-namespaces -o json | jq -r '.items[] | select(.subjects[]?.name=="<sa>" and .subjects[]?.namespace=="<ns>") | "\(.kind): \(.metadata.name) in \(.metadata.namespace // "cluster")"'
```

> **如果无法执行**：请执行 `kubectl get rolebinding -n <ns>` 和 `kubectl get clusterrolebinding` 并搜索包含该 ServiceAccount 名称的绑定。

```bash
kubectl get rolebinding -n <ns> -o yaml | grep -B 5 -A 5 "name: <sa>"
```

> **如果无法执行**：请通过控制台查看该命名空间下的 RoleBinding 详情，确认 subjects 中是否包含目标 ServiceAccount。

**预期用户回复**：该 ServiceAccount 没有任何 RoleBinding 或 ClusterRoleBinding，或绑定的 Role 权限不足。

**下一步判断**：
- 若无绑定 → 进入步骤 6 修复方案（添加 rolebinding）
- 若有绑定 → 进入步骤 3 分析权限

---

### 步骤 3: 分析 Role 权限

**顾问**：请查看绑定的 Role 或 ClusterRole 的详细权限：

```bash
kubectl describe role <role-name> -n <ns>
```

> **如果无法执行**：请执行 `kubectl get role <role-name> -n <ns> -o yaml` 查看规则详情。

```bash
kubectl describe clusterrole <clusterrole-name>
```

> **如果无法执行**：请执行 `kubectl get clusterrole <clusterrole-name> -o yaml` 查看集群级别的权限规则。

**预期用户回复**：Role 中没有包含 `pods` 资源的 `create` 动词，或仅包含 `get`、`list` 等只读权限。

**下一步判断**：
- 若 Role 缺少 create pods → 进入步骤 4 验证动词和资源
- 若 Role 已包含但类型为 Role 而非 ClusterRole → 检查命名空间范围是否正确

---

### 步骤 4: 验证动词和资源

**顾问**：请对比所需的权限和实际的规则配置：

```bash
kubectl get role <role-name> -n <ns> -o jsonpath='{.rules}'
```

> **如果无法执行**：请手动查看 Role YAML 的 `rules` 部分，确认 `apiGroups`、`resources` 和 `verbs` 字段。

```bash
kubectl get clusterrole <clusterrole-name> -o jsonpath='{.rules}' | jq '.[] | select(.resources[]? | contains("pods"))'
```

> **如果无法执行**：请查看 ClusterRole 的 rules，搜索包含 `pods` 的条目，确认 verbs 列表是否包含 `create`。

**预期用户回复**：规则中 resources 为 `["pods"]` 但 verbs 仅包含 `["get", "list", "watch"]`，缺少 `"create"`。

**下一步判断**：
- 若缺少必要动词 → 进入步骤 6 修复方案（扩展 rules）
- 若资源名不正确（如用了 `pod` 而非 `pods`）→ 进入步骤 6 修复方案（修正资源名）

---

### 步骤 5: 检查 aggregationRule

**顾问**：如果使用的是聚合 ClusterRole，请检查 aggregationRule：

```bash
kubectl get clusterrole <clusterrole-name> -o yaml | grep -A 10 aggregationRule
```

> **如果无法执行**：请查看 ClusterRole YAML 中是否存在 `aggregationRule` 字段，以及 `clusterRoleSelectors` 匹配的标签。

```bash
kubectl get clusterrole -l <aggregation-label-key>=<aggregation-label-value>
```

> **如果无法执行**：请搜索带有对应标签的其他 ClusterRole，确认聚合是否正确收集了所需权限。

**预期用户回复**：aggregationRule 的 clusterRoleSelectors 标签不匹配，或没有带有对应标签的 ClusterRole 来提供 `pods` 的 `create` 权限。

**下一步判断**：
- 若 aggregationRule 不匹配 → 进入步骤 6 修复方案（修正标签或创建聚合角色）
- 若 aggregationRule 正常 → 提示检查 controller 是否正常运行

---

### 步骤 6: 提供修复方案

**顾问**：根据以上排查，请按对应根因执行修复：

#### 方案 A：添加 RoleBinding

```bash
kubectl create rolebinding <sa>-pod-creator \
  --role=<role-name> \
  --serviceaccount=<ns>:<sa> \
  -n <ns>
```

> **如果无法执行**：请将上述配置保存为 YAML 文件后执行 `kubectl apply -f rolebinding.yaml`。

#### 方案 B：扩展 Role 规则

```bash
kubectl patch role <role-name> -n <ns> --type='json' -p='[{"op": "add", "path": "/rules/-", "value": {"apiGroups":[""],"resources":["pods"],"verbs":["create","get","list","watch"]}}]'
```

> **如果无法执行**：请使用 `kubectl edit role <role-name> -n <ns>` 手动在 rules 列表末尾添加 Pod 创建权限。

#### 方案 C：使用 ClusterRole 绑定（跨命名空间）

```bash
kubectl create clusterrolebinding <sa>-cluster-pod-creator \
  --clusterrole=<clusterrole-name> \
  --serviceaccount=<ns>:<sa>
```

> **如果无法执行**：请将 ClusterRoleBinding 保存为 YAML 后 apply，或确认当前用户是否有创建 clusterrolebinding 的权限。

#### 方案 D：使用 cluster-admin（仅限测试）

```bash
kubectl create clusterrolebinding <sa>-admin \
  --clusterrole=cluster-admin \
  --serviceaccount=<ns>:<sa>
```

> **如果无法执行**：请确认当前用户是否为集群管理员，否则请向集群管理员申请授权。此方案仅限测试环境，生产环境请使用最小权限原则。

**验证修复**：

```bash
kubectl auth can-i create pods --as=system:serviceaccount:<ns>:<sa> -n <ns>
```

> **如果无法执行**：请重新运行之前的创建 Pod 操作，确认 `Forbidden` 错误已消除。

---

## 相关概念

- [[concepts/rbac-authorization|RBAC 权限模型]]
- [[concepts/rbac-authorization|ServiceAccount]]
- [[concepts/rbac-authorization|RoleBinding 与 ClusterRoleBinding]]
