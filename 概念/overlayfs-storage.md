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
