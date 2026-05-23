---
title: 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
description: '- 运维工程师'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- prometheus
- containerd
- docker
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation 是什么
- 如何 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation 故障排查
- 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation 排障步骤
trigger_keywords:
- 节点资源压力诊断与修复
- Node
- Resource
- Pressure
- Diagnosis
- Remediation
- troubleshooting
- diagnostics
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- etcd-basics
- logging-basics
skill_id: SKILL-19_NODE_RESOURCE_PRESSURE-001
skill_name: 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
version: 1.0.0
created: "2026-05-23"
---

---
skill_id: "SKILL-NODE-002"
skill_name: "节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation"
version: "1.0"
category: "node"
severity_range: "P0-P2"
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
estimated_resolution_time: "10-60min"
risk_level: "high"
agent_execution_mode: "L2-semi-auto"
trigger_keywords:
  - "MemoryPressure"
  - "DiskPressure"
  - "PIDPressure"
  - "Evicted"
  - "节点资源压力"
  - "内存压力"
  - "磁盘压力"
  - "inode耗尽"
  - "OOM"
  - "节点驱逐"
trigger_events:
  - "NodeHasDiskPressure"
  - "NodeHasMemoryPressure"
  - "NodeHasPIDPressure"
  - "Evicted"
  - "NodeHasInsufficientMemory"
  - "InvalidDiskCapacity"
trigger_metrics:
  - 'kube_node_status_condition{condition="MemoryPressure",status="true"}'
  - 'kube_node_status_condition{condition="DiskPressure",status="true"}'
  - 'kube_node_status_condition{condition="PIDPressure",status="true"}'
  - 'node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes < 0.1'
  - 'node_filesystem_avail_bytes / node_filesystem_size_bytes < 0.15'
difficulty: "advanced"
reading_level: "advanced"
audience:
  - SRE
  - 运维工程师
  - 技术支持
estimated_read_time: "15min"
prerequisites:
  - "domain-10-troubleshooting-diagnostics"
  - "kubectl-basics"
  - "linux-resource-management"
related_skills:
  - "SKILL-NODE-001"
  - "SKILL-POD-001"
  - "SKILL-POD-002"
  - "SKILL-PERF-001"
  - "SKILL-STORE-001"
fta_refs:
  - "domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta.md"
knowledge_refs:
  - "domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis.md"
  - "domain-10-troubleshooting-diagnostics/09-node-comprehensive-troubleshooting.md"
  - "domain-10-troubleshooting-diagnostics/35-node-component-troubleshooting.md"
cross_refs:
  - type: "fta"
    path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta.md"
    label: "Node 故障树分析"
  - type: "domain"
    path: "../domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis.md"
    label: "OOM 内存问题深度诊断"
  - type: "domain"
    path: "../domain-10-troubleshooting-diagnostics/09-node-comprehensive-troubleshooting.md"
    label: "Node 全面故障排查"
  - type: "[[SKILL|skill]]"
    path: "../domain-10-troubleshooting-diagnostics/topic-skills/01-node-notready.md"
    label: "SKILL-NODE-001 节点 NotReady 诊断"
authors:
  - name: KUDIG Team
    role: contributor

tier: peripheral---

# 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation

节点资源压力（MemoryPressure / DiskPressure / PIDPressure）是 [[Kubernetes|Kubernetes]] 集群中最常见但常被忽视的故障类型。与 Node NotReady 不同，处于资源压力状态的节点仍标记为 `Ready`，但 kubelet 会主动驱逐 Pod 以回收资源。若不及时处理，资源压力可能级联扩散，导致大规模 Pod 驱逐、服务降级甚至集群雪崩。

本 Skill 覆盖内存压力、磁盘压力（含 inode 耗尽）、PID 压力、镜像/容器存储膨胀、系统 OOM 等全部 10 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| `kubectl get nodes` 显示 MemoryPressure/DiskPressure/PIDPressure | `kubectl get nodes` 或 Prometheus 告警 | 0.95 |
| 大量 Pod 被驱逐（Evicted） | `kubectl get pods -A | grep Evicted` | 0.90 |
| Prometheus 告警 `KubeNodePressure` 触发 | `kube_node_status_condition{condition="*Pressure",status="true"}` | 0.95 |
| 节点上 Pod 频繁 OOMKilled | `kubectl get events --field-selector reason=OOMKilled` | 0.85 |
| 镜像拉取失败（磁盘满） | `ImagePullBackOff` + 节点 DiskPressure | 0.80 |
| 新 Pod 无法调度到特定节点 | `FailedScheduling` + 目标节点有 Pressure 条件 | 0.85 |

**排除条件**: 节点状态为 NotReady → SKILL-NODE-001; Pod CrashLoopBackOff 但节点无 Pressure → SKILL-POD-001; PVC Pending → SKILL-STORE-001

## 快速分级（2 分钟内完成）

```
压力类型 + 影响范围
├── 多节点同时出现 MemoryPressure ──────→ P0（立即处理，可能集群级内存不足）
├── 单节点 DiskPressure + 运行核心服务 ─→ P0（30min 内修复，防止服务驱逐）
├── 单节点 MemoryPressure ─────────────→ P1（1h 内修复）
├── 单节点 DiskPressure（非核心服务）───→ P1（2h 内修复）
├── PIDPressure ────────────────────────→ P1（1h 内修复，通常关联内存/磁盘问题）
└── 轻微压力（阈值附近）────────────────→ P2（4h 内处理，预防性）
```

