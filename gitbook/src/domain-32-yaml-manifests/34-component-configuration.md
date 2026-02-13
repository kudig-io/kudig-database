# 34. Kubernetes 组件配置（Component Configuration）

> **适用版本**：Kubernetes v1.25 – v1.32 | **文档日期**：2026-02

---

## 1. 概述

### 1.1 组件配置简介

从 Kubernetes v1.10 开始，控制平面和节点组件逐步支持通过**类型化配置文件**（Typed Configuration Files）进行配置，而不是仅依赖命令行标志（CLI flags）。这种方式提供了以下优势:

- **结构化和类型安全**：配置文件有明确的 API schema，可验证字段类型和约束
- **版本化**：配置 API 遵循 Kubernetes API 版本控制原则，支持平滑升级
- **易于管理**：配置文件可以纳入版本控制，便于审计和自动化部署
- **减少命令行复杂度**：避免冗长的启动参数

### 1.2 主要组件配置类型

Kubernetes 目前提供以下稳定或 beta 的组件配置 API:

| 组件 | API Kind | API Group | 稳定版本 |
|------|----------|-----------|----------|
| **Kubelet** | KubeletConfiguration | kubelet.config.k8s.io | v1beta1 (v1.10+) |
| **Kube-Proxy** | KubeProxyConfiguration | kubeproxy.config.k8s.io | v1alpha1 (v1.10+) |
| **Kube-Scheduler** | KubeSchedulerConfiguration | kubescheduler.config.k8s.io | v1 (v1.25+) |
| **Kube-Controller-Manager** | - | - | ❌ 暂无稳定 API |

**注意**：
- **KubeControllerManagerConfiguration** 目前仍为内部 API（`kubecontrollermanager.config.k8s.io/v1alpha1`），尚未稳定，不建议在生产环境使用
- 配置文件通过 `--config` 参数加载，例如：`kubelet --config=/etc/kubernetes/kubelet-config.yaml`

### 1.3 配置加载优先级

组件配置的加载遵循以下优先级（从高到低）:

1. **命令行标志**（CLI flags）：显式指定的参数优先级最高
2. **配置文件**（Config file）：通过 `--config` 指定的 YAML 文件
3. **默认值**（Defaults）：组件内置的默认配置

**最佳实践**：
- 在配置文件中统一管理所有配置项
- 仅在必要时（如临时调试）使用 CLI flags 覆盖
- 避免混合使用配置文件和大量 CLI flags

---

## 2. KubeletConfiguration 完整参考

### 2.1 API 信息

**KubeletConfiguration** 是 Kubelet 组件的配置 API，用于控制节点上容器运行时管理、资源管理、驱逐策略、CPU/内存拓扑管理等核心行为。

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
```

**版本历史**：
- `v1beta1`：Kubernetes v1.10+ 引入，目前最稳定版本
- `v1alpha1`：已废弃
- `v1`：计划在未来版本升级至 GA

**配置文件加载**：
```bash
kubelet --config=/etc/kubernetes/kubelet-config.yaml
```

**从 v1.28 开始**，Kubelet 支持 **Drop-In 配置目录**：
```bash
kubelet --config=/etc/kubernetes/kubelet.yaml \
        --config-dir=/etc/kubernetes/kubelet.conf.d/
```
Drop-in 目录中的配置片段会按文件名字母顺序合并，后加载的字段会覆盖先前的值。

---

### 2.2 完整字段规范

下面是 KubeletConfiguration v1beta1 的**完整字段详解**，涵盖所有重要配置项。

---

#### 2.2.1 容器运行时配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **cgroupDriver** | string | `cgroupfs` | Cgroup 驱动类型：`cgroupfs` 或 `systemd`<br>- 必须与容器运行时一致（containerd/CRI-O 推荐 `systemd`） |
| **containerRuntimeEndpoint** | string | `unix:///var/run/containerd/containerd.sock` | CRI 运行时的 Unix socket 路径<br>- containerd: `/var/run/containerd/containerd.sock`<br>- CRI-O: `/var/run/crio/crio.sock` |
| **podLogsDir** | string | `/var/log/pods` | Pod 容器日志存储根目录 |
| **serializeImagePulls** | bool | `true` | 是否串行拉取镜像<br>- `true`: 避免并发拉取导致的网络/磁盘竞争<br>- `false`: 允许并行拉取（需确保存储和网络性能足够） |
| **maxParallelImagePulls** | *int | `nil` | **v1.27+** 最大并行镜像拉取数<br>- `nil`: 无限制（当 `serializeImagePulls=false`）<br>- `> 0`: 限制并发数（推荐 3-5） |

---

#### 2.2.2 DNS 和网络配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **clusterDNS** | []string | `[]` | 集群 DNS 服务的 ClusterIP 列表<br>- 例如：`["10.96.0.10"]`（CoreDNS 的 Service IP）<br>- Pod 的 `/etc/resolv.conf` 会自动配置为该 DNS |
| **clusterDomain** | string | `cluster.local` | 集群内部的 DNS 域名<br>- Service 的 FQDN 格式：`<service>.<namespace>.svc.cluster.local` |
| **resolvConf** | string | `/etc/resolv.conf` | 宿主机 DNS 配置文件路径<br>- Pod 继承该文件的 nameserver（如果没有 `clusterDNS`） |

---

#### 2.2.3 Pod 资源限制

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **maxPods** | int32 | `110` | 节点最大可运行 Pod 数量<br>- 受 CNI 插件 IP 地址池大小限制<br>- 云厂商默认值：阿里云 ECS 按规格有不同限制（如 ecs.g6.large = 29） |
| **podPidsLimit** | int64 | `-1` | 单个 Pod 的最大进程数（PID）<br>- `-1`: 不限制<br>- 推荐值：`1024` ~ `4096`（防止 fork bomb） |
| **maxOpenFiles** | int64 | `1000000` | Kubelet 进程的最大文件描述符数<br>- 影响可同时打开的容器日志、socket 连接数 |

---

#### 2.2.4 驱逐策略配置

驱逐策略用于在节点资源不足时主动终止 Pod，保护节点稳定性。

##### evictionHard（硬驱逐阈值）

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **evictionHard** | map[string]string | 见下方 | 硬驱逐阈值，达到后**立即终止 Pod**（无 grace period）<br>默认值：<br>- `memory.available`: `"100Mi"`<br>- `nodefs.available`: `"10%"`<br>- `nodefs.inodesFree`: `"5%"`<br>- `imagefs.available`: `"15%"` |

**资源信号说明**：
- `memory.available`：可用内存 = `MemAvailable`（Linux）
- `nodefs.available`：根文件系统可用空间
- `nodefs.inodesFree`：根文件系统可用 inode 百分比
- `imagefs.available`：镜像文件系统可用空间（如 `/var/lib/containerd`）
- `pid.available`：可用 PID 数量（v1.20+）

##### evictionSoft（软驱逐阈值）

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **evictionSoft** | map[string]string | `{}` | 软驱逐阈值，达到后在 grace period 内观察，持续超出则驱逐<br>例如：`memory.available: "200Mi"` |
| **evictionSoftGracePeriod** | map[string]string | `{}` | 软驱逐的观察时间窗口<br>例如：`memory.available: "1m30s"`（资源不足持续 90 秒后触发） |
| **evictionMinimumReclaim** | map[string]string | `{}` | 驱逐后最小回收量，避免频繁驱逐<br>例如：`memory.available: "500Mi"`（驱逐后至少回收 500Mi） |
| **evictionPressureTransitionPeriod** | duration | `5m` | 驱逐压力状态切换的稳定时间<br>- 资源恢复后需等待 5 分钟才解除 NodePressure condition |
| **evictionMaxPodGracePeriod** | int32 | `0` | 驱逐 Pod 时的最大优雅终止时间（秒）<br>- `0`: 使用 Pod 自身的 `terminationGracePeriodSeconds`<br>- `> 0`: 强制限制上限（避免 Pod 设置过长的 grace period） |

---

#### 2.2.5 节点资源预留

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **systemReserved** | map[string]string | `{}` | 系统守护进程预留资源（systemd、sshd、journald 等）<br>例如：`cpu: "100m", memory: "200Mi", ephemeral-storage: "1Gi"` |
| **kubeReserved** | map[string]string | `{}` | Kubernetes 组件预留资源（kubelet、container-runtime）<br>例如：`cpu: "100m", memory: "200Mi", pid: "1000"` |
| **enforceNodeAllocatable** | []string | `["pods"]` | 强制执行资源限制的 cgroup 范围：<br>- `pods`：对所有 Pod 的总和进行限制（推荐）<br>- `system-reserved`：对系统预留资源强制限制<br>- `kube-reserved`：对 Kubernetes 组件强制限制<br>⚠️ 启用 `system-reserved`/`kube-reserved` 需正确配置 cgroup 路径 |
| **systemReservedCgroup** | string | `""` | 系统预留资源的 cgroup 路径<br>- systemd cgroup driver: `/system.slice`<br>- cgroupfs driver: `/system` |
| **kubeReservedCgroup** | string | `""` | Kubernetes 组件预留资源的 cgroup 路径<br>- systemd: `/kubelet.slice`<br>- cgroupfs: `/kubelet` |

**Allocatable 计算公式**：
```
Allocatable[资源] = Capacity[资源]
                  - systemReserved[资源]
                  - kubeReserved[资源]
                  - evictionHard[资源]
```

**示例计算**（8 核 16GB 节点）：
```
CPU Allocatable    = 8000m - 100m (system) - 100m (kube) = 7800m
Memory Allocatable = 16Gi - 200Mi (system) - 200Mi (kube) - 100Mi (eviction) = 15.5Gi
```

---

#### 2.2.6 镜像垃圾回收

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **imageGCHighThresholdPercent** | int32 | `85` | 镜像磁盘使用率高水位线（%）<br>- 超过该值时触发镜像 GC，删除最久未使用的镜像 |
| **imageGCLowThresholdPercent** | int32 | `80` | 镜像磁盘使用率低水位线（%）<br>- GC 会持续删除镜像直到磁盘使用率降至该值 |
| **imageMinimumGCAge** | duration | `2m` | 镜像的最小存活时间<br>- 创建不到 2 分钟的镜像不会被 GC（避免删除刚拉取的镜像） |

**GC 触发条件**：
- 磁盘使用率 ≥ 85%：开始删除镜像
- 删除顺序：按镜像的 `lastUsedTime` 从旧到新
- 停止条件：磁盘使用率 ≤ 80%

---

#### 2.2.7 CPU 管理策略

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **cpuManagerPolicy** | string | `none` | CPU 管理策略：<br>- `none`：默认，共享 CPU 池，无亲和性保证<br>- `static`：为 Guaranteed Pod 分配独占 CPU 核心（提升延迟敏感应用性能） |
| **cpuManagerReconcilePeriod** | duration | `10s` | CPU 分配状态校验周期 |
| **reservedSystemCPUs** | string | `""` | 系统预留的 CPU 核心列表（逗号分隔或范围）<br>例如：`"0,1"` 或 `"0-1"`<br>- 这些核心不会分配给 Pod（即使 `static` 策略） |

**`static` 策略要求**：
1. Pod 的 QoS 类型必须是 **Guaranteed**（CPU/Memory requests = limits）
2. Pod 的 CPU requests 必须是**整数核心**（如 `2` 或 `4`，不能是 `1.5`）
3. 满足条件的 Pod 会独占整数个 CPU 核心，避免上下文切换

**示例**：
```yaml
cpuManagerPolicy: static
reservedSystemCPUs: "0-1"  # 预留前 2 个核心给系统
# 8 核节点：核心 0-1 给系统，核心 2-7 可分配给 Guaranteed Pod
```

---

#### 2.2.8 内存管理策略

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **memoryManagerPolicy** | string | `None` | 内存管理策略（**v1.22+ Beta**）：<br>- `None`：默认，无 NUMA 内存亲和性<br>- `Static`：为 Guaranteed Pod 分配 NUMA 本地内存（减少跨 NUMA 访问延迟） |
| **reservedMemory** | []MemoryReservation | `[]` | 每个 NUMA 节点的预留内存（**v1.23+ Beta**）<br>例如：<br>```yaml<br>- numaNode: 0<br>  limits:<br>    memory: "1Gi"<br>``` |

**`Static` 策略要求**：
1. Pod 必须是 **Guaranteed** QoS
2. CPU Manager 必须启用 `static` 策略
3. 仅在多 NUMA 节点的服务器上有效（如双路 CPU 服务器）

**适用场景**：
- 高性能计算（HPC）
- 延迟敏感的数据库（如 Redis、MySQL）
- AI/ML 训练任务

---

#### 2.2.9 拓扑管理器

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **topologyManagerPolicy** | string | `none` | 拓扑管理策略（**v1.18+ Beta**）：<br>- `none`：无拓扑对齐<br>- `best-effort`：尽量对齐 CPU/内存/设备到同一 NUMA 节点<br>- `restricted`：强制对齐，失败则 Pod Pending<br>- `single-numa-node`：强制所有资源在单个 NUMA 节点 |
| **topologyManagerScope** | string | `container` | 拓扑对齐粒度（**v1.27+ Beta**）：<br>- `container`：每个容器独立对齐<br>- `pod`：整个 Pod 的所有容器对齐到同一 NUMA 节点 |

**策略对比**：

| 策略 | 能否跨 NUMA | Pod 失败行为 | 适用场景 |
|------|------------|-------------|----------|
| `none` | ✅ 允许 | 不失败 | 通用工作负载 |
| `best-effort` | ✅ 允许 | 不失败 | 希望优化性能，但不强制 |
| `restricted` | ❌ 禁止 | 无法对齐则 Pending | 性能敏感，可接受调度失败 |
| `single-numa-node` | ❌ 禁止 | 单 NUMA 无法满足则 Pending | HPC、GPU 工作负载 |

**示例**（AI 训练节点）：
```yaml
topologyManagerPolicy: single-numa-node  # GPU、CPU、内存必须在同一 NUMA
topologyManagerScope: pod                # 整个训练 Pod 对齐
```

---

#### 2.2.10 特性门控

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **featureGates** | map[string]bool | `{}` | 启用/禁用实验性特性<br>例如：<br>```yaml<br>featureGates:<br>  GracefulNodeShutdown: true<br>  MemoryQoS: false<br>``` |

**常用 Feature Gates**（v1.25-v1.32）：

