---
title: kubelet 故障排查指南
description: '# kubelet 故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- etcd
- kubelet
- scheduler
- prometheus
- containerd
- cri-o
- docker
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- kubelet 故障排查指南 是什么
- 如何 kubelet 故障排查指南
- kubelet 故障排查指南 故障排查
- kubelet 故障排查指南 排障步骤
trigger_keywords:
- kubelet
- 故障排查指南
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- etcd-basics
---

# kubelet 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 高级

## 🎯 本文档价值

| 读者对象 | 价值体现 |
| :--- | :--- |
| **初学者** | 建立对 Node 节点核心组件 kubelet 的全局认识，掌握节点 Ready/NotReady 的底层逻辑，学会使用标准的 `journalctl` 和 `kubectl` 命令定位基础故障。 |
| **资深专家** | 深入理解 kubelet 内部架构（如 PLEG、Manager 机制）、CRI 交互细节、驱逐策略的数学边界，以及大规模集群下的性能调优 and 自动化自愈方案。 |

---

## 0. 10 分钟快速诊断与止血

1. **节点面状态**：`kubectl get nodes -o wide`，抽样 `kubectl describe node <name>` 查看 Conditions/Taints，区分单点 vs 批量故障。
2. **kubelet 存活**：节点上执行 `curl -s localhost:10248/healthz`、`systemctl status kubelet`，若健康探针失败优先查证书/配置/资源。
3. **资源与压力**：`free -m`、`df -h`、`df -i`、`pidstat -p $(pgrep kubelet)`，确认 Memory/Disk/PID Pressure；若磁盘吃满先清理 `/var/lib/containerd` 旧镜像与日志。
4. **CRI 交互**：`crictl info`、`crictl ps -a | head`，若 CRI 超时则检查 containerd/Docker 服务、cgroup 驱动一致性（`cat /var/lib/kubelet/config.yaml | grep cgroupDriver`）。
5. **PLEG/驱逐信号**：`journalctl -u kubelet | grep -E "PLEG is not healthy|eviction" | tail`，辨别是运行时阻塞还是驱逐触发。
6. **快速缓解**：
   - 将故障节点 `cordon`，必要时 `drain --ignore-daemonsets --delete-emptydir-data`。
   - 重启运行时与 kubelet（确认已备份配置/证书），并检查 cgroup 驱动一致后再放行。
   - 若磁盘/内存压力，立即清理镜像/容器/日志或扩容磁盘，调整 `evictionHard`。
7. **证据留存**：保存 kubelet/CRI 关键日志、节点 Conditions、磁盘/PID/内存快照，便于复盘。

---

## 1. 问题现象与影响分析

### 1.1 核心原理解析：kubelet 的角色

kubelet 是运行在每个节点上的“蜂群指令官”，它不直接运行容器，而是通过 **CRI (Container Runtime Interface)** 控制容器运行时（如 containerd）。它的核心职责是：
1. **状态对齐（Reconciliation）**：确保 API Server 定义的 Pod 期望状态与节点实际运行状态一致。
2. **节点心跳**：定期向 API Server 上报节点状态，若中断则导致 `NotReady`。
3. **资源守门员**：通过 `eviction` 机制保护节点不因 OOM 或磁盘爆满而彻底崩溃。

### 1.2 常见问题现象

#### 1.2.1 kubelet 服务与连接异常

| 现象 | 报错信息关键字 | 根本原因方向 |
| :--- | :--- | :--- |
| **进程频繁崩溃** | `panic: ...` / `OOMKill` | 内存配置不足、内核 Bug、不兼容的 Flag |
| **启动超时** | `context deadline exceeded` | CRI 响应过慢、挂载卷超多、插件初始化失败 |
| **API 连接断开** | `x509: certificate has expired` | 证书轮转失效（未开启 `rotateCertificates`） |
| **PLEG 异常** | `PLEG is not healthy` | 容器运行时挂死、大量容器高频启停导致事件堆积 |

#### 1.2.2 节点状态与压力限制

| 状态 | 触发阈值（默认示例） | 影响 |
| :--- | :--- | :--- |
| **MemoryPressure** | `memory.available < 100Mi` | 触发 Pod 驱逐（从低优先级开始） |
| **DiskPressure** | `nodefs.available < 10%` | 停止拉取镜像，开始删除已退出的容器和未使用镜像 |
| **PIDPressure** | 达到 `pid_max` | 无法创建新进程，容器启动报错 `fork: retry: Resource temporarily unavailable` |

#### 1.2.3 生产环境典型“连环坑”场景

1. **PLEG Is Not Healthy 导致节点雪崩**：
   - **现象**：节点状态在 Ready/NotReady 之间剧烈闪烁。
   - **深层原因**：kubelet 的 PLEG (Pod Lifecycle Event Generator) 每秒检查容器状态，若容器运行时（containerd）因 IO 负载过高响应超过 3 分钟，kubelet 认为 PLEG 不健康，停止更新节点心跳。
2. **cgroup Driver 不一致导致的“隐形”失败**：
   - **现象**：kubelet 启动正常，但 Pod 启动报错 `FailedCreatePodSandBox`。
   - **深层原因**：kubelet 使用 `systemd` 而 containerd 使用 `cgroupfs`，导致内核资源包管理冲突。
3. **Inode 耗尽导致的“伪磁盘充足”**：
   - **现象**：`df -h` 显示磁盘还有 50%，但 Pod 报错 `No space left on device`。
   - **深层原因**：大量小文件（通常是日志或临时文件）占满了 Inode，导致元数据无法写入。

### 1.3 观测工具链（Expert's Toolbox）

```bash
# 深度诊断：查看 kubelet 内部状态（需在节点执行）
curl -s localhost:10248/healthz   # 基础健康检查
curl -s localhost:10255/metrics   # 暴露大量内部监控指标（默认端口 10255 或 10250）

# 专家级：追踪 CRI 交互过程
# 使用 crictl 模拟 kubelet 行为
crictl inspect <container-id>     # 查看容器底层的详细运行时状态
crictl stats                      # 查看实时资源占用

# 专家级：内核级追踪（定位死锁或系统调用失败）
strace -fp $(pgrep kubelet) -e trace=network,file
```

---

## 2. 排查方法与步骤

### 2.1 排查原理：分层模型与核心机制

kubelet 的稳定依赖于多个层面的健康，深入理解其内部机制是高效排查的关键：

#### 2.1.1 宿主机环境层
- **内核版本要求**：推荐 4.19+ 内核，过旧内核缺少关键特性（如 cgroup v2 支持）
- **cgroup 子系统**：kubelet 通过 cgroup 限制容器资源，检查 `/sys/fs/cgroup` 挂载状态
- **磁盘 IO**：kubelet 日志、容器层、etcd 数据共用磁盘，高 IO 负载会拖慢所有组件
- **网络栈**：节点网络不通会导致 kubelet 无法上报心跳，触发 NotReady
- **文件描述符**：每个容器消耗多个 fd（日志、挂载、socket），`ulimit -n` 需设置足够大（推荐 65535+）

#### 2.1.2 容器运行时接口层（CRI）
- **CRI 架构**：kubelet → CRI API (gRPC) → containerd/CRI-O/Docker shim
- **关键操作超时**：
  - `runtimeRequestTimeout`（默认 2m）：CRI 操作超时时间
  - 超时会导致 kubelet 标记 PLEG 不健康