**立即升级条件**（跳过所有诊断步骤）：
- >30% 节点同时出现同一类型 Pressure
- 控制平面节点出现 MemoryPressure（可能威胁 etcd）
- 压力导致核心服务 Pod 被驱逐（如 kube-system 命名空间）
- 磁盘满导致节点无法写入日志/状态（可能导致 NotReady 级联）

## 执行流程

```
工单/告警触发
    │
    ▼
┌──────────────┐    Step: D1.1-D1.5
│ Phase 1      │    内容: kubectl 快速检查（只读，零风险）
│ 快速检查      │
└──────┬───────┘
       │ 无法确认根因或需定量分析
       ▼
┌──────────────┐    Step: D2.1-D2.8
│ Phase 2      │    内容: SSH 深度检查（只读，零风险）
│ 深度检查      │
└──────┬───────┘
       │ 需主动探测/清理
       ▼
┌──────────────┐    Step: D3.1-D3.3
│ Phase 3      │    内容: 主动探测/低风险修复（可能需审批）
│ 主动探测      │
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    RC-001~010
│ 根因匹配      │
└──────┬───────┘
       │
       ▼
┌──────────────┐    REM-001~009
│ 修复操作      │    风险: LOW → MEDIUM → HIGH → CRITICAL
└──────┬───────┘
       │
       ▼
┌──────────────┐    V1~V6
│ 验证确认      │
└──────────────┘
```

## 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 节点状态包含 MemoryPressure | `kubectl get nodes` | 0.95 | 节点 NotReady → SKILL-NODE-001 |
| S2 | 节点状态包含 DiskPressure | `kubectl get nodes` | 0.95 | 节点 NotReady → SKILL-NODE-001 |
| S3 | 节点状态包含 PIDPressure | `kubectl get nodes` | 0.95 | 节点 NotReady → SKILL-NODE-001 |
| S4 | Pod 状态为 Evicted | `kubectl get pods -A | grep Evicted` | 0.90 | 节点被 drain → 人工操作 |
| S5 | Pod 被 OOMKilled (exit 137) | `kubectl get events --field-selector reason=OOMKilled` | 0.85 | 容器 limits 过低 → SKILL-POD-001 |
| S6 | 镜像拉取失败且节点 DiskPressure | `ImagePullBackOff` + DiskPressure | 0.80 | 镜像不存在 → SKILL-IMAGE-001 |
| S7 | 调度失败且目标节点有 Pressure | `FailedScheduling` + 节点 Pressure | 0.85 | 资源不足 → SKILL-POD-002 |
| S8 | 容器运行时响应缓慢 | `crictl ps` 超时 | 0.75 | 运行时崩溃 → SKILL-NODE-001 |

### 2.2 工单关键词映射

- "节点内存压力大，Pod 被驱逐了"
- "DiskPressure 告警，节点磁盘快满了"
- "很多 Pod 显示 Evicted，节点好像有问题"
- "容器被 OOM kill，exit code 137"
- "节点 PID 压力，无法创建新进程"
- "镜像拉取失败，提示 no space left on device"

### 2.3 排除标准

- 节点状态为 NotReady 而不是 Ready + Pressure → 使用 SKILL-NODE-001
- Pod 被驱逐但节点无 Pressure（可能是手动 drain/污点）→ 检查是否为维护操作
- 容器 OOMKilled 但节点内存充足（容器 limits 设置过低）→ 使用 SKILL-POD-001
- 磁盘 I/O 性能问题但空间充足 → 使用 SKILL-PERF-001

## 快速分级（2 分钟内完成）

### 3.1 影响评估

**Step T1**: 统计压力节点数量与比例
```bash
kubectl get nodes -o json | jq -r '
  .items[] |
  select(.status.conditions[]?.status == "True" and
         (.status.conditions[]?.type | test("Pressure"))) |
  .metadata.name'
```
> **判断规则**: 若数量 > 总节点数 30% → 影响范围集群级，立即升级

**Step T2**: 检查被驱逐 Pod 的数量和命名空间
```bash
kubectl get pods -A --field-selector=status.phase=Failed | grep Evicted | \
  awk '{print $1}' | sort | uniq -c | sort -rn
```
> **判断规则**: 若 kube-system 命名空间有 Evicted Pod → 核心服务受影响，P0

**Step T3**: 检查压力节点的运行 Pod 数量和关键度
```bash
kubectl get pods -A --field-selector spec.nodeName=<node-name> \
  -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name | \
  grep -v kube-system | wc -l
```
> **判断规则**: 若运行生产核心服务 → P0/P1

**Step T4**: 检查资源压力的历史趋势
```bash
# 通过 Prometheus 查询（如可用）
# 内存压力趋势
echo 'query: increase(kube_node_status_condition{condition="MemoryPressure",status="true"}[1h])'
# 磁盘剩余趋势
echo 'query: predict_linear(node_filesystem_avail_bytes[1h], 3600) < 0'
```
> **判断规则**: 若趋势显示压力持续恶化 → 提升一级处理

### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| >30% 节点同类型 Pressure 或控制平面节点 Pressure | P0 | 15min 内确认根因并启动修复 |
| 单节点 DiskPressure + 核心服务 或 单节点 MemoryPressure + 大量 Evicted | P1 | 1h 内修复 |
| 单节点 PIDPressure 或 单节点轻微 DiskPressure | P2 | 2-4h 内修复 |
| 阈值附近（如磁盘 82%，阈值 85%）预防性处理 | P3 | 下次维护窗口处理 |

