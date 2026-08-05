---
title: K8s Deployment Rollout Failure 诊断与修复
description: Kubernetes Deployment 发布失败、滚动更新卡住的完整诊断-修复-验证 Skill
summary: Kubernetes Deployment 发布失败、滚动更新卡住的完整诊断-修复-验证 Skill
category: Kubernetes-Incident-Response
tags:
- k8s
- skills
- sop
- runbook
- deployment
- rollout
- replicaset
- strategy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 开发工程师
estimated_read_time: 5min
intent_queries:
- K8s Deployment Rollout Failure 诊断与修复 是什么
- 如何修复 Deployment 滚动更新卡住
trigger_keywords:
- Deployment rollout stuck
- 滚动更新卡住
- ReplicaSet 未更新
- ProgressDeadlineExceeded
- deployment paused
- rollout failed
- max unavailable
- max surge
prerequisites:
- kubectl-basics
- deployment-concepts
skill_id: SKILL-DEPLOY-001
skill_name: K8s Deployment Rollout Failure 诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s Deployment Rollout Failure 诊断与修复

Deployment Rollout Failure 是 [[Kubernetes|Kubernetes]] 中最常见的发布问题类型。滚动更新可能因为资源不足、镜像拉取失败、健康检查失败、调度限制等原因卡住，导致新版本无法上线或新旧版本同时运行。

本 [[SKILL|Skill]] 覆盖 Deployment 发布失败、滚动更新卡住、[[ReplicaSet|ReplicaSet]] 残留等全部常见根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| `kubectl rollout status` 长时间 pending | `kubectl rollout status deployment/<name>` | 0.95 |
| Deployment 的 Available Replicas < Desired | `kubectl get deployment` | 0.90 |
| Events 中出现 ProgressDeadlineExceeded | `kubectl get events --field-selector reason=ProgressDeadlineExceeded` | 0.95 |
| 新旧 ReplicaSet 同时存在 | `kubectl get rs -l app=<label>` | 0.85 |
| Pod 持续处于 Pending/ContainerCreating | `kubectl get pods -l app=<label>` | 0.80 |
| Deployment 被 paused | `kubectl get deployment <name> -o jsonpath='{.spec.paused}'` | 0.90 |

**排除条件**: 节点 NotReady → SKILL-NODE-001; 镜像拉取失败 → SKILL-IMG-001; 权限问题 → SKILL-RBAC-001

## 快速分级（2 分钟内完成）

```
影响范围
├── 核心服务（支付/订单/认证）──→ P0（15min 内修复）
├── 生产环境非核心服务 ──────────→ P1（30min 内修复）
├── 预发布/灰度环境 ─────────────→ P2（2h 内修复）
└── 开发/测试环境 ───────────────→ P3（4h 内处理）
```

**立即升级条件**:
- 核心服务超过 50% 副本不可用
- Deployment 回滚后仍然失败
- 多个 Deployment 同时出现 rollout 失败
- 发布导致级联问题（依赖服务大量报错）

## 执行流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
工单/告警触发
    │
    ▼
┌──────────────┐    脚本: scripts/diagnose-quick.sh
│ Phase 1      │    内容: kubectl 快速检查（只读，零风险）
│ 快速检查      │    Step: D1.1-D1.6
└──────┬───────┘
       │ 无法确认根因
       ▼
┌──────────────┐    参考: reference/diagnostic-workflow.md
│ Phase 2      │    内容: 深度诊断（只读）
│ 深度检查      │    Step: D2.1-D2.5
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    参考: reference/root-cause-catalog.md
│ 根因匹配      │
│ RC-001~007   │
└──────┬───────┘
       │
       ▼
┌──────────────┐    参考: reference/remediation-playbook.md
│ 修复操作      │    风险: LOW → MEDIUM → HIGH
│ REM-001~006  │
└──────┬───────┘
       │
       ▼
┌──────────────┐    脚本: scripts/verify-deployment.sh
│ 验证确认      │    检查: Rollout 状态/Pod 状态/RS 清理
└──────────────┘
```
## 可用脚本

| 脚本 | 用途 | 参数 | 风险 |
|------|------|------|------|
| `scripts/diagnose-quick.sh` | kubectl 快速诊断 | `NAMESPACE` `DEPLOYMENT_NAME` | 只读 |
| `scripts/verify-deployment.sh` | 修复后验证 | `NAMESPACE` `DEPLOYMENT_NAME` | 只读 |

**使用方式**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Phase 1: kubectl 快速诊断
bash scripts/diagnose-quick.sh <namespace> <deployment-name>

# 修复后验证
bash scripts/verify-deployment.sh <namespace> <deployment-name>
```
## 根因概览 (7 种)

