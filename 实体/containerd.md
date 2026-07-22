---
title: containerd (entities)
description: containerd — Kubernetes 生产运维知识库
summary: containerd — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- container
- runtime
- containerd
- cni
- csi
- kubelet
- docker
- wasm
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
- containerd 是什么
- 如何 containerd
trigger_keywords:
- containerd
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# containerd

containerd is an industry-standard [[概念/container-runtime.md|container runtime]] that manages the complete container lifecycle on a host system. It was donated to CNCF by Docker in 2017 and became the default K8s runtime after dockershim removal in v1.24.

## Key Facts

- **Status**: CNCF graduated project
- **Architecture**: Monolithic daemon with plugin system
- **Memory Footprint**: ~100MB RAM
- **Default OCI Runtime**: runc (also supports crun, kata)
- **CRI Plugin**: io.containerd.[[gRPC|grpc]].v1.cri
- **Configuration**: /etc/containerd/config.toml

## Core Components

| Component | Function |
|-----------|----------|
| Content Store | Content-addressable blob storage for images |
| Snapshots | Filesystem snapshot management (overlayfs, btrfs, zfs) |
| Tasks | Container process lifecycle management |
| Namespaces | Multi-tenant isolation within containerd |
| Events | Event streaming for monitoring and integration |

## K8s Integration

containerd exposes the Container Runtime Interface (CRI) via gRPC. kubelet communicates directly with containerd without intermediate shim layers. Key configuration: sandbox_image (pause container), SystemdCgroup (use systemd for cgroups), registry mirrors.

## 安装与配置

```bash
# 🟢 低风险：安装 containerd（apt/yum）
apt-get install -y containerd.io  # Debian/Ubuntu
yum install -y containerd.io      # RHEL/CentOS

# 🟡 中风险：生成默认配置
containerd config default > /etc/containerd/config.toml

# 🟡 中风险：启用 systemd cgroup（K8s 必须）
sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
systemctl restart containerd
```

```toml
# /etc/containerd/config.toml 生产配置示例
version = 2
root = "/var/lib/containerd"
state = "/run/containerd"

[grpc]
  address = "/run/containerd/containerd.sock"
  uid = 0
  gid = 0

[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.k8s.io/pause:3.9"
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
  [plugins."io.containerd.grpc.v1.cri".registry]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
        endpoint = ["https://mirror.example.com", "https://registry-1.docker.io"]
    [plugins."io.containerd.grpc.v1.cri".registry.configs]
      [plugins."io.containerd.grpc.v1.cri".registry.configs."harbor.internal.com".tls]
        insecure_skip_verify = false
        ca_file = "/etc/containerd/certs.d/harbor-ca.crt"
```

## 运维操作

```bash
# 🟢 低风险：检查 containerd 状态
systemctl status containerd
ctr version
crictl info

# 🟢 低风险：查看容器和镜像
ctr -n k8s.io containers list
crictl ps -a
crictl images

# 🟢 低风险：查看 containerd 日志
journalctl -u containerd -f --since "10min ago"

# 🟡 中风险：拉取/删除镜像
crictl pull registry.k8s.io/pause:3.9
crictl rmi <image-id>

# 🟡 中风险：重启 containerd（会短暂影响节点上所有 Pod）
systemctl restart containerd

# 🔴 高风险：清理未使用镜像（可能删除正在使用的层）
ctr -n k8s.io images prune

# 🟢 低风险：检查运行时健康
crictl stats
crictl inspect <container-id>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 卡在 ContainerCreating | 镜像拉取失败/registry 不可达 | `crictl pull <image>` | 检查网络、认证、镜像名 |
| kubelet 报 CRI 连接失败 | containerd socket 不存在 | `ls /run/containerd/containerd.sock` | `systemctl restart containerd` |
| 容器 OOMKilled 频繁 | cgroup 配置错误 | `cat /sys/fs/cgroup/memory/...` | 检查 SystemdCgroup 配置 |
| 磁盘空间不足 | 镜像层/容器日志累积 | `du -sh /var/lib/containerd/` | 清理无用镜像、配置日志轮转 |
| 容器启动超时 | runc 挂起/内核锁竞争 | `ctr -n k8s.io tasks list` | 检查 dmesg、升级内核 |
| 镜像拉取 401 | registry 认证过期 | `ctr -n k8s.io images pull --user u:p <img>` | 更新 registry credentials |

```
排查流程：
├── kubelet 无法连接 CRI？
│   ├── systemctl status containerd → 检查服务状态
│   ├── ls /run/containerd/containerd.sock → 确认 socket 存在
│   └── journalctl -u containerd → 查看启动错误
├── 容器创建失败？
│   ├── crictl ps -a → 查看容器状态
│   ├── crictl inspect <id> → 查看详细错误
│   └── dmesg | tail → 检查内核错误（OOM、cgroup）
└── 性能问题？
    ├── crictl stats → 查看资源使用
    ├── iostat -x 1 → 检查磁盘 IO
    └── ctr -n k8s.io content ls → 检查内容存储
