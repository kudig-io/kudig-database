---
title: containerd 到 CRI-O 迁移
description: 'containerd 与 CRI-O 功能差异对比、迁移评估清单、节点逐个切换流程、镜像兼容性与回滚方案'
summary: 'containerd 与 CRI-O 功能差异对比、迁移评估清单、节点逐个切换流程、镜像兼容性与回滚方案'
category: container-runtime
tags:
- containerd
- cri-o
- migration
- runtime-migration
- cri
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- containerd 到 CRI-O 迁移 是什么
- 如何从 containerd 迁移到 CRI-O
- CRI-O 和 containerd 区别是什么
trigger_keywords:
- containerd
- cri-o
- migration
- runtime
- cri
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# containerd 到 CRI-O 迁移

## 1. containerd vs CRI-O 对比

### 1.1 架构差异

```
containerd 架构:
kubelet → CRI plugin → containerd → runc
         (内置 CRI)     (完整容器运行时)

CRI-O 架构:
kubelet → CRI-O → runc (或 kata/gvisor)
         (专用 CRI)   (OCI 运行时)

关键区别:
- containerd: 通用容器运行时，支持 CRI 和非 CRI 场景
- CRI-O: 专为 Kubernetes 设计的轻量级 CRI 实现
```

### 1.2 功能特性对比

| 特性 | containerd | CRI-O |
|------|-----------|-------|
| 设计目标 | 通用容器运行时 | Kubernetes 专用 |
| CRI 支持 | 内置 CRI 插件 | 原生 CRI |
| OCI 兼容 | 完全兼容 | 完全兼容 |
| 镜像管理 | 完整镜像管理 | 精简镜像管理 |
| 快照驱动 | overlayfs/btrfs/devmapper | overlayfs/btrfs |
| 内存占用 | ~100MB | ~50MB |
| 启动速度 | 快 | 更快 |
| 社区支持 | CNCF 毕业项目 | Red Hat 主导 |
| 企业发行版 | Docker/containerd | OpenShift 默认 |
| 插件系统 | 丰富 | 有限 |
| 非 K8s 使用 | 支持 | 不推荐 |

### 1.3 配置文件对比

```toml
# containerd config.toml
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"

[plugins."io.containerd.grpc.v1.cri".registry]
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
    endpoint = ["https://registry-1.docker.io"]
```

```toml
# CRI-O /etc/crio/crio.conf
[crio]
  storage_driver = "overlay"
  log_level = "info"

[crio.runtime]
  default_runtime = "runc"
  [crio.runtime.runtimes.runc]
    runtime_path = "/usr/bin/runc"
    runtime_type = "oci"
    runtime_root = "/run/runc"

[crio.image]
  pause_image = "registry.k8s.io/pause:3.9"
  [crio.image.registry.mirrors."docker.io"]
    endpoint = ["https://registry-1.docker.io"]
```

## 2. 迁移评估清单

### 2.1 迁移前评估

```bash
# 1. 检查当前 containerd 版本和配置
containerd --version
cat /etc/containerd/config.toml

# 2. 检查使用的运行时
crictl info | jq '.config.containerd.runtimes'
# 如果使用 kata/gvisor，需要确保 CRI-O 也支持

# 3. 检查镜像注册表配置
crictl info | jq '.config.registry'
# 记录所有镜像和注册表配置

# 4. 检查 CNI 插件
ls /opt/cni/bin/
cat /etc/cni/net.d/*.conf

# 5. 检查存储后端
crictl info | jq '.config.containerd.snapshotter'
# 确认 CRI-O 支持相同的快照驱动

# 6. 检查非标准功能
# - 使用 containerd API 的工具
# - 使用 ctr 命令的脚本
# - 直接调用 containerd gRPC 的组件
```

### 2.2 兼容性检查

```bash
# 镜像兼容性检查
# CRI-O 和 containerd 都支持 OCI 镜像，但有细微差异

# 1. 检查是否有 Docker 特有的镜像格式
# Docker manifest list vs OCI index
docker manifest inspect <image> 2>/dev/null || echo "需要 Docker CLI"

# 2. 检查是否有使用 Docker 特有功能的镜像
# - Docker schema 1（已弃用）
# - 非 OCI 标准的层格式

# 3. 检查 Init Container 和 Ephemeral Container
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.initContainers != null) | .metadata.name'

# 4. 检查 RuntimeClass 使用
kubectl get runtimeclass
```

### 2.3 迁移风险评估

| 风险项 | 影响 | 缓解措施 |
|--------|------|---------|
| 镜像不兼容 | Pod 无法启动 | 迁移前验证镜像格式 |
| 运行时差异 | 行为不一致 | 测试环境全面测试 |
| CNI 问题 | 网络不通 | 使用相同的 CNI 配置 |
| 存储问题 | 数据丢失 | 备份持久化数据 |
| 配置差异 | 功能缺失 | 详细对比配置文件 |

