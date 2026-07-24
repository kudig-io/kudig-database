---
title: Node Conditions 完整参考与诊断映射
description: Kubernetes Node Conditions（Ready、MemoryPressure、DiskPressure、PIDPressure、NetworkUnavailable）的完整参考手册，包含每个 Condition 的含义、触发条件、诊断命令和修复方向
summary: Node Conditions 是节点健康状态的核心指标，本参考覆盖所有 Condition 类型及其在故障诊断中的映射关系
category: reference
tags:
- k8s
- node
- conditions
- reference
- troubleshooting
- kubelet
- diagnostics
sources:
- 故障诊断/技能体系/01-node-notready.md
- 故障诊断/资源排障/09-node-comprehensive-troubleshooting.md
- 故障诊断/FTA故障树/list/node-fta.md
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: supporting
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
- 所有工程师
estimated_read_time: 10min
intent_queries:
- Node Conditions 是什么意思
- 节点状态条件怎么看
- MemoryPressure True 代表什么
- 如何判断节点健康状态
trigger_keywords:
- Node Conditions
- Ready
- MemoryPressure
- DiskPressure
- PIDPressure
- NetworkUnavailable
- 节点状态
prerequisites:
- kubectl-basics
- node-architecture
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Node Conditions 完整参考与诊断映射

---

## 1. Conditions 概览

Node Conditions 是 kubelet 上报的节点健康状态指标，存储在 `node.status.conditions` 中。

### 查看命令

```bash
# 🟢 低风险
kubectl get nodes
kubectl describe node <node-name> | grep -A 20 "Conditions:"
kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status} reason={.reason} message={.message}{"\n"}{end}'
```

---

## 2. Condition 类型详解

### 2.1 Ready

| 值 | 含义 | 常见原因 |
|---|------|---------|
| `True` | kubelet 健康，可接收 Pod | 正常状态 |
| `False` | kubelet 异常，节点不可用 | kubelet 崩溃/运行时问题/资源压力 |
| `Unknown` | apiserver 长时间未收到心跳 | 网络分区/kubelet 停止 |

**Reason 字段映射**：

| Reason | 含义 | 诊断方向 |
|--------|------|---------|
| `KubeletReady` | kubelet 正常 | 无需处理 |
| `KubeletNotReady` | kubelet 报告自身不健康 | 检查 kubelet 日志 |
| `NodeStatusNeverUpdated` | 从未收到状态更新 | 新节点/网络问题 |

**诊断命令**：
```bash
# 检查 Ready 状态及最后转换时间
kubectl get node <node> -o jsonpath='{.status.conditions[?(@.type=="Ready")]}'

# 检查 Lease 续租
kubectl get lease -n kube-node-lease <node> -o jsonpath='{.spec.renewTime}'
```

---

### 2.2 MemoryPressure

| 值 | 含义 |
|---|------|
| `True` | 节点可用内存低于驱逐阈值 |
| `False` | 内存充足 |

**触发条件**：`memory.available < evictionHard.memory.available`（默认 100Mi）

**Reason 字段**：
- `KubeletHasSufficientMemory` → False（正常）
- `KubeletHasInsufficientMemory` → True（内存不足）

**诊断命令**：
```bash
# 节点内存使用
kubectl top node <node>
ssh <node-ip> "free -m"

# 检查 OOM 事件
ssh <node-ip> "dmesg -T | grep -i 'oom\|killed process' | tail -10"

# 检查 Pod 内存使用 Top
kubectl top pods -A --sort-by=memory | head -10
```

---

### 2.3 DiskPressure

| 值 | 含义 |
|---|------|
| `True` | 节点磁盘可用空间或 inode 低于阈值 |
| `False` | 磁盘充足 |

**触发条件**：
- `nodefs.available < 10%`（根分区）
- `imagefs.available < 15%`（镜像分区）
- `nodefs.inodesFree < 5%`（inode）

**Reason 字段**：
- `KubeletHasNoDiskPressure` → False（正常）
- `KubeletHasDiskPressure` → True（磁盘压力）

**诊断命令**：
```bash
# 磁盘使用
ssh <node-ip> "df -h / /var/lib/kubelet /var/lib/containerd /var/log"

# inode 使用
ssh <node-ip> "df -i / /var/lib/kubelet /var/lib/containerd"

# 大文件查找
ssh <node-ip> "du -sh /var/log/* /var/lib/containerd /tmp 2>/dev/null | sort -rh | head -10"
```

---