### 3.3 立即升级触发条件

- >30% 节点出现 MemoryPressure（可能是集群内存规划不足）
- etcd 所在节点出现 DiskPressure（威胁集群元数据）
- 磁盘 100% 满导致 kubelet/apiserver 无法写入（可能级联为 NotReady）
- 压力节点上运行有状态服务（StatefulSet + PV）且无法迁移

## 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集节点资源状态，无需 SSH。所有命令均为只读。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取所有压力节点概览
- **命令**:
  ```bash
  kubectl get nodes -o custom-columns=\
    NAME:.metadata.name,\
    STATUS:.status.conditions[-1].type,\
    MEM_PRESSURE:"status.conditions[?(@.type=='MemoryPressure')].status",\
    DISK_PRESSURE:"status.conditions[?(@.type=='DiskPressure')].status",\
    PID_PRESSURE:"status.conditions[?(@.type=='PIDPressure')].status"
  ```
- **超时**: 10s
- **预期输出模式**: 节点列表及三种压力条件状态
- **判断规则**:
  - MemoryPressure=True → 记录节点，关注 RC-001/002/003
  - DiskPressure=True → 记录节点，关注 RC-004/005/006/007
  - PIDPressure=True → 记录节点，关注 RC-008/009
  - 多种 Pressure 同时出现 → 通常根因关联（如 DiskPressure + PIDPressure 可能都是日志膨胀导致）
- **版本差异**: 无

**Step D1.2**: 检查节点详细 Conditions 和事件
- **命令**:
  ```bash
  kubectl describe node <node-name> | grep -A 10 "Conditions:"
  kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
    --sort-by=.lastTimestamp | tail -20
  ```
- **超时**: 15s
- **预期输出模式**: Conditions 详情和近期事件
- **判断规则**:
  - 出现 `NodeHasDiskPressure` 事件 → RC-004/005/006/007
  - 出现 `NodeHasMemoryPressure` 事件 → RC-001/002/003
  - 出现 `NodeHasPIDPressure` 事件 → RC-008/009
  - 出现 `Evicted` 事件，Reason 包含 `The node was low on resource` → 确认资源类型
  - 事件 Message 包含 `eviction threshold` → 记录具体阈值和当前值
- **版本差异**: 无

**Step D1.3**: 检查被驱逐 Pod 详情
- **命令**:
  ```bash
  kubectl get pods -A --field-selector=status.phase=Failed -o json | \
    jq -r '.items[] | select(.status.reason=="Evicted") |
    "\(.metadata.namespace)/\(.metadata.name) | \(.spec.nodeName) | \(.status.message)"' | head -20
  ```
- **超时**: 10s
- **预期输出模式**: 被驱逐 Pod 列表及驱逐原因
- **判断规则**:
  - Message 包含 `memory` → RC-001/002/003（内存压力）
  - Message 包含 `disk` / `ephemeral-storage` → RC-004/005/006/007（磁盘压力）
  - Message 包含 `pid` → RC-008/009（PID 压力）
  - 驱逐数量多且集中 → 压力严重，需立即处理
- **版本差异**: 无

**Step D1.4**: 检查节点资源分配和预留
- **命令**:
  ```bash
  kubectl describe node <node-name> | grep -A 20 "Allocated resources"
  kubectl get node <node-name> -o jsonpath='{.status.allocatable}' | jq .
  ```
- **超时**: 10s
- **预期输出模式**: Allocated resources 表格和 allocatable 字段
- **判断规则**:
  - memory requests 接近 allocatable memory → 可能触发 MemoryPressure（RC-001/002）
  - ephemeral-storage requests 接近 allocatable → 可能触发 DiskPressure（RC-005）
  - 注意：requests 不等于实际使用，需结合 Phase 2 深入检查
- **版本差异**: 无

**Step D1.5**: 检查节点上运行的 Pod 资源使用（Top 排序）
- **命令**:
  ```bash
  kubectl top node <node-name>
  kubectl top pods -A --sort-by=memory | grep <node-name> | head -10
  ```
- **超时**: 15s
- **预期输出模式**: 节点和 Pod 资源使用量
- **判断规则**:
  - 节点内存使用接近 100% → RC-001/002/003
  - 单个 Pod 内存使用异常高 → 可能是内存泄漏（RC-003 变种）
  - kubectl top 不可用（metrics-server 故障）→ 记录，后续通过 SSH 检查
- **版本差异**: 无

### Phase 2: 深度检查（只读，零风险，需 SSH）

> **目标**: SSH 登录压力节点，检查系统级资源使用。所有命令均为只读。
> **前提**: 需要对压力节点的 SSH 访问权限
> **预计耗时**: 5-15 分钟

**Step D2.1**: 检查系统内存使用详情
- **命令**:
  ```bash
  ssh <node-ip> "free -m && echo '---' && cat /proc/meminfo | grep -E '^(Mem|Swap|Buffers|Cached|Active|Inactive)'"
  ```
- **超时**: 10s
- **预期输出模式**: 内存使用统计
- **判断规则**:
  - available 内存 < 100Mi → 严重内存不足（RC-001/002）
  - buffers/cached 占比高但 available 低 → 可能缓存过多，可尝试清理（但通常不会导致 Pressure）
  - Swap 使用量高 → 内存不足的信号（RC-001），需注意 v1.30+ swap 支持可能改变行为
- **版本差异**:
  - **[v1.30+]**: NodeSwap beta，swap 使用可能是预期行为，需检查 kubelet 配置 `swapBehavior`

