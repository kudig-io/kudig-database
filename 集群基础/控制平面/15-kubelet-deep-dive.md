---
title: kubelet 深度解析 (kubelet Deep Dive)
description: 深入解析 kubelet 的架构设计、PLEG、CRI 交互、资源管理、驱逐策略、证书轮换与生产级运维排障。
summary: 深入解析 kubelet 的架构设计、PLEG、CRI 交互、资源管理、驱逐策略、证书轮换与生产级运维排障。
category: general
tags:
- k8s
- control-plane
- deep-dive
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- containerd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- 15-kubelet-deep-dive的工作原理是什么？
- 15-kubelet-deep-dive的内部机制详解
- 15-kubelet-deep-dive的技术深度分析
trigger_keywords:
- kubelet
- 深度解析
- kubelet
- Deep
- Dive
- cluster
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubelet 深度解析 (kubelet Deep Dive)

> kubelet 是 Kubernetes 中运行在每个节点上的核心代理，负责管理节点上的 Pod 和容器生命周期

---

<!-- chunk: 1. 架构概述 (Architecture Overview) -->
## 1. 架构概述 (Architecture Overview)

### 1.1 核心职责

| 职责 | 英文名 | 说明 |
|:---|:---|:---|
| **Pod生命周期管理** | Pod Lifecycle | 创建、启动、停止、删除Pod |
| **容器运行时交互** | Container Runtime | 通过CRI与containerd/CRI-O通信 |
| **资源管理** | Resource Management | CPU、内存、存储资源分配与限制 |
| **健康检查** | Health Probing | Liveness、Readiness、Startup探针 |
| **节点状态报告** | Node Status | 定期向API Server汇报节点状态 |
| **卷管理** | Volume Management | 挂载/卸载Pod所需存储卷 |
| **日志和监控** | Logging/Metrics | 容器日志收集、暴露指标 |
| **设备插件** | Device Plugins | GPU等特殊硬件资源管理 |
| **镜像管理** | Image Management | 镜像拉取、清理 |
| **静态Pod管理** | Static Pods | 通过manifest目录管理 |
| **拓扑管理** | Topology Manager | NUMA感知资源协调 |

### 1.2 整体架构

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌────────────────────────────────────────────────────────────────────────┐
│                              kubelet                                    │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                        API Server Client                          │  │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐                  │  │
│  │  │   Watch    │  │   Update   │  │   Report   │                  │  │
│  │  │   Pods     │  │   Status   │  │   Node     │                  │  │
│  │  └────────────┘  └────────────┘  └────────────┘                  │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                 │                                       │
│  ┌──────────────────────────────┴─────────────────────────────────┐   │
│  │                     Pod Lifecycle Manager                       │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────────────────────┐│   │
│  │  │   PLEG     │  │   Sync     │  │    Status Manager          ││   │
│  │  │ (PodLifecycle│  │   Loop     │  │                           ││   │
│  │  │  EventGen) │  │            │  │                            ││   │
│  │  └────────────┘  └────────────┘  └────────────────────────────┘│   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                 │                                       │
│  ┌──────────────────────────────┴─────────────────────────────────┐   │
│  │                       Sub-Managers                              │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────────────────┐│   │
│  │  │  Prober  │ │  Volume  │ │  Image   │ │   Device Plugin     ││   │
│  │  │  Manager │ │  Manager │ │  Manager │ │   Manager           ││   │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────────────────┘│   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────────────────┐│   │
│  │  │   Evict  │ │  Secret  │ │ConfigMap │ │   Resource/cgroup   ││   │
│  │  │  Manager │ │  Manager │ │  Manager │ │   Manager           ││   │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────────────────┘│   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                                 │                                       │
│                                 │ CRI (Container Runtime Interface)    │
│                                 ▼                                       │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    Container Runtime                             │   │
│  │         containerd / CRI-O / docker (deprecated)                │   │
│  └─────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────────┘
                                  │
                                  │ OCI
                                  ▼
┌────────────────────────────────────────────────────────────────────────┐
│                          Low-Level Runtime                              │
│                          runc / kata / gVisor                          │
└────────────────────────────────────────────────────────────────────────┘
```
### 1.3 关键组件说明

| 组件 | 英文名 | 职责 |
|:---|:---|:---|
| **PLEG** | Pod Lifecycle Event Generator | 检测容器状态变化，生成事件 |
| **SyncLoop** | Sync Loop | 主循环，处理Pod同步 |
| **ProbeManager** | Probe Manager | 执行健康检查探针 |
| **VolumeManager** | Volume Manager | 管理卷的挂载和卸载 |
| **ImageManager** | Image Manager | 管理容器镜像 |
| **EvictionManager** | Eviction Manager | 资源压力时驱逐Pod |
| **StatusManager** | Status Manager | 同步Pod状态到API Server |
| **SecretManager** | Secret Manager | 管理Secret的同步 |
| **ConfigMapManager** | ConfigMap Manager | 管理ConfigMap的同步 |
| **DevicePluginManager** | Device Plugin Manager | 管理设备插件 |

---

<!-- chunk: 2. Pod 生命周期管理 (Pod Lifecycle) -->
## 2. Pod 生命周期管理 (Pod Lifecycle)

### 2.1 Pod 同步流程

```
API Server Watch Event (Pod变化)
        │
        ▼
