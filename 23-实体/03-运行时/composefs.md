---
title: composefs (entities)
description: '## 概述'
summary: 'composefs 是一个 Linux 文件系统，设计用于高效挂载和共享容器镜像层。它结合了 EROFS（只读文件系统）作为元数据存储和 fs-verity 提供内容校验，实现了容器镜像的可验证挂载。composefs 允许多个容器镜像共享相同内容的文件块（基于内容寻址的对象存储），大幅减少磁盘空间占用，同时通过 fs-verity 确保镜像内容的完整性。'
category: entities
tags:
- k8s
- cncf
- runtime
- composefs
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- composefs 是什么
- 如何 composefs
trigger_keywords:
- composefs
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# composefs

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: C

## 概述

ComposeFS 是一个 Linux 文件系统项目，由 Red Hat 和 GNOME 社区推动开发，旨在提供高性能的容器镜像挂载方案。它通过将只读文件系统元数据（来自容器镜像）与底层内容寻址存储（CAS）分离，实现快速镜像挂载和高效存储利用。ComposeFS 使容器镜像可以以只读方式直接挂载，无需解包到 OverlayFS 上层，大幅减少存储空间和启动时间。它与 OCI 镜像格式兼容，特别适合大规模容器部署。

## Key Features（核心能力）

- **快速挂载**：容器镜像无需解包即可直接挂载，显著减少启动时间
- **内容寻址**：基于 fs-verity 的文件完整性校验
- **高效存储**：多个镜像共享相同的底层文件层，避免重复存储
- **OverlayFS 兼容**：可作为 OverlayFS 的 lower layer 使用
- **安全增强**：通过 fs-verity 提供文件级完整性保护
- **与 OCI 兼容**：支持标准 OCI 镜像格式

## 架构与工作原理

ComposeFS 由两部分组成：ComposeFS 元数据文件描述了文件系统的目录树结构（权限、文件名等），但不包含文件内容；文件内容存储在底层的内容寻址存储（CAS）中，通常是一个目录，文件名以内容的 SHA-256 哈希命名。挂载时，内核 ComposeFS 驱动读取元数据文件，引用 CAS 中的文件内容，构建虚拟的只读文件系统视图。底层文件通过 fs-verity 自动验证完整性。

## K8s 集成

ComposeFS 与 containerd 集成，作为镜像快照ter（Snapshotter）。Pod 创建时，containerd 不再需要将镜像层解包到 OverlayFS，而是直接通过 ComposeFS 挂载只读层。这减少了磁盘 I/O 和存储空间使用。在 K8s 节点上，所有 Pod 共享同一份 CAS 存储，相同文件只需存储一次。

## 生产用例

- **大规模容器部署**：数千 Pod 集群的镜像存储优化
- **快速启动**：通过直接挂载减少容器启动时间
- **安全加固**：利用 fs-verity 提供镜像文件完整性保护
- **边缘计算**：在存储受限的边缘节点上高效运行容器

## 安装与配置

```bash
# 🟢 加载 composefs 内核模块（Linux 6.7+）
modprobe composefs

# 🟢 验证模块加载
lsmod | grep composefs
cat /proc/filesystems | grep composefs

# 🟢 手动挂载 ComposeFS
mount.composefs metadata.cfs /mnt/composefs -o basedir=/var/lib/cas

# 🟢 安装 composefs 工具
# Fedora/RHEL
dnf install composefs
# Ubuntu (24.04+)
apt install composefs
# 源码编译
git clone https://github.com/containers/composefs
cd composefs && meson setup build && ninja -C build && ninja -C build install

# 🟢 创建 ComposeFS 镜像
mkcomposefs /path/to/rootfs /var/lib/composefs/images/app.cfs

# 🟢 containerd 集成配置
# /etc/containerd/config.toml:
# [plugins."io.containerd.grpc.v1.cri".containerd]
#   snapshotter = "composefs"
#   disable_snapshot_annotations = false
#   discard_unpacked_layers = false
```

### containerd ComposeFS Snapshotter 配置

```toml
# /etc/containerd/config.toml
version = 2

[plugins."io.containerd.grpc.v1.cri"]
  [plugins."io.containerd.grpc.v1.cri".containerd]
    snapshotter = "overlayfs"
    
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    snapshotter = "composefs"

[plugins."io.containerd.snapshotter.v1.composefs"]
  # CAS 存储目录
  root_path = "/var/lib/containerd/composefs"
  # 启用 fs-verity 校验
  fsverity = true
  # 最大并发挂载数
  max_concurrent_mounts = 128

[plugins."io.containerd.grpc.v1.cri".image]
  # 启用镜像完整性验证
  max_concurrent_downloads = 10
```

### Pod 使用 ComposeFS 挂载

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: composefs-demo
  annotations:
    # 指定使用 composefs snapshotter
    io.kubernetes.cri.containerd/snapshotter: composefs
