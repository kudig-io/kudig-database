---
title: containerd 2.0 新特性
description: 'description: ''## 1. containerd 2.0 概述'''
category: general
tags:
- cncf
- ecosystem
- kubelet
- prometheus
- containerd
- docker
- daemonset
- job
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- containerd 2.0 新特性 是什么
- 如何 containerd 2.0 新特性
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- containerd
- '2.0'
- 新特性
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

title: containerd 2.0 新特性
description: '## 1. containerd 2.0 概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- containerd
- containerd-2.0
- upgrade
- performance
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 8min
intent_queries:
- containerd 2.0 新特性 是什么
- containerd 2.0 有什么更新
- containerd 升级 2.0 如何升级
trigger_keywords:
- containerd 2.0
- containerd 新特性
- containerd upgrade
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# containerd 2.0 新特性

> **版本**: 2.0 | **发布时间**: 2023-Q4 | **最后更新**: 2026-05

---

## 1. containerd 2.0 概述

### 1.1 为什么是 containerd 2.0

containerd 2.0 是自 2017 年毕业以来的最大架构升级。之前的 1.x 系列是增量更新，而 2.0 代表了容器运行时的范式转变：

| 特性 | containerd 1.x | containerd 2.0 |
|------|----------------|----------------|
| **API 版本** | v1 (gRPC) | v2 (保留 v1 兼容) |
| **传输协议** | gRPC | ttrpc + gRPC 双协议 |
| **插件系统** | 静态编译 | 动态插件加载 |
| **快照管理** | 单一 snapshotter | 多 snapshotter 插拔 |
| **网络模型** | CNI only | CNI + Host networking 优化 |
| **安全特性** | 基本 seccomp | 全面安全 profile |
| **性能** | 基准 | 启动速度 +40%，内存 -30% |

### 1.2 架构变化

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         containerd 2.0 架构                                      │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Clients                                        │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────┐   │    │
│  │  │   kubelet   │ │    ctr      │ │  nerdctl    │ │   Docker Engine  │   │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────┘   │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                    │                                             │
│                         ┌──────────┴──────────┐                                 │
│                         ▼                     ▼                                 │
│               ┌─────────────────┐   ┌─────────────────┐                        │
│               │   v2 API        │   │   v1 API (兼容)  │                        │
│               │   (ttrpc)       │   │   (gRPC)        │                        │
│               └────────┬────────┘   └────────┬────────┘                        │
│                        │                     │                                  │
│                        └──────────┬──────────┘                                  │
│                                   ▼                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    Dynamic Plugin System                                │    │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐   │    │
│  │  │   CRI   │ │   CNI   │ │Snapshotter│ │ Runtime │ │   Streaming   │   │    │
│  │  │ Plugin  │ │ Plugin  │ │ (动态)   │ │ (动态)  │ │   (动态)      │   │    │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────────────┘   │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 核心新特性详解

### 2.1 v2 API (Runtime v2 API 正式版)

#### 2.1.1 ttrpc 传输协议

containerd 2.0 默认启用 ttrpc，相比 gRPC 有更低开销：

```toml
# /etc/containerd/config.toml
version = 2

[grpc]
  address = "/run/containerd/containerd.sock"
  # ttrpc 仅适用于同主机通信，kubelet 在同一节点
  ttrpc_enabled = true
  max_recv_message_size = 16777216
  max_send_message_size = 16777216
```

| 指标 | gRPC (v1) | ttrpc (v2) | 提升 |
|------|-----------|------------|------|
| **延迟** | 1.2ms | 0.7ms | -42% |
| **吞吐量** | 800 req/s | 1200 req/s | +50% |
| **内存占用** | 45MB | 28MB | -38% |

#### 2.1.2 v2 API 核心变化

