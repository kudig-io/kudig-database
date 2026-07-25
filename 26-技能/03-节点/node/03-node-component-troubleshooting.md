---
title: 节点组件故障排查
description: 针对 kubelet、containerd/CRI-O、kube-proxy、CNI 等节点核心组件的故障排查技能，包含组件状态检查、日志分析、性能诊断和修复方案
summary: 节点组件是集群工作节点上的核心守护进程，本技能覆盖 kubelet/containerd/kube-proxy/CNI 四大组件的完整故障排查路径
category: skill
tags:
- k8s
- node
- troubleshooting
- kubelet
- containerd
- kube-proxy
- cni
- cri
- component
- sop
sources:
- 故障诊断/高级排障/35-node-component-troubleshooting.md
- 故障诊断/高级排障/structural-02-node-components/01-kubelet-troubleshooting.md
- 故障诊断/高级排障/structural-02-node-components/04-node-troubleshooting.md
- 故障诊断/技能体系/01-node-notready.md
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
estimated_read_time: 20min
intent_queries:
- kubelet 故障怎么排查
- containerd 异常如何处理
- kube-proxy 不工作怎么办
- 节点组件异常怎么诊断
- CNI 插件故障怎么排查
trigger_keywords:
- kubelet
- containerd
- kube-proxy
- CNI
- 节点组件
- 容器运行时
- 组件故障
- PLEG
- cri
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- node-architecture
- networking-basics
skill_id: SKILL-NODE-003
skill_name: 节点组件故障排查
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
fta_path: TE-1 -> IE-1.1/IE-1.2
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 节点组件故障排查

> **Skill ID**: SKILL-NODE-003
> **Agent 执行模式**: L2-semi-auto
> **预计修复时间**: 10-30 分钟
> **FTA 路径**: TE-1 → IE-1.1 (kubelet) / IE-1.2 (容器运行时)

---

## 1. 概述

Kubernetes 节点组件是集群工作节点上的核心守护进程，负责 Pod 的运行、网络管理和资源调度：

| 组件 | 职责 | 故障影响 | 紧急程度 |
|------|------|---------|---------|
| **kubelet** | 节点代理，Pod 生命周期管理 | Node NotReady | P0 |
| **containerd/CRI-O** | 容器运行时，容器执行 | Pod 无法创建 | P0 |
| **kube-proxy** | 网络代理，Service 规则维护 | Service 访问失败 | P1 |
| **CNI 插件** | Pod 网络配置 | Pod 网络不通 | P1 |

---

## 2. 症状识别

| # | 症状描述（错误消息/事件原文） | 检测方法 | 置信度 | 排除条件 | 路由 |
|---|---------------------------|---------|:---:|---------|------|
| S1 | kubelet 服务 `inactive (dead)` / `activating (auto-restart)` | `systemctl status kubelet` + `journalctl -u kubelet -n 50` | 0.95 | 节点宕机/失联属硬件层 → 转 01 | → 第 3 章 |
| S2 | kubelet 日志 `Unable to register node` / `node not found` | `journalctl -u kubelet \| grep -i register` | 0.85 | apiserver 不可用 → 转 [[26-技能/01-集群运维/cluster/01-apiserver-controlplane.md|控制面诊断]] | → 第 3 章 |
| S3 | kubelet 日志 `PLEG is not healthy: pleg was last seen active ...` | `journalctl -u kubelet \| grep PLEG` | 0.90 | 瞬时超时已恢复且无重复属偷发抖动 | → 第 4 章 |
| S4 | `crictl ps` 报 `connection refused` / `failed to dial` | `crictl ps` / `systemctl status containerd` | 0.95 | socket 路径配置错误属配置问题非运行时崩溃 | → 第 4 章 |
| S5 | Pod Events `FailedCreatePodSandBox: ... rpc error` | `kubectl describe pod` Events 段 | 0.90 | 镜像类报错（pull）转镜像技能 | → 第 4/6 章 |
| S6 | CNI 报错 `failed to allocate IP` / `no IP addresses available in range` | Pod Events / CNI 插件日志 | 0.90 | 集群级网段耗尽属容量规划问题 | → 第 6 章 |
| S7 | Service 不通，kube-proxy 日志 `Failed to list *v1.EndpointSlice` / iptables-restore 失败 | kube-proxy Pod 日志 / `iptables-save \| grep KUBE` | 0.85 | 单 Pod 不通属应用层，转网络技能集 | → 第 5 章 |