- **cgroup 驱动一致性**：kubelet 和 CRI 必须使用相同驱动（systemd 或 cgroupfs）
  ```bash
  # 检查 kubelet cgroup 驱动
  grep cgroupDriver /var/lib/kubelet/config.yaml
  # 检查 containerd cgroup 驱动
  grep SystemdCgroup /etc/containerd/config.toml
  # 两者必须一致！
  ```
- **镜像管理**：kubelet 委托 CRI 拉取镜像，CRI 超时会阻塞 Pod 创建

#### 2.1.3 网络插件接口层（CNI）
- **CNI 调用时机**：Pod 创建时调用 CNI 插件配置网络（veth pair、路由、iptables）
- **配置路径**：`/etc/cni/net.d/` 和 `/opt/cni/bin/`
- **常见故障**：CNI 二进制缺失、配置错误、IP 池耗尽、网络插件 Pod 未就绪

#### 2.1.4 存储插件接口层（CSI）
- **卷挂载流程**：kubelet → CSI Plugin → 云厂商 API → 挂载到宿主机 → bind mount 到容器
- **挂载点泄露**：CSI 插件故障会导致挂载点僵死，kubelet 卡在清理阶段
- **检查命令**：`mount | grep kubernetes.io`

#### 2.1.5 配置与证书层
- **主配置文件**：`/var/lib/kubelet/config.yaml`（推荐）或启动参数
- **证书文件**：
  - `/var/lib/kubelet/pki/kubelet-client-current.pem`：kubelet 客户端证书
  - `/var/lib/kubelet/pki/kubelet.crt`：kubelet 服务端证书
- **证书轮转机制**：
  - `rotateCertificates: true`：启用自动轮转
  - kubelet 在证书到期前自动生成 CSR（CertificateSigningRequest）
  - Controller Manager 审批 CSR 并签发新证书
  - 失败原因：RBAC 权限不足、Controller Manager 未配置签发参数

#### 2.1.6 内部核心机制

##### 1. SyncLoop（同步循环）
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

##### 2. PLEG (Pod Lifecycle Event Generator)
- **职责**：通过定期 relist 检测容器状态变化（运行、退出、重启）
- **工作流程**：
  1. 每秒调用 CRI `ListPodSandbox` 和 `ListContainers`
  2. 比对前后两次结果，生成事件（ContainerStarted/ContainerDied/...）
  3. 事件进入 SyncLoop 处理队列
- **健康检查**：
  - 若 relist 耗时 > 3 分钟，PLEG 标记为不健康
  - 导致 kubelet 停止上报心跳，节点 NotReady
- **常见故障**：
  - CRI 响应慢（IO 负载高、containerd 死锁）
  - 容器数量过多（建议单节点 < 110 Pod）
  - 容器频繁启停（每秒 > 10 个事件）

##### 3. StatusManager（状态管理器）
- **职责**：将 Pod 状态同步到 API Server
- **批量优化**：收集多个 Pod 状态变化，批量更新（减少 API 调用）
- **冲突处理**：使用乐观锁（ResourceVersion）处理并发更新

##### 4. ProbeManager（探针管理器）
- **职责**：执行 Liveness/Readiness/Startup 探针
- **探针类型**：
  - HTTP GET：向容器发送 HTTP 请求
  - TCP Socket：尝试 TCP 连接
  - Exec：在容器内执行命令
- **并发限制**：默认每节点最多并发执行 10 个探针，避免过载

##### 5. VolumeManager（卷管理器）
- **职责**：管理 Pod 卷的挂载和卸载
- **挂载流程**：
  1. 等待卷 Attach（云盘挂载到节点）
  2. 执行 Mount（挂载到节点目录）
  3. Bind Mount 到容器
- **卸载流程**：反向操作，卸载失败会导致 Pod 删除卡住

##### 6. EvictionManager（驱逐管理器）
- **职责**：监控节点资源压力，驱逐低优先级 Pod 保护节点
- **压力类型**：
  - **MemoryPressure**：内存不足
  - **DiskPressure**：磁盘空间不足
  - **PIDPressure**：进程数达到上限
- **驱逐策略**：
  ```yaml
  # 硬驱逐（立即驱逐，无宽限期）
  evictionHard:
    memory.available: "100Mi"
    nodefs.available: "10%"
    nodefs.inodesFree: "5%"
    imagefs.available: "15%"
  
  # 软驱逐（宽限期后驱逐）
  evictionSoft:
    memory.available: "200Mi"
    nodefs.available: "15%"
  evictionSoftGracePeriod:
    memory.available: "1m30s"
    nodefs.available: "2m"
  ```
- **驱逐顺序**：
  1. BestEffort Pod（无资源请求）
  2. Burstable Pod 且使用量超过请求量
  3. Burstable Pod 且使用量未超请求量
  4. Guaranteed Pod（最后驱逐）

##### 7. ImageGCManager（镜像垃圾回收器）
- **职责**：回收未使用的镜像，释放磁盘空间
- **回收策略**：
  - `imageGCHighThresholdPercent`（默认 85%）：磁盘使用率超过此值触发 GC
  - `imageGCLowThresholdPercent`（默认 80%）：GC 直到降至此值
- **回收顺序**：按镜像使用时间排序，优先删除最久未用的

##### 8. ContainerGCManager（容器垃圾回收器）
- **职责**：删除已退出的容器
- **回收参数**：
  - `--maximum-dead-containers-per-container`（默认 1）：每个 Pod 保留的死容器数
  - `--minimum-container-ttl-duration`（默认 0）：容器死亡后最少保留时间

#### 2.1.7 性能与资源层
- **内存消耗**：
  - 基线：约 100-200MB
  - 每 Pod 增加：约 10-20MB（取决于卷、探针数量）
  - 大规模节点（110 Pod）：约 2-3GB
- **CPU 消耗**：
  - 空闲：< 50m
  - 高负载（频繁 Pod 启停）：500-1000m
- **并发参数**：
  - `--max-pods`（默认 110）：单节点最大 Pod 数
  - `--pods-per-core`：根据 CPU 核数限制 Pod 数
  - `--serialize-image-pulls`（默认 true）：串行拉取镜像，避免并发拉取压垮磁盘

### 2.2 专家级排查工作流

#### 阶段一：快速止损
1. **检查节点状态**：`kubectl get nodes`。
2. **确认是否为全局故障**：如果是多节点 NotReady，优先查网络、API Server 或证书过期。
3. **设置节点不可调度**：`kubectl cordon <node-name>`，防止故障期间负载继续涌入。

#### 阶段二：现场诊断
1. **查看服务状态**：`systemctl status kubelet`。
2. **抓取关键日志**：
   ```bash
   # 查找最近 5 分钟的严重错误
   journalctl -u kubelet --since "5m" -p err
   ```
3. **检查 PLEG 状态**：
   ```bash
   journalctl -u kubelet | grep "PLEG is not healthy"
   ```

#### 阶段三：联动排查
1. **CRI 状态确认**：
   ```bash
   crictl info | jq .status.conditions
   ```
2. **存储挂载确认**：
   ```bash
   # 检查是否有僵死挂载点
   mount | grep "kubernetes.io" | awk '{print $3}' | xargs ls > /dev/null
   ```

---

## 3. 专家级解决方案与性能调优

### 3.1 解决 PLEG Not Healthy
- **短期方案**：重启容器运行时（containerd）和 kubelet。
- **长期方案**：
  - 优化镜像拉取速度，减少高频 Pod 启停。
  - 调整内核参数 `fs.inotify.max_user_watches`（PLEG 依赖监听）。
  - 增加节点磁盘 IOPS。

