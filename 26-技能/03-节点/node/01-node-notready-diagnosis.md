---
title: Node NotReady 诊断与修复
description: 针对节点 NotReady/Unknown 状态的完整诊断技能，覆盖 kubelet 崩溃、容器运行时异常、网络分区、资源压力、证书过期等 15 种根因的分阶段诊断与修复
summary: Node NotReady 是 Kubernetes 集群中爆炸半径最大的问题类型之一，本技能提供从快速分级到根因确认的完整诊断路径
category: skill
tags:
- k8s
- node
- troubleshooting
- notready
- kubelet
- containerd
- network
- certificate
- sop
- runbook
sources:
- 故障诊断/技能体系/01-node-notready.md
- 故障诊断/FTA故障树/list/node-fta.md
- 故障诊断/核心排障/06-node-notready-diagnosis.md
- 故障诊断/技能体系/skill-set/k8s-node-notready/SKILL-DEEP-DIVE.md
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 30min
intent_queries:
- Node NotReady 怎么排查
- 节点不可用如何诊断
- kubelet 挂了怎么办
- 节点状态 Unknown 什么原因
- 多节点同时 NotReady 怎么处理
trigger_keywords:
- NotReady
- NodeNotReady
- 节点不可用
- 节点异常
- kubelet stopped
- node unreachable
- 节点不可达
- NodeStatusUnknown
- 节点失联
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- node-architecture
skill_id: SKILL-NODE-001
skill_name: Node NotReady 诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L2-semi-auto
fta_path: TE-1 -> IE-1.1~IE-1.6
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Node NotReady 诊断与修复

> **Skill ID**: SKILL-NODE-001
> **Agent 执行模式**: L2-semi-auto — 低风险操作自动执行，中/高风险需人工审批
> **预计修复时间**: 5-30 分钟
> **FTA 路径**: TE-1 → IE-1.1~IE-1.6

---

## 1. 概述

Node NotReady 是 Kubernetes 集群中**爆炸半径最大**的问题类型之一。当节点进入 NotReady 状态时，控制平面（kube-controller-manager 的 node-lifecycle-controller）将在 `pod-eviction-timeout`（默认 5 分钟）后开始驱逐该节点上的所有非 DaemonSet Pod，导致大规模服务中断。对于 control plane 节点，NotReady 可能直接威胁集群可用性。

> **版本差异说明**:
> - `pod-eviction-timeout` 默认 5 分钟，自 v1.28+ 可通过 `--node-monitor-grace-period` 调整
> - v1.29+ 引入 **PodDisruptionConditions** (GA)，驱逐的 Pod 会记录 `DisruptionTarget` 原因
> - v1.28+ **GracefulNodeShutdown** (GA) 使节点在计划关机时可优雅驱逐 Pod
> - v1.31+ **EventedPLEG** (GA) 替代 GenericPLEG，诊断方式有所不同

### 典型触发场景

1. **kubelet 异常**: 进程崩溃、OOM、配置错误，无法上报心跳
2. **容器运行时问题**: containerd/CRI-O 异常，PLEG 不健康
3. **网络分区**: 节点与 apiserver 网络不通
4. **资源压力**: 磁盘/内存/PID 耗尽触发驱逐管理器
5. **证书过期**: kubelet 客户端证书过期，TLS 连接失败

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述（错误消息/事件原文） | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | `kubectl get nodes` 显示 `NotReady`，Condition `Ready=False/Unknown` | STATUS 列 + `kubectl describe node` Ready Condition 的 Reason（如 `KubeletNotReady`/`NodeStatusUnknown`） | 0.95 | `SchedulingDisabled` 但 Ready 属人工 cordon |
| S2 | 节点状态在 Ready/NotReady 间频繁切换 | Events 交替出现 `NodeReady`/`NodeNotReady` | 0.85 | 单次抖动已恢复属 P3 观察项 |
| S3 | 节点上 Pod 被大量驱逐，Events `Evicted: The node was low on resource` | `kubectl get events --field-selector reason=Evicted` | 0.80 | 手动 drain 导致的驱逐属运维操作 |
| S4 | Conditions `DiskPressure`/`MemoryPressure`/`PIDPressure` 为 `True` | `kubectl describe node` Conditions（Events `NodeHasDiskPressure`/`NodeHasInsufficientMemory`） | 0.90 | 压力未致 NotReady → 转 [02-node-resource-pressure.md](02-node-resource-pressure.md) |
| S5 | kubelet 日志出现 `connection refused` / `Unable to register node` / `use of closed network connection` | `journalctl -u kubelet` grep 关键字 | 0.75 | apiserver 整体不可用 → 转 [[26-技能/01-集群运维/cluster/01-apiserver-controlplane.md|控制面诊断]] |
| S6 | Prometheus 告警 `KubeNodeNotReady` 触发 | `kube_node_status_condition{condition="Ready",status="false"}` | 0.95 | 确认非监控采集链路自身故障 |
| S7 | Node Lease 长时间未更新（`Ready=Unknown`，Reason `NodeStatusUnknown`） | `kubectl get lease -n kube-node-lease <node>` renewTime | 0.90 | 控制面与节点间网络分区时优先排网络 |
| S8 | kubelet 日志 `x509: certificate has expired or is not yet valid` | `journalctl -u kubelet` grep x509 | 0.90 | 节点时钟偏移也报 x509，先校验 NTP → 证书转 [[26-技能/01-集群运维/cluster/03-cluster-cert-upgrade.md|证书诊断]] |

