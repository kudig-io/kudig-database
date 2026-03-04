# composefs

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/containers/composefs |
| **许可证** | GPL-3.0 / Apache-2.0 |
| **开发语言** | C |
| **CNCF 状态** | Sandbox |

---

## 项目概述

composefs 是一个 Linux 文件系统，设计用于高效挂载和共享容器镜像层。它结合了 EROFS（只读文件系统）作为元数据存储和 fs-verity 提供内容校验，实现了容器镜像的可验证挂载。composefs 允许多个容器镜像共享相同内容的文件块（基于内容寻址的对象存储），大幅减少磁盘空间占用，同时通过 fs-verity 确保镜像内容的完整性。

### 核心特性

- **内容寻址共享**: 多个镜像层中的相同文件只存储一份，基于内容哈希去重
- **fs-verity 验证**: 内核级文件完整性验证，防止篡改
- **EROFS 元数据**: 使用 EROFS 存储文件系统元数据，极高的挂载性能
- **只读挂载**: 文件系统只读，保证容器镜像的不可变性
- **零开销**: 挂载后文件访问无额外性能开销
- **与 ostree/podman 集成**: 作为 ostree 和 podman 的存储后端

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│              composefs 存储架构                    │
│                                                    │
│  ┌──────────────────────────────────────────┐     │
│  │    EROFS Image (元数据层)                 │     │
│  │  文件名、权限、大小、目录结构             │     │
│  │  每个文件指向 → 内容寻址对象              │     │
│  └──────────────────┬───────────────────────┘     │
│                     │                              │
│  ┌──────────────────▼───────────────────────┐     │
│  │  Content-Addressed Object Store           │     │
│  │  /objects/ab/cdef1234...  (文件内容)       │     │
│  │  /objects/12/3456abcd...  (文件内容)       │     │
│  │                                            │     │
│  │  ┌────────────┐ ┌────────────┐            │     │
│  │  │ fs-verity  │ │ fs-verity  │ ...        │     │
│  │  │ 完整性校验 │ │ 完整性校验 │            │     │
│  │  └────────────┘ └────────────┘            │     │
│  └──────────────────────────────────────────┘     │
│                                                    │
│  镜像 A ──┐                                       │
│  镜像 B ──┼─► 共享相同内容的对象文件               │
│  镜像 C ──┘                                       │
└────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# Fedora/CentOS
dnf install composefs

# 从源码构建
git clone https://github.com/containers/composefs.git
cd composefs
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
```

### 创建和挂载

```bash
# 创建 composefs 镜像
mkcomposefs --from-file filelist.txt image.cfs

# 准备对象存储
mkcomposefs --from-file filelist.txt \
  --digest-store=/var/lib/composefs/objects \
  image.cfs

# 挂载 composefs
mount -t composefs image.cfs /mnt \
  -o basedir=/var/lib/composefs/objects,verity_check=2
```

### 与 podman 集成

```bash
# 配置 podman 使用 composefs 存储
# /etc/containers/storage.conf
[storage.options.overlay]
use_composefs = "true"

# 拉取镜像时自动使用 composefs 存储
podman pull nginx:latest
# 相同文件内容在多个镜像间自动共享
```

---

## 与其他方案对比

| 特性 | composefs | overlayfs | EROFS 直接 | squashfs |
|:---|:---|:---|:---|:---|
| 内容去重 | 跨镜像去重 | 不支持 | 不支持 | 不支持 |
| 完整性验证 | fs-verity | 无 | 无 | dm-verity |
| 挂载性能 | 极快 | 快 | 极快 | 需解压 |
| 读取开销 | 无 | loopback 轻微 | 无 | 解压开销 |
| 空间效率 | 极高 (去重) | 低 | 中 | 高 (压缩) |

---

## 最佳实践

1. **启用 fs-verity**: 生产环境启用 fs-verity 确保镜像文件完整性
2. **对象存储规划**: 合理规划对象存储目录的容量和文件系统
3. **与 podman 配合**: 在容器主机上启用 composefs 显著减少磁盘占用
4. **内核版本**: 确保内核支持 EROFS 和 fs-verity (5.4+)

---

## 参考资源

- [composefs GitHub](https://github.com/containers/composefs)
- [composefs 设计文档](https://github.com/containers/composefs/blob/main/README.md)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