### 3.2 优化资源预留（防止节点夯死）
生产环境必须配置资源预留，否则当 Pod 负载过高时，kubelet 自身会因申请不到 CPU/内存而假死。
```yaml
# /var/lib/kubelet/config.yaml
systemReserved:
  cpu: "500m"
  memory: "1Gi"
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
enforceNodeAllocatable: ["pods", "system-reserved", "kube-reserved"]
```

### 3.3 证书自动轮转实战
配置 `rotateCertificates: true` 仅是第一步，还需确保 Controller Manager 允许 CSR 自动审批：
1. 检查 kubelet 配置：`rotateCertificates: true`。
2. 检查 RBAC：确保 kubelet 有权创建 CSR。
3. 如果证书已过期无法启动：手动续签并重启。

### 3.4 磁盘压力（DiskPressure）的深度治理
- **自动清理策略**：
  ```yaml
  imageGCHighThresholdPercent: 80
  imageGCLowThresholdPercent: 70
  ```
- **日志轮转优化**：
  修改 `/etc/logrotate.d/` 确保宿主机日志不挤占空间。
  配置 `containerLogMaxSize` 和 `containerLogMaxFiles`。

---

## 4. 自动化运维与预防

### 4.1 节点健康自愈（NPD + Draino）
1. **Node Problem Detector (NPD)**：部署 NPD 监测内核死锁、文件系统只读、内存坏道等异常。
2. **Draino/Descheduler**：根据 NPD 暴露的 Condition 自动驱离 Pod 并重启节点。

### 4.2 监控核心指标（Prometheus）
| 指标 | 含义 | 风险点 |
| :--- | :--- | :--- |
| `kubelet_pleg_relist_duration_seconds` | PLEG 周期耗时 | 持续 > 1s 表示运行时压力大 |
| `kubelet_node_config_error` | 配置错误计数 | > 0 表示配置未生效 |
| `kubelet_runtime_operations_errors_total` | CRI 操作错误数 | 增长表示运行时异常 |

---

## 5. 排查方法与步骤 (基础版)

### 1.1 常见问题现象

#### 1.1.1 kubelet 服务不可用

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 进程未运行 | `kubelet.service: Failed` | systemd | `systemctl status kubelet` |
| 启动失败 | `failed to run kubelet` | kubelet 日志 | `journalctl -u kubelet` |
| 证书错误 | `x509: certificate has expired` | kubelet 日志 | kubelet 日志 |
| 配置错误 | `failed to load kubelet config` | kubelet 日志 | kubelet 启动日志 |
| API Server 连接失败 | `unable to connect to API server` | kubelet 日志 | kubelet 日志 |

#### 1.1.2 节点状态异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 节点 NotReady | `KubeletNotReady` | kubectl | `kubectl get nodes` |
| 节点 Unknown | `NodeStatusUnknown` | kubectl | `kubectl get nodes` |
| 节点压力 | `MemoryPressure/DiskPressure/PIDPressure` | kubectl | `kubectl describe node` |
| 容器运行时不可用 | `container runtime is down` | kubelet 日志 | kubelet 日志 |

#### 1.1.3 Pod 管理问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Pod 无法创建 | `failed to create pod` | Pod Events | `kubectl describe pod` |
| Pod 无法启动 | `failed to start container` | Pod Events | `kubectl describe pod` |
| 镜像拉取失败 | `ImagePullBackOff/ErrImagePull` | Pod Events | `kubectl describe pod` |
| 探针失败 | `Liveness/Readiness probe failed` | Pod Events | `kubectl describe pod` |
| Pod 被驱逐 | `The node was low on resource` | Pod Events | `kubectl describe pod` |
| CSI 卷挂载失败 | `MountVolume.SetUp failed` | Pod Events | `kubectl describe pod` |

#### 1.1.4 资源相关问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 磁盘空间不足 | `DiskPressure` | 节点状态 | `kubectl describe node` |
| 内存不足 | `MemoryPressure` | 节点状态 | `kubectl describe node` |
| PID 耗尽 | `PIDPressure` | 节点状态 | `kubectl describe node` |
| inode 耗尽 | `inodes exhausted` | kubelet 日志 | kubelet 日志 |
| cgroup 配置错误 | `cgroup driver mismatch` | kubelet 日志 | kubelet 日志 |

#### 1.1.5 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **磁盘爆满导致批量节点 NotReady** | 多节点同时变为 NotReady | 日志文件累积、镜像缓存膨胀 | 磁盘清理策略、监控告警 |
| **内核版本升级后 kubelet 异常** | 节点状态异常、cgroup 错误 | 内核与容器运行时不兼容 | 灰度升级、版本验证 |
| **网络分区导致节点失联** | 节点状态 Unknown | 网络故障、防火墙规则变更 | 网络质量监控、双网卡冗余 |
| **恶意挖矿程序占用资源** | 节点压力异常、CPU 使用率飙升 | 安全漏洞被利用 | 安全加固、准入控制 |
| **容器运行时版本不兼容** | Pod 启动失败、镜像拉取异常 | 运行时升级后兼容性问题 | 版本兼容性测试、回滚机制 |

### 1.2 报错查看方式汇总

```bash
# 查看 kubelet 服务状态
systemctl status kubelet

# 查看 kubelet 日志
journalctl -u kubelet -f --no-pager -l

# 查看最近的错误日志
journalctl -u kubelet -p err --since "1 hour ago"

# 查看节点状态
kubectl get nodes
kubectl describe node <node-name>

# 查看节点条件
kubectl get node <node-name> -o jsonpath='{.status.conditions[*]}' | jq

# 查看节点事件
kubectl get events --field-selector=involvedObject.kind=Node

# 检查 kubelet 健康状态
curl -k https://localhost:10250/healthz

# 查看 kubelet 指标
curl -k https://localhost:10250/metrics

# 查看 Pod 列表（kubelet API）
curl -k https://localhost:10250/pods
```

### 1.3 影响面分析

#### 1.3.1 直接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **该节点所有 Pod** | 高 | Pod 状态无法更新，新 Pod 无法创建 |
| **节点状态报告** | 完全失效 | 节点状态无法上报给 API Server |
| **容器生命周期** | 失效 | 容器无法创建、启动、停止 |
| **健康检查** | 失效 | 探针检查无法执行 |
| **日志采集** | 部分影响 | kubelet 日志 API 不可用 |
| **指标采集** | 部分影响 | kubelet 指标 API 不可用 |

#### 1.3.2 间接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **已运行的容器** | 容器继续运行 | 但无法被管理和监控 |
| **服务发现** | 部分影响 | Endpoints 可能过期 |
| **调度** | 受影响 | 新 Pod 可能被调度到异常节点 |
| **节点驱逐** | 触发 | 节点长时间 NotReady 会触发 Pod 驱逐 |
| **监控告警** | 可能失效 | 节点级监控数据缺失 |

