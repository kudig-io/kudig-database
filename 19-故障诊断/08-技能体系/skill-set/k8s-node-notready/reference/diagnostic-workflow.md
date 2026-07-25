---
title: 诊断工作流 / Diagnostic Workflow
description: '- [Phase 2: 深度检查（只读，零风险，需 SSH）](#phase-2-深度检查只读零风险需-ssh)'
summary: '- [Phase 2: 深度检查（只读，零风险，需 SSH）](#phase-2-深度检查只读零风险需-ssh)'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- cilium
- calico
- containerd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- 诊断工作流 / Diagnostic Workflow 是什么
- 如何 诊断工作流 / Diagnostic Workflow
trigger_keywords:
- 诊断工作流
- Diagnostic
- Workflow
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- cilium-basics
- cni-basics
- etcd-basics
skill_id: SKILL-DIAGNOSTIC_WORKFLOW-001
skill_name: 诊断工作流 / Diagnostic Workflow
version: 1.0.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 诊断工作流 / Diagnostic Workflow

> **来源**: SKILL-NODE-001 v1.0 — 节点 NotReady 诊断与修复
> **本文档**: 提取自 Section 4（诊断工作流），包含完整的三阶段诊断流程

---

## 目录

- [Phase 1: 快速检查（只读，零风险）](#phase-1-快速检查只读零风险)
  - [D1.1 获取节点全局状态概览](#step-d11-获取节点全局状态概览)
  - [D1.2 获取节点详细状态和 Conditions](#step-d12-获取节点详细状态和-conditions)
  - [D1.3 检查节点事件](#step-d13-检查节点事件)
  - [D1.4 检查节点 Taints](#step-d14-检查节点-taints)
  - [D1.5 检查节点 Lease 对象](#step-d15-检查节点-lease-对象)
- [Phase 2: 深度检查（只读，零风险，需 SSH）](#phase-2-深度检查只读零风险需-ssh)
  - [D2.1 检查 [[kubelet|kubelet]] 服务状态](#step-d21-检查-kubelet-服务状态)
  - [D2.2 检查 kubelet 日志](#step-d22-检查-kubelet-日志)
  - [D2.3 检查容器运行时（[[containerd|containerd]]）服务状态](#step-d23-检查容器运行时containerd服务状态)
  - [D2.4 检查容器运行时日志](#step-d24-检查容器运行时日志)
  - [D2.5 检查系统资源压力](#step-d25-检查系统资源压力)
  - [D2.6 检查 PLEG 健康状态](#step-d26-检查-plegpod-lifecycle-event-generator健康状态)
  - [D2.7 检查节点到 apiserver 的网络连通性](#step-d27-检查节点到-apiserver-的网络连通性)
  - [D2.8 检查 kubelet 证书有效期](#step-d28-检查-kubelet-证书有效期)
  - [D2.9 检查内核日志](#step-d29-检查内核日志)
  - [D2.10 检查 NTP/时间同步](#step-d210-检查-ntp时间同步)
- [Phase 3: 主动探测（低风险，可能需审批）](#phase-3-主动探测低风险可能需审批)
  - [D3.1 从节点测试 apiserver 健康状态](#step-d31-从节点测试-apiserver-健康状态)
  - [D3.2 检查 CNI 插件状态](#step-d32-检查-cni-插件状态)
  - [D3.3 检查 kube-proxy 状态](#step-d33-检查-kube-proxy-状态)

---

## Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集节点状态信息，无需 SSH 登录节点。所有命令均为只读操作。
> **预计耗时**: 2-5 分钟

### Step D1.1: 获取节点全局状态概览

- **命令**:
  ```bash
  kubectl get nodes -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 表格输出包含 NAME, STATUS, ROLES, AGE, VERSION, INTERNAL-IP, EXTERNAL-IP, OS-IMAGE, KERNEL-VERSION, CONTAINER-RUNTIME
- **判断规则**:
  - STATUS 列为 `NotReady` → 记录节点名称、IP、版本信息，继续 D1.2
  - STATUS 列为 `Ready,SchedulingDisabled` → 节点已被 cordon，可能是 RC-012（手动操作），跳转根因分类确认
  - 命令超时 → apiserver 可能不可用，立即升级（参见快速分级中的立即升级触发条件）
- **版本差异**: 无

---

### Step D1.2: 获取节点详细状态和 Conditions

- **命令**:
  ```bash
  kubectl describe node <node-name>
  ```
- **超时**: 15s
- **预期输出模式**: 关注以下 Conditions 字段：
  ```
  Conditions:
    Type                 Status  LastHeartbeatTime                 LastTransitionTime                Reason                       Message
    ----                 ------  -----------------                 ------------------                ------                       -------
    MemoryPressure       False   ...                               ...                               KubeletHasSufficientMemory   kubelet has sufficient memory available
    DiskPressure         False   ...                               ...                               KubeletHasNoDiskPressure     kubelet has no disk pressure
    PIDPressure          False   ...                               ...                               KubeletHasSufficientPID      kubelet has sufficient PID available
    Ready                True    ...                               ...                               KubeletReady                 kubelet is posting ready status
  ```
- **判断规则**:
  - `Ready` 状态为 `False`，Reason 为 `KubeletNotReady` → kubelet 无法正常工作，继续 D1.3 并重点关注 kubelet（RC-001）
  - `Ready` 状态为 `Unknown` → apiserver 长时间未收到心跳，可能是网络问题（RC-006）或 kubelet 停止（RC-001）
  - `MemoryPressure` 为 `True` → 记录，可能根因为 RC-004
  - `DiskPressure` 为 `True` → 记录，可能根因为 RC-003
  - `PIDPressure` 为 `True` → 记录，可能根因为 RC-005
  - Message 字段包含 `[[22-概念/15-运行时与系统/container-runtime.md|container runtime]] is down` → RC-002（容器运行时问题）
  - Message 字段包含 `PLEG is not healthy` → RC-008（PLEG 不健康）
  - Message 字段包含 `certificate` 或 `x509` → RC-007（证书问题），关联 SKILL-SEC-001
- **版本差异**:
  - **[v1.30+]**: 若启用了 Node swap support (beta)，MemoryPressure 计算可能包含 swap 使用量，需结合 `--fail-swap-on=false` 配置判断
  - **[v1.31+]**: 改进的节点状态上报可能包含更详细的 Reason 信息

---

### Step D1.3: 检查节点事件

- **命令**:
  ```bash
  kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
    --sort-by=.lastTimestamp --no-headers | tail -30
  ```
- **超时**: 10s
- **预期输出模式**: 事件列表，关注 Warning 类型事件
- **判断规则**:
  - 出现 `NodeNotReady` 事件 → 确认 NotReady 时间点
  - 出现 `NodeHasDiskPressure` → 磁盘压力导致（RC-003）
  - 出现 `NodeHasMemoryPressure` → 内存压力导致（RC-004）
  - 出现 `NodeHasPIDPressure` → PID 压力导致（RC-005）
  - 出现 `NodeHasInsufficientMemory` → 内存不足（RC-004）
  - 出现 `InvalidDiskCapacity` → 磁盘配置异常（RC-003 变种）
  - 出现 `Rebooted` → 节点曾重启（关注 RC-009 内核/硬件问题）
  - 出现 `Starting` → kubelet 刚重启过（RC-001 的恢复迹象）
  - 无近期事件 → 可能是网络分区，apiserver 未收到任何更新（RC-006）
- **版本差异**: 无

---

### Step D1.4: 检查节点 Taints

- **命令**:
  ```bash
  kubectl get node <node-name> -o jsonpath='{range .spec.taints[*]}{.key}={.value}:{.effect}{"\n"}{end}'
  ```
- **超时**: 5s
- **预期输出模式**: Taint 列表
- **判断规则**:
  - 存在 `node.[[Kubernetes|kubernetes]].io/not-ready:NoSchedule` → Kubernetes 自动添加的 taint，确认 NotReady 状态
  - 存在 `node.kubernetes.io/not-ready:NoExecute` → Pod 驱逐已触发
  - 存在 `node.kubernetes.io/unreachable:NoExecute` → 节点不可达
  - 存在 `node.kubernetes.io/unschedulable:NoSchedule` → 节点已被 cordon（RC-012）
  - 存在 `node.kubernetes.io/disk-pressure:NoSchedule` → DiskPressure（RC-003）
  - 存在 `node.kubernetes.io/memory-pressure:NoSchedule` → MemoryPressure（RC-004）
  - 存在 `node.kubernetes.io/pid-pressure:NoSchedule` → PIDPressure（RC-005）
- **版本差异**: 无

---

### Step D1.5: 检查节点 Lease 对象

- **命令**:
  ```bash
  kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
  ```
- **超时**: 5s
- **预期输出模式**: ISO 8601 时间戳
- **判断规则**:
  - renewTime 距当前时间 > 40s（默认 node-monitor-grace-period）→ kubelet 未能续租，可能 kubelet 停止（RC-001）或网络不通（RC-006）
  - renewTime 距当前时间 < 40s 但节点仍 NotReady → 可能是 kubelet 报告了不健康状态（检查 Conditions 详情）
- **版本差异**: 无

---

## Phase 2: 深度检查（只读，零风险，需 SSH）

> **目标**: SSH 登录问题节点，检查系统级组件状态。所有命令均为只读操作。
> **前提**: 需要对问题节点的 SSH 访问权限
> **预计耗时**: 5-10 分钟

### Step D2.1: 检查 kubelet 服务状态

- **命令**:
  ```bash
  ssh <node-ip> "systemctl status kubelet"
  ```
- **超时**: 10s
- **预期输出模式**: systemd unit 状态信息
- **判断规则**:
  - `Active: active (running)` → kubelet 进程在运行，问题可能在运行时层面或网络层面，继续 D2.2
  - `Active: inactive (dead)` → kubelet 未运行（RC-001），尝试查看 D2.2 中的日志了解停止原因
  - `Active: activating (auto-restart)` → kubelet 不断崩溃重启（RC-001），查看 D2.2 中的日志
  - `Active: failed` → kubelet 启动失败（RC-001），查看 D2.2 中的错误日志
  - `Loaded: not-found` → kubelet 服务未安装或 unit 文件丢失，极端情况
- **版本差异**: 无

---

### Step D2.2: 检查 kubelet 日志

- **命令**:
  ```bash
  ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager -n 200"
  ```
- **超时**: 15s
- **预期输出模式**: kubelet 日志条目
- **判断规则**:
  - 日志包含 `connection refused` 或 `dial tcp <apiserver-ip>:6443: connect: connection refused` → 网络不通或 apiserver 不可达（RC-006）
  - 日志包含 `x509: certificate has expired` 或 `certificate signed by unknown authority` → 证书问题（RC-007），关联 SKILL-SEC-001
  - 日志包含 `PLEG is not healthy` → PLEG 不健康（RC-008），继续 D2.6
  - 日志包含 `container runtime is not running` 或 `runtime connect using default endpoints` → 容器运行时问题（RC-002）
  - 日志包含 `failed to garbage collect` + 磁盘相关错误 → 磁盘空间不足（RC-003）
  - 日志包含 `OOM` 或 `oom_kill` → 内存压力（RC-004）
  - 日志包含 `too many open files` 或 `no space left on device` → 资源耗尽（RC-003 或 RC-005）
  - 日志包含 `node not found` → 节点对象可能被意外删除
  - 日志包含 `failed to renew lease` → Lease 续租失败，检查网络和 apiserver
  - 日志包含 `use of closed network connection` → 网络连接异常（RC-006）
- **版本差异**:
  - **[v1.28+]**: GracefulNodeShutdown 默认启用。如果日志中出现 `shutting down gracefully`，可能节点正在优雅关机，不一定是问题
  - **[v1.30+]**: swap 相关日志 `swap is enabled` 在启用 NodeSwap feature gate 时属于正常信息

---

### Step D2.3: 检查容器运行时（containerd）服务状态

- **命令**:
  ```bash
  ssh <node-ip> "systemctl status containerd"
  ```
- **超时**: 10s
- **预期输出模式**: systemd unit 状态信息
- **判断规则**:
  - `Active: active (running)` → containerd 在运行，继续 D2.4 检查日志
  - `Active: inactive (dead)` 或 `Active: failed` → containerd 未运行（RC-002），需要重启
  - `Active: activating (auto-restart)` → containerd 不断崩溃（RC-002）
- **版本差异**: 无
- **注意**: 部分集群使用 CRI-O 替代 containerd，需检查 `systemctl status crio`

---

### Step D2.4: 检查容器运行时日志

- **命令**:
  ```bash
  ssh <node-ip> "journalctl -u containerd --since '30 minutes ago' --no-pager -n 100"
  ```
- **超时**: 15s
- **预期输出模式**: containerd 日志条目
- **判断规则**:
  - 日志包含 `failed to create shim` → shim 进程创建失败，可能磁盘满或 PID 耗尽
  - 日志包含 `context deadline exceeded` → containerd 内部操作超时，可能是磁盘 I/O 过慢
  - 日志包含 `plugin` + `error` → 特定 containerd 插件问题
  - 日志包含 `no space left on device` → 磁盘空间不足（RC-003）
  - 无异常日志 → containerd 正常，问题可能在 kubelet 或网络层
- **版本差异**: 无

---

### Step D2.5: 检查系统资源压力

- **命令**:
  ```bash
  # 磁盘使用
  ssh <node-ip> "df -h / /var/lib/kubelet /var/lib/containerd /var/log"

  # 内存使用
  ssh <node-ip> "free -m"

  # PID 使用
  ssh <node-ip> "echo 'Current PIDs:' && ps aux --no-heading | wc -l && echo 'Max PIDs:' && cat /proc/sys/kernel/pid_max"

  # inode 使用（常被忽视的磁盘问题）
  ssh <node-ip> "df -i / /var/lib/kubelet /var/lib/containerd"
  ```
- **超时**: 10s
- **预期输出模式**: 资源使用数据
- **判断规则**:
  - 磁盘使用率 > 85%（kubelet 默认 imagefs.available 驱逐阈值为 15%）→ RC-003
  - 磁盘使用率 > 100%（已满）→ RC-003（紧急）
  - inode 使用率 > 90% → RC-003（inode 耗尽同样导致 DiskPressure）
  - 可用内存 < 100Mi → RC-004
  - PID 数量接近 pid_max（通常默认 32768 或 4194304）→ RC-005
  - swap 使用量大但 `--fail-swap-on=true`（默认）→ 不影响 kubelet，但可能是内存压力信号
- **版本差异**:
  - **[v1.30+]**: 若 NodeSwap feature gate 启用且 kubelet 配置 `swapBehavior: LimitedSwap`，swap 使用是预期行为

---

### Step D2.6: 检查 PLEG（Pod Lifecycle Event Generator）健康状态

- **命令**:
  ```bash
  # 检查 kubelet 日志中的 PLEG 相关信息
  ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager | grep -i 'PLEG|pleg'"

  # 检查 kubelet 的 healthz 端点（如果 kubelet 仍在运行）
  ssh <node-ip> "curl -sk https://localhost:10250/healthz"
  ```
- **超时**: 10s
- **预期输出模式**: PLEG 相关日志和 healthz 状态
- **判断规则**:
  - 日志中频繁出现 `PLEG is not healthy` → RC-008。PLEG 不健康通常是由于 container runtime 响应慢，导致 relist 超时（默认 3 分钟）
  - 日志中出现 `GenericPLEG: Unable to retrieve pods` → container runtime 查询失败，关联 RC-002
  - healthz 返回 `ok` → kubelet 内部认为自己健康
  - healthz 返回非 `ok` 或连接失败 → kubelet 不健康
- **版本差异**:
  - **[v1.28+]**: EventedPLEG 作为 beta feature 可用（需手动启用），可减少 PLEG 不健康的误报
  - **[v1.31+]**: PLEG 性能改进，relist 超时处理更优雅

---

### Step D2.7: 检查节点到 apiserver 的网络连通性

- **命令**:
  ```bash
  # 获取 apiserver 地址（从 kubelet 配置或 kubeconfig 中读取）
  ssh <node-ip> "cat /etc/kubernetes/kubelet.conf | grep server"

  # 测试网络连通性（不发送 TLS 请求，仅测 TCP 层）
  ssh <node-ip> "nc -zv <apiserver-ip> 6443 -w 5"

  # 或使用 curl 测试（含 TLS）
  ssh <node-ip> "curl -sk --max-time 5 https://<apiserver-ip>:6443/healthz"
  ```
- **超时**: 15s
- **预期输出模式**: 连接成功/失败信息
- **判断规则**:
  - TCP 连接失败 → 网络分区（RC-006），检查防火墙、路由、交换机
  - TCP 成功但 TLS 握手失败 → 证书问题（RC-007）
  - TCP 成功且 TLS 成功但 healthz 返回非 `ok` → apiserver 自身异常（超出本 Skill 范围）
  - 一切正常 → 网络层没问题，回到 D2.1/D2.2 重新检查 kubelet 内部错误
- **版本差异**: 无

---

### Step D2.8: 检查 kubelet 证书有效期

- **命令**:
  ```bash
  # 检查 kubelet 客户端证书
  ssh <node-ip> "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject 2>/dev/null || echo 'Certificate file not found or not readable'"

  # 检查 kubelet serving 证书
  ssh <node-ip> "openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates -subject 2>/dev/null || echo 'Certificate file not found or not readable'"
  ```
- **超时**: 10s
- **预期输出模式**: 证书有效期信息（notBefore, notAfter）
- **判断规则**:
  - `notAfter` 早于当前时间 → 证书已过期（RC-007）
  - `notAfter` 在 7 天内 → 证书即将过期，建议预防性轮转
  - 证书文件不存在 → 证书可能被误删或 auto-rotation 失败（RC-007）
  - 证书有效 → 排除证书原因，继续其他诊断
- **版本差异**:
  - **[v1.28+]**: kubelet 证书自动轮转（RotateKubeletClientCertificate）默认启用（GA）
  - **[v1.29+]**: 改进的证书轮转日志，便于审计

---

### Step D2.9: 检查内核日志

- **命令**:
  ```bash
  ssh <node-ip> "dmesg -T | tail -50"
  ```
- **超时**: 10s
- **预期输出模式**: 内核日志条目
- **判断规则**:
  - 出现 `Out of memory: Killed process` → OOM Killer 触发（RC-004），记录被杀的进程（如果是 kubelet/containerd 被杀，直接定位根因）
  - 出现 `Hardware Error` 或 `MCE` (Machine Check Exception) → 硬件问题（RC-009）
  - 出现 `I/O error` 或 `device not responding` → 磁盘硬件问题（RC-009）
  - 出现 `NMI watchdog: BUG: soft lockup` → CPU 软锁死（RC-009）
  - 出现 `nf_conntrack: table full` → conntrack 表满，可能影响网络（RC-006 变种）
  - 出现 `EXT4-fs error` 或 `XFS error` → 文件系统错误（RC-009）
  - 无异常条目 → 内核/硬件层面正常
- **版本差异**: 无（与 K8s 版本无关，取决于 OS/内核版本）

---

### Step D2.10: 检查 NTP/时间同步

- **命令**:
  ```bash
  # 检查时间同步状态
  ssh <node-ip> "timedatectl status"

  # 或检查 chrony/ntpd 状态
  ssh <node-ip> "chronyc tracking 2>/dev/null || ntpq -p 2>/dev/null || echo 'No NTP service found'"

  # 对比节点时间与本地时间
  ssh <node-ip> "date -u"
  ```
- **超时**: 10s
- **预期输出模式**: 时间同步状态
- **判断规则**:
  - `System clock synchronized: no` → 时间未同步（RC-010）
  - 时间偏差 > 5 秒 → 可能导致证书验证失败和 Lease 续租异常（RC-010）
  - 时间偏差 > 1 分钟 → 严重偏差，几乎确定导致 TLS 握手失败（RC-010 + RC-007）
  - 时间同步正常 → 排除时间原因
- **版本差异**: 无

---

## Phase 3: 主动探测（低风险，可能需审批）

### Step D3.1: 从节点测试 apiserver 健康状态

- **命令**:
  ```bash
  ssh <node-ip> "curl -sk --max-time 10 https://<apiserver-ip>:6443/healthz?verbose"
  ```
- **超时**: 15s
- **风险级别**: 🟢 低（只读 HTTP GET 请求）
- **预期输出模式**: 健康检查各组件状态
- **判断规则**:
  - 所有组件返回 `ok` → apiserver 健康，问题在节点侧
  - 部分组件返回 `failed` → apiserver 自身有问题（如 etcd 连接异常）
  - 连接超时 → 网络不通（RC-006）
  - TLS 握手失败 → 证书问题（RC-007）
- **版本差异**: 无

---

### Step D3.2: 检查 CNI 插件状态

- **命令**:
  ```bash
  # 检查 CNI 配置文件是否存在
  ssh <node-ip> "ls -la /etc/cni/net.d/"

  # 检查 CNI 二进制文件
  ssh <node-ip> "ls -la /opt/cni/bin/"

  # 如果使用 Calico，检查 calico-node 容器
  ssh <node-ip> "crictl ps | grep calico"

  # 如果使用 Cilium，检查 cilium agent
  ssh <node-ip> "crictl ps | grep cilium"
  ```
- **超时**: 10s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: CNI 配置和二进制文件列表
- **判断规则**:
  - `/etc/cni/net.d/` 为空 → CNI 插件未配置或配置被删除（RC-011）
  - CNI 二进制文件缺失 → CNI 插件安装不完整（RC-011）
  - calico-node/cilium 容器未运行 → CNI DaemonSet Pod 异常（RC-011）
  - 一切正常 → CNI 插件状态良好
- **版本差异**: 无

---

### Step D3.3: 检查 kube-proxy 状态

- **命令**:
  ```bash
  # 检查 kube-proxy Pod 状态（如果使用 DaemonSet 部署）
  kubectl get pods -n kube-system -l k8s-app=kube-proxy --field-selector spec.nodeName=<node-name>

  # 在节点上检查 kube-proxy 进程
  ssh <node-ip> "crictl ps | grep kube-proxy"

  # 检查 iptables/ipvs 规则（判断 kube-proxy 是否正常工作）
  ssh <node-ip> "iptables -t nat -L KUBE-SERVICES 2>/dev/null | head -5 || ipvsadm -Ln 2>/dev/null | head -10"
  ```
- **超时**: 10s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: kube-proxy Pod 状态和规则信息
- **判断规则**:
  - kube-proxy Pod 不存在或 CrashLoopBackOff → kube-proxy 异常（不直接导致 NotReady，但影响 Service 网络）
  - iptables/ipvs 规则为空 → kube-proxy 未能同步规则
  - kube-proxy 正常 → 节点服务代理工作正常
- **版本差异**:
  - **[v1.29+]**: nftables 模式作为 alpha 可用
  - **[v1.31+]**: nftables 模式升级为 beta
  - **[v1.32+]**: nftables 模式 GA。使用 nftables 模式时，`iptables -L` 不再显示 kube-proxy 规则，需使用 `nft list ruleset` 检查规则

## Related

- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
