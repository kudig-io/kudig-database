# Kubernetes 核心组件深度剖析 (Core Components Deep Dive)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/concepts/overview/components](https://kubernetes.io/docs/concepts/overview/components/)

## 目录

1. [控制平面组件](#1-控制平面组件)
   - [kube-apiserver 深度剖析](#11-kube-apiserver-深度剖析)
   - [etcd 生产实践](#12-etcd-生产实践)
   - [kube-scheduler 调度优化](#13-kube-scheduler-调度优化)
   - [kube-controller-manager 控制器详解](#14-kube-controller-manager-控制器详解)
   - [cloud-controller-manager 云集成](#15-cloud-controller-manager-云集成)
2. [节点组件](#2-节点组件)
   - [kubelet 运行机制](#21-kubelet-运行机制)
   - [kube-proxy 网络代理](#22-kube-proxy-网络代理)
   - [容器运行时对比](#23-容器运行时对比)
3. [附加组件](#3-附加组件)
4. [组件故障场景与恢复](#4-组件故障场景与恢复)
5. [性能基准与容量规划](#5-性能基准与容量规划)
6. [生产级配置模板](#6-生产级配置模板)
7. [诊断与排查工具](#7-诊断与排查工具)

---

## 控制平面组件

| 组件 | 角色 | 默认端口 | 关键配置标志 | 版本特定变更 | 监控端点 | 故障模式 | 运维最佳实践 |
|-----|------|---------|-------------|-------------|---------|---------|-------------|
| **kube-apiserver** | API网关，认证授权，准入控制 | 6443 | `--etcd-servers`, `--service-cluster-ip-range`, `--authorization-mode` | v1.29: 审计日志增强; v1.30: CEL准入策略GA | `/metrics`, `/healthz`, `/livez`, `/readyz` | 过载导致503; 证书过期 | 启用审计日志; 配置`--max-requests-inflight=400`; 监控请求延迟P99 |
| **etcd** | 分布式一致性存储，集群状态持久化 | 2379/2380 | `--data-dir`, `--quota-backend-bytes`, `--snapshot-count` | v1.6: v3 API默认; v1.22: 3.5.x推荐 | `/metrics`, `/health` | 磁盘满导致只读; 网络分区 | SSD存储; `--quota-backend-bytes=8589934592`(8GB); 每小时自动快照 |
| **kube-scheduler** | Pod到节点的调度决策 | 10259 | `--config`, `--leader-elect` | v1.25: 调度框架Beta; v1.27: 调度门GA | `/metrics`, `/healthz` | 调度延迟过高; 资源计算错误 | 自定义调度配置文件; 监控`scheduler_pending_pods`指标 |
| **kube-controller-manager** | 运行核心控制器循环 | 10257 | `--controllers`, `--concurrent-deployment-syncs`, `--leader-elect` | v1.27: 控制器拆分可选 | `/metrics`, `/healthz` | 控制器goroutine泄漏 | 调整并发参数; 监控控制器队列深度 |
| **cloud-controller-manager** | 云厂商集成（LB、节点、路由）| 10258 | `--cloud-provider`, `--controllers` | v1.25: 外部云控制器稳定 | `/metrics`, `/healthz` | 云API限流; 凭证过期 | 配置云API重试; ACK自动管理 |

## 节点组件

| 组件 | 角色 | 默认端口 | 关键配置标志 | 版本特定变更 | 监控端点 | 故障模式 | 运维最佳实践 |
|-----|------|---------|-------------|-------------|---------|---------|-------------|
| **kubelet** | 节点代理，Pod生命周期管理 | 10250/10255 | `--config`, `--container-runtime-endpoint`, `--max-pods` | v1.24: cgroup v2默认; v1.27: 就地资源调整Alpha | `/metrics`, `/healthz`, `/pods` | PLEG超时; 磁盘压力驱逐 | 配置`--max-pods=110`; 预留系统资源`--system-reserved` |
| **kube-proxy** | Service网络代理 | 10249/10256 | `--proxy-mode`, `--cluster-cidr` | v1.26: nftables Alpha; v1.29: IPVS改进 | `/metrics`, `/healthz` | conntrack表满; iptables规则过多 | 大集群用IPVS模式; 监控`kubeproxy_sync_proxy_rules_duration_seconds` |
| **containerd** | CRI兼容容器运行时 | - | `/etc/containerd/config.toml` | v1.24: 成为默认运行时; v1.30: 2.0支持 | `/metrics` | 镜像拉取失败; 容器创建超时 | 配置镜像加速器; 定期`crictl rmi --prune` |
| **CRI-O** | 轻量级CRI运行时 | - | `/etc/crio/crio.conf` | v1.25+: 与K8S版本对齐 | `/metrics` | 运行时崩溃 | OpenShift默认; 配置适当的运行时类 |

## 附加组件

| 组件 | 角色 | 部署方式 | 关键配置 | 版本特定变更 | 监控指标 | 常见问题 | 运维最佳实践 |
|-----|------|---------|---------|-------------|---------|---------|-------------|
| **CoreDNS** | 集群DNS服务 | Deployment | `Corefile` ConfigMap | v1.28: DNS缓存优化; v1.30: 性能提升 | `coredns_dns_requests_total`, `coredns_dns_responses_total` | 查询超时; 上游失败 | 副本数=max(2, nodes/100); 启用缓存插件 |
| **Metrics Server** | 资源指标API | Deployment | `--kubelet-insecure-tls`(测试) | v0.6+: 稳定API | `metrics_server_*` | kubelet连接失败 | 生产使用TLS; HPA/VPA依赖 |
| **Ingress Controller** | 入口流量路由 | Deployment/DaemonSet | 因控制器而异 | v1.19: networking.k8s.io/v1稳定 | 控制器特定 | 证书错误; 后端不可达 | 高可用部署; 配置健康检查 |
| **CNI Plugin** | 容器网络 | DaemonSet/主机安装 | `/etc/cni/net.d/` | v1.25: 双栈增强; v1.28: 网络策略改进 | 插件特定 | IP分配失败; 网络分区 | 选择适合场景的CNI; 监控网络指标 |

## 组件版本兼容性矩阵

| 组件 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|-----|-------|-------|-------|-------|-------|-------|-------|-------|
| **etcd** | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x |
| **containerd** | 1.6+ | 1.6+ | 1.7+ | 1.7+ | 1.7+ | 1.7+/2.0 | 1.7+/2.0 | 2.0+ |
| **CoreDNS** | 1.9+ | 1.9+ | 1.10+ | 1.10+ | 1.11+ | 1.11+ | 1.11+ | 1.11+ |
| **Metrics Server** | 0.6+ | 0.6+ | 0.6+ | 0.7+ | 0.7+ | 0.7+ | 0.7+ | 0.7+ |
| **Calico** | 3.24+ | 3.25+ | 3.26+ | 3.27+ | 3.27+ | 3.28+ | 3.28+ | 3.28+ |
| **Cilium** | 1.12+ | 1.13+ | 1.14+ | 1.14+ | 1.15+ | 1.15+ | 1.16+ | 1.16+ |

## 组件资源推荐配置

| 组件 | 小集群(<50节点) | 中集群(50-200节点) | 大集群(200-1000节点) | 超大集群(1000+节点) |
|-----|----------------|-------------------|---------------------|-------------------|
| **kube-apiserver** | 2核4G | 4核8G | 8核16G | 16核32G |
| **etcd** | 2核4G SSD | 4核8G SSD | 8核16G NVMe | 16核32G NVMe |
| **kube-scheduler** | 1核2G | 2核4G | 4核8G | 8核16G |
| **kube-controller-manager** | 2核4G | 4核8G | 8核16G | 16核32G |
| **CoreDNS** | 2副本,100m/128Mi | 3副本,200m/256Mi | 5副本,500m/512Mi | 10副本,1核/1Gi |
| **Metrics Server** | 1副本,100m/200Mi | 2副本,200m/400Mi | 3副本,500m/1Gi | 5副本,1核/2Gi |

## 组件启动顺序与依赖

| 启动顺序 | 组件 | 依赖条件 | 启动超时 | 健康检查 | 故障影响 |
|---------|------|---------|---------|---------|---------|
| 1 | **etcd** | 网络, 存储 | 60s | `etcdctl endpoint health` | 整个集群不可用 |
| 2 | **kube-apiserver** | etcd健康 | 30s | `curl -k https://localhost:6443/healthz` | 所有API调用失败 |
| 3 | **kube-controller-manager** | apiserver可用 | 30s | `/healthz` 端点 | 控制器停止协调 |
| 4 | **kube-scheduler** | apiserver可用 | 30s | `/healthz` 端点 | 新Pod无法调度 |
| 5 | **kubelet** | apiserver可用 | 30s | `curl http://localhost:10248/healthz` | 节点NotReady |
| 6 | **kube-proxy** | apiserver, kubelet | 30s | `/healthz` 端点 | Service网络故障 |
| 7 | **CoreDNS** | kube-proxy, CNI | 60s | DNS查询测试 | 服务发现失败 |
| 8 | **CNI** | kubelet | 60s | Pod网络测试 | Pod网络不通 |

## 关键配置参数速查

| 组件 | 参数 | 默认值 | 推荐生产值 | 说明 | 版本注意 |
|-----|------|-------|----------|------|---------|
| **apiserver** | `--max-requests-inflight` | 400 | 800 | 最大并发非变更请求 | - |
| **apiserver** | `--max-mutating-requests-inflight` | 200 | 400 | 最大并发变更请求 | - |
| **apiserver** | `--watch-cache-sizes` | 默认 | 根据对象调整 | Watch缓存大小 | v1.28+优化 |
| **etcd** | `--quota-backend-bytes` | 2GB | 8GB | 存储配额 | - |
| **etcd** | `--auto-compaction-retention` | 0 | 1h | 自动压缩保留 | - |
| **kubelet** | `--max-pods` | 110 | 110-250 | 单节点最大Pod数 | 需调整CIDR |
| **kubelet** | `--image-gc-high-threshold` | 85 | 80 | 镜像GC高水位 | - |
| **kubelet** | `--eviction-hard` | 默认 | 自定义 | 驱逐阈值 | v1.26+增强 |
| **scheduler** | `--kube-api-qps` | 50 | 100 | API请求QPS | - |
| **controller** | `--concurrent-deployment-syncs` | 5 | 10 | Deployment并发数 | - |
| **kube-proxy** | `--proxy-mode` | iptables | ipvs | 代理模式 | 大集群推荐IPVS |

## 组件日志关键字与排查

| 组件 | 日志路径/命令 | 关键错误关键字 | 可能原因 | 排查步骤 |
|-----|--------------|---------------|---------|---------|
| **apiserver** | `journalctl -u kube-apiserver` | `connection refused`, `etcd leader changed` | etcd不健康 | 检查etcd状态 |
| **etcd** | `journalctl -u etcd` | `mvcc: database space exceeded`, `rafthttp: failed to dial` | 存储满/网络问题 | 压缩+碎片整理 |
| **scheduler** | `kubectl logs -n kube-system kube-scheduler-*` | `unable to schedule pod`, `Preempting` | 资源不足 | 检查节点资源 |
| **kubelet** | `journalctl -u kubelet` | `PLEG is not healthy`, `failed to pull image` | 容器运行时问题 | 重启containerd |
| **kube-proxy** | `kubectl logs -n kube-system kube-proxy-*` | `conntrack table full` | conntrack表满 | 增大conntrack限制 |
| **CoreDNS** | `kubectl logs -n kube-system coredns-*` | `i/o timeout`, `SERVFAIL` | 上游DNS问题 | 检查上游DNS |

## ACK特定组件集成

| 组件 | ACK托管方式 | 配置入口 | 特殊优化 | 监控集成 |
|-----|------------|---------|---------|---------|
| **控制平面** | 全托管(Pro版) | 控制台 | 自动HA, 自动升级 | ARMS自动接入 |
| **etcd** | 托管或独立 | 控制台 | 自动备份 | 云监控集成 |
| **CoreDNS** | 托管 | ConfigMap | 阿里云DNS集成 | SLS日志 |
| **Ingress** | ALB/Nginx可选 | 控制台+YAML | SLB自动绑定 | ALB监控 |
| **CNI** | Terway/Flannel | 创建时选择 | ENI直通性能 | 网络监控 |

---

**诊断脚本示例**:
```bash
# 快速检查所有组件状态
kubectl get componentstatuses  # 已弃用但仍可用
kubectl get --raw='/readyz?verbose'
kubectl get nodes -o wide
kubectl get pods -n kube-system
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)```
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

## 3. 附加组件| 组件 | 角色 | 部署方式 | 关键配置 | 版本特定变更 | 监控指标 | 常见问题 | 运维最佳实践 |
|-----|------|---------|---------|-------------|---------|---------|-------------|
| **CoreDNS** | 集群DNS服务 | Deployment | `Corefile` ConfigMap | v1.28: DNS缓存优化; v1.30: 性能提升 | `coredns_dns_requests_total`, `coredns_dns_responses_total` | 查询超时; 上游失败 | 副本数=max(2, nodes/100); 启用缓存插件 |
| **Metrics Server** | 资源指标API | Deployment | `--kubelet-insecure-tls`(测试) | v0.6+: 稳定API | `metrics_server_*` | kubelet连接失败 | 生产使用TLS; HPA/VPA依赖 |
| **Ingress Controller** | 入口流量路由 | Deployment/DaemonSet | 因控制器而异 | v1.19: networking.k8s.io/v1稳定 | 控制器特定 | 证书错误; 后端不可达 | 高可用部署; 配置健康检查 |
| **CNI Plugin** | 容器网络 | DaemonSet/主机安装 | `/etc/cni/net.d/` | v1.25: 双栈增强; v1.28: 网络策略改进 | 插件特定 | IP分配失败; 网络分区 | 选择适合场景的CNI; 监控网络指标 |

---

## 4. 组件故障场景与恢复

### 4.1 控制平面故障场景

#### API Server 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **503 Service Unavailable** | 请求过载/etcd不健康 | 1. 检查`apiserver_current_inflight_requests`<br>2. 检查etcd健康 | 1. 增大`--max-requests-inflight`<br>2. 修复etcd | 启用APF流控; 监控P99延迟 |
| **证书过期** | 证书未自动轮转 | `kubeadm certs check-expiration` | `kubeadm certs renew all` | 启用证书自动轮转; 监控证书有效期 |
| **OOM Killed** | 内存不足 | `journalctl -u kube-apiserver` | 1. 增加内存限制<br>2. 减小Watch缓存 | 根据集群规模配置资源 |
| **审计日志磁盘满** | 审计日志未轮转 | `df -h /var/log` | 清理旧日志 | 配置日志轮转参数 |
| **etcd连接超时** | etcd响应慢/网络抖动 | 检查`etcd_request_duration_seconds` | 1. 优化etcd<br>2. 增加超时时间 | SSD存储; 压缩etcd历史版本 |

#### etcd 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **数据库空间满** | 超过quota-backend-bytes | `etcdctl alarm list` | 1. 压缩`etcdctl compact`<br>2. 碎片整理`etcdctl defrag`<br>3. 解除告警`etcdctl alarm disarm` | 自动压缩; 监控DB大小 |
| **Leader频繁切换** | 网络抖动/磁盘IO慢 | 检查`etcd_server_leader_changes_seen_total` | 1. 检查网络延迟<br>2. 更换SSD/NVMe<br>3. 调整心跳参数 | 专用网络; NVMe存储 |
| **集群脑裂** | 网络分区 | `etcdctl member list` | 1. 恢复网络<br>2. 移除故障节点`etcdctl member remove`<br>3. 添加新成员 | 3/5/7奇数节点; 跨机架部署 |
| **单节点故障** | 进程崩溃/磁盘损坏 | `systemctl status etcd` | 1. 重启etcd<br>2. 从快照恢复<br>3. 重新加入集群 | 定期备份快照; RAID磁盘 |
| **fsync延迟高** | 磁盘IO慢 | `etcd_disk_wal_fsync_duration_seconds` > 10ms | 更换SSD/NVMe; WAL独立分区 | 专用SSD; 禁用swap |

#### Scheduler 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **Pod长时间Pending** | 资源不足/调度约束 | `kubectl describe pod` 查看Events | 1. 增加节点<br>2. 放宽调度约束<br>3. 启用抢占 | 监控资源使用率; 预留buffer |
| **调度延迟高** | 大量待调度Pod/Filter插件慢 | 检查`scheduler_scheduling_attempt_duration_seconds` | 1. 增加Scheduler副本<br>2. 优化调度配置<br>3. 提高`--kube-api-qps` | 批量调度; 自定义调度器 |
| **Leader选举失败** | API Server不可达 | `kubectl logs -n kube-system kube-scheduler-*` | 修复API Server | HA部署Scheduler |

#### Controller Manager 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **Deployment不滚动更新** | Controller故障/API限流 | 检查`workqueue_depth` | 1. 重启Controller<br>2. 提高API QPS | 监控控制器队列深度 |
| **EndpointSlice未更新** | EndpointSlice Controller异常 | `kubectl get endpointslices` | 重启Controller Manager | 监控Service流量 |
| **Node驱逐失败** | 驱逐速率限制 | 检查`--node-eviction-rate` | 调整驱逐参数 | 配置合理驱逐速率 |

---

### 4.2 节点组件故障场景

#### kubelet 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **PLEG不健康** | CRI调用超时/容器运行时hung | `journalctl -u kubelet \| grep PLEG` | 1. 重启containerd<br>2. 重启kubelet<br>3. 检查磁盘IO | SSD存储; 监控PLEG延迟 |
| **Node NotReady** | kubelet未上报心跳 | `kubectl describe node` | 1. 重启kubelet<br>2. 检查网络 | 监控Node状态; 配置健康检查 |
| **磁盘压力驱逐** | 磁盘空间不足 | 检查`nodefs.available` | 1. 清理镜像`crictl rmi --prune`<br>2. 清理日志<br>3. 扩容磁盘 | 配置镜像GC; 日志轮转 |
| **内存压力驱逐** | 内存不足 | 检查`memory.available` | 1. 增加节点内存<br>2. 减少Pod副本 | 配置资源请求; 监控内存使用 |
| **CRI调用超时** | containerd响应慢 | `crictl ps` 超时 | 1. 重启containerd<br>2. 检查磁盘IO | 独立数据盘; 限制容器日志大小 |
| **证书轮转失败** | CSR未批准 | `kubectl get csr` | `kubectl certificate approve` | 启用自动批准CSR |

#### kube-proxy 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **Service不通** | iptables/IPVS规则未生成 | `iptables -t nat -L` / `ipvsadm -Ln` | 1. 重启kube-proxy<br>2. 检查日志 | 监控规则同步延迟 |
| **conntrack表满** | 高并发连接数 | `sysctl net.netfilter.nf_conntrack_count` | 增大`net.netfilter.nf_conntrack_max` | 调整conntrack参数; 启用conntrack清理 |
| **规则同步慢** | 大量Service变更 | 检查`kubeproxy_sync_proxy_rules_duration_seconds` | 切换IPVS模式 | 使用IPVS; 批量更新Service |
| **IPVS模块未加载** | 内核模块缺失 | `lsmod \| grep ip_vs` | `modprobe ip_vs` | 配置模块自动加载 |

#### Container Runtime 故障场景

| 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|---------|------|---------|---------|---------|
| **镜像拉取失败** | 网络问题/镜像不存在/认证失败 | `crictl pull <image>` | 1. 检查网络<br>2. 配置镜像加速器<br>3. 检查Secret | 使用内网镜像仓库; 预拉镜像 |
| **容器创建超时** | 磁盘IO慢/overlayfs问题 | `crictl ps -a` | 1. 重启containerd<br>2. 清理未使用镜像 | SSD存储; 定期GC |
| **containerd进程崩溃** | OOM/磁盘满 | `journalctl -u containerd` | 1. 增加内存<br>2. 扩容磁盘<br>3. 重启服务 | 监控资源使用; 限制容器日志 |
| **cgroup v2兼容问题** | 旧版containerd | 检查`/sys/fs/cgroup` | 升级containerd >= 1.6 | 使用新版运行时 |

---

## 5. 性能基准与容量规划

### 5.1 性能基准测试数据

#### API Server 性能基准

| 场景 | 集群规模 | QPS | P50延迟 | P99延迟 | 资源配置 |
|------|---------|-----|---------|---------|---------|
| **List Pods** | 5000 Pods | 100 | 50ms | 200ms | 4C8G |
| **Watch Pods** | 5000 Pods | 1000 events/s | 10ms | 50ms | 4C8G |
| **Create Pod** | 1000 req/s | 1000 | 100ms | 500ms | 8C16G |
| **Update Deployment** | 100 req/s | 100 | 150ms | 800ms | 4C8G |

#### etcd 性能基准

| 操作 | 延迟(SSD) | 延迟(NVMe) | 吞吐量 | 测试工具 |
|------|----------|-----------|--------|---------|
| **Write (fsync)** | 5-10ms | 1-3ms | 10000 writes/s | etcdctl benchmark |
| **Read** | <1ms | <0.5ms | 50000 reads/s | etcdctl benchmark |
| **Watch** | - | - | 10000 events/s | - |

```bash
# etcd性能测试
# 写入测试
etcdctl benchmark put --total=100000 --key-size=8 --val-size=256

# 读取测试
etcdctl benchmark range --total=100000 --key-size=8

# 混合测试
etcdctl benchmark txn-put --total=100000 --key-size=8 --val-size=256
```

#### Scheduler 性能基准

| 指标 | 小集群(<50节点) | 中集群(50-250节点) | 大集群(250-1000节点) |
|------|----------------|-------------------|---------------------|
| **调度吞吐量** | 100 Pods/s | 50 Pods/s | 20 Pods/s |
| **调度延迟 P99** | 50ms | 200ms | 1s |
| **内存使用** | 500MB | 2GB | 8GB |

---

### 5.2 容量规划指南

#### 集群规模评估

| 维度 | 小集群 | 中集群 | 大集群 | 超大集群 |
|------|-------|--------|--------|---------|
| **节点数** | <50 | 50-250 | 250-1000 | >1000 |
| **Pod数** | <1500 | 1500-7500 | 7500-30000 | >30000 |
| **Service数** | <500 | 500-2500 | 2500-10000 | >10000 |
| **Deployment数** | <200 | 200-1000 | 1000-5000 | >5000 |
| **CRD对象数** | <1000 | 1000-10000 | 10000-50000 | >50000 |

#### 控制平面资源配置

| 组件 | 小集群 | 中集群 | 大集群 | 超大集群 |
|------|-------|--------|--------|---------|
| **API Server** | 2C4G × 2 | 4C8G × 3 | 8C16G × 3 | 16C32G × 5 |
| **etcd** | 2C4G SSD × 3 | 4C8G SSD × 3 | 8C16G NVMe × 5 | 16C32G NVMe × 7 |
| **Scheduler** | 1C2G × 2 | 2C4G × 3 | 4C8G × 3 | 8C16G × 5 |
| **Controller Manager** | 2C4G × 2 | 4C8G × 3 | 8C16G × 3 | 16C32G × 5 |
| **合计 (最小)** | 14C24G | 30C56G | 60C120G | 130C260G |

#### 节点资源预留

```yaml
# kubelet配置 - 4C16G节点示例
systemReserved:
  cpu: 500m        # 系统进程预留
  memory: 1Gi
  ephemeral-storage: 10Gi

kubeReserved:
  cpu: 100m        # kubelet预留
  memory: 500Mi
  ephemeral-storage: 5Gi

# 可用于Pod的资源
# CPU: 4核 - 500m - 100m = 3400m
# Memory: 16Gi - 1Gi - 500Mi = 14.5Gi
```

#### etcd 存储容量规划

| 对象类型 | 单对象大小 | 10000对象 | 50000对象 | 100000对象 |
|---------|----------|----------|----------|-----------|
| **Pod** | ~5KB | 50MB | 250MB | 500MB |
| **Service** | ~2KB | 20MB | 100MB | 200MB |
| **Endpoint** | ~1KB | 10MB | 50MB | 100MB |
| **Event** | ~1KB | 10MB | 50MB | 100MB |
| **ConfigMap** | ~10KB | 100MB | 500MB | 1GB |
| **Secret** | ~5KB | 50MB | 250MB | 500MB |

```
推荐etcd存储配额:
- 小集群: 4GB
- 中集群: 8GB
- 大集群: 16GB
- 超大集群: 32GB
```

#### 网络带宽规划

| 场景 | 带宽需求 | 说明 |
|------|---------|------|
| **控制平面互联** | 1Gbps | Master节点间 |
| **Node → Master** | 100Mbps | 心跳+指标上报 |
| **Node间通信** | 10Gbps | Pod互访 (高流量场景) |
| **镜像拉取** | 1Gbps | 加速启动 |

---

## 6. 生产级配置模板

### 6.1 HA控制平面 kubeadm 配置

```yaml
# kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
controlPlaneEndpoint: "loadbalancer.example.com:6443"  # LB地址
certificatesDir: /etc/kubernetes/pki
imageRepository: registry.k8s.io
clusterName: production-cluster

# === etcd配置 ===
etcd:
  external:
    endpoints:
    - https://192.168.1.101:2379
    - https://192.168.1.102:2379
    - https://192.168.1.103:2379
    caFile: /etc/kubernetes/pki/etcd/ca.crt
    certFile: /etc/kubernetes/pki/apiserver-etcd-client.crt
    keyFile: /etc/kubernetes/pki/apiserver-etcd-client.key

# === 网络配置 ===
networking:
  serviceSubnet: 10.96.0.0/12
  podSubnet: 10.244.0.0/16
  dnsDomain: cluster.local

# === API Server配置 ===
apiServer:
  timeoutForControlPlane: 5m
  certSANs:
  - loadbalancer.example.com
  - 192.168.1.100
  - 127.0.0.1
  extraArgs:
    # 并发控制
    max-requests-inflight: "800"
    max-mutating-requests-inflight: "400"
    # 审计日志
    audit-log-path: /var/log/kubernetes/audit.log
    audit-log-maxage: "30"
    audit-log-maxbackup: "10"
    audit-log-maxsize: "100"
    audit-policy-file: /etc/kubernetes/audit-policy.yaml
    # etcd优化
    etcd-compaction-interval: "5m"
    # 事件TTL
    event-ttl: "1h"
  extraVolumes:
  - name: audit-policy
    hostPath: /etc/kubernetes/audit-policy.yaml
    mountPath: /etc/kubernetes/audit-policy.yaml
    readOnly: true
  - name: audit-log
    hostPath: /var/log/kubernetes
    mountPath: /var/log/kubernetes

# === Scheduler配置 ===
scheduler:
  extraArgs:
    kube-api-qps: "100"
    kube-api-burst: "200"
    config: /etc/kubernetes/scheduler-config.yaml
  extraVolumes:
  - name: scheduler-config
    hostPath: /etc/kubernetes/scheduler-config.yaml
    mountPath: /etc/kubernetes/scheduler-config.yaml
    readOnly: true

# === Controller Manager配置 ===
controllerManager:
  extraArgs:
    # 并发控制
    concurrent-deployment-syncs: "10"
    concurrent-replicaset-syncs: "10"
    concurrent-statefulset-syncs: "10"
    # API限流
    kube-api-qps: "100"
    kube-api-burst: "200"
    # 节点驱逐
    node-eviction-rate: "0.1"
    pod-eviction-timeout: "5m"

---
# kubelet配置
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cgroupDriver: systemd
maxPods: 110
systemReserved:
  cpu: 500m
  memory: 1Gi
kubeReserved:
  cpu: 100m
  memory: 500Mi
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
imageGCHighThresholdPercent: 80
imageGCLowThresholdPercent: 70
```

---

## 7. 诊断与排查工具

### 7.1 快速诊断脚本

```bash
#!/bin/bash
# k8s-healthcheck.sh - Kubernetes集群健康检查

echo "=== 1. 集群基础信息 ==="
kubectl version --short
kubectl cluster-info

echo -e "\n=== 2. 节点状态 ==="
kubectl get nodes -o wide

echo -e "\n=== 3. 控制平面组件状态 ==="
kubectl get pods -n kube-system -o wide

echo -e "\n=== 4. 组件健康检查 ==="
echo "API Server:"
curl -k https://127.0.0.1:6443/healthz && echo " - OK"

echo "etcd:"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health

echo -e "\n=== 5. 资源使用情况 ==="
kubectl top nodes

echo -e "\n=== 6. 关键指标检查 ==="
echo "待调度Pod数:"
kubectl get pods --all-namespaces --field-selector=status.phase=Pending | wc -l

echo "NotReady节点数:"
kubectl get nodes --no-headers | grep -v "Ready" | grep "NotReady" | wc -l

echo "CrashLoopBackOff Pod数:"
kubectl get pods --all-namespaces --field-selector=status.phase=Running | \
  grep -c "CrashLoopBackOff"

echo -e "\n=== 7. 证书有效期检查 ==="
kubeadm certs check-expiration 2>/dev/null || echo "需在master节点执行"

echo -e "\n=== 8. etcd数据库大小 ==="
ETCDCTL_API=3 etcdctl endpoint status --write-out=table \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

echo -e "\n=== 9. 组件日志快速检查 (最近10行错误) ==="
echo "kubelet:"
journalctl -u kubelet --no-pager -n 10 | grep -i error

echo "containerd:"
journalctl -u containerd --no-pager -n 10 | grep -i error
```

### 7.2 常用诊断命令

```bash
# === 组件状态检查 ===
# 检查所有组件
kubectl get componentstatuses

# 详细API就绪检查
kubectl get --raw='/readyz?verbose'

# === 节点诊断 ===
# 节点详细信息
kubectl describe node <node-name>

# 节点容量与分配
kubectl describe node <node-name> | grep -A 5 "Allocated resources"

# 驱逐Pod
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# === Pod诊断 ===
# Pod详细信息
kubectl describe pod <pod-name> -n <namespace>

# Pod日志 (最近100行)
kubectl logs <pod-name> -n <namespace> --tail=100

# 前一个容器日志 (重启后查看)
kubectl logs <pod-name> -n <namespace> --previous

# 进入Pod调试
kubectl exec -it <pod-name> -n <namespace> -- sh

# === etcd诊断 ===
# 查看成员列表
ETCDCTL_API=3 etcdctl member list --write-out=table

# 查看告警
ETCDCTL_API=3 etcdctl alarm list

# 查看性能指标
ETCDCTL_API=3 etcdctl endpoint health
ETCDCTL_API=3 etcdctl endpoint status --write-out=table

# === 容器运行时诊断 ===
# 列出所有容器
crictl ps -a

# 查看容器日志
crictl logs <container-id>

# 检查镜像
crictl images

# 容器详细信息
crictl inspect <container-id>

# === 网络诊断 ===
# Service Endpoints
kubectl get endpoints <service-name> -n <namespace>

# Service详细信息
kubectl describe service <service-name> -n <namespace>

# IPVS规则 (IPVS模式)
ipvsadm -Ln

# iptables规则 (iptables模式)
iptables -t nat -L -n -v | grep <service-name>

# DNS测试
kubectl run -it --rm debug --image=busybox --restart=Never -- nslookup kubernetes
```

---

## ACK特定组件集成

| 组件 | ACK托管方式 | 配置入口 | 特殊优化 | 监控集成 |
|-----|------------|---------|---------|---------|
| **控制平面** | 全托管(Pro版) | 控制台 | 自动HA, 自动升级 | ARMS自动接入 |
| **etcd** | 托管或独立 | 控制台 | 自动备份 | 云监控集成 |
| **CoreDNS** | 托管 | ConfigMap | 阿里云DNS集成 | SLS日志 |
| **Ingress** | ALB/Nginx可选 | 控制台+YAML | SLB自动绑定 | ALB监控 |
| **CNI** | Terway/Flannel | 创建时选择 | ENI直通性能 | 网络监控 |

---

## 组件版本兼容性矩阵

| 组件 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|-----|-------|-------|-------|-------|-------|-------|-------|-------|
| **etcd** | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x |
| **containerd** | 1.6+ | 1.6+ | 1.7+ | 1.7+ | 1.7+ | 1.7+/2.0 | 1.7+/2.0 | 2.0+ |
| **CoreDNS** | 1.9+ | 1.9+ | 1.10+ | 1.10+ | 1.11+ | 1.11+ | 1.11+ | 1.11+ |
| **Metrics Server** | 0.6+ | 0.6+ | 0.6+ | 0.7+ | 0.7+ | 0.7+ | 0.7+ | 0.7+ |
| **Calico** | 3.24+ | 3.25+ | 3.26+ | 3.27+ | 3.27+ | 3.28+ | 3.28+ | 3.28+ |
| **Cilium** | 1.12+ | 1.13+ | 1.14+ | 1.14+ | 1.15+ | 1.15+ | 1.16+ | 1.16+ |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-01
