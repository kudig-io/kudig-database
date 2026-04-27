---
title: "K8s Node NotReady 诊断与修复"
description: "Kubernetes 节点 NotReady 状态的完整诊断-修复-验证工单处理 Skill"
author: kudig-io
version: 1.0.0
category: Kubernetes-Incident-Response
skill_id: SKILL-NODE-001
severity_range: P0-P2
k8s_versions: ["1.28.x", "1.29.x", "1.30.x", "1.31.x", "1.32.x"]
tested_on: ["1.28.15", "1.29.12", "1.30.8", "1.31.4", "1.32.0"]
last_validated: "2026-04-26"
risk_level: high
agent_execution_mode: L1-advisory
requires:
  tools:
    - name: kubectl
      min_version: "1.28"
      check_cmd: "kubectl version --client"
    - name: ssh
      description: "SSH access to target nodes for deep diagnostics"
    - name: openssl
      min_version: "1.1.1"
  optional_tools:
    - name: jq
      min_version: "1.6"
    - name: crictl
      min_version: "1.28"
    - name: curl
provides:
  - node_notready_diagnosis
  - node_notready_remediation
  - node_health_verification
trigger_keywords:
  - NotReady
  - NodeNotReady
  - 节点不可用
  - 节点异常
  - kubelet stopped
  - node unreachable
  - 节点不可达
  - NodeStatusUnknown
trigger_events:
  - NodeNotReady
  - NodeStatusUnknown
  - KubeletNotReady
  - NodeHasDiskPressure
  - NodeHasMemoryPressure
  - NodeHasPIDPressure
trigger_metrics:
  - 'kube_node_status_condition{condition="Ready",status="false"}'
  - 'kube_node_status_condition{condition="Ready",status="unknown"}'
  - 'kube_node_status_condition{condition="MemoryPressure",status="true"}'
  - 'kube_node_status_condition{condition="DiskPressure",status="true"}'
  - 'kube_node_status_condition{condition="PIDPressure",status="true"}'
related_skills:
  - SKILL-POD-001
  - SKILL-POD-002
  - SKILL-SEC-001
fta_refs:
  - topic-fta/list/node-fta.md
knowledge_refs:
  - topic-structural-trouble-shooting/
  - domain-12-troubleshooting/
  - domain-1-architecture-fundamentals/
---

# K8s Node NotReady 诊断与修复

Node NotReady 是 Kubernetes 集群中**爆炸半径最大**的故障类型之一。当节点进入 NotReady 状态，控制平面将在 `pod-eviction-timeout`（默认 5 分钟）后驱逐该节点上所有非 DaemonSet Pod。控制平面节点 NotReady 可能直接威胁集群可用性。

本 Skill 覆盖 kubelet 异常、容器运行时故障、网络分区、资源压力（磁盘/内存/PID）、证书过期等全部 12 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| `kubectl get nodes` 显示 NotReady | `kubectl get nodes \| grep NotReady` | 0.95 |
| 节点状态在 Ready/NotReady 间频繁切换 | Events 中交替出现 NodeReady/NodeNotReady | 0.85 |
| Prometheus 告警 `KubeNodeNotReady` 触发 | `kube_node_status_condition{condition="Ready",status="false"}` | 0.95 |
| 节点上 Pod 被大量驱逐 | `kubectl get events --field-selector reason=Evicted` | 0.80 |
| Node Lease 长时间未更新 | `kubectl get lease -n kube-node-lease <node>` | 0.90 |

**排除条件**: 节点 Ready 但 Pod CrashLoopBackOff → SKILL-POD-001; 节点 Ready 但 Pod Pending → SKILL-POD-002; 证书错误但节点 Ready → SKILL-SEC-001

## 快速分级（2 分钟内完成）

```
NotReady 节点数 / 总节点数
├── > 50% ──────────────→ 立即升级（跳过诊断）
├── > 30% 或控制平面节点 → P0（15min 内确认根因）
├── 多个工作节点 ────────→ P1（30min 内修复）
├── 单个工作节点 ────────→ P2（2h 内修复）
└── 新节点未承载业务 ────→ P3（4h 内处理）
```

**立即升级条件**（跳过所有诊断步骤）:
- >50% 节点 NotReady
- 所有控制平面节点 NotReady
- `kubectl get nodes` 本身超时
- NotReady 数量在 5 分钟内持续增加

## 执行流程