**Step D2.2**: 检查系统磁盘使用详情（含 inode）
- **命令**:
  ```bash
  ssh <node-ip> "df -h / /var/lib/kubelet /var/lib/containerd /var/log /tmp && echo '---INODE---' && df -i / /var/lib/kubelet /var/lib/containerd"
  ```
- **超时**: 10s
- **预期输出模式**: 各挂载点磁盘和 inode 使用率
- **判断规则**:
  - /var/lib/kubelet 使用率 > 85% → RC-005（kubelet 存储膨胀）
  - /var/lib/containerd 使用率 > 85% → RC-006（镜像/容器层膨胀）
  - /var/log 使用率 > 85% → RC-004（日志膨胀）
  - inode 使用率 > 90% → RC-007（inode 耗尽，大量小文件）
  - /tmp 使用率 > 85% → RC-004 或应用程序临时文件
- **版本差异**: 无

**Step D2.3**: 检查容器运行时磁盘使用
- **命令**:
  ```bash
  ssh <node-ip> "crictl ps -a | wc -l && echo '---' && crictl images | wc -l && echo '---' && du -sh /var/lib/containerd 2>/dev/null || du -sh /var/lib/docker 2>/dev/null"
  ```
- **超时**: 15s
- **预期输出模式**: 容器数量、镜像数量、运行时存储总大小
- **判断规则**:
  - 停止容器（Exited）数量异常多 → RC-006（未清理的停止容器）
  - 镜像数量异常多（>100）→ RC-006（未清理的 dangling 镜像）
  - 运行时存储目录大小异常 → RC-005/006
- **版本差异**: 无

**Step D2.4**: 检查 kubelet 日志中的驱逐详情
- **命令**:
  ```bash
  ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager | grep -iE 'evict|pressure|threshold|disk|memory|pid' | tail -30"
  ```
- **超时**: 15s
- **预期输出模式**: kubelet 驱逐相关日志
- **判断规则**:
  - 日志包含 `eviction manager: must evict pod(s)` → 确认驱逐触发
  - 日志包含 `eviction threshold` 和具体数值 → 记录阈值（如 `imagefs.available<15%`）
  - 日志包含 `memory.available` 和当前值 → RC-001/002
  - 日志包含 `nodefs.available` 或 `imagefs.available` → RC-004/005/006/007
  - 日志包含 `pid.available` → RC-008/009
- **版本差异**: 无

**Step D2.5**: 检查系统 PID 使用
- **命令**:
  ```bash
  ssh <node-ip> "echo 'Current PIDs: ' && ps aux --no-heading | wc -l && echo 'Max PIDs: ' && cat /proc/sys/kernel/pid_max && echo 'Threads per user: ' && ps -eo user | sort | uniq -c | sort -rn | head -10"
  ```
- **超时**: 10s
- **预期输出模式**: PID 使用统计和用户线程分布
- **判断规则**:
  - 当前 PID 数量接近 pid_max（通常 32768 或 4194304）→ RC-008（系统级 PID 耗尽）
  - 单个用户进程数异常高 → RC-009（应用泄漏线程/进程）
  - 容器内进程数过多 → RC-009（容器内进程泄漏）
- **版本差异**: 无

**Step D2.6**: 检查大文件和日志目录
- **命令**:
  ```bash
  ssh <node-ip> "du -h /var/log /var/lib/containerd /var/lib/kubelet /tmp 2>/dev/null | sort -rh | head -20"
  ssh <node-ip> "find /var/log -type f -size +100M -exec ls -lh {} \; 2>/dev/null | head -10"
  ```
- **超时**: 15s
- **预期输出模式**: 大目录和大文件列表
- **判断规则**:
  - 单个日志文件 > 1GB → RC-004（日志未轮转）
  - /var/lib/containerd 中某层目录异常大 → RC-006（镜像/容器层问题）
  - /var/lib/kubelet/pods 中某 Pod 目录异常大 → RC-005（emptyDir/Pod 日志膨胀）
- **版本差异**: 无

**Step D2.7**: 检查内核 OOM Killer 日志
- **命令**:
  ```bash
  ssh <node-ip> "dmesg -T | grep -i 'killed process\|oom-killer\|out of memory' | tail -20"
  ```
- **超时**: 10s
- **预期输出模式**: OOM Killer 触发的进程列表
- **判断规则**:
  - 出现 `Out of memory: Killed process` → RC-002（系统级 OOM）
  - 被杀进程包含 kubelet/containerd → 严重，可能导致节点不稳定
  - OOM Killer 频繁触发 → 内存严重不足，需立即释放或扩容
- **版本差异**: 无

**Step D2.8**: 检查 kubelet 配置中的驱逐阈值
- **命令**:
  ```bash
  ssh <node-ip> "cat /var/lib/kubelet/config.yaml | grep -A 20 'evictionHard\|evictionSoft'"
  ```
- **超时**: 10s
- **预期输出模式**: kubelet 驱逐阈值配置
- **判断规则**:
  - 自定义阈值 vs 默认值对比 → 判断是否阈值设置过严
  - 默认阈值：`memory.available<100Mi`，`nodefs.available<10%`，`imagefs.available<15%`，`nodefs.inodesFree<5%`
  - 阈值过严（如 `memory.available<500Mi`）→ 可能频繁误触发
- **版本差异**: 无

### Phase 3: 主动探测（低风险，可能需审批）

> ⚠️ 以下步骤涉及磁盘清理或进程检查，在 L1-advisory 模式下，Agent 应**提出建议并等待人工确认**后执行。
> **预计耗时**: 5-10 分钟

