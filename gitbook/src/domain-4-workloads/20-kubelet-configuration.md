# Kubelet 配置与调优

## 概述

Kubelet 是运行在每个节点上的主要代理组件，负责确保容器按照 PodSpec 的定义运行。本文档详细介绍 kubelet 配置参数、性能调优和生产最佳实践。

## Kubelet 架构

### 组件交互架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              Kubernetes Control Plane                                │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐                     │
│  │   API Server    │  │   Scheduler     │  │   Controller    │                     │
│  │                 │  │                 │  │   Manager       │                     │
│  └────────┬────────┘  └─────────────────┘  └─────────────────┘                     │
│           │                                                                          │
│           │ Watch/Report                                                             │
│           │                                                                          │
└───────────┼─────────────────────────────────────────────────────────────────────────┘
            │
            │ HTTPS (6443)
            │
┌───────────┴─────────────────────────────────────────────────────────────────────────┐
│                                    Kubelet (10250)                                   │
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                            Kubelet Core Components                            │   │
│  │                                                                               │   │
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  │   │
│  │  │  Pod Manager  │  │Volume Manager │  │ Image Manager │  │ Node Status   │  │   │
│  │  │               │  │               │  │               │  │   Reporter    │  │   │
│  │  │ • PodSpec     │  │ • Mount       │  │ • Pull        │  │ • Capacity    │  │   │
│  │  │ • Lifecycle   │  │ • Unmount     │  │ • GC          │  │ • Conditions  │  │   │
│  │  │ • Admission   │  │ • CSI Driver  │  │ • Registry    │  │ • Allocatable │  │   │
│  │  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘  │   │
│  │                                                                               │   │
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  │   │
│  │  │ Probe Manager │  │ Eviction      │  │ cAdvisor      │  │ Device Plugin │  │   │
│  │  │               │  │   Manager     │  │               │  │   Manager     │  │   │
│  │  │ • Liveness    │  │ • Memory      │  │ • Metrics     │  │ • GPU         │  │   │
│  │  │ • Readiness   │  │ • Disk        │  │ • Stats       │  │ • FPGA        │  │   │
│  │  │ • Startup     │  │ • PID         │  │ • Container   │  │ • SR-IOV      │  │   │
│  │  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘  │   │
│  │                                                                               │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                              Runtime Interface                                  │ │
│  │                                                                                 │ │
│  │   ┌────────────────────────────────────────────────────────────────────────┐  │ │
│  │   │                         CRI (Container Runtime Interface)               │  │ │
│  │   │                                                                         │  │ │
│  │   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐              │  │ │
│  │   │   │  containerd  │   │   CRI-O      │   │   Docker     │              │  │ │
│  │   │   │              │   │              │   │  (dockershim)│              │  │ │
│  │   │   │ gRPC:10010   │   │ gRPC:10010   │   │  Deprecated  │              │  │ │
│  │   │   └──────────────┘   └──────────────┘   └──────────────┘              │  │ │
│  │   │                                                                         │  │ │
│  │   └────────────────────────────────────────────────────────────────────────┘  │ │
│  │                                                                                 │ │
│  │   ┌────────────────────────────────────────────────────────────────────────┐  │ │
│  │   │                            CNI (Container Network Interface)            │  │ │
│  │   │                                                                         │  │ │
│  │   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐              │  │ │
│  │   │   │   Calico     │   │   Cilium     │   │   Flannel    │              │  │ │
│  │   │   └──────────────┘   └──────────────┘   └──────────────┘              │  │ │
│  │   │                                                                         │  │ │
│  │   └────────────────────────────────────────────────────────────────────────┘  │ │
│  │                                                                                 │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
                                           │
                                           │ OCI Runtime
                                           │
