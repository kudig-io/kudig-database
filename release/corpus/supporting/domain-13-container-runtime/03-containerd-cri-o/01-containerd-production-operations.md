---
title: containerd 生产运维指南
description: 面向阿里云 ACK / 专有云 ASO 的 containerd 生产运维实践，涵盖安装配置、镜像加速、dockerd 迁移与故障排查
summary: 面向阿里云 ACK / 专有云 ASO 的 containerd 生产运维实践，涵盖安装配置、镜像加速、dockerd 迁移与故障排查
category: domain-13
tags:
- containerd
- cri
- 容器运行时
- 镜像加速
- docker-migration
- 阿里云
- 专有云
- ack
- aso
- production-operations
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- K8s 运维工程师
estimated_read_time: 15min
intent_queries:
- containerd 生产运维指南 是什么
- 如何在 ACK 节点上安装配置 containerd
- 如何从 dockerd 迁移到 containerd
trigger_keywords:
- containerd
- dockerd 迁移
- 镜像加速
- 容器运行时
- 生产运维
prerequisites:
- kubectl-basics
- containerd-basics
- linux-basics
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



# containerd 生产运维指南

## 目录

- [1. 概述](#1-概述)
- [2. 安装与初始化](#2-安装与初始化)
- [3. 核心配置优化](#3-核心配置优化)
- [4. 镜像命名空间与 crictl](#4-镜像命名空间与-crictl)
- [5. 镜像拉取加速](#5-镜像拉取加速)
- [6. 从 dockerd 迁移到 containerd](#6-从-dockerd-迁移到-containerd)
- [7. 故障排查](#7-故障排查)
- [8. Snapshotter 选型与性能](#8-snapshotter-选型与性能)
- [9. 监控与指标](#9-监控与指标)
- [10. 升级与回滚](#10-升级与回滚)
- [11. 日志轮转与审计](#11-日志轮转与审计)
- [12. 生产检查清单](#12-生产检查清单)
- [13. 相关文档](#13-相关文档)
## 1. 概述

自 Kubernetes 1.24 移除 dockershim 后，containerd 成为 ACK 与专有云 ASO 的默认容器运行时。掌握 containerd 的安装、配置、镜像加速和故障排查，是处理 ImagePullBackOff、Pod 启动慢、节点 NotReady 等工单的基础。

```
Client (crictl/ctr/kubectl)
    ↓ CRI / containerd API
containerd
    ↓
containerd-shim-runc-v2
    ↓
runc / crun
    ↓
Linux Namespace + Cgroups
```

## 2. 安装与初始化

阿里云 ACK 托管节点已预装 containerd，无需手工安装。专有云 ASO 自定义镜像或私有化节点，需要按以下步骤初始化。

以下命令在 Alibaba Cloud Linux 3 / CentOS 7 上安装 containerd 稳定版，并设置为开机自启：

```bash
# 安装 containerd.io（推荐从 Docker 官方 yum 源或 Alibaba Cloud Linux 源获取）
sudo yum install -y containerd.io

# 设置为开机自启并立即启动
sudo systemctl enable --now containerd

# 验证版本与运行状态
containerd --version
systemctl status containerd --no-pager
```

生成默认配置文件，作为后续调优的基线：

```bash
# 生成默认配置并重定向到 /etc/containerd/config.toml
sudo mkdir -p /etc/containerd
containerd config default | sudo tee /etc/containerd/config.toml >/dev/null
```

## 3. 核心配置优化

`/etc/containerd/config.toml` 是生产调优的关键入口。常见调整包括 sandbox 镜像地址、systemd cgroup、镜像仓库 mirror 与 TLS。

配置阿里云 ACR 镜像加速 endpoint，减少公网拉取延迟：

```toml
[plugins."io.containerd.grpc.v1.cri".registry.mirrors]
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
    endpoint = ["https://registry.cn-hangzhou.aliyuncs.com", "https://hub-mirror.c.163.com"]
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."registry.cn-hangzhou.aliyuncs.com"]
    endpoint = ["https://registry.cn-hangzhou.aliyuncs.com"]
```

指定符合阿里云环境的 pause 镜像，避免因拉取不到 sandbox 镜像导致 Pod 卡在 SandboxCreate：

```toml
[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.cn-hangzhou.aliyuncs.com/acs/pause:3.9"
```

启用 systemd cgroup driver，与 Kubelet 的 systemd 驱动保持一致：

```toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
  SystemdCgroup = true
```

修改配置后必须重启 containerd 生效：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 校验配置语法并重启
sudo containerd config dump >/dev/null && sudo systemctl restart containerd
```

## 4. 镜像命名空间与 crictl

containerd 通过 namespace 隔离镜像与容器元数据。Kubernetes 默认使用 `k8s.io`，docker 使用 `moby`。排查时应注意命令所处的 namespace。

配置 crictl 默认连接到 containerd socket，避免每次指定 endpoint：

```bash
# 创建 /etc/crictl.yaml，统一使用 containerd 的 CRI socket
cat <<EOF | sudo tee /etc/crictl.yaml
runtime-endpoint: unix:///run/containerd/containerd.sock
image-endpoint: unix:///run/containerd/containerd.sock
timeout: 10
debug: false
EOF
```

常用运维命令对照：

| 场景 | containerd 命令 | 说明 |
|------|----------------|------|
| 查看 K8s 镜像 | `crictl images` | 自动使用 k8s.io namespace |
| 查看所有 namespace 镜像 | `ctr -n k8s.io images list` | 等同于 crictl images |
| 查看容器 | `crictl ps -a` | 只列出 Pod 业务容器 |
| 查看所有 task | `ctr -n k8s.io tasks list` | 包含 shim 管理的 task |
| 拉取镜像 | `crictl pull nginx:latest` | 用于快速验证仓库连通性 |
| 删除镜像 | `crictl rmi <image-id>` | 清理节点磁盘空间 |

## 5. 镜像拉取加速

在大规模 ACK 集群中，批量扩容常因并发拉取镜像触发公网带宽瓶颈或仓库限流。推荐组合使用 ACR 镜像加速器、本地 Harbor 缓存、节点预热三种手段。

通过 DaemonSet 在节点就绪后预拉取核心业务镜像，可显著降低首次调度延迟：

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: image-prepull
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: image-prepull
  template:
    metadata:
      labels:
        app: image-prepull
    spec:
      initContainers:
      - name: prepull
        image: registry.cn-hangzhou.aliyuncs.com/acs/crictl:latest
        command: ["/bin/sh", "-c"]
        args:
        - |
          crictl pull registry.cn-hangzhou.aliyuncs.com/acs/pause:3.9
          crictl pull registry.cn-hangzhou.aliyuncs.com/myapp/backend:v2.3
        volumeMounts:
        - name: cri
          mountPath: /run/containerd
      containers:
      - name: pause
        image: registry.cn-hangzhou.aliyuncs.com/acs/pause:3.9
      volumes:
      - name: cri
        hostPath:
          path: /run/containerd
```

## 6. 从 dockerd 迁移到 containerd

ACK 1.24+ 已不再支持 dockerd，部分专有云存量节点仍需迁移。迁移前必须 drain 节点，避免业务中断。

迁移前确认节点上所有 Pod 信息已保存，并记录镜像清单：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 1. 驱逐节点上的工作负载
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 2. 记录当前 docker 镜像列表，便于迁移后核对
docker images --format '{{.Repository}}:{{.Tag}}' > /tmp/docker-images.txt

# 3. 停止 dockerd
sudo systemctl stop docker
sudo systemctl disable docker
```

完成 containerd 安装与配置后，恢复节点可调度：

```bash
# 4. 启动 containerd 并设置开机自启
sudo systemctl enable --now containerd

# 5. 验证 Kubelet 已使用 containerd
kubectl get node <node-name> -o jsonpath='{.status.nodeInfo.containerRuntimeVersion}'

# 6. 恢复可调度
kubectl uncordon <node-name>
```

## 7. 故障排查

containerd 相关故障通常表现为 Pod 一直 `ContainerCreating`、镜像拉取失败、容器反复退出等。排查时应先看事件，再看 containerd 日志与 crictl 状态。

实时查看 containerd 日志，定位拉取或 CRI 调用异常：

```bash
# 跟踪 containerd 日志，常用于复现 ImagePullBackOff
sudo journalctl -u containerd -f
```

查看 Kubelet 与 containerd 的交互事件：

```bash
# 查看 Pod 事件，定位 SandboxCreate 或 PullImage 失败
kubectl describe pod <pod-name> -n <namespace>
```

当 Pod 卡在 `ContainerCreating` 时，检查 sandbox 容器是否已创建：

```bash
# 列出所有 Pod sandbox，确认 infra 容器状态
crictl pods
```

常见错误与处理：

| 现象 | 可能根因 | 处理 |
|------|---------|------|
| `ImagePullBackOff` + `unauthorized` | imagePullSecrets 缺失或仓库凭证过期 | 重新创建 Secret 或刷新 ACR 凭据 |
| `failed to create shim` | runc 或 shim 二进制损坏 | 重装 containerd 或检查 `/run/containerd` 权限 |
| `cgroup driver` 不一致 | containerd 使用 cgroupfs 而 Kubelet 使用 systemd | 在 config.toml 中启用 `SystemdCgroup` |
| 节点 DiskPressure | 镜像层堆积 | `crictl rmi` 清理无用镜像，或设置 imageGCThreshold |

## 8. Snapshotter 选型与性能

containerd 支持多种 snapshotter，不同存储后端影响镜像拉取速度、磁盘占用与启动延迟。

| Snapshotter | 适用场景 | 特点 |
|---|---|---|
| overlayfs | 默认通用场景 | 性能好，无需额外依赖 |
| native | 调试 / 特殊测试 | 完整复制，空间占用大 |
| stargz | 大镜像延迟加载 | eStargz 按需拉取，启动快 |
| nydus | 龙蜥/专有云大规模分发 | 与 Dragonfly 配合，适合 ACK 大集群 |
| zfs | ZFS 文件系统 | 适合具备 ZFS 的节点 |

在阿里云 ACK 中，overlayfs 是默认且经过充分验证的方案。若镜像体积超过 1GB 且节点并发启动高，可评估 nydus 或 stargz 以降低节点启动时间。配置 nydus snapshotter 时，需要在 `/etc/containerd/config.toml` 中启用对应插件，并部署 nydus-snapshotter DaemonSet。

## 9. 监控与指标

containerd 原生暴露 Prometheus 指标，默认监听端口 `1338`。采集这些指标有助于提前发现镜像拉取延迟、task 异常、runtime 错误等问题。

```yaml
apiVersion: v1
kind: ServiceMonitor
metadata:
  name: containerd-metrics
  namespace: monitoring
spec:
  endpoints:
  - port: metrics
    interval: 30s
  selector:
    matchLabels:
      app: containerd-exporter
```

关键指标包括 `containerd_container_tasks_total`、`containerd_image_pull_duration_seconds` 与 `containerd_runtime_operations_total`。建议将 containerd 日志接入阿里云 SLS 或 Loki，保留不少于 7 天，便于事后审计。

## 10. 升级与回滚

containerd 升级必须遵循先灰度后全量的原则，避免一次升级所有节点导致集群级故障。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 升级前在单节点验证新版本兼容性
sudo yum update -y containerd.io
sudo systemctl restart containerd
kubectl get node <node-name> -o jsonpath='{.status.nodeInfo.containerRuntimeVersion}{"\n"}'

# 确认该节点 Pod 运行正常后，再按批次升级其他节点
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name>
```

若升级后出现容器无法启动或镜像拉取异常，应立即停止升级并回滚到旧版本 RPM/DEB 包，重启 containerd 后恢复节点可调度。

## 11. 日志轮转与审计

containerd 默认通过 systemd journal 输出日志，长期运行会产生大量日志。建议配置 journald 日志轮转：

```ini
# /etc/systemd/journald.conf
[Journal]
SystemMaxUse=2G
SystemMaxFileSize=100M
MaxFileSec=7day
```

修改后重启 systemd-journald：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
sudo systemctl restart systemd-journald
```

对于审计要求较高的 专有云 ASO 环境，可将 containerd 事件接入审计系统，记录镜像拉取、容器创建/删除等关键操作。

## 12. 生产检查清单

- [ ] `/etc/containerd/config.toml` 已配置 ACR / Harbor 镜像加速 endpoint
- [ ] `sandbox_image` 指向可达的 pause 镜像（建议 ACR 内网地址）
- [ ] containerd 与 Kubelet 使用相同的 cgroup driver（systemd）
- [ ] crictl 已配置默认 runtime-endpoint
- [ ] 已根据镜像规模与节点规模选定 snapshotter
- [ ] Prometheus 已采集 containerd 指标或日志已接入 SLS / Loki
- [ ] 节点保留足够磁盘空间，已配置 imageGC 阈值
- [ ] 从 dockerd 迁移前已完成 drain 与镜像清单核对
- [ ] 关键业务镜像已通过 DaemonSet 或镜像缓存预热
- [ ] containerd 升级与回滚 SOP 已文档化并演练

## 13. 相关文档

- [[domain-13-container-runtime/01-containerd-deep-guide.md|containerd 深度指南]]
- [[domain-13-container-runtime/01-docker/01-docker-architecture-overview.md|Docker 架构概述]]
- [[domain-13-container-runtime/02-image-management/01-harbor-enterprise-image-registry.md|Harbor 企业镜像仓库]]
- [[domain-05-security-compliance/README.md|容器安全合规]]
