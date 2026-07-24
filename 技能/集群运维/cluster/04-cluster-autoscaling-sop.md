---
title: 集群扩缩容异常诊断与集群级 SOP/Runbook
description: 针对 Cluster Autoscaler / Karpenter 不扩缩容、节点池异常的诊断技能，以及集群级故障的标准操作流程（SOP）与 Runbook 汇总
summary: 集群容量弹性直接影响成本与可用性。本技能提供扩缩容失败诊断路径，并汇总集群级故障的统一 SOP 与升级 Runbook
category: skill
tags:
- k8s
- cluster
- autoscaling
- cluster-autoscaler
- karpenter
- nodepool
- sop
- runbook
- capacity
sources:
- 故障诊断-集群运维/cluster-autoscaler/
- 故障诊断/FTA故障树/kubernetes-fta-full-analysis.md
- code/cloud-provider-master/
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- Cluster Autoscaler 不扩容原因
- 节点池不扩容怎么排查
- 集群扩容失败如何诊断
- 集群级故障 SOP 流程
- 集群缩容不生效怎么办
trigger_keywords:
- Cluster Autoscaler
- CA 不扩容
- Karpenter
- 节点池
- nodepool
- 扩容失败
- 缩容
- capacity
- 集群 SOP
prerequisites:
- kubectl-basics
- cluster-architecture
- autoscaling-basics
skill_id: SKILL-CLUSTER-004
skill_name: 集群扩缩容异常诊断与集群级 SOP/Runbook
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.30.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L2-semi-auto
fta_path: TE-C -> IE-C.5 -> BE-C.6
---

> **生产环境安全提示**
>
> 命令风险等级标注：🔴 高风险、🟡 中风险、🟢 低风险/只读。缩容与节点池变更可能影响运行中工作负载，执行前确认目标与授权。

# 集群扩缩容异常诊断与集群级 SOP/Runbook

> **Skill ID**: SKILL-CLUSTER-004
> **Agent 执行模式**: L2-semi-auto — 只读诊断与低风险配置查看自动执行，扩缩容变更需审批
> **FTA 路径**: TE-C → IE-C.5 → BE-C.6

---

## 1. 概述

集群自动扩缩容（Cluster Autoscaler / Karpenter）根据 Pod Pending 与节点利用率动态增删节点。扩容失败会导致工作负载长期 Pending；缩容异常会造成成本浪费或误删有负载节点。

**覆盖范围**：CA 检测到 Pending 却不扩容、扩容超时、缩容不生效或误缩、节点池配额/权限/污点导致的扩容失败；并汇总集群级故障的统一 SOP。

**边界**：单 Pod 因资源不足 Pending（非集群容量问题）→ [[技能/工作负载/pod/02-pod-pending-scheduling.md|Pod Pending 诊断]]。

---

## 2. 症状识别

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Pod 长期 Pending 但节点未增加 | `kubectl get pod`+节点数不变 | 0.85 | 需确认 CA 已识别该 Pod |
| S2 | CA 日志 `no scale up`/`max node group size reached` | CA Pod 日志 | 0.90 | 节点池已达上限 |
| S3 | CA 日志 `pod didn't trigger scale-up` | CA Pod 日志 | 0.85 | Pod 亲和/污点导致不可调度到新节点 |
| S4 | 扩容节点长期 NotReady | 新节点 join 失败 | 0.80 | 转 Node 技能集 |
| S5 | 缩容不生效，空闲节点保留 | 节点低利用率但不缩 | 0.75 | 有阻止缩容的 Pod（本地存储/PDB） |
| S6 | 云配额/权限报错 | CA 日志云 API 错误 | 0.80 | 配额超限/凭证失效 |

---

## 3. 快速分级

| 级别 | 判定条件 | 响应时限 | 处置 |
|:---:|---------|:---:|------|
| **P0** | 核心业务因无法扩容持续不可用 | 立即 | 手动扩容节点池兜底，并行定位 CA |
| **P1** | 关键工作负载 Pending，容量告急 | ≤15min | 提高节点池上限/修复配额 |
| **P2** | 扩容延迟或缩容不生效，无即时业务影响 | ≤1h | 调 CA 参数/排查阻止缩容因素 |
| **P3** | 偶发扩缩抖动 | ≤1d | 观察 CA 指标与节点利用率 |

---

## 4. 诊断工作流

### Phase 1: 快速定位（只读）

**D1.1**: 确认 Pending Pod 与节点容量