┌──────────────────────────────────────────┴──────────────────────────────────────────┐
│                                 OCI Runtime Layer                                    │
│                                                                                      │
│   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐        │
│   │    runc      │   │     gVisor   │   │    Kata      │   │   Firecracker│        │
│   │              │   │   (runsc)    │   │  Containers  │   │              │        │
│   │  Standard    │   │  Sandboxed   │   │    VM-based  │   │   MicroVM    │        │
│   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘        │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### Pod 生命周期管理

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              Pod Lifecycle Management                                │
│                                                                                      │
│   API Server                    Kubelet                        Container Runtime     │
│       │                            │                                  │              │
│       │  1. Pod Create Event       │                                  │              │
│       │ ────────────────────────►  │                                  │              │
│       │                            │                                  │              │
│       │                            │  2. Admission Check              │              │
│       │                            │  ┌────────────────────┐          │              │
│       │                            │  │ • Resource Limits  │          │              │
│       │                            │  │ • Node Selector    │          │              │
│       │                            │  │ • Tolerations      │          │              │
│       │                            │  │ • Security Context │          │              │
│       │                            │  └────────────────────┘          │              │
│       │                            │                                  │              │
│       │                            │  3. Volume Setup                 │              │
│       │                            │ ─────────────────────────────►   │              │
│       │                            │                                  │              │
│       │                            │  4. Network Setup (CNI)          │              │
│       │                            │ ─────────────────────────────►   │              │
│       │                            │                                  │              │
│       │                            │  5. Pull Image                   │              │
│       │                            │ ─────────────────────────────►   │              │
│       │                            │                                  │              │
│       │                            │  6. Create Container             │              │
│       │                            │ ─────────────────────────────►   │              │
│       │                            │                                  │              │
│       │                            │  7. Start Container              │              │
│       │                            │ ─────────────────────────────►   │              │
│       │                            │                                  │              │
│       │  8. Status Update          │                                  │              │
│       │ ◄────────────────────────  │                                  │              │
│       │                            │                                  │              │
│       │                            │  9. Probes (Liveness/Readiness)  │              │
│       │                            │ ─────────────────────────────►   │              │
│       │                            │ ◄─────────────────────────────   │              │
│       │                            │                                  │              │
│       │  10. Periodic Status       │                                  │              │
│       │ ◄────────────────────────  │                                  │              │
│       │                            │                                  │              │
└───────┴────────────────────────────┴──────────────────────────────────┴──────────────┘
```

## 配置参数详解

### 核心参数对比矩阵

| 参数类别 | 参数名 | 默认值 | 推荐值(生产) | 说明 |
|---------|--------|--------|--------------|------|
| **API 连接** | `--kubeconfig` | - | /etc/kubernetes/kubelet.conf | API Server 认证配置 |
| | `--node-status-update-frequency` | 10s | 10s | 节点状态更新频率 |
| | `--node-status-report-frequency` | 5m | 1m | 节点状态报告频率 |
| **资源管理** | `--kube-reserved` | - | cpu=100m,memory=256Mi | 为 K8s 组件预留资源 |
| | `--system-reserved` | - | cpu=100m,memory=256Mi | 为系统进程预留资源 |
| | `--eviction-hard` | - | 见下文 | 硬驱逐阈值 |
| **容器运行时** | `--container-runtime-endpoint` | - | unix:///run/containerd/containerd.sock | CRI 端点 |
| | `--pod-infra-container-image` | - | registry.k8s.io/pause:3.9 | Pause 镜像 |
| **网络** | `--cni-bin-dir` | /opt/cni/bin | /opt/cni/bin | CNI 插件目录 |
| | `--cni-conf-dir` | /etc/cni/net.d | /etc/cni/net.d | CNI 配置目录 |
| **存储** | `--root-dir` | /var/lib/kubelet | /var/lib/kubelet | Kubelet 数据目录 |
| | `--cert-dir` | /var/lib/kubelet/pki | /var/lib/kubelet/pki | 证书存储目录 |

### 资源预留参数详解

| 预留类型 | 参数 | 作用 | 计算公式 |
|---------|------|------|---------|
| **Kube Reserved** | `--kube-reserved` | 为 kubelet/kube-proxy 预留 | CPU: 100m, Memory: 256Mi (固定) |
| **System Reserved** | `--system-reserved` | 为系统进程预留 | CPU: 100m + (cores * 10m), Memory: 256Mi + (RAM * 1%) |
| **Eviction Threshold** | `--eviction-hard` | 驱逐保护 | memory.available < 100Mi |
| **Allocatable** | 计算值 | 可分配给 Pod 的资源 | Capacity - Kube - System - Eviction |

### 驱逐阈值配置

| 驱逐信号 | 硬驱逐默认 | 软驱逐推荐 | 软驱逐宽限期 |
|---------|-----------|-----------|-------------|
| `memory.available` | 100Mi | 200Mi | 30s |
| `nodefs.available` | 10% | 15% | 2m |
| `nodefs.inodesFree` | 5% | 10% | 2m |
| `imagefs.available` | 15% | 20% | 2m |
| `pid.available` | - | 5% | 10s |

## 完整配置示例

### 生产环境 KubeletConfiguration

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# =============================================================================
# 认证授权配置
# =============================================================================
authentication:
  anonymous:
    enabled: false                    # 禁用匿名访问
  webhook:
    enabled: true                     # 启用 Webhook 认证
    cacheTTL: 2m                      # 认证缓存时间
  x509:
    clientCAFile: /etc/kubernetes/pki/ca.crt  # 客户端证书 CA

authorization:
  mode: Webhook                       # 使用 Webhook 授权
  webhook:
    cacheAuthorizedTTL: 5m           # 授权缓存时间
    cacheUnauthorizedTTL: 30s        # 未授权缓存时间

# =============================================================================
# 集群配置
# =============================================================================
clusterDomain: cluster.local          # 集群 DNS 域名
clusterDNS:
  - 10.96.0.10                        # CoreDNS Service IP

# =============================================================================
# 网络配置
# =============================================================================
address: 0.0.0.0                      # 监听地址
port: 10250                           # Kubelet 端口
readOnlyPort: 0                       # 禁用只读端口 (安全)
healthzBindAddress: 127.0.0.1         # 健康检查绑定地址
healthzPort: 10248                    # 健康检查端口

# =============================================================================
# TLS 配置
# =============================================================================
tlsCertFile: /var/lib/kubelet/pki/kubelet.crt
tlsPrivateKeyFile: /var/lib/kubelet/pki/kubelet.key
tlsCipherSuites:
  - TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
  - TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
  - TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
  - TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
tlsMinVersion: VersionTLS12           # 最低 TLS 版本

# =============================================================================
# 资源预留配置
# =============================================================================
# Kube 组件预留资源
kubeReserved:
  cpu: "200m"
  memory: "512Mi"
  ephemeral-storage: "1Gi"

# 系统进程预留资源
systemReserved:
  cpu: "200m"
  memory: "512Mi"
  ephemeral-storage: "1Gi"

# 预留资源的 cgroup
kubeReservedCgroup: /kubelet.slice
systemReservedCgroup: /system.slice

# 强制执行资源预留
enforceNodeAllocatable:
  - pods
  - kube-reserved
  - system-reserved

# =============================================================================
# 驱逐配置
# =============================================================================
# 硬驱逐阈值 (立即驱逐)
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
  pid.available: "5%"

# 软驱逐阈值
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
  nodefs.inodesFree: "10%"
  imagefs.available: "20%"

# 软驱逐宽限期
evictionSoftGracePeriod:
  memory.available: "30s"
  nodefs.available: "2m"
  nodefs.inodesFree: "2m"
  imagefs.available: "2m"

# 驱逐压力过渡期
evictionPressureTransitionPeriod: 30s

# 最小回收量
evictionMinimumReclaim:
  memory.available: "0Mi"
  nodefs.available: "500Mi"
  imagefs.available: "2Gi"

# 每次驱逐的最大 Pod 数
evictionMaxPodGracePeriod: 30

# =============================================================================
# 镜像垃圾回收配置
# =============================================================================
imageGCHighThresholdPercent: 85       # 开始 GC 的磁盘使用率
imageGCLowThresholdPercent: 80        # 停止 GC 的磁盘使用率
imageMinimumGCAge: 2m                 # 镜像最小保留时间

# =============================================================================
# 容器垃圾回收配置
# =============================================================================
containerLogMaxSize: "50Mi"           # 容器日志最大大小
containerLogMaxFiles: 5               # 容器日志最大文件数

# =============================================================================
# Pod 配置
# =============================================================================
maxPods: 110                          # 每节点最大 Pod 数
podPidsLimit: 4096                    # 每 Pod 最大 PID 数
maxOpenFiles: 1000000                 # 最大打开文件数

# 静态 Pod 路径
staticPodPath: /etc/kubernetes/manifests

# Pod 驱逐后的宽限期
podEvictionTimeout: 5m

# =============================================================================
# CPU 管理配置
# =============================================================================
cpuManagerPolicy: static              # CPU 管理策略: none/static
cpuManagerReconcilePeriod: 10s        # CPU 管理器协调周期
cpuManagerPolicyOptions:
  full-pcpus-only: "true"             # 仅分配完整物理核心
topologyManagerPolicy: single-numa-node  # 拓扑管理策略
topologyManagerScope: container       # 拓扑管理范围

# =============================================================================
# 内存管理配置
# =============================================================================
memoryManagerPolicy: Static           # 内存管理策略
reservedMemory:
  - numaNode: 0
    limits:
      memory: "1Gi"

# =============================================================================
# Feature Gates
# =============================================================================
featureGates:
  RotateKubeletServerCertificate: true  # 自动轮转证书
  GracefulNodeShutdown: true            # 优雅节点关闭
  PodOverhead: true                     # Pod 开销计算
  TopologyManager: true                 # 拓扑管理器
  MemoryManager: true                   # 内存管理器
  CPUManager: true                      # CPU 管理器
  SeccompDefault: true                  # 默认 Seccomp

# =============================================================================
# 节点状态报告
# =============================================================================
nodeStatusUpdateFrequency: 10s        # 节点状态更新频率
nodeStatusReportFrequency: 1m         # 节点状态报告频率
nodeLeaseDurationSeconds: 40          # 节点租约时长

# =============================================================================
# 同步配置
# =============================================================================
syncFrequency: 1m                     # 同步配置频率
fileCheckFrequency: 20s               # 文件检查频率
httpCheckFrequency: 20s               # HTTP 检查频率

# =============================================================================
# 日志配置
# =============================================================================
logging:
  format: json                        # 日志格式
  flushFrequency: 5s                  # 日志刷新频率
  verbosity: 2                        # 日志级别
  options:
    json:
      infoBufferSize: "0"

# =============================================================================
# 关闭配置
# =============================================================================
shutdownGracePeriod: 30s              # 关闭宽限期
shutdownGracePeriodCriticalPods: 10s  # 关键 Pod 关闭宽限期

# =============================================================================
# 其他配置
# =============================================================================
enableControllerAttachDetach: true    # 启用控制器附加/分离
failSwapOn: true                      # Swap 启用时失败
rotateCertificates: true              # 自动轮转证书
serverTLSBootstrap: true              # 服务器 TLS 引导
streamingConnectionIdleTimeout: 4h    # 流连接空闲超时
protectKernelDefaults: true           # 保护内核默认值
makeIPTablesUtilChains: true          # 创建 iptables 链
iptablesMasqueradeBit: 14             # MASQUERADE 位
iptablesDropBit: 15                   # DROP 位
```