**工单关键词映射**：`kubelet 挂了`、`PLEG`、`sandbox`、`containerd 报错`、`kube-proxy`、`CNI`、`容器起不来但节点 Ready` → 触发本技能。

---

## 3. kubelet 故障排查

### 3.1 状态检查

```bash
# 🟢 低风险：只读/信息收集
# 基础状态验证
systemctl status kubelet
ps aux | grep kubelet
kubelet --version

# 配置文件检查
cat /var/lib/kubelet/config.yaml
ls -la /var/lib/kubelet/pki/
openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -text -noout

# 节点状态分析
kubectl describe node <node-name>
kubectl get node <node-name> -o jsonpath='{.status.conditions}'
kubectl get events --field-selector involvedObject.name=<node-name> --sort-by='.lastTimestamp'
```

### 3.2 日志分析

```bash
# 🟢 低风险
# 查看系统日志
journalctl -u kubelet --since "1 hour ago" --no-pager -n 200

# 常见错误模式识别
journalctl -u kubelet | grep -i "certificate|tls|x509"       # 证书错误
journalctl -u kubelet | grep -i "eviction|pressure|resource"  # 资源错误
journalctl -u kubelet | grep -i "network|connection|timeout"  # 网络错误
journalctl -u kubelet | grep -i "cri|container|containerd"    # 运行时错误
journalctl -u kubelet | grep -i "PLEG|pleg"                  # PLEG 错误
```

### 3.3 性能诊断

```bash
# 🟢 低风险
# 资源使用
top -p $(pgrep kubelet)
cat /proc/$(pgrep kubelet)/status | grep -E "VmRSS|VmSize"
ls -l /proc/$(pgrep kubelet)/fd | wc -l

# 健康检查
curl -sk https://localhost:10250/healthz
```

### 3.4 常见问题与修复

| 问题 | 症状 | 修复 |
|------|------|------|
| kubelet 未运行 | `systemctl status` 显示 inactive/failed | `systemctl restart kubelet` |
| 配置错误 | 启动日志报 config 错误 | 修复 `/var/lib/kubelet/config.yaml` |
| 证书过期 | x509 错误 | 证书轮转（见 SKILL-NODE-001 REM-008） |
| PLEG 不健康 | `PLEG is not healthy` | 检查 containerd → 重启运行时 |
| 内存泄漏 | kubelet RSS 持续增长 | 重启 kubelet + 升级版本 |

---

## 4. 容器运行时故障排查

### 4.1 containerd 状态检查

```bash
# 🟢 低风险
# 基础检查
systemctl status containerd
crictl info
containerd --version

# 镜像管理
crictl images
crictl inspecti <image-name>

# 容器状态
crictl ps
crictl ps -a  # 含已停止容器
crictl inspect <container-id>
crictl logs <container-id>
```

### 4.2 运行时日志分析

```bash
# 🟢 低风险
journalctl -u containerd --since "30 minutes ago" --no-pager -n 100
```
- `failed to create shim` → 磁盘满或 PID 耗尽
- `context deadline exceeded` → 磁盘 I/O 过慢
- `no space left on device` → 磁盘空间不足
- `plugin` + `error` → 特定插件问题

### 4.3 存储与性能检查

```bash
# 🟢 低风险
df -h /var/lib/containerd
df -i /var/lib/containerd
du -sh /var/lib/containerd

# 容器创建延迟测试
time crictl pull busybox:latest
```

### 4.4 常见问题与修复

| 问题 | 症状 | 修复 |
|------|------|------|
| containerd 未运行 | `systemctl status` failed | `systemctl restart containerd` |
| socket 无响应 | `crictl info` 超时 | 检查 socket 路径 + 重启 |
| 镜像拉取失败 | `crictl pull` 报错 | 检查网络/磁盘/registry |
| shim 进程泄漏 | 大量 containerd-shim | 清理 + 重启 containerd |
| 存储膨胀 | /var/lib/containerd >85% | `crictl rmi --prune` |

