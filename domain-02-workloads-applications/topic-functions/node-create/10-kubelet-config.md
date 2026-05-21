---
title: kubelet 进阶配置
description: '# kubelet 进阶配置 — cgroup / 资源管理 / 日志'
category: functions
tags:
- k8s
- operations
- cluster-management
- kubelet
- coredns
- containerd
- webhook
- rag
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- kubelet configuration cgroup driver systemd
- KubeletConfiguration complete reference
- kubelet resource reservation kube-reserved
- evictionHard evictionSoft configuration
- kubelet logging configuration
trigger_keywords:
- kubelet
- config
- cgroup
- systemd
- cgroupfs
- resource reservation
- kube-reserved
- system-reserved
- evictionHard
- evictionSoft
- maxPods
- podPidsLimit
- rotateCertificates
- serverTLSBootstrap
- logging
- v=2
prerequisites:
- kubectl-basics
- pod-lifecycle
related_domains:
- domain-01-cluster-fundamentals
- domain-10-troubleshooting-diagnostics
related_topics:
- node-create/06-certificate
- node-create/11-eviction
- node-create/02-registration
---

# kubelet 进阶配置 — cgroup / 资源管理 / 日志

## 概述

kubelet 是每个 [[entities/kubernetes|kubernetes]] 节点上最核心的组件，它的配置直接影响节点的稳定性、性能和安全性。kubelet 的配置通过 `KubeletConfiguration` API 定义，以 YAML 文件形式存储在 `/var/lib/kubelet/config.yaml`。

在生产环境中，合理的 kubelet 配置至关重要：

- **cgroup driver**：与容器运行时使用相同的 cgroup driver 是节点正常工作的前提
- **资源预留**：为系统和 kubelet 预留足够的资源，防止工作负载耗尽节点资源
- **驱逐管理**：设置合理的驱逐阈值，在资源紧张时主动驱逐低优先级 Pod
- **容器限制**：限制单节点 Pod 数量和每 Pod 的 PID 数量，防止资源耗尽攻击
- **日志级别**：合理配置日志级别，在问题排查和性能之间取得平衡

本文档详细分析 kubelet 的核心配置参数、配置文件结构、各参数的作用和推荐值，以及常见配置错误和解决方案。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| KubeletConfiguration | `pkg/kubelet/apis/config/` | 配置 API 定义 |
| kubelet 主入口 | `pkg/kubelet/kubelet.go` | kubelet 核心逻辑 |
| cgroup 管理 | `pkg/kubelet/cm/` | cgroup 管理器 |
| 驱逐管理 | `pkg/kubelet/eviction/` | 驱逐逻辑 |
| 资源管理 | `pkg/kubelet/cm/` | 容器资源管理 |
| 配置验证 | `pkg/kubelet/apis/config/validation/` | 配置校验 |

---

## 一、kubelet 配置文件详解

### 1.1 完整配置示例

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# --- 网络配置 ---
address: 0.0.0.0                 # kubelet 监听地址
port: 10250                       # kubelet HTTPS 端口
readOnlyPort: 10255               # 只读端口（已废弃，v1.28 移除）

# --- 容器运行时 ---
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock
cgroupDriver: systemd             # cgroup driver: systemd 或 cgroupfs
cgroupVersion: 2                  # cgroup 版本: 1 或 2

# --- 认证与授权 ---
authentication:
  anonymous:
    enabled: false                # 禁止匿名访问
  webhook:
    enabled: true                 # 使用 API Server 认证
    cacheTTL: 2h0m0s
  bootstrap:
    enabled: true                 # 启用 Bootstrap
authorization:
  mode: Webhook                   # 使用 API Server 授权

# --- 证书管理 ---
serverTLSBootstrap: true          # 服务端证书通过 CSR 签发
rotateCertificates: true          # 客户端证书自动轮换

# --- 驱逐配置 ---
evictionHard:
  memory.available: "100Mi"       # 硬驱逐：可用内存 < 100Mi
  nodefs.available: "10%"         # 硬驱逐：节点磁盘 < 10%
  imagefs.available: "15%"        # 硬驱逐：镜像磁盘 < 15%
  nodefs.inodesFree: "5%"         # 硬驱逐：inode < 5%
evictionSoft:
  memory.available: "200Mi"       # 软驱逐：可用内存 < 200Mi
  nodefs.available: "15%"         # 软驱逐：节点磁盘 < 15%
evictionSoftGracePeriod:
  memory.available: "1m30s"       # 软驱逐宽限期
  nodefs.available: "2m"
evictionMinimumReclaim:
  memory.available: "50Mi"        # 驱逐后至少回收 50Mi
evictionPressureTransitionPeriod: 5m  # 驱逐状态转换延迟

# --- 容器限制 ---
maxPods: 110                      # 单节点最大 Pod 数
podPidsLimit: 4096                # 单 Pod 最大 PID 数