### 启动参数配置

```bash
# /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
[Service]
Environment="KUBELET_KUBECONFIG_ARGS=--bootstrap-kubeconfig=/etc/kubernetes/bootstrap-kubelet.conf --kubeconfig=/etc/kubernetes/kubelet.conf"
Environment="KUBELET_CONFIG_ARGS=--config=/var/lib/kubelet/config.yaml"
Environment="KUBELET_EXTRA_ARGS=--node-ip=192.168.1.100 --hostname-override=node-01 --container-runtime-endpoint=unix:///run/containerd/containerd.sock"
ExecStart=
ExecStart=/usr/bin/kubelet $KUBELET_KUBECONFIG_ARGS $KUBELET_CONFIG_ARGS $KUBELET_EXTRA_ARGS
```

## 场景化配置

### 高密度节点配置 (小 Pod 多实例)

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 增加最大 Pod 数
maxPods: 250

# 增加 PID 限制
podPidsLimit: 2048

# 调整资源预留
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
  ephemeral-storage: "2Gi"
systemReserved:
  cpu: "500m"
  memory: "1Gi"
  ephemeral-storage: "2Gi"

# 调整驱逐阈值
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  imagefs.available: "10%"
  pid.available: "10%"

# 禁用 CPU Manager (高密度不需要独占)
cpuManagerPolicy: none

# 调整镜像 GC
imageGCHighThresholdPercent: 90
imageGCLowThresholdPercent: 85

# 增加容器日志限制
containerLogMaxSize: "20Mi"
containerLogMaxFiles: 3
```

### GPU 节点配置

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# GPU 节点通常运行较少 Pod
maxPods: 50

# 启用拓扑管理器确保 GPU 亲和性
topologyManagerPolicy: single-numa-node
topologyManagerScope: container

# 启用 CPU Manager 保证 GPU 工作负载性能
cpuManagerPolicy: static
cpuManagerPolicyOptions:
  full-pcpus-only: "true"

# 为系统和 GPU 驱动预留更多资源
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
systemReserved:
  cpu: "1000m"
  memory: "2Gi"

# 开启相关 Feature Gates
featureGates:
  TopologyManager: true
  CPUManager: true
  DevicePlugins: true
  KubeletPodResourcesGetAllocatable: true

# 宽松的驱逐阈值 (GPU 节点通常资源充足)
evictionHard:
  memory.available: "1Gi"
  nodefs.available: "10%"
  imagefs.available: "15%"
```