## 3. CRI-O 安装配置

### 3.1 安装 CRI-O

```bash
# 添加 CRI-O 仓库
OS=xUbuntu_22.04
CRIO_VERSION=1.29

echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/ /" | sudo tee /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable:/cri-o:/$CRIO_VERSION/$OS/ /" | sudo tee /etc/apt/sources.list.d/devel:kubic:libcontainers:stable:cri-o:$CRIO_VERSION.list

curl -L https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/$OS/Release.key | sudo apt-key add -
curl -L https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable:/cri-o:/$CRIO_VERSION/$OS/Release.key | sudo apt-key add -

sudo apt-get update
sudo apt-get install -y cri-o cri-o-runc

# 启动 CRI-O
sudo systemctl daemon-reload
sudo systemctl enable crio
sudo systemctl start crio

# 验证安装
crio --version
sudo systemctl status crio
```

### 3.2 CRI-O 配置

```toml
# /etc/crio/crio.conf
[crio]
  # 存储配置
  storage_driver = "overlay"
  storage_option = ["overlay.mountopt=nodev,metacopy=on"]

  # 日志配置
  log_level = "info"
  log_to_journald = true

[crio.runtime]
  # 默认运行时
  default_runtime = "runc"

  # 运行时配置
  [crio.runtime.runtimes.runc]
    runtime_path = "/usr/bin/runc"
    runtime_type = "oci"
    runtime_root = "/run/runc"
    runtime_config_path = ""

  # Kata Containers（可选）
  [crio.runtime.runtimes.kata]
    runtime_path = "/usr/bin/kata-runtime"
    runtime_type = "oci"
    runtime_root = "/run/vc"
    privileged_without_host_devices = true

  # cgroup 配置
  conmon_cgroup = "pod"
  default_sysctls = []

[crio.image]
  # Pause 镜像
  pause_image = "registry.k8s.io/pause:3.9"
  pause_command = "/pause"

  # 注册表配置
  [crio.image.registry.mirrors."docker.io"]
    endpoint = ["https://registry-1.docker.io"]

  # 不安全注册表
  [crio.image.registry.mirrors."registry.internal:5000"]
    endpoint = ["http://registry.internal:5000"]
  [crio.image.registry.configs."registry.internal:5000".tls]
    insecure_skip_verify = true

[crio.network]
  # CNI 配置
  network_dir = "/etc/cni/net.d/"
  plugin_dirs = ["/opt/cni/bin/"]
```

## 4. 节点逐个切换流程

### 4.1 迁移步骤

```bash
# ===== CRI-O 迁移流程 =====

# Step 1: 标记节点不可调度
kubectl cordon node-1

# Step 2: 驱逐工作负载
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --force --grace-period=30

# Step 3: 停止 containerd
sudo systemctl stop containerd
sudo systemctl disable containerd

# Step 4: 安装 CRI-O（如果尚未安装）
sudo apt-get install -y cri-o cri-o-runc

# Step 5: 配置 CRI-O
# 复制 containerd 的注册表配置到 CRI-O
# 编辑 /etc/crio/crio.conf

# Step 6: 启动 CRI-O
sudo systemctl daemon-reload
sudo systemctl enable crio
sudo systemctl start crio

# Step 7: 配置 kubelet 使用 CRI-O
sudo tee /etc/default/kubelet << 'EOF'
KUBELET_EXTRA_ARGS="--container-runtime-endpoint=unix:///var/run/crio/crio.sock"
EOF

# Step 8: 启动 kubelet
sudo systemctl daemon-reload
sudo systemctl start kubelet

# Step 9: 验证
sleep 10
kubectl get node node-1 -o wide
# 确认 CONTAINER-RUNTIME 显示 cri-o://...

# Step 10: 恢复调度
kubectl uncordon node-1

# Step 11: 验证 Pod
kubectl get pods --field-selector spec.nodeName=node-1 -A
```

### 4.2 crictl 配置切换

```bash
# CRI-O 的 crictl 配置
sudo tee /etc/crictl.yaml << 'EOF'
runtime-endpoint: unix:///var/run/crio/crio.sock
image-endpoint: unix:///var/run/crio/crio.sock
timeout: 10
debug: false
EOF

# 验证 crictl 连接
crictl ps
crictl images
```

### 4.3 镜像迁移

```bash
# CRI-O 和 containerd 使用不同的镜像存储
# 迁移后需要重新拉取镜像

# 方式 1：从注册表重新拉取（推荐）
# Kubernetes 会自动拉取所需镜像

# 方式 2：导出/导入镜像（离线环境）
# 在 containerd 节点导出
ctr images export images.tar <image>:<tag>

# 在 CRI-O 节点导入
crictl pull <image>:<tag>
# 或使用 skopeo
skopeo copy docker://<image>:<tag> docker-archive:images.tar
```

