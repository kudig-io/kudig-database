---
title: Pod 一直 Pending，无法调度 — 远程顾问对话脚本
summary: Pod 一直 Pending，无法调度 — 远程顾问对话脚本：kubectl describe pod <pod-name> -n <namespace>
category: dialogue
tags:
- dialogue
- remote-consultant
- troubleshooting
- visibility/public
tier: supporting
created: 2026-05-21
updated: 2026-05-21
dialogue_id: DIALOGUE-SCHEDULER-001
skill_id: SKILL-SCHEDULER-001
role: remote-consultant
language: zh
severity: high
status: reviewed
last_updated: 2026-05-21
---



# Pod 一直 Pending，无法调度 — 远程顾问对话脚本

> 对应概念：[[concepts/kube-scheduler.md|Kubernetes Scheduler]]
> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

---

## 对话入口

**工程师**：Pod 一直处于 Pending 状态，无法调度到节点上运行。

**顾问回应**：收到。请先确认：该 Pod 的名称、命名空间，以及是否为新创建的 Pod 还是之前正常运行的 Pod？

---

### 步骤 1: 查看 Pod 事件

**顾问**：请查看该 Pod 的详细事件信息：

```bash
kubectl describe pod <pod-name> -n <namespace>
```

> **如果无法执行**：请通过控制台查看 Pod 详情页的 Events 标签，或提供 describe 输出的截图。

```bash
kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name> --sort-by='.lastTimestamp'
```

> **如果无法执行**：请搜索该命名空间下最近 1 小时内与目标 Pod 相关的所有 Events。

**预期用户回复**：Events 中出现 `FailedScheduling`、`0/3 nodes are available`、`Insufficient cpu/memory`、`Taint` 或 `Affinity` 相关错误。

**下一步判断**：
- 若 Insufficient cpu/memory → 进入步骤 2 检查节点资源
- 若 Taint 相关 → 进入步骤 3 检查 taints
- 若 Affinity/Anti-affinity → 进入步骤 4 检查 affinity
- 若 Unbound PVC → 进入步骤 5 检查 PVC

---

### 步骤 2: 检查节点资源

**顾问**：请检查集群节点的资源使用情况：

```bash
kubectl describe nodes
```

> **如果无法执行**：请通过控制台查看节点资源监控，或执行 `kubectl get nodes -o yaml` 查看 allocatable 和 allocated 资源。

```bash
kubectl top nodes
```

> **如果无法执行**：请查看集群监控大盘中的节点 CPU 和内存使用率，确认是否有节点的资源已耗尽。

**预期用户回复**：所有节点的 `Allocated resources` 中 CPU 或内存已接近 `Allocatable` 上限，或节点处于 NotReady 状态。

**下一步判断**：
- 若资源不足 → 进入步骤 6 修复方案（增加节点）
- 若节点 NotReady → 提示检查节点健康状态
- 若节点资源充足 → 进入步骤 3 检查 taints

---

### 步骤 3: 检查节点 Taints

**顾问**：请检查集群节点的 Taints 配置：

```bash
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints
```

> **如果无法执行**：请逐个执行 `kubectl describe node <node-name>` 并查看 Taints 字段。

```bash
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.tolerations}'
```

> **如果无法执行**：请执行 `kubectl describe pod <pod-name> -n <namespace>` 查看 Tolerations 字段。

**预期用户回复**：节点上存在 NoSchedule Taint（如 `node-role.kubernetes.io/master:NoSchedule`），但 Pod 未配置对应的 toleration。

**下一步判断**：
- 若存在不匹配的 Taint → 进入步骤 6 修复方案（添加 toleration）
- 若 Taint 匹配 → 进入步骤 4 检查 affinity

---

### 步骤 4: 检查 Affinity 配置

**顾问**：请检查 Pod 的亲和性配置：

```bash
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 20 affinity
```

> **如果无法执行**：请执行 `kubectl describe pod <pod-name> -n <namespace>` 查看 NodeSelector、NodeAffinity、PodAffinity 和 PodAntiAffinity 配置。