---

## 5. kube-proxy 故障排查

### 5.1 状态验证

```bash
# 🟢 低风险
# Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl describe pods -n kube-system -l k8s-app=kube-proxy

# 配置检查
kubectl get configmap -n kube-system kube-proxy -o yaml
kubectl get configmap -n kube-system kube-proxy -o jsonpath='{.data.config\.conf}' | grep mode
```

### 5.2 网络规则检查

```bash
# 🟢 低风险
# iptables 模式
iptables-save | grep -E "KUBE-SERVICES|KUBE-NODEPORTS" | head -20
iptables -t nat -L KUBE-SERVICES -n | wc -l

# ipvs 模式
ipvsadm -Ln | head -20

# 规则同步验证
kubectl get services --all-namespaces | wc -l
```

### 5.3 连接性测试

```bash
# 🟢 低风险
# 测试 ClusterIP 访问
kubectl run debug-pod --image=busybox --rm -it -- wget -qO- http://<service-ip>:<port>

# 测试 NodePort 访问
curl http://<node-ip>:<node-port>
```

### 5.4 常见问题与修复

| 问题 | 症状 | 修复 |
|------|------|------|
| kube-proxy CrashLoop | Pod 反复重启 | 检查日志 + 修复配置 |
| 规则未同步 | Service 数量与规则数不匹配 | 重启 kube-proxy Pod |
| conntrack 表满 | `nf_conntrack: table full` | 增大 `nf_conntrack_max` |
| 模式配置错误 | ipvs 模块未加载 | 加载内核模块或切换模式 |

---

## 6. CNI 插件故障排查

### 6.1 状态检查

```bash
# 🟢 低风险
# CNI 配置文件
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/*.conf 2>/dev/null || cat /etc/cni/net.d/*.conflist 2>/dev/null

# CNI 二进制文件
ls -la /opt/cni/bin/

# CNI Pod 状态（Calico/Cilium/Flannel）
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl get pods -n kube-system -l k8s-app=cilium
kubectl get pods -n kube-system -l app=flannel
```

### 6.2 常见问题与修复

| 问题 | 症状 | 修复 |
|------|------|------|
| CNI 配置缺失 | `/etc/cni/net.d/` 为空 | 重启 CNI DaemonSet Pod |
| CNI Pod 异常 | CrashLoopBackOff | 检查日志 + 修复配置 |
| Pod 网络不通 | 跨节点通信失败 | 检查 CNI 日志 + 路由 |
| NetworkUnavailable | 节点 Condition | 重新部署 CNI |

---

## 7. 综合诊断流程

```
节点组件异常报告
    │
    ├── kubectl get node → NotReady?
    │   ├── Yes → 检查 kubelet (Section 2)
    │   │   ├── kubelet 未运行 → 重启 kubelet
    │   │   ├── kubelet 运行但报错 → 分析日志
    │   │   │   ├── 运行时错误 → 检查 containerd (Section 3)
    │   │   │   ├── 网络错误 → 检查网络连通性
    │   │   │   └── 证书错误 → 证书轮转
    │   │   └── 资源压力 → SKILL-NODE-002
    │   └── No (Ready) → 检查具体症状
    │       ├── Pod 无法创建 → 检查 containerd (Section 3)
    │       ├── Service 不通 → 检查 kube-proxy (Section 4)
    │       └── Pod 网络不通 → 检查 CNI (Section 5)
    │
    └── 修复后验证
        ├── systemctl status kubelet/containerd
        ├── kubectl get node <node>
        └── kubectl get pods --field-selector spec.nodeName=<node> -A
```

---

## 8. 深层排查方法论

> 以下内容整合自 `技能/ts-node-components.md` 结构化排障框架

### 8.1 kubelet 分层模型与核心机制

kubelet 的稳定依赖于多个层面的健康，深入理解其内部机制是高效排查的关键：

