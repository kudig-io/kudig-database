---
title: Pod 异常故障树分析 (skills)
description: '### 1. 调度失败/挂起'
category: general
tags:
- k8s
- scheduler
- agent
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
created: "2026-05-23"
---

---
title: "Pod 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -A --field-selector=status.phase!=Running -o jsonpath='{range .items[?(@.status.phase!=\'Running\')]} {.metadata.namespace}/{.metadata.name}{\'\..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md"]
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
| `evt_node_unready` | 节点不可用/污点 | `kubectl get nodes -o json \| jq '[.items[] \| select(.status.conditions[] \| .type=="Ready" and .status=="True")] \| length'` | `0` | **确认根因** |
| | | `kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${POD_NAME},reason=FailedScheduling -o json \| jq -r '.items[-1].message'` | 包含 `had taint` 或 `didn't match` | **确认根因** |
| `evt_resource_insufficient` | 资源不足 | `kubectl describe nodes \| grep -A 5 'Allocated resources'` | CPU/Memory 接近 100% | **确认根因** |
| | | `kubectl get events -n ${NAMESPACE} --field-selector reason=FailedScheduling -o json \| jq -r '.items[].message' \| grep 'Insufficient'` | 包含 `Insufficient cpu/memory` | **确认根因** |
| `evt_affinity_conflict` | 亲和性冲突 | `kubectl get pod ${POD_NAME} -n ${NAMESPACE} -o json \| jq '.spec.affinity'` | 配置了 `requiredDuringScheduling` | 进一步检查 |
| | | `kubectl describe pod ${POD_NAME} -n ${NAMESPACE} \| grep "didn't match pod affinity"` | 包含亲和性不匹配信息 | **确认根因** |
| `evt_scheduler_down` | 调度器异常 | `kubectl get pods -n kube-system -l component=kube-scheduler -o wide` | Pod 非 Running | **确认根因** |
| `evt_ns_quota` | 配额限制 | `kubectl describe quota -n ${NAMESPACE}` | Used 接近 Hard | **确认根因** |
| `evt_node_selector_conflict` | 节点选择器冲突 | `kubectl get pod ${POD_NAME} -n ${NAMESPACE} -o json \| jq '.spec.nodeSelector'` | 有 nodeSelector | 检查匹配 |
| | | `kubectl get nodes --show-labels \| grep '<label-key>'` | 
...(截断)

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[troubleshoot-pod-issues|Pod 故障排查]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[domain-19-landscape-references/topic-index/pod-index.md|Pod 知识图谱索引]]