┌───────────────────────────────────────────────────────────────┐
│                    SyncLoop (主循环)                          │
│                                                               │
│  Event Sources:                                               │
│  ├─ configCh: API Server的Pod配置                            │
│  ├─ syncCh: 周期性同步 (默认1s)                              │
│  ├─ housekeepingCh: 清理任务 (默认2s)                        │
│  ├─ plegCh: PLEG事件 (容器状态变化)                          │
│  └─ livenessManager: 存活探针失败事件                        │
│                                                               │
└───────────────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────────┐
│                   SyncPod (单Pod同步)                         │
│                                                               │
│  1. 计算期望状态 vs 实际状态                                  │
│  2. 创建/更新 Pod Sandbox (pause容器)                         │
│  3. 创建/更新 Init Containers (顺序)                         │
│  4. 创建/更新 Regular Containers (并行)                      │
│  5. 启动探针检查                                              │
│  6. 更新Pod状态                                               │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

### 2.2 容器状态流转

```
                    ┌─────────────┐
                    │   Waiting   │
                    │ (等待创建)   │
                    └──────┬──────┘
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
         ▼                 ▼                 ▼
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ContainerCreating│   ImagePullBackOff │   ErrImagePull │
└──────┬──────┘    └─────────────┘    └─────────────┘
       │
       ▼
┌─────────────┐
│   Running   │◀──────────────────────────────────┐
└──────┬──────┘                                   │
       │                                          │
       ├─────────────────┬────────────────┬──────┘
       │                 │                │  (重启策略)
       ▼                 ▼                ▼
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│ Completed   │  │  Error      │  │ CrashLoop   │
│ (正常退出)   │  │ (异常退出)   │  │ BackOff     │
└─────────────┘  └─────────────┘  └─────────────┘
```

### 2.3 PLEG (Pod Lifecycle Event Generator)

| 事件类型 | 说明 | 触发条件 |
|:---|:---|:---|
| **ContainerStarted** | 容器启动 | 容器状态从非Running变为Running |
| **ContainerDied** | 容器死亡 | 容器状态从Running变为非Running |
| **ContainerRemoved** | 容器移除 | 容器从运行时删除 |
| **ContainerChanged** | 容器变化 | 容器配置发生变化 |
| **PodSync** | Pod同步 | 需要重新同步Pod状态 |

```bash
# PLEG 工作参数
--pleg-relist-period=1s          # PLEG重新列举周期
--pod-manifest-path=/etc/kubernetes/manifests  # 静态Pod目录
```

---

<!-- chunk: 3. 健康检查探针 (Health Probes) -->
## 3. 健康检查探针 (Health Probes)

### 3.1 探针类型对比

| 探针类型 | 英文名 | 用途 | 失败后果 |
|:---|:---|:---|:---|
| **存活探针** | Liveness Probe | 检测容器是否存活 | 重启容器 |
| **就绪探针** | Readiness Probe | 检测容器是否就绪 | 从Service端点移除 |
| **启动探针** | Startup Probe | 检测容器是否完成启动 | 阻止其他探针执行 |

### 3.2 探针检查方式

| 方式 | 说明 | 适用场景 |
|:---|:---|:---|
| **HTTP GET** | 发送HTTP请求，2xx/3xx为成功 | Web服务 |
| **TCP Socket** | 建立TCP连接，连接成功为成功 | 数据库、缓存 |
| **Exec** | 在容器内执行命令，退出码0为成功 | 复杂检查逻辑 |
| **gRPC** | gRPC健康检查协议 | gRPC服务 |

### 3.3 探针配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: probe-demo
spec:
  containers:
  - name: app
    image: my-app:latest
    ports:
    - containerPort: 8080
    
    # 存活探针 - 检测死锁等问题
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
        httpHeaders:
        - name: Custom-Header
          value: Awesome
      initialDelaySeconds: 15    # 首次检查延迟
      periodSeconds: 10          # 检查周期
      timeoutSeconds: 3          # 超时时间
      successThreshold: 1        # 成功阈值
      failureThreshold: 3        # 失败阈值
    
    # 就绪探针 - 检测是否可接收流量
    readinessProbe:
      tcpSocket:
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 5
      timeoutSeconds: 2
      successThreshold: 1
      failureThreshold: 3
    
    # 启动探针 - 慢启动应用
    startupProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 0
      periodSeconds: 10
      timeoutSeconds: 3
      successThreshold: 1
      failureThreshold: 30       # 允许5分钟启动时间

---
# gRPC 健康检查示例 (K8s 1.24+)
apiVersion: v1
kind: Pod
metadata:
  name: grpc-probe-demo
spec:
  containers:
  - name: grpc-app
    image: my-grpc-app:latest
    livenessProbe:
      grpc:
        port: 9090
        service: ""              # 空字符串检查整体健康
      initialDelaySeconds: 10
      periodSeconds: 10
```

### 3.4 探针最佳实践

| 最佳实践 | 说明 |
|:---|:---|
| 分离健康检查端点 | 不要在健康检查端点执行重操作 |
| 合理设置延迟 | initialDelaySeconds应大于应用启动时间 |
| 使用Startup Probe | 慢启动应用避免被Liveness杀死 |
| 不依赖外部服务 | 健康检查不应依赖数据库等外部服务 |
| 设置合理超时 | 超时时间应小于检查周期 |
| 区分存活和就绪 | 存活检测死锁，就绪检测是否可服务 |

---

<!-- chunk: 4. 关键配置参数 (Configuration Parameters) -->
## 4. 关键配置参数 (Configuration Parameters)

### 4.1 通用参数

| 参数 | 默认值 | 推荐值 | 说明 |
|:---|:---|:---|:---|
| `--config` | - | /var/lib/kubelet/config.yaml | 配置文件路径 |
| `--kubeconfig` | - | /etc/kubernetes/kubelet.conf | API Server连接配置 |
| `--container-runtime-endpoint` | - | unix:///run/containerd/containerd.sock | 容器运行时端点 |
| `--hostname-override` | 主机名 | - | 覆盖节点主机名 |
| `--node-ip` | 自动检测 | 节点IP | 节点IP地址 |
| `--cloud-provider` | - | external | 云提供商模式 |
| `--register-node` | true | true | 自动注册节点 |
| `--register-with-taints` | - | - | 注册时添加的Taint |

### 4.2 资源管理参数

| 参数 | 默认值 | 推荐值 | 说明 |
|:---|:---|:---|:---|
| `--kube-reserved` | 无 | cpu=100m,memory=256Mi | Kubernetes组件预留 |
| `--system-reserved` | 无 | cpu=100m,memory=256Mi | 系统进程预留 |
| `--eviction-hard` | 见下 | 根据节点调整 | 硬驱逐阈值 |
| `--eviction-soft` | 无 | 见下 | 软驱逐阈值 |
| `--eviction-soft-grace-period` | 无 | 见下 | 软驱逐宽限期 |
| `--max-pods` | 110 | 根据节点调整 | 节点最大Pod数 |
| `--pods-per-core` | 0 | 0 | 每核心Pod数限制 |
| `--enforce-node-allocatable` | pods | pods,kube-reserved,system-reserved | 强制分配策略 |

```yaml
# 驱逐配置示例 (KubeletConfiguration)
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"

