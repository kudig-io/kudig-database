---
title: Pod 异常故障树分析 (skills)
description: '### 1. 调度失败/挂起'
summary: '### 1. 调度失败/挂起'
category: general
tags:
- k8s
- scheduler
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod 异常故障树分析 是什么
- 如何 Pod 异常故障树分析
trigger_keywords:
- Pod
- 异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-POD-001
component: Pod
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "Pod 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[pods|pods]] -A --field-selector=status.phase!=Running -o jsonpath='{range .items[?(@.status.phase!=\'Running\')]} {.metadata.namespace}/{.metadata.name}{\'\..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/FTA故障树/list/pod-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Pod 异常故障树分析

### 诊断命令速查表

> 本表列出 FTA 树各节点的实际诊断命令，供 SRE 手工执行或 AI Agent 自动化调用。
> 变量说明: `${POD_NAME}` - Pod 名称 | `${NAMESPACE}` - 命名空间 | `${NODE_NAME}` - 节点名称 | `${CONTAINER_NAME}` - 容器名称

### 1. 调度失败/挂起

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|---------|------------|------|
| `cat_scheduling` | 调度失败分类 | `kubectl get pod ${POD_NAME} -n ${NAMESPACE} -o jsonpath='{.status.phase}'` | `Pending` | → 进入调度子树 |
| | | `kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${POD_NAME},reason=FailedScheduling -o jsonpath='{.items[-1:].message}'` | 包含 `no nodes available` | → 进入调度子树 |
| `evt_node_unready` | 节点不可用/污点 | `kubectl get nodes -o json | jq '[.items[] | select(.status.conditions[] | .type=="Ready" and .status=="True")] | length'` | `0` | **确认根因** |
| | | `kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${POD_NAME},reason=FailedScheduling -o json | jq -r '.items[-1].message'` | 包含 `had taint` 或 `didn't match` | **确认根因** |
| `evt_resource_insufficient` | 资源不足 | `kubectl describe nodes | grep -A 5 'Allocated resources'` | CPU/Memory 接近 100% | **确认根因** |
| | | `kubectl get events -n ${NAMESPACE} --field-selector reason=FailedScheduling -o json | jq -r '.items[].message' | grep 'Insufficient'` | 包含 `Insufficient cpu/memory` | **确认根因** |
| `evt_affinity_conflict` | 亲和性冲突 | `kubectl get pod ${POD_NAME} -n ${NAMESPACE} -o json | jq '.spec.affinity'` | 配置了 `requiredDuringScheduling` | 进一步检查 |
| | | `kubectl describe pod ${POD_NAME} -n ${NAMESPACE} | grep "didn't match pod affinity"` | 包含亲和性不匹配信息 | **确认根因** |
| `evt_scheduler_down` | 调度器异常 | `kubectl get pods -n kube-system -l component=kube-scheduler -o wide` | Pod 非 Running | **确认根因** |
| `evt_ns_quota` | 配额限制 | `kubectl describe quota -n ${NAMESPACE}` | Used 接近 Hard | **确认根因** |
| `evt_node_selector_conflict` | 节点选择器冲突 | `kubectl get pod ${POD_NAME} -n ${NAMESPACE} -o json | jq '.spec.nodeSelector'` | 有 nodeSelector | 检查匹配 |
| | | `kubectl get nodes --show-labels | grep '<label-key>'` | 
...(截断)

## 生产案例

### 案例1: 资源不足导致大规模 Pending

**时间线**:
- 09:00 业务团队批量提交 200 个 Pod（各请求 4C8G）
- 09:01 集群可分配资源耗尽，150 个 Pod Pending
- 09:05 告警触发，运维介入
- 09:15 确认 Cluster Autoscaler 未触发（节点池达到上限）
- 09:20 临时调高节点池上限，新节点加入后 Pod 调度成功

**根因链**:
```
批量提交 → 资源耗尽 → FailedScheduling(Insufficient cpu/memory)
→ CA未扩容(节点池max限制) → Pod持续Pending
```

**修复**:
```bash
# 🟢 查看调度失败原因
kubectl get events -n ${NS} --field-selector reason=FailedScheduling --sort-by='.lastTimestamp' | tail -20
# 🟡 调整节点池上限
kubectl patch nodepool ${POOL} -p '{"spec":{"maxSize":50}}'
```

### 案例2: 镜像拉取失败导致 CrashLoopBackOff

**现象**: Pod 状态 ImagePullBackOff，`describe` 显示 `rpc error: code = Unknown desc = Error response from daemon: unauthorized`

**根因**: 私有镜像仓库 Secret 过期，imagePullSecrets 引用的 token 已失效

**修复**:
```bash
# 🟡 更新 imagePullSecret
kubectl create secret docker-registry regcred --docker-server=${REGISTRY} --docker-username=${USER} --docker-password=${TOKEN} -n ${NS} --dry-run=client -o yaml | kubectl apply -f -
# 🟢 验证
kubectl delete pod ${POD} -n ${NS}  # 触发重建
kubectl get pod ${POD} -n ${NS} -w
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: pod-alerts
  rules:
  - alert: PodPendingTooLong
    expr: kube_pod_status_phase{phase="Pending"} == 1
    for: 10m
    labels:
      severity: warning
  - alert: PodCrashLooping
    expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
    for: 5m
    labels:
      severity: critical
  - alert: PodNotReady
    expr: kube_pod_status_ready{condition="true"} == 0
    for: 15m
    labels:
      severity: warning
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| 资源配置合理性审查 | requests 不超过节点可分配量的 80% | P0 |
| 镜像拉取失败重试 | 配置 imagePullPolicy + 镜像缓存 | P0 |
| PDB 保护 | 避免维护时全部 Pod 被驱逐 | P1 |
| 节点池自动扩容 | CA 配置合理的 scale-up 触发条件 | P1 |

## 面试要点

1. **Q: Pod Pending 的常见原因和排查步骤？**
   A: 资源不足(Insufficient cpu/memory) → 节点污点(taint) → 亲和性不匹配 → PVC未绑定 → 调度器异常。用 `kubectl describe pod` 查看 Events

2. **Q: CrashLoopBackOff 的排查思路？**
   A: `kubectl logs --previous` 查看上次崩溃日志 → 检查 livenessProbe 配置 → 验证资源限制(OOMKilled) → 检查依赖服务可达性

3. **Q: 如何避免滚动更新时的服务中断？**
   A: 配置合理的 maxUnavailable/maxSurge → readinessProbe 确保新 Pod 就绪后再终止旧 Pod → preStop hook 优雅关闭 → PDB 保证最小可用数

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[troubleshoot-pod-issues|Pod 故障排查]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[19-故障诊断/06-FTA故障树/list/pod-fta.md|Pod FTA 完整版]]


<!-- risk-assessed -->
