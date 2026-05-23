---
title: 39 - 容器运行时对比表
description: '| **gVisor** | 沙箱 | 安全隔离 | 性能开销 | v1.25+ | 支持 |'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- kubelet
- containerd
- cri-o
- docker
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 容器运行时对比表 是什么
- 如何 容器运行时对比表
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- 容器运行时对比表
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
created: "2026-05-23"
---

# 39 - 容器运行时对比表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[entities/kubernetes.md|kubernetes]].io/docs/setup/production-environment/container-runtimes](https://kubernetes.io/docs/setup/production-environment/container-runtimes/)

<!-- chunk: 容器运行时对比 -->
## 容器运行时对比

| 运行时 | 类型 | 优势 | 劣势 | K8S版本 | ACK支持 |
|-------|------|------|------|--------|---------|
| **[[containerd|containerd]]** | CRI原生 | 轻量，K8S默认 | 功能比Docker少 | v1.24+默认 | 默认 |
| **CRI-O** | CRI原生 | 轻量，OCI标准 | 生态较小 | v1.24+ | 支持 |
| **Docker** | dockershim | 功能丰富，生态成熟 | **已移除** | <v1.24 | 不支持 |
| **gVisor** | 沙箱 | 安全隔离 | 性能开销 | v1.25+ | 支持 |
| **Kata Containers** | 轻量VM | 强隔离 | 资源开销 | v1.25+ | 支持 |
| **Firecracker** | microVM | 极轻量VM | AWS生态 | v1.25+ | - |

<!-- chunk: containerd详解 -->
## containerd详解

| 特性 | 说明 | 配置方式 |
|-----|------|---------|
| **CRI插件** | K8S原生支持 | /etc/containerd/config.toml |
| **镜像管理** | 拉取/存储/分发 | crictl命令 |
| **容器生命周期** | 创建/启动/停止 | 自动管理 |
| **CNI集成** | 网络插件 | /etc/cni/net.d/ |
| **快照** | 文件系统快照 | overlayfs/native |

```toml
# /etc/containerd/config.toml 示例
version = 2

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    sandbox_image = "registry.k8s.io/pause:3.9"
    [plugins."io.containerd.grpc.v1.cri".containerd]
      default_runtime_name = "runc"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true
    [plugins."io.containerd.grpc.v1.cri".registry]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
          endpoint = ["https://mirror.ccs.tencentyun.com"]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."registry.cn-hangzhou.aliyuncs.com"]
          endpoint = ["https://registry.cn-hangzhou.aliyuncs.com"]
```

<!-- chunk: CRI-O详解 -->
## CRI-O详解

| 特性 | 说明 | 配置文件 |
|-----|------|---------|
| **OCI兼容** | 完全OCI标准 | /etc/crio/crio.conf |
| **版本对齐** | 与K8S版本同步 | 版本号一致 |
| **安全** | 最小攻击面 | 配置 |

```toml
# /etc/crio/crio.conf 示例
[crio]
  [crio.runtime]
    default_runtime = "runc"
    [crio.runtime.runtimes.runc]
      runtime_path = "/usr/bin/runc"
      runtime_type = "oci"
  [crio.image]
    pause_image = "registry.k8s.io/pause:3.9"
  [crio.network]
    network_dir = "/etc/cni/net.d/"
    plugin_dirs = ["/opt/cni/bin/"]
```

<!-- chunk: 性能对比 -->
## 性能对比

| 指标 | containerd | CRI-O | Docker(历史) |
|-----|-----------|-------|-------------|
| **启动延迟** | ~300ms | ~350ms | ~500ms |
| **内存开销** | ~50MB | ~40MB | ~100MB |
| **CPU开销** | 低 | 低 | 中 |
| **镜像拉取** | 快 | 快 | 中 |
| **并发容器** | 高 | 高 | 中 |

<!-- chunk: 安全运行时 -->
## 安全运行时

| 运行时 | 隔离级别 | 原理 | 适用场景 | 性能开销 |
|-------|---------|------|---------|---------|
| **runc** | 命名空间 | Linux NS | 默认 | 无 |
| **gVisor(runsc)** | 用户空间内核 | 系统调用拦截 | 不可信代码 | 20-50% |
| **Kata** | 轻量VM | QEMU/Firecracker | 多租户 | 10-30% |

```yaml
# RuntimeClass配置
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
overhead:
  podFixed:
    memory: "120Mi"
    cpu: "250m"
scheduling:
  nodeSelector:
    runtime: gvisor
---
# 使用RuntimeClass的Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  runtimeClassName: gvisor  # 使用gVisor运行时
  containers:
  - name: app
    image: nginx
```