### 2.2 排除标准

| 排除条件 | 正确路由 |
|---------|----------|
| 节点 Ready，但 Pod CrashLoopBackOff | SKILL-POD-001 |
| 节点 Ready，但 Pod Pending | SKILL-POD-002 |
| 节点 SchedulingDisabled 但 Ready | 人工操作，非问题 |
| 新集群所有节点从未 Ready | 集群初始化问题 |

### 2.3 常见错误消息与事件日志速查

> 以下错误消息和事件日志是 Node NotReady 场景的高频诊断线索。Agent 在 Phase 1 采集节点 Conditions 和 Events 后，可直接匹配本表快速路由到对应根因。

#### 节点 Conditions Message（`kubectl describe node` → Conditions）

| Condition | Message 模式 | 含义 | 检测命令 | 路由 |
|-----------|-------------|------|---------|------|
| `Ready=False` | `KubeletNotReady: container runtime is down` | 容器运行时崩溃/未运行 | `kubectl describe node <node> \| grep -A3 "Ready"` | → RC-002 |
| `Ready=False` | `KubeletNotReady: PLEG is not healthy: pleg was last seen active Xm ago` | PLEG 不健康（运行时响应慢） | 同上 | → RC-008 |
| `Ready=False` | `KubeletNotReady: runtime network plugin is not ready: CNI plugin not initialized` | CNI 插件未初始化 | 同上 | → 转 CNI 排查 |
| `Ready=Unknown` | `NodeStatusUnknown: Kubelet stopped posting node status` | kubelet 停止上报（崩溃/网络分区） | 同上 | → RC-001/RC-006 |
| `MemoryPressure=True` | `kubelet has insufficient memory available` | 节点内存压力 | `kubectl describe node <node> \| grep -A2 MemoryPressure` | → RC-004 |
| `DiskPressure=True` | `kubelet has disk pressure: imagefs/ nodefs is using X%` | 节点磁盘压力 | `kubectl describe node <node> \| grep -A2 DiskPressure` | → RC-003 |
| `PIDPressure=True` | `kubelet has PID pressure` | 节点 PID 耗尽 | `kubectl describe node <node> \| grep -A2 PIDPressure` | → RC-005 |
| `Ready=False` | `KubeletNotReady: failed to get node info: ...` | kubelet 内部错误 | 同上 | → RC-001 |

#### 节点 Events（`kubectl get events --field-selector involvedObject.kind=Node`）

