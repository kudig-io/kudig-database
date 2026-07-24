---
title: OverlayFS Storage
description: OverlayFS Storage — Kubernetes 生产运维知识库
summary: OverlayFS Storage — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- storage
- overlayfs
- container
- filesystem
- cow
- containerd
- docker
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OverlayFS Storage 是什么
- 如何 OverlayFS Storage
trigger_keywords:
- OverlayFS
- Storage
prerequisites:
- kubectl-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OverlayFS Storage

## How OverlayFS Works

OverlayFS merges multiple directories into a single unified view:

```
upperdir (writable)  <- Container writes go here
    +
lowerdir (read-only) <- Image layers (multiple stacked)
    =
merged (unified view) <- What the container sees
```

| Component | Description | K8s Equivalent |
|-----------|-------------|----------------|
| lowerdir | Read-only image layers | Container image layers |
| upperdir | Writable layer for container changes | Container runtime writable layer |
| workdir | Intermediate directory for atomic operations | Containerd snapshot workspace |
| merged | Unified mount point presented to container | Container root filesystem |

## Copy-on-Write (COW) Mechanism

- **Read**: Files read from upperdir first, then lowerdir (upper takes precedence)
- **Write**: File copied from lowerdir to upperdir before modification (copy-up)
- **Delete**: Whiteout file created in upperdir to hide lowerdir file
- **Rename**: Atomic operation using workdir as staging area

## Container Image Layering

Docker/OCI images are stacked OverlayFS layers:
```
Layer 1: Base OS (alpine/ubuntu)    <- lowest lowerdir
Layer 2: Runtime (python/node)      <- middle lowerdir
Layer 3: Dependencies (pip/npm)     <- middle lowerdir
Layer 4: Application code            <- top lowerdir
Container: Writable changes          <- upperdir
```

This design enables:
- Layer sharing across images (disk efficiency)
- Fast container startup (only upperdir is writable)
- Image versioning (immutable lowerdir layers)

## Performance Implications

- **Copy-up overhead**: First write to a large file copies entire file from lower to upper
- **Metadata operations**: Directory listings traverse all layers
- **Best practice**: Use emptyDir or volumes for heavy write workloads instead of container filesystem

## Debugging

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# View OverlayFS mounts
mount | grep overlay

# Inspect container storage driver
docker inspect --format='{{.GraphDriver}}' <container-id>
crictl inspect <container-id>  # K8s nodes
```

## 源码实现分析

### containerd Snapshot 机制

```go
// containerd/snapshots/overlay/overlay.go
func (o *snapshotter) Prepare(ctx context.Context, key, parent string) {
    // 1. 创建 upperdir 和 workdir
    upper := filepath.Join(root, "snapshots", key, "fs")
    work := filepath.Join(root, "snapshots", key, "work")
    
    // 2. 构建 mount 选项
    // mount -t overlay overlay \
    //   -o lowerdir=layer1:layer2:layer3,upperdir=upper,workdir=work merged
    options := []string{
        "lowerdir=" + strings.Join(parentLayers, ":"),
        "upperdir=" + upper,
        "workdir=" + work,
    }
    
    // 3. 返回 mount 信息给 kubelet
    return []mount.Mount{{
        Type:    "overlay",
        Source:  "overlay",
        Options: options,
    }}
}
```

### 镜像层存储结构

```
/var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots/
├── 1/           # 基础镜像层 (lowerdir)
│   └── fs/      # 解压的文件系统
├── 2/           # 第二层 (lowerdir)
│   └── fs/
├── 3/           # 容器可写层 (upperdir)
│   ├── fs/      # 容器写入的文件
│   └── work/    # OverlayFS 工作目录
└── ...

# 查看容器实际挂载:
# mount | grep overlay
# overlay on /var/lib/containerd/.../rootfs type overlay
#   (lowerdir=.../1/fs:.../2/fs, upperdir=.../3/fs, workdir=.../3/work)
```

## 使用场景

### 场景一：检查容器存储驱动

```bash
# 🟢 低风险 - 查看 containerd 配置
cat /etc/containerd/config.toml | grep snapshotter
# [plugins."io.containerd.grpc.v1.cri".containerd]
#   snapshotter = "overlayfs"

# 🟢 低风险 - 查看容器挂载信息
crictl inspect <container-id> | jq '.info.runtimeSpec.mounts'

# 🟢 低风险 - 查看磁盘使用
du -sh /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/
```

### 场景二：避免 COW 性能问题

```yaml
# 重写入负载使用 emptyDir 或 PVC，避免容器文件系统
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: app
    image: myapp:1.0
    volumeMounts:
    - name: tmp-data
      mountPath: /tmp/processing   # 重写入路径用 volume
    - name: cache
      mountPath: /var/cache/app
  volumes:
  - name: tmp-data
    emptyDir: {}                   # 节点本地磁盘，无 COW 开销
  - name: cache
    persistentVolumeClaim:
      claimName: app-cache         # 持久化存储
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 容器内写文件很快 | 第一次写入触发 copy-up（复制整个文件到 upper），大文件很慢 |
| 删除容器后数据丢失 | 容器可写层确实丢失，但应用数据应存 Volume（PV/emptyDir） |
| 镜像层越多越好 | 层数多增加 mount 复杂度和元数据开销，应合并层（multi-stage build） |
| OverlayFS 是唯一选择 | 还有 fuse-overlayfs（rootless）、btrfs、zfs、devicemapper(已废弃) |
| 容器内 df 显示的是实际大小 | df 显示的是合并视图，实际占用需看 upperdir 大小 |
| 所有文件操作都有 COW 开销 | 只读操作无开销，仅写入/删除触发 copy-up/whiteout |

## 面试要点

1. **OverlayFS 如何实现容器文件系统？** — 镜像层作为只读 lowerdir（多层叠加），容器可写层作为 upperdir，通过 overlay mount 合并为统一视图。写入触发 copy-up（复制文件到 upper），删除创建 whiteout 文件遮蔽下层。

2. **为什么容器内重写入性能差？** — copy-up 机制：第一次修改文件时需复制整个文件从 lower 到 upper。大文件（如数据库文件）首次写入开销巨大。解决：重写入路径挂载 Volume（emptyDir/PVC）绕过 COW。

3. **镜像层共享如何节省磁盘？** — 多个镜像共享相同的 base 层（如 alpine），磁盘只存储一份。containerd 通过 content-addressable storage（SHA256 digest）去重。拉取镜像时已存在的层直接复用。

4. **生产环境存储驱动选择？** — overlayfs：默认选择，性能好、广泛支持；fuse-overlayfs：rootless 容器场景；btrfs/zfs：需要快照/压缩等高级特性。避免 devicemapper（已废弃，性能差）。

## Related

- [[概念/block-file-object-storage.md|block-file-object-storage]] — Block, File, and Object Storage
- [[docker]] — Docker
- [[实体/container-runtime.md|container-runtime]] — Container Runtime
- [[containerd]] — containerd
- [[概念/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[概念/docker-architecture.md|Docker Architecture]]
- [[概念/linux-container-foundation.md|Linux Container Foundation]]
- [[概念/block-file-object-storage.md|Block, File, and Object Storage]]


<!-- risk-assessed -->
