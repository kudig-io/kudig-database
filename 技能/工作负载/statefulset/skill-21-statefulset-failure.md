---
title: StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation (skills)
description: '| S4 | Headless Service 无 Endpoints | `kubectl get endpoints <svc>`
  | 0.85 | Service 配置错误 |'
summary: '| S4 | Headless Service 无 Endpoints | `kubectl get endpoints <svc>` | 0.85
  | Service 配置错误 |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- statefulset
- sts
- pvc pending
- pod not starting
- coredns
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation 是什么
- 如何 StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
trigger_keywords:
- StatefulSet
- 故障诊断与修复
- StatefulSet
- Failure
- Diagnosis
- Remediation
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[StatefulSet|StatefulSet]] 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation

### 症状识别



### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | StatefulSet Pod 未按序号启动（如 -1 存在但 -0 不存在） | `kubectl get [[Pods|pods]] -l app=<name>` | 0.95 | 手动删除 → 检查操作记录 |
| S2 | Pod 序号中断（如 -0, -2 存在但 -1 不存在） | `kubectl get pods -l app=<name>` | 0.95 | 无 |
| S3 | PVC 一直 Pending | `kubectl get pvc -n <ns>` | 0.95 | 通用 PVC 问题 → SKILL-STORE-001 |
| S4 | Headless [[Service|Service]] 无 Endpoints | `kubectl get endpoints <svc>` | 0.85 | Service 配置错误 |
| S5 | 滚动更新卡在特定序号 | `kubectl rollout status sts/<name>` | 0.90 | 应用启动慢 → SKILL-POD-001 |
| S6 | Pod 删除后新 Pod 无法创建 | `kubectl get events` | 0.85 | 节点资源不足 → SKILL-POD-002 |
| S7 | DNS 解析 `<pod>.<svc>` 失败 | `nslookup` from test Pod | 0.85 | CoreDNS 问题 → SKILL-NET-001 |
| S8 | 有状态集群应用报告节点不一致 | 应用日志/状态检查 | 0.80 | 应用自身 bug |

### 诊断工作流



### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集 StatefulSet、Pod、PVC 和 Service 状态。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取 StatefulSet 概览
- **命令**:
  ```bash
  kubectl get statefulset <name> -n <namespace> -o wide
  kubectl describe statefulset <name> -n <namespace> | head -50
  ```
- **超时**: 10s
- **预期输出模式**: DESIRED/CURRENT/READY 列和 Events
- **判断规则**:
  - READY < DESIRED → 有 Pod 未就绪
  - CURRENT < DESIRED → 有 Pod 未创建（可能是顺序启动卡住）
  - Events 包含 `FailedCreate` → RC-001/002/003
  - Events 包含 `FailedDelete` → 删除/重建问题
- **版本差异**: 无

**Step D1.2**: 检查 Pod 状态和序号
- **命令**:
  ```bash
  kubectl get pods -n <namespace> -l <statefulset-label-selector> \
    -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,READY:.status.containerStatuses[*].ready,RESTARTS:.status.containerStatuses[*].restartCount,NODE:.spec.nodeName
  ```
- **超时**: 10s
- **预期输出模式**: Pod 列表及状态
- **判断规则**:
  - Pod 序号不连续（如 -0, -2 但无 -1）→ RC-001（启动顺序异常）
  - 高序号 Pod 存在但低序号不存在 → 异常情况，可能手动操作导致
  - Pod 状态 Pending → RC-002/003（PVC/调度问题）
  - Pod 状态 CrashLoopBackOff → 应用启动失败（RC-010 或 SKILL-POD-001）
- **版本差异**: 无

**Step D1.3**: 检查 PVC 绑定状态
- **命令**:
  ```bash
  kubectl get pvc -n <namespace> -l <statefulset-label-selector>
  kubectl describe pvc <pvc-name> -n <namespace> | grep -A 5 "Events:"
  ```
- **超时**: 10s
- **预期输出模式**: PVC 列表和状态
- **判断规则**:
  - PVC Pending → RC-002（PVC 绑定失败）
  - PVC Bound 但 Pod 未挂载 → RC-003（挂载问题）
  - 无 PVC（StatefulSet 未定义 volumeClaimTemplate）→ 正常（无状态部分）
- **版本差异**: 无

