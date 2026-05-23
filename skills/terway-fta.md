---
title: Terway 异常故障树分析 (skills)
description: Terway 异常故障树分析 — Kubernetes 生产运维知识库
category: general
tags:
- k8s
- statefulset
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 异常故障树分析 是什么
- 如何 Terway 异常故障树分析
trigger_keywords:
- Terway
- 异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-TERWAY-001
component: Terway
severity: high
created: "2026-05-23"
---

---
title: "Terway 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -n kube-system -l app=terway -o jsonpath='{range .items[?(@.status.phase!='Running')]} {.metadata.name}{\'\n\'}{end}' 显示 Terway 异常 --> - **目标**：..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["domain-10-troubleshooting-diagnostics/topic-fta/list/terway-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Terway 异常故障树分析

### 诊断命令速查表

> 本表列出 FTA 树各节点的实际诊断命令，供 SRE 手工执行或 AI Agent 自动化调用。
> 变量说明: `${NODE_NAME}` - 节点名称 | `${NAMESPACE}` - 命名空间 | `${POD_NAME}` - Pod 名称 | `${INSTANCE_ID}` - ECS 实例 ID | `${VSWITCH_ID}` - 交换机 ID
> 注：部分命令需要 aliyun CLI 和相应 RAM 权限；terway-cli 命令需在 Terway Pod 内执行

### 1. ENI 分配异常

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|---------|------------|------|
| `cat_eni` | ENI 异常分类 | `kubectl get events -n ${NAMESPACE} --field-selector reason=FailedCreatePodSandBox -o json \| jq '[.items[] \| select(.message \| test("ENI\|bindquota\|AttachNetworkInterface"))] \| length'` | `> 0` | → 进入 ENI 子树 |
| `evt_eni_quota` | ENI 配额不足 | `aliyun ecs DescribeInstances --InstanceIds '["${INSTANCE_ID}"]' \| jq '.Instances.Instance[0].NetworkInterfaces.NetworkInterface \| length'` | 达到实例类型上限 | **确认根因** |
| | | `kubectl logs -n kube-system -l app=terway --tail=50 \| grep -E "bindquota exceeded\|no available ENI slot"` | 包含配额超限 | **确认根因** |
| `evt_eni_bind_fail` | ENI 绑定失败 | `kubectl logs -n kube-system -l app=terway --tail=50 \| grep -E "AttachNetworkInterface failed\|bindENI failed"` | 包含绑定失败 | **确认根因** |
| | | `aliyun ecs DescribeNetworkInterfaces --InstanceId ${INSTANCE_ID} \| jq '.NetworkInterfaceSets.NetworkInterfaceSet[] \| {id: .NetworkInterfaceId, status: .Status}'` | ENI 状态非 InUse | 进一步检查 |
| `evt_eni_drift` | ENI 状态漂移 | `kubectl exec -n kube-system $(kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].metadata.name}') -- terway-cli show` | 与云平台 ENI 列表不匹配 | **确认根因** |
| | | `aliyun ecs DescribeNetworkInterfaces --InstanceId ${INSTANCE_ID} --Status Detaching \| jq '.NetworkInterfaceSets.NetworkInterfaceSet \| length'` | 有 Detaching 状态 ENI | **确认根因** |

### 2. IP 地址池异常

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|---------|------------|------|
| `cat_ip` | IP 异常分类 | `kubectl get events -n ${NAMESPACE} --field-selector reason=FailedCreatePodSandBox -o json \| jq '[.items[] \| select(.message \| test("IP\|pool\|address"))] \| length'` | `> 0` | → 进入 IP 子树 |

...(截断)

## 相关链接

- [[FTA Methodology and Core Principles|FTA 方法论]]
- [[FTA Diagnostic Execution Engine|FTA 诊断执行引擎]]

## See Also

- [[skills/skills-run-README.md|skills-run-README]]
- [[skills/statefulset-fta.md|statefulset-fta]]
- [[skills/troubleshoot-node-issues.md|troubleshoot-node-issues]]
- [[skills/troubleshoot-pod-issues.md|troubleshoot-pod-issues]]

## Related

- [[domain-19-landscape-references/topic-index/terway-index|Terway 知识图谱索引]]
