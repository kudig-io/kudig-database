---
title: 命令输出根因解析
description: '# 命令输出根因解析'
summary: '# 命令输出根因解析'
category: skills
tags:
- k8s
- troubleshooting
- structural
- command-output
- flannel
- helm
- daemonset
- rbac
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
- 命令输出根因解析 是什么
- 如何 命令输出根因解析
trigger_keywords:
- 命令输出根因解析
prerequisites:
- kubectl-basics
- helm-basics
---



# 命令输出根因解析

### 00 Command Output Root Cause Parser

#### 附录：命令输出 → 根因 快速索引表

| 症状关键词 | 命令 | 指示器 | 初步诊断 |
|-----------|------|--------|---------|
| `OOMKilled` | `kubectl describe pod` | `Reason: OOMKilled` | 容器内存超限 |
| `CrashLoopBackOff` | `kubectl describe pod` | `Reason: CrashLoopBackOff` | 应用启动失败 |
| `ImagePullBackOff` | `kubectl describe pod` | `Reason: ImagePullBackOff` | 镜像拉取失败 |
| `Evicted` | `kubectl get events` | `Evicted` | Pod 被驱逐 |
| `Terminating` | `kubectl get pod` | `status: Terminating` | Pod 删除卡住 |
| `Pending` | `kubectl get pod` | `status: Pending` | 调度失败 |
| `Unschedulable` | `kubectl describe pod` | `Reason: Unschedulable` | 调度失败（资源不足） |
| `Connection refused` | `kubectl logs` | `connect: connection refused` | 后端服务不可达 |
| `no such host` | `kubectl logs` | `no such host` | DNS 解析失败 |
| `x509` | `kubectl logs` | `x509: certificate` | 证书验证失败 |
| `panic` | `kubectl logs` | `panic:` | Go 应用崩溃 |
| `OutOfMemoryError` | `kubectl logs` | `OutOfMemoryError` | JVM 内存溢出 |
| `NotReady` | `kubectl describe node` | `Ready: False` | 节点不可用 |
| `MemoryPressure` | `kubectl describe node` | `MemoryPressure: True` | 节点内存压力大 |
| `<none>` (endpoints) | `kubectl get endpoints` | `ENDPOINTS <none>` | [[Service|Service]] 无后端 |
| `metrics not available` | `kubectl top node` | `metrics not available yet` | metrics-server 异常 |
| `not authorized` | `kubectl exec` | `not authorized` | RBAC 权限不足 |
| `unable to upgrade` | `kubectl exec` | `unable to upgrade connection` | API Server 与 Pod 隧道中断 |

---

---

### 01 Kubectl Watch Output Parser

#### 1.1 事件类型对照表

| 事件类型 | 显示格式 | 含义 | Agent 判断 |
|---------|---------|------|-----------|
| `ADDED` | `NAME ... AGE` 第一次出现 | 资源被创建 | 正常（新建资源） |
| `MODIFIED` | `NAME ... AGE` 行变化 | 资源状态/配置更新 | 需判断是否异常变化 |
| `DELETED` | 行消失 | 资源被删除 | 需判断是否异常删除 |
| `ERROR` | `<error>` | 与 API Server 通信问题 | 异常，需立即排查 |

#### 1.2 kubectl get --watch vs --watch-only

```bash
# 完整输出（含当前状态）
kubectl get pods --watch

# 仅新事件（从当前时刻开始）
kubectl get pods --watch-only
```

---

#### 2.1 Pod 重启（CrashLoopBackOff）watch 表现

```yaml
output_pattern:
  - id: "watch-pod-001"
    command: "kubectl get pods -n <namespace> --watch"
    raw_output: |
      NAME    READY   STATUS    AGE
      api     1/1     Running   5m
      api     1/1     Running   5m, 0/1   Running   0s    # ← MODIFIED（restart count 变化）
      api     0/1     Running   5m, 1/1   Running   0s    # ← restart 后 READY 恢复
      api     1/1     Running   5m                                # ← 稳定
    diagnosis: "Pod 经历了 1 次重启但自行恢复，restart count 从 0→1，重启后 READY 回到 1/1"
    severity: P1
    possible_causes:
      - cause: "应用临时性崩溃（如 OOM Kill 后自行恢复）"
        indicators: ["0/1" 出现后快速恢复]
        next_step: "kubectl logs --previous <pod> 查看崩溃时的日志"
    expected_output: "READ 为 1/1 且不再出现 0/1"  # 正常
```

## 相关链接

- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断引擎]]

## Related

- [[flannel-fta]] — Flannel 网络异常故障树分析
- [[skills/skill-22-daemonset-failure.md|skill-22-daemonset-failure]] — DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation
- [[csi-fta]] — CSI 存储异常故障树分析
- [[helm-fta]] — Helm 发布异常故障树分析
- [[skills/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