evictionSoft:
  memory.available: "500Mi"
  nodefs.available: "15%"

evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"

evictionPressureTransitionPeriod: "5m"
```

### 4.3 Pod管理参数

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `--pod-manifest-path` | - | 静态Pod配置目录 |
| `--file-check-frequency` | 20s | 静态Pod检查频率 |
| `--sync-frequency` | 1m | Pod配置同步频率 |
| `--max-open-files` | 1000000 | 最大打开文件数 |
| `--serialize-image-pulls` | true | 串行拉取镜像 |
| `--image-pull-progress-deadline` | 1m | 镜像拉取超时 |
| `--streaming-connection-idle-timeout` | 4h | 流式连接空闲超时 |

### 4.4 网络参数

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `--cluster-dns` | - | 集群DNS服务IP |
| `--cluster-domain` | cluster.local | 集群域名 |
| `--resolv-conf` | /etc/resolv.conf | DNS解析配置 |
| `--network-plugin` | - | 已弃用，使用CNI |
| `--cni-bin-dir` | /opt/cni/bin | CNI插件目录 |
| `--cni-conf-dir` | /etc/cni/net.d | CNI配置目录 |
| `--hairpin-mode` | promiscuous-bridge | Hairpin NAT模式 |

### 4.5 安全参数

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `--anonymous-auth` | false | 匿名访问(kubelet API) |
| `--authentication-token-webhook` | true | Webhook Token认证 |
| `--authorization-mode` | Webhook | 授权模式 |
| `--client-ca-file` | - | 客户端CA证书 |
| `--tls-cert-file` | - | Kubelet服务器证书 |
| `--tls-private-key-file` | - | Kubelet服务器私钥 |
| `--rotate-certificates` | true | 证书自动轮换 |
| `--protect-kernel-defaults` | false | 保护内核默认值 |
| `--make-iptables-util-chains` | true | 创建iptables链 |

---

<!-- chunk: 5. 配置文件方式 (KubeletConfiguration) -->
## 5. 配置文件方式 (KubeletConfiguration)

### 5.1 完整配置示例

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 认证授权
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
    cacheTTL: "2m"
  x509:
    clientCAFile: "/etc/kubernetes/pki/ca.crt"

authorization:
  mode: Webhook
  webhook:
    cacheAuthorizedTTL: "5m"
    cacheUnauthorizedTTL: "30s"

# 集群DNS
clusterDNS:
  - "10.96.0.10"
clusterDomain: "cluster.local"
resolvConf: "/etc/resolv.conf"

# 资源管理
kubeReserved:
  cpu: "100m"
  memory: "256Mi"
  ephemeral-storage: "1Gi"
systemReserved:
  cpu: "100m"
  memory: "256Mi"
  ephemeral-storage: "1Gi"

# Pod配置
maxPods: 110
podPidsLimit: 4096
cpuManagerPolicy: "static"     # none/static
cpuManagerReconcilePeriod: "10s"
memoryManagerPolicy: "None"    # None/Static
topologyManagerPolicy: "none"  # none/best-effort/restricted/single-numa-node

# 驱逐配置
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "500Mi"
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"
evictionPressureTransitionPeriod: "5m"
evictionMaxPodGracePeriod: 30

# 镜像GC
imageMinimumGCAge: "2m"
imageGCHighThresholdPercent: 85
imageGCLowThresholdPercent: 80

# 容器日志
containerLogMaxSize: "10Mi"
containerLogMaxFiles: 5

# CGroups
cgroupDriver: "systemd"        # cgroupfs/systemd
cgroupsPerQOS: true
cgroupRoot: "/"
enforceNodeAllocatable:
  - "pods"
  - "kube-reserved"
  - "system-reserved"

# 特性门控
featureGates:
  GracefulNodeShutdown: true
  MemoryManager: true
  CPUManager: true
  TopologyManager: true

# 优雅关闭
shutdownGracePeriod: "30s"
shutdownGracePeriodCriticalPods: "10s"

# 日志
logging:
  format: "json"
  sanitization: false
  options:
    json:
      infoBufferSize: "0"

# 健康检查
healthzPort: 10248
healthzBindAddress: "127.0.0.1"

# 只读端口 (不推荐启用)
readOnlyPort: 0
```

---

<!-- chunk: 6. cgroup 管理 (cgroup Management) -->
## 6. cgroup 管理 (cgroup Management)

### 6.1 cgroup 驱动对比

| 驱动 | 说明 | 推荐 |
|:---|:---|:---|
| **cgroupfs** | kubelet直接操作cgroup文件系统 | 不推荐 |
| **systemd** | 通过systemd管理cgroup | 推荐(与系统一致) |