### 2.4 PIDPressure

| 值 | 含义 |
|---|------|
| `True` | 节点可用 PID 低于阈值 |
| `False` | PID 充足 |

**触发条件**：`pid.available < evictionHard.pid.available`

**Reason 字段**：
- `KubeletHasSufficientPID` → False（正常）
- `KubeletHasInsufficientPID` → True（PID 不足）

**诊断命令**：
```bash
# PID 使用
ssh <node-ip> "echo 'Current:' && ps aux --no-heading | wc -l && echo 'Max:' && cat /proc/sys/kernel/pid_max"

# 按用户统计进程数
ssh <node-ip> "ps -eo user | sort | uniq -c | sort -rn | head -10"

# 按 Pod 统计进程数（通过 cgroup）
ssh <node-ip> "find /sys/fs/cgroup/pids/kubepods -name pids.current -exec sh -c 'echo \"\$(cat {}): {}\"' \; | sort -rn | head -10"
```

---

### 2.5 NetworkUnavailable

| 值 | 含义 |
|---|------|
| `True` | 节点网络未正确配置 |
| `False` | 网络正常 |

**触发条件**：CNI 插件未配置或配置异常

**Reason 字段**：
- `RouteCreated` → False（网络已配置）
- `NoNetworkConfigured` → True（无网络配置）

**诊断命令**：
```bash
# CNI 配置
ssh <node-ip> "ls -la /etc/cni/net.d/"

# CNI Pod 状态
kubectl get pods -n kube-system -l k8s-app=calico-node --field-selector spec.nodeName=<node>
kubectl get pods -n kube-system -l k8s-app=cilium --field-selector spec.nodeName=<node>
```

---

## 3. Conditions 组合诊断矩阵

| Ready | MemoryPressure | DiskPressure | PIDPressure | 诊断方向 |
|:---:|:---:|:---:|:---:|:---|
| True | False | False | False | 正常 |
| True | True | False | False | 内存压力 → SKILL-NODE-002 |
| True | False | True | False | 磁盘压力 → SKILL-NODE-002 |
| True | False | False | True | PID 压力 → SKILL-NODE-002 |
| True | True | True | False | 内存+磁盘压力（通常关联） |
| False | False | False | False | kubelet/运行时/网络问题 → SKILL-NODE-001 |
| False | True | False | False | 内存耗尽导致 NotReady |
| False | False | True | False | 磁盘满导致 NotReady |
| Unknown | * | * | * | 网络分区/kubelet 完全停止 |

---

## 4. Taints 与 Conditions 的关联

当 Condition 为 True 时，kubelet 会自动添加对应 Taint：

| Condition | 自动 Taint | Effect |
|-----------|-----------|--------|
| NotReady | `node.kubernetes.io/not-ready` | NoSchedule → NoExecute |
| Unreachable | `node.kubernetes.io/unreachable` | NoSchedule → NoExecute |
| MemoryPressure | `node.kubernetes.io/memory-pressure` | NoSchedule |
| DiskPressure | `node.kubernetes.io/disk-pressure` | NoSchedule |
| PIDPressure | `node.kubernetes.io/pid-pressure` | NoSchedule |
| Unschedulable | `node.kubernetes.io/unschedulable` | NoSchedule |

**查看 Taints**：
```bash
kubectl get node <node> -o jsonpath='{range .spec.taints[*]}{.key}={.value}:{.effect}{"\n"}{end}'
```

---

## 5. 驱逐阈值默认值

| 资源 | 硬驱逐阈值 (evictionHard) | 含义 |
|------|--------------------------|------|
| `memory.available` | < 100Mi | 可用内存低于 100Mi |
| `nodefs.available` | < 10% | 根分区可用空间低于 10% |
| `imagefs.available` | < 15% | 镜像分区可用空间低于 15% |
| `nodefs.inodesFree` | < 5% | 根分区可用 inode 低于 5% |
| `pid.available` | < 100 | 可用 PID 低于 100 |

**查看当前配置**：
```bash
ssh <node-ip> "cat /var/lib/kubelet/config.yaml | grep -A 10 evictionHard"
```

---

## 相关链接

- [[技能/故障诊断-节点/node/README.md|Node 异常诊断技能集]]
- [[技能/故障诊断-节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]]
- [[技能/故障诊断-节点/node/02-node-resource-pressure.md|节点资源压力诊断]]
- [[技能/故障诊断-节点/node/reference/node-root-cause-catalog.md|根因目录]]
