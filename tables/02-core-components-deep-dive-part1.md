```
┌─────────────────────────────────────────────────────────────────────────┐
│                       kubelet Core Modules                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌────────────────── Pod Lifecycle Manager ──────────────────────────┐  │
│  │  syncPod() - 核心调谐函数                                         │  │
│  │  ├── 检查网络就绪 (CNI插件)                                       │  │
│  │  ├── 创建Pod Sandbox (pause容器)                                  │  │
│  │  ├── 启动Init Containers (顺序执行)                               │  │
│  │  ├── 启动Main Containers (并行启动)                               │  │
│  │  ├── 执行PostStart Hook                                           │  │
│  │  └── 持续健康检查 (Liveness/Readiness/Startup)                    │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌────────────────── PLEG (Pod Lifecycle Event Generator) ───────────┐  │
│  │  职责: 监听容器运行时事件，生成Pod状态变化事件                    │  │
│  │                                                                    │  │
│  │  工作流程:                                                         │  │
│  │  1. 每秒调用CRI RuntimeService.ListPodSandbox()                   │  │
│  │  2. 对比上次状态 (Old State vs New State)                         │  │
│  │  3. 生成PodLifecycleEvent (ContainerStarted/Died/Removed)        │  │
│  │  4. 更新Pod状态缓存                                               │  │
│  │  5. 触发syncPod()调谐                                             │  │
│  │                                                                    │  │
│  │  PLEG故障: "PLEG is not healthy: pleg was last seen active..."   │  │
│  │  原因: CRI调用超时 (>3min) / 容器运行时hung                       │  │
│  │  影响: 节点标记NotReady, Pod无法调度到该节点                      │  │
│  │  排查: journalctl -u kubelet | grep PLEG                          │  │
│  │        crictl ps (检查容器运行时)                                 │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌────────────────── Probe Manager (健康检查) ────────────────────────┐  │
│  │  Startup Probe (启动探测):                                        │  │
│  │  └── 成功前阻止Liveness/Readiness探测                             │  │
│  │                                                                    │  │
│  │  Liveness Probe (存活探测):                                       │  │
│  │  └── 失败 → 重启容器                                              │  │
│  │                                                                    │  │
│  │  Readiness Probe (就绪探测):                                      │  │
│  │  └── 失败 → 从Service Endpoints移除                               │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌────────────────── Volume Manager (卷管理) ─────────────────────────┐  │
│  │  ├── AttachDetach Controller (在API Server中)                     │  │
│  │  │   └── Attach云盘到节点                                         │  │
│  │  ├── Volume Plugin Manager                                        │  │
│  │  │   └── Mount卷到Pod (hostPath/emptyDir/CSI)                    │  │
│  │  └── CSI Node Plugin 交互                                         │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌────────────────── Eviction Manager (驱逐管理) ─────────────────────┐  │
│  │  监控节点资源压力:                                                 │  │
│  │  ├── memory.available (可用内存)                                  │  │
│  │  ├── nodefs.available (节点文件系统可用空间)                      │  │
│  │  ├── nodefs.inodesFree (节点文件系统inode可用数)                  │  │
│  │  └── imagefs.available (镜像文件系统可用空间)                     │  │
│  │                                                                    │  │
│  │  Hard Eviction (硬驱逐 - 立即驱逐):                               │  │
│  │  └── memory.available<100Mi                                       │  │
│  │                                                                    │  │
│  │  Soft Eviction (软驱逐 - 宽限期后驱逐):                           │  │
│  │  └── memory.available<200Mi, grace-period=1m30s                   │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌────────────────── cAdvisor (资源监控) ──────────────────────────┐  │
│  │  嵌入kubelet, 采集容器资源指标:                                   │  │
│  │  ├── CPU使用率/限流次数                                           │  │
│  │  ├── 内存使用量/RSS/Cache                                         │  │
│  │  ├── 网络I/O (rx_bytes/tx_bytes)                                  │  │
│  │  └── 磁盘I/O (read_bytes/write_bytes)                             │  │
│  │                                                                    │  │
│  │  暴露端点: http://NODE_IP:10250/metrics/cadvisor                  │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

#### kubelet 生产配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# === 基础配置 ===
clusterDNS:
- 10.96.0.10
clusterDomain: cluster.local
staticPodPath: /etc/kubernetes/manifests

# === 容器运行时 ===
containerRuntimeEndpoint: unix:///run/containerd/containerd.sock
cgroupDriver: systemd  # 与容器运行时一致

# === 资源管理 ===
maxPods: 110  # 单节点最大Pod数 (需与CNI CIDR匹配)

# 系统预留资源
systemReserved:
  cpu: 1000m
  memory: 2Gi
  ephemeral-storage: 10Gi

# kubelet预留资源
kubeReserved:
  cpu: 100m
  memory: 1Gi
  ephemeral-storage: 5Gi

# 强制执行资源预留
enforceNodeAllocatable:
- pods
- system-reserved
- kube-reserved

# === 驱逐配置 ===
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"

evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
  nodefs.inodesFree: "10%"

evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "2m"
  nodefs.inodesFree: "2m"

evictionMaxPodGracePeriod: 90  # 驱逐时最大宽限期

# === 镜像GC ===
imageGCHighThresholdPercent: 80  # 磁盘使用>80%触发GC
imageGCLowThresholdPercent: 70   # GC到磁盘使用<70%
imageMinimumGCAge: 2m            # 镜像最小存活时间

# === 日志轮转 ===
containerLogMaxSize: 10Mi
containerLogMaxFiles: 5

# === 认证授权 ===
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
    cacheTTL: 2m
  x509:
    clientCAFile: /etc/kubernetes/pki/ca.crt

authorization:
  mode: Webhook
  webhook:
    cacheAuthorizedTTL: 5m
    cacheUnauthorizedTTL: 30s

# === 探测配置 ===
nodeStatusUpdateFrequency: 10s
nodeStatusReportFrequency: 5m
runtimeRequestTimeout: 2m  # CRI请求超时

# === 特性门控 ===
featureGates:
  RotateKubeletServerCertificate: true  # 自动轮转证书
  GracefulNodeShutdown: true             # 优雅关闭节点
```