### 边缘节点配置 (低资源/不稳定网络)

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 减少最大 Pod 数
maxPods: 30

# 增加节点状态报告容忍度
nodeStatusUpdateFrequency: 30s
nodeStatusReportFrequency: 5m
nodeLeaseDurationSeconds: 120

# 减少资源预留 (边缘设备资源有限)
kubeReserved:
  cpu: "50m"
  memory: "128Mi"
systemReserved:
  cpu: "50m"
  memory: "128Mi"

# 激进的驱逐阈值
evictionHard:
  memory.available: "50Mi"
  nodefs.available: "5%"
  imagefs.available: "10%"

# 减少镜像保留
imageMinimumGCAge: 1m
imageGCHighThresholdPercent: 70
imageGCLowThresholdPercent: 60

# 减少同步频率
syncFrequency: 5m
fileCheckFrequency: 1m

# 启用优雅节点关闭
featureGates:
  GracefulNodeShutdown: true

shutdownGracePeriod: 60s
shutdownGracePeriodCriticalPods: 30s

# 允许在弱网环境下更长的超时
streamingConnectionIdleTimeout: 1h
```

### 大规模集群节点配置 (5000+ 节点)

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# 减少 API Server 压力
nodeStatusUpdateFrequency: 20s
nodeStatusReportFrequency: 5m

# 使用节点租约减少 etcd 负载
nodeLeaseDurationSeconds: 40

# 减少同步频率
syncFrequency: 2m

# 启用 watch 缓存
featureGates:
  APIListChunking: true
  APIPriorityAndFairness: true

# 优化驱逐以避免级联故障
evictionPressureTransitionPeriod: 1m
evictionSoftGracePeriod:
  memory.available: "1m"
  nodefs.available: "5m"
  imagefs.available: "5m"
```

## 运行时配置

### Containerd 配置

```toml
# /etc/containerd/config.toml
version = 2
root = "/var/lib/containerd"
state = "/run/containerd"

[grpc]
  address = "/run/containerd/containerd.sock"
  max_recv_message_size = 16777216
  max_send_message_size = 16777216

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    sandbox_image = "registry.k8s.io/pause:3.9"
    max_concurrent_downloads = 10
    max_container_log_line_size = 16384

    [plugins."io.containerd.grpc.v1.cri".containerd]
      default_runtime_name = "runc"
      snapshotter = "overlayfs"
      
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true
            
    [plugins."io.containerd.grpc.v1.cri".registry]
      config_path = "/etc/containerd/certs.d"
      
    [plugins."io.containerd.grpc.v1.cri".cni]
      bin_dir = "/opt/cni/bin"
      conf_dir = "/etc/cni/net.d"

[metrics]
  address = "0.0.0.0:10257"
  grpc_histogram = false
```

### CRI-O 配置

```toml
# /etc/crio/crio.conf
[crio]
root = "/var/lib/containers/storage"
runroot = "/var/run/containers/storage"
log_dir = "/var/log/crio/pods"
version_file = "/var/run/crio/version"
clean_shutdown_file = "/var/lib/crio/clean.shutdown"

[crio.api]
listen = "/var/run/crio/crio.sock"
stream_address = "0.0.0.0"
stream_port = "0"
grpc_max_send_msg_size = 16777216
grpc_max_recv_msg_size = 16777216

[crio.runtime]
default_runtime = "runc"
no_pivot = false
decryption_keys_path = "/etc/crio/keys/"
conmon = ""
conmon_cgroup = "pod"
conmon_env = [
    "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
]
default_env = []
selinux = false
seccomp_profile = ""
apparmor_profile = "crio-default"
cgroup_manager = "systemd"
default_capabilities = [
    "CHOWN",
    "DAC_OVERRIDE",
    "FSETID",
    "FOWNER",
    "SETGID",
    "SETUID",
    "SETPCAP",
    "NET_BIND_SERVICE",
    "KILL",
]
default_sysctls = []
additional_devices = []
hooks_dir = [
    "/usr/share/containers/oci/hooks.d",
]
default_mounts = []
pids_limit = 4096
log_size_max = -1
log_to_journald = false
container_exits_dir = "/var/run/crio/exits"
container_attach_socket_dir = "/var/run/crio"
namespaces_dir = "/var/run"
pinns_path = ""

[crio.runtime.runtimes.runc]
runtime_path = ""
runtime_type = "oci"
runtime_root = "/run/runc"
allowed_annotations = [
    "io.containers.trace-syscall",
]

[crio.image]
default_transport = "docker://"
global_auth_file = ""
pause_image = "registry.k8s.io/pause:3.9"
pause_image_auth_file = ""
pause_command = "/pause"
signature_policy = ""
image_volumes = "mkdir"
big_files_temporary_dir = ""

[crio.network]
network_dir = "/etc/cni/net.d/"
plugin_dirs = [
    "/opt/cni/bin/",
    "/usr/libexec/cni/",
]
```

## 性能调优

### 系统级调优