```bash
# 🟢 低风险：只读
kubectl get pods -A --field-selector=status.phase=Pending -o wide
kubectl get nodes
kubectl top nodes 2>/dev/null
```

**D1.2**: 查看 Cluster Autoscaler 状态与日志

```bash
# 🟢 低风险：只读
kubectl -n kube-system logs -l app=cluster-autoscaler --tail=100
# CA 状态 ConfigMap（记录 scale up/down 决策）
kubectl -n kube-system get configmap cluster-autoscaler-status -o yaml
```

### Phase 2: 深度检查（只读）

**D2.1**: 分析不扩容原因（S2/S3 分支）

```bash
# 🟢 低风险：只读
# 在 CA 日志中定位关键行
kubectl -n kube-system logs -l app=cluster-autoscaler | grep -E "scale.up|max size|didn't trigger|failed to increase"
```

- `max node group size reached` → 节点池已达上限（RC-001）
- `pod didn't trigger scale-up: ... node(s) didn't match` → Pod 约束使新节点也不可调度（RC-003）

**D2.2**: 检查阻止缩容的因素（S5 分支）

```bash
# 🟢 低风险：只读
# 带本地存储/无 PDB 保护/kube-system 无控制器的 Pod 会阻止缩容
kubectl get pdb -A
```

### 4.6 证据三元组

```promql
# 🟢 集群不可调度 Pod（触发扩容的信号）
cluster_autoscaler_unschedulable_pods_count > 0

# 🟢 CA 是否有扩容错误
rate(cluster_autoscaler_errors_total[10m]) > 0

# 🟢 节点组是否达上限
cluster_autoscaler_nodes_count >= cluster_autoscaler_max_nodes_count
```

| 维度 | 来源 | 取值 |
|------|------|------|
| Metrics | CA /metrics | unschedulable_pods、errors、node group 上限 |
| Logs | CA Pod 日志 | `max size reached` / `didn't trigger scale-up` / 云 API 错误 |
| Events | Pod events | `FailedScheduling` + `pod didn't trigger scale-up` |

---

## 5. 根因分类

| RC-ID | 根因 | 概率 | 关键证据 | FTA | 修复 | 风险 |
|-------|------|------|---------|-----|------|------|
| RC-001 | 节点池已达 max size | 28% | `max node group size reached` | BE-C.6 | 提高上限 | 🟡 |
| RC-002 | 云配额超限 | 20% | 云 API `quota exceeded` | BE-C.6 | 申请配额 | 🟡 |
| RC-003 | Pod 约束致新节点也不可调度 | 18% | `didn't trigger scale-up` | BE-C.6 | 调整亲和/污点 | 🟡 |
| RC-004 | CA 未管理该节点池 | 12% | 节点组无 CA 标签/自动发现失败 | BE-C.6 | 修正 CA 配置 | 🟡 |
| RC-005 | 云凭证/权限失效 | 10% | 云 API 401/403 | BE-C.6 | 修复凭证/IAM | 🔴 |
| RC-006 | 阻止缩容的 Pod（本地存储/PDB） | 8% | CA 日志 `not eligible for scale down` | BE-C.6 | 加注解/调 PDB | 🟡 |
| RC-007 | 新节点 join 失败 | 4% | 节点 NotReady | BE-C.6 | 转 Node 技能集 | 🟡 |

---

## 6. 修复操作

**REM-001（🟡 中风险）：提高节点池上限**

```bash
# 🟡 中风险：云厂商 CLI 或 CA 配置，示例为通用占位
# 编辑节点池 max size 或 CA --nodes=<min>:<max>:<nodegroup>
```

**REM-003（🟡 中风险）：放宽 Pod 约束使可扩容调度**

```bash
# 🟡 中风险：调整 nodeSelector/亲和/tolerations 使新节点可承载
```

**REM-006（🟡 中风险）：允许缩容带本地存储的 Pod**

```bash
# 🟡 中风险：为可安全驱逐的 Pod 添加注解
kubectl annotate pod <pod> -n <ns> \
  cluster-autoscaler.kubernetes.io/safe-to-evict=true
```

**P0 兜底（🟡 中风险）：手动扩容节点池**

```bash
# 🟡 中风险：紧急扩容兜底，绕过 CA 决策
# 通过云控制台/CLI 手动增加节点池期望节点数
```

---

## 7. 验证确认