#### 宿主机环境层
- **内核版本要求**：推荐 4.19+ 内核，过旧内核缺少关键特性（如 cgroup v2 支持）
- **cgroup 子系统**：kubelet 通过 cgroup 限制容器资源，检查 `/sys/fs/cgroup` 挂载状态
- **磁盘 IO**：kubelet 日志、容器层、etcd 数据共用磁盘，高 IO 负载会拖慢所有组件
- **网络栈**：节点网络不通会导致 kubelet 无法上报心跳，触发 NotReady
- **文件描述符**：每个容器消耗多个 fd，`ulimit -n` 需设置足够大（推荐 65535+）

#### 容器运行时接口层（CRI）
- **CRI 架构**：kubelet → CRI API (gRPC) → containerd/CRI-O
- **关键操作超时**：`runtimeRequestTimeout`（默认 2m），超时会导致 kubelet 标记 PLEG 不健康
- **cgroup 驱动一致性**：kubelet 和 CRI 必须使用相同驱动（systemd 或 cgroupfs）
  ```bash
  # 🟢 检查 cgroup 驱动一致性
  grep cgroupDriver /var/lib/kubelet/config.yaml
  grep SystemdCgroup /etc/containerd/config.toml
  # 两者必须一致！
  ```

#### 网络插件接口层（CNI）
- **CNI 调用时机**：Pod 创建时调用 CNI 插件配置网络（veth pair、路由、iptables）
- **配置路径**：`/etc/cni/net.d/` 和 `/opt/cni/bin/`
- **常见问题**：CNI 二进制缺失、配置错误、IP 池耗尽、网络插件 Pod 未就绪

#### 存储插件接口层（CSI）
- **卷挂载流程**：kubelet → CSI Plugin → 云厂商 API → 挂载到宿主机 → bind mount 到容器
- **挂载点泄露**：CSI 插件问题会导致挂载点僵死，kubelet 卡在清理阶段
- **检查命令**：`mount | grep kubernetes.io`

#### 配置与证书层
- **主配置文件**：`/var/lib/kubelet/config.yaml`
- **证书文件**：
  - `/var/lib/kubelet/pki/kubelet-client-current.pem`：客户端证书
  - `/var/lib/kubelet/pki/kubelet.crt`：服务端证书
- **证书轮转机制**：`rotateCertificates: true` 启用自动轮转，kubelet 在证书到期前自动生成 CSR

### 8.2 kube-proxy 三级排查模型

1. **Control Plane**：确认 API Server 中 Service 和 Endpoints 是否正确对齐
2. **Data Plane (Kernel)**：确认内核规则（iptables/IPVS/nftables）是否已生成并正确映射到 Endpoints
3. **Environment Layer**：确认内核参数（conntrack_max）、宿主机防火墙、CNI 网络连通性

### 8.3 容器运行时“剥洋葱”排查法

1. **接口层**：`crictl info` 是否能通？
2. **进程层**：`containerd-shim` 和 `runc` 是否正常？
3. **内核层**：`dmesg` 是否有 OOM 或文件系统报错？
4. **资源层**：Inode、磁盘空间、PID 限制是否触达？

### 8.4 节点问题综合决策树

```
节点问题
    │
    ├─── 节点 NotReady？
    │         │
    │         ├─ kubelet 状态 ──→ systemctl status kubelet
    │         ├─ 容器运行时 ──→ systemctl status containerd
    │         ├─ 网络问题 ──→ 检查节点网络连通性
    │         └─ 资源压力 ──→ 检查 Conditions
    │
    ├─── 资源压力？
    │         │
    │         ├─ MemoryPressure ──→ 检查内存使用/OOM
    │         ├─ DiskPressure ──→ 检查磁盘/inode
    │         └─ PIDPressure ──→ 检查进程数
    │
    ├─── Pod 无法调度？
    │         │
    │         ├─ 污点问题 ──→ 检查节点污点和 Pod 容忍
    │         ├─ 亲和性问题 ──→ 检查节点标签和亲和性规则
    │         ├─ 资源不足 ──→ 检查可用资源
    │         └─ 拓扑约束 ──→ 检查 topologySpreadConstraints
    │
    └─── Pod 被驱逐？
              │
              ├─ 优先级 ──→ 检查 PriorityClass
              ├─ QoS 类别 ──→ 检查资源配置
              └─ 驱逐策略 ──→ 检查 kubelet evictionHard 配置
```