```bash
kubectl get nodes --show-labels | grep <required-label-key>
```

> **如果无法执行**：请逐个检查节点标签是否满足 Pod 的 nodeSelector 或 nodeAffinity 要求。

**预期用户回复**：Pod 配置了 `requiredDuringSchedulingIgnoredDuringExecution` 的 nodeAffinity，但集群中没有节点匹配所需标签；或 PodAntiAffinity 导致同一拓扑域下无法共存。

**下一步判断**：
- 若 affinity 条件不满足 → 进入步骤 6 修复方案（放宽 affinity）
- 若 affinity 正常 → 进入步骤 5 检查 PVC

---

### 步骤 5: 检查 PVC 绑定状态

**顾问**：请检查 Pod 引用的 PVC 状态：

```bash
kubectl get pvc -n <namespace>
```

> **如果无法执行**：请执行 `kubectl describe pod <pod-name> -n <namespace>` 查看 Volumes 部分引用的 PVC 名称，然后单独检查该 PVC。

```bash
kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.status.phase}'
```

> **如果无法执行**：请查看 PVC 的 STATUS 列是否为 `Pending`，以及 Events 中是否有绑定失败的错误。

```bash
kubectl get pv | grep <pvc-name>
```

> **如果无法执行**：请检查集群中是否有可用的 PV 可以匹配该 PVC 的 storageClass 和容量要求。

**预期用户回复**：PVC 处于 Pending 状态，没有匹配的 PV；或 StorageClass 的 provisioner 配置错误导致无法动态供应。

**下一步判断**：
- 若 PVC Pending → 进入步骤 6 修复方案（创建 PV 或修改 StorageClass）
- 若 PVC 正常 → 提示检查 scheduler 组件是否正常运行

---

### 步骤 6: 提供修复方案

**顾问**：根据以上排查，请按对应根因执行修复：

#### 方案 A：增加节点或清理资源

```bash
kubectl top pods -n <namespace> --sort-by=cpu
```

> **如果无法执行**：请通过控制台或监控系统找出资源占用高的 Pod，评估是否可以删除或缩容。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
kubectl cordon <node-name> && kubectl drain <node-name> --ignore-daemonsets
```

> **如果无法执行**：请向集群管理员申请扩容节点，或通过云厂商控制台添加新的工作节点。

#### 方案 B：添加 Toleration

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch pod <pod-name> -n <namespace> --type='merge' -p='{"spec":{"tolerations":[{"key":"<taint-key>","operator":"Equal","value":"<taint-value>","effect":"NoSchedule"}]}}'
```

> **如果无法执行**：请修改 Deployment/StatefulSet 的 Pod 模板，在 spec.template.spec.tolerations 中添加对应的 toleration 后重新部署。

#### 方案 C：放宽 Affinity

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch deployment <deployment-name> -n <namespace> --type='merge' -p='{"spec":{"template":{"spec":{"affinity":{"nodeAffinity":{"requiredDuringSchedulingIgnoredDuringExecution":null}}}}}}'
```

> **如果无法执行**：请使用 `kubectl edit deployment <deployment-name> -n <namespace>` 将 `requiredDuringSchedulingIgnoredDuringExecution` 改为 `preferredDuringSchedulingIgnoredDuringExecution`，或删除不必要的 nodeSelector。

#### 方案 D：等待 PVC 绑定

```bash
kubectl get pvc <pvc-name> -n <namespace> -w
```

> **如果无法执行**：请检查 StorageClass 的 provisioner 是否正常运行，或手动创建匹配的 PV 后观察 PVC 是否自动绑定。

**验证修复**：

```bash
kubectl get pod <pod-name> -n <namespace> -w
```

> **如果无法执行**：请间歇性执行 `kubectl get pod <pod-name> -n <namespace>`，确认 Pod 状态从 Pending 变为 Running。

---

## 相关概念

- [[concepts/kube-scheduler.md|Kubernetes Scheduler]]
- [[concepts/node-taint.md|节点 Taint 与 Toleration]]
- [[concepts/pod-affinity.md|Pod 亲和性与反亲和性]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
