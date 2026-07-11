---
title: etcd 与 containerd 存储架构
summary: 'etcd 与 containerd 存储架构：Kubernetes 集群有两个关键的存储层:'
category: synthesis
tags:
- synthesis
- etcd
- containerd
- storage
tier: supporting
sources: []
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
---



# etcd 与 containerd 存储架构

> etcd 作为 Kubernetes 的数据存储后端与 containerd 作为容器运行时的存储机制对比与集成。

## 存储层次

Kubernetes 集群有两个关键的存储层，分别服务于不同目的：

| 组件 | 存储内容 | 存储引擎 | 数据类型 | 位置 |
|------|---------|---------|---------|------|
| etcd | 集群状态、配置、Secrets | bbolt (B+tree) | 键值对 | 控制平面节点 |
| containerd | 镜像层、容器快照 | content store | OCI 格式 | 所有工作节点 |

## etcd 存储深度解析

### 内部架构

etcd 使用 **bbolt**（B+tree）作为底层存储引擎，所有 Kubernetes 资源对象以**修订版本（revision）**方式存储。每个 key-value 对应一个 MVCC 多版本记录。

```
/var/lib/etcd/
├── member/
│   ├── snap/          # 快照文件（定期压缩后保存）
│   │   └── 0000000000000001-000000000000abcd.snap
│   ├── wal/           # Write-Ahead Log（预写日志）
│   │   ├── 0000000000000000-0000000000000000.wal
│   │   └── 014d2c9c...
│   └── db             # bbolt 数据库文件
└── ...
```

### 关键运维操作

```bash
# 🟢 低风险：只读/信息收集
# 检查 etcd 集群健康状态
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/etcd/pki/ca.crt \
  --cert=/etc/etcd/pki/server.crt \
  --key=/etc/etcd/pki/server.key

# 查看 etcd 数据库大小
ETCDCTL_API=3 etcdctl endpoint status -w table

# 🟡 中风险：会修改状态
# 压缩历史修订版本（生产环境定期执行）
ETCDCTL_API=3 etcdctl compact $(etcdctl endpoint status -w json | jq .[0].Status.header.revision)

# 清理碎片空间
ETCDCTL_API=3 etcdctl defrag
```

### 性能关键参数

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| `--quota-backend-bytes` | 8GiB | 数据库大小上限 |
| `--auto-compaction-retention` | 5 | 保留 5 小时修订历史 |
| 磁盘类型 | NVMe SSD | fsync 延迟 < 10ms |
| `--heartbeat-interval` | 250ms | 心跳间隔 |

## containerd 存储深度解析

### 镜像存储结构

containerd 采用 **content-addressable** 方式存储镜像，每层数据以 SHA256 哈希为键：

```
/var/lib/containerd/
├── io.containerd.content.v1.content/
│   ├── ingest/                 # 正在下载的层
│   └── blobs/sha256/           # 已完成的镜像层
│       ├── a1b2c3d4...         # 镜像 manifest
│       └── e5f6g7h8...         # 镜像层 blob
├── io.containerd.snapshotter.v1.overlayfs/
│   └── snapshots/              # 容器快照（可写层）
└── io.containerd.metadata.v1.bolt/
    └── meta.db                 # 元数据索引
```

### 关键运维操作

```bash
# 🟢 低风险：只读/信息收集
# 查看镜像列表
crictl images

# 查看容器存储使用
crictl stats

# 🟡 中风险：清理操作
# 清理未使用的镜像（节点磁盘空间回收）
crictl rmi --prune

# containerd 原生清理
ctr -n k8s.io content list
ctr -n k8s.io images list
```

## 运维交叉点与故障场景

### 1. etcd 磁盘 I/O 瓶颈

etcd 的 fsync 操作对延迟极度敏感。如果 etcd 数据目录与其他高 I/O 工作负载共享磁盘：

```
症状: API Server 响应变慢，kubectl 操作超时
原因: containerd 镜像拉取的 I/O 阻塞了 etcd 的 fsync
解决: 将 etcd 数据目录放在独立专用磁盘上
配置: --data-dir=/var/lib/etcd (独占 NVMe 盘)
```

### 2. containerd 磁盘耗尽

节点磁盘被镜像层和容器日志占满时，会导致 Pod 创建失败和 kubelet 异常。

```bash
# 🟢 低风险：监控磁盘使用
# 检查节点存储
df -h /var/lib/containerd /var/log/pods

# 配置 containerd 垃圾回收
# /etc/containerd/config.toml
[plugins."io.containerd.gc"]
  scheduler_interval = "10m"
  paused = false
```

### 3. 双重存储治理

| 维度 | etcd 治理 | containerd 治理 |
|------|----------|----------------|
| 备份 | 定期 `etcdctl snapshot save` | 不需要（镜像可重新拉取） |
| 空间回收 | compact + defrag | `crictl rmi --prune` |
| 监控指标 | db_size, fsync_duration | image_count, snapshot_disk_usage |
| 灾难恢复 | 快照恢复到新集群 | 镜像仓库即备份 |

## 最佳实践

- **etcd 数据目录使用独占 NVMe 磁盘**：fsync 延迟直接影响集群写性能，共享磁盘是最常见的性能问题来源
- **配置定期快照备份**：至少每 30 分钟执行一次 `etcdctl snapshot save`，备份文件存储到异地
- **设置 containerd 镜像清理策略**：配置 GC 策略或使用镜像预热 + 定期 `crictl rmi --prune` 回收空间
- **监控 etcd db_size 趋势**：db_size 持续增长通常表示 compact 未执行或存在大量资源泄漏
- **containerd 配置镜像加速器**：配置 registry mirror 减少拉取延迟，避免大量镜像层并发写入导致 I/O 峰值

## 常见陷阱

- **etcd 磁盘满了导致集群不可用**：超过 `--quota-backend-bytes` 后 etcd 变为只读，必须 compact + defrag 恢复，操作前务必先做快照备份
- **containerd 快照目录残留**：Pod 删除后如果容器清理不完整，overlayfs 挂载点残留会持续占用磁盘空间，需要手动 `umount` 清理

## 相关页面

- [[etcd]] — etcd 运维
- [[containerd]] — containerd 运行时
- [[概念/kubernetes-containerd-integration.md|Kubernetes 与 containerd 集成]] — CRI 通信架构
- [[概念/containerd-pod-lifecycle.md|containerd Pod 生命周期]] — 存储层与容器生命周期的关系