#### 1.3.3 故障传播链

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         kubelet 故障影响传播链                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   kubelet 故障                                                               │
│       │                                                                      │
│       ├──► 节点状态无法上报 ──► 节点变为 NotReady                            │
│       │                              │                                       │
│       │                              └──► 触发 Node Controller                │
│       │                                        │                             │
│       │                                        └──► 超时后驱逐 Pod            │
│       │                                                                      │
│       ├──► Pod 状态无法更新 ──► Pod 状态显示为旧状态                         │
│       │                                                                      │
│       ├──► 新 Pod 无法创建 ──► 该节点上新调度的 Pod 卡在 Pending             │
│       │                                                                      │
│       ├──► 容器运行时交互失败 ──► 容器无法创建/删除                          │
│       │                                                                      │
│       ├──► 健康检查停止 ──► 已有 Pod 状态可能不准确                          │
│       │                                                                      │
│       └──► 卷管理失效 ──► 卷挂载/卸载失败                                    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 5. 排查方法与步骤 (基础版)

### 5.1 排查原理

kubelet 是节点上的核心代理，负责 Pod 生命周期管理。排查需要从以下层面：

1. **服务层面**：kubelet 进程是否正常运行
2. **连接层面**：与 API Server、容器运行时的连接
3. **配置层面**：kubelet 配置是否正确
4. **资源层面**：节点资源是否充足
5. **证书层面**：证书是否有效

### 2.2 排查逻辑决策树

```
开始排查
    │
    ├─► 检查 kubelet 进程
    │       │
    │       ├─► 进程不存在 ──► 检查启动失败原因
    │       │
    │       └─► 进程存在 ──► 继续下一步
    │
    ├─► 检查容器运行时
    │       │
    │       ├─► 运行时故障 ──► 排查容器运行时
    │       │
    │       └─► 运行时正常 ──► 继续下一步
    │
    ├─► 检查 API Server 连接
    │       │
    │       ├─► 连接失败 ──► 检查网络和证书
    │       │
    │       └─► 连接正常 ──► 继续下一步
    │
    ├─► 检查节点资源
    │       │
    │       ├─► 资源不足 ──► 清理资源或扩容
    │       │
    │       └─► 资源充足 ──► 继续下一步
    │
    └─► 检查具体错误
            │
            ├─► Pod 创建失败 ──► 分析 Pod Events
            │
            └─► 其他错误 ──► 根据日志分析
```

### 2.3 排查步骤和具体命令

#### 2.3.1 第一步：检查 kubelet 进程状态

```bash
# 检查 kubelet 服务状态
systemctl status kubelet

# 检查进程是否存在
ps aux | grep kubelet | grep -v grep

# 查看启动参数
cat /proc/$(pgrep kubelet)/cmdline | tr '\0' '\n'

# 检查 kubelet 配置文件
cat /var/lib/kubelet/config.yaml

# 查看 kubelet 启动配置
cat /etc/systemd/system/kubelet.service.d/10-kubeadm.conf

# 检查健康端点
curl -k https://localhost:10250/healthz

# 查看 kubelet 版本
kubelet --version
```

#### 2.3.2 第二步：检查容器运行时

```bash
# 检查 containerd 状态
systemctl status containerd

# 检查 Docker 状态（如果使用 Docker）
systemctl status docker

# 使用 crictl 检查运行时
crictl info

# 列出所有容器
crictl ps -a

# 检查容器运行时 socket
ls -la /run/containerd/containerd.sock
# 或
ls -la /var/run/cri-dockerd.sock

# 测试容器运行时连接
crictl version
```

#### 2.3.3 第三步：检查 API Server 连接

```bash
# 检查 kubelet 证书
ls -la /var/lib/kubelet/pki/

# 检查证书有效期
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 测试 API Server 连接
kubectl --kubeconfig=/etc/kubernetes/kubelet.conf get nodes

# 查看 kubelet 日志中的连接错误
journalctl -u kubelet | grep -iE "(unable to connect|connection refused)" | tail -20

# 检查 API Server 地址配置
grep server /etc/kubernetes/kubelet.conf
```

#### 2.3.4 第四步：检查节点资源

```bash
# 检查磁盘空间
df -h
df -i  # inode 使用

# 检查内存
free -h

# 检查 PID 数量
ls /proc | grep -E "^[0-9]+$" | wc -l
cat /proc/sys/kernel/pid_max

# 检查容器镜像占用
crictl images
du -sh /var/lib/containerd/
du -sh /var/lib/docker/  # 如果使用 Docker

# 检查日志占用
du -sh /var/log/

# 检查节点压力
kubectl describe node $(hostname) | grep -A5 Conditions
```

#### 2.3.5 第五步：检查 cgroup 配置

```bash
# 检查 kubelet cgroup 驱动配置
cat /var/lib/kubelet/config.yaml | grep cgroupDriver

# 检查容器运行时 cgroup 驱动
# containerd
cat /etc/containerd/config.toml | grep SystemdCgroup

# Docker
docker info | grep "Cgroup Driver"

# 检查系统 cgroup 版本
mount | grep cgroup
cat /sys/fs/cgroup/cgroup.controllers  # cgroup v2
```

#### 2.3.6 第六步：检查 Pod 相关问题

```bash
# 查看节点上的 Pod 列表
kubectl get pods --all-namespaces --field-selector=spec.nodeName=$(hostname)

# 查看 Pod Events
kubectl get events --field-selector=involvedObject.kind=Pod --sort-by='.lastTimestamp'

# 检查特定 Pod 详情
kubectl describe pod <pod-name> -n <namespace>

# 查看 Pod 日志
kubectl logs <pod-name> -n <namespace>

# 通过 kubelet API 查看 Pod
curl -k https://localhost:10250/pods | jq '.items[].metadata.name'

# 检查静态 Pod 目录
ls -la /etc/kubernetes/manifests/
```

#### 2.3.7 第七步：检查日志

```bash
# 实时查看 kubelet 日志
journalctl -u kubelet -f --no-pager

# 查看最近错误
journalctl -u kubelet -p err --since "30 minutes ago"

# 查看启动日志
journalctl -u kubelet -b | head -100

# 查找特定错误
journalctl -u kubelet | grep -iE "(error|failed|unable)" | tail -50

# 查找镜像相关错误
journalctl -u kubelet | grep -i "image" | tail -30

# 查找卷相关错误
journalctl -u kubelet | grep -i "volume" | tail -30

# 查找探针相关错误
journalctl -u kubelet | grep -i "probe" | tail -30
```

### 2.4 排查注意事项

#### 2.4.1 安全注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **kubelet 证书** | 包含节点认证信息 | 不要泄露 |
| **kubeconfig** | 有节点权限 | 妥善保管 |
| **kubelet API** | 可以访问 Pod 信息 | 限制访问 |
| **日志敏感性** | 可能包含敏感信息 | 注意分享范围 |

#### 2.4.2 操作注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **重启影响** | 重启 kubelet 会影响 Pod 管理 | 在维护窗口操作 |
| **容器运行时依赖** | kubelet 依赖容器运行时 | 先检查运行时 |
| **静态 Pod** | 静态 Pod 由 kubelet 直接管理 | 修改 manifest 需谨慎 |
| **驱逐时间** | kubelet 长时间不可用会触发驱逐 | 尽快恢复 |

---

## 解决方案与风险控制（基础版）

### 6.1 kubelet 进程未运行

#### 3.1.1 解决步骤

```bash
# 步骤 1：检查启动失败原因
journalctl -u kubelet -b --no-pager | tail -100

# 步骤 2：检查配置文件
cat /var/lib/kubelet/config.yaml

# 步骤 3：验证配置语法
kubelet --config=/var/lib/kubelet/config.yaml --dry-run

# 步骤 4：检查依赖服务
systemctl status containerd
# 或
systemctl status docker

# 步骤 5：修复问题后重启
systemctl daemon-reload
systemctl restart kubelet

# 步骤 6：验证恢复
systemctl status kubelet
kubectl get node $(hostname)
```