**Step D3.1**: 检查可安全清理的资源
- **命令**:
  ```bash
  ssh <node-ip> "crictl ps -a | grep Exited | wc -l"
  ssh <node-ip> "crictl images | grep '<none>' | wc -l"
  ssh <node-ip> "docker system df 2>/dev/null || echo 'containerd: check manually'"
  ```
- **超时**: 15s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: 可清理资源数量
- **判断规则**:
  - 停止容器数量多 → REM-003 可清理
  - dangling 镜像数量多 → REM-004 可清理
  - 可清理空间估计 → 判断清理是否能解决问题

**Step D3.2**: 检查 Pod 空目录（emptyDir）使用
- **命令**:
  ```bash
  ssh <node-ip> "du -sh /var/lib/kubelet/pods/*/volumes/kubernetes.io~empty-dir/* 2>/dev/null | sort -rh | head -10"
  ```
- **超时**: 10s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: emptyDir 使用排序
- **判断规则**:
  - 单个 emptyDir > 1GB → 应用使用 emptyDir 存储大量数据 → RC-005
  - 需确认应用是否应使用 PVC 替代 emptyDir

**Step D3.3**: 测试镜像拉取能力（验证磁盘空间是否影响运行时）
- **命令**:
  ```bash
  ssh <node-ip> "crictl pull busybox:latest 2>&1 | head -5"
  ```
- **超时**: 30s
- **风险级别**: 🟡 中（会下载小镜像，可能消耗少量磁盘空间）
- **预期输出模式**: 镜像拉取成功/失败信息
- **判断规则**:
  - 拉取失败，提示 `no space left on device` → 确认 DiskPressure 影响新 Pod 创建
  - 拉取成功 → 磁盘空间可能刚好在阈值边缘

## 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | 节点内存实际耗尽（非 kubelet 阈值） | 高 | D2.1 available<100Mi; D2.7 OOM Killer 触发 | memory_exhaustion |
| RC-002 | kubelet 内存驱逐阈值过于严格 | 中 | D2.8 阈值设置; D1.2 事件; D2.1 实际内存充足 | eviction_threshold_too_strict |
| RC-003 | 容器/应用内存泄漏导致节点内存不足 | 中 | D1.5 单个 Pod 内存异常高; D2.7 重复 OOM 同一进程 | container_memory_leak |
| RC-004 | 日志文件膨胀（未轮转或应用日志过多） | 高 | D2.2 /var/log 使用率高; D2.6 大日志文件 | log_exhaustion |
| RC-005 | kubelet/emptyDir/Pod 数据占用磁盘 | 中 | D2.2 /var/lib/kubelet 使用率高; D3.2 emptyDir 大 | kubelet_storage_bloat |
| RC-006 | 镜像/容器层膨胀（未清理 dangling 镜像和停止容器） | 高 | D2.2 /var/lib/containerd 使用率高; D2.3 镜像/容器数量多; D3.1 可清理资源多 | image_layer_bloat |
| RC-007 | inode 耗尽（大量小文件） | 低 | D2.2 inode 使用率>90%; D2.6 大量小文件目录 | inode_exhaustion |
| RC-008 | 系统 PID 上限耗尽 | 低 | D2.5 PID 接近上限; D1.2 PIDPressure 事件 | pid_exhaustion |
| RC-009 | 容器/应用线程泄漏导致 PID 压力 | 低 | D2.5 单个用户线程数异常高; D1.3 驱逐原因包含 pid | thread_leak |
| RC-010 | 临时文件堆积（/tmp 或其他目录） | 中 | D2.2 /tmp 使用率高; D2.6 /tmp 中大文件多 | temp_file_accumulation |

## 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 清理已停止的容器
- **适用根因**: RC-006
- **前置检查**:
  ```bash
  ssh <node-ip> "crictl ps -a | grep Exited | wc -l"
  # 预期: 输出 > 0
  ```
- **执行命令**:
  ```bash
  ssh <node-ip> "crictl ps -a | grep Exited | awk '{print \$1}' | xargs -r crictl rm"
  # 或使用 docker:
  # ssh <node-ip> "docker container prune -f"
  ```
- **后置验证**:
  ```bash
  ssh <node-ip> "crictl ps -a | grep Exited | wc -l"
  # 预期: 输出 0 或大幅减少
  ssh <node-ip> "df -h /var/lib/containerd"
  # 预期: 可用空间增加
  ```
- **回滚命令**: 无法回滚（已停止容器已删除，不影响运行中的 Pod）

#### REM-002: 清理 dangling 镜像
- **适用根因**: RC-006
- **前置检查**:
  ```bash
  ssh <node-ip> "crictl images | grep '<none>' | wc -l"
  # 或使用 docker:
  # ssh <node-ip> "docker images -f dangling=true -q | wc -l"
  ```
- **执行命令**:
  ```bash
  # containerd: 需手动清理（crictl 无 dangling 清理命令）
  # 可查找无标签镜像并删除
  ssh <node-ip> "crictl images -q | xargs -I {} crictl rmi {} 2>/dev/null || true"
  # 或使用 ctr:
  ssh <node-ip> "ctr -n k8s.io images ls | grep -v '\\S' | awk '{print \$1}' | xargs -r ctr -n k8s.io images rm"
  # docker:
  # ssh <node-ip> "docker image prune -f"
  ```
- **后置验证**:
  ```bash
  ssh <node-ip> "crictl images | wc -l"
  ssh <node-ip> "df -h /var/lib/containerd"
  ```