```protobuf
// v2 API 核心服务
service Runtime {
    // 统一的任务管理
    rpc CreateTask(CreateTaskRequest) returns (CreateTaskResponse);
    rpc StartTask(StartTaskRequest) returns (StartTaskResponse);
    rpc WaitTask(WaitTaskRequest) returns (WaitTaskResponse);
    rpc DeleteTask(DeleteTaskRequest) returns (DeleteTaskResponse);
    
    // 批量操作支持
    rpc CreateTasksBatch(CreateTasksBatchRequest) returns (CreateTasksBatchResponse);
    rpc DeleteTasksBatch(DeleteTasksBatchRequest) returns (DeleteTasksBatchResponse);
}
```

#### 2.1.3 向后兼容性

```bash
# v1 API 仍然可用（通过适配层）
crictl --runtime-endpoint unix:///run/containerd/containerd.sock version
# Client: crictl v1.27+
# Server: containerd 2.0 (API v2, gRPC compatible)
```

---

### 2.2 动态插件系统

#### 2.2.1 插件架构变化

```
containerd 1.x                          containerd 2.0
┌─────────────────────┐                ┌─────────────────────┐
│   Static Plugins     │                │   Dynamic Plugins    │
│   (编译时绑定)       │                │   (运行时加载)       │
│                     │                │                     │
│  - CRI plugin       │                │  - CRI (动态)       │
│  - CNI plugin       │                │  - CNI (动态)       │
│  - Snapshotter      │                │  - Snapshotter (动态)│
│  - Runtime          │                │  - Runtime (动态)   │
│  - Streaming        │                │  - Streaming (动态)  │
└─────────────────────┘                └─────────────────────┘
                                              │
                                              ▼
                                    ┌─────────────────────┐
                                    │   Plugin Registry   │
                                    │   (运行时发现)      │
                                    └─────────────────────┘
```

#### 2.2.2 插件配置

```toml
# /etc/containerd/config.toml
version = 2

# 动态插件配置
[plugins]
  # 插件注册
  [plugins."io.containerd.plugin.v2"]
    disabled_plugins = []  # 禁用特定插件
    forced_plugins = []    # 强制启用特定插件
  
  # CRI 插件（动态）
  [plugins."io.containerd.grpc.v2.cri"]
    sandbox_image = "registry.k8s.io/pause:3.10"
    
  # Snapshotter 插件（动态选择）
  [plugins."io.containerd.snapshotter.v2"]
    default = "overlayfs"
    available = ["overlayfs", "btrfs", "zfs", "devmapper"]
```

#### 2.2.3 运行时插件管理

```bash
# 列出已加载的插件
ctr plugin list

# 输出示例
TYPE        NAME                                        VERSION
snapshot    io.containerd.snapshotter.v1.overlayfs       2.0.0
snapshot    io.containerd.snapshotter.v1.btrfs           2.0.0
runtime     io.containerd.runc.v2                         2.0.0
runtime     io.containerd.kata.v2                        2.0.0
cri         io.containerd.cri.v2                          2.0.0

# 验证插件状态
ctr plugin inspect io.containerd.snapshotter.v1.overlayfs
```

---

### 2.3 增强的 Snapshotter

#### 2.3.1 多 Snapshotter 支持

containerd 2.0 支持运行时切换 snapshotter，无需重启：

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v2.cri".containerd]
  snapshotter = "overlayfs"  # 默认
  
  # 可用 snapshotter:
  # - overlayfs: 默认，最佳性能
  # - stargz: 延迟加载，大镜像优化
  # - nydus: RAFS 格式，P2P 分发
  # - devmapper: 块设备，大规模集群
```

#### 2.3.2 Stargz Snapshotter (延迟加载)

```bash
# 启用 stargz snapshotter
[proxy_plugins]
  [proxy_plugins.stargz]
    type = "snapshot"
    address = "/run/containerd-stargz-grpc.sock"

[plugins."io.containerd.grpc.v2.cri".containerd]
  snapshotter = "stargz"

# 拉取延迟加载镜像
ctr images pull --snapshotter stargz registry.k8s.io/some-app:latest