#### 3.1.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 重启期间 Pod 管理中断 | 在维护窗口操作 |
| **低** | 配置检查一般无风险 | - |
| **中** | 配置修改可能引入新问题 | 修改前备份 |

#### 3.1.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. kubelet 重启期间节点上的 Pod 管理暂停
2. 已运行的容器不会被停止
3. 长时间故障会触发 Pod 驱逐
4. 修改配置前备份原始文件
5. 确保容器运行时正常后再重启 kubelet
```

### 3.2 节点 NotReady

#### 3.2.1 解决步骤

```bash
# 步骤 1：确认节点状态
kubectl get node $(hostname) -o wide
kubectl describe node $(hostname) | grep -A10 Conditions

# 步骤 2：检查 kubelet 状态
systemctl status kubelet
journalctl -u kubelet --since "10 minutes ago" | tail -50

# 步骤 3：检查容器运行时
systemctl status containerd
crictl info

# 步骤 4：检查网络连接
ping -c 3 <api-server-ip>
curl -k https://<api-server-ip>:6443/healthz

# 步骤 5：如果是证书问题，续签证书
kubeadm certs renew kubelet-client

# 步骤 6：重启 kubelet
systemctl restart kubelet

# 步骤 7：验证恢复
kubectl get node $(hostname)
```

#### 3.2.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | NotReady 持续可能触发驱逐 | 尽快恢复 |
| **低** | 检查状态无风险 | - |
| **中** | 证书续签需要重启 | 在维护窗口操作 |

#### 3.2.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 节点 NotReady 超过 pod-eviction-timeout 会触发驱逐
2. 默认驱逐超时为 5 分钟
3. 先排除网络问题再考虑重启
4. 证书续签会短暂中断连接
5. 监控节点状态恢复时间
```

### 3.3 节点资源压力（DiskPressure/MemoryPressure/PIDPressure）

#### 3.3.1 解决步骤

```bash
# 步骤 1：确认压力类型
kubectl describe node $(hostname) | grep -A10 Conditions

# DiskPressure 解决方案
# 步骤 2a：清理无用镜像
crictl rmi --prune

# 步骤 3a：清理已退出的容器
crictl rm $(crictl ps -a -q --state exited)

# 步骤 4a：清理日志
find /var/log -type f -name "*.log" -mtime +7 -delete
journalctl --vacuum-time=3d

# 步骤 5a：检查大文件
du -sh /* | sort -rh | head -10

# MemoryPressure 解决方案
# 步骤 2b：查找内存占用高的进程
ps aux --sort=-%mem | head -20

# 步骤 3b：查找内存占用高的 Pod
kubectl top pods --all-namespaces --sort-by=memory

# 步骤 4b：考虑驱逐低优先级 Pod
kubectl delete pod <low-priority-pod> -n <namespace>

# PIDPressure 解决方案
# 步骤 2c：查找 PID 占用多的进程
ps -eo pid,ppid,cmd | wc -l
for pid in $(ls /proc | grep -E "^[0-9]+$"); do
  threads=$(ls /proc/$pid/task 2>/dev/null | wc -l)
  if [ "$threads" -gt 100 ]; then
    echo "PID $pid: $threads threads"
  fi
done

# 步骤 3c：增加 PID 限制
echo 65536 > /proc/sys/kernel/pid_max

# 验证恢复
kubectl describe node $(hostname) | grep -A10 Conditions
```

#### 3.3.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 清理镜像可能影响 Pod 启动 | 只清理未使用的镜像 |
| **中** | 删除 Pod 会影响服务 | 优先删除非关键 Pod |
| **低** | 清理日志一般无风险 | 保留最近的日志 |

#### 3.3.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 节点压力会触发 Pod 驱逐
2. 清理前确认不会影响正在运行的服务
3. 增加 PID 限制需要评估系统承载能力
4. 考虑配置节点资源预留（system-reserved）
5. 长期方案是增加节点资源或分散负载
```

### 3.4 镜像拉取失败

#### 3.4.1 解决步骤

```bash
# 步骤 1：确认错误类型
kubectl describe pod <pod-name> | grep -A5 "Events:"

# 常见错误类型：
# - ImagePullBackOff: 多次拉取失败后的退避状态
# - ErrImagePull: 拉取失败
# - ErrImageNeverPull: imagePullPolicy=Never 但本地无镜像

# 步骤 2：测试镜像拉取
crictl pull <image-name>

# 步骤 3：检查镜像仓库认证
kubectl get secret -n <namespace> | grep -i registry
kubectl get pod <pod-name> -o yaml | grep -A5 imagePullSecrets

# 步骤 4：检查镜像仓库连通性
curl -v https://<registry-url>/v2/

# 步骤 5：如果是私有仓库认证问题，创建 Secret
kubectl create secret docker-registry regcred \
  --docker-server=<registry-url> \
  --docker-username=<username> \
  --docker-password=<password> \
  --docker-email=<email> \
  -n <namespace>

# 步骤 6：更新 Pod 使用 imagePullSecrets
kubectl patch serviceaccount default -n <namespace> \
  -p '{"imagePullSecrets": [{"name": "regcred"}]}'

# 步骤 7：重新创建 Pod
kubectl delete pod <pod-name> -n <namespace>
```

#### 3.4.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **低** | 创建 Secret 无风险 | - |
| **中** | 删除 Pod 会导致服务中断 | 确保有副本或在维护窗口 |
| **低** | 测试拉取无风险 | - |

#### 3.4.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 镜像仓库凭证是敏感信息
2. 不要在命令历史中留下密码
3. 优先使用 ServiceAccount 绑定 imagePullSecrets
4. 考虑使用镜像缓存或镜像仓库代理
5. 检查网络策略是否阻止了镜像拉取
```

### 3.5 探针失败

#### 3.5.1 解决步骤

```bash
# 步骤 1：确认探针配置
kubectl get pod <pod-name> -o yaml | grep -A20 livenessProbe
kubectl get pod <pod-name> -o yaml | grep -A20 readinessProbe

# 步骤 2：查看探针失败日志
kubectl describe pod <pod-name> | grep -A10 Events

# 步骤 3：进入容器手动测试探针
kubectl exec -it <pod-name> -- sh

# HTTP 探针测试
curl -v http://localhost:<port>/<path>

# TCP 探针测试
nc -zv localhost <port>

# 命令探针测试
<probe-command>

# 步骤 4：检查应用日志
kubectl logs <pod-name>

# 步骤 5：调整探针参数（如果探针配置不合理）
kubectl patch deployment <name> -p '{
  "spec": {
    "template": {
      "spec": {
        "containers": [{
          "name": "<container-name>",
          "livenessProbe": {
            "initialDelaySeconds": 60,
            "periodSeconds": 10,
            "timeoutSeconds": 5,
            "failureThreshold": 3
          }
        }]
      }
    }
  }
}'

# 步骤 6：验证修复
kubectl get pod <pod-name> -w
```

#### 3.5.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **低** | 查看探针配置无风险 | - |
| **中** | 修改探针参数可能影响故障检测 | 评估后再调整 |
| **低** | 手动测试探针无风险 | - |

