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

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| 镜像拉取极慢 | overlayfs 层数过多 | `ctr images ls` 查看层数 | 优化 Dockerfile 减少层数 |
| devmapper thin pool 满 | 容量规划不足 | `dmsetup status` | 扩展 thin pool 或清理无用镜像 |
| fuse-overlayfs 挂载失败 | 内核模块未加载 | `modprobe fuse && lsmod | grep fuse` | 安装 fuse3 并加载内核模块 |
| 容器启动延迟高 | 懒加载 snapshotter 未预热 | `crictl inspect <id>` 查看启动时间 | 配置镜像预拉取策略 |
| 磁盘空间不足 | 镜像层未清理 | `du -sh /var/lib/containerd/` | 执行 `crictl rmi --prune` |
| stargz 拉取失败 | registry 不支持 range 请求 | `curl -I -r 0-1 <registry-url>` | 确认 registry 支持 HTTP Range |
| native snapshotter 性能差 | 无 CoW 支持 | `crictl info | jq .config.containerd.snapshotter` | 切换到 overlayfs |
| rootless 容器挂载失败 | 用户命名空间配置错误 | `cat /etc/subuid` | 配置 subuid/subgid 映射 |

## Snapshotter 对比矩阵

| Snapshotter | 性能 | 空间效率 | 适用场景 | 内核要求 |
|-------------|------|----------|----------|----------|
| overlayfs | 高 | 高 | 通用生产环境 | 3.18+ |
| native | 低 | 低 | 测试/开发 | 无特殊要求 |
| devmapper | 高 | 高 | 块存储需求 | 需要 thin pool |
| fuse-overlayfs | 中 | 高 | rootless 容器 | 需要 fuse |
| stargz | 中 | 高 | 懒加载/大镜像 | 无特殊要求 |
| nydus | 中 | 高 | 懒加载/按需加载 | 需要 nydusd |
| zfs | 高 | 高 | ZFS 存储环境 | 需要 zfs 模块 |
| btrfs | 中 | 中 | btrfs 文件系统 | 需要 btrfs |

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 默认选择 | 生产环境使用 overlayfs | 性能和兼容性最佳 |
| 容量规划 | 预留 30% 磁盘空间 | 避免镜像层写入失败 |
| 清理策略 | 定期清理无用镜像和层 | `crictl rmi --prune` |
| 监控 | 监控磁盘使用率和 I/O | 超过 80% 告警 |
| 懒加载 | 大镜像场景评估 stargz/nydus | 显著降低启动延迟 |
| devmapper | 独立 thin pool，不复用系统盘 | 避免影响系统稳定性 |
| rootless | 使用 fuse-overlayfs | 无 root 权限时的最佳选择 |
| 升级 | 切换 snapshotter 需重新拉取镜像 | 不可在线切换 |

## 相关工具

| 工具 | 用途 | 使用方式 |
|------|------|----------|
| ctr | 查看 snapshotter 状态 | `ctr snapshots ls` |
| dmsetup | devmapper 管理 | `dmsetup status` |
| fuse-overlayfs | rootless 挂载 | 随 fuse-overlayfs 包安装 |
| stargz-snapshotter | 懒加载支持 | 单独安装并配置 |
| nydus-snapshotter | 按需加载 | 随 nydus 安装 |
| crictl | 验证 snapshotter 配置 | `crictl info` |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| 如何查看当前 snapshotter？ | `crictl info | jq .config.containerd.snapshotter` |
| overlayfs 和 overlay 的区别？ | overlay 是旧版（3.18 前），overlayfs 是新版，推荐 overlayfs |
| 能否在线切换 snapshotter？ | 不能，需停止容器、切换配置、重新拉取镜像 |
| stargz 和 nydus 如何选择？ | stargz 兼容 OCI，nydus 性能更优但需专用工具链 |
| devmapper thin pool 如何创建？ | `ctr devmapper create-pool` 或手动 lvcreate |
| 如何监控 snapshotter 性能？ | containerd metrics + 磁盘 I/O 监控 |
| rootless 为什么不能用 overlayfs？ | 需要内核 5.11+ 或 unprivileged_userns_clone=1 |
| 镜像层数上限是多少？ | overlayfs 默认 128 层，超过需合并 |

## Snapshotter 配置示例

```toml
# /etc/containerd/config.toml - overlayfs 配置
version = 2
[plugins."io.containerd.grpc.v1.cri".containerd]
  snapshotter = "overlayfs"
  disable_snapshot_annotations = false

# devmapper 配置示例
[plugins."io.containerd.snapshotter.v1.devmapper"]
  pool_name = "containerd-pool"
  root_path = "/var/lib/containerd/devmapper"
  base_image_size = "10GB"
  discard_blocks = true

# stargz 懒加载配置
[plugins."io.containerd.grpc.v1.cri".containerd]
  snapshotter = "stargz"
[proxy_plugins.stargz]
  type = "snapshot"
  address = "/run/containerd-stargz-grpc/containerd-stargz-grpc.sock"
```

## 性能调优指南

| 场景 | 优化方向 | 具体操作 |
|------|----------|----------|
| 镜像拉取慢 | 减少层数 | 合并 Dockerfile RUN 指令 |
| 容器启动慢 | 懒加载 | 配置 stargz/nydus snapshotter |
| 磁盘 I/O 高 | 独立数据盘 | 将 /var/lib/containerd 挂载到 SSD |
| 空间不足 | 及时清理 | 配置 kubelet image GC 阈值 |
| 大镜像场景 | 按需加载 | nydus/stargz 只拉取需要的层 |
| 高并发拉取 | 并行度调优 | 调整 containerd max_concurrent_downloads |

## 版本兼容性