| RC ID | 根因 | 概率 | 首选修复 | 风险 |
|-------|------|------|---------|------|
| RC-001 | 资源不足（CPU/内存/节点） | 高 | REM-001 扩容/缩容 | LOW |
| RC-002 | 镜像拉取失败 | 高 | REM-002 修复镜像 | MEDIUM |
| RC-003 | 健康检查（Liveness/Readiness）失败 | 高 | REM-003 调整探针 | MEDIUM |
| RC-004 | 滚动更新策略配置不当 | 中 | REM-004 调整 strategy | LOW |
| RC-005 | Deployment 被 paused | 低 | REM-005 resume | LOW |
| RC-006 | 调度约束无法满足（亲和性/tolerations） | 中 | REM-006 调整调度 | MEDIUM |
| RC-007 | 初始化容器（Init Container）失败 | 中 | REM-007 修复 init | MEDIUM |

> 完整根因详情见 [reference/root-cause-catalog.md](./reference/root-cause-catalog.md)
> 完整修复步骤见 [reference/remediation-playbook.md](./reference/remediation-playbook.md)

## 关联资源

| 资源 | 路径 |
|------|------|
| 修复操作手册 | [reference/remediation-playbook.md](./reference/remediation-playbook.md) |
| 根因目录 | [reference/root-cause-catalog.md](./reference/root-cause-catalog.md) |
| 单文件完整版 | [../08-deployment-rollout-failure.md](09-deployment-rollout-failure.md) |

## Related

- Workloads 知识图谱索引


## 远程顾问信息收集

> 作为远程顾问，我**无法直接连接你的集群**。请帮我收集以下信息，我会根据你提供的内容给出准确的诊断建议。

### 第一步：快速确认（30 秒内回答）

1. **影响范围**：这个问题影响多少个节点 / Pod / 命名空间？
2. **紧急程度**：业务是否已中断？是否有用户投诉？
3. **发生时间**：问题是突然发生还是逐渐恶化？最近是否有变更？

### 第二步：关键信息（请提供你能获取的）

4. **kubectl 版本**：`kubectl version --short` 的输出
5. **K8s 集群版本**：`kubectl get nodes -o wide` 中的 VERSION 列
6. **节点状态**：控制平面节点是否正常？工作节点是否正常？

### 第三步：诊断信息（按需补充）

> 如果以下命令你无法执行，请直接告诉我「无法执行」，我会提供替代方案。

7. **相关组件日志**：`kubectl logs -n <namespace> <pod>` 的最后 30 行
8. **节点资源**：`kubectl top nodes` 或 `kubectl describe node <node>` 的 Capacity/Allocated resources
9. **近期变更**：最近 24 小时是否有部署、扩缩容、配置变更？

### 如果信息不足

如果你目前只能提供部分信息，**请从第一步开始**。我会根据已有信息先给出初步判断，并告诉你还需要收集什么。

> **替代沟通方式**：如果你不方便执行命令，也可以直接描述你看到的页面/告警内容，我会帮你解读。


## 命令替代方案

> 如果你无法执行以下命令，请参考对应的替代方案。

### 通用替代方案

| 原命令 | 无法执行的原因 | 替代方案 A | 替代方案 B |
|:---|:---|:---|:---|
| `kubectl get pods` | 无 kubectl 权限 | 通过集群管理控制台查看 Pod 列表 | 请有权限的同事执行并截图 |
| `kubectl logs <pod>` | 无日志权限 | 查看应用自身的日志文件（/var/log/） | 使用日志聚合系统（如 ELK/Loki）查询 |
| `kubectl describe node <node>` | 无节点查看权限 | 查看监控系统的节点仪表盘 | 使用 `kubectl get node -o yaml`（如权限允许） |
| `ssh <node>` | 无法 SSH 到节点 | 使用 `kubectl debug node/<node> -it --image=busybox` | 通过跳板机访问：`ssh -J bastion <node>` |
| `systemctl status kubelet` | 无法进入节点 | 查看节点上的 kubelet 日志：`kubectl logs -n kube-system <kubelet-pod>` | 查看容器运行时日志 |
| `docker/crictl` | 无容器运行时权限 | 使用 `kubectl exec` 进入容器检查 | 查看容器运行时的事件 |