| Feature | 默认值 | 阶段 | 说明 |
|---------|--------|------|------|
| `GracefulNodeShutdown` | `true` | Beta (v1.21+) | 节点关机时优雅终止 Pod |
| `GracefulNodeShutdownBasedOnPodPriority` | `true` | Beta (v1.24+) | 按 Pod 优先级顺序终止 |
| `MemoryQoS` | `false` | Alpha (v1.22+) | 基于 cgroup v2 的内存 QoS |
| `CPUManagerPolicyBetaOptions` | `false` | Beta (v1.23+) | CPU Manager 高级选项 |
| `MemoryManager` | `true` | Beta (v1.22+) | NUMA 内存管理器 |
| `TopologyManager` | `true` | Beta (v1.18+) | NUMA 拓扑管理器 |
| `KubeletTracing` | `true` | Beta (v1.27+) | Kubelet OpenTelemetry 追踪 |
| `PodReadyToStartContainersCondition` | `true` | Beta (v1.29+) | Pod Ready 状态细化 |

---

#### 2.2.11 日志配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **logging.format** | string | `text` | 日志格式：<br>- `text`：传统文本格式<br>- `json`：结构化 JSON（便于日志采集和分析） |
| **logging.verbosity** | int | `2` | 日志详细级别（0-10）：<br>- `0`：仅 ERROR<br>- `2`：INFO（推荐）<br>- `4`：DEBUG<br>- `6+`：TRACE（包含请求详情） |

**生产环境推荐**：
```yaml
logging:
  format: json        # 结构化日志，便于 ELK/Loki 采集
  verbosity: 2        # 平衡信息量和性能
```

---

#### 2.2.12 健康检查和端口

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **healthzBindAddress** | string | `127.0.0.1` | Healthz 服务器监听地址 |
| **healthzPort** | int32 | `10248` | Healthz 服务器端口<br>- 探测端点：`http://<IP>:10248/healthz` |
| **readOnlyPort** | int32 | `0` | 只读 API 端口（⚠️ **已废弃**）<br>- `0`：禁用（推荐，避免安全风险）<br>- `10255`：旧版默认端口（无认证，不应使用） |

---

#### 2.2.13 认证和授权

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **authentication.anonymous.enabled** | bool | `false` | 是否允许匿名访问 Kubelet API |
| **authentication.webhook.enabled** | bool | `true` | 启用 Webhook 认证（通过 API Server TokenReview） |
| **authentication.webhook.cacheTTL** | duration | `2m` | Webhook 认证结果缓存时间 |
| **authentication.x509.clientCAFile** | string | `""` | 客户端证书 CA 文件路径（用于双向 TLS） |
| **authorization.mode** | string | `Webhook` | 授权模式：<br>- `Webhook`：通过 API Server SubjectAccessReview 验证权限<br>- `AlwaysAllow`：允许所有请求（⚠️ 仅用于测试） |
| **authorization.webhook.cacheAuthorizedTTL** | duration | `5m` | 授权成功结果缓存时间 |
| **authorization.webhook.cacheUnauthorizedTTL** | duration | `30s` | 授权失败结果缓存时间 |

**推荐配置**（生产环境）：
```yaml
authentication:
  anonymous:
    enabled: false     # 禁止匿名访问
  webhook:
    enabled: true      # 通过 API Server 验证身份
  x509:
    clientCAFile: /etc/kubernetes/pki/ca.crt
authorization:
  mode: Webhook        # 通过 RBAC 验证权限
```

---

#### 2.2.14 证书轮换

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **rotateCertificates** | bool | `true` | 自动轮换客户端证书（kubelet → API Server）<br>- 证书过期前自动续期 |
| **serverTLSBootstrap** | bool | `false` | 启用 Server TLS Bootstrap（**v1.20+ Beta**）<br>- Kubelet 服务端证书通过 CSR 自动签发和轮换 |

**证书轮换流程**：
1. Kubelet 启动时使用 Bootstrap Token 获取初始客户端证书
2. 证书过期前，Kubelet 自动创建 CertificateSigningRequest（CSR）
3. Controller Manager 自动批准并签发新证书
4. Kubelet 加载新证书，无需重启

**启用 Server TLS Bootstrap**：
```yaml
serverTLSBootstrap: true
# 需在 API Server 启用 --kubelet-certificate-authority
```

---

#### 2.2.15 优雅关机

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **shutdownGracePeriod** | duration | `0s` | 节点关机时的总优雅终止时间<br>- 需启用 `GracefulNodeShutdown` Feature Gate |
| **shutdownGracePeriodCriticalPods** | duration | `0s` | 关键 Pod 的额外优雅终止时间<br>- 关键 Pod（PriorityClass ≥ 2000000000）会最后终止 |

**示例**：
```yaml
shutdownGracePeriod: 30s               # 总共 30 秒
shutdownGracePeriodCriticalPods: 10s   # 最后 10 秒留给关键 Pod
# 时间分配：
# 0-20s: 终止普通 Pod（优先级从低到高）
# 20-30s: 终止关键 Pod（如监控 agent、日志 collector）
```

---

#### 2.2.16 系统安全配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **protectKernelDefaults** | bool | `false` | 验证内核参数是否符合 Kubernetes 要求<br>- `true`: 启动时检查内核参数（如 `vm.overcommit_memory`），不符合则退出<br>- 用于确保节点配置的一致性 |
| **makeIPTablesUtilChains** | bool | `true` | 自动创建 iptables 实用链（如 `KUBE-MARK-MASQ`）<br>- 用于 Service、NetworkPolicy 等网络规则 |

**推荐内核参数**（启用 `protectKernelDefaults: true` 时需配置）：
```bash
# /etc/sysctl.d/99-kubernetes.conf
vm.overcommit_memory=1           # 允许内存超分配
kernel.panic=10                  # Panic 10 秒后重启
kernel.panic_on_oops=1           # Oops 时触发 panic
vm.swappiness=0                  # 禁用 swap（或设为 1）
net.ipv4.ip_forward=1            # 启用 IP 转发
net.bridge.bridge-nf-call-iptables=1
net.bridge.bridge-nf-call-ip6tables=1
```

---

#### 2.2.17 节点状态上报

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **nodeStatusReportFrequency** | duration | `5m` | 节点状态上报频率（当状态未变化时）<br>- Kubelet 定期向 API Server 上报节点状态（CPU/Memory/Conditions） |
| **nodeStatusUpdateFrequency** | duration | `10s` | 节点状态更新检测频率<br>- Kubelet 每 10 秒检测节点状态变化，变化后立即上报 |

**上报逻辑**：
- 状态变化（如 MemoryPressure）：立即上报
- 状态不变：每 5 分钟上报一次（心跳）

---

#### 2.2.18 容器日志管理

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **containerLogMaxSize** | string | `10Mi` | 单个容器日志文件的最大大小<br>- 超过后自动轮转（rotate） |
| **containerLogMaxFiles** | int32 | `5` | 单个容器保留的最大日志文件数<br>- 轮转后保留最近 5 个文件 |

**日志存储路径**：
```
/var/log/pods/<namespace>_<pod-name>_<pod-uid>/<container-name>/
├── 0.log        # 当前日志
├── 1.log.gz     # 轮转日志 1
├── 2.log.gz     # 轮转日志 2
...
└── 4.log.gz     # 轮转日志 4（最老）
```

**磁盘使用计算**：
```
单容器最大日志 = containerLogMaxSize × containerLogMaxFiles
               = 10Mi × 5 = 50Mi
```

---

#### 2.2.19 节点注册

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **registerNode** | bool | `true` | 是否自动向 API Server 注册节点<br>- `false`：手动注册（不推荐） |
| **registerWithTaints** | []Taint | `[]` | 注册节点时添加的 Taints<br>例如：<br>```yaml<br>- key: "node-role.kubernetes.io/control-plane"<br>  effect: "NoSchedule"<br>``` |

**使用场景**：
- 新节点启动时自动打上污点，防止调度非关键 Pod
- 节点初始化完成后再手动去除污点

---

### 2.3 最小化示例

以下是一个**最小可用**的 KubeletConfiguration，适用于测试环境：

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 容器运行时
cgroupDriver: systemd
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock

# DNS 配置
clusterDNS:
  - 10.96.0.10
clusterDomain: cluster.local

# 基本限制
maxPods: 110

# 认证授权
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
authorization:
  mode: Webhook

# 证书轮换
rotateCertificates: true
```

**配置说明**：
- 使用 systemd cgroup 驱动（推荐）
- 启用 Webhook 认证/授权（最佳实践）
- 允许最多 110 个 Pod（默认值）
- 启用客户端证书自动轮换

---

### 2.4 生产级示例

以下是一个**生产级**的 KubeletConfiguration，包含资源预留、驱逐策略、CPU/内存管理等完整配置：

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# ========================================
# 容器运行时配置
# ========================================
cgroupDriver: systemd  # 使用 systemd cgroup 驱动（推荐）
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock
podLogsDir: /var/log/pods

# 镜像拉取策略
serializeImagePulls: true  # 串行拉取，避免网络/磁盘竞争
maxParallelImagePulls: 3   # v1.27+ 允许 3 个并行拉取

# ========================================
# DNS 和网络配置
# ========================================
clusterDNS:
  - 10.96.0.10  # CoreDNS Service ClusterIP
clusterDomain: cluster.local
resolvConf: /etc/resolv.conf

# ========================================
# Pod 资源限制
# ========================================
maxPods: 110          # 节点最大 Pod 数（根据网络插件 IP 池调整）
podPidsLimit: 4096    # 单个 Pod 最大进程数，防止 fork bomb
maxOpenFiles: 1000000 # Kubelet 最大文件描述符

# ========================================
# 驱逐策略（保护节点稳定性）
# ========================================
# 硬驱逐：达到阈值立即终止 Pod
evictionHard:
  memory.available: "100Mi"       # 可用内存 < 100Mi
  nodefs.available: "10%"         # 根文件系统可用空间 < 10%
  nodefs.inodesFree: "5%"         # inode 可用 < 5%
  imagefs.available: "15%"        # 镜像文件系统可用空间 < 15%
  pid.available: "5%"             # 可用 PID < 5%

# 软驱逐：达到阈值后观察一段时间再驱逐
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
  imagefs.available: "20%"

# 软驱逐的观察时间窗口
evictionSoftGracePeriod:
  memory.available: "1m30s"  # 内存不足持续 90 秒后触发
  nodefs.available: "2m"
  imagefs.available: "2m"

# 驱逐后最小回收量，避免频繁驱逐
evictionMinimumReclaim:
  memory.available: "500Mi"
  nodefs.available: "1Gi"
  imagefs.available: "2Gi"

# 压力状态切换稳定时间
evictionPressureTransitionPeriod: 5m

# 驱逐 Pod 时的最大优雅终止时间（秒）
evictionMaxPodGracePeriod: 30

# ========================================
# 节点资源预留（计算 Allocatable）
# ========================================
# 系统守护进程预留（systemd、sshd 等）
systemReserved:
  cpu: "100m"
  memory: "200Mi"
  ephemeral-storage: "1Gi"
  pid: "500"

# Kubernetes 组件预留（kubelet、containerd）
kubeReserved:
  cpu: "100m"
  memory: "200Mi"
  ephemeral-storage: "1Gi"
  pid: "500"

# 强制执行资源限制
enforceNodeAllocatable:
  - pods  # 对所有 Pod 的资源总和进行限制（推荐）
  # - system-reserved  # 对系统预留强制限制（需配置 cgroup）
  # - kube-reserved    # 对 Kubernetes 组件强制限制（需配置 cgroup）

# Cgroup 路径配置（使用 systemd 驱动时）
systemReservedCgroup: /system.slice
kubeReservedCgroup: /podruntime.slice

# ========================================
# 镜像垃圾回收
# ========================================
imageGCHighThresholdPercent: 85  # 磁盘使用率 ≥ 85% 触发 GC
imageGCLowThresholdPercent: 80   # GC 持续到磁盘使用率 ≤ 80%
imageMinimumGCAge: 2m            # 镜像至少存活 2 分钟才能被 GC

# ========================================
# CPU 管理策略（性能优化）
# ========================================
cpuManagerPolicy: static         # 为 Guaranteed Pod 分配独占 CPU
cpuManagerReconcilePeriod: 10s
reservedSystemCPUs: "0-1"        # 核心 0-1 预留给系统（8 核节点示例）

# ========================================
# 内存管理策略（NUMA 优化）
# ========================================
memoryManagerPolicy: Static      # 为 Guaranteed Pod 分配 NUMA 本地内存
reservedMemory:
  - numaNode: 0
    limits:
      memory: "1Gi"  # NUMA 节点 0 预留 1Gi
  - numaNode: 1
    limits:
      memory: "1Gi"  # NUMA 节点 1 预留 1Gi

# ========================================
# 拓扑管理器（NUMA 对齐）
# ========================================
topologyManagerPolicy: best-effort  # 尽量对齐 CPU/内存/设备到同一 NUMA
topologyManagerScope: pod           # 整个 Pod 对齐到同一 NUMA 节点

# ========================================
# 特性门控
# ========================================
featureGates:
  GracefulNodeShutdown: true                           # 优雅关机
  GracefulNodeShutdownBasedOnPodPriority: true         # 按优先级关机
  MemoryQoS: false                                     # 内存 QoS（需 cgroup v2）
  KubeletTracing: true                                 # OpenTelemetry 追踪
  PodReadyToStartContainersCondition: true             # 细化 Pod Ready 状态

# ========================================
# 日志配置
# ========================================
logging:
  format: json      # 结构化 JSON 日志，便于采集
  verbosity: 2      # INFO 级别

# ========================================
# 健康检查和端口
# ========================================
healthzBindAddress: 127.0.0.1  # 仅本地访问
healthzPort: 10248
readOnlyPort: 0                # 禁用只读 API（安全最佳实践）

# ========================================
# 认证和授权
# ========================================
authentication:
  anonymous:
    enabled: false  # 禁止匿名访问
  webhook:
    enabled: true   # 通过 API Server 验证身份
    cacheTTL: 2m
  x509:
    clientCAFile: /etc/kubernetes/pki/ca.crt

authorization:
  mode: Webhook  # 通过 RBAC 验证权限
  webhook:
    cacheAuthorizedTTL: 5m
    cacheUnauthorizedTTL: 30s

# ========================================
# 证书轮换
# ========================================
rotateCertificates: true        # 自动轮换客户端证书
serverTLSBootstrap: true        # 启用服务端证书自动签发（v1.20+）

# ========================================
# 优雅关机
# ========================================
shutdownGracePeriod: 30s                 # 总共 30 秒优雅终止
shutdownGracePeriodCriticalPods: 10s     # 最后 10 秒留给关键 Pod

# ========================================
# 系统安全配置
# ========================================
protectKernelDefaults: true     # 验证内核参数（需配置 sysctl）
makeIPTablesUtilChains: true    # 自动创建 iptables 规则链

# ========================================
# 节点状态上报
# ========================================
nodeStatusReportFrequency: 5m   # 状态未变化时每 5 分钟上报
nodeStatusUpdateFrequency: 10s  # 每 10 秒检测状态变化

# ========================================
# 容器日志管理
# ========================================
containerLogMaxSize: 10Mi       # 单个日志文件最大 10Mi
containerLogMaxFiles: 5         # 保留最近 5 个日志文件

# ========================================
# 节点注册
# ========================================
registerNode: true
# 注册时不添加 taints（按需配置）
# registerWithTaints:
#   - key: "node.kubernetes.io/uninitialized"
#     effect: "NoSchedule"
```

