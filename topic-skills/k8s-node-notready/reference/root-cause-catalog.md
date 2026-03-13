# 根因分类 / Root Cause Catalog

> **来源**: SKILL-NODE-001 v1.0 — 节点 NotReady 诊断与修复
> **本文档**: 提取自 Section 5（根因分类），包含全部 12 个根因的详细描述、诊断证据和交叉关联

---

## 目录

- [根因总览表](#根因总览表)
- [详细根因描述](#详细根因描述)
  - [RC-001 kubelet 进程崩溃或未运行](#rc-001-kubelet-进程崩溃或未运行)
  - [RC-002 容器运行时（containerd）异常](#rc-002-容器运行时containerd异常)
  - [RC-003 节点磁盘空间耗尽（DiskPressure）](#rc-003-节点磁盘空间耗尽diskpressure)
  - [RC-004 节点内存耗尽（MemoryPressure）](#rc-004-节点内存耗尽memorypressure)
  - [RC-005 节点 PID 耗尽（PIDPressure）](#rc-005-节点-pid-耗尽pidpressure)
  - [RC-006 节点与 apiserver 网络不通](#rc-006-节点与-apiserver-网络不通)
  - [RC-007 kubelet 客户端证书过期](#rc-007-kubelet-客户端证书过期)
  - [RC-008 PLEG 不健康导致 NotReady](#rc-008-pleg-不健康导致-notready)
  - [RC-009 内核故障/硬件异常](#rc-009-内核故障硬件异常)
  - [RC-010 NTP 时间不同步](#rc-010-ntp-时间不同步)
  - [RC-011 CNI 插件异常](#rc-011-cni-插件异常)
  - [RC-012 节点被手动 cordon/drain](#rc-012-节点被手动-cordondrain)
- [根因交叉关联图](#根因交叉关联图)

---

## 根因总览表

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | **kubelet 进程崩溃或未运行** | 高 | D2.1, D2.2, D1.5 | `evt_kubelet_down`, `evt_heartbeat_fail` |
| RC-002 | **容器运行时（containerd）异常** | 高 | D2.3, D2.4, D2.6, D1.2 | `evt_rt_down`, `evt_cri_sock`, `evt_rt_hang` |
| RC-003 | **节点磁盘空间耗尽（DiskPressure）** | 高 | D1.2, D2.5, D1.3 | `evt_disk_pressure`, `evt_image_gc_fail` |
| RC-004 | **节点内存耗尽（MemoryPressure）** | 中 | D1.2, D2.5, D2.9 | `evt_mem_pressure`, `evt_and_mem_low` |
| RC-005 | **节点 PID 耗尽（PIDPressure）** | 中 | D1.2, D2.5, D2.2 | `evt_pid_exhaust` |
| RC-006 | **节点与 apiserver 网络不通** | 中 | D2.7, D2.2, D1.2 | `evt_api_unreachable`, `evt_policy_block`, `evt_route_fail` |
| RC-007 | **kubelet 客户端证书过期** | 中 | D2.8, D2.2, D2.7 | `evt_kubelet_cert`, `evt_node_cert_expire` |
| RC-008 | **PLEG 不健康导致 NotReady** | 中 | D2.6, D1.2, D2.3 | `evt_pleg`, `evt_and_pleg_timeout`, `evt_and_pleg_overload` |
| RC-009 | **内核故障/硬件异常** | 低 | D2.9 | `evt_kernel_panic`, `evt_driver_issue` |
| RC-010 | **NTP 时间不同步** | 低 | D2.10, D2.8 | `evt_time_skew_tls` |
| RC-011 | **CNI 插件异常** | 中 | D3.2, D1.2 | `evt_cni_fail` |
| RC-012 | **节点被手动 cordon/drain** | 低 | D1.4, D1.1 | `evt_cordon`（非故障） |

---

## 详细根因描述

### RC-001: kubelet 进程崩溃或未运行

- **描述**: kubelet 进程因 panic、OOM、配置错误等原因停止运行或反复崩溃重启，无法向 apiserver 发送心跳
- **概率**: 高
- **诊断证据**:
  - **D2.1** 显示 kubelet 未运行（`Active: inactive (dead)` / `Active: failed` / `Active: activating (auto-restart)`）
  - **D2.2** 日志显示 panic/fatal 错误
  - **D1.5** Lease 未更新（renewTime 距当前时间 > 40s）
- **FTA 底层事件映射**: `node-fta.md → evt_kubelet_down`, `evt_heartbeat_fail`
- **关联修复**: REM-003（重启 kubelet）、REM-006（排空节点并重启）、REM-007（替换节点）
- **交叉关联**:
  - RC-003（磁盘压力）可导致 kubelet 无法写入日志或状态文件而崩溃
  - RC-004（内存压力）可导致 OOM Killer 杀死 kubelet 进程
  - RC-005（PID 压力）可导致 kubelet 无法 fork 子进程

---

### RC-002: 容器运行时（containerd）异常

- **描述**: containerd 或 CRI-O 守护进程停止、崩溃或响应超时，kubelet 无法执行容器操作导致 PLEG 不健康
- **概率**: 高
- **诊断证据**:
  - **D2.3** 显示 containerd 未运行（`Active: inactive (dead)` / `Active: failed`）
  - **D2.4** 日志有错误（`failed to create shim`、`context deadline exceeded` 等）
  - **D2.6** PLEG 不健康（`GenericPLEG: Unable to retrieve pods`）
  - **D1.2** Message 包含 "container runtime is down"
- **FTA 底层事件映射**: `node-fta.md → evt_rt_down`, `evt_cri_sock`, `evt_rt_hang`
- **关联修复**: REM-004（重启 containerd）、REM-006（排空节点并重启）
- **交叉关联**:
  - RC-003（磁盘压力）可导致 containerd 无法创建 shim 或写入层数据
  - RC-005（PID 压力）可导致 containerd 无法 fork shim 进程
  - RC-008（PLEG 不健康）通常是 RC-002 的下游症状

---

### RC-003: 节点磁盘空间耗尽（DiskPressure）

- **描述**: 根分区、/var/lib/kubelet、/var/lib/containerd 或 /var/log 分区磁盘使用率超过驱逐阈值（默认 85%），或 inode 耗尽
- **概率**: 高
- **诊断证据**:
  - **D1.2** DiskPressure=True
  - **D2.5** 磁盘使用率 >85% 或 inode 使用率 >90%
  - **D1.3** 事件包含 `NodeHasDiskPressure` 或 `InvalidDiskCapacity`
- **FTA 底层事件映射**: `node-fta.md → evt_disk_pressure`, `evt_image_gc_fail`
- **关联修复**: REM-002（清理磁盘空间）、REM-005（调整驱逐阈值）
- **交叉关联**:
  - RC-003 可导致 RC-001（kubelet 因无法写入而崩溃）
  - RC-003 可导致 RC-002（containerd 因无法写入镜像层/shim 数据而异常）
  - 日志轮转失败是常见的磁盘耗尽隐因，需检查 `/var/log/pods/` 和 `/var/log/containers/` 下的大文件

---

### RC-004: 节点内存耗尽（MemoryPressure）

- **描述**: 节点可用内存低于 kubelet 驱逐阈值（默认 100Mi），触发内存压力条件
- **概率**: 中
- **诊断证据**:
  - **D1.2** MemoryPressure=True
  - **D2.5** 可用内存极低（< 100Mi）
  - **D2.9** OOM Killer 日志（`Out of memory: Killed process`）
- **FTA 底层事件映射**: `node-fta.md → evt_mem_pressure`, `evt_and_mem_low`, `evt_and_mem_nolimit`
- **关联修复**: REM-005（调整驱逐阈值）、REM-006（排空节点并重启）
- **交叉关联**:
  - RC-004 可导致 RC-001（OOM Killer 杀死 kubelet 进程）
  - RC-004 可导致 RC-002（OOM Killer 杀死 containerd 进程）
  - [v1.30+] 若启用 Node swap support，MemoryPressure 计算可能包含 swap 使用量

---

### RC-005: 节点 PID 耗尽（PIDPressure）

- **描述**: 节点上进程数量接近或达到 pid_max 限制，kubelet 报告 PID 压力
- **概率**: 中
- **诊断证据**:
  - **D1.2** PIDPressure=True
  - **D2.5** PID 数量接近上限（默认 pid_max 为 32768 或 4194304）
  - **D2.2** 日志包含 PID 相关错误（`too many open files` 等）
- **FTA 底层事件映射**: `node-fta.md → evt_pid_exhaust`
- **关联修复**: REM-005（调整驱逐阈值）、REM-006（排空节点并重启）
- **交叉关联**:
  - RC-005 可导致 RC-001（kubelet 无法 fork 子进程）
  - RC-005 可导致 RC-002（containerd 无法创建 shim 进程）

---

### RC-006: 节点与 apiserver 网络不通

- **描述**: 防火墙规则变更、安全组配置、路由故障、物理网络问题导致节点无法与 apiserver 通信
- **概率**: 中
- **诊断证据**:
  - **D2.7** TCP 连接失败（`nc -zv` 超时或被拒绝）
  - **D2.2** 日志包含 `connection refused`、`dial tcp`、`use of closed network connection`
  - **D1.2** Ready=Unknown（apiserver 长时间未收到心跳）
- **FTA 底层事件映射**: `node-fta.md → evt_api_unreachable`, `evt_policy_block`, `evt_route_fail`
- **关联修复**: 需人工排查网络（防火墙、路由、交换机）
- **交叉关联**:
  - RC-009（内核 `nf_conntrack: table full`）可导致网络不通的变种表现
  - 与 RC-007 容易混淆 — TLS 握手失败可能被误判为网络问题，需先检查证书（D2.8）

---

### RC-007: kubelet 客户端证书过期

- **描述**: kubelet 用于与 apiserver 通信的客户端证书过期或被吊销，TLS 握手失败
- **概率**: 中
- **诊断证据**:
  - **D2.8** 证书已过期（`notAfter` 早于当前时间）或证书文件不存在
  - **D2.2** 日志包含 `x509: certificate has expired` 或 `certificate signed by unknown authority`
  - **D2.7** TLS 握手失败（TCP 成功但 HTTPS 请求失败）
- **FTA 底层事件映射**: `node-fta.md → evt_kubelet_cert`, `evt_node_cert_expire`
- **关联修复**: REM-008（手动证书轮转）
- **关联 Skill**: SKILL-SEC-001
- **交叉关联**:
  - RC-010（时间不同步）可导致证书验证失败的表现（证书看似有效但因时间偏差仍无法验证）
  - [v1.28+] kubelet 证书自动轮转（RotateKubeletClientCertificate）默认启用，若自动轮转失败需手动干预

---

### RC-008: PLEG 不健康导致 NotReady

- **描述**: Pod Lifecycle Event Generator 的 relist 操作超时（>3min），通常由 container runtime 响应慢引起
- **概率**: 中
- **诊断证据**:
  - **D2.6** 日志出现 `PLEG is not healthy`
  - **D1.2** Message 包含 "PLEG"
  - **D2.3** containerd 延迟高
- **FTA 底层事件映射**: `node-fta.md → evt_pleg`, `evt_and_pleg_timeout`, `evt_and_pleg_overload`
- **关联修复**: REM-003（重启 kubelet）、REM-004（重启 containerd）、REM-006（排空节点并重启）
- **交叉关联**:
  - RC-002（容器运行时故障）是 PLEG 不健康最常见的上游原因
  - 某个容器处于 D 状态（不可中断 I/O 等待）也可阻塞 CRI 调用导致 PLEG 超时
  - [v1.31+] EventedPLEG 默认启用后，传统 GenericPLEG 误报减少

---

### RC-009: 内核故障/硬件异常

- **描述**: 服务器硬件故障（磁盘坏块、内存 ECC 错误、CPU MCE）、内核 panic、文件系统损坏
- **概率**: 低
- **诊断证据**:
  - **D2.9** dmesg 包含 `Hardware Error`、`MCE` (Machine Check Exception)、`I/O error`、`device not responding`、`NMI watchdog: BUG: soft lockup`、`EXT4-fs error`、`XFS error`
  - 节点可能完全无法 SSH（硬件级故障）
- **FTA 底层事件映射**: `node-fta.md → evt_kernel_panic`, `evt_driver_issue`
- **关联修复**: REM-006（排空节点并重启）、REM-007（替换节点）、REM-009（内核热补丁/OS 升级）、REM-010（硬件更换）
- **交叉关联**:
  - 硬件故障可能导致 RC-001（kubelet 崩溃）、RC-002（containerd 崩溃）、RC-003（磁盘故障导致空间不可用）
  - `nf_conntrack: table full` 属于 RC-006（网络）和 RC-009（内核）的交叉区域

---

### RC-010: NTP 时间不同步

- **描述**: 节点时钟偏差过大，导致 TLS 证书验证失败和 Lease 续租异常
- **概率**: 低
- **诊断证据**:
  - **D2.10** 时钟未同步（`System clock synchronized: no`）或偏差 > 5s
  - **D2.8** 证书看似有效但 TLS 仍失败（因时间偏差导致证书验证时判断为过期/未生效）
- **FTA 底层事件映射**: `node-fta.md → evt_time_skew_tls`
- **关联修复**: 修复 NTP 同步（chrony/ntpd 配置）
- **交叉关联**:
  - RC-010 可导致 RC-007 的表现（证书有效但因时间偏差验证失败）
  - 时间偏差是最容易被忽视但影响广泛的根因，在诊断早期（D2.10）就应检查

---

### RC-011: CNI 插件异常

- **描述**: CNI 配置文件缺失、CNI 二进制文件损坏、CNI DaemonSet Pod 异常，导致节点网络不可用，kubelet 报告 NetworkUnavailable
- **概率**: 中
- **诊断证据**:
  - **D3.2** CNI 配置缺失（`/etc/cni/net.d/` 为空）或 CNI Pod 未运行
  - **D1.2** NetworkUnavailable=True
- **FTA 底层事件映射**: `node-fta.md → evt_cni_fail`
- **关联修复**: 重新部署 CNI DaemonSet、恢复 CNI 配置文件
- **交叉关联**:
  - CNI Pod 异常可能是 RC-002（容器运行时故障）的下游症状
  - CNI 配置被误删通常是运维误操作

---

### RC-012: 节点被手动 cordon/drain

- **描述**: 运维人员手动执行了 `kubectl cordon` 或 `kubectl drain`，节点被标记为 SchedulingDisabled，不属于故障
- **概率**: 低
- **诊断证据**:
  - **D1.4** 存在 `node.kubernetes.io/unschedulable:NoSchedule` taint
  - **D1.1** STATUS 包含 "SchedulingDisabled"（注意：STATUS 可能显示为 `Ready,SchedulingDisabled`，此时节点实际是健康的）
- **FTA 底层事件映射**: `node-fta.md → evt_cordon`（非故障，人工操作）
- **关联修复**: REM-001（取消 cordon 标记）
- **交叉关联**:
  - 这是常见的误诊场景 — 用户报告"节点异常"但实际是 SchedulingDisabled 而非 NotReady
  - D1.1 中需仔细区分 `NotReady` 和 `Ready,SchedulingDisabled`

---

## 根因交叉关联图

以下展示根因之间的因果和关联关系：

```
RC-009 (硬件/内核故障)
  ├─→ RC-001 (kubelet 崩溃)
  ├─→ RC-002 (containerd 崩溃)
  └─→ RC-003 (磁盘不可用)

RC-003 (磁盘压力)
  ├─→ RC-001 (kubelet 无法写入 → 崩溃)
  └─→ RC-002 (containerd 无法写入 → 异常)

RC-004 (内存压力)
  ├─→ RC-001 (OOM Killer 杀死 kubelet)
  └─→ RC-002 (OOM Killer 杀死 containerd)

RC-005 (PID 压力)
  ├─→ RC-001 (kubelet 无法 fork)
  └─→ RC-002 (containerd 无法创建 shim)

RC-002 (containerd 异常)
  └─→ RC-008 (PLEG 不健康 — 最常见的上游原因)

RC-010 (NTP 时间偏差)
  └─→ RC-007 的表现 (证书验证失败)

RC-006 (网络不通) ←→ RC-007 (证书过期)
  注意: TLS 握手失败可能被误判为网络故障
  诊断优先级: 先 D2.8 检查证书，再 D2.7 排查网络
```

> **提示**: 在诊断时注意根因的级联关系。例如，当发现 PLEG 不健康（RC-008）时，不应仅重启 kubelet，还应检查 containerd 状态（RC-002）和系统资源（RC-003/RC-004/RC-005），排查上游根因。