## 5. 回滚方案

### 5.1 回滚步骤

```bash
# 回滚到 containerd

# Step 1: 标记节点不可调度
kubectl cordon node-1
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --force

# Step 2: 停止 CRI-O
sudo systemctl stop crio
sudo systemctl disable crio

# Step 3: 启动 containerd
sudo systemctl enable containerd
sudo systemctl start containerd

# Step 4: 恢复 kubelet 配置
sudo tee /etc/default/kubelet << 'EOF'
KUBELET_EXTRA_ARGS="--container-runtime-endpoint=unix:///run/containerd/containerd.sock"
EOF

# Step 5: 重启 kubelet
sudo systemctl daemon-reload
sudo systemctl restart kubelet

# Step 6: 验证
kubectl get node node-1 -o wide
kubectl uncordon node-1
```

### 5.2 数据恢复

```bash
# CRI-O 和 containerd 的容器数据不兼容
# 回滚后，之前的容器需要重新创建

# 检查 Pod 状态
kubectl get pods -A --field-selector spec.nodeName=node-1

# 对于 StatefulSet，数据在 PVC 中，不受运行时切换影响
kubectl get pvc -A

# 验证持久化数据完整
kubectl exec <pod-name> -- ls /data
```

## 6. 验证清单

### 6.1 迁移后验证

```bash
# 1. 节点状态
kubectl get node node-1 -o wide
# 期望: STATUS=Ready, CONTAINER-RUNTIME=cri-o://...

# 2. CRI-O 状态
sudo systemctl status crio
crio --version

# 3. 容器运行状态
crictl ps | head -10

# 4. 日志检查
journalctl -u crio --since "1 hour ago" | grep -i error
journalctl -u kubelet --since "1 hour ago" | grep -i error

# 5. 系统 Pod
kubectl get pods -n kube-system --field-selector spec.nodeName=node-1

# 6. 网络测试
kubectl run nettest --image=busybox --restart=Never -- ping -c 3 8.8.8.8
kubectl logs nettest
kubectl delete pod nettest

# 7. DNS 测试
kubectl run dnstest --image=busybox --restart=Never -- nslookup kubernetes.default
kubectl logs dnstest
kubectl delete pod dnstest
```

### 6.2 性能对比

```bash
# 迁移前后性能对比

# 容器启动时间
time crictl run <container-config> <pod-config>

# 资源使用
crictl stats

# 网络性能
kubectl run netperf --image=networkstatic/iperf3 --restart=Never -- -s
# 从另一个 Pod 测试
kubectl run netperf-client --image=networkstatic/iperf3 --restart=Never -- -c <netperf-ip> -t 30
```

## 7. 常见问题排查

### 7.1 连接问题

```bash
# 问题：kubelet 无法连接 CRI-O
# 检查 socket
ls -la /var/run/crio/crio.sock
# 检查 CRI-O 日志
journalctl -u crio -f
# 检查 SELinux
sudo ausearch -m avc -ts recent
```

### 7.2 镜像问题

```bash
# 问题：镜像拉取失败
# 检查注册表配置
cat /etc/crio/crio.conf | grep -A 10 "registry"
# 手动拉取测试
crictl pull <image>:<tag>
# 检查认证
ls /etc/containers/auth.json
```

### 7.3 网络问题

```bash
# 问题：Pod 网络不通
# 检查 CNI 配置
ls /etc/cni/net.d/
cat /etc/cni/net.d/*.conf
# 检查 CNI 插件
ls /opt/cni/bin/
# 检查网络命名空间
ip netns list
```

## 8. 生产最佳实践

| 实践 | 建议 |
|------|------|
| 迁移理由 | OpenShift 生态、Red Hat 支持、更轻量 |
| 测试周期 | 至少 2 周全面测试 |
| 节点批次 | 每次 1 个节点，验证后继续 |
| 监控 | 迁移后监控 48 小时 |
| 回滚准备 | 保留 containerd 安装和配置 |
| 文档 | 记录配置差异和迁移步骤 |

## Related

- [[domain-13-container-runtime/03-containerd-cri-o/01-containerd-production-operations|containerd 生产运维]]
- [[domain-13-container-runtime/03-containerd-cri-o/02-cri-o-production-guide|CRI-O 生产指南]]
- [[domain-13-container-runtime/05-runtime-migration/01-docker-to-containerd-migration|Docker 到 containerd 迁移]]

## See Also

- [CRI-O 文档](https://cri-o.io/)
- [containerd 文档](https://containerd.io/docs/)