---

## 版本兼容性注意事项

> 详细版本差异请参考 [reference/node-version-differences.md](reference/node-version-differences.md)

| 版本 | 关键差异 | 诊断影响 |
|------|---------|----------|
| 1.26+ | EventedPLEG 引入（Alpha，默认关） | PLEG 不健康诊断需确认是否启用了事件驱动模式 |
| 1.27+ | NodeLogQuery Alpha | kubelet 支持结构化日志查询 |
| 1.30+ | NodeLogQuery Beta | `kubectl get --raw /api/v1/nodes/<name>/log?query=` 可用 |
| 1.32+ | KubeletCrashLoopBackOffMax Alpha | CrashLoop 退避策略可配置 |
| 1.32+ | NodeStatus.runtimeHandlers | 可通过 API 查看运行时能力 |
| 1.35+ | KubeletCrashLoopBackOffMax Beta(默认开) | CrashLoop 最大退避时间默认生效 |
| 1.36+ | NodeLogQuery GA | 结构化日志查询始终可用 |
| 1.36+ | NodeStatus.declaredFeatures | 可查看节点声明的特性列表 |
| 1.36+ | **PLEG 架构重构**（PLEGOnDemandRelist Beta） | 新增按需单 Pod relist，日志中可能出现 "Relist request channel full" |

**版本特定诊断命令**：

```bash
# 🟢 查看节点运行时处理器（1.32+）
kubectl get node <node-name> -o jsonpath='{.status.runtimeHandlers}' | jq .

# 🟢 查看节点 CRI 特性（1.32+）
kubectl get node <node-name> -o jsonpath='{.status.features}' | jq .

# 🟢 结构化日志查询（1.30+ Beta，1.36+ GA）
kubectl get --raw "/api/v1/nodes/<node-name>/log/?query=kubelet&sinceTime=2026-01-01T00:00:00Z"

# 🟢 通用：直接查询 containerd 运行时信息（所有版本）
crictl info | jq '.config'
```

**CRI 接口稳定性说明**：代码分析确认 CRI API 接口（`RuntimeService`、`PodSandboxManager`、`ContainerManager`）在 1.28~1.36 间保持稳定，`crictl` 命令在所有版本中通用。

[存疑：此处关于 EventedPLEG 在 1.36 版本仍为 Alpha 的状态可能存在不准确之处，代码中确认 1.36 仍为 Alpha，但考虑到该特性自 1.26 引入已跨越多个版本，后续版本可能快速推进，需关注 KEP-3386 最新进展]

---

## 9. kubelet 内部核心机制详解

> 来源：`故障诊断/高级排障/structural-02-node-components/01-kubelet-troubleshooting.md`

深入理解 kubelet 内部架构是高效排查的关键。kubelet 由以下核心管理器协同工作：

### 9.1 SyncLoop（同步循环）

- **主控循环**：kubelet 的核心，持续运行 `watch → compare → reconcile`
- **数据源**：
  - API Server：监听分配到本节点的 Pod
  - 静态 Pod：监听 `/etc/kubernetes/manifests/` 目录
  - HTTP Endpoint：接收 HTTP 请求创建的 Pod
- **调和逻辑**：
  1. 计算期望状态与实际状态的差异
  2. 调用 CRI 创建/更新/删除容器
  3. 调用 CNI 配置网络
  4. 调用 CSI 挂载卷
  5. 更新 Pod 状态到 API Server

### 9.2 PLEG (Pod Lifecycle Event Generator)

- **职责**：通过定期 relist 检测容器状态变化（运行、退出、重启）
- **工作流程**：
  1. 每秒调用 CRI `ListPodSandbox` 和 `ListContainers`
  2. 比对前后两次结果，生成事件（ContainerStarted/ContainerDied/...）
  3. 事件进入 SyncLoop 处理队列