#### kubelet 关键指标

| 指标名称 | 含义 | 健康基准 | 告警阈值 |
|---------|------|---------|---------|
| `kubelet_running_pods` | 运行中的Pod数 | <maxPods | >maxPods*0.9 |
| `kubelet_pod_start_duration_seconds` | Pod启动延迟 | P99 <30s | P99 >60s |
| `kubelet_pleg_relist_duration_seconds` | PLEG重新列出延迟 | P99 <1s | P99 >3s |
| `kubelet_runtime_operations_duration_seconds` | CRI操作延迟 | P99 <1s | P99 >10s |
| `kubelet_node_name` | 节点名称 | - | - |
| `kubelet_volume_stats_*` | 卷统计信息 | - | - |

---

### 2.2 kube-proxy 网络代理

#### kube-proxy 模式对比

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     kube-proxy Mode Comparison                           │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌────────────────── iptables Mode ──────────────────────────────────┐  │
│  │                                                                    │  │
│  │  Client → Service VIP (10.96.0.1:80)                              │  │
│  │      │                                                             │  │
│  │      ▼                                                             │  │
│  │  iptables NAT表 (PREROUTING → KUBE-SERVICES链)                   │  │
│  │      │                                                             │  │
│  │      ├── KUBE-SVC-XXX (Service规则)                               │  │
│  │      │   ├── statistic mode random probability 0.33 → Pod1        │  │
│  │      │   ├── statistic mode random probability 0.50 → Pod2        │  │
│  │      │   └── → Pod3 (最后一个)                                    │  │
│  │      │                                                             │  │
│  │      └── DNAT → Backend Pod IP                                    │  │
│  │                                                                    │  │
│  │  缺点:                                                             │  │
│  │  - O(n)规则匹配 (n=Service数*Pod数)                               │  │
│  │  - 规则更新慢 (需全量刷新)                                        │  │
│  │  - 负载不均衡 (随机概率)                                          │  │
│  │  - 大集群(>5000 Service)性能差                                    │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌────────────────── IPVS Mode (推荐) ────────────────────────────────┐  │
│  │                                                                    │  │
│  │  Client → Service VIP (10.96.0.1:80)                              │  │
│  │      │                                                             │  │
│  │      ▼                                                             │  │
│  │  IPVS虚拟服务器 (kube-ipvs0网卡)                                  │  │
│  │      │                                                             │  │
│  │      ├── Virtual Server: 10.96.0.1:80                             │  │
│  │      │   └── Scheduler: rr (Round Robin)                          │  │
│  │      │       ├── Real Server: 10.244.1.10:80 (Pod1) Weight=1      │  │
│  │      │       ├── Real Server: 10.244.2.20:80 (Pod2) Weight=1      │  │
│  │      │       └── Real Server: 10.244.3.30:80 (Pod3) Weight=1      │  │
│  │      │                                                             │  │
│  │      └── DNAT → Backend Pod IP                                    │  │
│  │                                                                    │  │
│  │  优点:                                                             │  │
│  │  - O(1)哈希表查找                                                  │  │
│  │  - 规则更新快 (增量更新)                                          │  │
│  │  - 多种负载均衡算法                                               │  │
│  │  - 支持大规模集群 (>10000 Service)                                │  │
│  │                                                                    │  │
│  │  负载均衡算法:                                                     │  │
│  │  - rr (Round Robin): 轮询                                         │  │
│  │  - lc (Least Connection): 最少连接                               │  │
│  │  - dh (Destination Hashing): 目标哈希                             │  │
│  │  - sh (Source Hashing): 源哈希 (会话保持)                        │  │
│  │  - sed (Shortest Expected Delay): 最短期望延迟                   │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌────────────────── eBPF Mode (Cilium) ──────────────────────────────┐  │
│  │                                                                    │  │
│  │  Client → XDP/TC eBPF Program (内核态)                            │  │
│  │      │                                                             │  │
│  │      ├── 查询eBPF Map (Service → Endpoints)                       │  │
│  │      ├── 直接修改数据包 (DNAT)                                    │  │
│  │      └── Socket重定向 (Bypass TCP/IP协议栈)                       │  │
│  │                                                                    │  │
│  │  优点:                                                             │  │
│  │  - 最高性能 (内核旁路)                                            │  │
│  │  - L7感知 (HTTP/gRPC负载均衡)                                     │  │
│  │  - 无需kube-proxy                                                 │  │
│  │                                                                    │  │
│  │  缺点:                                                             │  │
│  │  - 需要新内核 (>=4.19)                                            │  │
│  │  - 复杂度高                                                        │  │
│  └────────────────────────────────────────────────────────────────────┘  │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