#### 3.5.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 探针过于激进可能导致不必要的重启
2. 探针过于宽松可能延迟故障检测
3. 修改前理解应用启动特性
4. initialDelaySeconds 要大于应用启动时间
5. 生产环境建议同时配置 liveness 和 readiness 探针
```

### 3.6 卷挂载失败

#### 3.6.1 解决步骤

```bash
# 步骤 1：确认错误类型
kubectl describe pod <pod-name> | grep -A10 Events

# 常见错误：
# - MountVolume.SetUp failed: volume not attached
# - MountVolume.WaitForAttach failed
# - Unable to mount volumes: timed out

# 步骤 2：检查 PVC 状态
kubectl get pvc -n <namespace>
kubectl describe pvc <pvc-name> -n <namespace>

# 步骤 3：检查 PV 状态
kubectl get pv
kubectl describe pv <pv-name>

# 步骤 4：检查 CSI 驱动状态
kubectl get pods -n kube-system | grep csi
kubectl logs -n kube-system <csi-pod>

# 步骤 5：检查节点上的挂载
mount | grep <volume-name>
ls -la /var/lib/kubelet/pods/<pod-uid>/volumes/

# 步骤 6：如果是云盘，检查云平台状态
# 阿里云
aliyun ecs DescribeDisks --DiskIds='["<disk-id>"]'
# AWS
aws ec2 describe-volumes --volume-ids <volume-id>

# 步骤 7：强制卸载并重新挂载
# ⚠️ 危险操作，确认后执行
umount /var/lib/kubelet/pods/<pod-uid>/volumes/<volume-type>/<volume-name>

# 步骤 8：重启 Pod
kubectl delete pod <pod-name> -n <namespace>
```

#### 3.6.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 强制卸载可能导致数据损坏 | 确保数据已同步 |
| **中** | 删除 Pod 会导致服务中断 | 在维护窗口操作 |
| **低** | 检查状态无风险 | - |

#### 3.6.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 卷挂载失败可能是云平台配额问题
2. 强制卸载前确认没有写操作进行
3. 检查 CSI 驱动的 RBAC 权限
4. 多 AZ 场景注意卷和节点的 AZ 匹配
5. 考虑使用卷快照进行数据保护
```

### 3.7 kubelet 证书问题

#### 3.7.1 解决步骤

```bash
# 步骤 1：检查证书状态
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject

# 步骤 2：检查证书是否即将过期
kubeadm certs check-expiration

# 步骤 3：如果证书过期，续签证书
# 方法 1：使用 kubeadm 续签
kubeadm certs renew kubelet-client

# 方法 2：重新加入集群（如果证书完全不可用）
# 在 master 节点获取 token
kubeadm token create --print-join-command

# 在工作节点执行
kubeadm reset
kubeadm join <master-ip>:6443 --token <token> --discovery-token-ca-cert-hash <hash>

# 步骤 4：重启 kubelet
systemctl restart kubelet

# 步骤 5：验证恢复
kubectl get node $(hostname)
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

#### 3.7.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | kubeadm reset 会删除节点配置 | 仅在必要时使用 |
| **中** | 重新加入需要停止节点上的 Pod | 在维护窗口操作 |
| **低** | 证书续签一般无风险 | 验证后重启 |

#### 3.7.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. kubelet 证书续签会短暂中断服务
2. 建议配置自动证书轮转
3. 在 kubelet 配置中设置 rotateCertificates: true
4. 定期检查证书有效期，设置告警
5. kubeadm reset 是破坏性操作，谨慎使用
```

---

## 附录

### A. kubelet 关键指标

| 指标名称 | 说明 | 告警阈值建议 |
|----------|------|--------------|
| `kubelet_running_containers` | 运行中的容器数 | 异常变化 |
| `kubelet_runtime_operations_duration_seconds` | 运行时操作延迟 | P99 > 10s |
| `kubelet_runtime_operations_errors_total` | 运行时操作错误 | > 0 |
| `kubelet_volume_stats_used_bytes` | 卷使用量 | > 80% 容量 |
| `kubelet_pod_start_duration_seconds` | Pod 启动时间 | P99 > 30s |

### B. 常见配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--container-runtime-endpoint` | - | 容器运行时 socket |
| `--cgroup-driver` | cgroupfs | cgroup 驱动 |
| `--max-pods` | 110 | 最大 Pod 数 |
| `--eviction-hard` | - | 硬驱逐阈值 |
| `--eviction-soft` | - | 软驱逐阈值 |
| `--system-reserved` | - | 系统预留资源 |
| `--kube-reserved` | - | Kubernetes 预留资源 |

### C. kubelet 配置文件示例

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cgroupDriver: systemd
containerRuntimeEndpoint: unix:///run/containerd/containerd.sock
evictionHard:
  imagefs.available: 15%
  memory.available: 100Mi
  nodefs.available: 10%
  nodefs.inodesFree: 5%
evictionSoft:
  imagefs.available: 20%
  memory.available: 200Mi
  nodefs.available: 15%
evictionSoftGracePeriod:
  imagefs.available: 1m
  memory.available: 1m
  nodefs.available: 2m
kubeReserved:
  cpu: 100m
  memory: 1Gi
maxPods: 110
rotateCertificates: true
serverTLSBootstrap: true
systemReserved:
  cpu: 100m
  memory: 500Mi
```

---

## 📚 D. 生产环境实战案例精选

### 案例 1：PLEG Not Healthy 导致节点雪崩式 NotReady

#### 🎯 故障场景
某电商公司双十一大促，集群 300 节点突然在 10 分钟内有 50+ 节点状态在 Ready/NotReady 之间剧烈闪烁，导致大量 Pod 被驱逐和重新调度，业务出现大面积 5xx 错误。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   # 大量节点 NotReady
   kubectl get nodes | grep NotReady | wc -l
   # 53  # ❌ 集群 1/6 节点异常
   
   # 节点状态频繁变化
   kubectl get nodes --watch
   # node-worker-10   Ready      5s ago
   # node-worker-10   NotReady   10s ago
   # node-worker-10   Ready      15s ago  # ❌ 剧烈闪烁
   ```

2. **kubelet 日志检查**：
   ```bash
   # 登录故障节点查看日志
   ssh node-worker-10
   journalctl -u kubelet | grep -i "PLEG"
   # Jan 10 08:15:23 kubelet[1234]: E0110 PLEG is not healthy: pleg was last seen active 3m15s ago
   # Jan 10 08:15:28 kubelet[1234]: E0110 PLEG is not healthy: pleg was last seen active 3m20s ago
   # ❌ PLEG 超过 3 分钟未响应！
   
   # 查看 relist 耗时
   journalctl -u kubelet | grep "GenericPLEG.*took"
   # I0110 08:15:10 generic.go:123] GenericPLEG: Relisting took 185.234s
   # I0110 08:15:15 generic.go:123] GenericPLEG: Relisting took 192.456s
   # ❌ 单次 relist 耗时 3 分钟+！
   ```

3. **CRI 性能分析**：
   ```bash
   # 检查 containerd 状态
   systemctl status containerd
   # Active: active (running)  # 进程存活
   
   # 测试 CRI 响应速度
   time crictl pods | wc -l
   # real    3m15.234s  # ❌ 耗时 3+ 分钟！
   # 正常应 < 1 秒
   
   # 检查容器数量
   crictl ps | wc -l
   # 350  # 单节点 350 个容器（含已退出）
   
   # 检查磁盘 IO
   iostat -x 1 10
   # Device  r/s   w/s   util
   # sda     5000  3000  100%  # ❌ 磁盘 IO 打满！
   ```