- **回滚命令**: 无法回滚，但 dangling 镜像不影响运行（拉取时间会略微增加）
- **注意事项**: 清理前确认无正在进行的镜像拉取操作

#### REM-003: 清理 Pod emptyDir 数据
- **适用根因**: RC-005
- **前置检查**:
  ```bash
  ssh <node-ip> "du -sh /var/lib/kubelet/pods/*/volumes/kubernetes.io~empty-dir/* 2>/dev/null | sort -rh | head -5"
  # 确认要清理的 Pod
  ```
- **执行命令**:
  ```bash
  # 方式1: 删除 Pod 让其重建（推荐，安全）
  kubectl delete pod <pod-name> -n <namespace>
  # Pod 重建后 emptyDir 会被清空

  # 方式2: 直接进入 emptyDir 清理（风险更高，需确认应用状态）
  ssh <node-ip> "rm -rf /var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~empty-dir/<volume-name>/*"
  ```
- **后置验证**:
  ```bash
  ssh <node-ip> "du -sh /var/lib/kubelet/pods/*/volumes/kubernetes.io~empty-dir/* 2>/dev/null | sort -rh | head -5"
  kubectl get pod <pod-name> -n <namespace>
  ```
- **回滚命令**: 无法回滚 emptyDir 数据（设计为临时存储）
- **注意事项**: 确认 emptyDir 中的数据为临时数据，删除不影响业务

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-004: 清理日志文件
- **适用根因**: RC-004
- **影响说明**: 删除日志文件会丢失历史日志数据，可能影响故障排查。建议先确认日志已收集到中央日志系统。
- **审批提示**: "建议清理节点 <node> 上的日志文件，预计释放 X GB 空间，影响历史日志查询。是否批准？"
- **前置检查**:
  ```bash
  ssh <node-ip> "find /var/log -type f -size +100M -exec ls -lh {} \; | head -10"
  # 确认日志收集状态（如 [[domain-19-landscape-references/01-cncf-landscape/graduated/fluentd/fluentd|Fluentd]]/Fluent Bit 是否正常运行）
  kubectl get pods -n logging -l app=fluentd
  ```
- **执行命令**:
  ```bash
  # 方式1: 手动清理大日志文件
  ssh <node-ip> "find /var/log -type f -size +500M -exec truncate -s 0 {} \;"

  # 方式2: 清理 journal 日志
  ssh <node-ip> "journalctl --vacuum-size=500M"

  # 方式3: 强制轮转日志
  ssh <node-ip> "logrotate -f /etc/logrotate.conf"
  ```
- **后置验证**:
  ```bash
  ssh <node-ip> "df -h /var/log && du -sh /var/log"
  ```
- **回滚命令**: 日志文件清空后无法恢复（需确保已归档）
- **预防措施**: 配置 logrotate 和日志大小限制

#### REM-005: 调整 kubelet 驱逐阈值（临时）
- **适用根因**: RC-002（阈值过严）
- **影响说明**: 放宽阈值会减少驱逐频率，但可能增加节点资源耗尽风险。仅作为临时措施。
- **审批提示**: "建议临时放宽节点 <node> 的 kubelet 驱逐阈值，从 X 调整为 Y。这可能导致更晚触发驱逐。是否批准？"
- **前置检查**:
  ```bash
  ssh <node-ip> "cat /var/lib/kubelet/config.yaml | grep -A 10 evictionHard"
  # 记录当前阈值
  ```
- **执行命令**:
  ```bash
  # 修改 kubelet 配置（需根据实际配置路径调整）
  ssh <node-ip> "sudo sed -i 's/memory.available<100Mi/memory.available<50Mi/' /var/lib/kubelet/config.yaml"
  ssh <node-ip> "sudo systemctl restart kubelet"
  ```
- **后置验证**:
  ```bash
  kubectl get node <node-name>
  # 预期: Pressure 条件消失或改善
  ```
- **回滚命令**:
  ```bash
  ssh <node-ip> "sudo sed -i 's/memory.available<50Mi/memory.available<100Mi/' /var/lib/kubelet/config.yaml"
  ssh <node-ip> "sudo systemctl restart kubelet"
  ```
- **注意事项**: 此操作为临时措施，需配合资源扩容或应用优化

#### REM-006: 清理临时文件
- **适用根因**: RC-010
- **影响说明**: /tmp 中的文件可能被运行中的应用使用，清理前需确认。
- **审批提示**: "建议清理节点 <node> 的 /tmp 目录，预计释放 X GB。是否批准？"
- **前置检查**:
  ```bash
  ssh <node-ip> "lsof +D /tmp | grep -v 'COMMAND' | head -20"
  # 检查是否有进程正在使用 /tmp 中的文件
  ```
- **执行命令**:
  ```bash
  # 清理超过 7 天的临时文件
  ssh <node-ip> "find /tmp -type f -atime +7 -delete 2>/dev/null"
  # 或使用 tmpwatch/tmpreaper
  ```
- **后置验证**:
  ```bash
  ssh <node-ip> "du -sh /tmp"
  ```
- **回滚命令**: 临时文件通常无需回滚

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-007: 驱逐节点上非关键 Pod（手动选择）
- **适用根因**: RC-001/003（内存紧急释放）
- **影响说明**: 手动驱逐 Pod 会导致服务中断。需优先驱逐可中断的、非关键的 Pod。
- **操作步骤**:
  1. 列出节点上所有 Pod，按优先级和 QoS 排序
     ```bash
     kubectl get pods -A --field-selector spec.nodeName=<node-name> \
       -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,\
       QOS:.status.qosClass,PRIORITY:.spec.priority | sort -k3,3
     ```
  2. 优先驱逐 BestEffort 和低优先级的 Pod
     ```bash
     kubectl delete pod <pod-name> -n <namespace>
     ```
  3. 监控内存释放情况
     ```bash
     ssh <node-ip> "free -m"
     ```