```bash
# 检查当前cgroup驱动
cat /var/lib/kubelet/config.yaml | grep cgroupDriver

# 检查容器运行时cgroup驱动 (containerd)
cat /etc/containerd/config.toml | grep SystemdCgroup

# 确保kubelet和容器运行时使用相同的cgroup驱动
```

### 6.2 cgroup v1 vs v2

| 特性 | cgroup v1 | cgroup v2 |
|:---|:---|:---|
| **层级结构** | 多层级(每控制器一个) | 单一统一层级 |
| **资源控制** | 分散在不同控制器 | 统一接口 |
| **Kubernetes支持** | 完全支持 | 1.25+ 稳定支持 |
| **推荐** | 兼容性好 | 新部署推荐 |

```bash
# 检查系统使用的cgroup版本
stat -fc %T /sys/fs/cgroup/

# cgroup v1: tmpfs
# cgroup v2: cgroup2fs

# 或者
mount | grep cgroup
```

### 6.3 Pod QoS 与 cgroup

| QoS 类别 | 条件 | cgroup 位置 | 驱逐优先级 |
|:---|:---|:---|:---|
| **Guaranteed** | requests = limits (全部资源) | `/kubepods/pod<uid>` | 最后 |
| **Burstable** | requests < limits 或部分设置 | `/kubepods/burstable/pod<uid>` | 中等 |
| **BestEffort** | 未设置requests和limits | `/kubepods/besteffort/pod<uid>` | 最先 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看Pod的cgroup
# systemd cgroup driver
cat /sys/fs/cgroup/memory/kubepods.slice/kubepods-burstable.slice/kubepods-burstable-pod<uid>.slice/memory.limit_in_bytes

# 或使用crictl
crictl inspect <container-id> | jq .info.runtimeSpec.linux.cgroupsPath
```
---

<!-- chunk: 7. 监控指标 (Monitoring Metrics) -->
## 7. 监控指标 (Monitoring Metrics)

### 7.1 关键指标表

| 指标名称 | 类型 | 说明 | 告警阈值 |
|:---|:---|:---|:---|
| `kubelet_running_pods` | Gauge | 运行中的Pod数 | - |
| `kubelet_running_containers` | Gauge | 运行中的容器数 | - |
| `kubelet_node_name` | Gauge | 节点名称(标签) | - |
| `kubelet_pleg_relist_duration_seconds` | Histogram | PLEG重列举耗时 | p99 > 1s |
| `kubelet_pleg_relist_interval_seconds` | Histogram | PLEG重列举间隔 | p99 > 3s |
| `kubelet_pod_start_duration_seconds` | Histogram | Pod启动耗时 | p99 > 60s |
| `kubelet_pod_worker_duration_seconds` | Histogram | Pod Worker耗时 | p99 > 10s |
| `kubelet_runtime_operations_total` | Counter | 运行时操作总数 | - |
| `kubelet_runtime_operations_duration_seconds` | Histogram | 运行时操作耗时 | p99 > 5s |
| `kubelet_runtime_operations_errors_total` | Counter | 运行时操作错误 | 持续增长 |
| `kubelet_cgroup_manager_duration_seconds` | Histogram | cgroup操作耗时 | p99 > 100ms |
| `kubelet_volume_stats_*` | Gauge | 卷统计信息 | - |
| `kubelet_eviction_stats_age_seconds` | Histogram | 驱逐统计 | - |
| `kubelet_http_requests_total` | Counter | HTTP请求总数 | - |

### 7.2 Prometheus 告警规则

```yaml
groups:
- name: kubelet
  rules:
  - alert: KubeletDown
    expr: absent(up{job="kubelet"} == 1)
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Kubelet is down on {{ $labels.instance }}"

  - alert: KubeletTooManyPods
    expr: kubelet_running_pods / kubelet_node_config_max_pods > 0.95
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Kubelet is running too many pods"
      description: "{{ $labels.instance }} is running {{ $value | humanizePercentage }} of max pods"

  - alert: KubeletPLEGDurationHigh
    expr: histogram_quantile(0.99, rate(kubelet_pleg_relist_duration_seconds_bucket[5m])) > 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Kubelet PLEG duration is high"
      description: "PLEG p99 duration is {{ $value }}s on {{ $labels.instance }}"

  - alert: KubeletPodStartLatencyHigh
    expr: histogram_quantile(0.99, rate(kubelet_pod_start_duration_seconds_bucket[5m])) > 60
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Kubelet pod start latency is high"

  - alert: KubeletRuntimeOperationErrors
    expr: increase(kubelet_runtime_operations_errors_total[5m]) > 10
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Kubelet runtime operations errors increasing"

  - alert: KubeletVolumePluginError
    expr: kubelet_volume_stats_inodes_free / kubelet_volume_stats_inodes < 0.1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Volume is running low on inodes"

  - alert: KubeletClientCertExpiration
    expr: kubelet_certificate_manager_client_expiration_seconds - time() < 86400 * 7
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "Kubelet client certificate expires in less than 7 days"
```

---

<!-- chunk: 8. 故障排查 (Troubleshooting) -->
## 8. 故障排查 (Troubleshooting)

### 8.1 常见问题诊断

| 症状 | 可能原因 | 诊断方法 | 解决方案 |
|:---|:---|:---|:---|
| **节点NotReady** | kubelet未运行/运行时问题 | systemctl status kubelet | 重启kubelet/运行时 |
| **Pod启动慢** | 镜像拉取慢/资源不足 | 检查events/PLEG指标 | 优化镜像/增加资源 |
| **Pod Eviction** | 资源压力 | kubectl describe node | 检查驱逐原因 |
| **容器CrashLoop** | 应用问题/资源不足 | kubectl logs/describe | 检查应用日志 |
| **镜像拉取失败** | 网络/认证问题 | crictl pull | 检查网络/凭证 |
| **卷挂载失败** | 存储问题/权限 | kubectl describe pod | 检查存储后端 |
| **PLEG不健康** | 运行时问题/高负载 | 检查PLEG指标 | 检查运行时/减少Pod |
| **OOM Kill** | 内存不足 | dmesg/journalctl | 增加limits/节点内存 |

### 8.2 诊断命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 kubelet 状态
systemctl status kubelet
journalctl -u kubelet -f --no-pager

# 检查节点状态
kubectl describe node <node-name>
kubectl get node <node-name> -o yaml

# 检查节点条件
kubectl get nodes -o custom-columns='NAME:.metadata.name,READY:.status.conditions[?(@.type=="Ready")].status,MEMORY:.status.conditions[?(@.type=="MemoryPressure")].status,DISK:.status.conditions[?(@.type=="DiskPressure")].status,PID:.status.conditions[?(@.type=="PIDPressure")].status'

# 检查 kubelet 配置
cat /var/lib/kubelet/config.yaml

# 检查容器运行时
crictl info
crictl ps -a
crictl logs <container-id>

# 检查 PLEG 健康
curl -s http://localhost:10248/healthz
curl -s http://localhost:10255/metrics | grep pleg

# 检查 kubelet API (需认证)
curl -k https://localhost:10250/healthz
curl -k https://localhost:10250/pods

# 检查 cgroup
cat /sys/fs/cgroup/memory/kubepods/memory.limit_in_bytes
cat /sys/fs/cgroup/cpu/kubepods/cpu.cfs_quota_us

# 检查静态Pod
ls -la /etc/kubernetes/manifests/

# 检查证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```
### 8.3 常见日志模式