4. **根因分析**：
   - **直接原因**：磁盘 IO 打满（100% util），containerd 响应极慢
   - **触发链条**：
     1. 大促流量激增 → 大量 Pod 创建/销毁
     2. 容器日志疯狂写入磁盘（每 Pod 10MB/s × 350 = 3.5GB/s）
     3. 磁盘 IO 饱和 → containerd 操作缓慢（ListPods 耗时 3+ 分钟）
     4. PLEG relist 超时 → kubelet 停止心跳 → 节点 NotReady
     5. 节点 NotReady → Pod 驱逐 → 更多 Pod 创建 → 恶性循环
   - **为什么是部分节点**：这些节点使用机械硬盘（300 IOPS），其他节点使用 SSD（10000 IOPS）

#### ⚡ 应急措施
1. **立即隔离故障节点**：
   ```bash
   # 批量 cordon 机械硬盘节点
   kubectl get nodes -l disk-type=hdd -o name | xargs kubectl cordon
   
   # 驱逐 Pod 到 SSD 节点
   for node in $(kubectl get nodes -l disk-type=hdd -o name); do
     kubectl drain $node --ignore-daemonsets --delete-emptydir-data --grace-period=30 &
   done
   ```

2. **临时限制日志写入**：
   ```bash
   # 在故障节点临时限制容器日志大小
   ssh node-worker-10 "crictl ps -q | xargs -I {} crictl inspect {} | \
     jq -r '.info.config.logPath' | xargs truncate -s 0"
   
   # 效果：秒级清空所有容器日志，释放 IO
   iostat -x 1 3
   # Device  util
   # sda     30%  # ✅ IO 恢复正常
   ```

3. **重启 kubelet 恢复心跳**：
   ```bash
   # 批量重启故障节点 kubelet
   for node in $(kubectl get nodes -l disk-type=hdd -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}'); do
     ssh $node "systemctl restart kubelet" &
   done
   
   # 5 分钟后验证
   kubectl get nodes | grep NotReady | wc -l
   # 0  # ✅ 全部恢复
   ```

#### 🛡️ 长期优化
1. **迁移至 SSD 存储**：
   ```bash
   # 评估成本
   # 机械硬盘：300 IOPS，$0.05/GB/月
   # SSD：10000+ IOPS，$0.10/GB/月
   # ROI：减少 90% 故障率，值得投入
   
   # 逐步迁移
   # 1. 新节点全部使用 SSD
   # 2. 逐步下线机械硬盘节点
   # 3. 3 个月内完成迁移
   ```

2. **优化日志管理**：
   ```yaml
   # kubelet 配置限制容器日志
   apiVersion: kubelet.config.k8s.io/v1beta1
   kind: KubeletConfiguration
   containerLogMaxSize: 10Mi      # ✅ 单文件最大 10MB（默认无限）
   containerLogMaxFiles: 3        # ✅ 保留 3 个轮转文件
   
   # 效果：每容器最多 30MB 日志，350 容器 = 10GB 总量（可控）
   ```

3. **配置日志收集外部化**：
   ```yaml
   # 使用 Fluent Bit DaemonSet 收集日志到外部存储
   apiVersion: apps/v1
   kind: DaemonSet
   metadata:
     name: fluent-bit
     namespace: logging
   spec:
     template:
       spec:
         containers:
         - name: fluent-bit
           image: fluent/fluent-bit:2.0
           volumeMounts:
           - name: varlog
             mountPath: /var/log
             readOnly: true
           - name: containers
             mountPath: /var/lib/docker/containers
             readOnly: true
         volumes:
         - name: varlog
           hostPath:
             path: /var/log
         - name: containers
           hostPath:
             path: /var/lib/docker/containers
   
   # 应用配置禁用 stdout 日志
   apiVersion: v1
   kind: Pod
   metadata:
     name: myapp
   spec:
     containers:
     - name: app
       image: myapp:latest
       args:
       - --log-to-file=/logs/app.log  # ✅ 日志写入文件，由 Fluent Bit 收集
       volumeMounts:
       - name: logs
         mountPath: /logs
   ```

4. **提高 PLEG 容忍度**：
   ```yaml
   # kubelet 配置（谨慎调整）
   apiVersion: kubelet.config.k8s.io/v1beta1
   kind: KubeletConfiguration
   runtimeRequestTimeout: 5m      # ✅ 从默认 2m 提高至 5m
   # 注意：仅缓解症状，根本解决需优化磁盘 IO
   ```

5. **监控告警**：
   ```yaml
   # Prometheus 告警规则
   groups:
   - name: kubelet-pleg
     rules:
     - alert: PLEGDurationHigh
       expr: histogram_quantile(0.99, rate(kubelet_pleg_relist_duration_seconds_bucket[5m])) > 10
       for: 5m
       labels:
         severity: warning
       annotations:
         summary: "PLEG relist 耗时过高"
         description: "节点 {{ $labels.node }} PLEG P99 耗时 {{ $value }}s，可能导致 NotReady"
     
     - alert: ContainerdSlowResponse
       expr: histogram_quantile(0.99, rate(kubelet_runtime_operations_duration_seconds_bucket{operation_type="list_pods"}[5m])) > 30
       for: 5m
       labels:
         severity: warning
       annotations:
         summary: "containerd 响应慢"
         description: "节点 {{ $labels.node }} containerd ListPods P99 耗时 {{ $value }}s"
     
     - alert: DiskIOUtilHigh
       expr: node_disk_io_time_seconds_total > 0.9
       for: 5m
       labels:
         severity: warning
       annotations:
         summary: "磁盘 IO 利用率高"
         description: "节点 {{ $labels.node }} 磁盘 IO 利用率 {{ $value | humanizePercentage }}"
   ```

#### 💡 经验总结
- **存储选型错误**：对 IO 敏感的 kubelet/containerd 运行在机械硬盘上
- **日志失控**：未限制容器日志大小，导致 IO 打满
- **监控盲区**：未监控 PLEG 耗时和磁盘 IO 利用率
- **改进方向**：SSD 迁移、日志外部化、容器日志限制、监控告警、定期压测

---

### 案例 2：cgroup 驱动不一致导致 Pod 创建失败

#### 🎯 故障场景
某科技公司升级 Kubernetes 从 v1.24 到 v1.28，升级后新节点加入集群，所有 Pod 都无法创建，报错 `FailedCreatePodSandBox`，但老节点正常运行。

#### 🔍 排查过程
1. **现象确认**：
   ```bash
   # 新节点加入成功
   kubectl get nodes
   # NAME           STATUS   ROLES    AGE   VERSION
   # node-new-01    Ready    worker   5m    v1.28.0  # ✅ Ready
   
   # 但 Pod 创建失败
   kubectl get pods -o wide | grep node-new-01
   # myapp-abc123   0/1   ContainerCreating   0   10m   node-new-01
   
   kubectl describe pod myapp-abc123
   # Events:
   # Warning  FailedCreatePodSandBox  1m  Failed to create pod sandbox: rpc error: code = Unknown desc = failed to create containerd task
   ```

2. **kubelet 日志检查**：
   ```bash
   ssh node-new-01
   journalctl -u kubelet | grep -i "failed to create pod sandbox"
   # E0110 failed to create pod sandbox: rpc error: code = Unknown desc = failed to setup OOM score for container: write /sys/fs/cgroup/system.slice/containerd.service/kubepods-besteffort-pod123.slice/cgroup.procs: no such file or directory
   # ❌ cgroup 路径错误！
   ```