| 事件 Reason | 事件 Message 模式 | 含义 | 检测命令 | 路由 |
|-------------|------------------|------|---------|------|
| `NodeNotReady` | `Node <node> status is now: NodeNotReady` | 节点进入 NotReady 状态 | `kubectl get events --field-selector reason=NodeNotReady,involvedObject.name=<node>` | 确认时间点 |
| `NodeReady` | `Node <node> status is now: NodeReady` | 节点恢复 Ready（用于确认恢复时间） | `kubectl get events --field-selector reason=NodeReady` | 辅助判断抱死/恢复循环 |
| `NodeHasDiskPressure` | `Node <node> status is now: NodeHasDiskPressure` | 磁盘压力触发 | `kubectl get events --field-selector reason=NodeHasDiskPressure` | → RC-003 |
| `NodeHasMemoryPressure` | `Node <node> status is now: NodeHasMemoryPressure` | 内存压力触发 | `kubectl get events --field-selector reason=NodeHasMemoryPressure` | → RC-004 |
| `NodeHasPIDPressure` | `Node <node> status is now: NodeHasPIDPressure` | PID 压力触发 | `kubectl get events --field-selector reason=NodeHasPIDPressure` | → RC-005 |
| `Evicted` | `The node was low on resource: [memory\|ephemeral-storage]. Container <name> was using <amount>.` | 资源不足触发 Pod 驱逐 | `kubectl get events -A --field-selector reason=Evicted` | → RC-003/RC-004 |
| `EvictionThresholdMet` | `Attempting to reclaim ephemeral-storage` / `Attempting to reclaim memory` | kubelet 驱逐管理器触发 | `kubectl get events --field-selector reason=EvictionThresholdMet` | → RC-003/RC-004 |
| `SystemOOM` | `System OOM encountered, victim process: <process>, pid: <pid>` | 节点级 OOM（非 cgroup） | `kubectl get events --field-selector reason=SystemOOM` | → RC-004 |
| `Rebooted` | `Node <node> has been rebooted, boot duration: <duration>` | 节点重启 | `kubectl get events --field-selector reason=Rebooted` | → 检查重启原因 |
| `Starting` | `Starting kubelet.` | kubelet 启动 | `kubectl get events --field-selector reason=Starting` | 确认 kubelet 最近启动时间 |

#### kubelet 日志关键错误模式（`journalctl -u kubelet`）

```bash
# 🟢 低风险：只读/信息收集（需节点 SSH 权限）
ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager -n 200 | grep -iE 'error|failed|refused|expired|timeout|oom|pleg'"
```

| 日志模式 | 含义 | 对应根因 | 修复方向 |
|---------|------|---------|----------|
| `Failed to connect to apiserver: dial tcp <ip>:6443: connect: connection refused` | API Server 不可达 | RC-006 网络 | 检查网络/apiserver 状态 |
| `Failed to connect to apiserver: ... i/o timeout` | API Server 连接超时 | RC-006 网络 | 检查防火墙/路由 |
| `certificate has expired` / `x509: certificate has expired or is not yet valid` | kubelet 客户端证书过期 | RC-007 证书 | 轮换证书 |
| `certificate signed by unknown authority` | CA 证书不匹配 | RC-007 证书 | 检查 CA 配置 |
| `PLEG is not healthy: pleg was last seen active Xm ago; threshold is 3m0s` | PLEG 超时（运行时卡死） | RC-008 PLEG | 检查 containerd/磁盘 IO |
| `Container runtime is not running` / `Failed to connect to container runtime socket` | 容器运行时崩溃 | RC-002 运行时 | 重启 containerd |
| `no space left on device` | 磁盘空间耗尽 | RC-003 磁盘 | 清理/扩容 |
| `failed to run Kubelet: unable to load bootstrap kubeconfig` | kubelet 配置错误 | RC-001 kubelet | 检查配置文件 |
| `Failed to create pod sandbox: ... network plugin is not ready` | CNI 未就绪 | CNI 问题 | 转 CNI 排查 |
| `eviction manager: attempting to reclaim memory` | 内存驱逐触发 | RC-004 内存 | 检查内存分配 |
| `eviction manager: attempting to reclaim ephemeral-storage` | 磁盘驱逐触发 | RC-003 磁盘 | 清理磁盘 |
| `Failed to update node status: ... context deadline exceeded` | 心跳上报失败 | RC-006 网络 | 检查网络连通性 |
| `node lease update failed: ...` | Lease 续租失败 | RC-001/RC-006 | 检查 kubelet/apiserver |

#### 容器运行时日志（containerd）

```bash
# 🟢 低风险：只读/信息收集（需节点 SSH 权限）
ssh <node-ip> "journalctl -u containerd --since '30 minutes ago' --no-pager -n 100 | grep -iE 'error|failed|timeout|panic'"
```

