---
title: Snapshotter 策略选型
description: containerd overlayfs、fuse-overlayfs、devmapper、stargz、nydus snapshotter 选型与生产配置对比
summary: containerd overlayfs、fuse-overlayfs、devmapper、stargz、nydus snapshotter 选型与生产配置对比
category: container-runtime
tags:
- containerd
- cri
- runtime
- snapshotter
- overlayfs
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 平台工程师
---

> **生产环境安全提示**
>
> 风险标注：🔴 高风险 / 🟡 中风险 / 🟢 只读。

# Snapshotter 策略选型

## 概述

Snapshotter 负责把镜像层组装成容器可写的根文件系统（rootfs）。不同 snapshotter 在性能、存储后端、懒加载能力、隔离性上差异显著，直接影响镜像拉取速度、节点磁盘占用与 Pod 启动延迟。

## 选型对比矩阵

| Snapshotter | 适用场景 | 性能 | 懒加载 | 依赖 |
|---|---|---|---|---|
| overlayfs | 默认通用（root） | ⭐⭐⭐⭐⭐ | 否 | Linux kernel ≥ 3.18 |
| fuse-overlayfs | rootless 容器 | ⭐⭐⭐ | 否 | FUSE + fuse-overlayfs |
| devmapper | 块设备精简配置 | ⭐⭐⭐⭐ | 否 | LVM thin pool |
| native | 调试/兼容 | ⭐⭐ | 否 | 无 |
| stargz | 大镜像延迟加载 | ⭐⭐⭐ | 是 | eStargz 镜像 |
| nydus | 大规模分发/专有云 | ⭐⭐⭐⭐ | 是 | nydus daemon + Dragonfly |

## overlayfs（默认推荐）

内核原生联合挂载，性能最优，ACK/专有云默认方案。lowerdir 为镜像只读层，upperdir 为容器可写层。

``` bash
# 🟢 只读：确认节点 snapshotter
crictl info | grep -i snapshotter
mount | grep overlay
```

## fuse-overlayfs（rootless 场景）

当容器以非 root 用户运行（rootless）时，无权挂载 overlayfs，使用用户态 FUSE 实现 `fuse-overlayfs`。

```toml
[plugins."io.containerd.grpc.v1.cri".containerd]
  snapshotter = "fuse-overlayfs"
```

``` bash
# 🟢 只读：安装 fuse-overlayfs
which fuse-overlayfs
# 启动 rootless containerd
containerd-rootless-setuptool.sh install
```

性能损失约 10-30%（用户态拷贝），仅用于必须 rootless 的多租户环境。

## devmapper（块设备）

将镜像层存放在 LVM thin pool 上，每容器一个精简配置块设备。适合需要可预测 I/O 或固定后端的场景，但运维复杂（需预留 data/metadata 卷）。

```toml
[plugins."io.containerd.snapshotter.devmapper"]
  root_path = "/var/lib/containerd/devmapper"
  pool_name = "containerd-pool"
  base_image_size = "10GB"
```

> ⚠️ 配置错误会导致 thin pool 耗尽，节点 DiskPressure。生产慎用，需配合 `thin_check` 定期校验。

## stargz / nydus（懒加载）

大镜像（>1GB，如 AI/数据镜像）首屏启动慢。懒加载 snapshotter 把镜像转为可按需读取的格式（eStargz / Nydus RAF），Pod 启动时只拉取首块数据，其余按需从 registry 或本地 Dragonfly P2P 拉取。

```toml
# nydus snapshotter（需先部署 nydus-snapshotter）
[plugins."io.containerd.grpc.v1.cri".containerd]
  snapshotter = "nydus"
```

| 维度 | stargz | nydus |
|---|---|---|
| 镜像转换 | `ctr-remote convert estargz` | `nydusify convert` |
| 分发加速 | 依赖 registry 缓存 | 配合 Dragonfly P2P |
| ACK 支持 | 社区 | 龙蜥/ACK 原生 |
| 推荐规模 | 中等镜像 | 超大镜像 + 大集群 |

## 切换 snapshotter

> ⚠️ **🟠 高危操作** — 切换后旧 snapshotter 数据需清理，影响节点所有容器

``` bash
# 🔴 高风险：需 drain 节点、变更窗口
sudo systemctl stop containerd
# 修改 config.toml 的 snapshotter 字段
sudo systemctl start containerd
# 清理旧 rootfs（按需）
sudo rm -rf /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs
```

## 生产选型建议

- **通用业务**：overlayfs，零依赖、性能最佳
- **多租户 rootless**：fuse-overlayfs
- **AI/大镜像 + 大集群**：nydus + Dragonfly，ACK 推荐组合
- **特殊 I/O 隔离**：devmapper（需专职存储团队）

## 生产检查清单

- [ ] 节点 `crictl info` 显示预期 snapshotter
- [ ] 大镜像场景已评估懒加载 snapshotter 的延迟收益
- [ ] devmapper 已规划 thin pool 容量与告警
- [ ] rootless 节点已安装 fuse-overlayfs 并验证挂载

## 相关文档

- [[容器运行时/containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]
- [[容器运行时/containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维]]
- [[容器运行时/镜像管理/01-harbor-enterprise-image-registry.md|Harbor 企业镜像仓库]]

<!-- risk-assessed -->