```bash
#!/bin/bash
# kubelet-sysctl-tuning.sh
# Kubelet 节点系统级调优脚本

set -e

echo "=== 配置内核参数 ==="

cat > /etc/sysctl.d/99-kubernetes.conf << 'EOF'
# =============================================================================
# 网络优化
# =============================================================================
# 启用 IP 转发
net.ipv4.ip_forward = 1
net.ipv6.conf.all.forwarding = 1

# 允许绑定非本地 IP
net.ipv4.ip_nonlocal_bind = 1

# 连接跟踪表大小
net.netfilter.nf_conntrack_max = 1048576
net.netfilter.nf_conntrack_tcp_timeout_established = 86400
net.netfilter.nf_conntrack_tcp_timeout_close_wait = 3600

# 增加本地端口范围
net.ipv4.ip_local_port_range = 10000 65535

# TCP 优化
net.core.somaxconn = 32768
net.ipv4.tcp_max_syn_backlog = 32768
net.core.netdev_max_backlog = 32768

# 增加 TCP 缓冲区
net.core.rmem_default = 262144
net.core.rmem_max = 16777216
net.core.wmem_default = 262144
net.core.wmem_max = 16777216
net.ipv4.tcp_rmem = 4096 262144 16777216
net.ipv4.tcp_wmem = 4096 262144 16777216

# TCP keepalive
net.ipv4.tcp_keepalive_time = 600
net.ipv4.tcp_keepalive_intvl = 30
net.ipv4.tcp_keepalive_probes = 10

# TIME_WAIT 优化
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 30

# =============================================================================
# 内存优化
# =============================================================================
# 虚拟内存设置
vm.max_map_count = 262144
vm.swappiness = 0
vm.overcommit_memory = 1
vm.panic_on_oom = 0

# 脏页设置
vm.dirty_ratio = 10
vm.dirty_background_ratio = 5

# =============================================================================
# 文件系统优化
# =============================================================================
# 文件描述符限制
fs.file-max = 2097152
fs.inotify.max_user_instances = 8192
fs.inotify.max_user_watches = 524288

# AIO 限制
fs.aio-max-nr = 1048576

# =============================================================================
# 内核优化
# =============================================================================
kernel.pid_max = 65536
kernel.threads-max = 65536

# 禁用 NUMA 平衡 (如果使用 CPU Manager)
# kernel.numa_balancing = 0

# 启用 BPF JIT
net.core.bpf_jit_enable = 1
net.core.bpf_jit_harden = 0
EOF

sysctl --system

echo "=== 配置资源限制 ==="

cat > /etc/security/limits.d/99-kubernetes.conf << 'EOF'
*       soft    nofile          1048576
*       hard    nofile          1048576
*       soft    nproc           65536
*       hard    nproc           65536
*       soft    memlock         unlimited
*       hard    memlock         unlimited
root    soft    nofile          1048576
root    hard    nofile          1048576
EOF

echo "=== 禁用 Swap ==="
swapoff -a
sed -i '/swap/d' /etc/fstab

echo "=== 加载必要内核模块 ==="
cat > /etc/modules-load.d/kubernetes.conf << 'EOF'
overlay
br_netfilter
ip_vs
ip_vs_rr
ip_vs_wrr
ip_vs_sh
nf_conntrack
EOF

modprobe overlay
modprobe br_netfilter
modprobe ip_vs
modprobe ip_vs_rr
modprobe ip_vs_wrr
modprobe ip_vs_sh
modprobe nf_conntrack

echo "=== 系统调优完成 ==="
```

### Kubelet 启动优化

```bash
#!/bin/bash
# kubelet-startup-optimization.sh
# Kubelet 启动优化脚本

set -e

echo "=== 优化 Kubelet systemd 配置 ==="

mkdir -p /etc/systemd/system/kubelet.service.d

cat > /etc/systemd/system/kubelet.service.d/20-optimization.conf << 'EOF'
[Service]
# 增加文件描述符限制
LimitNOFILE=1048576
LimitNPROC=65536
LimitCORE=infinity
LimitMEMLOCK=infinity

# 内存锁定
MemoryLock=true

# CPU 亲和性 (可选,根据需要调整)
# CPUAffinity=0-3

# 资源控制
CPUAccounting=true
MemoryAccounting=true
IOAccounting=true

# 重启策略
Restart=always
RestartSec=10
StartLimitInterval=0

# OOM 分数调整 (保护 kubelet)
OOMScoreAdjust=-999
EOF

echo "=== 重新加载 systemd 配置 ==="
systemctl daemon-reload

echo "=== Kubelet 启动优化完成 ==="
```

## 故障诊断

### 诊断脚本