| 日志模式 | 含义 | 修复 |
|---------|------|------|
| `level=error msg="failed to create shim task"` | 容器创建失败 | 检查 runc/内核状态 |
| `level=error msg="failed to reserve sandbox name"` | 沙箱名称冲突 | 重启 containerd |
| `level=warning msg="could not create snapshotter"` | 快照器异常 | 检查磁盘/文件系统 |
| `panic: runtime error` | containerd 崩溃 | 立即重启 containerd（🔴 高风险） |
| `failed to dial "/run/containerd/containerd.sock": connection refused` | containerd socket 不可用 | 检查 containerd 服务状态 |

---

## 3. 快速分级（2 分钟内完成）

```bash
# 🟢 低风险：只读/信息收集
# Step T1: 统计 NotReady 节点
kubectl get nodes --no-headers | awk '{print $2}' | sort | uniq -c

# Step T2: 确认是否控制平面节点
kubectl get nodes --no-headers | grep "NotReady" | grep -E "control-plane|master"

# Step T3: 评估工作负载影响
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node> --no-headers | wc -l

# Step T4: 检查 NotReady 持续时间
kubectl get nodes -o custom-columns=NAME:.metadata.name,STATUS:.status.conditions[-1].type,LAST_TRANSITION:.status.conditions[-1].lastTransitionTime
```

### 严重性分级

| 条件 | 级别 | SLA 要求 |
|------|------|---------|
| >30% 节点 NotReady **或** 控制平面节点 NotReady | **P0** | 立即响应，15min 内确认根因 |
| 多个工作节点 NotReady（2-30%） | **P1** | 15min 内响应，30min 内修复 |
| 单个工作节点 NotReady | **P2** | 30min 内响应，2h 内修复 |
| 新节点从未 Ready / 未承载业务 | **P3** | 4h 内处理 |

### 立即升级条件

- >50% 节点 NotReady
- 所有 control-plane 节点 NotReady（etcd 可能丢失 quorum）
- `kubectl get nodes` 本身超时（apiserver 不可达）
- NotReady 节点数 5 分钟内持续增加
- 伴随安全告警

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集节点状态信息，无需 SSH
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取节点全局状态
```bash
kubectl get nodes -o wide
```
- STATUS 为 `NotReady` → 继续 D1.2
- STATUS 为 `Ready,SchedulingDisabled` → 可能被 cordon（RC-012）
- 命令超时 → apiserver 不可用，立即升级

**Step D1.2**: 获取节点 Conditions
```bash
kubectl describe node <node-name>
```
- `Ready=False` + `KubeletNotReady` → kubelet 问题（RC-001）
- `Ready=Unknown` → 网络问题（RC-006）或 kubelet 停止
- `MemoryPressure=True` → RC-004
- `DiskPressure=True` → RC-003
- Message 含 `container runtime is down` → RC-002
- Message 含 `PLEG is not healthy` → RC-008
- Message 含 `certificate`/`x509` → RC-007

**Step D1.3**: 检查节点事件
```bash
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
  --sort-by=.lastTimestamp --no-headers | tail -30
```

**Step D1.4**: 检查 Taints
```bash
kubectl get node <node-name> -o jsonpath='{range .spec.taints[*]}{.key}={.value}:{.effect}{"\n"}{end}'
```

**Step D1.5**: 检查 Lease 对象
```bash
kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
```
- renewTime 距今 > 40s → kubelet 未续租

---

### Phase 2: 深度检查（只读，需 SSH）

> **目标**: SSH 登录问题节点，检查系统级组件状态
> **预计耗时**: 5-10 分钟

**Step D2.1**: 检查 kubelet 服务
```bash
ssh <node-ip> "systemctl status kubelet"
```

**Step D2.2**: 检查 kubelet 日志
```bash
ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager -n 200"
```
- `connection refused` → RC-006（网络）
- `x509: certificate has expired` → RC-007（证书）
- `PLEG is not healthy` → RC-008
- `container runtime is not running` → RC-002
- `no space left on device` → RC-003

**Step D2.3**: 检查 containerd
```bash
ssh <node-ip> "systemctl status containerd"
```

**Step D2.4**: 检查系统资源
```bash
ssh <node-ip> "df -h / /var/lib/kubelet /var/lib/containerd /var/log"
ssh <node-ip> "free -m"
ssh <node-ip> "ps aux --no-heading | wc -l && cat /proc/sys/kernel/pid_max"
```