**配置亮点**：
- **完整的资源预留和驱逐策略**：保护节点在高负载下的稳定性
- **CPU/内存拓扑管理**：为延迟敏感应用（数据库、AI 训练）提供 NUMA 优化
- **镜像 GC 和日志轮转**：自动清理，防止磁盘满
- **优雅关机和证书轮转**：提升可靠性和安全性
- **结构化 JSON 日志**：便于 ELK/Loki 等日志系统采集

---

### 2.5 内部机制

#### 2.5.1 配置文件加载

**单配置文件**（v1.10+）：
```bash
kubelet --config=/etc/kubernetes/kubelet-config.yaml
```

**Drop-In 配置目录**（v1.28+）：
```bash
kubelet --config=/etc/kubernetes/kubelet.yaml \
        --config-dir=/etc/kubernetes/kubelet.conf.d/
```

Drop-in 目录工作原理：
1. 加载 `--config` 指定的主配置文件
2. 按文件名字母顺序加载 `--config-dir` 中的 `.yaml` 文件
3. 后加载的字段会**覆盖**先前的值（浅合并，非深度合并）

**示例**：
```bash
# 主配置文件
/etc/kubernetes/kubelet.yaml  # maxPods: 110

# Drop-in 配置
/etc/kubernetes/kubelet.conf.d/10-gpu.yaml    # featureGates: {GPUManager: true}
/etc/kubernetes/kubelet.conf.d/20-limits.yaml # maxPods: 200  # 覆盖主配置
```

最终效果：`maxPods: 200`（被 20-limits.yaml 覆盖）

---

#### 2.5.2 CLI Flags vs Config File 优先级

**优先级**（从高到低）：
1. **命令行标志**：显式指定的参数优先级最高
2. **配置文件**：通过 `--config` 加载的 YAML
3. **默认值**：Kubelet 内置默认值

**示例**：
```bash
kubelet --config=/etc/kubernetes/kubelet.yaml --max-pods=150
# 如果 kubelet.yaml 中 maxPods: 110
# 最终生效值：150（CLI flags 优先）
```

**推荐实践**：
- ✅ 在配置文件中集中管理所有配置
- ❌ 避免混合使用大量 CLI flags 和配置文件（难以维护）
- ⚠️ CLI flags 仅用于临时覆盖（如调试）

---

#### 2.5.3 Dynamic Kubelet Configuration（已废弃）

**历史**：
- **v1.10-v1.21**：支持通过 ConfigMap 动态更新 Kubelet 配置
- **v1.22**：标记为 deprecated
- **v1.24**：完全移除

**为何废弃**：
- 配置变更难以审计和回滚
- 导致节点配置不一致（不同节点可能使用不同版本的 ConfigMap）
- 推荐使用 **GitOps**（如 Ansible、SaltStack）统一管理节点配置

**替代方案**：
- 使用配置管理工具（Ansible/Puppet）推送配置文件
- 使用 Node Feature Discovery (NFD) 自动标记节点特性
- 使用 MachineConfig（OpenShift）或 KubeadmConfig（Cluster API）

---

#### 2.5.4 Node Allocatable 计算机制

**Allocatable 公式**：
```
Allocatable[资源] = Capacity[资源]
                  - systemReserved[资源]
                  - kubeReserved[资源]
                  - evictionHard[资源]
```

**字段含义**：
- **Capacity**：节点的物理资源总量（CPU 核心数、内存大小）
- **Allocatable**：可分配给 Pod 的资源量（Scheduler 调度时使用）
- **systemReserved**：系统守护进程预留（OS、sshd、systemd）
- **kubeReserved**：Kubernetes 组件预留（kubelet、containerd、kube-proxy）
- **evictionHard**：硬驱逐阈值（触发后立即终止 Pod）

**计算示例**（16 核 32GB 节点）：

| 资源 | Capacity | systemReserved | kubeReserved | evictionHard | Allocatable |
|------|----------|----------------|--------------|--------------|-------------|
| CPU | 16000m | 200m | 200m | - | **15600m** |
| Memory | 32Gi | 1Gi | 1Gi | 100Mi | **30Gi - 100Mi** |

**验证方法**：
```bash
kubectl describe node <node-name>

# 输出示例：
Capacity:
  cpu:                16
  memory:             33554432Ki  # 32Gi
Allocatable:
  cpu:                15600m
  memory:             31850Mi     # ≈ 31Gi（扣除预留和驱逐阈值）
```

**调度影响**：
- Scheduler 仅考虑 **Allocatable** 资源
- 如果 Pod 的 requests 总和超过 Allocatable，Pod 会 Pending
- Kubelet 会强制执行 Allocatable 限制（通过 cgroup）

**Cgroup 强制执行**：
当 `enforceNodeAllocatable: ["pods"]` 时，Kubelet 会创建以下 cgroup 层级：
```
/kubepods/  # 限制所有 Pod 的资源总和 ≤ Allocatable
  ├── burstable/
  │   └── pod<uid>/
  └── besteffort/
      └── pod<uid>/
```

**查看 cgroup 限制**：
```bash
# CPU 限制（CFS quota）
cat /sys/fs/cgroup/kubepods/cpu.cfs_quota_us

# 内存限制
cat /sys/fs/cgroup/kubepods/memory.limit_in_bytes
```

---

#### 2.5.5 配置更新最佳实践

**更新流程**：
1. 在配置管理系统（Ansible/GitLab）中修改 KubeletConfiguration
2. 使用自动化工具推送到节点（`/etc/kubernetes/kubelet-config.yaml`）
3. 重启 Kubelet：`systemctl restart kubelet`
4. 验证配置生效：`kubectl get node <name> -o yaml`

**滚动更新**：
- 逐个节点更新，避免一次性影响整个集群
- 先在测试节点验证配置，再推广到生产节点

**回滚机制**：
- 保留上一个版本的配置文件：`kubelet-config.yaml.bak`
- 快速回滚：`mv kubelet-config.yaml.bak kubelet-config.yaml && systemctl restart kubelet`

**监控配置变更**：
- 使用 ConfigMap/Git 记录配置历史
- 监控 Node Conditions（MemoryPressure、DiskPressure）
- 设置告警：Allocatable 资源变化超过阈值

---

**本节小结**：
- KubeletConfiguration 是 Kubelet 的核心配置文件，涵盖容器运行时、资源管理、驱逐策略、NUMA 优化等
- 生产环境应配置完整的资源预留（systemReserved/kubeReserved）和驱逐策略（evictionHard/Soft）
- 对于性能敏感应用，启用 CPU Manager（static）、Memory Manager（Static）和 Topology Manager
- 使用 Drop-In 配置目录（v1.28+）可实现模块化配置管理
- Node Allocatable 决定了 Pod 可调度的资源上限，需正确配置以避免节点资源耗尽

---

---

## 3. KubeProxyConfiguration 完整参考

### 3.1 API 信息

**KubeProxyConfiguration** 是 Kube-Proxy 组件的配置 API,用于控制 Kubernetes Service 的负载均衡和网络代理行为。

```yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
```

**版本历史**:
- `v1alpha1`: Kubernetes v1.10+ 引入,目前唯一可用版本
- ⚠️ 长期处于 Alpha 阶段,但实际上已非常稳定,广泛用于生产环境

**配置文件加载**:
```bash
kube-proxy --config=/etc/kubernetes/kube-proxy-config.yaml
```

---

### 3.2 完整字段规范

#### 3.2.1 代理模式 (mode)

| 模式 | 平台 | 说明 | 适用场景 |
|------|------|------|----------|
| **iptables** | Linux | 默认模式,基于 netfilter iptables 规则<br>- 使用 DNAT 实现 Service 负载均衡<br>- 随机选择后端 Pod<br>- 性能瓶颈:大量 Service 时规则链很长 | 小型集群 (< 1000 Services) |
| **ipvs** | Linux | 基于 Linux IPVS (IP Virtual Server)<br>- 支持多种负载均衡算法 (rr/lc/dh/sh/sed/nq)<br>- 性能优于 iptables (O(1) vs O(n))<br>- 需要加载内核模块: ip_vs, ip_vs_rr 等 | 大型集群,高性能要求 |
| **nftables** | Linux | **v1.29+ Alpha**,基于 nftables 替代 iptables<br>- 更现代的 netfilter 接口<br>- 性能和规则管理优于 iptables<br>- 仍在实验阶段 | 实验性,未来趋势 |
| **kernelspace** | Windows | Windows 平台的内核空间代理<br>- 使用 Windows HNS (Host Network Service)<br>- 性能优于早期的 userspace 模式 | Windows 节点 |

**模式选择**:
```yaml
mode: "ipvs"  # 推荐:大型集群使用 IPVS
```

---

#### 3.2.2 iptables 模式配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **masqueradeAll** | bool | `false` | 是否对所有出站流量进行 SNAT<br>- `true`: 所有 Pod 访问 Service 的源 IP 都被改为节点 IP<br>- `false`: 仅对非 Pod CIDR 的流量进行 SNAT (推荐) |
| **masqueradeBit** | *int32 | `14` | iptables fwmark 标记位 (用于 SNAT 标记)<br>- 默认 14,对应 fwmark 0x4000 |
| **minSyncPeriod** | duration | `0s` | iptables 规则最小同步间隔<br>- 避免频繁更新规则导致的性能抖动 |
| **syncPeriod** | duration | `30s` | iptables 规则全量同步周期<br>- 定期全量刷新规则,确保一致性 |
| **localhostNodePorts** | *bool | `true` (v1.24+) | 是否允许从本机 (127.0.0.1) 访问 NodePort<br>- `true`: 可通过 `localhost:<nodePort>` 访问<br>- `false`: 仅允许外部 IP 访问 |

**iptables 规则结构**:
```
KUBE-SERVICES (入口链)
  ├── KUBE-SVC-<hash>  (每个 Service 一条链)
  │   ├── KUBE-SEP-<hash1>  (Endpoint 1,概率 1/n)
  │   ├── KUBE-SEP-<hash2>  (Endpoint 2,概率 1/(n-1))
  │   └── KUBE-SEP-<hash3>  (Endpoint 3,概率 1)
  └── KUBE-NODEPORTS (NodePort 入口链)
```

**性能问题**:
- 1000+ Services 时,规则链可达数万条
- 每个数据包需遍历所有规则 (O(n) 复杂度)
- 大规模场景推荐使用 **IPVS 模式**

---

#### 3.2.3 IPVS 模式配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **scheduler** | string | `""` | IPVS 负载均衡算法:<br>- `rr`: Round-Robin (轮询,默认)<br>- `lc`: Least Connection (最少连接)<br>- `dh`: Destination Hashing (目标哈希)<br>- `sh`: Source Hashing (源哈希,会话保持)<br>- `sed`: Shortest Expected Delay (最短期望延迟)<br>- `nq`: Never Queue (无队列) |
| **strictARP** | bool | `false` | 严格 ARP 响应模式 (**MetalLB 必须开启**)<br>- `true`: 仅响应本机 IP 的 ARP 请求<br>- `false`: 响应所有 Service IP 的 ARP (可能导致 IP 冲突)<br>需配置内核参数:<br>`arp_ignore=1, arp_announce=2` |
| **minSyncPeriod** | duration | `0s` | IPVS 规则最小同步间隔 |
| **syncPeriod** | duration | `30s` | IPVS 规则全量同步周期 |
| **tcpTimeout** | duration | `0s` | TCP 连接空闲超时<br>- `0s`: 使用系统默认 (通常 15 分钟) |
| **tcpFinTimeout** | duration | `0s` | TCP FIN_WAIT 超时<br>- `0s`: 使用系统默认 (通常 120 秒) |
| **udpTimeout** | duration | `0s` | UDP 会话超时<br>- `0s`: 使用系统默认 (通常 300 秒) |
| **excludeCIDRs** | []string | `[]` | 排除的 CIDR 列表 (不创建 IPVS 规则)<br>例如: `["10.0.0.0/8"]` |

**IPVS 调度算法对比**:

| 算法 | 负载均衡策略 | 会话保持 | 适用场景 |
|------|-------------|---------|----------|
| **rr** | 轮询 | ❌ | 通用,无状态服务 |
| **lc** | 最少连接 | ❌ | 长连接服务 (如数据库) |
| **dh** | 目标 IP 哈希 | ✅ | 需要会话保持 (通过目标 IP) |
| **sh** | 源 IP 哈希 | ✅ | 需要会话保持 (通过客户端 IP) |
| **sed** | 最短延迟 | ❌ | 延迟敏感应用 |
| **nq** | 无队列 | ❌ | 低延迟场景 |

**IPVS 表结构**:
```bash
# 查看 IPVS 虚拟服务
ipvsadm -Ln

# 输出示例:
TCP  10.96.0.1:443 rr
  -> 10.244.0.2:6443    Masq  1  0  0
  -> 10.244.0.3:6443    Masq  1  0  0
```

**性能优势**:
- **O(1) 查找复杂度** (vs iptables 的 O(n))
- 10000+ Services 场景下延迟仅增加几微秒
- 内核级别负载均衡,吞吐量高

---

