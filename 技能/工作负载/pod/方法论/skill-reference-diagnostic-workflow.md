---
title: Diagnostic Workflow
description: '- 出现 `InvalidDiskCapacity` → 磁盘配置异常（RC-003 变种）'
summary: '- 出现 `InvalidDiskCapacity` → 磁盘配置异常（RC-003 变种）'
category: skills
tags:
- k8s
- troubleshooting
- skill
- apiserver
- kubelet
- containerd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Diagnostic Workflow 是什么
- 如何 Diagnostic Workflow
trigger_keywords:
- Diagnostic
- Workflow
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Diagnostic Workflow

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集节点状态信息，无需 SSH 登录节点。所有命令均为只读操作。
> **预计耗时**: 2-5 分钟

### Step D1.3: 检查节点事件

- **命令**:
  ```bash
  kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
    --sort-by=.lastTimestamp --no-headers | tail -30
  ```
- **超时**: 10s
- **预期输出模式**: 事件列表，关注 Warning 类型事件
- **判断规则**:
  - 出现 `NodeNotReady` 事件 → 确认 NotReady 时间点
  - 出现 `NodeHasDiskPressure` → 磁盘压力导致（RC-003）
  - 出现 `NodeHasMemoryPressure` → 内存压力导致（RC-004）
  - 出现 `NodeHasPIDPressure` → PID 压力导致（RC-005）
  - 出现 `NodeHasInsufficientMemory` → 内存不足（RC-004）
  - 出现 `InvalidDiskCapacity` → 磁盘配置异常（RC-003 变种）
  - 出现 `Rebooted` → 节点曾重启（关注 RC-009 内核/硬件问题）
  - 出现 `Starting` → [[kubelet|kubelet]] 刚重启过（RC-001 的恢复迹象）
  - 无近期事件 → 可能是网络分区，apiserver 未收到任何更新（RC-006）
- **版本差异**: 无

---

### Step D1.4: 检查节点 Taints

- **命令**:
  ```bash
  kubectl get node <node-name> -o jsonpath='{range .spec.taints[*]}{.key}={.value}:{.effect}{"\n"}{end}'
  ```
- **超时**: 5s
- **预期输出模式**: Taint 列表
- **判断规则**:
  - 存在 `node.kubernetes.io/not-ready:NoSchedule` → Kubernetes 自动添加的 taint，确认 NotReady 状态
  - 存在 `node.kubernetes.io/not-ready:NoExecute` → Pod 驱逐已触发
  - 存在 `node.kubernetes.io/unreachable:NoExecute` → 节点不可达
  - 存在 `node.kubernetes.io/unschedulable:NoSchedule` → 节点已被 cordon（RC-012）
  - 存在 `node.kubernetes.io/disk-pressure:NoSchedule` → DiskPressure（RC-003）
  - 存在 `node.kubernetes.io/memory-pressure:NoSchedule` → MemoryPressure（RC-004）
  - 存在 `node.kubernetes.io/pid-pressure:NoSchedule` → PIDPressure（RC-005）
- **版本差异**: 无

---

### Step D1.5: 检查节点 Lease 对象

- **命令**:
  ```bash
  kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
  ```
- **超时**: 5s
- **预期输出模式**: ISO 8601 时间戳
- **判断规则**:
  - renewTime 距当前时间 > 40s（默认 node-monitor-grace-period）→ kubelet 未能续租，可能 kubelet 停止（RC-001）或网络不通（RC-006）
  - renewTime 距当前时间 < 40s 但节点仍 NotReady → 可能是 kubelet 报告了不健康状态（检查 Conditions 详情）
- **版本差异**: 无

---

### Phase 2: 深度检查（只读，零风险，需 SSH）

> **目标**: SSH 登录问题节点，检查系统级组件状态。所有命令均为只读操作。
> **前提**: 需要对问题节点的 SSH 访问权限
> **预计耗时**: 5-10 分钟

### Step D2.1: 检查 kubelet 服务状态

- **命令**:
  ```bash
  ssh <node-ip> "systemctl status kubelet"
  ```