```bash
# 正常日志
I0101 00:00:00.000000   1 kubelet.go:2400] SyncLoop (PLEG): event for pod "nginx"
I0101 00:00:00.000000   1 kubelet.go:1925] syncPod(UID: xxx) completed successfully

# 警告日志
W0101 00:00:00.000000   1 eviction_manager.go:166] attempting to evict pod; usage: 95%
W0101 00:00:00.000000   1 image_gc_manager.go:321] Failed to garbage collect images

# 错误日志
E0101 00:00:00.000000   1 kubelet.go:2472] Container runtime network not ready
E0101 00:00:00.000000   1 pod_workers.go:191] Error syncing pod xxx: failed to pull image
E0101 00:00:00.000000   1 remote_runtime.go:116] RunPodSandbox from runtime failed
```

---

<!-- chunk: 9. 性能优化 (Performance Tuning) -->
## 9. 性能优化 (Performance Tuning)

### 9.1 大规模节点优化

| 优化项 | 默认值 | 大节点推荐值 | 说明 |
|:---|:---|:---|:---|
| `--max-pods` | 110 | 250+ | 增加Pod容量 |
| `--kube-api-qps` | 50 | 100-200 | API QPS限制 |
| `--kube-api-burst` | 100 | 200-400 | API Burst限制 |
| `--serialize-image-pulls` | true | false | 并行拉取镜像 |
| `--registry-qps` | 5 | 20 | Registry QPS |
| `--registry-burst` | 10 | 40 | Registry Burst |
| `--event-qps` | 50 | 100 | Event QPS |
| `--event-burst` | 100 | 200 | Event Burst |

### 9.2 内存优化

```yaml
# 减少 ConfigMap/Secret 缓存
configMapAndSecretChangeDetectionStrategy: Watch  # Watch比Get更高效

# 启用内存管理器 (NUMA感知)
memoryManagerPolicy: Static
reservedMemory:
  - numaNode: 0
    limits:
      memory: "1Gi"
```

### 9.3 Linux 内核优化

```bash
# 文件描述符
cat >> /etc/security/limits.conf << EOF
* soft nofile 1048576
* hard nofile 1048576
root soft nofile 1048576
root hard nofile 1048576
EOF

# 内核参数
cat >> /etc/sysctl.conf << EOF
# 网络
net.bridge.bridge-nf-call-iptables = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward = 1

# 文件系统
fs.file-max = 2097152
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192

# 内存
vm.swappiness = 0
vm.overcommit_memory = 1
vm.panic_on_oom = 0

# 进程
kernel.pid_max = 4194304
kernel.threads-max = 4194304
EOF

sysctl -p
```

---

<!-- chunk: 10. 生产环境 Checklist -->
## 10. 生产环境 Checklist

### 10.1 部署检查

| 检查项 | 状态 | 说明 |
|:---|:---|:---|
| [ ] 使用配置文件方式 | | 便于管理和版本控制 |
| [ ] cgroup驱动一致 | | kubelet与运行时使用相同驱动 |
| [ ] 资源预留配置 | | 配置kube-reserved/system-reserved |
| [ ] 驱逐阈值配置 | | 防止节点资源耗尽 |
| [ ] 证书自动轮换 | | 启用rotate-certificates |
| [ ] 监控告警配置 | | PLEG、Pod启动延迟等 |
| [ ] 日志收集配置 | | 便于问题排查 |

### 10.2 安全加固

| 加固项 | 配置 |
|:---|:---|
| 禁用匿名访问 | `anonymous-auth: false` |
| 启用Webhook认证 | `authentication.webhook.enabled: true` |
| 启用Webhook授权 | `authorization.mode: Webhook` |
| 禁用只读端口 | `readOnlyPort: 0` |
| 启用TLS | 配置证书文件 |
| 保护内核默认值 | `protectKernelDefaults: true` |

---