- **安全检查**: 确认驱逐的 Pod 不是核心服务；确认有副本在其他节点运行
- **回滚方案**: 重新创建被删除的 Pod（Deployment/StatefulSet 会自动重建）

#### REM-008: 扩容节点磁盘
- **适用根因**: RC-004/005/006/007（磁盘长期不足）
- **影响说明**: 涉及云厂商操作或 LVM 调整，可能需要停机。
- **操作步骤**:
  1. 云环境：通过云厂商控制台扩容云盘
     ```bash
     # ACK
     aliyun ecs ResizeDisk --DiskId <disk-id> --NewSize <new-size>
     # AWS
     aws ec2 modify-volume --volume-id <vol-id> --size <new-size>
     ```
  2. 扩容后扩展文件系统
     ```bash
     ssh <node-ip> "lsblk && df -h"
     # 根据文件系统类型扩展
     ssh <node-ip> "resize2fs /dev/<partition>"  # ext4
     # 或
     ssh <node-ip> "xfs_growfs /"  # xfs
     ```
- **安全检查**: 确认磁盘支持在线扩容；备份重要数据
- **回滚方案**: 云盘扩容通常不可回滚，需提前规划

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-009: 节点排空和替换（Drain + Replace）
- **适用根因**: RC-001/004/007（资源压力无法通过清理解决）
- **审批要求**: 需要值班经理或高级 SRE 审批
- **数据备份**: 确认节点上无独占本地数据（emptyDir 数据会丢失）
- **操作步骤**:
  1. 标记节点不可调度
     ```bash
     kubectl cordon <node-name>
     ```
  2. 排空节点上的 Pod
     ```bash
     kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force
     ```
  3. 从集群中删除节点（云环境通常会自动替换）
     ```bash
     kubectl delete node <node-name>
     ```
  4. 云环境：缩容后扩容或替换实例
- **回滚方案**: 重新将节点加入集群（需重新执行 kubeadm join 或等效操作）

## 验证确认

### 7.1 即时验证（修复后 1 分钟内）

```bash
# V1: 检查节点 Pressure 条件是否清除
kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}' | grep Pressure
# 预期: 所有 Pressure 条件为 False

# V2: 检查磁盘空间
ssh <node-ip> "df -h / /var/lib/kubelet /var/lib/containerd"
# 预期: 使用率低于驱逐阈值

# V3: 检查内存可用量
ssh <node-ip> "free -m | grep 'Mem:'"
# 预期: available 内存 > 驱逐阈值（默认 100Mi）

# V4: 检查 PID 使用
ssh <node-ip> "ps aux --no-heading | wc -l && cat /proc/sys/kernel/pid_max"
# 预期: 当前 PID 远低于上限
```

### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| 节点 Pressure 条件 | `kube_node_status_condition{condition="*Pressure",status="true"}` | 保持 False | 重新变为 True |
| 磁盘使用趋势 | `node_filesystem_avail_bytes` | 稳定或上升 | 持续下降 |
| 内存使用趋势 | `node_memory_MemAvailable_bytes` | 稳定或上升 | 持续下降 |
| Pod 驱逐事件 | `kube_pod_status_reason{reason="Evicted"}` 新增数 | 0 | > 0 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认故障已解决：
- [ ] 节点所有 Pressure 条件为 False
- [ ] 5 分钟内无新的 Evicted Pod
- [ ] 磁盘/内存可用量稳定在阈值之上（至少 20% 余量）
- [ ] 新 Pod 可正常调度到该节点

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| Pressure 条件复发 | `kubectl get nodes` 或 Prometheus 告警 | 每 4h | 若复发 → 排查根因未彻底修复 |
| 磁盘使用趋势 | `node_filesystem_avail_bytes` 预测 | 每 4h | 若趋势仍恶化 → 需扩容或优化应用 |
| 内存使用趋势 | `node_memory_MemAvailable_bytes` | 每 4h | 若趋势恶化 → 检查应用内存泄漏 |
| OOM Killer 触发 | `dmesg` 或 `node_vmstat_oom_kill` | 每 4h | 若触发 → 检查内存分配策略 |

## 升级协议

### 8.1 自动升级条件

| 条件 | 说明 |
|------|------|
| 诊断超时 | 诊断工作流执行超过 30 分钟未确认根因 |
| 修复失败 | 同一修复操作执行 2 次仍未通过验证 |
| 严重性升级 | 初始分级为 P2 但影响面扩大到多节点 |
| 根因不明 | 诊断完成但无法匹配任何已知根因 |
| 硬件故障 | 诊断发现磁盘/内存硬件问题 |

### 8.2 升级消息模板

```
【{severity}】{skill_name} - {cluster_name}
- 故障概述: 节点 {node_name} 出现 {pressure_type}，已驱逐 {evicted_count} 个 Pod
- 影响范围: {affected_namespaces} 命名空间受影响
- 已完成诊断: {completed_steps}
- 初步发现: {findings}
- 根因候选: {root_cause_candidates}
- 需要: {action_needed}
- 工单编号: {ticket_id}
```

### 8.3 交接信息包

