---
title: Rootless 容器指南
description: 'Rootless 容器配置：Rootless containerd、User Namespace 映射、Podman rootless、cgroup v2 与常见问题排查'
summary: 'Rootless 容器配置：Rootless containerd、User Namespace 映射、Podman rootless、cgroup v2 与常见问题排查'
category: container-runtime
tags:
- rootless
- containerd
- podman
- user-namespace
- cgroup-v2
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
- Rootless 容器 是什么
- 如何配置 Rootless containerd
- Podman rootless 怎么用
- User Namespace 映射怎么配置
trigger_keywords:
- rootless
- containerd
- podman
- user-namespace
- cgroup-v2
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Rootless 容器指南

## 1. 概述

Rootless 容器是指不以 root 权限运行容器运行时和容器进程的方案。即使容器运行时被攻破，攻击者也无法获得宿主机的 root 权限，大幅提升安全性。

### 1.1 Root vs Rootless 对比

```
传统 Root 模式:
┌────────────────────────┐
│     宿主机 (root)      │
│  ┌──────────────────┐  │
│  │ containerd (root)│  │
│  │  ┌────────────┐  │  │
│  │  │ 容器 (root) │  │  │ ← 攻击者获得 root
│  │  └────────────┘  │  │
│  └──────────────────┘  │
└────────────────────────┘

Rootless 模式:
┌────────────────────────┐
│     宿主机 (root)      │
│  ┌──────────────────┐  │
│  │ 用户空间          │  │
│  │ containerd (user)│  │
│  │  ┌────────────┐  │  │
│  │  │ 容器 (user) │  │  │ ← 攻击者仅获得普通用户
│  │  └────────────┘  │  │
│  └──────────────────┘  │
└────────────────────────┘
```

### 1.2 关键技术

| 技术 | 作用 |
|------|------|
| **User Namespace** | UID/GID 映射，容器内 root 映射为普通用户 |
| **cgroup v2** | 非 root 用户的资源限制 |
| **slirp4netns** | 用户空间网络栈 |
| **fuse-overlayfs** | 用户空间 OverlayFS |
| **newuidmap/newgidmap** | UID/GID 映射工具 |

## 2. Rootless containerd 配置

### 2.1 前置条件

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
# 1. 检查内核版本（≥ 5.11 推荐，完整 cgroup v2 支持）
uname -r

# 2. 启用 cgroup v2
# 检查当前 cgroup 版本
stat -fc %T /sys/fs/cgroup/
# cgroup2fs 表示 v2，tmpfs 表示 v1

# 如果是 cgroup v1，需要切换
# 编辑 /etc/default/grub
# GRUB_CMDLINE_LINUX="systemd.unified_cgroup_hierarchy=1"
# sudo update-grub && sudo reboot

# 3. 安装依赖
sudo apt-get install -y uidmap dbus-user-session slirp4netns fuse-overlayfs

# 4. 配置 UID/GID 范围
sudo sh -c 'echo "allen:100000:65536" >> /etc/subuid'
sudo sh -c 'echo "allen:100000:65536" >> /etc/subgid'

# 5. 启用 user lingering（允许非 root 用户的服务在无登录时运行）
sudo loginctl enable-linger allen
```
### 2.2 安装 Rootless containerd

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 containerd（如果尚未安装）
# 使用官方脚本
curl -fsSL https://get.docker.com | sh

# 停止系统级 containerd
sudo systemctl disable --now containerd

# 安装 rootless containerd
containerd-rootless-setuptool.sh install

# 验证安装
systemctl --user status containerd
containerd --version

# 配置 PATH
export PATH=$HOME/bin:$PATH
echo 'export PATH=$HOME/bin:$PATH' >> ~/.bashrc
```
### 2.3 Rootless containerd 配置文件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 配置文件位置
mkdir -p ~/.config/containerd
cat > ~/.config/containerd/config.toml << 'EOF'
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"

[plugins."io.containerd.grpc.v1.cri"]
  # 使用非 root 用户的 socket
  stream_server_address = "127.0.0.1"
  stream_server_port = "0"