<!-- chunk: 11. 静态 Pod (Static Pods) -->
## 11. 静态 Pod (Static Pods)

### 11.1 核心概念

静态 Pod 是由 **kubelet 直接管理**的 Pod，**不经过 API Server**，通过本地 manifest 目录中的 YAML/JSON 文件创建。每个节点上的 kubelet 独立监视其 manifest 目录，根据文件变化创建或删除对应的 Pod。

| 特性 | 说明 |
|:---|:---|
| **管理方式** | kubelet 直接管理，无需 API Server |
| **生命周期** | 删除 manifest 文件即删除 Pod |
| **可见性** | 通过 Mirror Pod 同步到 API Server（只读） |
| **调度** | 绑定到特定节点，不可调度 |
| **控制器** | 无控制器，kubelet 是唯一的控制者 |

### 11.2 管理流程

```
/etc/kubernetes/manifests/
        │
        ▼
┌─────────────────────────────────────┐
│            kubelet                   │
│  ┌───────────────────────────────┐  │
│  │   File Check (默认 20s)       │  │
│  │   读取 manifest 目录变化      │  │
│  └───────────────────────────────┘  │
│                 │                    │
│                 ▼                    │
│  ┌───────────────────────────────┐  │
│  │   SyncPod (静态 Pod)          │  │
│  │   创建/更新/删除容器          │  │
│  └───────────────────────────────┘  │
│                 │                    │
│                 ▼                    │
│  ┌───────────────────────────────┐  │
│  │   Mirror Pod 同步             │  │
│  │   创建只读副本到 API Server   │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

### 11.3 关键配置参数

| 参数 | 配置方式 | 说明 |
|:---|:---|:---|
| `--pod-manifest-path` | 命令行 | 静态 Pod manifest 目录路径 |
| `staticPodPath` | KubeletConfiguration | 配置文件中的等效参数 |
| `--file-check-frequency` | 命令行 | 静态 Pod 检查频率（默认 20s） |

```yaml
# KubeletConfiguration
staticPodPath: "/etc/kubernetes/manifests"
```

### 11.4 示例 Manifest（kube-apiserver 静态 Pod）

```yaml
# /etc/kubernetes/manifests/kube-apiserver.yaml
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
  labels:
    component: kube-apiserver
    tier: control-plane
spec:
  hostNetwork: true
  priorityClassName: system-node-critical
  containers:
  - name: kube-apiserver
    image: registry.k8s.io/kube-apiserver:v1.29.0
    command:
    - kube-apiserver
    - --advertise-address=192.168.1.10
    - --allow-privileged=true
    - --authorization-mode=Node,RBAC
    - --client-ca-file=/etc/kubernetes/pki/ca.crt
    - --etcd-servers=https://127.0.0.1:2379
    - --etcd-cafile=/etc/kubernetes/pki/etcd/ca.crt
    - --etcd-certfile=/etc/kubernetes/pki/apiserver-etcd-client.crt
    - --etcd-keyfile=/etc/kubernetes/pki/apiserver-etcd-client.key
    - --tls-cert-file=/etc/kubernetes/pki/apiserver.crt
    - --tls-private-key-file=/etc/kubernetes/pki/apiserver.key
    - --kubelet-client-certificate=/etc/kubernetes/pki/apiserver-kubelet-client.crt
    - --kubelet-client-key=/etc/kubernetes/pki/apiserver-kubelet-client.key
    - --service-account-issuer=https://kubernetes.default.svc.cluster.local
    - --service-account-key-file=/etc/kubernetes/pki/sa.pub
    - --service-account-signing-key-file=/etc/kubernetes/pki/sa.key
    - --service-cluster-ip-range=10.96.0.0/12
    - --secure-port=6443
    volumeMounts:
    - mountPath: /etc/kubernetes/pki
      name: k8s-certs
      readOnly: true
    - mountPath: /etc/ssl/certs
      name: ca-certs
      readOnly: true
  volumes:
  - hostPath:
      path: /etc/kubernetes/pki
      type: DirectoryOrCreate
    name: k8s-certs
  - hostPath:
      path: /etc/ssl/certs
      type: DirectoryOrCreate
    name: ca-certs

```

### 11.5 镜像 Pod (Mirror Pod)

当 kubelet 创建静态 Pod 后，会自动在 API Server 中创建一个对应的 **Mirror Pod**，使静态 Pod 可以被 `kubectl` 查看。

| 特性 | 说明 |
|:---|:---|
| **命名规则** | 静态 Pod 名称后缀为 `-<node-name>` |
| **只读性** | 通过 API Server 无法修改或删除（`kubectl delete` 无效） |
| **删除方式** | 只能删除节点上的 manifest 文件 |
| **状态同步** | kubelet 将容器状态同步到 Mirror Pod |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看静态 Pod 的 Mirror Pod
kubectl get pods -n kube-system
# NAME                               READY   STATUS
# kube-apiserver-master-1            1/1     Running    <- 镜像 Pod
# kube-controller-manager-master-1   1/1     Running
# kube-scheduler-master-1            1/1     Running

# 尝试删除会提示无法删除（或自动重建）
kubectl delete pod kube-apiserver-master-1 -n kube-system

```
### 11.6 与 DaemonSet 的区别

| 对比项 | 静态 Pod (Static Pod) | DaemonSet |
|:---|:---|:---|
| **控制器** | kubelet 直接管理 | DaemonSet Controller |
| **API Server** | 不依赖（创建时不经过） | 必须经过 API Server |
| **调度** | 绑定到特定节点 | 由调度器分配到所有/指定节点 |
| **更新方式** | 手动修改 manifest 文件 | `kubectl apply` 或 RollingUpdate |
| **副本管理** | 每个节点独立管理 | 控制器确保副本数 |
| **回滚** | 手动备份/恢复文件 | 支持 `kubectl rollout undo` |
| **健康检查** | kubelet 探针 | kubelet 探针 + 控制器 |
| **适用场景** | 控制平面组件、节点级服务 | 集群级守护进程（日志、监控） |