升级时，Agent 需准备以下信息：
1. 完整诊断路径和每步输出
2. 已排除的根因及原因
3. 可能的根因假设及置信度
4. 节点资源使用快照（磁盘、内存、PID）
5. 最近 30 分钟的 kubelet 日志关键行
6. 被驱逐 Pod 列表及所属服务

## 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| NodeSwap | alpha | alpha | beta | beta | beta |
| GracefulNodeShutdown | GA | GA | GA | GA | GA |
| 原地 Pod 资源调整 | - | - | alpha | beta | beta |
| 回收站（Storage Protection） | GA | GA | GA | GA | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| kubelet 日志位置 | journalctl | journalctl | journalctl | journalctl | journalctl |
| swap 检查 | `free -m`（swap 视为不可用） | `free -m` | `free -m` + kubelet swapBehavior 配置 | 同 v1.30 | 同 v1.30 |

### 9.3 关键驱逐阈值默认值

| 阈值 | 默认值 | 说明 |
|------|--------|------|
| memory.available | < 100Mi | 硬驱逐阈值 |
| nodefs.available | < 10% | 节点文件系统可用空间 |
| imagefs.available | < 15% | 镜像文件系统可用空间 |
| nodefs.inodesFree | < 5% | 节点文件系统 inode |
| pid.available | < 无默认值 | 需手动配置 |

## 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 将 MemoryPressure 误判为应用问题 | Pod OOMKilled | 节点内存不足，kubelet 应先驱逐 | 检查节点条件，区分节点级和应用级 OOM |
| 将 DiskPressure 误判为应用磁盘问题 | PVC 满 | 节点磁盘满导致镜像无法拉取 | 检查节点磁盘 vs PVC 磁盘 |
| 忽略 inode 耗尽 | DiskPressure 但 df -h 显示有空间 | inode 耗尽，无法创建新文件 | 始终检查 `df -i` |
| 频繁触发驱逐但阈值正常 | 持续 Evicted Pod | 应用内存泄漏导致实际内存耗尽 | 检查 Pod 内存使用趋势 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：
- kubelet 驱逐机制 → `domain-01-cluster-fundamentals/33-kubelet-eviction-thresholds.md`
- OOM 内存诊断 → `domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis.md`
- 节点全面排查 → `domain-10-troubleshooting-diagnostics/09-node-comprehensive-troubleshooting.md`
- 性能瓶颈诊断 → `domain-10-troubleshooting-diagnostics/topic-skills/17-performance-bottleneck.md`

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-05 | v1.0 | 初始版本，覆盖 MemoryPressure/DiskPressure/PIDPressure | 补齐 SKILL-NODE-001 未覆盖的场景 |

## 云厂商特异性

### 11.1 ACK (Alibaba Cloud)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| 云盘扩容 | `aliyun ecs ResizeDisk --DiskId <id> --NewSize <size>` | 扩容后需登录节点扩展文件系统 |
| 节点自动修复 | ACK 托管节点池支持自动节点替换 | 检查节点池配置 |
| 日志收集 | 默认安装 logtail | 清理日志前确认 logtail 状态 |

### 11.2 EKS (Amazon Web Services)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| EBS 扩容 | `aws ec2 modify-volume --volume-id <id> --size <size>` | gp3 支持在线扩容 |
| 节点替换 | Managed Node Group 自动替换不健康节点 | 可结合 drain 使用 |
| Bottlerocket OS | 使用不同文件系统布局 | 日志和容器存储路径不同 |

### 11.3 GKE (Google Kubernetes Engine)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| 节点自动修复 | GKE 自动修复节点 | 短暂压力可能被自动修复 |
| 磁盘类型 | 默认使用 Container-Optimized OS | 容器存储路径不同 |
| 日志 | 默认使用 Cloud Logging | 本地日志较少 |

### 11.4 AKS (Azure Kubernetes Service)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| 磁盘扩容 | `az vmss disk attach/expand` | 需通过 VMSS 操作 |
| 节点 OS | Ubuntu 或 Windows | 文件系统工具差异 |

## 自动化集成接口

### 12.1 脚本入口

```bash
# Phase 1: 快速诊断
./scripts/diagnose-node-pressure-quick.sh --node <NODE_NAME>

# Phase 2: 深度诊断
./scripts/diagnose-node-pressure-deep.sh --node <NODE_NAME> --ssh

# 验证
./scripts/verify-node-pressure.sh --node <NODE_NAME>
```

### 12.2 Webhook 回调

```yaml
# AlertManager 示例
receivers:
- name: skill-node-pressure-trigger
  webhook_configs:
  - url: 'http://agent-gateway/skill/SKILL-NODE-002'
    send_resolved: true
```

### 12.3 输出 JSON Schema

```json
{
  "skill_id": "SKILL-NODE-002",
  "node_name": "node-1",
  "findings": [
    { "step": "D1.1", "result": "DiskPressure=True", "severity": "critical" },
    { "step": "D2.2", "result": "/var/lib/containerd 92%", "severity": "high" }
  ],
  "root_cause_candidates": [
    { "rc_id": "RC-006", "confidence": 0.90, "evidence": ["D2.2", "D2.3", "D3.1"] }
  ],
  "recommended_action": {
    "rem_id": "REM-002",
    "risk_level": "low",
    "command": "crictl images prune",
    "rollback": "N/A"
  }
}
```

---

*文档版本: 1.0*  
*Skill ID: SKILL-NODE-002*  
*创建时间: 2026-05*  
*维护者: Kudig Team*