# 效果：镜像秒级启动，内容按需加载
```

#### 2.3.3 Nydus Snapshotter (RAF 格式)

```bash
# Nydus 配置
[proxy_plugins]
  [proxy_plugins.nydus]
    type = "snapshot"
    address = "/run/containerd-nydus/containerd-nydus-grpc.sock"

[plugins."io.containerd.grpc.v2.cri".containerd]
  snapshotter = "nydus"
  disable_snapshot_annotations = false

# Nydus 镜像转换
nydusify convert --source nginx:latest --target my-registry/nginx:nydus

# 效果：
# - 镜像体积减少 40-60%
# - 启动时间 < 1s (无论镜像大小)
```

---

### 2.4 网络增强

#### 2.4.1 Host Network 优化

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v2.cri"]
  # 网络配置
  [plugins."io.containerd.grpc.v2.cri".cni]
    bin_dir = "/opt/cni/bin"
    conf_dir = "/etc/cni/net.d"
    max_conf_num = 5  # 支持多网络
    
  # Host 网络优化（降低延迟）
  [plugins."io.containerd.grpc.v2.cri".network]
    plugin_dir = "/opt/cni/bin"
    conf_template = ""
```

#### 2.4.2 网络隔离增强

```yaml
# Kubernetes Pod 网络配置
apiVersion: v1
kind: Pod
metadata:
  name: multi-network-pod
  annotations:
    k8s.v1.cni.cncf.io/networks: '[{"name":"backend","interface":"eth1"},{"name":"storage","interface":"eth2"}]'
spec:
  containers:
  - name: app
    image: nginx
  # Pod 具有多个网络接口
```

---

### 2.5 安全增强

#### 2.5.1 Seccomp 增强

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v2.cri"]
  # Seccomp 配置
  enable_seccomp = true
  seccomp_profile = "/etc/containerd/seccomp.json"
  
  # AppArmor 配置
  enable_apparmor = true
  apparmor_profile = "containerd-default"
```

#### 2.5.2 用户命名空间支持

```yaml
# Kubernetes Pod 使用用户命名空间
apiVersion: v1
kind: Pod
metadata:
  name: rootless-pod
spec:
  securityContext:
    runAsUser: 10000
    runAsGroup: 10000
    fsGroup: 10000
  # 容器内 UID 10000 映射到宿主机 UID 100000+
```

#### 2.5.3 cgroup v2 全面支持

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v2.cri".containerd.runtimes.runc]
  runtime_type = "io.containerd.runc.v2"
  [plugins."io.containerd.grpc.v2.cri".containerd.runtimes.runc.options]
    SystemdCgroup = true  # cgroup v2
    
    # cgroup v2 特有选项
    Rootless = false
    CgroupMode = "private"  # 或 "shared"
```

---

### 2.6 性能优化

#### 2.6.1 内存优化

| 指标 | containerd 1.7 | containerd 2.0 | 变化 |
|------|----------------|----------------|------|
| **空闲内存** | 45MB | 28MB | -38% |
| **峰值内存** | 120MB | 85MB | -29% |
| **GC 频率** | 30s | 60s | +100% |

#### 2.6.2 启动速度优化

```bash
# 容器启动延迟对比 (P99)
containerd 1.7: 850ms
containerd 2.0: 520ms  # -39%

# Shim 启动时间
containerd 1.7: 45ms
containerd 2.0: 28ms  # -38%
```

#### 2.6.3 并发拉取优化

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v2.cri"]
  # 并发拉取优化
  max_concurrent_downloads = 15  # 默认 3 → 15
  max_container_log_line_size = 32768
  
  # 镜像预热
  [plugins."io.containerd.grpc.v2.cri".image_decryption]
    key_model = "node"  # 或 "cluster"
```

---

## 3. 升级指南

### 3.1 从 1.x 升级到 2.0

#### 3.1.1 升级前检查

```bash
# 1. 确认当前版本
containerd --version
# containerd github.com/containerd/containerd 1.7.x