[plugins."io.containerd.grpc.v1.cri".cni]
  # CNI 配置
  bin_dir = "/opt/cni/bin"
  conf_dir = "/etc/cni/net.d"
EOF

# 重启 rootless containerd
systemctl --user restart containerd
```
### 2.4 CNI 网络配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Rootless 模式下的网络方案

# 方案 1：slirp4netns（默认，兼容性好）
# 自动配置，无需额外设置

# 方案 2：pasta（性能更好，内核 ≥ 5.11）
# 在 containerd 配置中指定
cat >> ~/.config/containerd/config.toml << 'EOF'
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
  BinaryName = "runc"
EOF

# 方案 3：vpnkit（Docker Desktop 使用）
# 适用于 macOS 和 Windows
```
## 3. User Namespace 映射

### 3.1 UID/GID 映射原理

```
宿主机 UID 范围: 100000-165535
容器内 UID 范围: 0-65535

映射关系:
  容器 UID 0 → 宿主机 UID 100000
  容器 UID 1 → 宿主机 UID 100001
  ...
  容器 UID 65535 → 宿主机 UID 165535
```

### 3.2 配置映射

```bash
# 查看当前映射
cat /etc/subuid
# allen:100000:65536

cat /etc/subgid
# allen:100000:65536

# 手动设置映射
sudo usermod --add-subuids 100000-165535 --add-subgids 100000-165535 allen

# 验证映射
grep allen /etc/subuid /etc/subgid
```

### 3.3 newuidmap/newgidmap

```bash
# 确保 newuidmap 有正确的权限
sudo chmod u+s /usr/bin/newuidmap
sudo chmod u+s /usr/bin/newgidmap

# 验证
ls -la /usr/bin/newuidmap
# -rwsr-xr-x 1 root root ... /usr/bin/newuidmap

# 测试映射
unshare --user --map-auto --map-root-user id
# uid=0(root) gid=0(root) groups=0(root)
```

## 4. Podman Rootless

### 4.1 安装配置

```bash
# 安装 Podman
sudo apt-get install -y podman

# 验证 rootless 模式
podman info | grep rootless
# rootless: true

# 配置 rootless 存储
mkdir -p ~/.config/containers
cat > ~/.config/containers/storage.conf << 'EOF'
[storage]
driver = "overlay"

[storage.options.overlay]
mount_program = "/usr/bin/fuse-overlayfs"
EOF
```

### 4.2 Podman Rootless 使用

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 运行容器
podman run -d --name web nginx:latest

# 查看容器状态
podman ps

# 查看容器日志
podman logs web

# 进入容器
podman exec -it web bash

# 查看存储
podman info --storage

# 配置注册表
cat > ~/.config/containers/registries.conf << 'EOF'
[registries.search]
registries = ['docker.io', 'quay.io', 'ghcr.io']

[registries.insecure]
registries = []

[registries.block]
registries = []
EOF
```
### 4.3 Podman 与 systemd 集成

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 生成 systemd 服务文件
podman generate systemd --new --name web > ~/.config/systemd/user/web.service

# 启用服务
systemctl --user daemon-reload
systemctl --user enable --now web.service

# 设置开机自启（需要 lingering）
loginctl enable-linger $USER

# 查看服务状态
systemctl --user status web.service
```
## 5. cgroup v2 配置

### 5.1 验证 cgroup v2

```bash
# 检查 cgroup 版本
stat -fc %T /sys/fs/cgroup/
# cgroup2fs 表示 v2

# 检查 cgroup v2 挂载
mount | grep cgroup
# cgroup2 on /sys/fs/cgroup type cgroup2 (rw,nosuid,nodev,noexec,relatime)

# 检查 rootless cgroup 支持
cat /sys/fs/cgroup/user.slice/user-$(id -u).slice/cgroup.controllers
# cpu io memory pids
```

### 5.2 配置资源限制

```bash
# 在 rootless 模式下设置资源限制
# 1. 确保 systemd 管理 cgroup
cat > ~/.config/systemd/user/containerd.service.d/override.conf << 'EOF'
[Service]
# 启用资源限制
Delegate=yes
EOF

# 2. 使用 podman 设置资源限制
podman run -d --name app \
  --cpus=1.5 \
  --memory=512m \
  --memory-swap=1g \
  --pids-limit=100 \
  nginx:latest

# 3. 验证资源限制
podman stats app
```

