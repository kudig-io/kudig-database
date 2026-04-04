# 版本兼容矩阵与知识进化 / Version Matrix & Knowledge Evolution

> **来源**: SKILL-NODE-001 v1.0 — 节点 NotReady 诊断与修复
> **本文档**: 提取自 Section 9（K8s 版本兼容矩阵）和 Section 10（知识进化）

---

## 目录

- [版本兼容矩阵](#版本兼容矩阵)
  - [功能差异表（v1.28-v1.32）](#91-功能差异表)
  - [诊断命令差异](#92-诊断命令差异)
  - [关键 API 版本](#93-关键-api-版本)
  - [版本相关的诊断注意事项](#94-版本相关的诊断注意事项)
- [知识进化](#知识进化)
  - [常见误诊模式](#101-常见误诊模式)
  - [深度知识引用](#102-深度知识引用)
  - [Skill 改进记录](#103-skill-改进记录)
  - [待补充的知识空白](#104-待补充的知识空白)

---

## 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| GracefulNodeShutdown | GA（默认启用） | GA | GA | GA | GA |
| Node swap support | alpha | alpha | beta（默认关闭） | beta | beta |
| kubelet 证书自动轮转 (RotateKubeletClientCertificate) | GA（默认启用） | GA | GA | GA | GA |
| kubelet 证书自动轮转 (RotateKubeletServerCertificate) | beta（默认启用） | beta | GA | GA | GA |
| EventedPLEG | beta（默认关闭） | beta（默认关闭） | beta（默认关闭） | beta（默认启用） | GA |
| `kubectl debug node/` | GA | GA | GA | GA | GA |
| Custom Debug Profiles | beta | beta | GA | GA | GA |
| NodeStatus 上报改进 | 基础 | 优化心跳频率 | 改进 Lease 上报 | 增强状态报告详细度 | 稳定 |
| Sidecar Containers | alpha | beta | beta | GA | GA |
| Node Resource Fit Scoring | 基础 | 基础 | 改进 | 改进 | 增强 |
| PodDisruptionConditions | beta | GA | GA | GA | GA |

---

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl debug node/<name>` | 支持，使用 `--image` 指定调试镜像 | 同左 | 新增 `--profile` 参数（GA） | 同左 | 同左 |
| `kubectl get --raw /api/v1/nodes/<name>/proxy/healthz` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get --raw /api/v1/nodes/<name>/proxy/configz` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl top node` (metrics-server) | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get lease -n kube-node-lease` | 支持（v1.17+ GA） | 同左 | 同左 | 同左 | 同左 |
| `crictl` 版本要求 | >=1.28 | >=1.29 | >=1.30 | >=1.31 | >=1.32 |

---

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Node | v1 (core) | v1 | v1 | v1 | v1 |
| Lease | coordination.k8s.io/v1 | v1 | v1 | v1 | v1 |
| Event | events.k8s.io/v1 | v1 | v1 | v1 | v1 |
| CSR (CertificateSigningRequest) | certificates.k8s.io/v1 | v1 | v1 | v1 | v1 |
| RuntimeClass | node.k8s.io/v1 | v1 | v1 | v1 | v1 |

---

### 9.4 版本相关的诊断注意事项

#### [v1.28+]: GracefulNodeShutdown 默认启用

当节点正在关机时，kubelet 会尝试优雅终止 Pod。在诊断时需注意区分计划关机和异常关机：

- 检查 `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods` 配置
- 日志中出现 `shutting down gracefully` 不一定是故障
- **诊断影响**: D2.2 中看到 `shutting down gracefully` 日志时，需确认是否为计划内操作

#### [v1.30+]: Node swap support (beta)

可能影响内存压力的判断：

- 如果 `NodeSwap` feature gate 启用且 `swapBehavior: LimitedSwap`，需同时检查 swap 使用情况
- `free -m` 输出中的 Swap 行不再是"异常"信号
- kubelet 的 `--fail-swap-on` 标志在启用 swap 时为 `false`
- **诊断影响**: D1.2 中 MemoryPressure 的计算可能包含 swap 使用量；D2.2 中 `swap is enabled` 日志属于正常信息；D2.5 中需结合 swap 配置判断内存压力

#### [v1.31+]: EventedPLEG 默认启用

- 传统 GenericPLEG 的 relist 操作频率降低，`PLEG is not healthy` 误报减少
- 但如果 EventedPLEG 本身异常，可能出现新的故障模式
- 诊断时需检查 `--feature-gates=EventedPLEG=true` 是否生效
- **诊断影响**: D2.6 中 PLEG 相关日志的解读需考虑 EventedPLEG 的行为差异；RC-008 的诊断逻辑需更新

#### [v1.32+]: nftables kube-proxy 模式 GA

- 使用 nftables 模式时，`iptables -L` 不再显示 kube-proxy 规则
- 需使用 `nft list ruleset` 检查规则
- **诊断影响**: D3.3 中检查 kube-proxy 规则的命令需根据模式调整：
  ```bash
  # iptables 模式（传统）
  iptables -t nat -L KUBE-SERVICES 2>/dev/null | head -5
  
  # ipvs 模式
  ipvsadm -Ln 2>/dev/null | head -10
  
  # nftables 模式（v1.32+ GA）
  nft list ruleset 2>/dev/null | grep -A5 "KUBE-SERVICES"
  ```

---

## 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **网络抖动误判为 kubelet 崩溃** | Node Condition 中 Ready=Unknown，看似 kubelet 停止发送心跳 | 网络链路不稳定（交换机端口 flapping、MTU 问题、云网络限流），kubelet 实际在运行但心跳包被丢弃 | 先 SSH 到节点确认 kubelet 进程状态（D2.1），再测试网络连通性（D2.7）。如果 kubelet 运行正常且本地 healthz 正常，优先排查网络 |
| **DiskPressure 归因于镜像过多，实则是日志轮转失败** | DiskPressure=True，磁盘使用率高 | 容器日志（stdout/stderr）未正确配置轮转（logMaxSize/logMaxFiles），单个 Pod 的日志占用几十 GB | 在 D2.5 中不仅检查整体磁盘使用率，还要检查 `/var/log/pods/` 或 `/var/log/containers/` 下的大文件：`du -sh /var/log/pods/* | sort -rh | head -10` |
| **PLEG 不健康误判为容器运行时故障** | kubelet 日志出现 `PLEG is not healthy`，初步判断为 containerd 异常 | 实际是某个 Pod 的 container 处于 D 状态（不可中断的 I/O 等待），阻塞了 CRI 调用，containerd 本身正常 | 在 D2.6 之后检查是否有 D 状态进程：`ps aux | awk '$8=="D"'`。如果有，定位到具体容器和 Pod，问题在应用层而非运行时 |
| **证书过期误判为网络故障** | kubelet 日志出现 "connection refused" 或 TLS 错误 | kubelet 客户端证书已过期，TLS 握手失败被解读为网络问题 | 在排查网络问题（D2.7）前先检查证书有效期（D2.8）。TLS 握手失败和 TCP 连接失败有本质区别 |
| **cordon 操作误判为节点故障** | 用户报告 Pod 无法调度到某节点，误认为节点 NotReady | 运维人员之前执行了 `kubectl cordon` 但未记录，节点状态为 `Ready,SchedulingDisabled` | D1.1 中仔细区分 `NotReady` 和 `Ready,SchedulingDisabled`；D1.4 检查 taints 中的 `unschedulable` 标记 |
| **时间偏差导致的间歇性故障** | 节点状态不稳定，时好时坏，难以找到明确根因 | 节点 NTP 未同步，时钟偏差导致 TLS 证书间歇性验证失败和 Lease 续租异常 | 在诊断早期（D2.10）就检查时间同步。时间偏差是最容易被忽视但影响广泛的根因 |

---

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| kubelet 架构与内部机制 | `domain-1-architecture-fundamentals/` | 理解 kubelet 心跳机制、node-lifecycle-controller 的驱逐逻辑 |
| Node 故障树分析 | `topic-fta/list/node-fta.md` | 理解 Node NotReady 的完整因果链和概率模型 |
| 节点级故障排查深度指南 | `topic-structural-trouble-shooting/` | 超出本 Skill 覆盖范围的深度排查方法 |
| Kubernetes 故障排查方法论 | `domain-12-troubleshooting/` | 系统化故障排查的理论基础和方法论 |
| 证书管理与 TLS | `SKILL-SEC-001` (06-certificate-expiry.md) | kubelet 证书过期的详细诊断与修复（本 Skill 的 RC-007 关联） |
| Pod 驱逐与调度 | `SKILL-POD-002` (03-pod-pending.md) | 节点恢复后 Pod 重新调度的相关问题 |
| 容器运行时排障 | `topic-structural-trouble-shooting/` | containerd/CRI-O 深度排查 |
| Linux 内核排障 | `domain-12-troubleshooting/` | OOM Killer、内核 panic、硬件错误的深度分析 |

---

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、10 个修复操作 | 首批 Skill 库建设，基于 top 工单分析确定节点 NotReady 为最高优先级场景 |

---

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **GPU 节点 NotReady**: GPU 驱动异常导致的节点 NotReady 场景（NVIDIA device plugin crash, GPU memory error）
2. **Windows 节点**: Windows 容器节点的 NotReady 诊断差异（kubelet on Windows, containerd on Windows）
3. **ARM 架构节点**: ARM 节点的特定故障模式
4. **边缘节点**: 使用 KubeEdge / OpenYurt 等边缘方案的节点 NotReady 诊断差异（弱网环境、离线容忍）
5. **虚拟节点**: Virtual Kubelet 实现的虚拟节点 NotReady 诊断