```
工单/告警触发
    │
    ▼
┌──────────────┐    脚本: scripts/diagnose-quick.sh
│ Phase 1      │    内容: kubectl 快速检查（只读，零风险）
│ 快速检查      │    Step: D1.1-D1.5
└──────┬───────┘
       │ 无法确认根因
       ▼
┌──────────────┐    脚本: scripts/diagnose-deep.sh
│ Phase 2      │         scripts/check-resources.sh
│ 深度检查      │    内容: SSH 到节点检查（只读，零风险）
│ (需SSH)      │    Step: D2.1-D2.10
└──────┬───────┘
       │ 需主动探测
       ▼
┌──────────────┐    参考: reference/diagnostic-workflow.md
│ Phase 3      │    内容: 主动探测（低风险，可能需审批）
│ 主动探测      │    Step: D3.1-D3.3
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    参考: reference/root-cause-catalog.md
│ 根因匹配      │    数据: assets/root-cause-map.yaml
│ RC-001~012   │
└──────┬───────┘
       │
       ▼
┌──────────────┐    参考: reference/remediation-playbook.md
│ 修复操作      │    脚本: scripts/cleanup-disk.sh (REM-002)
│ REM-001~010  │    风险: LOW → MEDIUM → HIGH → CRITICAL
└──────┬───────┘
       │
       ▼
┌──────────────┐    脚本: scripts/verify-node.sh
│ 验证确认      │    检查: 节点状态/Conditions/Lease/Pod
└──────────────┘
```

## 可用脚本

| 脚本 | 用途 | 参数 | 风险 |
|------|------|------|------|
| `scripts/diagnose-quick.sh` | Phase 1 kubectl 快速检查 | `NODE_NAME` | 只读 |
| `scripts/diagnose-deep.sh` | Phase 2 SSH 深度检查 | `NODE_IP` | 只读 |
| `scripts/check-resources.sh` | 资源压力检查（磁盘/内存/PID/inode） | `NODE_IP` | 只读 |
| `scripts/cleanup-disk.sh` | 磁盘空间清理（REM-002） | `NODE_IP` | 🟢 低风险 |
| `scripts/verify-node.sh` | 修复后节点健康验证 | `NODE_NAME` | 只读 |

**使用方式**:
```bash
# Phase 1: kubectl 快速诊断
bash scripts/diagnose-quick.sh <node-name>

# Phase 2: SSH 深度诊断
bash scripts/diagnose-deep.sh <node-ip>

# 资源压力专项检查
bash scripts/check-resources.sh <node-ip>

# 修复: 清理磁盘空间
bash scripts/cleanup-disk.sh <node-ip>

# 修复后验证
bash scripts/verify-node.sh <node-name>
```

## 根因概览 (12 种)

| RC ID | 根因 | 概率 | 首选修复 | 风险 |
|-------|------|------|---------|------|
| RC-001 | kubelet 进程崩溃或未运行 | 高 | REM-003 重启 kubelet | MEDIUM |
| RC-002 | 容器运行时(containerd)异常 | 高 | REM-004 重启 containerd | MEDIUM |
| RC-003 | 磁盘空间耗尽 (DiskPressure) | 高 | REM-002 清理磁盘 | LOW |
| RC-004 | 内存耗尽 (MemoryPressure) | 中 | REM-006 排空重启 | HIGH |
| RC-005 | PID 耗尽 (PIDPressure) | 中 | REM-003 重启 kubelet | MEDIUM |
| RC-006 | 节点与 apiserver 网络不通 | 中 | 网络修复(手动) | HIGH |
| RC-007 | kubelet 客户端证书过期 | 中 | REM-008 证书轮转 | HIGH |
| RC-008 | PLEG 不健康 | 中 | REM-004 重启 containerd | MEDIUM |
| RC-009 | 内核故障/硬件异常 | 低 | REM-007 替换节点 | HIGH |
| RC-010 | NTP 时间不同步 | 低 | 修复 NTP(手动) | MEDIUM |
| RC-011 | CNI 插件异常 | 中 | 重启 CNI Pod(手动) | MEDIUM |
| RC-012 | 节点被手动 cordon | 低 | REM-001 uncordon | LOW |

> 完整根因详情见 [reference/root-cause-catalog.md](./reference/root-cause-catalog.md)
> 完整修复步骤见 [reference/remediation-playbook.md](./reference/remediation-playbook.md)

## 关联资源

| 资源 | 路径 |
|------|------|
| FTA 故障树 | [topic-fta/list/node-fta.md](../../topic-fta/list/node-fta.md) |
| 版本兼容矩阵 | [reference/version-matrix.md](./reference/version-matrix.md) |
| 诊断工作流详情 | [reference/diagnostic-workflow.md](./reference/diagnostic-workflow.md) |
| 修复操作手册 | [reference/remediation-playbook.md](./reference/remediation-playbook.md) |
| 根因目录 | [reference/root-cause-catalog.md](./reference/root-cause-catalog.md) |
| 结构化排查 | [topic-structural-trouble-shooting/](../../topic-structural-trouble-shooting/) |
| 单文件完整版 | [../01-node-notready.md](../01-node-notready.md) |