| 阶段 | 判据 | 通过标准 |
|------|------|---------|
| 即时验证 | Pending Pod 被调度 | 新节点 Ready 且 Pod Running |
| 短期监控 | CA errors | 10min 内无新增 errors |
| 解决标准 | unschedulable pods | 归零 |
| 回归检测 | 缩容正常 | 空闲节点在冷却期后正常缩容 |

---

## 8. 升级协议

- 核心业务因无法扩容不可用 → P0，手动兜底 + 升级平台/云团队。
- 云配额/权限问题 → 升级云账号管理员。
- 交接信息包：Pending Pod 列表、CA 状态 ConfigMap、CA 关键日志行、节点池上限与云配额、`errors_total` 曲线。

---

## 9. 集群级统一 SOP / Runbook

集群级故障的通用响应流程（适用本技能集全部场景）：

```
集群级告警触发
      │
      ▼
[T1] 影响面判定（1min）：单点/控制面/全集群？→ 定 P0-P3
      │
      ▼
[T2] 快照与备份（P0/P1 强制）：etcd snapshot + /etc/kubernetes 备份
      │
      ▼
[T3] 分域路由：
      ├─ apiserver/控制面 → 01
      ├─ etcd/写失败      → 02
      ├─ x509/升级        → 03
      └─ 容量/扩缩容      → 04
      │
      ▼
[T4] 只读诊断 → 证据三元组取证 → 根因确认
      │
      ▼
[T5] 修复（写操作双人复核）→ 7 章验证闭环
      │
      ▼
[T6] 复盘：更新技能/误诊模式/告警阈值
```

### 集群级操作红线（禁忌）

| 🔴 禁止 | 原因 |
|--------|------|
| 未备份直接 etcd restore | 不可逆数据丢失 |
| 升级跳小版本 | 存储/API 不兼容 |
| 全 etcd 成员并行 defrag | 同时不可用致失 quorum |
| 无审批移除安全 Webhook | 绕过安全策略 |

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊 | 纠正 |
|------|------|
| Pending 即判 CA 故障 | 需先确认是容量不足还是 Pod 约束不可调度 |
| 达 max size 当作 CA bug | 是配置上限，应评估提高上限 |
| 缩容不生效当作卡死 | 多为 PDB/本地存储 Pod 阻止，属预期保护 |

### 10.2 生产案例

**案例: 节点池达上限导致核心服务扩容失败**

| 时间 | 事件 |
|------|------|
| T0 | 流量高峰，大量 Pod Pending |
| T1 | CA 日志 `max node group size reached` |
| 根因 | 节点池 max size 设置过低（RC-001） |
| 修复 | 🟡 提高节点池上限并手动兜底扩容，事后评估容量规划 |

### 10.3 混沌验证

| 注入场景 | 方法（测试集群） | 应命中 | 验证标准 |
|---------|----------------|-------|---------|
| 达上限不扩容 | 设低 max size 并压 Pending | RC-001 | CA 日志 max size reached |
| 约束不可调度 | 部署无匹配节点的亲和 Pod | RC-003 | didn't trigger scale-up |

---

## 11. 云厂商特异性

| 厂商 | 差异 |
|------|------|
| 阿里云 ACK | CA 与弹性伸缩组集成，配额在 ECS 侧；也支持 ACK 虚拟节点(ECI)弹性 |
| AWS EKS | CA 基于 ASG；Karpenter 为推荐的更快弹性方案 |
| GKE | Autopilot 全托管弹性；Standard 用 CA |

---

## 12. 自动化集成接口

```json
{
  "skill_id": "SKILL-CLUSTER-004",
  "symptom": "scale_up_failed",
  "unschedulable_pods": 12,
  "root_cause": "RC-001",
  "action": "raise_nodegroup_max",
  "requires_approval": true,
  "risk": "medium"
}
```

- 🟢 自动执行：所有只读诊断、CA 日志/状态读取
- 🟡 需审批：提高节点池上限、手动扩容、调整 PDB/注解
- 🔴 禁止自动：修改云 IAM/凭证

---

## 相关链接

- [[技能/集群运维/cluster/README.md|Cluster 集群级故障诊断技能集]]
- [[技能/集群运维/cluster/01-apiserver-controlplane.md|控制平面不可用诊断]]
- [[技能/集群运维/cluster/02-etcd-troubleshooting.md|etcd 故障诊断]]
- [[技能/工作负载/pod/02-pod-pending-scheduling.md|Pod Pending 诊断]]
- [[技能/节点/node/README.md|Node 异常诊断技能集]]
- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[cluster-autoscaler]] — 集群自动扩缩容
- [[kube-scheduler]] — 调度器