### 5.3 cgroup v2 资源控制

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 cgroup 控制器
cat /sys/fs/cgroup/user.slice/user-$(id -u).slice/cgroup.controllers

# 查看当前资源使用
cat /sys/fs/cgroup/user.slice/user-$(id -u).slice/cgroup.stat

# 设置 v2 资源限制（通过 systemd）
systemctl --user set-property app.service CPUQuota=150%
systemctl --user set-property app.service MemoryMax=512M
```
## 6. 安全加固

### 6.1 Rootless 安全优势

```yaml
# Rootless Pod 安全配置
apiVersion: v1
kind: Pod
metadata:
  name: rootless-app
spec:
  hostUsers: false  # 使用 User Namespace
  containers:
  - name: app
    image: my-app:latest
    securityContext:
      # 基础安全
      privileged: false
      runAsNonRoot: true
      runAsUser: 1000
      readOnlyRootFilesystem: true
      allowPrivilegeEscalation: false

      # 能力限制
      capabilities:
        drop:
        - ALL

      # Seccomp
      seccompProfile:
        type: RuntimeDefault
```

### 6.2 网络安全

```bash
# Rootless 模式下的网络隔离
# slirp4netns 提供用户空间网络栈

# 配置端口转发
podman run -d --name app \
  -p 8080:80 \
  nginx:latest

# 查看网络配置
podman network ls
podman network inspect podman
```

## 7. 常见问题排查

### 7.1 权限问题

```bash
# 问题 1：newuidmap 权限不足
# 解决：设置 SUID 位
sudo chmod u+s /usr/bin/newuidmap
sudo chmod u+s /usr/bin/newgidmap

# 问题 2：subuid/subgid 范围不足
# 解决：增加映射范围
sudo usermod --add-subuids 100000-200000 --add-subgids 100000-200000 $USER

# 问题 3：cgroup 权限拒绝
# 解决：确保 cgroup v2 和 systemd 管理
sudo sh -c 'echo "kernel.unprivileged_userns_clone=1" >> /etc/sysctl.d/rootless.conf'
sudo sysctl --system
```

### 7.2 网络问题

```bash
# 问题 1：端口绑定失败（< 1024）
# 解决：使用端口转发或设置 sysctl
sudo sysctl net.ipv4.ip_unprivileged_port_start=80

# 问题 2：DNS 解析失败
# 解决：检查 resolv.conf
cat /etc/resolv.conf
# 确保 slirp4netns 配置正确

# 问题 3：网络性能差
# 解决：切换到 pasta（内核 ≥ 5.11）
podman run --network=pasta ...
```

### 7.3 存储问题

```bash
# 问题 1：OverlayFS 不支持
# 解决：使用 fuse-overlayfs
sudo apt-get install -y fuse-overlayfs

# 问题 2：存储空间不足
# 解决：清理未使用的镜像和容器
podman system prune -a

# 问题 3：存储驱动错误
# 解决：检查存储配置
podman info --storage
```

## 8. 生产最佳实践

| 实践 | 建议 |
|------|------|
| 内核版本 | ≥ 5.11（完整 cgroup v2 支持） |
| 网络方案 | pasta > slirp4netns |
| 存储驱动 | overlay + fuse-overlayfs |
| UID 映射 | 预留足够范围（65536+） |
| 资源限制 | 使用 cgroup v2 限制 CPU/内存 |
| 安全基线 | 禁用特权、只读文件系统 |

## Related

- [[14-容器运行时/03-containerd-CRI-O/01-containerd-production-operations|containerd 生产运维]]
- [[14-容器运行时/03-containerd-CRI-O/05-gvisor-sandbox-runtime|gVisor 沙箱运行时]]

## See Also

- [Rootless containerd 文档](https://rootlesscontaine.rs/)
- [Podman rootless 文档](https://github.com/containers/podman/blob/main/docs/tutorials/rootless_tutorial.md)


<!-- risk-assessed -->