**Step D2.5**: 检查网络连通性
```bash
ssh <node-ip> "nc -zv <apiserver-ip> 6443 -w 5"
ssh <node-ip> "curl -sk --max-time 5 https://<apiserver-ip>:6443/healthz"
```

**Step D2.6**: 检查证书有效期
```bash
ssh <node-ip> "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates"
```

**Step D2.7**: 检查内核日志
```bash
ssh <node-ip> "dmesg -T | tail -50"
```
- `Out of memory: Killed process` → RC-004
- `Hardware Error`/`MCE` → RC-009
- `nf_conntrack: table full` → RC-006 变种

**Step D2.8**: 检查 NTP 时间同步
```bash
ssh <node-ip> "timedatectl status"
```

---

### Phase 3: 批量 NotReady 级联分析

> **触发条件**: >2 个节点在 5 分钟内同时 NotReady

**Step D4.1**: 关联性分析
```bash
kubectl get nodes --no-headers | grep "NotReady" | awk '{print $1}' | while read node; do
  echo "=== Node: $node ==="
  kubectl get node $node -o jsonpath='IP={.status.addresses[?(@.type=="InternalIP")].address} Zone={.metadata.labels.topology\.kubernetes\.io/zone}{"\n"}'
done
```
- 同一时间戳 → 网络/控制平面问题
- 同一 Zone/Rack → 物理网络设备问题
- 同一网段 → VLAN/子网问题

---

## 5. 根因分类（15 种）

| RC ID | 根因 | 概率 | 诊断证据 |
|-------|------|------|---------|
| RC-001 | kubelet 进程崩溃/未运行 | 高 | D2.1 inactive/failed |
| RC-002 | containerd 异常 | 高 | D2.3 未运行 |
| RC-003 | 磁盘空间耗尽 (DiskPressure) | 高 | D2.4 使用率>85% |
| RC-004 | 内存耗尽 (MemoryPressure) | 中 | D2.4 可用内存<100Mi |
| RC-005 | PID 耗尽 (PIDPressure) | 中 | D2.4 PID 接近上限 |
| RC-006 | 节点-apiserver 网络不通 | 中 | D2.5 TCP 连接失败 |
| RC-007 | kubelet 证书过期 | 中 | D2.6 证书已过期 |
| RC-008 | PLEG 不健康 | 中 | D2.2 PLEG 日志 |
| RC-009 | 内核/硬件异常 | 低 | D2.7 MCE/I/O error |
| RC-010 | NTP 时间不同步 | 低 | D2.8 偏差>5s |
| RC-011 | CNI 插件异常 | 中 | CNI 配置/Pod 缺失 |
| RC-012 | 手动 cordon | 低 | D1.4 unschedulable |
| RC-013 | 内核 panic | ~5% | D2.7 kernel panic |
| RC-014 | 云厂商节点池异常 | ~8% | 云平台实例状态异常 |
| RC-015 | 证书自动轮转失败 | ~4% | CSR Pending |

---

## 6. 修复操作

### 6.1 🟢 低风险

#### REM-001: Uncordon 节点
```bash
kubectl uncordon <node-name>
# 验证: kubectl get node <node-name>
```

#### REM-002: 清理磁盘空间
```bash
ssh <node-ip> "crictl rmi --prune"
ssh <node-ip> "find /var/log -name '*.gz' -mtime +7 -delete"
ssh <node-ip> "journalctl --vacuum-time=2d"
```

### 6.2 🟡 中风险

#### REM-003: 重启 kubelet
```bash
ssh <node-ip> "systemctl restart kubelet"
# 等待 30s 后验证
kubectl get node <node-name>
```

#### REM-004: 重启 containerd
```bash
ssh <node-ip> "systemctl restart containerd && sleep 10 && systemctl restart kubelet"
# 等待 60s 后验证
kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
```

#### REM-005: 调整驱逐阈值
```bash
ssh <node-ip> "cp /var/lib/kubelet/config.yaml /var/lib/kubelet/config.yaml.bak"
# 修改阈值后重启 kubelet
ssh <node-ip> "systemctl restart kubelet"
```

### 6.3 🔴 高风险