```bash
#!/bin/bash
# kubelet-diagnostics.sh
# Kubelet 综合诊断脚本

set -e

echo "=========================================="
echo "         Kubelet 诊断报告"
echo "=========================================="
echo "时间: $(date)"
echo "节点: $(hostname)"
echo ""

echo "=== 1. Kubelet 服务状态 ==="
systemctl status kubelet --no-pager || true
echo ""

echo "=== 2. Kubelet 版本信息 ==="
kubelet --version
echo ""

echo "=== 3. Kubelet 进程信息 ==="
ps aux | grep kubelet | grep -v grep || echo "Kubelet 进程未运行"
echo ""

echo "=== 4. Kubelet 端口监听 ==="
ss -tlnp | grep -E "10250|10248|10255" || echo "未找到 Kubelet 端口"
echo ""

echo "=== 5. Kubelet 配置文件 ==="
if [ -f /var/lib/kubelet/config.yaml ]; then
    echo "配置文件存在: /var/lib/kubelet/config.yaml"
    echo "--- 关键配置 ---"
    grep -E "^(maxPods|cpuManagerPolicy|memoryManagerPolicy|topologyManagerPolicy|evictionHard)" /var/lib/kubelet/config.yaml || true
else
    echo "配置文件不存在"
fi
echo ""

echo "=== 6. 容器运行时状态 ==="
if [ -S /run/containerd/containerd.sock ]; then
    echo "Containerd socket 存在"
    crictl version 2>/dev/null || echo "crictl 命令不可用"
    echo ""
    echo "--- 运行中的容器 ---"
    crictl ps --no-trunc 2>/dev/null | head -20 || true
elif [ -S /var/run/crio/crio.sock ]; then
    echo "CRI-O socket 存在"
    crictl version 2>/dev/null || echo "crictl 命令不可用"
fi
echo ""

echo "=== 7. 节点资源状态 ==="
echo "--- CPU ---"
nproc
lscpu | grep -E "^(Architecture|CPU\(s\)|Model name|NUMA)"
echo ""

echo "--- 内存 ---"
free -h
echo ""

echo "--- 磁盘 ---"
df -h /var/lib/kubelet /var/lib/containerd /var/log 2>/dev/null || df -h
echo ""

echo "--- Inode ---"
df -i /var/lib/kubelet 2>/dev/null || df -i /
echo ""

echo "=== 8. Kubelet 最近日志 ==="
journalctl -u kubelet --no-pager -n 50 --since "5 minutes ago" 2>/dev/null || \
    tail -50 /var/log/messages | grep kubelet || true
echo ""

echo "=== 9. 节点状态 (通过 API) ==="
if [ -f /etc/kubernetes/kubelet.conf ]; then
    kubectl --kubeconfig=/etc/kubernetes/kubelet.conf get node $(hostname) -o wide 2>/dev/null || \
        echo "无法获取节点状态"
    echo ""
    echo "--- 节点 Conditions ---"
    kubectl --kubeconfig=/etc/kubernetes/kubelet.conf get node $(hostname) -o jsonpath='{range .status.conditions[*]}{.type}: {.status} ({.reason}) - {.message}{"\n"}{end}' 2>/dev/null || true
fi
echo ""

echo "=== 10. 常见问题检查 ==="

# 检查 Swap
if [ "$(swapon -s | wc -l)" -gt 1 ]; then
    echo "[警告] Swap 已启用，这可能导致问题"
else
    echo "[正常] Swap 已禁用"
fi

# 检查 IP 转发
if [ "$(sysctl -n net.ipv4.ip_forward)" -eq 1 ]; then
    echo "[正常] IP 转发已启用"
else
    echo "[错误] IP 转发未启用"
fi

# 检查 br_netfilter
if lsmod | grep -q br_netfilter; then
    echo "[正常] br_netfilter 模块已加载"
else
    echo "[错误] br_netfilter 模块未加载"
fi

# 检查 cgroup
if [ -d /sys/fs/cgroup/memory ]; then
    echo "[正常] Cgroup v1 memory 控制器可用"
fi
if [ -f /sys/fs/cgroup/cgroup.controllers ]; then
    echo "[正常] Cgroup v2 可用"
fi

# 检查证书有效期
if [ -f /var/lib/kubelet/pki/kubelet.crt ]; then
    CERT_EXPIRY=$(openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -enddate 2>/dev/null | cut -d= -f2)
    echo "[信息] Kubelet 证书过期时间: $CERT_EXPIRY"
fi

echo ""
echo "=========================================="
echo "         诊断报告结束"
echo "=========================================="
```

### 常见问题排查

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| **Kubelet 无法启动** | 配置文件语法错误 | `kubelet --config=/var/lib/kubelet/config.yaml` | 检查 YAML 语法 |
| | 证书问题 | `openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -text` | 更新证书 |
| | 端口被占用 | `ss -tlnp \| grep 10250` | 释放端口或修改配置 |
| **节点 NotReady** | 运行时不可用 | `crictl info` | 重启 containerd/crio |
| | CNI 配置问题 | `ls /etc/cni/net.d/` | 检查 CNI 配置 |
| | 内存压力 | `kubectl describe node` | 清理或扩容内存 |
| **Pod 无法调度** | 资源不足 | `kubectl describe node \| grep Allocatable -A 10` | 调整资源预留 |
| | Taint 问题 | `kubectl get node -o jsonpath='{.spec.taints}'` | 移除/添加 Toleration |
| **Pod 频繁被驱逐** | 驱逐阈值过高 | 检查 `evictionHard` 配置 | 调整驱逐阈值 |
| | 资源泄漏 | `crictl stats` | 定位泄漏容器 |
| **镜像拉取失败** | 认证问题 | `crictl pull <image>` | 配置镜像仓库认证 |
| | 网络问题 | `curl -v https://registry` | 检查网络/代理配置 |

### CPU Manager 问题排查

```bash
#!/bin/bash
# cpu-manager-debug.sh
# CPU Manager 问题诊断

echo "=== CPU Manager 状态 ==="

# 检查 CPU Manager 状态文件
CPU_MANAGER_STATE="/var/lib/kubelet/cpu_manager_state"
if [ -f "$CPU_MANAGER_STATE" ]; then
    echo "CPU Manager 状态文件内容:"
    cat "$CPU_MANAGER_STATE" | jq . 2>/dev/null || cat "$CPU_MANAGER_STATE"
else
    echo "CPU Manager 状态文件不存在"
fi
echo ""

# 检查 CPU 拓扑
echo "=== CPU 拓扑信息 ==="
lscpu | grep -E "^(CPU\(s\)|Thread|Core|Socket|NUMA)"
echo ""

# 检查 cgroup
echo "=== CPU Cgroup 信息 ==="
if [ -d /sys/fs/cgroup/cpu/kubepods.slice ]; then
    echo "Cgroup v1 路径: /sys/fs/cgroup/cpu/kubepods.slice"
    ls /sys/fs/cgroup/cpu/kubepods.slice/
elif [ -d /sys/fs/cgroup/kubepods.slice ]; then
    echo "Cgroup v2 路径: /sys/fs/cgroup/kubepods.slice"
    ls /sys/fs/cgroup/kubepods.slice/
fi
echo ""

# 检查 Guaranteed Pod 的 CPU 绑定
echo "=== Guaranteed Pod CPU 绑定 ==="
for cgroup in /sys/fs/cgroup/cpu/kubepods.slice/kubepods-pod*.slice/*/cpuset.cpus 2>/dev/null; do
    if [ -f "$cgroup" ]; then
        echo "$cgroup: $(cat $cgroup)"
    fi
done

# 检查 kubelet 日志中的 CPU Manager 相关信息
echo ""
echo "=== Kubelet CPU Manager 日志 ==="
journalctl -u kubelet --no-pager -n 100 | grep -i "cpu.*manager" || echo "未找到相关日志"
```

## 监控告警

### Prometheus 监控规则