#### 3.2.4 nftables 模式配置 (v1.29+)

| 字段 | 类型 | 默认值 | 说明 |
|------|--------|------|------|
| **masqueradeAll** | bool | `false` | 是否对所有出站流量进行 SNAT |
| **minSyncPeriod** | duration | `0s` | nftables 规则最小同步间隔 |
| **syncPeriod** | duration | `30s` | nftables 规则全量同步周期 |

**nftables vs iptables**:
- **规则管理**: nftables 使用单一事务更新,避免 iptables 的竞态条件
- **性能**: nftables 使用更高效的数据结构,查找性能优于 iptables
- **表结构**: nftables 的 table/chain/rule 层级更灵活
- **成熟度**: ⚠️ **v1.29-v1.32 仍为 Alpha 阶段,不建议生产使用**

---

#### 3.2.5 Conntrack 配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **maxPerCore** | int32 | `32768` | 每 CPU 核心的最大 conntrack 条目数<br>- 总条目数 = maxPerCore × CPU 核心数<br>- 8 核节点: 32768 × 8 = 262144 |
| **min** | *int32 | `131072` | conntrack 表的最小大小<br>- 如果 maxPerCore × 核心数 < min,使用 min |
| **tcpEstablishedTimeout** | *duration | `24h` | TCP ESTABLISHED 状态超时<br>- 默认 24 小时 (86400 秒) |
| **tcpCloseWaitTimeout** | *duration | `1h` | TCP CLOSE_WAIT 状态超时<br>- 默认 1 小时 (3600 秒) |
| **udpTimeout** | duration | `30s` | UDP 连接超时 |
| **udpStreamTimeout** | duration | `180s` | UDP 流式连接超时 (双向流量) |

**Conntrack 表大小计算**:
```
实际大小 = max(min, maxPerCore × CPU 核心数)

示例 (16 核节点):
实际大小 = max(131072, 32768 × 16) = 524288
```

**Conntrack 耗尽问题**:
- **现象**: Service 连接失败,`nf_conntrack: table full, dropping packet`
- **原因**: 高并发短连接,conntrack 表满
- **解决**:
  1. 增大 `maxPerCore` (如 `65536`)
  2. 减小超时时间 (如 `tcpEstablishedTimeout: 1h`)
  3. 启用 conntrack 自动清理: `net.netfilter.nf_conntrack_tcp_timeout_time_wait=30`

**查看 conntrack 使用**:
```bash
# 当前条目数
cat /proc/sys/net/netfilter/nf_conntrack_count

# 最大容量
cat /proc/sys/net/netfilter/nf_conntrack_max

# 使用率
echo "scale=2; $(cat /proc/sys/net/netfilter/nf_conntrack_count) * 100 / $(cat /proc/sys/net/netfilter/nf_conntrack_max)" | bc
```

---

#### 3.2.6 通用配置

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **clusterCIDR** | string | `""` | 集群 Pod CIDR<br>- 用于判断是否需要 SNAT (Pod → Service) |
| **hostnameOverride** | string | `""` | 覆盖节点主机名<br>- 默认使用 `os.Hostname()` |
| **healthzBindAddress** | string | `0.0.0.0:10256` | Healthz 服务器监听地址<br>- 探测端点: `http://<IP>:10256/healthz` |
| **metricsBindAddress** | string | `127.0.0.1:10249` | Metrics 服务器监听地址<br>- Prometheus 抓取: `/metrics` |
| **nodePortAddresses** | []string | `[]` | NodePort 绑定的 IP 地址范围<br>- 默认: 所有接口<br>- 例如: `["10.0.0.0/8"]` (仅绑定内网 IP) |
| **detectLocalMode** | string | `""` | 本地流量检测模式:<br>- `ClusterCIDR`: 通过 clusterCIDR 判断<br>- `NodeCIDR`: 通过节点 PodCIDR 判断<br>- `BridgeInterface`: 通过网桥接口判断<br>- `InterfaceNamePrefix`: 通过接口名前缀判断 |
| **detectLocal** | LocalDetector | `nil` | 本地流量检测配置 (v1.30+)<br>- 替代 detectLocalMode,更灵活 |
| **featureGates** | map[string]bool | `{}` | 启用/禁用实验性特性 |
| **logging** | LoggingConfiguration | - | 日志配置 (格式、级别) |

**detectLocalMode 说明**:
- **用途**: 判断流量是否来自本节点 Pod,决定是否保留源 IP
- **默认行为**: 外部流量访问 NodePort 时,源 IP 被 SNAT 为节点 IP
- **开启本地流量检测**: 本节点 Pod 访问 Service 时保留源 IP (不 SNAT)

---

### 3.3 最小化示例 (iptables mode)

```yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration

# ========================================
# 代理模式
# ========================================
mode: "iptables"  # 默认模式,小型集群适用

# ========================================
# iptables 配置
# ========================================
iptables:
  masqueradeAll: false  # 仅对必要流量进行 SNAT
  syncPeriod: 30s       # 每 30 秒全量同步规则

# ========================================
# 集群网络
# ========================================
clusterCIDR: "10.244.0.0/16"  # Pod CIDR (与 CNI 一致)

# ========================================
# 健康检查和监控
# ========================================
healthzBindAddress: "0.0.0.0:10256"  # Healthz 端口
metricsBindAddress: "127.0.0.1:10249"  # Metrics 端口 (仅本地)
```

**适用场景**:
- 小型集群 (< 100 节点,< 1000 Services)
- 无特殊性能要求
- 使用传统 iptables 防火墙

---

### 3.4 生产级示例 (IPVS mode with optimized conntrack)

```yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration

# ========================================
# 代理模式
# ========================================
mode: "ipvs"  # 高性能 IPVS 模式 (推荐)

# ========================================
# IPVS 配置
# ========================================
ipvs:
  scheduler: "rr"         # 轮询算法 (round-robin)
                          # 其他选项: lc/dh/sh/sed/nq
  
  strictARP: true         # ⚠️ 启用严格 ARP (MetalLB 必需)
                          # 需配置内核参数:
                          # net.ipv4.conf.all.arp_ignore=1
                          # net.ipv4.conf.all.arp_announce=2
  
  minSyncPeriod: 1s       # 最小同步间隔 1 秒
  syncPeriod: 10s         # 全量同步周期 10 秒 (比 iptables 更频繁)
  
  # TCP/UDP 超时配置
  tcpTimeout: 900s        # TCP 空闲超时 15 分钟
  tcpFinTimeout: 120s     # TCP FIN_WAIT 超时 2 分钟
  udpTimeout: 300s        # UDP 超时 5 分钟
  
  # 排除特定 CIDR (不创建 IPVS 规则)
  # excludeCIDRs:
  #   - "169.254.0.0/16"  # 排除 link-local 地址

# ========================================
# Conntrack 优化 (高并发场景)
# ========================================
conntrack:
  maxPerCore: 65536       # 每核心 64K 条目 (默认 32K)
                          # 16 核节点: 65536 × 16 = 1048576 (约 100 万)
  
  min: 262144             # 最小 256K 条目 (保底值)
  
  # TCP 超时优化 (减少 conntrack 占用)
  tcpEstablishedTimeout: 3600s  # 1 小时 (默认 24 小时太长)
  tcpCloseWaitTimeout: 1800s    # 30 分钟 (默认 1 小时)
  
  # UDP 超时
  udpTimeout: 30s         # UDP 单向流超时
  udpStreamTimeout: 180s  # UDP 双向流超时

# ========================================
# 集群网络
# ========================================
clusterCIDR: "10.244.0.0/16"  # Pod CIDR (必须与 CNI 一致)

# ========================================
# NodePort 配置
# ========================================
# nodePortAddresses:      # 限制 NodePort 绑定的 IP 范围
#   - "10.0.0.0/8"        # 仅绑定内网 IP (提升安全性)
#   - "192.168.0.0/16"

# ========================================
# 本地流量检测 (保留源 IP)
# ========================================
detectLocalMode: "NodeCIDR"  # 通过节点 PodCIDR 判断本地流量
                              # 本地 Pod 访问 Service 时保留源 IP

# ========================================
# 健康检查和监控
# ========================================
healthzBindAddress: "0.0.0.0:10256"  # 允许外部健康检查
metricsBindAddress: "0.0.0.0:10249"  # 允许 Prometheus 抓取
                                      # ⚠️ 生产环境建议配合 NetworkPolicy 限制访问

# ========================================
# 特性门控
# ========================================
featureGates:
  # v1.27+ 支持 nftables 模式 (实验性)
  # NFTablesProxyMode: false

# ========================================
# 日志配置
# ========================================
logging:
  format: json      # JSON 格式 (便于日志采集)
  verbosity: 2      # INFO 级别
```

**配置亮点**:
- **IPVS 高性能**: 适用于 1000+ Services 的大型集群
- **Conntrack 优化**: 支持高并发短连接 (如微服务架构)
- **strictARP**: 支持 MetalLB 裸金属负载均衡
- **本地流量检测**: 保留源 IP,便于日志审计

**所需内核模块** (IPVS):
```bash
# 加载 IPVS 内核模块
modprobe ip_vs
modprobe ip_vs_rr    # Round-Robin
modprobe ip_vs_wrr   # Weighted Round-Robin
modprobe ip_vs_sh    # Source Hashing
modprobe nf_conntrack

# 永久加载 (写入 /etc/modules-load.d/ipvs.conf)
cat <<EOF > /etc/modules-load.d/ipvs.conf
ip_vs
ip_vs_rr
ip_vs_wrr
ip_vs_sh
nf_conntrack
EOF
```

**所需内核参数** (strictARP):
```bash
# /etc/sysctl.d/99-kube-proxy.conf
net.ipv4.conf.all.arp_ignore=1       # 仅响应目标 IP 为本机 IP 的 ARP 请求
net.ipv4.conf.all.arp_announce=2     # 使用最佳本地地址作为 ARP 应答的源 IP
net.netfilter.nf_conntrack_max=1048576  # 手动设置 conntrack 上限 (可选)

# 应用配置
sysctl -p /etc/sysctl.d/99-kube-proxy.conf
```

---

### 3.5 内部机制

#### 3.5.1 iptables 规则链结构

**完整规则链**:
```
PREROUTING (nat 表)
  └── KUBE-SERVICES  ← 入口链
      ├── KUBE-SVC-ABCDEF123456  (Service 1: ClusterIP)
      │   ├── KUBE-SEP-111111  (Endpoint 1, statistic --probability 0.33)
      │   ├── KUBE-SEP-222222  (Endpoint 2, statistic --probability 0.50)
      │   └── KUBE-SEP-333333  (Endpoint 3, statistic --probability 1.00)
      ├── KUBE-NODEPORTS  ← NodePort 入口
      │   └── KUBE-SVC-ABCDEF123456  (跳转到对应 Service)
      └── KUBE-MARK-MASQ  ← SNAT 标记链

OUTPUT (nat 表)
  └── KUBE-SERVICES  (同上,处理本地发起的流量)

POSTROUTING (nat 表)
  └── KUBE-POSTROUTING
      └── MASQUERADE (根据 fwmark 进行 SNAT)
```

**示例规则** (Service: nginx, 3 个 Pod):
```bash
# 主链
-A KUBE-SERVICES -d 10.96.0.1/32 -p tcp -m tcp --dport 80 -j KUBE-SVC-NGINX

# Service 链 (负载均衡)
-A KUBE-SVC-NGINX -m statistic --mode random --probability 0.33333 -j KUBE-SEP-POD1
-A KUBE-SVC-NGINX -m statistic --mode random --probability 0.50000 -j KUBE-SEP-POD2
-A KUBE-SVC-NGINX -j KUBE-SEP-POD3

# Endpoint 链 (DNAT 到 Pod IP)
-A KUBE-SEP-POD1 -p tcp -m tcp -j DNAT --to-destination 10.244.1.2:80
-A KUBE-SEP-POD2 -p tcp -m tcp -j DNAT --to-destination 10.244.1.3:80
-A KUBE-SEP-POD3 -p tcp -m tcp -j DNAT --to-destination 10.244.1.4:80
```

**负载均衡算法** (随机概率):
- 第一个 Pod: 1/3 概率
- 第二个 Pod: 1/2 × 2/3 = 1/3 概率
- 第三个 Pod: 1 × 1/3 = 1/3 概率

**性能问题**:
- **规则数量**: 10 Services × 10 Endpoints = 100+ 规则链
- **遍历开销**: 每个数据包需遍历所有规则 (O(n))
- **更新延迟**: 全量刷新规则链,耗时数秒

---

#### 3.5.2 IPVS 虚拟服务器结构

**IPVS 表示例**:
```bash
ipvsadm -Ln
```

**输出**:
```
IP Virtual Server version 1.2.1 (size=4096)
Prot LocalAddress:Port Scheduler Flags
  -> RemoteAddress:Port           Forward Weight ActiveConn InActConn

# ClusterIP Service
TCP  10.96.0.1:80 rr
  -> 10.244.1.2:80                Masq    1      0          0
  -> 10.244.1.3:80                Masq    1      0          0
  -> 10.244.1.4:80                Masq    1      0          0

# NodePort Service
TCP  10.0.0.10:30080 rr
  -> 10.244.1.2:80                Masq    1      0          0
  -> 10.244.1.3:80                Masq    1      0          0
  -> 10.244.1.4:80                Masq    1      0          0
```

**字段说明**:
- **Scheduler**: 负载均衡算法 (rr/lc/sh 等)
- **Forward**: 转发模式
  - `Masq`: NAT 模式 (SNAT/DNAT)
  - `Route`: 直接路由 (需 LVS-DR 配置)
  - `Tunnel`: IP 隧道
- **Weight**: 权重 (1-65535,默认 1)
- **ActiveConn**: 当前活跃连接数
- **InActConn**: 非活跃连接数

**IPVS 数据结构**:
- **哈希表**: O(1) 查找虚拟服务器
- **链表**: O(1) 遍历 Endpoints
- **内核模块**: 直接在内核空间处理,无需用户态上下文切换

**iptables 辅助规则**:
即使使用 IPVS,仍需少量 iptables 规则处理:
- **SNAT 标记**: `KUBE-MARK-MASQ` 链
- **NodePort 流量**: `KUBE-NODEPORTS` 链
- **本地流量**: `KUBE-IPVS-LOCAL` 链

