---
title: 39 - 容器运行时对比表
description: '| **gVisor** | 沙箱 | 安全隔离 | 性能开销 | v1.25+ | 支持 |'
summary: '| **gVisor** | 沙箱 | 安全隔离 | 性能开销 | v1.25+ | 支持 |'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- kubelet
- containerd
- cri-o
- docker
- daemonset
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 容器运行时对比表 是什么
- 如何 容器运行时对比表
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- 容器运行时对比表
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 39 - 容器运行时对比表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].io/docs/setup/production-environment/container-runtimes](https://kubernetes.io/docs/setup/production-environment/container-runtimes/)

<!-- chunk: 容器运行时对比 -->
## 容器运行时对比

| 运行时 | 类型 | 优势 | 劣势 | K8S版本 | ACK支持 |
|-------|------|------|------|--------|---------|
| **[[containerd|containerd]]** | CRI原生 | 轻量，K8S默认 | 功能比Docker少 | v1.24+默认 | 默认 |
| **CRI-O** | CRI原生 | 轻量，OCI标准 | 生态较小 | v1.24+ | 支持 |
| **Docker** | dockershim | 功能丰富，生态成熟 | **已移除** | <v1.24 | 不支持 |
| **gVisor** | 沙箱 | 安全隔离 | 性能开销 | v1.25+ | 支持 |
| **Kata Containers** | 轻量VM | 强隔离 | 资源开销 | v1.25+ | 支持 |
| **Firecracker** | microVM | 极轻量VM | AWS生态 | v1.25+ | - |

<!-- chunk: containerd详解 -->
## containerd详解

| 特性 | 说明 | 配置方式 |
|-----|------|---------|
| **CRI插件** | K8S原生支持 | /etc/containerd/config.toml |
| **镜像管理** | 拉取/存储/分发 | crictl命令 |
| **容器生命周期** | 创建/启动/停止 | 自动管理 |
| **CNI集成** | 网络插件 | /etc/cni/net.d/ |
| **快照** | 文件系统快照 | overlayfs/native |

```toml
# /etc/containerd/config.toml 示例
version = 2

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    sandbox_image = "registry.k8s.io/pause:3.9"
    [plugins."io.containerd.grpc.v1.cri".containerd]
      default_runtime_name = "runc"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true
    [plugins."io.containerd.grpc.v1.cri".registry]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
          endpoint = ["https://mirror.ccs.tencentyun.com"]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."registry.cn-hangzhou.aliyuncs.com"]
          endpoint = ["https://registry.cn-hangzhou.aliyuncs.com"]
```

<!-- chunk: CRI-O详解 -->
## CRI-O详解

| 特性 | 说明 | 配置文件 |
|-----|------|---------|
| **OCI兼容** | 完全OCI标准 | /etc/crio/crio.conf |
| **版本对齐** | 与K8S版本同步 | 版本号一致 |
| **安全** | 最小攻击面 | 配置 |

```toml
# /etc/crio/crio.conf 示例
[crio]
  [crio.runtime]
    default_runtime = "runc"
    [crio.runtime.runtimes.runc]
      runtime_path = "/usr/bin/runc"
      runtime_type = "oci"
  [crio.image]
    pause_image = "registry.k8s.io/pause:3.9"
  [crio.network]
    network_dir = "/etc/cni/net.d/"
    plugin_dirs = ["/opt/cni/bin/"]
```

<!-- chunk: 性能对比 -->
## 性能对比

| 指标 | containerd | CRI-O | Docker(历史) |
|-----|-----------|-------|-------------|
| **启动延迟** | ~300ms | ~350ms | ~500ms |
| **内存开销** | ~50MB | ~40MB | ~100MB |
| **CPU开销** | 低 | 低 | 中 |
| **镜像拉取** | 快 | 快 | 中 |
| **并发容器** | 高 | 高 | 中 |

<!-- chunk: 安全运行时 -->
## 安全运行时

| 运行时 | 隔离级别 | 原理 | 适用场景 | 性能开销 |
|-------|---------|------|---------|---------|
| **runc** | 命名空间 | Linux NS | 默认 | 无 |
| **gVisor(runsc)** | 用户空间内核 | 系统调用拦截 | 不可信代码 | 20-50% |
| **Kata** | 轻量VM | QEMU/Firecracker | 多租户 | 10-30% |

```yaml
# RuntimeClass配置
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
overhead:
  podFixed:
    memory: "120Mi"
    cpu: "250m"
scheduling:
  nodeSelector:
    runtime: gvisor
---
# 使用RuntimeClass的Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  runtimeClassName: gvisor  # 使用gVisor运行时
  containers:
  - name: app
    image: nginx

```

<!-- chunk: 从Docker迁移到containerd -->
## 从Docker迁移到containerd

| 步骤 | 操作 | 命令 |
|-----|------|------|
| 1 | 安装containerd | `apt install containerd` |
| 2 | 配置containerd | 编辑config.toml |
| 3 | 配置kubelet | `--container-runtime-endpoint=unix:///run/containerd/containerd.sock` |
| 4 | 重启kubelet | `systemctl restart kubelet` |
| 5 | 验证 | `crictl info` |

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# Docker到containerd迁移检查
# 1. 停止kubelet
systemctl stop kubelet

# 2. 停止docker
systemctl stop docker

# 3. 配置containerd
mkdir -p /etc/containerd
containerd config default > /etc/containerd/config.toml
# 编辑config.toml设置SystemdCgroup = true

# 4. 启动containerd
systemctl enable --now containerd

# 5. 修改kubelet配置
# /var/lib/kubelet/kubeadm-flags.env
# 添加: --container-runtime-endpoint=unix:///run/containerd/containerd.sock

# 6. 启动kubelet
systemctl start kubelet

# 7. 验证
crictl info
kubectl get nodes
```
<!-- chunk: crictl命令参考 -->
## crictl命令参考

| 命令 | 用途 | 示例 |
|-----|------|------|
| **crictl ps** | 列出容器 | `crictl ps -a` |
| **crictl pods** | 列出Pod | `crictl pods` |
| **crictl images** | 列出镜像 | `crictl images` |
| **crictl pull** | 拉取镜像 | `crictl pull nginx:latest` |
| **crictl logs** | 查看日志 | `crictl logs <container-id>` |
| **crictl exec** | 执行命令 | `crictl exec -it <id> sh` |
| **crictl inspect** | 检查容器 | `crictl inspect <id>` |
| **crictl rmi** | 删除镜像 | `crictl rmi <image>` |
| **crictl rm** | 删除容器 | `crictl rm <id>` |
| **crictl stats** | 资源统计 | `crictl stats` |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# crictl配置
cat > /etc/crictl.yaml <<EOF
runtime-endpoint: unix:///run/containerd/containerd.sock
image-endpoint: unix:///run/containerd/containerd.sock
timeout: 10
debug: false
EOF
```
<!-- chunk: 镜像加速配置 -->
## 镜像加速配置

```toml
# containerd镜像加速
[plugins."io.containerd.grpc.v1.cri".registry]
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
      endpoint = ["https://registry.cn-hangzhou.aliyuncs.com"]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."gcr.io"]
      endpoint = ["https://gcr.mirrors.ustc.edu.cn"]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."k8s.gcr.io"]
      endpoint = ["https://registry.cn-hangzhou.aliyuncs.com/google_containers"]
  [plugins."io.containerd.grpc.v1.cri".registry.configs]
    [plugins."io.containerd.grpc.v1.cri".registry.configs."registry.cn-hangzhou.aliyuncs.com".auth]
      username = "user"
      password = "password"
```

<!-- chunk: 运行时故障排查 -->
## 运行时故障排查

| 问题 | 症状 | 诊断命令 | 解决方案 |
|-----|------|---------|---------|
| **容器无法启动** | ContainerCreating | `crictl logs` | 检查运行时日志 |
| **镜像拉取失败** | ImagePullBackOff | `crictl pull <image>` | 检查仓库配置 |
| **运行时不响应** | 节点NotReady | `systemctl status containerd` | 重启运行时 |
| **存储满** | 创建失败 | `df -h` | 清理未用镜像/容器 |

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 清理未使用镜像
crictl rmi --prune

# 清理停止的容器
crictl rm $(crictl ps -a -q --state exited)

# 检查运行时状态
systemctl status containerd
journalctl -u containerd -f
```
<!-- chunk: ACK容器运行时 -->
## ACK容器运行时

| 运行时 | ACK配置 | 适用场景 |
|-------|--------|---------|
| **containerd** | 默认 | 标准工作负载 |
| **安全沙箱** | 节点池选择 | 安全敏感 |

<!-- chunk: CRI接口工作原理 -->
## CRI接口工作原理

### 架构分层

```
┌─────────────────────────────────────────────────────────────┐
│                      kubelet                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │              CRI Client (gRPC)                        │  │
│  └───────────────────────┬───────────────────────────────┘  │
└──────────────────────────┼──────────────────────────────────┘
                           │ unix:///run/containerd/containerd.sock
┌──────────────────────────┼──────────────────────────────────┐
│                   Container Runtime                          │
│  ┌───────────────────────┴───────────────────────────────┐  │
│  │              CRI Server (gRPC)                        │  │
│  ├───────────────────────────────────────────────────────┤  │
│  │  Runtime Service          │  Image Service            │  │
│  │  - RunPodSandbox          │  - ListImages             │  │
│  │  - CreateContainer        │  - PullImage              │  │
│  │  - StartContainer         │  - RemoveImage            │  │
│  │  - StopContainer          │  - ImageStatus            │  │
│  │  - ExecSync/Exec          │                           │  │
│  └───────────────────────────┴───────────────────────────┘  │
│                          │                                   │
│  ┌───────────────────────┴───────────────────────────────┐  │
│  │              OCI Runtime (runc/runsc/kata)            │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### CRI 关键接口说明

| 接口 | 功能 | 调用时机 | 超时默认 |
|-----|------|---------|----------|
| `RunPodSandbox` | 创建 Pod 网络命名空间 | Pod 调度到节点 | 2min |
| `CreateContainer` | 创建容器（未启动） | Pod 内每个容器 | 1min |
| `StartContainer` | 启动容器 | 容器创建后 | 1min |
| `StopContainer` | 停止容器 | 终止/驱逐 | grace period |
| `ExecSync` | 同步执行命令 | kubectl exec | 可配置 |
| `PullImage` | 拉取镜像 | 镜像不存在时 | 5min |
| `UpdateRuntimeConfig` | 更新运行时配置 | CIDR 变更等 | 30s |

### CRI 版本兼容性

| K8s 版本 | CRI 版本 | 支持的运行时 | 备注 |
|---------|---------|------------|------|
| v1.25 | v1alpha2 | containerd 1.6+, CRI-O 1.25+ | dockershim 已移除 |
| v1.26 | v1 | containerd 1.7+, CRI-O 1.26+ | CRI v1 稳定 |
| v1.27+ | v1 | containerd 1.7+, CRI-O 1.27+ | 用户命名空间支持 |
| v1.29+ | v1 | containerd 2.0+, CRI-O 1.29+ | Wasm 运行时改进 |

<!-- chunk: 运行时安全加固 -->
## 运行时安全加固

### containerd 安全配置

```toml
# /etc/containerd/config.toml 安全加固配置
version = 2

[plugins."io.containerd.grpc.v1.cri"]
  # 禁用特权容器（生产环境推荐）
  disable_privileged_containers = false  # 由 PSA 控制
  
  # 限制容器 capabilities
  [plugins."io.containerd.grpc.v1.cri".containerd]
    # 默认运行时
    default_runtime_name = "runc"
    
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
        # 启用 seccomp
        # 通过 Pod SecurityContext 配置
  
  # 镜像安全
  [plugins."io.containerd.grpc.v1.cri".registry]
    # 仅允许 HTTPS 仓库
    [plugins."io.containerd.grpc.v1.cri".registry.configs]
      # 配置仓库认证
  
  # 限制容器可访问的设备
  [plugins."io.containerd.grpc.v1.cri".cni]
    bin_dir = "/opt/cni/bin"
    conf_dir = "/etc/cni/net.d"
```

### 运行时安全检查清单

| 序号 | 检查项 | 验证命令 | 通过标准 |
|-----|--------|---------|----------|
| 1 | containerd 版本最新 | `containerd --version` | ≥ 1.7.x |
| 2 | SystemdCgroup 已启用 | `grep SystemdCgroup /etc/containerd/config.toml` | true |
| 3 | 无特权容器运行 | `kubectl get pods -A -o json \| jq '.items[].spec.containers[].securityContext.privileged'` | 无 true |
| 4 | seccomp 已启用 | 检查 Pod securityContext | RuntimeDefault |
| 5 | 镜像来源可信 | 检查 imagePullSecrets + 仓库配置 | 仅内部仓库 |
| 6 | 运行时日志已采集 | `journalctl -u containerd --since '1h ago'` | 日志正常输出 |
| 7 | 无未授权镜像 | `crictl images` | 无未知镜像 |
| 8 | socket 权限正确 | `ls -la /run/containerd/containerd.sock` | root:root 0660 |

### 运行时安全加固脚本

```bash
#!/bin/bash
# 🟡 中风险：运行时安全加固检查脚本
set -euo pipefail

echo "=== 容器运行时安全检查 ==="

# 1. 检查 containerd 版本
VERSION=$(containerd --version | awk '{print $3}')
echo "[1] containerd 版本: $VERSION"

# 2. 检查 SystemdCgroup
if grep -q "SystemdCgroup = true" /etc/containerd/config.toml; then
  echo "[2] ✓ SystemdCgroup 已启用"
else
  echo "[2] ✗ SystemdCgroup 未启用，建议启用"
fi

# 3. 检查特权容器
echo "[3] 特权容器检查:"
kubectl get pods -A -o json | jq -r '
  .items[] |
  select(.spec.containers[].securityContext.privileged == true) |
  "  ✗ \(.metadata.namespace)/\(.metadata.name)"' 2>/dev/null || echo "  ✓ 无特权容器"

# 4. 检查未使用镜像
echo "[4] 未使用镜像:"
UNUSED=$(crictl images -q | wc -l)
USED=$(crictl ps -a -o json | jq -r '.containers[].imageRef' | sort -u | wc -l)
echo "  总镜像数: $UNUSED, 使用中: $USED"

# 5. 检查运行时日志
echo "[5] 运行时日志检查:"
ERRORS=$(journalctl -u containerd --since '1 hour ago' --no-pager | grep -c -i error || true)
echo "  最近 1 小时错误数: $ERRORS"

# 6. 检查 socket 权限
SOCKET_PERMS=$(stat -c '%a %U:%G' /run/containerd/containerd.sock 2>/dev/null || echo "unknown")
echo "[6] Socket 权限: $SOCKET_PERMS"

echo "=== 检查完成 ==="
```

<!-- chunk: 运行时性能调优 -->
## 运行时性能调优

### containerd 性能参数

| 参数 | 默认值 | 推荐值 | 说明 |
|-----|-------|-------|------|
| `max_concurrent_downloads` | 3 | 5-10 | 并发镜像拉取数 |
| `max_container_log_line_size` | 16384 | 32768 | 容器日志行大小限制 |
| `stats_collect_period` | 10s | 5s | 资源统计采集周期 |
| `systemd_cgroup` | false | true | 使用 systemd cgroup 驱动 |
| `snapshotter` | overlayfs | overlayfs | 文件系统快照驱动 |

### 性能调优配置

```toml
# /etc/containerd/config.toml 性能调优
version = 2

[plugins."io.containerd.grpc.v1.cri"]
  # 并发镜像拉取数
  max_concurrent_downloads = 10
  
  # 容器日志配置
  max_container_log_line_size = 32768
  
  [plugins."io.containerd.grpc.v1.cri".containerd]
    # 快照驱动（overlayfs 性能最佳）
    snapshotter = "overlayfs"
    
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
        # 启用 shim cgroup（减少 cgroup 数量）
        ShimCgroup = ""
        # IO 优化
        IoUid = 0
        IoGid = 0

# 镜像存储优化
[plugins."io.containerd.snapshotter.v1.overlayfs"]
  root_path = "/var/lib/containerd/io.containerd.snapshotter.v1.overlayfs"
  # 启用 upperdir 同步（数据安全性）
  sync_remove = false
```

### 镜像拉取优化

```bash
# 🟢 低风险：镜像拉取性能测试
# 测试镜像拉取速度
time crictl pull nginx:latest

# 预拉取常用镜像（DaemonSet 方式）
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: image-puller
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: image-puller
  template:
    metadata:
      labels:
        app: image-puller
    spec:
      initContainers:
        - name: pull-nginx
          image: nginx:1.25
          command: ["echo", "pulled"]
        - name: pull-busybox
          image: busybox:1.36
          command: ["echo", "pulled"]
      containers:
        - name: pause
          image: registry.k8s.io/pause:3.9
EOF
```

### 性能基准测试脚本

```bash
#!/bin/bash
# 🟢 低风险：运行时性能基准测试
set -euo pipefail

echo "=== 容器运行时性能基准 ==="

# 1. 镜像拉取测试
echo "[1] 镜像拉取测试"
START=$(date +%s%N)
crictl pull busybox:latest > /dev/null 2>&1
END=$(date +%s%N)
echo "  拉取时间: $(( (END - START) / 1000000 ))ms"

# 2. 容器启动测试
echo "[2] 容器启动测试"
START=$(date +%s%N)
CID=$(crictl runp <(echo '{"metadata":{"name":"bench","namespace":"default","uid":"bench-001"}}'))
END=$(date +%s%N)
echo "  Pod Sandbox 创建: $(( (END - START) / 1000000 ))ms"

# 3. 容器创建测试
START=$(date +%s%N)
crictl create $CID <(echo '{"metadata":{"name":"test"},"image":{"image":"docker.io/library/busybox:latest"},"command":["sleep","3600"]}') > /dev/null 2>&1
END=$(date +%s%N)
echo "  容器创建: $(( (END - START) / 1000000 ))ms"

# 4. 清理
crictl stopp $CID > /dev/null 2>&1 || true
crictl rmp $CID > /dev/null 2>&1 || true

echo "=== 测试完成 ==="
```

<!-- chunk: 多运行时集群管理 -->
## 多运行时集群管理

### 运行时拓扑规划

| 节点池 | 运行时 | 用途 | 节点标签 | 污点 |
|-------|-------|------|---------|------|
| default | runc | 通用工作负载 | `runtime=runc` | 无 |
| secure | gVisor | 不可信代码 | `runtime=gvisor` | `runtime=gvisor:NoSchedule` |
| high-security | kata | 金融/医疗 | `runtime=kata` | `runtime=kata:NoSchedule` |
| gpu | nvidia | AI/ML 训练 | `nvidia.com/gpu.present=true` | `nvidia.com/gpu=present:NoSchedule` |
| edge | wasmedge | 边缘函数 | `runtime=wasmedge` | `runtime=wasmedge:NoSchedule` |

### 运行时健康检查 DaemonSet

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: runtime-health-checker
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: runtime-health-checker
  template:
    metadata:
      labels:
        app: runtime-health-checker
    spec:
      hostPID: true
      tolerations:
        - operator: Exists  # 运行在所有节点
      containers:
        - name: checker
          image: bitnami/kubectl:latest
          securityContext:
            privileged: true
          command:
            - /bin/sh
            - -c
            - |
              while true; do
                echo "=== Runtime Health Check $(date) ==="
                
                # 检查 containerd 状态
                if systemctl is-active containerd > /dev/null 2>&1; then
                  echo "✓ containerd is running"
                else
                  echo "✗ containerd is NOT running"
                  # 尝试重启
                  systemctl restart containerd
                fi
                
                # 检查运行时响应
                if crictl info > /dev/null 2>&1; then
                  echo "✓ CRI is responsive"
                else
                  echo "✗ CRI is NOT responsive"
                fi
                
                # 检查磁盘空间
                DISK_USAGE=$(df /var/lib/containerd | awk 'NR==2{print $5}' | tr -d '%')
                if [ "$DISK_USAGE" -gt 85 ]; then
                  echo "⚠ Disk usage high: ${DISK_USAGE}%"
                  # 清理未使用镜像
                  crictl rmi --prune
                fi
                
                sleep 60
              done
          volumeMounts:
            - name: containerd-sock
              mountPath: /run/containerd
            - name: containerd-data
              mountPath: /var/lib/containerd
      volumes:
        - name: containerd-sock
          hostPath:
            path: /run/containerd
        - name: containerd-data
          hostPath:
            path: /var/lib/containerd
```

<!-- chunk: 运行时升级与迁移 -->
## 运行时升级与迁移

### containerd 升级流程

| 步骤 | 操作 | 风险 | 回滚方案 |
|-----|------|------|----------|
| 1 | 备份配置 | 🟢 | - |
| 2 | 排空节点 | 🟡 | uncordon |
| 3 | 停止 kubelet | 🔴 | 启动 kubelet |
| 4 | 升级 containerd | 🔴 | 降级包 |
| 5 | 验证配置 | 🟢 | 恢复备份 |
| 6 | 启动服务 | 🟡 | - |
| 7 | 恢复调度 | 🟢 | - |

### 升级脚本

```bash
#!/bin/bash
# 🔴 高风险：containerd 升级脚本，需变更审批
set -euo pipefail

NODE_NAME=$(hostname)
NEW_VERSION=${1:?"Usage: $0 <new-version>"}

echo "=== 升级 containerd 到 $NEW_VERSION (节点: $NODE_NAME) ==="

# 1. 备份配置
echo "[1] 备份配置..."
cp /etc/containerd/config.toml /etc/containerd/config.toml.bak.$(date +%Y%m%d)

# 2. 排空节点（在控制平面执行）
echo "[2] 请确认节点已排空: kubectl drain $NODE_NAME --ignore-daemonsets --delete-emptydir-data"
read -p "按 Enter 继续..."

# 3. 停止服务
echo "[3] 停止服务..."
systemctl stop kubelet
systemctl stop containerd

# 4. 升级
echo "[4] 升级 containerd..."
apt-get update
apt-get install -y containerd.io=$NEW_VERSION

# 5. 验证配置
echo "[5] 验证配置..."
containerd config dump > /dev/null
if [ $? -ne 0 ]; then
  echo "✗ 配置验证失败，回滚..."
  cp /etc/containerd/config.toml.bak.* /etc/containerd/config.toml
  apt-get install -y containerd.io=<old-version>
fi

# 6. 启动服务
echo "[6] 启动服务..."
systemctl start containerd
sleep 5
systemctl start kubelet

# 7. 验证
echo "[7] 验证..."
crictl info > /dev/null && echo "✓ CRI 正常" || echo "✗ CRI 异常"
kubectl get node $NODE_NAME

echo "=== 升级完成，请执行: kubectl uncordon $NODE_NAME ==="
```

### Docker 到 containerd 迁移检查清单

| 序号 | 检查项 | 命令 | 状态 |
|-----|--------|------|------|
| 1 | 确认 K8s 版本 ≥ 1.24 | `kubectl version` | ☐ |
| 2 | 备份 Docker 镜像列表 | `docker images > /tmp/docker-images.txt` | ☐ |
| 3 | 确认无 Docker 特有功能依赖 | 检查 docker.sock 挂载 | ☐ |
| 4 | 安装 containerd | `apt install containerd.io` | ☐ |
| 5 | 配置 SystemdCgroup | 编辑 config.toml | ☐ |
| 6 | 配置 kubelet CRI endpoint | 修改 kubeadm-flags.env | ☐ |
| 7 | 逐节点迁移（先非生产） | 按升级流程执行 | ☐ |
| 8 | 验证所有 Pod 正常 | `kubectl get pods -A` | ☐ |
| 9 | 清理 Docker | `apt remove docker-ce` | ☐ |

<!-- chunk: 运行时监控体系 -->
## 运行时监控体系

### 关键监控指标

| 指标 | 含义 | 告警阈值 | 采集方式 |
|-----|------|---------|----------|
| `container_runtime_operations_total` | 运行时操作计数 | - | kubelet /metrics |
| `container_runtime_operations_errors_total` | 操作错误数 | >5/min | kubelet /metrics |
| `container_runtime_operations_duration_seconds` | 操作延迟 | P99>5s | kubelet /metrics |
| `containerd_container_count` | 容器数量 | >110/节点 | containerd metrics |
| `containerd_image_count` | 镜像数量 | >200/节点 | containerd metrics |
| `process_resident_memory_bytes{job="containerd"}` | containerd 内存 | >1Gi | containerd metrics |

### PrometheusRule 告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: container-runtime-alerts
  namespace: monitoring
spec:
  groups:
    - name: runtime.rules
      rules:
        - alert: ContainerdDown
          expr: up{job="containerd"} == 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "节点 {{ $labels.instance }} containerd 宕机"

        - alert: ContainerdHighMemory
          expr: |
            process_resident_memory_bytes{job="containerd"} > 1e9
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "containerd 内存使用超过 1GB"

        - alert: RuntimeOperationErrors
          expr: |
            rate(container_runtime_operations_errors_total{job="kubelet"}[5m]) > 0.1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} 运行时操作错误率过高"

        - alert: TooManyImages
          expr: |
            containerd_image_count > 200
          for: 30m
          labels:
            severity: info
          annotations:
            summary: "节点 {{ $labels.instance }} 镜像数量过多，建议清理"
```

### Grafana Dashboard 面板

| 面板 | 数据源 | 用途 |
|-----|--------|------|
| Runtime Operations Rate | Prometheus | 各节点运行时操作速率 |
| Runtime Error Ratio | Prometheus | 错误率趋势 |
| Container Count by Node | Prometheus | 各节点容器数量 |
| Image Storage Usage | Node Exporter | 镜像存储使用率 |
| Runtime Latency Heatmap | Prometheus | 操作延迟分布 |

---

**运行时原则**: v1.24+使用containerd，安全场景用沙箱运行时

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 工作负载 MOC
- [[02-工作负载/README.md|Domain-4: Kubernetes工作负载管理]]
- Domain-4 工作负载 — 开源项目索引
- 01 - Kubernetes 工作负载架构概览 (Workload Architecture Overview)
- 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)
- 03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)
- 04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)
- 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)
- 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
- 07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...
- 08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...
- 09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...

## See Also

- 13-container-lifecycle-hooks
- 14-sidecar-containers-patterns
- 16-runtime-class-configuration
- 17-container-images-registry

```

<!-- risk-assessed -->