```yaml
# kubelet-monitoring-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kubelet-rules
  namespace: monitoring
  labels:
    prometheus: k8s
    role: alert-rules
spec:
  groups:
    - name: kubelet.rules
      interval: 30s
      rules:
        # =================================================================
        # Kubelet 可用性告警
        # =================================================================
        - alert: KubeletDown
          expr: up{job="kubelet"} == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Kubelet 宕机"
            description: "节点 {{ $labels.instance }} 的 Kubelet 已宕机超过 5 分钟"
            
        - alert: KubeletNotReady
          expr: kube_node_status_condition{condition="Ready",status="true"} == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "节点 NotReady"
            description: "节点 {{ $labels.node }} 处于 NotReady 状态超过 5 分钟"
            
        # =================================================================
        # Pod 启动性能告警
        # =================================================================
        - alert: KubeletPodStartupLatencyHigh
          expr: |
            histogram_quantile(0.99, 
              sum(rate(kubelet_pod_start_duration_seconds_bucket{job="kubelet"}[5m])) by (le, node)
            ) > 60
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "Pod 启动延迟高"
            description: "节点 {{ $labels.node }} 的 P99 Pod 启动延迟超过 60 秒"
            
        - alert: KubeletContainerStartupLatencyHigh
          expr: |
            histogram_quantile(0.99, 
              sum(rate(kubelet_container_start_duration_seconds_bucket{job="kubelet"}[5m])) by (le, node)
            ) > 30
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "容器启动延迟高"
            description: "节点 {{ $labels.node }} 的 P99 容器启动延迟超过 30 秒"
            
        # =================================================================
        # 资源压力告警
        # =================================================================
        - alert: KubeletMemoryPressure
          expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "节点内存压力"
            description: "节点 {{ $labels.node }} 存在内存压力"
            
        - alert: KubeletDiskPressure
          expr: kube_node_status_condition{condition="DiskPressure",status="true"} == 1
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "节点磁盘压力"
            description: "节点 {{ $labels.node }} 存在磁盘压力"
            
        - alert: KubeletPIDPressure
          expr: kube_node_status_condition{condition="PIDPressure",status="true"} == 1
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "节点 PID 压力"
            description: "节点 {{ $labels.node }} 存在 PID 压力"
            
        # =================================================================
        # PLEG (Pod Lifecycle Event Generator) 告警
        # =================================================================
        - alert: KubeletPLEGDurationHigh
          expr: |
            histogram_quantile(0.99, 
              sum(rate(kubelet_pleg_relist_duration_seconds_bucket{job="kubelet"}[5m])) by (le, node)
            ) > 10
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "PLEG 延迟高"
            description: "节点 {{ $labels.node }} 的 PLEG relist 延迟超过 10 秒"
            
        - alert: KubeletPLEGNotHealthy
          expr: kubelet_pleg_health{job="kubelet"} == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "PLEG 不健康"
            description: "节点 {{ $labels.node }} 的 PLEG 不健康"
            
        # =================================================================
        # 运行时告警
        # =================================================================
        - alert: KubeletRuntimeOperationLatencyHigh
          expr: |
            histogram_quantile(0.99, 
              sum(rate(kubelet_runtime_operations_duration_seconds_bucket{job="kubelet"}[5m])) by (le, node, operation_type)
            ) > 30
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "运行时操作延迟高"
            description: "节点 {{ $labels.node }} 的 {{ $labels.operation_type }} 操作 P99 延迟超过 30 秒"
            
        - alert: KubeletRuntimeOperationErrors
          expr: |
            sum(rate(kubelet_runtime_operations_errors_total{job="kubelet"}[5m])) by (node, operation_type) > 0.1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "运行时操作错误"
            description: "节点 {{ $labels.node }} 的 {{ $labels.operation_type }} 操作出现错误"
            
        # =================================================================
        # Pod 数量告警
        # =================================================================
        - alert: KubeletTooManyPods
          expr: |
            kubelet_running_pods{job="kubelet"} / on(node) 
            kube_node_status_allocatable{resource="pods"} > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 Pod 数量接近上限"
            description: "节点 {{ $labels.node }} 的 Pod 数量超过限制的 90%"
            
        # =================================================================
        # 卷操作告警
        # =================================================================
        - alert: KubeletVolumeOperationLatencyHigh
          expr: |
            histogram_quantile(0.99, 
              sum(rate(kubelet_volume_stats_inodes_used{job="kubelet"}[5m])) by (le, node)
            ) / 
            kubelet_volume_stats_inodes > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "卷 Inode 使用率高"
            description: "节点 {{ $labels.node }} 的卷 Inode 使用率超过 90%"
            
        # =================================================================
        # 证书告警
        # =================================================================
        - alert: KubeletCertificateExpiringSoon
          expr: |
            kubelet_certificate_manager_client_ttl_seconds < 86400 * 7
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "Kubelet 证书即将过期"
            description: "节点 {{ $labels.node }} 的 Kubelet 证书将在 7 天内过期"
            
        - alert: KubeletCertificateExpiringCritical
          expr: |
            kubelet_certificate_manager_client_ttl_seconds < 86400
          for: 1h
          labels:
            severity: critical
          annotations:
            summary: "Kubelet 证书即将过期 (紧急)"
            description: "节点 {{ $labels.node }} 的 Kubelet 证书将在 24 小时内过期"
            
    # =================================================================
    # 记录规则
    # =================================================================
    - name: kubelet.recording
      interval: 30s
      rules:
        - record: node:kubelet_running_pods:count
          expr: kubelet_running_pods{job="kubelet"}
          
        - record: node:kubelet_running_containers:count
          expr: kubelet_running_containers{job="kubelet"}
          
        - record: node:kubelet_pod_start_duration:p99
          expr: |
            histogram_quantile(0.99, 
              sum(rate(kubelet_pod_start_duration_seconds_bucket{job="kubelet"}[5m])) by (le, node)
            )
            
        - record: node:kubelet_pleg_relist_duration:p99
          expr: |
            histogram_quantile(0.99, 
              sum(rate(kubelet_pleg_relist_duration_seconds_bucket{job="kubelet"}[5m])) by (le, node)
            )
```