```bash
iptables -t nat -L KUBE-SERVICES -n

# 输出示例:
-A KUBE-SERVICES -m set --match-set KUBE-CLUSTER-IP dst,dst -j ACCEPT
-A KUBE-SERVICES -m set --match-set KUBE-LOOP-BACK src,dst -j ACCEPT
```

---

#### 3.5.3 nftables 表结构 (v1.29+)

**nftables 命名空间**:
```bash
nft list tables
```

**输出**:
```
table ip kube-proxy  ← Kube-Proxy 创建的 nftables 表
```

**查看规则**:
```bash
nft list table ip kube-proxy
```

**示例结构**:
```
table ip kube-proxy {
  chain services {
    type nat hook prerouting priority 0; policy accept;
    
    # ClusterIP Service
    ip daddr 10.96.0.1 tcp dport 80 jump service-nginx
  }
  
  chain service-nginx {
    # 负载均衡到 Endpoints
    numgen random mod 3 vmap {
      0: dnat to 10.244.1.2:80,
      1: dnat to 10.244.1.3:80,
      2: dnat to 10.244.1.4:80
    }
  }
}
```

**nftables 优势**:
- **原子更新**: 使用事务机制,避免竞态条件
- **表达力**: 支持 `vmap`(映射表)、`set`(集合) 等高级特性
- **性能**: 规则匹配使用字节码虚拟机,优于 iptables 的线性匹配

---

#### 3.5.4 Conntrack 表管理

**Conntrack 条目生命周期**:
1. **NEW**: 新建连接,首包到达
2. **ESTABLISHED**: 双向通信建立
3. **RELATED**: 相关连接 (如 FTP 数据连接)
4. **CLOSING**: 连接关闭中 (TCP FIN)
5. **TIME_WAIT**: 连接已关闭,等待超时

**查看 conntrack 条目**:
```bash
# 查看所有条目
conntrack -L

# 查看统计信息
conntrack -S

# 输出示例:
cpu=0           found=12345 invalid=10 ignore=0 insert=100000 insert_failed=5 drop=5
cpu=1           found=23456 invalid=8  ignore=0 insert=120000 insert_failed=3 drop=3
```

**conntrack 条目示例**:
```
tcp  6 86395 ESTABLISHED src=10.244.1.2 dst=10.96.0.1 sport=52134 dport=80 \
     src=10.244.1.3 dst=10.244.1.2 sport=80 dport=52134 [ASSURED] mark=0 use=1
```

**字段说明**:
- **86395**: 剩余超时时间 (秒)
- **ESTABLISHED**: 连接状态
- **[ASSURED]**: 双向流量已确认

**conntrack 表满问题**:
```bash
# 内核日志
dmesg | grep nf_conntrack
# nf_conntrack: table full, dropping packet

# 解决方案 1: 增大表大小
sysctl -w net.netfilter.nf_conntrack_max=1048576

# 解决方案 2: 减小超时时间
sysctl -w net.netfilter.nf_conntrack_tcp_timeout_established=3600

# 解决方案 3: 启用 conntrack 自动清理
sysctl -w net.netfilter.nf_conntrack_tcp_timeout_time_wait=30
```

---

#### 3.5.5 iptables vs IPVS 性能对比

**测试场景**: 10000 Services,每个 Service 10 个 Endpoints

| 指标 | iptables | IPVS | 改进 |
|------|----------|------|------|
| **规则数量** | 100,000+ 条规则 | 10,000 个虚拟服务器 | 10x ↓ |
| **首包延迟** | 5-10 ms (线性查找) | < 1 ms (哈希查找) | 10x ↑ |
| **规则更新时间** | 30-60 秒 | < 1 秒 | 60x ↑ |
| **内存占用** | 500 MB | 100 MB | 5x ↓ |
| **CPU 占用** | 10-20% (更新时) | < 5% | 4x ↓ |

**结论**:
- **小型集群** (< 100 Services): iptables 足够
- **大型集群** (> 1000 Services): **强烈推荐 IPVS**
- **超大集群** (> 10000 Services): IPVS + Cilium eBPF

---

## 4. KubeSchedulerConfiguration 完整参考

### 4.1 API 信息

**KubeSchedulerConfiguration** 是 Kube-Scheduler 组件的配置 API,用于控制 Pod 调度策略、插件配置和调度器行为。

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
```

**版本历史**:
- `v1alpha1`: v1.18-v1.22 (已废弃)
- `v1alpha2`: v1.19-v1.22 (已废弃)
- `v1beta1`: v1.19-v1.24 (已废弃)
- `v1beta2`: v1.22-v1.24 (已废弃)
- `v1beta3`: v1.23-v1.25 (已废弃)
- **`v1`**: **v1.25+ GA (稳定版本)**

**配置文件加载**:
```bash
kube-scheduler --config=/etc/kubernetes/scheduler-config.yaml
```

---

### 4.2 完整字段规范

#### 4.2.1 profiles[] (调度器 Profile)

一个 Scheduler 可运行多个 Profile,每个 Profile 是独立的调度器实例,有自己的名称和插件配置。

| 字段 | 类型 | 说明 |
|------|------|------|
| **schedulerName** | string | 调度器名称<br>- 默认: `default-scheduler`<br>- Pod 通过 `spec.schedulerName` 指定使用的调度器 |
| **plugins** | Plugins | 各扩展点的插件配置 (启用/禁用) |
| **pluginConfig[]** | PluginConfig | 各插件的参数配置 |

**Scheduling Framework 扩展点**:

| 扩展点 | 阶段 | 说明 |
|--------|------|------|
| **queueSort** | 排队 | Pod 入队排序 (默认: `PrioritySort`) |
| **preFilter** | 预过滤 | 计算 Pod 调度的先决条件 |
| **filter** | 过滤 | 筛选可调度节点 (核心阶段) |
| **postFilter** | 后过滤 | 处理无可调度节点的情况 (如抢占) |
| **preScore** | 预打分 | 预计算打分所需数据 |
| **score** | 打分 | 为候选节点打分 (0-100) |
| **reserve** | 预留 | 预留资源 (如 Volume) |
| **permit** | 许可 | 批准或延迟调度 |
| **preBind** | 预绑定 | 绑定前的准备工作 (如创建 PV) |
| **bind** | 绑定 | 将 Pod 绑定到节点 |
| **postBind** | 后绑定 | 绑定后的清理工作 |

**插件启用/禁用**:
```yaml
profiles:
  - schedulerName: default-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesBalancedAllocation
            weight: 5
        disabled:
          - name: NodeResourcesLeastAllocated  # 禁用默认插件
```

---

#### 4.2.2 重要插件配置

##### NodeResourcesFit (资源适配)

**作用**: 检查节点资源 (CPU/Memory) 是否满足 Pod 的 requests。

**打分策略** (scoringStrategy):

| 策略 | 说明 | 适用场景 |
|------|------|----------|
| **LeastAllocated** | 优先选择资源使用率最低的节点<br>分数 = (Capacity - Allocated) / Capacity × 100 | 资源均衡分布,避免单节点过载 |
| **MostAllocated** | 优先选择资源使用率最高的节点<br>分数 = Allocated / Capacity × 100 | 资源紧凑分布,节省成本 (云环境) |
| **RequestedToCapacityRatio** | 根据自定义函数计算分数<br>灵活配置资源利用率曲线 | 特殊场景 (如 GPU 节点优先) |

**配置示例**:
```yaml
pluginConfig:
  - name: NodeResourcesFit
    args:
      scoringStrategy:
        type: LeastAllocated  # 均衡分布
        resources:
          - name: cpu
            weight: 1
          - name: memory
            weight: 1
```

---

##### PodTopologySpread (拓扑分布)

**作用**: 控制 Pod 在拓扑域 (zone/node/hostname) 上的分布,提高可用性。

**配置字段**:

| 字段 | 类型 | 说明 |
|------|------|------|
| **defaultConstraints[]** | TopologySpreadConstraint | 默认拓扑约束 (Pod 未指定 `topologySpreadConstraints` 时使用) |
| **defaultingType** | string | 默认行为:<br>- `System`: 使用系统默认约束<br>- `List`: 使用 `defaultConstraints` 列表 |

**示例**:
```yaml
pluginConfig:
  - name: PodTopologySpread
    args:
      defaultConstraints:
        - maxSkew: 1                 # 最大偏差 1 个 Pod
          topologyKey: topology.kubernetes.io/zone  # 按可用区分布
          whenUnsatisfiable: ScheduleAnyway         # 无法满足时仍调度
      defaultingType: List
```

---

##### InterPodAffinity (Pod 亲和性)

**作用**: 处理 Pod 的亲和性 (Affinity) 和反亲和性 (Anti-Affinity) 规则。

**配置字段**:

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **hardPodAffinityWeight** | int32 | `1` | 硬亲和性的权重<br>- 影响打分阶段的权重分配<br>- 取值范围: 0-100 |

**示例**:
```yaml
pluginConfig:
  - name: InterPodAffinity
    args:
      hardPodAffinityWeight: 10  # 提高亲和性权重
```

---

##### NodeAffinity (节点亲和性)

**作用**: 处理 Pod 的节点选择器 (NodeSelector) 和节点亲和性。

**配置字段**:

| 字段 | 类型 | 说明 |
|------|------|------|
| **addedAffinity** | NodeAffinity | 全局节点亲和性 (追加到所有 Pod) |

**示例** (强制所有 Pod 调度到 SSD 节点):
```yaml
pluginConfig:
  - name: NodeAffinity
    args:
      addedAffinity:
        requiredDuringSchedulingIgnoredDuringExecution:
          nodeSelectorTerms:
            - matchExpressions:
                - key: disk-type
                  operator: In
                  values:
                    - ssd
```

---

##### VolumeBinding (卷绑定)

**作用**: 处理 PV/PVC 绑定,确保 Pod 和卷在同一拓扑域。

**配置字段**:

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **bindTimeoutSeconds** | int64 | `600` | 卷绑定超时时间 (秒)<br>- 超时后 Pod 调度失败 |

**示例**:
```yaml
pluginConfig:
  - name: VolumeBinding
    args:
      bindTimeoutSeconds: 300  # 5 分钟超时
```

---

#### 4.2.3 全局配置

##### percentageOfNodesToScore (节点采样比例)

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **percentageOfNodesToScore** | int32 | `0` | 打分阶段的节点采样比例 (%)<br>- `0`: 自动计算 (50 节点以下全部打分,以上采样)<br>- `100`: 对所有候选节点打分 (精确,但慢)<br>- `50`: 对 50% 候选节点打分 (快速,但可能不够优) |

**自动计算公式** (当设为 `0` 时):
```
采样比例 = max(50%, 50 / 节点数量)

示例:
- 100 节点: 50% (采样 50 个节点)
- 1000 节点: 5% (采样 50 个节点)
- 10 节点: 100% (全部打分)
```

**示例**:
```yaml
percentageOfNodesToScore: 50  # 固定采样 50%
```

---

##### leaderElection (领导选举)

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| **leaderElect** | bool | `true` | 是否启用领导选举 (高可用部署必需) |
| **leaseDuration** | duration | `15s` | Lease 租约时长<br>- Leader 每 15 秒续约 |
| **renewDeadline** | duration | `10s` | 续约截止时间<br>- Leader 必须在 10 秒内完成续约 |
| **retryPeriod** | duration | `2s` | 续约重试间隔<br>- 失败后每 2 秒重试 |
| **resourceNamespace** | string | `kube-system` | Lease 资源所在的命名空间 |
| **resourceName** | string | `kube-scheduler` | Lease 资源名称 |

**示例**:
```yaml
leaderElection:
  leaderElect: true
  leaseDuration: 15s
  renewDeadline: 10s
  retryPeriod: 2s
  resourceNamespace: kube-system
  resourceName: kube-scheduler
```

---

#### 4.2.4 extenders[] (调度器扩展器)

**Extender** 允许通过 HTTP Webhook 扩展调度器功能,调用外部服务进行过滤、打分、绑定。

| 字段 | 类型 | 说明 |
|------|------|------|
| **urlPrefix** | string | 外部服务的 URL 前缀<br>例如: `http://custom-scheduler:8080` |
| **filterVerb** | string | 过滤 API 路径 (默认: `/filter`) |
| **prioritizeVerb** | string | 打分 API 路径 (默认: `/prioritize`) |
| **bindVerb** | string | 绑定 API 路径 (默认: `/bind`)<br>⚠️ 如果设置,绑定由 Extender 负责 |
| **weight** | int64 | 打分权重 (1-100) |
| **enableHTTPS** | bool | 是否启用 HTTPS |
| **httpTimeout** | duration | HTTP 请求超时 (默认: `5s`) |
| **tlsConfig** | ExtenderTLSConfig | TLS 配置 (证书、CA) |
| **nodeCacheCapable** | bool | Extender 是否缓存节点信息 |
| **managedResources[]** | ExtendedResource | Extender 管理的扩展资源 (如 GPU) |
| **ignorable** | bool | Extender 失败时是否可忽略<br>- `true`: 失败时跳过,继续调度<br>- `false`: 失败时调度失败 |

**示例** (GPU 调度扩展器):
```yaml
extenders:
  - urlPrefix: "http://gpu-scheduler:8080"
    filterVerb: "filter"
    prioritizeVerb: "prioritize"
    weight: 10
    httpTimeout: 5s
    managedResources:
      - name: "nvidia.com/gpu"
        ignoredByScheduler: true  # 调度器忽略该资源,由 Extender 处理
    ignorable: false  # GPU 调度失败时不可忽略
```

---

### 4.3 最小化示例

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration

# ========================================
# 调度器 Profile
# ========================================
profiles:
  - schedulerName: default-scheduler  # 默认调度器
    plugins: {}  # 使用所有默认插件

# ========================================
# 领导选举 (高可用)
# ========================================
leaderElection:
  leaderElect: true
  leaseDuration: 15s
  renewDeadline: 10s
  retryPeriod: 2s