- **超时**: 10s
- **预期输出模式**: systemd unit 状态信息
- **判断规则**:
  - `Active: active (running)` → kubelet 进程在运行，问题可能在运行时层面或网络层面，继续 D2.2
  - `Active: inactive (dead)` → kubelet 未运行（RC-001），尝试查看 D2.2 中的日志了解停止原因
  - `Active: activating (auto-restart)` → kubelet 不断崩溃重启（RC-001），查看 D2.2 中的日志
  - `Active: failed` → kubelet 启动失败（RC-001），查看 D2.2 中的错误日志
  - `Loaded: not-found` → kubelet 服务未安装或 unit 文件丢失，极端情况
- **版本差异**: 无

---

### Step D2.2: 检查 kubelet 日志

- **命令**:
  ```bash
  ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager -n 200"
  ```
- **超时**: 15s
- **预期输出模式**: kubelet 日志条目
- **判断规则**:
  - 日志包含 `connection refused` 或 `dial tcp <apiserver-ip>:6443: connect: connection refused` → 网络不通或 apiserver 不可达（RC-006）
  - 日志包含 `x509: certificate has expired` 或 `certificate signed by unknown authority` → 证书问题（RC-007），关联 SKILL-SEC-001
  - 日志包含 `PLEG is not healthy` → PLEG 不健康（RC-008），继续 D2.6
  - 日志包含 `container runtime is not running` 或 `runtime connect using default endpoints` → 容器运行时问题（RC-002）
  - 日志包含 `failed to garbage collect` + 磁盘相关错误 → 磁盘空间不足（RC-003）
  - 日志包含 `OOM` 或 `oom_kill` → 内存压力（RC-004）
  - 日志包含 `too many open files` 或 `no space left on device` → 资源耗尽（RC-003 或 RC-005）
  - 日志包含 `node not found` → 节点对象可能被意外删除
  - 日志包含 `failed to renew lease` → Lease 续租失败，检查网络和 apiserver
  - 日志包含 `use of closed network connection` → 网络连接异常（RC-006）
- **版本差异**:
  - **[v1.28+]**: GracefulNodeShutdown 默认启用。如果日志中出现 `shutting down gracefully`，可能节点正在优雅关机，不一定是问题
  - **[v1.30+]**: swap 相关日志 `swap is enabled` 在启用 NodeSwap feature gate 时属于正常信息

---

### Step D2.3: 检查容器运行时（containerd）服务状态

- **命令**:
  ```bash
  ssh <node-ip> "systemctl status containerd"
  ```
- **超时**: 10s
- **预期输出模式**: systemd unit 状态信息
- **判断规则**:
  - `Active: active (runn

--- (内容截断，完整内容见源文件) ---

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 诊断工作流详解

### 标准诊断流程

```
1. 观察现象 (Observe)
   └─ kubectl get pods/events, 监控告警
2. 形成假设 (Hypothesize)
   └─ 基于 FTA 故障树分析可能原因
3. 验证假设 (Verify)
   └─ kubectl describe/logs, 节点检查
4. 确认根因 (Confirm)
   └─ 证据链完整，可复现
5. 执行修复 (Remediate)
   └─ 按风险等级执行修复命令
6. 验证恢复 (Validate)
   └─ 确认服务恢复正常
7. 复盘归档 (Document)
   └─ 记录案例，更新知识库
```

### 诊断工具矩阵

| 层次 | 工具 | 用途 |
|---|---|---|
| 集群 | kubectl, kubectx | 资源状态查看 |
| 节点 | ssh, journalctl | 节点级诊断 |
| 网络 | tcpdump, curl | 连通性测试 |
| 存储 | df, iostat | 存储诊断 |
| 日志 | kubectl logs, stern | 日志分析 |
| 监控 | Prometheus, Grafana | 指标分析 |

## 面试要点

1. **Q：诊断工作流的核心原则？**
   A：从现象到根因、分层排查、证据驱动、最小影响、可回滚。

2. **Q：如何加速诊断过程？**
   A：FTA 故障树、历史案例匹配、自动化工具、完善监控、经验积累。

3. **Q：诊断后的关键动作？**
   A：修复验证、复盘归档、更新 Runbook、预防措施、知识分享。

## Related

- [[技能/节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|SKILL]].md|skill-k8s-node-notready-SKILL]] — Skill
- [[实体/container-runtime.md|container-runtime]] — Container Runtime
- [[实体/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