<!-- chunk: 从Docker迁移到containerd -->
## 从Docker迁移到containerd

| 步骤 | 操作 | 命令 |
|-----|------|------|
| 1 | 安装containerd | `apt install containerd` |
| 2 | 配置containerd | 编辑config.toml |
| 3 | 配置kubelet | `--container-runtime-endpoint=unix:///run/containerd/containerd.sock` |
| 4 | 重启kubelet | `systemctl restart kubelet` |
| 5 | 验证 | `crictl info` |

```bash
# Docker到containerd迁移检查
# 1. 停止kubelet
systemctl stop kubelet

# 2. 停止docker
systemctl stop docker

# 3. 配置containerd
mkdir -p /etc/containerd
containerd config default > /etc/containerd/config.toml
# 编辑config.toml设置SystemdCgroup = true

# 4. 启动containerd
systemctl enable --now containerd

# 5. 修改kubelet配置
# /var/lib/kubelet/kubeadm-flags.env
# 添加: --container-runtime-endpoint=unix:///run/containerd/containerd.sock

# 6. 启动kubelet
systemctl start kubelet

# 7. 验证
crictl info
kubectl get nodes
```

<!-- chunk: crictl命令参考 -->
## crictl命令参考

| 命令 | 用途 | 示例 |
|-----|------|------|
| **crictl ps** | 列出容器 | `crictl ps -a` |
| **crictl pods** | 列出Pod | `crictl pods` |
| **crictl images** | 列出镜像 | `crictl images` |
| **crictl pull** | 拉取镜像 | `crictl pull nginx:latest` |
| **crictl logs** | 查看日志 | `crictl logs <container-id>` |
| **crictl exec** | 执行命令 | `crictl exec -it <id> sh` |
| **crictl inspect** | 检查容器 | `crictl inspect <id>` |
| **crictl rmi** | 删除镜像 | `crictl rmi <image>` |
| **crictl rm** | 删除容器 | `crictl rm <id>` |
| **crictl stats** | 资源统计 | `crictl stats` |

```bash
# crictl配置
cat > /etc/crictl.yaml <<EOF
runtime-endpoint: unix:///run/containerd/containerd.sock
image-endpoint: unix:///run/containerd/containerd.sock
timeout: 10
debug: false
EOF
```

<!-- chunk: 镜像加速配置 -->
## 镜像加速配置

```toml
# containerd镜像加速
[plugins."io.containerd.grpc.v1.cri".registry]
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
      endpoint = ["https://registry.cn-hangzhou.aliyuncs.com"]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."gcr.io"]
      endpoint = ["https://gcr.mirrors.ustc.edu.cn"]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors."k8s.gcr.io"]
      endpoint = ["https://registry.cn-hangzhou.aliyuncs.com/google_containers"]
  [plugins."io.containerd.grpc.v1.cri".registry.configs]
    [plugins."io.containerd.grpc.v1.cri".registry.configs."registry.cn-hangzhou.aliyuncs.com".auth]
      username = "user"
      password = "password"
```

<!-- chunk: 运行时故障排查 -->
## 运行时故障排查

| 问题 | 症状 | 诊断命令 | 解决方案 |
|-----|------|---------|---------|
| **容器无法启动** | ContainerCreating | `crictl logs` | 检查运行时日志 |
| **镜像拉取失败** | ImagePullBackOff | `crictl pull <image>` | 检查仓库配置 |
| **运行时不响应** | 节点NotReady | `systemctl status containerd` | 重启运行时 |
| **存储满** | 创建失败 | `df -h` | 清理未用镜像/容器 |

```bash
# 清理未使用镜像
crictl rmi --prune

# 清理停止的容器
crictl rm $(crictl ps -a -q --state exited)

# 检查运行时状态
systemctl status containerd
journalctl -u containerd -f
```

<!-- chunk: ACK容器运行时 -->
## ACK容器运行时

| 运行时 | ACK配置 | 适用场景 |
|-------|--------|---------|
| **containerd** | 默认 | 标准工作负载 |
| **安全沙箱** | 节点池选择 | 安全敏感 |

---

**运行时原则**: v1.24+使用containerd，安全场景用沙箱运行时

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-02-workloads-applications MOC
- [[domain-02-workloads-applications/README.md|Domain-4: Kubernetes工作负载管理]]
- Domain-4 工作负载 — 开源项目索引
- 01 - Kubernetes 工作负载架构概览 (Workload Architecture Overview)
- 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)
- 03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)
- 04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)
- 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)
- 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
- 07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...
- 08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...
- 09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...

## See Also

- 13-container-lifecycle-hooks
- 14-sidecar-containers-patterns
- 16-runtime-class-configuration
- 17-container-images-registry