```

**适用场景**:
- 小型集群
- 无特殊调度需求
- 使用默认调度策略

---

### 4.4 多 Profile 示例

**场景**: 为高优先级 Pod 和普通 Pod 使用不同的调度策略。

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration

# ========================================
# 多调度器 Profile
# ========================================
profiles:
  # Profile 1: 默认调度器 (资源均衡)
  - schedulerName: default-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesBalancedAllocation  # 资源均衡
            weight: 10
          - name: NodeResourcesLeastAllocated     # 优先低负载节点
            weight: 5
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: LeastAllocated  # 均衡分布策略

  # Profile 2: 高优先级调度器 (资源紧凑)
  - schedulerName: high-priority-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesMostAllocated      # 资源紧凑
            weight: 10
        disabled:
          - name: NodeResourcesBalancedAllocation # 禁用均衡
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: MostAllocated  # 紧凑分布策略 (节省成本)
      - name: PodTopologySpread
        args:
          defaultConstraints:
            - maxSkew: 1
              topologyKey: topology.kubernetes.io/zone
              whenUnsatisfiable: DoNotSchedule  # 强制跨可用区

# ========================================
# 领导选举
# ========================================
leaderElection:
  leaderElect: true
```

**使用方式**:
```yaml
# 默认 Pod (使用 default-scheduler)
apiVersion: v1
kind: Pod
metadata:
  name: nginx
spec:
  containers:
    - name: nginx
      image: nginx

---

# 高优先级 Pod (使用 high-priority-scheduler)
apiVersion: v1
kind: Pod
metadata:
  name: critical-app
spec:
  schedulerName: high-priority-scheduler  # 指定调度器
  priorityClassName: system-cluster-critical
  containers:
    - name: app
      image: critical-app
```

---

### 4.5 Extender 示例

**场景**: 使用外部调度器处理自定义 GPU 调度逻辑。

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration

# ========================================
# 默认 Profile
# ========================================
profiles:
  - schedulerName: default-scheduler
    plugins: {}

# ========================================
# 调度器扩展器 (GPU 调度)
# ========================================
extenders:
  - urlPrefix: "http://gpu-scheduler.kube-system.svc:8080"
    filterVerb: "filter"        # 过滤接口: POST /filter
    prioritizeVerb: "prioritize" # 打分接口: POST /prioritize
    weight: 20                   # 打分权重
    enableHTTPS: false
    httpTimeout: 10s             # 超时 10 秒
    
    # 管理的扩展资源
    managedResources:
      - name: "nvidia.com/gpu"
        ignoredByScheduler: true  # 调度器忽略 GPU 资源检查
    
    ignorable: false  # GPU 调度失败时不可忽略
    nodeCacheCapable: true  # Extender 缓存节点信息

# ========================================
# 领导选举
# ========================================
leaderElection:
  leaderElect: true
  leaseDuration: 15s
  renewDeadline: 10s
  retryPeriod: 2s
```

**Extender API 规范**:

**过滤接口** (POST /filter):
```json
{
  "pod": { /* Pod 对象 */ },
  "nodes": { "items": [ /* Node 列表 */ ] }
}
```

**响应**:
```json
{
  "nodes": { "items": [ /* 可调度节点 */ ] },
  "failedNodes": { "node1": "GPU 不足" },
  "error": ""
}
```

**打分接口** (POST /prioritize):
```json
{
  "pod": { /* Pod 对象 */ },
  "nodes": { "items": [ /* Node 列表 */ ] }
}
```

**响应**:
```json
[
  { "host": "node1", "score": 90 },
  { "host": "node2", "score": 70 }
]
```

---

### 4.6 内部机制

#### 4.6.1 Scheduling Framework 架构

**调度周期** (Scheduling Cycle,同步):
```
QueueSort (排队排序)
  ↓
PreFilter (预过滤,计算先决条件)
  ↓
Filter (过滤,筛选可调度节点)  ← 并行处理所有节点
  ↓
PostFilter (后过滤,处理无可调度节点,如抢占)
  ↓
PreScore (预打分,预计算数据)
  ↓
Score (打分,为候选节点打分)   ← 并行处理候选节点
  ↓
Normalize Scores (归一化分数,0-100)
  ↓
Reserve (预留资源)
  ↓
Permit (许可,批准或延迟)
```

**绑定周期** (Binding Cycle,异步):
```
WaitOnPermit (等待许可通过)
  ↓
PreBind (预绑定,如创建 PV)
  ↓
Bind (绑定,更新 Pod.spec.nodeName)
  ↓
PostBind (后绑定,清理工作)
```

**关键点**:
- **调度周期是同步的**: 一次只调度一个 Pod
- **绑定周期是异步的**: 可并发绑定多个 Pod
- **失败处理**: Filter 失败 → PostFilter (抢占),Bind 失败 → 重新调度

---

#### 4.6.2 插件执行模型和错误处理

**插件返回值**:
- **Success**: 继续执行
- **Error**: 调度失败,重新入队
- **Unschedulable**: 节点不可调度 (Filter 插件)
- **Wait**: 等待外部条件 (Permit 插件)

**并行执行**:
- **Filter 阶段**: 所有节点并行过滤 (Go routines)
- **Score 阶段**: 所有候选节点并行打分

**错误处理**:

| 阶段 | 失败行为 |
|------|----------|
| **QueueSort** | 调度器退出 (致命错误) |
| **PreFilter** | Pod 调度失败,重新入队 |
| **Filter** | 跳过该节点,继续筛选其他节点 |
| **PostFilter** | Pod 调度失败,重新入队 (可能触发抢占) |
| **Score** | 跳过该插件的打分 (使用 0 分) |
| **Reserve** | 回滚所有 Reserve 操作,Pod 重新入队 |
| **Permit** | Pod 调度失败,重新入队 |
| **Bind** | Pod 绑定失败,重新入队 (不回滚 Reserve) |

**重试机制**:
- **初始延迟**: 1 秒
- **最大延迟**: 10 秒
- **退避策略**: 指数退避 (1s → 2s → 4s → 8s → 10s)

---

#### 4.6.3 percentageOfNodesToScore 优化

**调度性能优化**:
- **小集群** (< 50 节点): 对所有节点打分 (100% 采样)
- **中型集群** (50-5000 节点): 采样 50% 节点
- **大型集群** (> 5000 节点): 采样 5% 节点 (最少 50 个)

**trade-off**:
- **采样率高**: 调度决策更优,但耗时长
- **采样率低**: 调度速度快,但可能次优

**验证方法**:
```bash
# 查看调度延迟 (metrics)
kubectl get --raw /metrics | grep scheduler_scheduling_duration_seconds

# 输出示例:
scheduler_scheduling_duration_seconds_bucket{le="0.1"} 1200
scheduler_scheduling_duration_seconds_bucket{le="0.5"} 1500
scheduler_scheduling_duration_seconds_bucket{le="1.0"} 1800
```

**调优建议**:
- **延迟敏感**: 设置 `percentageOfNodesToScore: 30` (快速调度)
- **资源优化**: 设置 `percentageOfNodesToScore: 100` (精确调度)

---

## 5. KubeControllerManagerConfiguration

**KubeControllerManagerConfiguration** 目前没有稳定的类型化配置 API (v1.32 仍为内部 API),配置仍依赖命令行标志 (CLI flags)。

以下是生产环境常用的关键参数:

---

### 5.1 关键 CLI Flags

#### 节点健康检查

| Flag | 默认值 | 说明 |
|------|--------|------|
| **--node-monitor-period** | `5s` | 节点状态检查周期<br>- Node Controller 每 5 秒检查一次节点心跳 |
| **--node-monitor-grace-period** | `40s` | 节点心跳超时时间<br>- 超过 40 秒未收到心跳,标记节点为 NotReady |
| **--pod-eviction-timeout** | `5m` | **已废弃** (v1.13+)<br>- 使用 Taint-based Eviction 替代 |

**Taint-based Eviction** (v1.13+):
- 节点 NotReady 后自动添加 taint: `node.kubernetes.io/not-ready:NoExecute`
- Pod 的 `tolerationSeconds` 控制驱逐延迟

---

#### Controller 并发数

| Flag | 默认值 | 说明 |
|------|--------|------|
| **--concurrent-deployment-syncs** | `5` | Deployment Controller 并发同步数 |
| **--concurrent-statefulset-syncs** | `5` | StatefulSet Controller 并发同步数 |
| **--concurrent-job-syncs** | `5` | Job Controller 并发同步数 |
| **--concurrent-replicaset-syncs** | `5` | ReplicaSet Controller 并发同步数 |
| **--concurrent-namespace-syncs** | `10` | Namespace Controller 并发同步数 |

**调优建议**:
- **大型集群**: 增大并发数 (如 `--concurrent-deployment-syncs=10`)
- **资源受限**: 保持默认值 (避免 API Server 过载)

---

#### Pod GC (垃圾回收)

| Flag | 默认值 | 说明 |
|------|--------|------|
| **--terminated-pod-gc-threshold** | `12500` | 已终止 Pod 的最大保留数量<br>- 超过后自动删除最老的 Pod |

**示例场景**:
- 大量 Job/CronJob 的集群,已完成 Pod 会堆积
- 减小阈值 (如 `5000`) 以释放 etcd 空间

---

#### 证书签发

| Flag | 默认值 | 说明 |
|------|--------|------|
| **--cluster-signing-cert-file** | `/etc/kubernetes/pki/ca.crt` | 集群 CA 证书路径 |
| **--cluster-signing-key-file** | `/etc/kubernetes/pki/ca.key` | 集群 CA 私钥路径 |
| **--cluster-signing-duration** | `8760h` | 签发证书的有效期 (默认 1 年) |

**用途**:
- 自动批准 Kubelet 的 CSR (证书签名请求)
- 签发 Kubelet 客户端证书和服务端证书

---

#### 特性门控

| Flag | 说明 |
|------|------|
| **--feature-gates** | 启用/禁用实验性特性<br>例如: `--feature-gates=CronJobTimeZone=true` |

**常用 Feature Gates**:
- `CronJobTimeZone=true`: CronJob 支持时区 (v1.25+ Beta)
- `JobPodFailurePolicy=true`: Job Pod 失败策略 (v1.26+ Beta)
- `StatefulSetAutoDeletePVC=true`: StatefulSet 自动删除 PVC (v1.27+ Beta)

---

### 5.2 systemd Unit 文件示例

```ini
# /etc/systemd/system/kube-controller-manager.service

[Unit]
Description=Kubernetes Controller Manager
Documentation=https://kubernetes.io/docs/concepts/overview/components/
After=network.target

[Service]
Type=notify
ExecStart=/usr/local/bin/kube-controller-manager \
  --kubeconfig=/etc/kubernetes/controller-manager.conf \
  --authentication-kubeconfig=/etc/kubernetes/controller-manager.conf \
  --authorization-kubeconfig=/etc/kubernetes/controller-manager.conf \
  --client-ca-file=/etc/kubernetes/pki/ca.crt \
  --requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt \
  \
  # ========================================
  # 节点健康检查
  # ========================================
  --node-monitor-period=5s \
  --node-monitor-grace-period=40s \
  \
  # ========================================
  # Controller 并发数 (大型集群优化)
  # ========================================
  --concurrent-deployment-syncs=10 \
  --concurrent-statefulset-syncs=10 \
  --concurrent-job-syncs=10 \
  --concurrent-replicaset-syncs=10 \
  \
  # ========================================
  # Pod GC
  # ========================================
  --terminated-pod-gc-threshold=5000 \
  \
  # ========================================
  # 证书签发
  # ========================================
  --cluster-signing-cert-file=/etc/kubernetes/pki/ca.crt \
  --cluster-signing-key-file=/etc/kubernetes/pki/ca.key \
  --cluster-signing-duration=8760h \
  --root-ca-file=/etc/kubernetes/pki/ca.crt \
  --service-account-private-key-file=/etc/kubernetes/pki/sa.key \
  \
  # ========================================
  # 领导选举 (高可用)
  # ========================================
  --leader-elect=true \
  --leader-elect-lease-duration=15s \
  --leader-elect-renew-deadline=10s \
  --leader-elect-retry-period=2s \
  \
  # ========================================
  # 集群网络
  # ========================================
  --cluster-cidr=10.244.0.0/16 \
  --service-cluster-ip-range=10.96.0.0/12 \
  --allocate-node-cidrs=true \
  \
  # ========================================
  # 特性门控
  # ========================================
  --feature-gates=CronJobTimeZone=true,JobPodFailurePolicy=true \
  \
  # ========================================
  # 日志和监控
  # ========================================
  --bind-address=0.0.0.0 \
  --secure-port=10257 \
  --v=2

Restart=on-failure
RestartSec=5s
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
```

**启动服务**:
```bash
systemctl daemon-reload
systemctl enable kube-controller-manager
systemctl start kube-controller-manager
systemctl status kube-controller-manager
```

---

## 6. 版本兼容性矩阵

### 6.1 组件配置 API 版本

| 组件 | API Group | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-----------|-------|-------|-------|-------|-------|-------|-------|-------|
| **Kubelet** | kubelet.config.k8s.io | v1beta1 | v1beta1 | v1beta1 | v1beta1 | v1beta1 | v1beta1 | v1beta1 | v1beta1 |
| **KubeProxy** | kubeproxy.config.k8s.io | v1alpha1 | v1alpha1 | v1alpha1 | v1alpha1 | v1alpha1 | v1alpha1 | v1alpha1 | v1alpha1 |
| **KubeScheduler** | kubescheduler.config.k8s.io | **v1 (GA)** | v1 | v1 | v1 | v1 | v1 | v1 | v1 |
| **KubeControllerManager** | - | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |

**说明**:
- **KubeletConfiguration v1beta1**: 长期 Beta,未来可能升级为 v1
- **KubeProxyConfiguration v1alpha1**: 长期 Alpha,但实际非常稳定
- **KubeSchedulerConfiguration v1**: v1.25+ GA,推荐使用
- **KubeControllerManager**: 无稳定 API,使用 CLI flags

---

### 6.2 Feature Gates 时间线

| Feature | 引入 | Beta | GA | 说明 |
|---------|------|------|----|------|
| **GracefulNodeShutdown** | v1.20 (Alpha) | v1.21 (Beta) | v1.28 (GA) | 节点优雅关机 |
| **MemoryQoS** | v1.22 (Alpha) | - | - | 内存 QoS (需 cgroup v2) |
| **MemoryManager** | v1.21 (Alpha) | v1.22 (Beta) | - | NUMA 内存管理 |
| **CPUManager** | v1.8 (Alpha) | v1.10 (Beta) | v1.26 (GA) | CPU 拓扑管理 |
| **TopologyManager** | v1.16 (Alpha) | v1.18 (Beta) | v1.27 (GA) | 设备拓扑管理 |
| **KubeletTracing** | v1.25 (Alpha) | v1.27 (Beta) | - | Kubelet OpenTelemetry 追踪 |
| **NFTablesProxyMode** | v1.29 (Alpha) | - | - | nftables 代理模式 |
| **CronJobTimeZone** | v1.24 (Alpha) | v1.25 (Beta) | v1.27 (GA) | CronJob 时区支持 |
| **JobPodFailurePolicy** | v1.25 (Alpha) | v1.26 (Beta) | v1.31 (GA) | Job Pod 失败策略 |

**推荐策略**:
- **GA 特性**: 直接启用,稳定可靠
- **Beta 特性**: 测试环境验证后启用
- **Alpha 特性**: 仅在非生产环境尝试

---

## 7. 最佳实践

### 7.1 大规模集群调优 (1000+ nodes)

#### Kubelet 优化
```yaml
# 减少 API Server 压力
nodeStatusReportFrequency: 10m  # 状态未变化时每 10 分钟上报 (默认 5m)
nodeStatusUpdateFrequency: 30s  # 状态检查频率 30 秒 (默认 10s)

