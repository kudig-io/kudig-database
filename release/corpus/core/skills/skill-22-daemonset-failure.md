---
title: DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation (skills)
description: '| S5 | 新节点未自动部署 DaemonSet Pod | 新节点加入后检查 | 0.90 | 节点 NotReady → SKILL-NODE-001
  |'
summary: '| S5 | 新节点未自动部署 DaemonSet Pod | 新节点加入后检查 | 0.90 | 节点 NotReady → SKILL-NODE-001
  |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- daemonset
- ds
- 节点缺少 pod
- node missing pod
- flannel
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation 是什么
- 如何 DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation
trigger_keywords:
- DaemonSet
- 故障诊断与修复
- DaemonSet
- Failure
- Diagnosis
- Remediation
prerequisites:
- kubectl-basics
---



# [[DaemonSet|DaemonSet]] 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation

### 症状识别



### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 某些节点缺少 DaemonSet Pod | `kubectl get [[Pods|pods]] -n <ns> -l <selector> -o wide` | 0.95 | 节点被手动排除 → 检查操作记录 |
| S2 | DaemonSet DESIRED > CURRENT | `kubectl get ds -n <ns>` | 0.95 | 节点正在加入/退出 → 等待完成 |
| S3 | DaemonSet Pod CrashLoopBackOff | `kubectl get pods -n <ns> -l <selector>` | 0.90 | 应用自身 bug |
| S4 | DaemonSet 更新卡住 | `kubectl rollout status ds/<name>` | 0.90 | 应用启动慢 |
| S5 | 新节点未自动部署 DaemonSet Pod | 新节点加入后检查 | 0.90 | 节点 NotReady → SKILL-NODE-001 |
| S6 | 节点上系统功能缺失（如无日志/监控） | 可观测性告警 | 0.85 | 配置问题 |
| S7 | DaemonSet Pod 被驱逐 | `kubectl get events --field-selector reason=Evicted` | 0.85 | 节点资源压力 → SKILL-NODE-002 |
| S8 | 特权容器启动失败 | Pod Events 显示权限错误 | 0.80 | 安全策略限制 |

### 诊断工作流



### Phase 1: 快速检查（只读，零风险）

**Step D1.1**: 获取 DaemonSet 概览
- **命令**:
  ```bash
  kubectl get daemonset <name> -n <namespace> -o wide
  kubectl describe daemonset <name> -n <namespace> | head -40
  ```
- **超时**: 10s
- **判断规则**:
  - DESIRED > CURRENT → 有节点未调度（RC-001/003/004）
  - READY < CURRENT → 有 Pod 未就绪（RC-002/005/006/007/008）
  - UP-TO-DATE < CURRENT → 更新策略卡住（RC-009）

**Step D1.2**: 列出所有 DaemonSet Pod 及所在节点
- **命令**:
  ```bash
  kubectl get pods -n <namespace> -l <selector> \
    -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,READY:.status.containerStatuses[*].ready,NODE:.spec.nodeName,RESTARTS:.status.containerStatuses[*].restartCount
  ```
- **超时**: 10s
- **判断规则**:
  - 某些节点无对应 Pod → RC-001/003/004
  - Pod 状态 CrashLoopBackOff → RC-002/005/006/008
  - RESTARTS 频繁 → RC-002/005/006

**Step D1.3**: 检查缺失 Pod 的节点详情
- **命令**:
  ```bash
  # 获取缺失 Pod 的节点
  kubectl get nodes -o json | jq -r '.items[].metadata.name' | sort > /tmp/all_nodes.txt
  kubectl get pods -n <namespace> -l <selector> -o json | jq -r '.items[].spec.nodeName' | sort | uniq > /tmp/has_pod.txt
  MISSING=$(comm -23 /tmp/all_nodes.txt /tmp/has_pod.txt)
  for node in $MISSING; do
    echo "=== $node ==="
    kubectl get node $node -o jsonpath='{.spec.taints}' | jq -c .
    kubectl get node $node -o jsonpath='{.metadata.labels}' | jq -c .
  done
  ```
- **超时**: 15s
- **判断规则**:
  - 节点有 NoSchedule/NoExecute 污点 → RC-003/004（污点排斥）
  - 节点标签不匹配 DaemonSet nodeSelector → RC-001（节点选择器不匹配）
  - 节点 Ready 但无 Pod → 需进一步检查

**Step D1.4**: 检查 DaemonSet 的调度约束
- **命令**:
  ```bash
  kubectl get daemonset <name> -n <namespace> -o jsonpath='{.spec.template.spec}' | jq '{
    nodeSelector: .nodeSelector,
    affinity: .affinity,
    tolerations: .tolerations,
    hostNetwork: .hostNetwork,
    hostPID: .hostPID,
    hostIPC: .hostIPC
  }'
  ```
- **超时**: 10s
- **判断规则**:
  - nodeSelector 设置但节点无对应标签 → RC-001
  - tolerations 未包含节点污点 → RC-004
  - hostNetwork=true 但端口冲突 → RC-006

**Step D1.5**: 检查 Pod 事件和日志
- **命令**:
  ```bash
  # 获取一个异常 Pod 的事件
  ku
...(截断)

### Phase 2: 深度检查（只读，零风险）

**Step D2.1**: 检查节点资源是否足够调度 DaemonSet
- **命令**:
  ```bash
  kubectl describe node <missing-node> | grep -A 15 "Allocated resources"
  kubectl top node <missing-node> 2>/dev/null || echo "metrics-server unavailable"
  ```
- **超时**: 10s
- **判断规则**:
  - 节点资源已耗尽 → RC-005（资源不足）
  - DaemonSet 资源请求过高 → RC-005

**Step D2.2**: 检查污点与容忍度匹配
- **命令**:
  ```bash
  NODE_TAINTS=$(kubectl get node <node> -o json | jq -c '.spec.taints')
  DS_TOLERATIONS=$(kubectl get daemonset <name> -n <namespace> -o json | jq -c '.spec.template.spec.tolerations')
  echo "Node taints: $NODE_TAINTS"
  echo "DaemonSet tolerations: $DS_TOLERATIONS"
  ```
- **超时**: 10s
- **判断规则**:
  - 节点有污点但 DaemonSet 无对应 toleration → RC-004
  - 控制平面节点有 `node-role.kubernetes.io/control-plane:NoSchedule` → 需要添加 toleration

**Step D2.3**: 检查 hostPort/hostNetwork 冲突
- **命令**:
  ```bash
  # 获取 DaemonSet 的端口配置
  kubec

--- (内容截断，完整内容见源文件) ---

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[entities/kudig-metadata-index.md|README]]]] — FTA 故障树清单索引
- networking.md|ts-networking]] — 网络故障排查
- [[flannel-fta]] — Flannel 网络异常故障树分析
- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[kubernetes]] — Kubernetes (CNCF Graduated)