3. **cgroup 驱动检查**：
   ```bash
   # 检查 kubelet cgroup 驱动
   grep cgroupDriver /var/lib/kubelet/config.yaml
   # cgroupDriver: systemd  # kubelet 使用 systemd
   
   # 检查 containerd cgroup 驱动
   grep SystemdCgroup /etc/containerd/config.toml
   # SystemdCgroup = false  # ❌ containerd 使用 cgroupfs！
   
   # 不一致！
   ```

4. **老节点对比**：
   ```bash
   ssh node-old-01
   grep cgroupDriver /var/lib/kubelet/config.yaml
   # cgroupDriver: cgroupfs  # 老节点都用 cgroupfs
   
   grep SystemdCgroup /etc/containerd/config.toml
   # SystemdCgroup = false  # 一致 ✅
   ```

5. **根因分析**：
   - **变更历史**：v1.28 推荐使用 systemd cgroup 驱动
   - **配置不一致**：新节点 kubelet 配置为 systemd，但 containerd 仍为 cgroupfs
   - **错误原因**：
     - kubelet 按 systemd 路径创建 cgroup：`/sys/fs/cgroup/system.slice/kubepods.slice/...`
     - containerd 按 cgroupfs 路径查找：`/sys/fs/cgroup/cpu/kubepods/...`
     - 路径不匹配导致容器创建失败

#### ⚡ 应急措施
1. **统一 cgroup 驱动为 systemd**：
   ```bash
   # 修改 containerd 配置
   ssh node-new-01
   vim /etc/containerd/config.toml
   
   # 修改以下部分
   [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
     SystemdCgroup = true  # ✅ 改为 true
   
   # 重启 containerd
   systemctl restart containerd
   
   # 验证配置
   crictl info | grep -i cgroup
   # "systemdCgroup": true  ✅
   ```

2. **重启 kubelet**：
   ```bash
   systemctl restart kubelet
   
   # 等待 kubelet Ready
   kubectl wait --for=condition=Ready node/node-new-01 --timeout=60s
   ```

3. **验证 Pod 创建**：
   ```bash
   # 删除旧 Pod 触发重建
   kubectl delete pod myapp-abc123
   
   # 验证新 Pod 创建成功
   kubectl get pod myapp-abc123 -o wide --watch
   # myapp-abc123   1/1   Running   0   30s   10.244.10.50   node-new-01  ✅
   ```

#### 🛡️ 长期优化
1. **全集群统一 cgroup 驱动**：
   ```bash
   # 制定迁移计划
   # 目标：全部节点统一使用 systemd cgroup 驱动
   
   # 步骤 1：验证 kubelet 版本支持（v1.22+）
   kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}{end}'
   
   # 步骤 2：逐节点迁移（先测试环境，再生产环境）
   for node in $(kubectl get nodes -o name); do
     echo "Migrating $node"
     
     # Drain 节点
     kubectl drain $node --ignore-daemonsets --delete-emptydir-data
     
     # SSH 到节点修改配置
     node_ip=$(kubectl get $node -o jsonpath='{.status.addresses[?(@.type=="InternalIP")].address}')
     
     ssh $node_ip << 'EOF'
       # 修改 kubelet 配置
       sed -i 's/cgroupDriver: cgroupfs/cgroupDriver: systemd/' /var/lib/kubelet/config.yaml
       
       # 修改 containerd 配置
       sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
       
       # 重启服务
       systemctl restart containerd
       systemctl restart kubelet
   EOF
     
     # Uncordon 节点
     kubectl uncordon $node
     
     # 等待节点 Ready
     kubectl wait --for=condition=Ready $node --timeout=120s
     
     # 等待 5 分钟观察稳定性
     sleep 300
   done
   ```

2. **自动化配置验证**：
   ```yaml
   # 使用 DaemonSet 部署配置检查器
   apiVersion: apps/v1
   kind: DaemonSet
   metadata:
     name: cgroup-checker
     namespace: kube-system
   spec:
     selector:
       matchLabels:
         app: cgroup-checker
     template:
       metadata:
         labels:
           app: cgroup-checker
       spec:
         hostPID: true
         hostNetwork: true
         containers:
         - name: checker
           image: busybox
           command:
           - /bin/sh
           - -c
           - |
             while true; do
               kubelet_driver=$(grep cgroupDriver /host/var/lib/kubelet/config.yaml | awk '{print $2}')
               containerd_driver=$(grep SystemdCgroup /host/etc/containerd/config.toml | awk '{print $3}')
               
               if [ "$kubelet_driver" = "systemd" ] && [ "$containerd_driver" = "true" ]; then
                 echo "✅ cgroup 驱动一致: systemd"
               elif [ "$kubelet_driver" = "cgroupfs" ] && [ "$containerd_driver" = "false" ]; then
                 echo "✅ cgroup 驱动一致: cgroupfs"
               else
                 echo "❌ cgroup 驱动不一致！kubelet: $kubelet_driver, containerd: $containerd_driver"
                 # 触发告警（发送到监控系统）
               fi
               
               sleep 60
             done
           volumeMounts:
           - name: host-var
             mountPath: /host/var
           - name: host-etc
             mountPath: /host/etc
         volumes:
         - name: host-var
           hostPath:
             path: /var
         - name: host-etc
           hostPath:
             path: /etc
   ```

3. **文档化配置标准**：
   ```markdown
   # Kubernetes 节点配置标准 v1.0
   
   ## cgroup 驱动配置
   
   ### kubelet 配置（/var/lib/kubelet/config.yaml）
   ```yaml
   apiVersion: kubelet.config.k8s.io/v1beta1
   kind: KubeletConfiguration
   cgroupDriver: systemd  # ✅ 必须配置为 systemd
   ```
   
   ### containerd 配置（/etc/containerd/config.toml）
   ```toml
   [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
     SystemdCgroup = true  # ✅ 必须配置为 true
   ```
   
   ### 验证命令
   ```bash
   # kubelet
   grep cgroupDriver /var/lib/kubelet/config.yaml
   # 预期输出: cgroupDriver: systemd
   
   # containerd
   grep SystemdCgroup /etc/containerd/config.toml
   # 预期输出: SystemdCgroup = true
   ```
 

4. **监控告警**：
   ```yaml
   # Prometheus 告警规则
   - alert: CgroupDriverMismatch
     expr: kubelet_cgroup_manager_duration_seconds_count{cgroup_driver="cgroupfs"} > 0
       and
       container_runtime_cgroup_manager_duration_seconds_count{cgroup_driver="systemd"} > 0
     for: 5m
     labels:
       severity: critical
     annotations:
       summary: "cgroup 驱动不一致"
       description: "节点 {{ $labels.node }} kubelet 和 containerd cgroup 驱动不一致，可能导致 Pod 创建失败"
   ```

#### 💡 经验总结
- **配置管理混乱**：升级时未统一配置标准，新老节点配置不一致
- **测试不足**：未在测试环境验证新配置的兼容性
- **文档缺失**：缺少节点配置标准文档，运维人员配置错误
- **改进方向**：配置标准化、自动化验证、全集群统一迁移、监控告警


## Related

- [[domain-19-landscape-references/topic-index/pod-index|Pod 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/scheduler-index|Scheduler 调度与弹性伸缩知识图谱索引]]
