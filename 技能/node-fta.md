---
title: Node 异常故障树分析 (skills)
description: '| | | `ssh ${NODE_NAME} ''journalctl -u kubelet --since "10 min ago"
  | grep -E "error|fatal|exit"''` | 包含 kubelet 崩溃日志 | **确认根因** |'
summary: '| | | `ssh ${NODE_NAME} ''journalctl -u kubelet --since "10 min ago" |
  grep -E "error|fatal|exit"''` | 包含 kubelet 崩溃日志 | **确认根因** |'
category: general
tags:
- k8s
- kubelet
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
- Node 异常故障树分析 是什么
- 如何 Node 异常故障树分析
trigger_keywords:
- Node
- 异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-NODE-001
component: Node
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "Node 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get nodes -o jsonpath='{range .items[?(@.status.conditions[?(@.type==\'Ready\' && @.status!=\'True\')].nodeName]' 显示有 NotReady 节点 --> - **目标**：覆盖节点不可用/不稳..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/topic-fta/list/node-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Node 异常故障树分析

### 诊断命令速查表

> 本表列出 FTA 树各节点的实际诊断命令，供 SRE 手工执行或 AI Agent 自动化调用。
> 变量说明: `${NODE_NAME}` - 节点名称 | `${NAMESPACE}` - 命名空间（少数命令使用）
> 注：SSH 命令需节点可达；K8s 1.23+ 可用 `kubectl debug node/${NODE_NAME}` 替代部分 SSH 操作

### 1. 节点状态异常

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|---------|------------|------|
| `cat_nstat` | 节点状态分类 | `kubectl get node ${NODE_NAME} -o json | jq '.status.conditions[] | select(.type=="Ready") | .status'` | `False` / `Unknown` | → 进入状态子树 |
| `evt_notready` | NotReady/Unknown | `kubectl get node ${NODE_NAME} -o json | jq '.status.conditions[] | select(.type=="Ready")'` | `status: False/Unknown` | **确认根因** |
| | | `kubectl describe node ${NODE_NAME} | grep -A 5 'Conditions:'` | 包含 `False` 或 `Unknown` | **确认根因** |
| `evt_reboot` | 节点频繁重启 | `ssh ${NODE_NAME} 'last reboot | head -5'` | 近期有多次 reboot 记录 | **确认根因** |
| | | `ssh ${NODE_NAME} 'dmesg | grep -iE "reboot|panic|kernel BUG"'` | 包含异常重启信息 | **确认根因** |
| `evt_cordon` | 节点被 cordon | `kubectl get node ${NODE_NAME} -o jsonpath='{.spec.unschedulable}'` | `true` | **确认根因** |
| | | `kubectl describe node ${NODE_NAME} | grep Taints` | 包含 `node.kubernetes.io/unschedulable` | **确认根因** |

### 2. kubelet 异常

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|---------|------------|------|
| `cat_kubelet` | kubelet 异常分类 | `ssh ${NODE_NAME} 'systemctl is-active kubelet'` | `inactive` / `failed` | → 进入 kubelet 子树 |
| `evt_kubelet_down` | kubelet 服务异常 | `ssh ${NODE_NAME} 'systemctl status kubelet --no-pager -l | tail -20'` | `failed` / `inactive` | **确认根因** |
| | | `ssh ${NODE_NAME} 'journalctl -u kubelet --since "10 min ago" | grep -E "error|fatal|exit"'` | 包含 kubelet 崩溃日志 | **确认根因** |
| `evt_heartbeat_fail` | 心跳上报失败 | `kubectl get lease -n kube-node-lease ${NODE_NAME} -o json | jq '.spec.renewTime'` | renewTime 过期 | 进一步检查 |
| | | `ssh ${NODE_NAME} 'journalctl -u kubelet | grep -E "failed to update lease|unable to update node status"'` | 包含 Lease 更新失败 | **确认根因** |
| `evt_kubelet_cert` | 证书/鉴权失败 | `ssh 
...(截断)

## 生产案例

### 案例 1: kubelet 证书过期导致节点 NotReady

| 时间 | 事件 |
|------|------|
| 03:00 | 监控告警: 3 个节点 NotReady |
| 03:05 | `kubectl describe node` 显示 "KubeletNotReady" condition |
| 03:10 | SSH 登录节点，`journalctl -u kubelet` 显示 x509 certificate has expired |
| 03:15 | 🔴 `kubeadm certs renew all` + `systemctl restart kubelet` |
| 03:20 | 节点恢复 Ready，Pod 重新调度 |

**根因**: 集群初始化时未配置 kubelet 证书自动轮换 (`--rotate-certificates=true`)，1 年后证书过期。

### 案例 2: 节点磁盘压力导致批量 Pod 驱逐

**现象**: 节点上 20+ Pod 被 Evicted，`kubectl describe node` 显示 DiskPressure=True。

**诊断**: `df -h` → /var/lib/docker 95% → 容器日志未轮转占满磁盘

**修复**: 🟡 配置 containerLogMaxSize=100Mi + containerLogMaxFiles=5，清理旧日志

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 多节点同时 NotReady，业务受损 | 立即检查控制平面 + 节点 kubelet |
| P1 | 单节点 NotReady，Pod 已重调度 | 30min 内排查节点问题 |
| P2 | 节点资源压力告警 | 规划扩容或清理 |

## 面试要点

1. **Q: 节点 NotReady 的常见原因有哪些？**
   A: ① kubelet 进程崩溃/挂起 ② 证书过期 ③ 网络分区(apiserver 不可达) ④ 磁盘/内存压力 ⑤ CNI 插件异常 ⑥ 节点时间偏移超过证书容忍。

2. **Q: kubelet 的节点租约(Lease)机制是如何工作的？**
   A: kubelet 每 10s 更新 Lease 对象(node-lease-namespace)，controller-manager 检查 lease 超时(node-monitor-grace-period=40s)，超时后标记 NotReady 并触发 Pod 驱逐(pod-eviction-timeout=5min)。

3. **Q: 如何安全地维护一个节点？**
   A: `kubectl cordon node` (禁止新调度) → `kubectl drain node --ignore-daemonsets --delete-emptydir-data` (驱逐 Pod) → 维护 → `kubectl uncordon node` (恢复调度)。

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[troubleshoot-node-issues|节点故障排查]]

## Related

- [[kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/node-index.md|Node 知识图谱索引]]


<!-- risk-assessed -->
