---
title: Pod 异常故障树分析 (skills)
description: '### 1. 调度失败/挂起'
summary: '### 1. 调度失败/挂起'
category: general
tags:
- k8s
- scheduler
- agent
tier: peripheral
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
summary: "<!-- condition: kubectl get [[Pods|pods]] -A --field-selector=status.phase!=Running -o jsonpath='{range .items[?(@.status.phase!=\'Running\')]} {.metadata.namespace}/{.metadata.name}{\'\..."
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

### 案例 1: Pod CrashLoopBackOff——配置错误导致启动失败

| 时间 | 事件 |
|------|------|
| 09:00 | 新版本发布后 Pod CrashLoopBackOff |
| 09:05 | `kubectl logs pod-name --previous` 显示 "FATAL: invalid database URL" |
| 09:08 | ConfigMap 中数据库连接串格式错误 |
| 09:10 | 🟡 修复 ConfigMap，`kubectl rollout restart deployment/app` |
| 09:15 | Pod 正常启动 |

**根因**: 配置变更未经过测试环境验证直接应用到生产。

### 案例 2: Pod Pending——资源请求超出节点可分配量

**现象**: Pod 持续 Pending，Events: "0/5 nodes are available: 5 Insufficient memory"。

**诊断**: `kubectl describe nodes | grep -A5 "Allocated resources"` 显示 memory 已分配 95%

**修复**: 🟡 降低 Pod memory request 或扩容节点

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 核心服务 Pod 全部不可用 | 立即回滚 + 检查配置 |
| P1 | 部分 Pod 异常 | 检查日志和事件 |
| P2 | Pod 重启偶发 | 优化探针配置 |

## 面试要点

1. **Q: Pod 的生命周期状态有哪些？**
   A: Pending(调度中/拉取镜像) → Running(至少一个容器运行) → Succeeded/Failed(终止)。异常状态: CrashLoopBackOff、ImagePullBackOff、Evicted、OOMKilled。

2. **Q: Pod 被驱逐的常见原因？**
   A: ① 节点资源压力(DiskPressure/MemoryPressure) ② 超出 cgroup limit ③ 节点维护(drain) ④ 优先级抢占 ⑤ 节点 NotReady 超时。

3. **Q: 如何设计健康的探针配置？**
   A: livenessProbe: 检测死锁，失败重启容器；readinessProbe: 检测就绪，失败从 Service 移除；startupProbe: 保护慢启动应用。建议: initialDelaySeconds 充足、periodSeconds 不要太短、failureThreshold 3-5。

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[troubleshoot-pod-issues|Pod 故障排查]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[生态参考/领域索引/pod-index.md|Pod 知识图谱索引]]


<!-- risk-assessed -->