| containerd 版本 | 支持的 snapshotter | 说明 |
|----------------|-------------------|------|
| 1.6.x | overlayfs/native/devmapper/zfs/btrfs | 稳定版 |
| 1.7.x | + stargz/nydus (proxy plugin) | 懒加载支持 |
| 2.0.x | 全部 + 新插件接口 | 新架构 |

## 监控指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| containerd_snapshot_count | 快照总数 | > 1000 |
| disk_usage_percent | 磁盘使用率 | > 80% |
| snapshot_create_duration | 快照创建耗时 | P99 > 5s |
| image_pull_duration | 镜像拉取耗时 | P99 > 60s |
| gc_reclaimed_bytes | GC 回收空间 | 持续为 0 需检查 |

## 安全加固

| 维度 | 建议 | 说明 |
|------|------|------|
| 数据目录 | 独立挂载，权限 700 | 避免信息泄露 |
| 镜像验证 | 启用签名验证 | cosign + admission |
| 清理 | 定期 GC | 避免磁盘占满 |
| 监控 | 磁盘使用率告警 | > 80% 告警 |
| 备份 | 关键镜像多副本 | 避免单点故障 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| native | overlayfs | 停容器→改配置→重启→重拉镜像 |
| overlayfs | stargz | 安装 stargz-snapshotter→配置 proxy plugin |
| overlayfs | devmapper | 创建 thin pool→配置→重启 |
| overlayfs | nydus | 安装 nydusd→配置→重启 |

## 检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 当前 snapshotter | `crictl info` | 显示预期值 |
| 磁盘使用 | `du -sh /var/lib/containerd/` | < 80% 容量 |
| 镜像层数 | `ctr images ls` | < 128 层 |
| GC 状态 | `journalctl -u containerd` | 无错误 |
| 性能 | `crictl pull` 计时 | < 60s |

## 版本历史

| 版本 | 时间 | 关键变化 |
|------|------|----------|
| containerd 1.0 | 2017 | overlayfs 默认 |
| containerd 1.4 | 2020 | devmapper 稳定 |
| containerd 1.6 | 2022 | stargz 插件支持 |
| containerd 1.7 | 2023 | nydus 插件支持 |
| containerd 2.0 | 2024 | 新插件接口 |

## 架构对比

```text
Snapshotter 架构层次：

overlayfs:
  upperdir (writable) → merged → container rootfs
  lowerdir (read-only layers)

devmapper:
  thin pool → thin volume → container rootfs

stargz/nydus (lazy loading):
  registry → on-demand fetch → FUSE mount → container rootfs
```

## 容量规划

| 场景 | 建议容量 | 说明 |
|------|----------|------|
| 小集群 (<50 节点) | 100GB/节点 | 基础容量 |
| 中集群 (50-200) | 200GB/节点 | 含镜像缓存 |
| 大集群 (>200) | 500GB/节点 | 含多版本镜像 |
| 大镜像场景 | 1TB/节点 | AI/ML 工作负载 |

## 故障排查（补充）

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| overlayfs 挂载失败 | 内核不支持 | `dmesg | grep overlay` | 升级内核或加载模块 |
| devmapper 初始化失败 | thin pool 不存在 | `dmsetup ls` | 创建 thin pool |
| stargz 拉取失败 | registry 不支持 range | `curl -I -r 0-1 <url>` | 确认 registry 配置 |
| nydus 挂载失败 | nydusd 未启动 | `systemctl status nydusd` | 启动 nydusd |
| 磁盘 I/O 高 | 层数过多 | `iostat -x 1` | 优化镜像层数 |
| 空间不足 | GC 未触发 | `du -sh /var/lib/containerd/` | 手动 GC 或调阈值 |
| 权限拒绝 | 目录权限错误 | `ls -la /var/lib/containerd/` | 修正权限 |
| 性能下降 | 磁盘碎片 | `filefrag /var/lib/containerd/` | 整理或更换磁盘 |

## 常见问题 FAQ（补充）

| 问题 | 解答 |
|------|------|
| overlayfs 与 devmapper 如何选择？ | 通用场景用 overlayfs，需要块级隔离用 devmapper |
| stargz 懒加载如何启用？ | 配置 stargz snapshotter 插件，镜像需转换为 estargz 格式 |
| nydus 与 stargz 区别？ | nydus 支持去重和 P2P，stargz 更轻量且兼容 OCI |
| 如何查看当前 snapshotter？ | `containerd config dump | grep snapshotter` |
| 切换 snapshotter 需要重启吗？ | 是，且已有容器需重建 |
| overlayfs 层数限制？ | 内核默认 128 层，建议镜像 < 50 层 |
| devmapper thin pool 如何创建？ | `lvcreate` 创建 thin pool 或使用 loopback 文件 |
| 如何监控 snapshotter 性能？ | containerd metrics + node_exporter diskstats |

## 性能调优参数

| 参数 | 默认值 | 生产建议 | 说明 |
|------|--------|----------|------|
| `root_path` | /var/lib/containerd | SSD 路径 | 数据目录 |
| `sync_remove` | false | true | 删除时同步刷盘 |
| `upperdir_label` | false | true | 支持 SELinux 标签 |
| `max_open_files` | 1024 | 65535 | 高并发场景调大 |
| `discard_blocks` | false | true (SSD) | SSD 启用 TRIM |
| `io_priority` | 无 | best-effort | I/O 调度优先级 |

## 相关文档

- [[容器运行时/containerd-CRI-O/07-containerd-configuration-deep-guide.md|containerd 配置深度指南]]
- [[容器运行时/containerd-CRI-O/01-containerd-production-operations.md|containerd 生产运维]]
- [[容器运行时/镜像管理/01-harbor-enterprise-image-registry.md|Harbor 企业镜像仓库]]

<!-- risk-assessed -->