# 增大资源上限
maxPods: 250  # 高密度节点 (需配合 CNI IP 池)
maxOpenFiles: 2000000  # 增大文件描述符

# 并行镜像拉取
serializeImagePulls: false
maxParallelImagePulls: 5

# 优化镜像 GC
imageGCHighThresholdPercent: 90  # 提高高水位 (减少 GC 频率)
imageGCLowThresholdPercent: 85
```

#### KubeProxy 优化
```yaml
mode: "ipvs"  # 必须使用 IPVS (iptables 无法支撑)

ipvs:
  syncPeriod: 30s  # 增大同步周期 (减少 CPU)
  minSyncPeriod: 5s

conntrack:
  maxPerCore: 131072  # 每核心 128K 条目 (支持百万级连接)
  min: 1048576  # 最小 1M 条目
  tcpEstablishedTimeout: 3600s  # 减小超时 (释放表空间)
```

#### KubeScheduler 优化
```yaml
percentageOfNodesToScore: 10  # 仅采样 10% 节点 (加速调度)

profiles:
  - schedulerName: default-scheduler
    pluginConfig:
      - name: InterPodAffinity
        args:
          hardPodAffinityWeight: 5  # 降低亲和性权重 (减少计算量)
```

#### KubeControllerManager 优化
```bash
--concurrent-deployment-syncs=20  # 增大并发数
--concurrent-statefulset-syncs=20
--concurrent-job-syncs=20
--terminated-pod-gc-threshold=2000  # 减小 Pod GC 阈值
```

---

### 7.2 Conntrack 表大小计算

**计算公式**:
```
所需 conntrack 条目数 = 并发连接数 × 1.2 (20% 余量)

实际配置 = max(
  所需条目数,
  maxPerCore × CPU 核心数,
  min
)
```

**场景示例**:

**场景 1: 微服务集群** (100 Services,1000 Pods,每 Pod 100 并发连接)
```
并发连接数 = 1000 Pods × 100 = 100,000
所需条目 = 100,000 × 1.2 = 120,000

配置 (16 核节点):
maxPerCore: 16384  # 16384 × 16 = 262,144 (足够)
min: 131072
```

**场景 2: 高并发 Web 集群** (NodePort 服务,10 万并发连接)
```
并发连接数 = 100,000
所需条目 = 100,000 × 1.2 = 120,000

配置 (32 核节点):
maxPerCore: 65536  # 65536 × 32 = 2,097,152 (冗余)
min: 262144
tcpEstablishedTimeout: 1800s  # 减小超时 (加速回收)
```

**监控 conntrack 使用率**:
```bash
# Prometheus 指标
node_nf_conntrack_entries / node_nf_conntrack_entries_limit

# 告警规则 (使用率 > 80%)
- alert: ConntrackTableNearlyFull
  expr: node_nf_conntrack_entries / node_nf_conntrack_entries_limit > 0.8
  for: 5m
  annotations:
    summary: "Conntrack table nearly full on {{ $labels.instance }}"
```

---

### 7.3 CPU Manager Static 策略使用条件

**适用场景**:
- ✅ 延迟敏感应用 (如实时音视频、游戏服务器)
- ✅ CPU 密集型计算 (如 AI 训练、科学计算)
- ✅ 数据库 (如 MySQL、Redis、PostgreSQL)
- ❌ 通用 Web 服务 (无明显收益)
- ❌ IO 密集型应用 (瓶颈在 IO,非 CPU)

**前提条件**:
1. **节点配置**:
   - CPU 核心数 ≥ 8 (预留 2 核给系统,至少 6 核给 Pod)
   - 已配置 `reservedSystemCPUs` (如 `"0-1"`)

2. **Pod 要求**:
   - QoS 类型: **Guaranteed** (CPU/Memory requests = limits)
   - CPU requests: **整数核心** (如 `cpu: "2"`,不能是 `cpu: "1.5"`)

3. **Kubelet 配置**:
   ```yaml
   cpuManagerPolicy: static
   reservedSystemCPUs: "0-1"  # 预留核心 0-1 给系统
   ```

**验证独占 CPU**:
```bash
# 查看 Pod 的 cgroup CPU 亲和性
cat /proc/$(pidof <pod-process>)/status | grep Cpus_allowed_list

# 输出示例:
Cpus_allowed_list: 2-5  # Pod 独占核心 2-5
```

**性能提升**:
- **延迟**: 减少 10-30% (避免 CPU 上下文切换)
- **吞吐量**: 提升 5-15% (L1/L2 cache 命中率提高)

---

### 7.4 IPVS vs iptables 选择标准

| 指标 | iptables | IPVS | 推荐 |
|------|----------|------|------|
| **Services 数量** | < 100 | > 100 | IPVS |
| **Endpoints 数量** | < 1000 | > 1000 | IPVS |
| **首包延迟** | 高 (O(n)) | 低 (O(1)) | IPVS |
| **规则更新时间** | 慢 (10s+) | 快 (< 1s) | IPVS |
| **负载均衡算法** | 随机 | 多种 (rr/lc/sh) | IPVS |
| **会话保持** | ❌ | ✅ (sh/dh) | IPVS |
| **内核要求** | ✅ 内置 | 需加载模块 | - |
| **调试难度** | 简单 (iptables -L) | 中等 (ipvsadm -L) | iptables |

**决策树**:
```
Services 数量 > 100?
├── 是 → 使用 IPVS
└── 否 → 需要会话保持?
    ├── 是 → 使用 IPVS (sh/dh 算法)
    └── 否 → 使用 iptables (简单场景)
```

**迁移步骤** (iptables → IPVS):
1. **加载内核模块**:
   ```bash
   modprobe ip_vs ip_vs_rr ip_vs_sh nf_conntrack
   ```

2. **更新 KubeProxy 配置**:
   ```yaml
   mode: "ipvs"
   ipvs:
     scheduler: "rr"
     strictARP: false  # 初期关闭 (避免 ARP 问题)
   ```

3. **逐节点重启** Kube-Proxy:
   ```bash
   kubectl rollout restart daemonset kube-proxy -n kube-system
   ```

4. **验证规则**:
   ```bash
   ipvsadm -Ln | head -20
   ```

---

### 7.5 优雅节点关机配置

**目的**: 节点关机/重启时,优雅终止 Pod,避免服务中断。

**Kubelet 配置**:
```yaml
featureGates:
  GracefulNodeShutdown: true  # v1.28+ 默认开启

shutdownGracePeriod: 60s                # 总优雅终止时间 60 秒
shutdownGracePeriodCriticalPods: 20s    # 最后 20 秒留给关键 Pod
```

**时间分配**:
```
0-40s: 终止普通 Pod (按 PriorityClass 从低到高)
40-60s: 终止关键 Pod (PriorityClass ≥ 2000000000)
```

**系统集成**:

**systemd 配置** (支持 systemd inhibitor lock):
```bash
# Kubelet 自动注册 inhibitor lock,systemd 关机时会等待
systemctl show kubelet | grep InhibitDelayMaxSec
# InhibitDelayMaxSec=60s (必须 ≥ shutdownGracePeriod)
```

**节点关机流程**:
1. systemd 发送 SIGTERM 信号
2. Kubelet 收到信号,创建 inhibitor lock
3. Kubelet 按优先级终止所有 Pod
4. 所有 Pod 终止完成 (或超时),释放 lock
5. systemd 继续关机流程

**验证**:
```bash
# 模拟关机 (不会真正关机)
systemctl start systemd-inhibit --who=test --what=shutdown --why="test" sleep 60

# 查看 inhibitor lock
systemd-inhibit --list

# 输出应包含 kubelet 的 lock
WHO   UID  USER  PID  COMM       WHAT     WHY                         MODE
kubelet 0  root  1234 kubelet    shutdown Node graceful shutdown     delay
```

---

### 7.6 镜像垃圾回收调优

**目标**: 平衡磁盘空间和镜像可用性。

**默认配置** (保守策略):
```yaml
imageGCHighThresholdPercent: 85  # 磁盘使用 85% 触发 GC
imageGCLowThresholdPercent: 80   # GC 到 80% 停止
imageMinimumGCAge: 2m            # 镜像至少存活 2 分钟
```

**激进策略** (磁盘空间紧张):
```yaml
imageGCHighThresholdPercent: 70  # 提前触发 GC
imageGCLowThresholdPercent: 60   # 更彻底清理
imageMinimumGCAge: 1m            # 加速清理
```

**宽松策略** (磁盘空间充足,减少镜像拉取):
```yaml
imageGCHighThresholdPercent: 90  # 延迟触发 GC
imageGCLowThresholdPercent: 85
imageMinimumGCAge: 5m            # 保留镜像更久
```

**监控指标**:
```bash
# Kubelet metrics
kubelet_volume_stats_available_bytes / kubelet_volume_stats_capacity_bytes

# 磁盘空间告警
- alert: NodeDiskPressure
  expr: kubelet_volume_stats_available_bytes / kubelet_volume_stats_capacity_bytes < 0.15
  for: 5m
  annotations:
    summary: "Node disk pressure on {{ $labels.node }}"
```

**手动清理**:
```bash
# 清理未使用的镜像
crictl rmi --prune

# 查看镜像列表
crictl images
```

---

## 8. 常见问题 (FAQ)

### Q1: KubeletConfiguration 修改后需要重启 Kubelet 吗?

**A**: 是的,需要重启 Kubelet。

**原因**: KubeletConfiguration 是静态配置文件,仅在 Kubelet 启动时加载一次。

**安全重启流程**:
1. **驱逐 Pod** (可选):
   ```bash
   kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
   ```

2. **修改配置文件**:
   ```bash
   vim /etc/kubernetes/kubelet-config.yaml
   ```

3. **重启 Kubelet**:
   ```bash
   systemctl restart kubelet
   ```

4. **验证配置生效**:
   ```bash
   kubectl get node <node-name> -o yaml | grep -A 20 allocatable
   ```

5. **恢复调度** (如果执行了 drain):
   ```bash
   kubectl uncordon <node-name>
   ```

---

### Q2: IPVS 模式下为什么还有 iptables 规则?

**A**: IPVS 仅处理负载均衡,其他网络功能仍需 iptables 辅助。

**iptables 辅助功能**:
- **SNAT 标记**: `KUBE-MARK-MASQ` 链标记需要 SNAT 的数据包
- **NodePort 流量**: `KUBE-NODEPORTS` 链处理 NodePort 入口流量
- **本地流量检测**: `KUBE-IPVS-LOCAL` 链判断流量是否来自本节点 Pod
- **Service CIDR 匹配**: 使用 ipset `KUBE-CLUSTER-IP` 快速匹配 Service IP

**规则数量对比**:
- **纯 iptables**: 10000+ 条规则
- **IPVS + iptables**: 100-200 条规则 (辅助规则)

**查看辅助规则**:
```bash
iptables -t nat -L KUBE-SERVICES -n
```

---

### Q3: CPU Manager Static 策略为什么要求整数核心?

**A**: 保证 CPU 亲和性,避免核心共享。

**技术原因**:
- **Linux CPU 亲和性**: 进程只能绑定到整数个 CPU 核心 (如 `taskset -c 2-5`)
- **独占性保证**: 如果允许 `cpu: "1.5"`,无法保证独占 (0.5 核无法独占)
- **性能优化**: 独占核心可充分利用 L1/L2 cache,减少上下文切换

**示例**:
```yaml
# ✅ 正确: 整数核心
resources:
  requests:
    cpu: "4"
  limits:
    cpu: "4"

# ❌ 错误: 非整数核心
resources:
  requests:
    cpu: "2.5"  # Static 策略会忽略此 Pod
  limits:
    cpu: "2.5"
```

**验证独占 CPU**:
```bash
# 查看 Pod 绑定的 CPU
cat /proc/<pid>/status | grep Cpus_allowed_list
# 输出: Cpus_allowed_list: 2-5 (独占核心 2-5)
```

---

### Q4: Conntrack 表满导致 Service 不可用,如何排查?

**A**: 分 4 步排查和解决。

**1. 确认 conntrack 表满**:
```bash
# 查看内核日志
dmesg | grep nf_conntrack
# nf_conntrack: table full, dropping packet

# 查看使用率
cat /proc/sys/net/netfilter/nf_conntrack_count
cat /proc/sys/net/netfilter/nf_conntrack_max
# 如果 count ≈ max,说明表满
```

**2. 临时扩容** (立即生效):
```bash
# 增大 conntrack 表
sysctl -w net.netfilter.nf_conntrack_max=1048576

# 减小超时时间 (加速回收)
sysctl -w net.netfilter.nf_conntrack_tcp_timeout_established=3600
```

**3. 永久配置** (写入 KubeProxyConfiguration):
```yaml
conntrack:
  maxPerCore: 65536  # 永久增大
  tcpEstablishedTimeout: 3600s
```

**4. 监控和告警**:
```promql
# Prometheus 告警规则
- alert: ConntrackTableNearlyFull
  expr: node_nf_conntrack_entries / node_nf_conntrack_entries_limit > 0.8
  for: 5m
```

---

### Q5: 多调度器 Profile 的 Pod 如何选择调度器?

**A**: 通过 `spec.schedulerName` 字段指定。

**配置多调度器**:
```yaml
# KubeSchedulerConfiguration
profiles:
  - schedulerName: default-scheduler
  - schedulerName: gpu-scheduler