**Step D1.4**: 检查 Headless Service
- **命令**:
  ```bash
  kubectl get service <service-name> -n <namespace>
  kubectl get endpoints <service-name> -n <namespace>
  kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.clusterIP}'
  ```
- **超时**: 10s
- **预期输出模式**: Service 类型和 Endpoints
- **判断规则**:
  - `clusterIP` 不为 `None` → RC-004（Service 不是 Headless）
  - Endpoints 为空 → RC-004（Service selector 不匹配 Pod 标签）
  - Service 不存在 → RC-004（Service 未创建）
- **版本差异**: 无

**Step D1.5**: 检查 StatefulSet 更新策略
- **命令**:
  ```bash
  kubectl get statefulset
...(截断)

### Phase 2: 深度检查（只读，零风险）

> **目标**: 深入分析 Pod 事件、存储状态和网络标识。
> **预计耗时**: 5-15 分钟

**Step D2.1**: 分析 Pod 创建事件
- **命令**:
  ```bash
  kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name> \
    --sort-by=.lastTimestamp | tail -20
  kubectl describe pod <pod-name> -n <namespace> | grep -A 30 "Events:"
  ```
- **超时**: 15s
- **预期输出模式**: Pod 事件列表
- **判断规则**:
  - `FailedScheduling` → RC-002（调度失败，可能是资源不足）
  - `FailedMount` → RC-003（存储挂载失败）
  - `FailedCreatePodSandBox` → CNI/网络问题
  - `BackOff` + 容器启动失败 → 应用问题（RC-010）
  - `RecreatingFailedPod` → 前一 Pod 失败，StatefulSet 尝试重建
- **版本差异**: 无

**Step D2.2**: 检查 PVC 和 PV 详情
- **命令**:
  ```bash
  kubectl describe pvc <pvc-name> -n <namespace>
  kubectl get pv <pv-name> -o yaml
  kubectl get storageclass <sc-name> -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: PVC/PV/StorageClass 详情
- **判断规则**:
  - PVC 等待 PV 绑定且 StorageClass 不存在 → RC-002（StorageClass 缺失）
  - PV 容量不足 → RC-007（存储

--- (内容截断，完整内容见源文件) ---

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 生产案例

### 案例 1: StatefulSet Pod 卡在 Terminating——PVC 卸载失败

| 时间 | 事件 |
|------|------|
| 10:00 | 删除 StatefulSet Pod 后卡在 Terminating 10min |
| 10:05 | `kubectl describe pod` 显示 "FailedKillPod: volume detach failed" |
| 10:08 | CSI driver 无法卸载卷，存储后端连接超时 |
| 10:12 | 🔴 强制删除: `kubectl delete pod --force --grace-period=0` |
| 10:15 | 检查存储后端健康状态 |

**根因**: 存储后端(NAS/云盘)临时不可用，CSI 无法完成卸载。

### 案例 2: StatefulSet 扩容后新 Pod 无法绑定 PVC

**现象**: StatefulSet 从 3 扩容到 5，Pod-3/Pod-4 Pending。

**诊断**: StorageClass 配额耗尽，无法创建新 PV

**修复**: 🟡 提升存储配额或清理无用 PVC

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 数据库主节点不可用 | 检查 PVC + 强制恢复 |
| P1 | 扩容失败 | 检查存储配额 |
| P2 | 更新策略优化 | 调整 partition |

## 面试要点

1. **Q: StatefulSet Pod 卡在 Terminating 的处理方法？**
   A: ① 检查 PVC/PV 状态 ② 检查 CSI driver 日志 ③ 确认存储后端健康 ④ 必要时 `--force --grace-period=0` 强制删除 ⑤ 检查 finalizers 是否阻塞。

2. **Q: StatefulSet 的 PVC 生命周期管理？**
   A: StatefulSet 为每个 Pod 创建独立 PVC，Pod 删除/缩容时 PVC 保留(数据不丢失)。K8s 1.27+ 支持 persistentVolumeClaimRetentionPolicy 自动清理。

3. **Q: StatefulSet 故障恢复的最佳实践？**
   A: ① 配置 PDB 保护最小可用数 ② 使用 volumeClaimTemplates 确保存储持久化 ③ 定期备份 PVC 数据 ④ 设置 podManagementPolicy=Parallel 加速恢复。

## Related

- [[技能/节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[实体/statefulset.md|statefulset]] — StatefulSet
- [[coredns]] — CoreDNS
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