### Grafana Dashboard 配置

```json
{
  "dashboard": {
    "title": "Kubelet Performance Dashboard",
    "uid": "kubelet-performance",
    "panels": [
      {
        "title": "Running Pods per Node",
        "type": "graph",
        "targets": [
          {
            "expr": "kubelet_running_pods{job=\"kubelet\"}",
            "legendFormat": "{{ node }}"
          }
        ]
      },
      {
        "title": "Pod Start Latency P99",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, sum(rate(kubelet_pod_start_duration_seconds_bucket{job=\"kubelet\"}[5m])) by (le, node))",
            "legendFormat": "{{ node }}"
          }
        ]
      },
      {
        "title": "PLEG Relist Duration P99",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, sum(rate(kubelet_pleg_relist_duration_seconds_bucket{job=\"kubelet\"}[5m])) by (le, node))",
            "legendFormat": "{{ node }}"
          }
        ]
      },
      {
        "title": "Runtime Operation Errors",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(kubelet_runtime_operations_errors_total{job=\"kubelet\"}[5m])) by (node, operation_type)",
            "legendFormat": "{{ node }} - {{ operation_type }}"
          }
        ]
      },
      {
        "title": "Node Conditions",
        "type": "stat",
        "targets": [
          {
            "expr": "sum(kube_node_status_condition{condition=~\"MemoryPressure|DiskPressure|PIDPressure\", status=\"true\"}) by (condition)",
            "legendFormat": "{{ condition }}"
          }
        ]
      }
    ]
  }
}
```

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| **v1.31** | 新增 `topologyManagerPolicyOptions` 配置项 | 更精细的 NUMA 调度控制 |
| | Cgroup v2 成为默认 | 需要确保系统支持 |
| **v1.30** | `cpuManagerPolicyOptions` 新增 `distribute-cpus-across-numa` | 改进多 NUMA 节点 CPU 分配 |
| | 移除 `DynamicKubeletConfig` Feature Gate | 不再支持动态配置 |
| **v1.29** | `memoryManagerPolicy` 新增 `NumaAwareEviction` | 改进 NUMA 感知驱逐 |
| | 新增 `localStorageCapacityIsolationFSQuotaMonitoring` | 更精确的存储隔离 |
| **v1.28** | 新增 `KubeletTracing` Feature Gate | 支持分布式追踪 |
| | `SeccompDefault` 升级为 GA | 默认启用 Seccomp |
| **v1.27** | 移除 `dockershim` | 必须使用 containerd/CRI-O |
| | `GracefulNodeShutdown` 升级为 GA | 优雅关闭成为稳定功能 |

## 云厂商特定配置

### 阿里云 ACK 配置

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# ACK 优化配置
maxPods: 256                          # ACK 支持更高密度

# 阿里云专用 Feature Gates
featureGates:
  ExpandCSIVolumes: true
  CSIStorageCapacity: true
  CSIVolumeHealth: true

# 阿里云 GPU 节点优化
topologyManagerPolicy: best-effort

# 阿里云安全加固
authentication:
  anonymous:
    enabled: false
authorization:
  mode: Webhook
```

### AWS EKS 配置

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# EKS 推荐配置
maxPods: 58                           # t3.medium 默认值

# AWS CNI 优化
featureGates:
  PodReadinessGates: true

# 资源预留 (针对 Amazon Linux 2)
kubeReserved:
  cpu: "70m"
  memory: "574Mi"
  ephemeral-storage: "1Gi"
systemReserved:
  cpu: "60m"
  memory: "393Mi"
  ephemeral-storage: "1Gi"
```

### Azure AKS 配置

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# AKS 优化配置
maxPods: 250

# Azure 磁盘优化
featureGates:
  CSIInlineVolume: true
  AzureDiskCSIDriver: true

# 针对 Azure CNI 的配置
networkPlugin: "azure"
```

## 最佳实践总结

### 安全加固清单

- [ ] 禁用匿名认证 (`authentication.anonymous.enabled: false`)
- [ ] 启用 Webhook 授权 (`authorization.mode: Webhook`)
- [ ] 禁用只读端口 (`readOnlyPort: 0`)
- [ ] 配置 TLS 最低版本 (`tlsMinVersion: VersionTLS12`)
- [ ] 启用证书轮转 (`rotateCertificates: true`)
- [ ] 保护内核默认值 (`protectKernelDefaults: true`)
- [ ] 启用默认 Seccomp (`SeccompDefault: true`)

### 性能优化清单

- [ ] 配置适当的资源预留 (`kubeReserved`, `systemReserved`)
- [ ] 调整驱逐阈值防止 OOM (`evictionHard`, `evictionSoft`)
- [ ] 启用 CPU Manager (Guaranteed QoS 工作负载)
- [ ] 配置镜像 GC 策略
- [ ] 优化容器日志大小限制
- [ ] 调整同步频率减少 API Server 压力

### 监控关键指标

- `kubelet_running_pods` - 运行中 Pod 数
- `kubelet_pod_start_duration_seconds` - Pod 启动延迟
- `kubelet_pleg_relist_duration_seconds` - PLEG 延迟
- `kubelet_runtime_operations_duration_seconds` - 运行时操作延迟
- `kubelet_node_status_ready` - 节点就绪状态
- `kube_node_status_condition` - 节点条件状态

---

**参考资料**:
- [Kubernetes Kubelet 官方文档](https://kubernetes.io/docs/reference/command-line-tools-reference/kubelet/)
- [Kubelet Configuration (v1beta1)](https://kubernetes.io/docs/reference/config-api/kubelet-config.v1beta1/)
- [Node 资源管理](https://kubernetes.io/docs/tasks/administer-cluster/reserve-compute-resources/)