### 11.7 使用场景

| 场景 | 说明 |
|:---|:---|
| **控制平面自托管** | kubeadm 使用静态 Pod 部署 kube-apiserver、kube-controller-manager、kube-scheduler |
| **节点级守护服务** | 需要在 kubelet 启动前运行的关键服务 |
| **网络插件初始化** | 某些 CNI 插件使用静态 Pod 确保网络就绪 |
| **单节点集群** | minikube、kind 等使用静态 Pod 简化部署 |

---

<!-- chunk: 12. Topology Manager -->
## 12. Topology Manager

### 12.1 核心概念

Topology Manager 是 kubelet 的一个 **NUMA 感知资源协调组件**，负责协调 CPU Manager、Memory Manager 和设备插件（Device Plugin）之间的资源分配决策，确保 Pod 的所有资源（CPU、内存、GPU 等）分配在同一个 NUMA 节点上，从而优化性能。

```
┌─────────────────────────────────────────────────────────────┐
│                        kubelet                               │
│                                                              │
│   ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│   │ CPU Manager │  │Memory Manager│  │  Device Plugin      │ │
│   │  (static/   │  │  (None/      │  │  Manager            │ │
│   │   none)     │  │   Static)    │  │                     │ │
│   └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘ │
│          │                │                      │            │
│          └────────────────┼──────────────────────┘            │
│                           ▼                                  │
│              ┌─────────────────────────┐                     │
│              │    Topology Manager     │                     │
│              │  (none/best-effort/     │                     │
│              │   restricted/           │                     │
│              │   single-numa-node)     │                     │
│              └───────────┬─────────────┘                     │
│                          │                                   │
│                          ▼                                   │
│              ┌─────────────────────────┐                     │
│              │   合并 NUMA 亲和性提示   │                     │
│              │   决定最优 NUMA 分配     │                     │
│              └─────────────────────────┘                     │
└─────────────────────────────────────────────────────────────┘
```

### 12.2 策略对比

| 策略 | 说明 | NUMA 亲和性要求 | 资源分配失败行为 |
|:---|:---|:---|:---|
| **none** | 禁用 Topology Manager，各管理器独立工作 | 无要求 | 从不失败 |
| **best-effort** | 尝试按 NUMA 亲和性分配，但不强制 | 尽量满足 | 允许跨 NUMA |
| **restricted** | 强制按 NUMA 亲和性分配，不满足则拒绝 | 强制满足 | Pod 进入 Terminated |
| **single-numa-node** | 所有资源必须在单个 NUMA 节点上 | 最严格 | Pod 进入 Terminated |

### 12.3 KubeletConfiguration 配置示例

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 必须同时启用相关管理器
cpuManagerPolicy: "static"           # 必须设置为 static
memoryManagerPolicy: "Static"        # 启用 Memory Manager
topologyManagerPolicy: "restricted"  # 或 best-effort / single-numa-node
topologyManagerScope: "container"    # container / pod

# Memory Manager 预留配置 (NUMA 节点内存预留)
reservedMemory:
  - numaNode: 0
    limits:
      memory: 1Gi
  - numaNode: 1
    limits:
      memory: 1Gi

# 特性门控
featureGates:
  CPUManager: true
  MemoryManager: true
  TopologyManager: true
```

### 12.4 与 CPU Manager / Memory Manager 的交互

| 管理器 | 配置要求 | 向 Topology Manager 提供 |
|:---|:---|:---|
| **CPU Manager** | `cpuManagerPolicy: static` | CPU 亲和性提示 (Affinity Hints) |
| **Memory Manager** | `memoryManagerPolicy: Static` | 内存 NUMA 亲和性提示 |
| **Device Plugin** | 设备插件实现 `Topology` 接口 | 设备 NUMA 亲和性提示 |

```yaml
# 使用 Topology Manager 的 Pod 示例
apiVersion: v1
kind: Pod
metadata:
  name: numa-aware-pod
spec:
  containers:
  - name: app
    image: my-app:latest
    resources:
      limits:
        cpu: "4"
        memory: "8Gi"
        nvidia.com/gpu: "1"    # GPU 设备
      requests:
        cpu: "4"
        memory: "8Gi"
        nvidia.com/gpu: "1"
    # 必须设置 Guaranteed QoS (limits = requests)