# --- 超时配置 ---
runtimeRequestTimeout: 2m0s       # CRI 请求超时
syncFrequency: 1m0s               # Pod 同步频率
fileCheckFrequency: 20s           # 静态 Pod 文件检查频率
httpCheckFrequency: 20s           # HTTP 健康检查频率

# --- 其他 ---
clusterDNS:
  - 10.96.0.10                    # CoreDNS Service IP
clusterDomain: cluster.local      # 集群域名
serializeImagePulls: false        # 并行拉取镜像（需要 containerd 支持）
registryPullQPS: 5                # 镜像拉取 QPS 限制
registryBurst: 10                 # 镜像拉取突发限制
```

---

## 二、cgroup Driver 配置

### 2.1 systemd vs cgroupfs

cgroup driver 决定了 kubelet 如何管理容器的 cgroup 层级。它**必须**与容器运行时使用相同的 driver，否则会导致资源限制失效、Pod 无法启动等严重问题。

| 特性 | systemd | cgroupfs |
|------|---------|----------|
| 管理方式 | systemd 单元管理 | 直接操作 cgroup 文件系统 |
| 推荐程度 | **推荐**（kubeadm 默认） | 不推荐 |
| 兼容性 | 与 systemd 服务管理集成 | 独立管理 |
| 资源追踪 | 通过 systemctl status 查看 | 通过 /sys/fs/cgroup/ 查看 |

### 2.2 检查和配置 cgroup driver

```bash
# 检查 kubelet 当前 cgroup driver
kubectl get --raw /api/v1/nodes/<node>/proxy/configz | jq '.kubeletconfig.cgroupDriver'

# 检查容器运行时 cgroup driver
# containerd:
containerd config dump | grep systemd
# 或
cat /etc/containerd/config.toml | grep systemd

# 检查系统 cgroup 版本
stat -fc %T /sys/fs/cgroup/
# cgroup2fs → cgroup v2
# tmpfs → cgroup v1

# 配置 containerd 使用 systemd driver
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd]
  runc_options = { SystemdCgroup = true }
```

### 2.3 cgroup 版本

```bash
# cgroup v1 (传统)
# - 每个子系统（cpu, memory, blkio）独立的层级
# - 兼容性好，但功能有限

# cgroup v2 (推荐)
# - 统一的层级结构
# - 更好的资源控制（如 memory.max 用于限制，memory.peak 用于监控）
# - 压力通知机制（memory.pressure_level）
# - Kubernetes v1.25+ 默认使用 cgroup v2

# 检查 cgroup v2 支持
cat /proc/cgroups
ls /sys/fs/cgroup/cgroup.controllers
```

---

## 三、资源预留配置

### 3.1 资源分配模型

```
节点资源分配:
  ┌─────────────────────────────────────────────────────────────┐
  │  Node Capacity (总资源)                                     │
  │  ├── System Reserved (系统守护进程)                         │
  │  │   └── sshd, systemd, containerd, journald 等            │
  │  ├── Kube Reserved (Kubernetes 组件)                        │
  │  │   └── kubelet, kube-proxy, etc.                         │
  │  ├── Eviction Threshold (驱逐阈值)                          │
  │  │   └── 预留的缓冲区                                       │
  │  └── Allocatable (可分配给 Pod 的资源)                      │
  │      Allocatable = Capacity - System - Kube - Eviction     │
  │      └── Pod 使用的资源                                      │
  └─────────────────────────────────────────────────────────────┘
```

### 3.2 资源预留配置

```bash
# kubelet 启动参数 (systemd drop-in)
# /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
[Service]
Environment="KUBELET_KUBECONFIG_ARGS=--bootstrap-kubeconfig=/etc/kubernetes/bootstrap-kubelet.conf --kubeconfig=/etc/kubernetes/kubelet.conf"

# 资源预留参数:
# --system-reserved=cpu=500m,memory=1Gi,ephemeral-storage=2Gi
# --kube-reserved=cpu=500m,memory=1Gi,ephemeral-storage=2Gi
# --eviction-hard=memory.available<500Mi,nodefs.available<10%
# --enforce-node-allocatable=pods,kube-reserved,system-reserved
```

### 3.3 推荐预留值

| 节点规格 | system-reserved | kube-reserved | eviction-hard |
|---------|----------------|---------------|---------------|
| 2 CPU / 4Gi | cpu=200m,memory=200Mi | cpu=200m,memory=200Mi | memory.available<200Mi |
| 4 CPU / 8Gi | cpu=500m,memory=500Mi | cpu=500m,memory=500Mi | memory.available<500Mi |
| 8 CPU / 16Gi | cpu=1000m,memory=1Gi | cpu=1000m,memory=1Gi | memory.available<1Gi |
| 16 CPU / 32Gi | cpu=1500m,memory=2Gi | cpu=1500m,memory=2Gi | memory.available<1Gi |

---

## 四、Pod 数量与 PID 限制

### 4.1 maxPods

```bash
# 默认 110 个 Pod
# 可调整，但需要考虑:
# 1. IP 地址空间 (每个 Pod 需要一个 IP)
# 2. PodCIDR 大小 (/24 = 254 个 IP)
# 3. 性能影响 (kubelet 处理更多 Pod)