- **健康检查**：若 relist 耗时 > 3 分钟，PLEG 标记为不健康 → kubelet 停止上报心跳 → 节点 NotReady
- **常见问题**：
  - CRI 响应慢（IO 负载高、containerd 死锁）
  - 容器数量过多（建议单节点 < 110 Pod）
  - 容器频繁启停（每秒 > 10 个事件）

### 9.3 StatusManager（状态管理器）

- **职责**：将 Pod 状态同步到 API Server
- **批量优化**：收集多个 Pod 状态变化，批量更新（减少 API 调用）
- **冲突处理**：使用乐观锁（ResourceVersion）处理并发更新

### 9.4 ProbeManager（探针管理器）

- **职责**：执行 Liveness/Readiness/Startup 探针
- **探针类型**：HTTP GET / TCP Socket / Exec
- **并发限制**：默认每节点最多并发执行 10 个探针

### 9.5 VolumeManager（卷管理器）

- **职责**：管理 Pod 卷的挂载和卸载
- **挂载流程**：等待 Attach → Mount → Bind Mount 到容器
- **常见问题**：CSI 插件故障导致挂载点僵死，kubelet 卡在清理阶段
- **检查命令**：`mount | grep kubernetes.io`

### 9.6 EvictionManager（驱逐管理器）

- **职责**：监控节点资源压力，驱逐低优先级 Pod 保护节点
- **驱逐顺序**：BestEffort → Burstable(超请求) → Burstable(未超) → Guaranteed
- **关键配置**：见 [[26-技能/03-节点/node/02-node-resource-pressure.md|节点资源压力诊断]] §9

### 9.7 ImageGCManager / ContainerGCManager

| 管理器 | 职责 | 关键参数 |
|--------|------|----------|
| ImageGCManager | 回收未使用镜像 | `imageGCHighThresholdPercent`(85%) / `imageGCLowThresholdPercent`(80%) |
| ContainerGCManager | 删除已退出容器 | `--maximum-dead-containers-per-container`(1) |

### 9.8 kubelet 性能参数参考

| 参数 | 默认值 | 说明 |
|------|--------|------|
| 内存基线 | 100-200MB | 无 Pod 时 |
| 每 Pod 增量 | 10-20MB | 取决于卷、探针数量 |
| 大规模节点(110 Pod) | 2-3GB | 总内存消耗 |
| CPU 空闲 | < 50m | 正常状态 |
| CPU 高负载 | 500-1000m | 频繁 Pod 启停 |
| `--max-pods` | 110 | 单节点最大 Pod 数 |
| `--serialize-image-pulls` | true | 串行拉取镜像 |

### 9.9 生产环境"连环坑"场景

1. **PLEG Is Not Healthy 导致节点雪崩**：
   - 节点状态在 Ready/NotReady 间剧烈闪烁
   - 深层原因：containerd 因 IO 负载过高响应超过 3 分钟
2. **cgroup Driver 不一致的"隐形"失败**：
   - kubelet 启动正常，但 Pod 报 `FailedCreatePodSandBox`
   - 深层原因：kubelet 用 `systemd` 而 containerd 用 `cgroupfs`
   - 检查：`grep cgroupDriver /var/lib/kubelet/config.yaml` vs `grep SystemdCgroup /etc/containerd/config.toml`
3. **Inode 耗尽的"伪磁盘充足"**：
   - `df -h` 显示 50% 可用，但 Pod 报 `No space left on device`
   - 深层原因：大量小文件占满 Inode
   - 检查：`df -i`

---

## 相关链接

- [[26-技能/03-节点/node/README.md|Node 异常诊断技能集]]
- [[26-技能/03-节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]]
- [[26-技能/03-节点/node/02-node-resource-pressure.md|节点资源压力诊断]]
- [[26-技能/03-节点/node/05-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查]]
- [[26-技能/03-节点/node/reference/node-version-differences.md|版本差异对比]]
- [[19-故障诊断/04-高级排障/35-node-component-troubleshooting.md|节点组件故障排查（详细版）]]
- [[19-故障诊断/04-高级排障/structural-02-node-components/01-kubelet-troubleshooting.md|kubelet 故障排查指南（原始文件）]]