```

**Pod 使用指定调度器**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gpu-workload
spec:
  schedulerName: gpu-scheduler  # 指定使用 gpu-scheduler
  containers:
    - name: app
      image: tensorflow/tensorflow:latest-gpu
      resources:
        limits:
          nvidia.com/gpu: 1
```

**默认行为**:
- 如果 Pod 未指定 `schedulerName`,使用 `default-scheduler`
- 如果指定的调度器不存在,Pod 会 Pending

**查看 Pod 使用的调度器**:
```bash
kubectl get pod gpu-workload -o yaml | grep schedulerName
```

---

### Q6: KubeProxy IPVS strictARP 为什么对 MetalLB 是必需的?

**A**: 避免 ARP 欺骗导致的流量黑洞。

**问题场景** (strictARP=false):
1. MetalLB 在节点 A 上宣告 Service IP `192.168.1.100`
2. 客户端发送 ARP 请求: "Who has 192.168.1.100?"
3. **节点 A 和节点 B 都响应 ARP** (因为 Kube-Proxy 在两个节点都配置了该 IP)
4. 客户端随机选择节点 B 的 MAC 地址
5. 流量发送到节点 B,但 MetalLB 仅在节点 A 上宣告,**流量丢失**

**解决方案** (strictARP=true):
```yaml
ipvs:
  strictARP: true  # 仅节点 A 响应 ARP (本地有该 Service 的 Endpoint)
```

**所需内核参数**:
```bash
# /etc/sysctl.d/99-kube-proxy.conf
net.ipv4.conf.all.arp_ignore=1      # 仅响应目标 IP 为本机 IP 的 ARP
net.ipv4.conf.all.arp_announce=2    # 使用最佳本地地址作为 ARP 源 IP
```

**验证**:
```bash
# 查看 ARP 响应
tcpdump -i eth0 arp

# 仅应看到一个节点响应 ARP
```

---

## 9. 生产案例

### 9.1 高密度节点配置 (250 Pods)

**场景**: 高性能服务器 (64 核 256GB 内存),运行 250 个微服务 Pod。

**KubeletConfiguration**:
```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# ========================================
# 容器运行时
# ========================================
cgroupDriver: systemd
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock

# 镜像拉取优化 (并行拉取)
serializeImagePulls: false
maxParallelImagePulls: 10  # 允许 10 个并行拉取

# ========================================
# Pod 资源限制
# ========================================
maxPods: 250  # 高密度配置 (需配合 CNI IP 池)
podPidsLimit: 2048  # 限制单 Pod 进程数

# ========================================
# 驱逐策略 (激进清理)
# ========================================
evictionHard:
  memory.available: "500Mi"   # 内存充足,提高阈值
  nodefs.available: "5%"      # 磁盘空间紧张
  imagefs.available: "5%"
  pid.available: "5%"

evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "10%"

evictionSoftGracePeriod:
  memory.available: "30s"  # 缩短观察窗口
  nodefs.available: "1m"

evictionMinimumReclaim:
  memory.available: "1Gi"
  nodefs.available: "2Gi"

# ========================================
# 资源预留 (高密度优化)
# ========================================
systemReserved:
  cpu: "2000m"     # 预留 2 核给系统
  memory: "4Gi"    # 预留 4Gi 给系统
  pid: "1000"

kubeReserved:
  cpu: "2000m"     # 预留 2 核给 Kubernetes
  memory: "4Gi"    # 预留 4Gi 给 Kubernetes
  pid: "1000"

enforceNodeAllocatable:
  - pods

# ========================================
# 镜像 GC (频繁清理)
# ========================================
imageGCHighThresholdPercent: 70  # 提前触发 GC
imageGCLowThresholdPercent: 60
imageMinimumGCAge: 1m

# ========================================
# 日志管理 (限制日志大小)
# ========================================
containerLogMaxSize: 5Mi   # 单文件 5Mi (高密度场景)
containerLogMaxFiles: 3    # 仅保留 3 个文件

# ========================================
# 节点状态上报 (减少 API 压力)
# ========================================
nodeStatusReportFrequency: 10m  # 10 分钟上报一次
nodeStatusUpdateFrequency: 30s

# ========================================
# 认证授权
# ========================================
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true

authorization:
  mode: Webhook

# ========================================
# 证书轮换
# ========================================
rotateCertificates: true
serverTLSBootstrap: true
```

**CNI 配置** (Calico 示例):
```yaml
# 增大节点 IP 池
apiVersion: crd.projectcalico.org/v1
kind: IPPool
metadata:
  name: default-ipv4-ippool
spec:
  cidr: 10.244.0.0/16
  blockSize: 24  # 每节点 /24 子网 (254 个 IP)
  ipipMode: Never
  natOutgoing: true
```

**预期资源分配**:
```
Capacity:
  CPU: 64 核
  Memory: 256Gi

Allocatable (扣除预留):
  CPU: 60 核 (64 - 2 - 2)
  Memory: 248Gi (256 - 4 - 4)

单 Pod 平均资源:
  CPU: 60 / 250 = 240m
  Memory: 248 / 250 = 1Gi
```

---

### 9.2 CPU 密集型工作负载优化

**场景**: 延迟敏感的实时交易系统,要求 CPU 独占和 NUMA 本地内存。

**KubeletConfiguration**:
```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

cgroupDriver: systemd
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock

# ========================================
# CPU 管理 (Static 策略)
# ========================================
cpuManagerPolicy: static
cpuManagerReconcilePeriod: 10s
reservedSystemCPUs: "0-3"  # 核心 0-3 预留给系统 (共 4 核)

# ========================================
# 内存管理 (Static 策略,NUMA 优化)
# ========================================
memoryManagerPolicy: Static
reservedMemory:
  - numaNode: 0
    limits:
      memory: "2Gi"  # NUMA 节点 0 预留 2Gi
  - numaNode: 1
    limits:
      memory: "2Gi"  # NUMA 节点 1 预留 2Gi

# ========================================
# 拓扑管理器 (单 NUMA 节点对齐)
# ========================================
topologyManagerPolicy: single-numa-node  # 强制单 NUMA
topologyManagerScope: pod                # 整个 Pod 对齐

# ========================================
# 资源预留
# ========================================
systemReserved:
  cpu: "2000m"
  memory: "4Gi"

kubeReserved:
  cpu: "2000m"
  memory: "4Gi"

enforceNodeAllocatable:
  - pods

# ========================================
# 特性门控
# ========================================
featureGates:
  CPUManager: true          # v1.26+ 默认 GA
  MemoryManager: true       # v1.22+ Beta
  TopologyManager: true     # v1.27+ GA

# 其他配置...
maxPods: 50  # 限制 Pod 数量 (CPU 独占场景)
```

**Pod 配置** (Guaranteed QoS + 整数 CPU):
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: trading-engine
spec:
  containers:
    - name: engine
      image: trading-engine:v1.0
      resources:
        requests:
          cpu: "8"      # 整数核心,独占 8 核
          memory: "16Gi"
        limits:
          cpu: "8"      # requests = limits (Guaranteed QoS)
          memory: "16Gi"
  nodeSelector:
    node-type: high-performance  # 调度到高性能节点
```

**验证 CPU 独占**:
```bash
# 查看 Pod 进程的 CPU 亲和性
PID=$(crictl inspect <container-id> | jq -r '.info.pid')
cat /proc/$PID/status | grep Cpus_allowed_list

# 输出: Cpus_allowed_list: 4-11 (独占核心 4-11)
```

**性能提升**:
- **延迟**: P99 延迟降低 25% (避免 CPU 抢占)
- **吞吐量**: 提升 15% (L1/L2 cache 命中率提高)

---

### 9.3 IPVS 高性能代理 + MetalLB

**场景**: 裸金属 Kubernetes 集群,使用 MetalLB 暴露 LoadBalancer 服务。

**KubeProxyConfiguration**:
```yaml
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration

# ========================================
# IPVS 模式 (高性能)
# ========================================
mode: "ipvs"

ipvs:
  scheduler: "rr"      # 轮询算法
  strictARP: true      # ⚠️ MetalLB 必需
  minSyncPeriod: 1s
  syncPeriod: 10s
  
  # 超时优化
  tcpTimeout: 900s
  tcpFinTimeout: 120s
  udpTimeout: 300s

# ========================================
# Conntrack 优化
# ========================================
conntrack:
  maxPerCore: 65536
  min: 524288
  tcpEstablishedTimeout: 3600s
  tcpCloseWaitTimeout: 1800s

# ========================================
# 集群网络
# ========================================
clusterCIDR: "10.244.0.0/16"

# ========================================
# 健康检查和监控
# ========================================
healthzBindAddress: "0.0.0.0:10256"
metricsBindAddress: "0.0.0.0:10249"

# ========================================
# 日志
# ========================================
logging:
  format: json
  verbosity: 2
```

**内核参数** (strictARP 必需):
```bash
# /etc/sysctl.d/99-kube-proxy.conf
net.ipv4.conf.all.arp_ignore=1
net.ipv4.conf.all.arp_announce=2

# 应用配置
sysctl -p /etc/sysctl.d/99-kube-proxy.conf
```

**MetalLB 配置**:
```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  namespace: metallb-system
  name: config
data:
  config: |
    address-pools:
      - name: default
        protocol: layer2
        addresses:
          - 192.168.1.100-192.168.1.200  # 外部 IP 池
```

**LoadBalancer Service**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx
spec:
  type: LoadBalancer  # MetalLB 自动分配外部 IP
  ports:
    - port: 80
      targetPort: 80
  selector:
    app: nginx
```

**验证**:
```bash
# 查看 Service 的外部 IP
kubectl get svc nginx
# NAME    TYPE           CLUSTER-IP     EXTERNAL-IP      PORT(S)
# nginx   LoadBalancer   10.96.0.1      192.168.1.100    80:30080/TCP

# 查看 IPVS 规则
ipvsadm -Ln | grep 192.168.1.100
# TCP  192.168.1.100:80 rr
#   -> 10.244.1.2:80    Masq  1  0  0

# 测试外部访问
curl http://192.168.1.100
```

---

### 9.4 多调度器架构

**场景**: 集群运行多种工作负载 (Web 服务、批处理、AI 训练),需要不同的调度策略。

**KubeSchedulerConfiguration**:
```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration

# ========================================
# Profile 1: 默认调度器 (Web 服务,资源均衡)
# ========================================
profiles:
  - schedulerName: default-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesBalancedAllocation
            weight: 10  # 优先均衡分布
          - name: NodeResourcesLeastAllocated
            weight: 5
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: LeastAllocated  # 优先低负载节点
      - name: PodTopologySpread
        args:
          defaultConstraints:
            - maxSkew: 1
              topologyKey: topology.kubernetes.io/zone
              whenUnsatisfiable: ScheduleAnyway

  # ========================================
  # Profile 2: 批处理调度器 (资源紧凑,节省成本)
  # ========================================
  - schedulerName: batch-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesMostAllocated
            weight: 10  # 优先高负载节点 (紧凑分布)
        disabled:
          - name: NodeResourcesBalancedAllocation
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: MostAllocated  # 资源紧凑

  # ========================================
  # Profile 3: AI 训练调度器 (GPU 亲和性)
  # ========================================
  - schedulerName: gpu-scheduler
    plugins:
      filter:
        enabled:
          - name: NodeResourcesFit  # 检查 GPU 资源
      score:
        enabled:
          - name: NodeResourcesLeastAllocated
            weight: 10
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: LeastAllocated
            resources:
              - name: nvidia.com/gpu
                weight: 10  # GPU 权重最高
              - name: cpu
                weight: 1
              - name: memory
                weight: 1

# ========================================
# 全局配置
# ========================================
percentageOfNodesToScore: 50  # 采样 50% 节点

leaderElection:
  leaderElect: true
  leaseDuration: 15s
  renewDeadline: 10s
  retryPeriod: 2s
```

**使用示例**:

**Web 服务** (default-scheduler):
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 10
  template:
    spec:
      # schedulerName: default-scheduler (默认,可省略)
      containers:
        - name: web
          image: nginx
          resources:
            requests:
              cpu: "500m"
              memory: "512Mi"
```

**批处理任务** (batch-scheduler):
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: data-processing
spec:
  template:
    spec:
      schedulerName: batch-scheduler  # 使用批处理调度器
      restartPolicy: Never
      containers:
        - name: processor
          image: data-processor:v1.0
          resources:
            requests:
              cpu: "4"
              memory: "8Gi"
```

**AI 训练任务** (gpu-scheduler):
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ai-training
spec:
  schedulerName: gpu-scheduler  # 使用 GPU 调度器
  containers:
    - name: trainer
      image: tensorflow/tensorflow:latest-gpu
      resources:
        requests:
          nvidia.com/gpu: 2  # 请求 2 个 GPU
          cpu: "8"
          memory: "32Gi"
        limits:
          nvidia.com/gpu: 2
          cpu: "8"
          memory: "32Gi"
```

**调度器性能监控**:
```bash
# 查看调度延迟
kubectl get --raw /metrics | grep scheduler_scheduling_duration_seconds

# 查看调度失败率
kubectl get --raw /metrics | grep scheduler_schedule_attempts_total
```

---

**本文档结束**,涵盖了 Kubernetes v1.25-v1.32 的完整组件配置参考,包括:
- **KubeletConfiguration**: 节点资源管理、驱逐策略、CPU/内存拓扑优化
- **KubeProxyConfiguration**: iptables/IPVS/nftables 模式、conntrack 优化
- **KubeSchedulerConfiguration**: 多 Profile 架构、插件配置、Extender 扩展
- **KubeControllerManagerConfiguration**: CLI flags 参考
- **版本兼容性矩阵**: API 版本演进、Feature Gates 时间线
- **最佳实践**: 大规模集群调优、conntrack 计算、CPU Manager 使用
- **生产案例**: 高密度节点、CPU 密集型优化、IPVS+MetalLB、多调度器架构

**推荐实践**:
1. 使用 **IPVS 模式** (Services > 100 时)
2. 配置 **完整的资源预留和驱逐策略** (生产环境必需)
3. 为性能敏感应用启用 **CPU Manager + Topology Manager**
4. 使用 **多调度器 Profile** 支持异构工作负载
5. 监控 **conntrack 表使用率** (避免表满导致服务中断)
