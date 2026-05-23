---
title: "Deployment Rollout Remediation Playbook"
category: remediation
skill_set: "k8s-deployment-rollout"
created: "2026-05-22"
updated: "2026-05-22"
---

# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-DEPLOY-001 v1.0 — Deployment Rollout Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险 — Agent 可建议自动执行](#-低风险)
    - [REM-001 扩容资源或调整副本数](#rem-001)
    - [REM-004 调整滚动更新策略](#rem-004)
    - [REM-005 恢复 paused Deployment](#rem-005)
  - [🟡 中风险 — Agent 建议，人工审批后执行](#-中风险)
    - [REM-002 修复镜像问题](#rem-002)
    - [REM-003 调整健康检查探针](#rem-003)
    - [REM-006 调整调度约束](#rem-006)
    - [REM-007 修复 Init Container](#rem-007)
  - [🔴 高风险 — Agent 仅提供指导，人工执行](#-高风险)
    - [REM-008 强制回滚 Deployment](#rem-008)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 只读或影响极小的修改 | 可建议自动执行 |
| 中风险 | 🟡 | 可能导致短暂服务中断 | 建议操作并等待人工审批 |
| 高风险 | 🔴 | 将导致工作负载中断 | 仅提供操作指导，由人工执行 |

## 修复操作

### 🟢 低风险

#### REM-001: 扩容资源或调整副本数

- **适用根因**: RC-001（资源不足）
- **前置检查**:
  ```bash
  kubectl top nodes
  kubectl describe pod <pending-pod> -n <namespace> | grep -A 5 "Events"
  # 查看 FailedScheduling 原因
  ```
- **执行命令**:
  ```bash
  # 方案 A: 增加副本数（如果当前为 1）
  kubectl scale deployment/<name> --replicas=2 -n <namespace>

  # 方案 B: 降低资源请求
  kubectl patch deployment/<name> -n <namespace> -p '
  {"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"requests":{"cpu":"100m","memory":"128Mi"}}}]}}}}'

  # 方案 C: 扩容节点（如果集群资源不足）
  # 通过 Cluster Autoscaler 或手动添加节点
  ```
- **后置验证**:
  ```bash
  kubectl get deployment <name> -n <namespace>
  # 预期: Ready == Desired
  ```
- **回滚命令**:
  ```bash
  kubectl scale deployment/<name> --replicas=<original> -n <namespace>
  ```

#### REM-004: 调整滚动更新策略

- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.strategy}'
  ```
- **执行命令**:
  ```bash
  # 放宽 maxUnavailable 以允许更新进行
  kubectl patch deployment/<name> -n <namespace> -p '
  {"spec":{"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":1,"maxSurge":1}}}}'
  ```
- **后置验证**:
  ```bash
  kubectl rollout status deployment/<name> -n <namespace>
  ```
- **回滚命令**:
  ```bash
  kubectl patch deployment/<name> -n <namespace> -p '
  {"spec":{"strategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":"25%","maxSurge":"25%"}}}}'
  ```

#### REM-005: 恢复 paused Deployment

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.paused}'
  # 预期: true
  ```
- **执行命令**:
  ```bash
  kubectl rollout resume deployment/<name> -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.paused}'
  # 预期: 无输出或 <no value>
  kubectl rollout status deployment/<name> -n <namespace>
  ```
- **回滚命令**:
  ```bash
  kubectl rollout pause deployment/<name> -n <namespace>
  ```

### 🟡 中风险

#### REM-002: 修复镜像问题

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  kubectl describe pod <pod> -n <namespace> | grep -A 10 "Events"
  # 确认是镜像不存在、标签错误还是仓库认证问题
  ```
- **执行命令**:
  ```bash
  # 方案 A: 修正镜像标签
  kubectl set image deployment/<name> app=<correct-image>:<tag> -n <namespace>

  # 方案 B: 更新 imagePullSecrets
  kubectl patch serviceaccount default -n <namespace> -p '
  {"imagePullSecrets":[{"name":"registry-secret"}]}'
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l app=<label>
  # 预期: 无 ImagePullBackOff
  ```

#### REM-003: 调整健康检查探针

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl describe pod <pod> -n <namespace> | grep -A 20 "Liveness\|Readiness"
  kubectl logs <pod> -n <namespace> --previous 2>/dev/null | tail -50
  ```
- **执行命令**:
  ```bash
  # 放宽初始延迟和超时时间
  kubectl patch deployment/<name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/template/spec/containers/0/livenessProbe/initialDelaySeconds", "value": 60},
   {"op": "replace", "path": "/spec/template/spec/containers/0/readinessProbe/initialDelaySeconds", "value": 30}]'
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l app=<label>
  # 预期: Pod 进入 Running 且 restart 停止增长
  ```

#### REM-006: 调整调度约束

- **适用根因**: RC-006
- **前置检查**:
  ```bash
  kubectl describe pod <pending-pod> -n <namespace> | grep -A 10 "Events"
  # 查看 FailedScheduling 原因（node affinity, taints, resources）
  ```
- **执行命令**:
  ```bash
  # 方案 A: 添加 tolerations
  kubectl patch deployment/<name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/spec/template/spec/tolerations", "value":
    [{"key":"dedicated","operator":"Equal","value":"web","effect":"NoSchedule"}]}]'

  # 方案 B: 移除节点亲和性限制
  kubectl patch deployment/<name> -n <namespace> --type='json' -p='
  [{"op": "remove", "path": "/spec/template/spec/affinity/nodeAffinity"}]'
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l app=<label>
  # 预期: Pending Pod 开始调度
  ```

#### REM-007: 修复 Init Container

- **适用根因**: RC-007
- **前置检查**:
  ```bash
  kubectl describe pod <pod> -n <namespace> | grep -A 20 "Init Containers"
  kubectl logs <pod> -n <namespace> -c <init-container-name>
  ```
- **执行命令**:
  ```bash
  # 根据 init container 失败原因修复
  # 常见原因: 依赖服务未就绪、配置错误、权限不足
  kubectl patch deployment/<name> -n <namespace> -p '<init-container-fix-json>'
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l app=<label>
  # 预期: Init 完成，Pod 进入 Running
  ```

### 🔴 高风险

#### REM-008: 强制回滚 Deployment

- **适用根因**: 所有无法快速修复的 rollout 失败
- **影响说明**: 回滚到上一个版本，会中断当前发布流程。新版本 Pod 被终止，旧版本 Pod 被重新创建。
- **操作步骤**:
  1. **查看历史版本**:
     ```bash
     kubectl rollout history deployment/<name> -n <namespace>
     ```
  2. **执行回滚**:
     ```bash
     kubectl rollout undo deployment/<name> -n <namespace>
     # 或回滚到指定版本
     kubectl rollout undo deployment/<name> -n <namespace> --to-revision=<revision>
     ```
  3. **等待回滚完成**:
     ```bash
     kubectl rollout status deployment/<name> -n <namespace>
     ```
  4. **暂停发布（如需调查）**:
     ```bash
     kubectl rollout pause deployment/<name> -n <namespace>
     ```
- **安全检查**:
  - 确认回滚版本的业务兼容性
  - 通知相关团队发布已回滚
  - 检查是否有数据库 schema 变更需要同步回滚
- **回滚方案**:
  ```bash
  # 如果回滚后问题依旧，再次 undo 可回到新版本
  kubectl rollout undo deployment/<name> -n <namespace>
  ```

## 验证确认

### 即时验证（修复后 1-2 分钟内）

```bash
# V1: 副本数正常
kubectl get deployment <name> -n <namespace>
# 预期: READY == DESIRED, UP-TO-DATE == DESIRED

# V2: 无旧 ReplicaSet 残留
kubectl get rs -n <namespace> -l app=<label>
# 预期: 仅新 ReplicaSet 有 pod

# V3: Pod 全部 Running
kubectl get pods -n <namespace> -l app=<label>
# 预期: 全部 Running，无 restarts

# V4: 无 failure events
kubectl get events -n <namespace> --field-selector involvedObject.name=<name>
# 预期: 无 ProgressDeadlineExceeded、FailedCreate

# V5: rollout status 成功
kubectl rollout status deployment/<name> -n <namespace>
# 预期: deployment "<name>" successfully rolled out
```

### 解决确认标准

- [ ] Ready replicas == Desired replicas
- [ ] Updated replicas == Desired replicas
- [ ] Unavailable replicas == 0
- [ ] Observed generation == Spec generation
- [ ] 所有 Pod 处于 Running 状态
- [ ] 无 rollout failure events
- [ ] 新版本业务功能验证通过（如有测试用例）

### 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| Pod 重启次数 | `kubectl get pods` RESTARTS 列 | 每小时 | 持续增加 → 检查探针或应用稳定性 |
| 资源使用率 | `kubectl top pods` | 每小时 | 持续高负载 → 考虑扩容 |
| 发布成功率 | Deployment events | 每次发布 | 再次失败 → 深入排查应用代码 |

## 升级协议

### 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| 回滚失败 | rollout undo 后仍然失败 | 回滚执行后 5 分钟 |
| 级联问题 | 发布导致依赖服务异常 | 发现关联服务告警 |
| 数据风险 | 涉及数据库 schema 变更 | 任何回滚操作前 |
| 多次失败 | 同一 Deployment 24h 内失败 3 次以上 | 第三次失败时 |

### 升级消息模板

```
【{severity}】Deployment Rollout Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: Deployment {namespace}/{deployment} rollout 失败
- 影响范围: 
  - 服务可用性: {available}/{desired} replicas
  - 受影响功能: {affected_features}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- Skill 版本: SKILL-DEPLOY-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