```

> **注意**：Pod 必须是 **Guaranteed QoS**（所有容器的 limits = requests，且只设置 CPU/Memory），Topology Manager 才能生效。

### 12.5 适用场景

| 场景 | 说明 |
|:---|:---|
| **AI/ML GPU 训练** | 确保 GPU、CPU、内存处于同一 NUMA 节点，减少跨 NUMA 访问延迟 |
| **HPC 高性能计算** | MPI 等应用对 NUMA 亲和性敏感 |
| **低延迟应用** | 金融交易、实时游戏等对延迟要求严格的场景 |
| **DPDK/网络加速** | 网卡与 CPU 内存的 NUMA 对齐 |

---

<!-- chunk: 13. Memory QoS (cgroup v2) -->
## 13. Memory QoS (cgroup v2)

### 13.1 核心概念

Memory QoS 是 Kubernetes 利用 **cgroup v2** 的 `memory.min` 和 `memory.high` 机制实现的 Pod 内存服务质量保障。通过为不同 QoS 类别的 Pod 设置不同的 cgroup v2 内存参数，确保高优先级 Pod 获得更稳定的内存资源，减少被系统回收（reclaim）的概率。

| cgroup v2 参数 | 作用 | 对应 K8s 概念 |
|:---|:---|:---|
| **memory.min** | 内存硬保护，系统不会回收这部分内存 | 预留/保证内存 |
| **memory.high** | 内存软限制，超过后内核开始节流（throttle） | 限制内存突发 |
| **memory.max** | 内存硬限制，超过后触发 OOM Kill | limits |

### 13.2 K8s Memory QoS 分类与 cgroup v2 配置

| QoS 类别 | Pod 条件 | memory.min | memory.high | memory.max |
|:---|:---|:---|:---|:---|
| **Guaranteed** | limits = requests，且只设 CPU/Memory | requests 值 | limits 值 | limits 值 |
| **Burstable** | requests < limits 或部分资源设置 | requests 值 | limits 值 | limits 值 |
| **BestEffort** | 未设置 requests 和 limits | 0 | 节点可分配内存 | 无限制 |

```
┌──────────────────────────────────────────────────────────────┐
│                    cgroup v2 内存层级                         │
│                                                              │
│   ┌────────────────────────────────────────────────────┐    │
│   │              kubepods (Pod 总限制)                  │    │
│   │                                                      │    │
│   │   ┌─────────────────┐  ┌──────────────────────┐    │    │
│   │   │ Guaranteed Pod  │  │   Burstable Pod      │    │    │
│   │   │ memory.min=X    │  │   memory.min=Y       │    │    │
│   │   │ memory.high=X   │  │   memory.high=Z      │    │    │
│   │   │ memory.max=X    │  │   memory.max=Z       │    │    │
│   │   └─────────────────┘  └──────────────────────┘    │    │
│   │                                                      │    │
│   │   ┌────────────────────────────────────────────┐    │    │
│   │   │         BestEffort Pod                      │    │    │
│   │   │         memory.min=0                        │    │    │
│   │   │         memory.high=node_allocatable        │    │    │
│   │   └────────────────────────────────────────────┘    │    │
│   └────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
```

### 13.3 KubeletConfiguration 启用配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 启用 Memory QoS (cgroup v2 必需)
memoryThrottlingFactor: 0.8   # memory.high = limits * 0.8
                              # 取值范围: 0-1，默认 0.8

# 必须使用 cgroup v2
cgroupDriver: "systemd"

# 特性门控 (K8s 1.22+ 默认启用)
featureGates:
  MemoryQoS: true
```

> `memoryThrottlingFactor` 控制 `memory.high` 的计算：
> - `memory.high = limits * memoryThrottlingFactor`
> - 当 Pod 内存使用超过 `memory.high` 时，内核会开始对内存分配进行节流（throttle），而不是直接 OOM Kill

### 13.4 与驱逐机制的关系

| 机制 | 触发条件 | 行为 |
|:---|:---|:---|
| **Memory QoS (memory.high)** | 内存使用超过 `limits * factor` | 内核节流内存分配，进程变慢 |
| **软驱逐 (eviction-soft)** | 节点内存压力达到阈值 | 优雅终止低优先级 Pod |
| **硬驱逐 (eviction-hard)** | 节点内存严重压力 | 立即强制驱逐 Pod |
| **OOM Kill** | 内存使用超过 `memory.max` (limits) | 触发 cgroup OOM，杀死容器 |

```
内存使用增长路径:

0% ────────────────────────────────────────────────────> 100%
│          │                    │              │
│          ▼                    ▼              ▼
│      memory.high       eviction-soft   memory.max / eviction-hard
│      (开始节流)         (开始驱逐)      (强制 OOM Kill)
│
└─ Memory QoS 提供更早的干预，避免直接 OOM
```

### 13.5 适用版本与前提

| 要求 | 说明 |
|:---|:---|
| **Kubernetes** | 1.22+（Alpha），1.27+（Beta，默认启用） |
| **cgroup** | 必须启用 cgroup v2 |
| **容器运行时** | containerd 1.4+ / CRI-O 1.20+ |
| **操作系统** | Linux 内核 5.2+（推荐 5.8+） |

```bash
# 检查系统是否使用 cgroup v2
stat -fc %T /sys/fs/cgroup/
# 输出: cgroup2fs

# 检查 Memory QoS 是否生效
cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<uid>.slice/memory.high
cat /sys/fs/cgroup/kubepods.slice/kubepods-pod<uid>.slice/memory.min
```

---

<!-- chunk: 附录: kubelet API 端点 -->
## 附录: kubelet API 端点

| 端点 | 端口 | 说明 |
|:---|:---|:---|
| `/healthz` | 10248 | 健康检查 |
| `/metrics` | 10250 | Prometheus指标 |
| `/metrics/cadvisor` | 10250 | cAdvisor指标 |
| `/metrics/probes` | 10250 | 探针指标 |
| `/metrics/resource` | 10250 | 资源指标 |
| `/pods` | 10250 | Pod列表 |
| `/runningpods` | 10250 | 运行中Pod |
| `/spec` | 10250 | 节点规格 |
| `/stats/summary` | 10250 | 统计摘要 |
| `/logs` | 10250 | 日志访问 |
| `/exec` | 10250 | 容器exec |
| `/attach` | 10250 | 容器attach |
| `/portForward` | 10250 | 端口转发 |
| `/containerLogs` | 10250 | 容器日志 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[集群基础/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)

## See Also

- 13-kube-controller-manager-deep-dive
- 14-cloud-controller-manager-deep-dive
- 16-kube-proxy-deep-dive
- 17-apiserver-tuning

## Related

- [[deep-dive|#deep-dive Hub]] — tag hub

- [[生态参考/领域索引/node-index.md|Node 知识图谱索引]]

```

<!-- risk-assessed -->