# 调整 maxPods
# /var/lib/kubelet/config.yaml
maxPods: 250

# 或启动参数
--max-pods=250

# 查看当前限制
kubectl get node <node> -o jsonpath='{.status.capacity.pods}'

# 注意: maxPods 不能超过 PodCIDR 可用 IP 数
# /24 = 254 IP → maxPods 最大约 250
```

### 4.2 podPidsLimit

```bash
# 每个 Pod 最大 PID 数
# 防止 Pod 内进程 PID 耗尽或 fork bomb 攻击
--pod-pids-limit=4096

# 查看系统 PID 上限
cat /proc/sys/kernel/pid_max
# 默认 32768 或 4194303 (64 位系统)

# 查看节点 PID 使用情况
ps -eLf | wc -l
```

---

## 五、kubelet 日志配置

### 5.1 日志级别

```bash
# kubelet 日志位置
# 方式 1: journald
journalctl -u kubelet --no-pager -n 100

# 方式 2: 文件 (如果配置了)
/var/log/kubelet.log

# 配置日志级别
# /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
Environment="KUBELET_LOG_LEVEL=--v=2"

# 日志级别说明:
# --v=0: 仅显示重要信息 (生产推荐)
# --v=2: 默认级别，显示一般信息
# --v=4: 调试级别，显示详细处理过程
# --v=6: 跟踪级别，显示函数调用
# --v=9: 最详细，显示所有日志 (仅用于排查严重问题)
```

### 5.2 日志过滤

```bash
# 查看错误日志
journalctl -u kubelet -p err --no-pager -n 50

# 实时跟踪日志
journalctl -u kubelet -f --no-pager

# 按时间范围查看
journalctl -u kubelet --since "2024-01-01 00:00:00" --until "2024-01-01 01:00:00"

# 搜索特定关键词
journalctl -u kubelet | grep -i "error\|failed\|panic"
journalctl -u kubelet | grep -i "certificate\|csr"
journalctl -u kubelet | grep "syncPod"
```

---

## 六、kubelet 启动参数 vs 配置文件

### 6.1 参数传递方式

kubelet 支持两种配置方式，配置文件优先级高于命令行参数：

```bash
# 方式 1: 命令行参数 (不推荐)
kubelet --config=/var/lib/kubelet/config.yaml --max-pods=250

# 方式 2: 配置文件 (推荐)
# /var/lib/kubelet/config.yaml
maxPods: 250

# 方式 3: systemd drop-in (kubeadm 使用的方式)
# /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
[Service]
Environment="KUBELET_EXTRA_ARGS=--max-pods=250"
```

### 6.2 查看当前 kubelet 配置

```bash
# 查看运行中的 kubelet 配置
kubectl get --raw /api/v1/nodes/<node>/proxy/configz | jq .

# 查看启动参数
ps aux | grep kubelet
cat /proc/$(pidof kubelet)/cmdline | tr '\0' '\n'

# 查看 systemd 配置
systemctl cat kubelet
cat /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
```

---

## 七、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| cgroup driver 不匹配 | kubelet 和 containerd driver 不同 | `containerd config dump \| grep systemd` | 统一为 `systemd` |
| Pod 数量不足 | maxPods 限制 | `kubectl get node <node> -o jsonpath='{.status.capacity.pods}'` | 增加 `maxPods` |
| 日志过大 | 日志级别太高 | `journalctl --disk-usage` | 调整 `--v` 级别，配置日志轮转 |
| 内存不足 | 资源预留不足 | `kubectl describe node <node> \| grep -A 5 Allocatable` | 增加 `--kube-reserved` |
| kubelet 启动失败 | 配置文件语法错误 | `kubelet --validate --config=/var/lib/kubelet/config.yaml` | 修复配置语法 |
| 容器运行时连接失败 | containerd 未运行 | `systemctl status containerd` | 启动 containerd |
| 证书验证失败 | 证书过期或路径错误 | `openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates` | 续期证书 |

---

## 相关函数

| 函数/组件 | 源码位置 | 说明 |
|----------|---------|------|
| `KubeletConfiguration` | `pkg/kubelet/apis/config/types.go` | 配置 API 定义 |
| `ValidateKubeletConfiguration` | `pkg/kubelet/apis/config/validation/validation.go` | 配置校验 |
| `containerManager` | `pkg/kubelet/cm/` | cgroup 管理 |
| `setupNodeActiveDeadlineHandler` | `pkg/kubelet/kubelet.go` | 节点状态管理 |
| `evictionManager` | `pkg/kubelet/eviction/` | 驱逐管理 |
| `podManager` | `pkg/kubelet/pod/` | Pod 管理 |
