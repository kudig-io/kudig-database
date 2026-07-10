---
title: K8s Node NotReady 诊断与修复
description: Kubernetes 节点 NotReady 状态的完整诊断-修复-验证工单处理 Skill
summary: Kubernetes 节点 NotReady 状态的完整诊断-修复-验证工单处理 Skill
category: Kubernetes-Incident-Response
tags:
- k8s
- skills
- sop
- runbook
- apiserver
- kubelet
- prometheus
- containerd
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- K8s Node NotReady 诊断与修复 是什么
- 如何 K8s Node NotReady 诊断与修复
trigger_keywords:
- NotReady
- NodeNotReady
- 节点不可用
- 节点异常
- kubelet stopped
- node unreachable
- 节点不可达
- NodeStatusUnknown
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
skill_id: SKILL-SKILL-001
skill_name: K8s Node NotReady 诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s Node NotReady 诊断与修复

Node NotReady 是 [[Kubernetes|Kubernetes]] 集群中**爆炸半径最大**的问题类型之一。当节点进入 NotReady 状态，控制平面将在 `pod-eviction-timeout`（默认 5 分钟）后驱逐该节点上所有非 [[DaemonSet|DaemonSet]] Pod。控制平面节点 NotReady 可能直接威胁集群可用性。

本 [[SKILL|Skill]] 覆盖 kubelet 异常、容器运行时问题、网络分区、资源压力（磁盘/内存/PID）、证书过期等全部 12 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| `kubectl get nodes` 显示 NotReady | `kubectl get nodes | grep NotReady` | 0.95 |
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
# 🟢 低风险：只读/信息收集，通常无副作用
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
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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
| RC-009 | 内核问题/硬件异常 | 低 | REM-007 替换节点 | HIGH |
| RC-010 | NTP 时间不同步 | 低 | 修复 NTP(手动) | MEDIUM |
| RC-011 | CNI 插件异常 | 中 | 重启 CNI Pod(手动) | MEDIUM |
| RC-012 | 节点被手动 cordon | 低 | REM-001 uncordon | LOW |

> 完整根因详情见 [reference/root-cause-catalog.md](./reference/root-cause-catalog.md)
> 完整修复步骤见 [reference/remediation-playbook.md](./reference/remediation-playbook.md)

## 关联资源

| 资源 | 路径 |
|------|------|
| FTA 问题树 | [故障诊断/topic-fta/list/node-fta.md](../../故障诊断/FTA故障树/list/node-fta.md) |
| 版本兼容矩阵 | [reference/version-matrix.md](./reference/version-matrix.md) |
| 诊断工作流详情 | [reference/diagnostic-workflow.md](./reference/diagnostic-workflow.md) |
| 修复操作手册 | [reference/remediation-playbook.md](./reference/remediation-playbook.md) |
| 根因目录 | [reference/root-cause-catalog.md](./reference/root-cause-catalog.md) |
| 结构化排查 | [故障诊断/topic-structural-trouble-shooting/](../../故障诊断/高级排障/) |
| 单文件完整版 | [../01-node-notready.md](../01-node-notready.md) |

## Related

- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[生态参考/领域索引/node-index.md|Node 知识图谱索引]]


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

- **命令返回"connection refused"** → 检查kubelet端口10250是否被防火墙阻断

- **kubectl get nodes显示所有节点NotReady** → 优先检查apiserver和控制平面

- **SSH节点失败** → 使用带外管理(iLO/iDRAC)或云平台控制台

- **systemctl status kubelet显示依赖服务失败** → 检查container runtime状态


## 相关Skill交叉引用

本Skill诊断过程中可能涉及的其他Skill：

- k8s-pod-pending

- k8s-performance

- k8s-autoscaling


当本Skill的诊断步骤无法定位根因时，建议按上述顺序排查相关Skill。


## 远程顾问特别提示

> 作为部署在客户环境之外的远程顾问，以下场景需要特别注意：

### 信息收集优先级
1. **集群版本和发行版** — 不同发行版（EKS/GKE/ACK/OpenShift）的诊断路径差异很大
2. **网络拓扑** — 是否需要VPN/堡垒机？是否有专门的运维跳板机？
3. **变更时间线** — 近24小时内的所有变更（部署、配置更新、节点操作）
4. **监控数据** — 能否提供Prometheus/Grafana截图或导出数据？

### 受限场景处理
| 限制 | 应对策略 |
|:---|:---|
| 工程师无kubectl权限 | 指导使用Dashboard或提供只读kubeconfig |
| 无法SSH节点 | 依赖kubectl debug/node-shell或云平台控制台 |
| 无法访问日志 | 要求导出关键日志片段或使用日志系统查询 |
| 网络隔离无法下载工具 | 使用容器镜像内置工具或busybox |
| 安全策略禁止执行命令 | 转为配置审查和文档指导 |

### 沟通模板
- **开场**："我是远程SRE顾问，无法直接连接您的集群。请按步骤执行命令并反馈结果。"
- **确认**："请执行上述命令，将输出贴回给我。如有任何异常请立即说明。"
- **升级**："当前情况需要升级处理。请同时联系贵司高级SRE，我会准备详细报告。"
- **结束**："问题已定位，请按上述步骤修复。修复后请验证并反馈结果。如有反复随时联系。"

## 预防性措施

### 监控告警
```yaml
# PrometheusRule - 节点健康
- alert: NodeNotReady
  expr: kube_node_status_condition{condition="Ready",status="true"} == 0
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "节点 {{ $labels.node }} NotReady"

# 磁盘使用率
- alert: NodeDiskPressure
  expr: (node_filesystem_avail_bytes / node_filesystem_size_bytes) < 0.1
  for: 5m
  labels:
    severity: warning
```

### 节点维护SOP
1. **节点上线前**：验证kubelet版本、CNI配置、容器运行时
2. **定期巡检**：每周执行节点健康检查脚本
3. **容量规划**：磁盘使用率告警阈值设为80%
4. **变更管理**：节点操作前标记为不可调度并排空

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

- [[概念/pod-lifecycle.md|Pod 生命周期]] — Pod 创建、运行、终止的完整生命周期
- [[概念/node-lifecycle-management.md|节点生命周期管理]] — Kubernetes 节点状态管理与维护


<!-- risk-assessed -->