# 2. 检查依赖
crictl version
# Client: 1.27.0
# Server: 1.7.x

# 3. 备份配置
cp /etc/containerd/config.toml /etc/containerd/config.toml.backup

# 4. 检查集群状态
kubectl get nodes
# 确保所有节点 Ready
```

#### 3.1.2 升级步骤

```bash
# 1. 封锁节点（生产环境）
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 2. 停止 containerd
systemctl stop containerd

# 3. 备份数据
tar -czf /backup/containerd-data-$(date +%Y%m%d).tar.gz /var/lib/containerd/

# 4. 安装新版本
# Ubuntu/Debian
apt-get install containerd.io=2.0.*

# 或者手动安装
wget https://github.com/containerd/containerd/releases/download/v2.0.0/containerd-2.0.0-linux-amd64.tar.gz
tar xvf containerd-2.0.0-linux-amd64.tar.gz -C /usr/local

# 5. 更新配置（如果需要）
containerd config migrate > /etc/containerd/config.toml

# 6. 启动 containerd
systemctl start containerd

# 7. 验证
crictl info | grep -i version
# RuntimeVersion: 2.0.0

# 8. 解锁节点
kubectl uncordon <node-name>
```

#### 3.1.3 配置迁移

```bash
# 自动迁移配置
containerd config migrate > /etc/containerd/config.toml.new

# 手动检查关键配置
grep -E "(version|SystemdCgroup|sandbox_image)" /etc/containerd/config.toml
```

### 3.2 兼容性矩阵

| 组件 | 最低版本 | 推荐版本 |
|------|----------|----------|
| **Kubernetes** | 1.24 | 1.27+ |
| **crictl** | 1.27 | 1.28+ |
| **nerdctl** | 1.7 | 2.0 |
| **Docker** | 24.0 | 25.0+ |
| **runc** | 1.1 | 1.2+ |
| **CNI plugins** | 1.0 | 1.2+ |

---

## 4. 回滚策略

### 4.1 回滚触发条件

| 条件 | 说明 |
|------|------|
| **服务不可用** | containerd 无法启动 |
| **容器创建失败** | 新容器无法创建 |
| **镜像拉取失败** | 所有镜像拉取失败 |
| **性能下降** | 延迟增加 > 50% |

### 4.2 回滚步骤

```bash
# 1. 停止 containerd
systemctl stop containerd

# 2. 恢复配置
cp /etc/containerd/config.toml.backup /etc/containerd/config.toml