#### IPVS模式配置

```yaml
# kube-proxy ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    apiVersion: kubeproxy.config.k8s.io/v1alpha1
    kind: KubeProxyConfiguration
    
    # === IPVS配置 ===
    mode: ipvs
    ipvs:
      scheduler: rr  # 调度算法: rr/lc/dh/sh/sed
      strictARP: true  # 严格ARP (配合MetalLB)
      syncPeriod: 30s
      minSyncPeriod: 5s
      excludeCIDRs: []  # 排除CIDR
    
    # === iptables配置 (IPVS模式仍需iptables) ===
    iptables:
      masqueradeAll: false  # SNAT所有流量
      masqueradeBit: 14
      minSyncPeriod: 0s
      syncPeriod: 30s
    
    # === 网络配置 ===
    clusterCIDR: 10.244.0.0/16
    
    # === 监控 ===
    metricsBindAddress: 0.0.0.0:10249
    
    # === Conntrack ===
    conntrack:
      maxPerCore: 32768  # 每核心最大conntrack条目
      min: 131072
      tcpEstablishedTimeout: 86400s  # TCP连接超时
      tcpCloseWaitTimeout: 3600s
```

#### 启用IPVS模式 (kubeadm集群)

```bash
# 1. 加载IPVS内核模块
cat > /etc/modules-load.d/ipvs.conf <<EOF
ip_vs
ip_vs_rr
ip_vs_wrr
ip_vs_sh
nf_conntrack
EOF

modprobe ip_vs
modprobe ip_vs_rr
modprobe ip_vs_wrr
modprobe ip_vs_sh
modprobe nf_conntrack

# 2. 安装ipvsadm工具
apt install ipvsadm -y  # Ubuntu/Debian
yum install ipvsadm -y  # CentOS/RHEL

# 3. 修改kube-proxy ConfigMap (如上)

# 4. 重启kube-proxy
kubectl rollout restart daemonset kube-proxy -n kube-system

# 5. 验证IPVS规则
ipvsadm -Ln
# IP Virtual Server version 1.2.1 (size=4096)
# Prot LocalAddress:Port Scheduler Flags
#   -> RemoteAddress:Port           Forward Weight ActiveConn InActConn
# TCP  10.96.0.1:443 rr
#   -> 192.168.1.101:6443           Masq    1      0          0
```

#### kube-proxy 故障排查

| 故障现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| **Service不通** | iptables/IPVS规则未生成 | `iptables -t nat -L -n -v` (iptables模式) <br> `ipvsadm -Ln` (IPVS模式) | 检查kube-proxy日志 |
| **conntrack表满** | 高并发连接 | `sysctl net.netfilter.nf_conntrack_count` | 增大 `net.netfilter.nf_conntrack_max` |
| **规则同步慢** | 大量Service变更 | 监控 `kubeproxy_sync_proxy_rules_duration_seconds` | 切换IPVS模式 |
| **hairpin流量失败** | hairpin-mode配置错误 | `sysctl net.bridge.bridge-nf-call-iptables` | 设置为1 |

---

### 2.3 容器运行时对比