#### REM-006: 排空节点并重启
```bash
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force --grace-period=60
ssh <node-ip> "reboot"
# 等待 2-5 分钟后
kubectl uncordon <node-name>
```

#### REM-007: 替换节点（云环境）
```bash
kubectl drain <node-name> --ignore-daemonsets --force
kubectl delete node <node-name>
# 云平台终止实例并创建新实例
```

#### REM-008: 手动证书轮转
```bash
kubectl get csr | grep -i pending
kubectl certificate approve <csr-name>
# 或重新 bootstrap
ssh <node-ip> "rm -f /var/lib/kubelet/pki/kubelet-client-current.pem"
ssh <node-ip> "systemctl restart kubelet"
kubectl certificate approve <new-csr-name>
```

---

## 7. 验证确认

### 解决确认标准

- [ ] 节点 STATUS 显示 Ready，持续超过 5 分钟
- [ ] 所有 Conditions（MemoryPressure, DiskPressure, PIDPressure）均为 False
- [ ] Node Lease 正常续租（renewTime 持续更新）
- [ ] 节点上 Pod 已恢复正常运行
- [ ] kubelet 和 containerd 稳定运行（无崩溃重启）
- [ ] 无新增 Warning 事件

### 验证命令

```bash
# 🟢 低风险
kubectl get node <node-name>
kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
```

---

## 8. 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 网络抖动误判为 kubelet 崩溃 | Ready=Unknown | 网络链路不稳定 | 先 SSH 确认 kubelet 进程状态 |
| DiskPressure 归因于镜像过多 | 磁盘使用率高 | 容器日志未轮转 | 检查 `/var/log/pods/` 大文件 |
| PLEG 不健康误判为运行时问题 | PLEG 日志 | D 状态进程阻塞 CRI | 检查 `ps aux | awk '$8=="D"'` |
| 证书过期误判为网络问题 | connection refused | TLS 握手失败 | 先检查证书有效期 |
| cordon 误判为节点问题 | Pod 无法调度 | 手动 cordon 未记录 | 区分 NotReady 和 SchedulingDisabled |
| 时间偏差导致间歇性问题 | 状态时好时坏 | NTP 未同步 | 早期检查时间同步 |

---

## 9. 版本兼容性注意事项

> 详细版本差异请参考 [reference/node-version-differences.md](reference/node-version-differences.md)

| 版本 | 关键差异 | 诊断影响 |
|------|---------|----------|
| 1.18~1.32 | NodeMonitorGracePeriod 默认 40s | 节点失联后 40s 标记 NotReady |
| 1.34+ | NodeMonitorGracePeriod 默认 **50s** | 节点失联后 50s 标记 NotReady，告警阈值需调整 |
| 1.18 以前 | 使用 podEvictionTimeout 直接驱逐 | 检查 `--pod-eviction-timeout` 参数 |
| 1.18+ | Taint-based 驱逐（GA） | 检查节点 Taint 和 Pod tolerations |
| 1.32+ | TaintEviction 独立控制器 | 驱逐日志需单独查看 |
| 1.20 以前 | 无 GracefulNodeShutdown | 节点重启后 Pod 直接被 kill |
| 1.21+ | GracefulNodeShutdown Beta | 节点关机时 Pod 优雅终止 |

**版本特定诊断命令**：

```bash
# 🟢 检查节点 Taint（1.18+ 驱逐机制核心）
kubectl get node <node-name> -o jsonpath='{.spec.taints}' | jq .

# 🟢 查看节点声明的特性（1.36+）
kubectl get node <node-name> -o jsonpath='{.status.declaredFeatures}' | jq .
```

[存疑：此处关于 1.34 版本 NodeMonitorGracePeriod 从 40s 变更为 50s 的精确起始版本可能存在不准确之处，代码中 1.32 仍为 40s、1.34 已为 50s，但 1.33 版本未在 code/ 目录中，需进一步核实]

---

## 10. 阿里云/专有云特有场景

> 来源：`故障诊断/技能体系/skill-set/k8s-node-notready/SKILL-DEEP-DIVE.md` §2