```

## 生产案例

### 案例 1：containerd 升级导致节点 NotReady

- **场景**：批量升级 containerd 1.6→1.7，部分节点 kubelet 报 CRI 连接超时
- **排查**：journalctl 显示新版 config.toml 不兼容（`version = 2` 字段变更），containerd 启动失败
- **方案**：回滚 containerd 版本，使用 `containerd config migrate` 迁移配置文件，分批灰度升级
- **效果**：制定升级 SOP：先 migrate config → 单节点验证 → 分批滚动，后续零故障

### 案例 2：镜像层累积导致磁盘告警

- **场景**：生产节点 /var/lib/containerd 使用率达 92%，触发磁盘告警
- **排查**：`ctr -n k8s.io images ls` 发现 200+ 历史镜像未清理，大量 `<none>` 标签的悬空层
- **方案**：配置 kubelet imageGCHighThresholdPercent=80 自动 GC，同时部署 CronJob 定期执行 `crictl rmi --prune`
- **效果**：磁盘使用率稳定在 60% 以下，消除磁盘告警

## 替代方案

| 维度 | containerd | CRI-O | Docker (containerd) |
|------|-----------|-------|--------------------|
| CNCF 状态 | Graduated | CNCF | 非 CNCF |
| 架构 | 单体+插件 | 模块化 | 多层封装 |
| 内存占用 | ~100MB | ~80MB | ~300MB+ |
| K8s 集成 | CRI 原生 | CRI 原生 | 需 shim |
| 多租户 | Namespace 隔离 | 有限 | 有限 |
| 适用场景 | 通用生产 | 纯 K8s | 开发/兼容 |

## Related

- [[概念/linux-container-foundation.md|linux-container-foundation]] — Linux Container Foundation
- [[概念/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[概念/container-runtime-comparison.md|container-runtime-comparison]] — Container Runtime Comparison
- [[docker]] — Docker
- [[实体/container-runtime.md|container-runtime]] — Container Runtime
- [[概念/docker-architecture.md|Docker Architecture]]
- [[概念/container-runtime-comparison.md|Container Runtime Comparison]]
- [[概念/linux-container-foundation.md|Linux Container Foundation]]

- 07-containerd-disaster-recovery
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- [[归档/release-notes/core-deps/containerd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- RELEASE-NOTES-1.6
- [[归档/release-notes/core-deps/containerd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- [[归档/release-notes/core-deps/containerd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- RELEASE-NOTES-1.1
- RELEASE-NOTES-0.0
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- 04-containerd-upgrade-migration
- 02-containerd-wasm-shim
- containerd
- 05-containerd-windows-support
- 02-containerd-v2-features
- 08-containerd-multi-tenant
- 03-containerd-security-hardening
- 06-containerd-observability
- [[实体/k8s-structured-troubleshooting.md|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[实体/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[实体/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[概念/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[概念/linux-sysctl-tuning.md|Linux Sysctl Tuning for Kubernetes]] — Cross-reference
- [[概念/overlayfs-storage.md|OverlayFS Storage]] — Cross-reference
- [[概念/node-lifecycle-management.md|节点生命周期管理]] — Cross-reference
- [[技能/ts-node-components.md|节点组件故障排查]] — Cross-reference
- [[技能/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]] — Cross-reference
- [[技能/k8s-cluster-configuration-guide.md|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[技能/skill-reference-diagnostic-workflow.md|Diagnostic Workflow]] — Cross-reference
- [[技能/skill-reference-root-cause-catalog.md|Root Cause Catalog]] — Cross-reference
- [[技能/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[实体/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/node-index.md|Node 知识图谱索引]]


<!-- risk-assessed -->