#### CRI (Container Runtime Interface) 架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       CRI Architecture                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  kubelet                                                                 │
│     │                                                                    │
│     ▼                                                                    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │               CRI gRPC Client                                     │   │
│  │  ┌────────────────────┐  ┌────────────────────┐                  │   │
│  │  │ RuntimeService     │  │  ImageService      │                  │   │
│  │  │                    │  │                    │                  │   │
│  │  │ - RunPodSandbox    │  │ - PullImage        │                  │   │
│  │  │ - StopPodSandbox   │  │ - RemoveImage      │                  │   │
│  │  │ - CreateContainer  │  │ - ImageStatus      │                  │   │
│  │  │ - StartContainer   │  │ - ListImages       │                  │   │
│  │  │ - StopContainer    │  │                    │                  │   │
│  │  │ - RemoveContainer  │  │                    │                  │   │
│  │  │ - ListContainers   │  │                    │                  │   │
│  │  │ - ContainerStatus  │  │                    │                  │   │
│  │  │ - ExecSync         │  │                    │                  │   │
│  │  │ - Exec             │  │                    │                  │   │
│  │  │ - Attach           │  │                    │                  │   │
│  │  │ - PortForward      │  │                    │                  │   │
│  │  └────────────────────┘  └────────────────────┘                  │   │
│  └───────────────────────────┬──────────────────────────────────────┘   │
│                               │ gRPC over Unix Socket                   │
│                               ▼                                          │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │         CRI Runtime (containerd / CRI-O)                          │   │
│  │                                                                   │   │
│  │  ┌─────────────────────────────────────────────────────────────┐ │   │
│  │  │  containerd                                                  │ │   │
│  │  │  ├── Namespace隔离 (k8s.io)                                 │ │   │
│  │  │  ├── Image管理 (pull/push/list/remove)                      │ │   │
│  │  │  ├── Container管理 (create/start/stop/delete)               │ │   │
│  │  │  ├── Snapshot管理 (overlayfs/btrfs)                         │ │   │
│  │  │  └── Task管理 (进程监控)                                    │ │   │
│  │  └─────────────────────────────────────────────────────────────┘ │   │
│  │                               │                                   │   │
│  │                               ▼                                   │   │
│  │  ┌─────────────────────────────────────────────────────────────┐ │   │
│  │  │  OCI Runtime (runc / crun / gVisor / Kata)                  │ │   │
│  │  │  - 创建Linux Namespace (PID/NET/MNT/IPC/UTS)                │ │   │
│  │  │  - 配置cgroup限制 (CPU/Memory/IO)                           │ │   │
│  │  │  - 挂载rootfs (overlayfs)                                   │ │   │
│  │  │  - 执行容器进程                                             │ │   │
│  │  └─────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

#### 容器运行时对比

| 特性 | containerd | CRI-O | Docker (cri-dockerd) |
|------|-----------|-------|---------------------|
| **架构** | 模块化 (containerd → runc) | 极简 (CRI-O → runc) | 单体 (dockerd → containerd → runc) |
| **性能** | 高 | 高 | 中 (多层抽象) |
| **内存占用** | ~50MB | ~30MB | ~100MB |
| **Kubernetes原生** | 是 | 是 | 否 (需cri-dockerd适配) |
| **镜像管理** | containerd CLI / nerdctl | crictl / podman | docker CLI |
| **生态** | CNCF标准 | 与K8s版本对齐 | 最丰富 (但K8s已移除支持) |
| **调试工具** | nerdctl, crictl | crictl, podman | docker (最友好) |
| **适用场景** | 通用生产 | K8s专用 | 开发环境 |
| **厂商支持** | 云原生主流 | OpenShift默认 | 已废弃 |

#### containerd 生产配置

```toml
# /etc/containerd/config.toml
version = 2

# === 根目录 ===
root = "/var/lib/containerd"
state = "/run/containerd"

# === gRPC配置 ===
[grpc]
  address = "/run/containerd/containerd.sock"
  max_recv_message_size = 16777216
  max_send_message_size = 16777216

# === 插件配置 ===
[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    sandbox_image = "registry.k8s.io/pause:3.9"  # pause镜像
    
    # === CNI配置 ===
    [plugins."io.containerd.grpc.v1.cri".cni]
      bin_dir = "/opt/cni/bin"
      conf_dir = "/etc/cni/net.d"
    
    # === 镜像加速器 ===
    [plugins."io.containerd.grpc.v1.cri".registry]
      config_path = "/etc/containerd/certs.d"
      
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
          endpoint = ["https://registry.cn-hangzhou.aliyuncs.com"]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."registry.k8s.io"]
          endpoint = ["https://registry.aliyuncs.com/google_containers"]
    
    # === containerd运行时配置 ===
    [plugins."io.containerd.grpc.v1.cri".containerd]
      default_runtime_name = "runc"
      
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true  # 使用systemd cgroup驱动
        
        # gVisor沙箱运行时 (可选)
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
          runtime_type = "io.containerd.runsc.v1"

# === 日志 ===
[plugins."io.containerd.internal.v1.opt"]
  path = "/opt/containerd"
```

---

## 3. 附加组件