# 3. 恢复数据
rm -rf /var/lib/containerd/*
tar -xzf /backup/containerd-data-$(date +%Y%m%d).tar.gz -C /

# 4. 降级
apt-get install containerd.io=1.7.*

# 或手动降级
wget https://github.com/containerd/containerd/releases/download/v1.7.8/containerd-1.7.8-linux-amd64.tar.gz
tar xvf containerd-1.7.8-linux-amd64.tar.gz -C /usr/local

# 5. 启动
systemctl start containerd

# 6. 验证
crictl info
kubectl get pods -A
```

---

## 5. 生产环境配置模板

### 5.1 高性能配置

```toml
# /etc/containerd/config.toml (containerd 2.0 高性能版)
version = 2

[grpc]
  address = "/run/containerd/containerd.sock"
  ttrpc_enabled = true
  max_recv_message_size = 16777216
  max_send_message_size = 16777216

[plugins]
  [plugins."io.containerd.grpc.v2.cri"]
    sandbox_image = "registry.k8s.io/pause:3.10"
    max_concurrent_downloads = 15
    max_container_log_line_size = 32768
    disable_tcp_service = true
    
    [plugins."io.containerd.grpc.v2.cri".containerd]
      snapshotter = "overlayfs"
      default_runtime_name = "runc"
      
      [plugins."io.containerd.grpc.v2.cri".containerd.runtimes.runc]
        runtime_type = "io.containerd.runc.v2"
        [plugins."io.containerd.grpc.v2.cri".containerd.runtimes.runc.options]
          SystemdCgroup = true
          BinaryName = "/usr/bin/runc"
    
    [plugins."io.containerd.grpc.v2.cri".cni]
      bin_dir = "/opt/cni/bin"
      conf_dir = "/etc/cni/net.d"
      max_conf_num = 5

[metrics]
  address = "127.0.0.1:1338"
  grpc_histogram = true

[timeouts]
  "io.containerd.timeout.shim.cleanup" = "5s"
  "io.containerd.timeout.shim.shutdown" = "3s"
```

### 5.2 安全加固配置

```toml
# /etc/containerd/config.toml (containerd 2.0 安全加固版)
version = 2

[grpc]
  address = "/run/containerd/containerd.sock"
  ttrpc_enabled = true

[plugins]
  [plugins."io.containerd.grpc.v2.cri"]
    sandbox_image = "registry.k8s.io/pause:3.10"
    enable_selinux = true
    enable_apparmor = false  # 根据环境调整
    enable_unprivileged_ports = false
    enable_unprivileged_icmp = false
    
    [plugins."io.containerd.grpc.v2.cri".containerd]
      snapshotter = "overlayfs"
      
      [plugins."io.containerd.grpc.v2.cri".containerd.runtimes.runc]
        runtime_type = "io.containerd.runc.v2"
        privileged_without_host_devices = true
        [plugins."io.containerd.grpc.v2.cri".containerd.runtimes.runc.options]
          SystemdCgroup = true
          NoNewKeyring = true
    
    [plugins."io.containerd.grpc.v2.cri".registry]
      config_path = "/etc/containerd/certs.d"
      [plugins."io.containerd.grpc.v2.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v2.cri".registry.mirrors."docker.io"]
          endpoint = ["https://my-registry.io"]

[timeouts]
  "io.containerd.timeout.bolt.open" = "5s"
```

---

## 6. 监控指标

### 6.1 containerd 2.0 新增指标

| 指标名称 | 类型 | 说明 |
|----------|------|------|
| `containerd_plugin_operations_total` | Counter | 插件操作计数 |
| `containerd_ttrpc_connection_active` | Gauge | ttrpc 活跃连接 |
| `containerd_snapshotter_duration_seconds` | Histogram | 快照操作延迟 |
| `containerd_v2_api_requests_total` | Counter | v2 API 请求统计 |

### 6.2 Prometheus 采集配置

```yaml
# Prometheus scrape config
- job_name: containerd
  static_configs:
  - targets: ['localhost:1338']
  metrics_path: /v1/metrics
  scrape_interval: 15s
  
  relabel_configs:
  - source_labels: [__address__]
    target_label: instance
    regex: '(.*):1338'
    replacement: '${1}'
```

---

## 7. 故障排查

### 7.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| ttrpc 连接失败 | 网络配置错误 | 检查 socket 权限 |
| 插件加载失败 | 版本不匹配 | 降级到 1.7 或升级插件 |
| 镜像拉取慢 | 并发数过低 | 增加 max_concurrent_downloads |

### 7.2 诊断命令

```bash
# 检查 containerd 版本
containerd --version

# 检查 v2 API 状态
crictl info | grep -i api

# 检查插件状态
ctr plugin list

# 查看日志
journalctl -u containerd --since "10m" | grep -i error

# 检查 ttrpc 连接
ss -lx | grep containerd
```

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[hot.md|hot]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking.md|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]

## See Also

- [[domain-19-landscape-references/graduated/containerd/08-containerd-multi-tenant.md|08-containerd-multi-tenant]]
- [[domain-19-landscape-references/graduated/containerd/containerd.md|containerd]]
- [[domain-19-landscape-references/graduated/containerd/03-containerd-security-hardening.md|03-containerd-security-hardening]]
- [[domain-19-landscape-references/graduated/containerd/04-containerd-upgrade-migration.md|04-containerd-upgrade-migration]]
