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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/diagnostic-overview/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/troubleshoot-pod-issues.md|Pod 故障排查]]

## 生产案例

### 案例 1: DaemonSet 更新卡住——新 Pod 无法调度

| 时间 | 事件 |
|------|------|
| 11:00 | 更新 DaemonSet 镜像，部分节点 Pod 未更新 |
| 11:05 | `kubectl rollout status ds/fluentd` 显示 "waiting for daemon set rollout" |
| 11:08 | 新 Pod 在部分节点 ImagePullBackOff |
| 11:10 | 私有镜像仓库凭据过期 |
| 11:15 | 🟡 更新 imagePullSecrets，DaemonSet 自动重试 |

**根因**: 镜像仓库 Secret 过期，新镜像无法拉取。

### 案例 2: 节点污点导致 DaemonSet Pod 未调度

**现象**: 新加入节点无 DaemonSet Pod，`kubectl get ds` DESIRED < 节点数。

**诊断**: 节点有 `node.kubernetes.io/unschedulable` 污点，DaemonSet 未配置对应 toleration

**修复**: 🟢 DaemonSet 添加 tolerations 容忍该污点

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 系统级 DaemonSet 全部异常 | 立即检查并重启 |
| P1 | 部分节点缺少 Pod | 检查污点和调度 |
| P2 | 更新策略优化 | 调整 updateStrategy |

## 面试要点

1. **Q: DaemonSet 与 Deployment 的调度区别？**
   A: DaemonSet 在每个节点(匹配 tolerations/nodeSelector)上运行一个 Pod，不使用默认调度器(1.12 前)，而是通过节点亲和性确保每节点一个。Deployment 由调度器决定 Pod 分布。

2. **Q: DaemonSet 的更新策略？**
   A: RollingUpdate(默认): 逐节点删除旧 Pod 创建新 Pod，maxUnavailable 控制并发；OnDelete: 手动删除 Pod 触发更新。生产推荐 RollingUpdate + maxUnavailable=10%。

3. **Q: 常见的 DaemonSet 有哪些？**
   A: 日志收集(fluentd/filebeat)、监控(node-exporter/datadog-agent)、网络插件(calico/flannel/terway)、存储插件(csi-node)、安全代理(falco)。

## Related

- [[实体/kudig-metadata-index.md|README]]]] — FTA 故障树清单索引
- networking.md|ts-networking]] — 网络故障排查
- [[flannel-fta]] — Flannel 网络异常故障树分析
- [[技能/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