spec:
  containers:
    - name: app
      image: quay.io/org/app:v1.0
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
  # 节点选择（需要支持 composefs 的节点）
  nodeSelector:
    composefs: enabled
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: composefs-verifier
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: composefs-verifier
  template:
    metadata:
      labels:
        app: composefs-verifier
    spec:
      containers:
        - name: verifier
          image: quay.io/org/composefs-verifier:latest
          securityContext:
            privileged: true
          volumeMounts:
            - name: cas-store
              mountPath: /var/lib/containerd/composefs
      volumes:
        - name: cas-store
          hostPath:
            path: /var/lib/containerd/composefs
```

## 运维操作

```bash
# 🟢 查看 ComposeFS 挂载状态
mount | grep composefs
cat /proc/mounts | grep composefs

# 🟢 查看 CAS 存储使用情况
du -sh /var/lib/containerd/composefs/
find /var/lib/containerd/composefs -type f | wc -l

# 🟢 验证 fs-verity 完整性
fsverity measure /var/lib/containerd/composefs/objects/<hash>

# 🟢 查看 containerd snapshotter 状态
ctr snapshots --snapshotter composefs ls
ctr snapshots --snapshotter composefs info <snapshot-name>

# 🟡 清理未使用的 CAS 对象
ctr snapshots --snapshotter composefs ls | grep -v active

# 🟡 重新加载 composefs 模块
modprobe -r composefs && modprobe composefs

# 🔴 清除所有 ComposeFS 缓存（需要重新拉取镜像）
rm -rf /var/lib/containerd/composefs/*
systemctl restart containerd
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| mount.composefs 失败 | 内核不支持 | `modprobe composefs` | 升级内核到 6.7+ |
| 容器启动慢 | CAS 对象缺失 | `ctr snapshots info <name>` | 重新拉取镜像 |
| fs-verity 校验失败 | 磁盘损坏 | `fsverity measure <file>` | 从备份恢复或重新拉取 |
| 磁盘空间不足 | CAS 未清理 | `du -sh /var/lib/containerd/composefs/` | 清理未引用对象 |

```bash
# 排查流程
# 1. 检查内核模块状态
lsmod | grep composefs
dmesg | grep -i composefs

# 2. 检查 containerd 日志
journalctl -u containerd --since "5 min ago" | grep -i composefs

# 3. 检查 CAS 存储健康
df -h /var/lib/containerd/composefs
fsck /dev/sdX  # 如果怀疑磁盘问题

# 4. 检查挂载点状态
findmnt -t composefs
cat /proc/self/mountinfo | grep composefs
```

## 生产案例

### 案例1：大规模集群镜像存储优化
- **场景**：5000 Pod 集群，节点磁盘空间紧张，镜像层重复存储严重
- **方案**：启用 ComposeFS snapshotter；所有节点共享 CAS 存储；相同文件只存储一次（基于 SHA-256 内容寻址）
- **效果**：节点磁盘使用减少 40%，容器启动时间减少 30%（无需解包镜像层）

### 案例2：安全加固 - 镜像完整性保护
- **场景**：金融企业需要确保容器镜像在运行时未被篡改
- **方案**：启用 fs-verity 校验；每个文件内容通过 Merkle Tree 验证；异常修改立即触发 I/O 错误
- **效果**：通过等保三级审计，镜像篡改检测时间从“发现时”缩短到“实时”

## 对比替代方案

| 维度 | ComposeFS | OverlayFS | Stargz/SOCI | dm-verity |
|------|-----------|-----------|-------------|----------|
| 挂载方式 | 零拷贝 | 需解包 | 懒加载 | 块设备 |
| 存储效率 | 极高(CAS) | 低(重复) | 中 | 中 |
| 启动速度 | 极快 | 慢 | 快 | 中 |
| 完整性校验 | fs-verity | 无 | 部分 | dm-verity |
| 内核要求 | 6.7+ | 3.18+ | 无特殊 | 3.4+ |
| 生产成熟度 | 中 | 极高 | 中 | 高 |

## 检查清单

- [ ] 内核版本 >= 6.7 且支持 composefs 模块
- [ ] containerd 版本支持 composefs snapshotter
- [ ] CAS 存储目录已配置且有足够空间
- [ ] fs-verity 已启用（安全场景）
- [ ] 节点已标记 composefs=enabled 标签
- [ ] 已在测试节点验证镜像拉取和容器启动
- [ ] 磁盘空间监控已配置

## Related

- [[containerssh]] — ContainerSSH
- [[modelpack]] — ModelPack
- [[oauth2-proxy]] — OAuth2 Proxy
- [[schemahero]] — SchemaHero
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- composefs
- [[23-实体/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference


<!-- risk-assessed -->