### 如果以上都无法执行

如果你因为安全策略、网络隔离或权限限制无法执行任何诊断命令：

1. **请收集你能访问的任何信息**：
   - 监控系统的截图
   - 告警通知的内容
   - 应用自身的错误页面/日志
   - 最近是否有变更（部署、扩缩容、配置更新）

2. **如果信息严重不足**：
   - 我会根据你描述的症状给出最可能的根因和修复建议
   - 但请注意：**信息不足时建议的置信度会降低**
   - 如果问题影响严重，建议立即升级给有权限的高级 SRE

3. **紧急情况下**：
   - 如果业务已中断且你无法执行任何操作
   - 请立即联系有集群管理员权限的同事
   - 同时可以准备以下信息以便快速交接：
     - 问题发生时间
     - 影响范围
     - 已尝试的操作
     - 当前的任何异常观察

## 异常反馈处理

以下场景工程师可能给出异常反馈，需准备应对：

- **rollout undo后仍失败** → 检查ReplicaSet历史版本是否完整

- **新Pod创建成功但旧Pod未终止** → 检查terminationGracePeriod和preStop钩子

- **滚动更新进度卡在某个百分比** → 检查maxUnavailable和maxSurge配置

- **Deployment更新后服务中断** → 检查就绪探针配置和minReadySeconds


## 相关Skill交叉引用

本Skill诊断过程中可能涉及的其他Skill：

- k8s-image-pull

- k8s-config-secret

- k8s-autoscaling


当本Skill的诊断步骤无法定位根因时，建议按上述顺序排查相关Skill。

## 预防性措施

### 发布策略
1. **蓝绿发布**：生产环境核心服务使用蓝绿部署
2. **金丝雀发布**：渐进式流量切换，每步观察5分钟
3. **特性开关**：使用Feature Flag控制功能启用
4. **回滚演练**：每月执行一次回滚演练，验证回滚路径

### 发布检查清单
- [ ] 镜像标签明确（不使用latest）
- [ ] 资源限制已配置
- [ ] 健康检查路径正确
- [ ] 配置变更已验证
- [ ] 数据库迁移已准备
- [ ] 回滚方案已确认

## 诊断决策流程

```mermaid
flowchart TD
    A[工程师报告问题] --> B{Round 1: 快速确认}
    B -->|症状明确| C[执行针对性命令]
    B -->|症状模糊| D[执行通用检查命令]
    C --> E{Round 2: 深度诊断}
    D --> E
    E -->|定位根因| F[执行修复命令]
    E -->|根因不明| G[检查相关Skill]
    F --> H{Round 3: 验证修复}
    G --> H
    H -->|修复成功| I[结束并记录]
    H -->|修复失败| J[升级给高级SRE]
    I --> K[更新监控告警]
    J --> L[准备问题报告]
```

## 工具速查表

| 工具 | 用途 | 典型命令 |
|:---|:---|:---|
| kubectl | Kubernetes CLI | `kubectl get/describe/logs/exec` |
| jq | JSON处理 | `kubectl get ... -o json | jq ...` |
| openssl | 证书检查 | `openssl x509 -in <cert> -noout -dates` |
| tcpdump | 网络抓包 | `tcpdump -i any port <port> -n` |
| strace | 系统调用追踪 | `strace -p <pid> -f` |
| iostat/vmstat | IO/内存监控 | `iostat -x 1` |
| journalctl | 系统日志 | `journalctl -u <service> -f` |
| crictl | 容器运行时 | `crictl ps/logs/inspect` |

## 远程顾问执行清单

- [ ] 确认工程师身份和环境访问权限
- [ ] 收集集群版本、发行版、网络拓扑
- [ ] 确认问题影响范围和紧急程度
- [ ] 指导执行Round 1命令并收集输出
- [ ] 分析输出，选择Round 2分支
- [ ] 指导执行Round 2命令并收集输出
- [ ] 定位根因，提供修复方案
- [ ] 指导执行修复命令并验证
- [ ] 确认修复成功，更新相关文档
- [ ] 评估是否需要升级或事后复盘


## 相关概念

- [[concepts/deployment-controller-architecture.md|Deployment 控制器架构]] — Deployment 滚动更新与副本管理原理


<!-- risk-assessed -->