| 场景 | 症状 | 根因 | 排查要点 |
|------|------|------|----------|
| ECS 系统事件 | 节点突然 NotReady/消失 | 底层硬件维护计划重启/实例停用 | ACK 控制台 > 节点事件；云监控事件中心 |
| 云盘 IO Hang | SSH 卡顿、crictl 超时、D 状态进程 | ESSD 云盘 IO 挂起 | `dmesg` 查 `task blocked for more than 120s` |
| Terway ENI IP 耗尽 | Pod 无法分配 IP、网络异常 | 交换机可用 IP 耗尽 | 检查交换机网段剩余 IP、Terway Pod 日志 |
| 安全组变更 | 部分端口通部分不通 | 6443/10250/8472 等端口被误删 | 核对安全组规则 |
| 专有云底座异常 | 节点 IO 失败、网络抱死 | 飞天/盘古/洛神网络抖动 | 收集底座告警+BMC日志，需平台侧介入 |

---

## 11. 边界条件与禁忌操作

> 来源：`故障诊断/技能体系/skill-set/k8s-node-notready/SKILL-DEEP-DIVE.md` §3/§5

### 11.1 边界条件

| 场景 | 处理策略 |
|------|----------|
| >50% 节点同时 NotReady | 立即升级，优先查 API Server/etcd/控制平面网络 |
| 控制平面节点 NotReady | 优先保证 etcd quorum，避免多节点同时操作 |
| kubectl 本身超时 | 用 `--request-timeout=5s` 测试，先排查控制平面而非工作节点 |

### 11.2 禁忌操作

| 禁忌 | 原因 |
|------|------|
| 未 drain 直接重启控制平面节点 | 可能破坏 etcd quorum |
| 批量重启所有节点 kubelet | 若根因是 API Server/网络，批量重启加剧抖动 |
| 直接删除 `/var/lib/kubelet` | 丢失 Pod 状态、卷挂载信息，导致有状态服务数据不一致 |
| 随意修改 `--eviction-hard` 阈值 | 只是隐藏告警，不解决资源不足 |
| 看到 PLEG 不健康只重启 kubelet | 不排查 containerd 和 IO，问题会反复 |
| 把 TLS 证书失败当成网络不通 | TCP 通但 HTTPS 失败时应先查证书和时间同步 |

### 11.3 推荐诊断顺序

1. 先确认影响范围（单节点 / 多节点 / 控制平面）
2. 确认是 Ready=False 还是 Ready=Unknown 还是 SchedulingDisabled
3. 查看 Lease 是否更新，区分 kubelet 侧问题和控制平面侧问题
4. SSH 到节点后按 kubelet → containerd → 资源 → 网络 → 证书 → 内核/硬件 顺序排查
5. 在阿里云/专有云环境中，同步查看云平台事件和底座告警

---

## 12. 证据三元组（诊断结论必须可溯源）

每个 NotReady 根因结论必须同时具备 Metrics + Logs/Events 证据，时间窗对齐故障时刻 ±5 分钟：

```promql
# 🟢 节点 NotReady 判据
kube_node_status_condition{condition="Ready",status="true"} == 0

# 🟢 节点内存压力判据（对应资源压力型 NotReady）
(1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.90

# 🟢 kubelet Lease 续租超时判据（对应 kubelet 崩溃/网络分区）
time() - kube_node_status_condition{condition="Ready"} > 40
```

| 证据维度 | 采集来源 | NotReady 场景取值 |
|---------|---------|------------------|
| Metrics | Prometheus / node_exporter | Ready condition=0；内存/磁盘/PID 水位 |
| Logs | `journalctl -u kubelet` | kubelet panic / cert expired / PLEG 超时 |
| Events | `kubectl get events` | `NodeNotReady` / `KubeletNotReady` / Lease 停止更新 |

---

## 相关链接

- [[26-技能/03-节点/node/README.md|Node 异常诊断技能集]]
- [[26-技能/03-节点/node/02-node-resource-pressure.md|节点资源压力诊断]]
- [[26-技能/03-节点/node/04-node-sop-runbook.md|Node SOP 与 Runbook]]
- [[26-技能/03-节点/node/05-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查]]
- [[26-技能/03-节点/node/reference/node-version-differences.md|版本差异对比]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[19-故障诊断/06-FTA故障树/list/node-fta.md|Node 故障树分析]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/SKILL-DEEP-DIVE.md|Node NotReady 深度解析（原始文件）]]
