---
title: containerd 2.0 新特性 (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- 02-containerd-v2-features
- kubelet
- prometheus
- grafana
- containerd
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 2.0 新特性 是什么
- 如何 containerd 2.0 新特性
trigger_keywords:
- containerd
- '2.0'
- 新特性
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 2.0 新特性

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

Containerd 2.0 是 containerd 运行时的重大版本更新，于 2024 年底发布。它带来了全新的 Sandboxed API、增强的 Transfer Service、改进的 NRI（Node Resource Interface）、更高效的镜像分发和全面的 K8s 1.29+ 支持。Containerd 2.0 移除了大量已废弃的 API 和功能，精简了代码库，提升了性能和安全性。作为 K8s 最广泛使用的容器运行时，containerd 2.0 是云原生基础设施的重要里程碑。

## Key Features（核心能力）

- **Sandboxed API**：新的沙箱 API 替代旧版 CRI PodSandbox，支持更灵活的沙箱管理
- **Transfer Service**：内置镜像传输服务，支持 Registry 间直接镜像同步
- **NRI 增强**：Node Resource Interface 2.0，支持更丰富的容器运行时扩展
- **镜像分发优化**：支持 ORAS 镜像、Lazy Pulling（Stargz/SOCI）
- **安全提升**：默认启用 Seccomp Profile、移弃用 API 清理
- **性能改进**：更快的容器启动、更低的内存占用

## 架构与工作原理

Containerd 2.0 架构保持了核心的 containerd-shim 模型，但对沙箱管理进行了重构。新的 Sandboxed API 将沙箱（Sandbox）作为一等公民，支持在沙箱层面进行资源隔离和生命周期管理。Transfer Service 作为独立子系统，支持镜像的 Pull/Push/Mount 操作，可通过插件扩展。NRI 2.0 允许第三方插件在容器创建和运行时注入设备、环境变量等配置。runc v2 成为默认 shim。

## K8s 集成

Containerd 2.0 通过 CRI v1 与 kubelet 集成，完全兼容 K8s 1.29+。新的 Sandboxed API 为未来 K8s 的 Pod 级别隔离增强奠定基础。Transfer Service 可用于大规模集群的镜像预分发。NRI 增强使 K8s 节点上的资源管理更灵活（如 GPU 设备注入）。containerd 2.0 移弃的 API 需要 K8s 1.24+ 环境。

## 生产用例

- **K8s 生产运行时升级**：从 containerd 1.7 升级到 2.0 获取性能和安全改进
- **边缘部署**：更低的资源占用适合资源受限的边缘节点
- **安全加固集群**：利用默认 seccomp 和增强的安全特性
- **镜像加速**：利用 Lazy Pulling 和 Transfer Service 加速大规模镜像分发

## 安装与配置

### 从 containerd 1.7 升级到 2.0

```bash
# 🟢 检查当前版本
containerd --version
ctr version

# 🟢 备份现有配置
cp /etc/containerd/config.toml /etc/containerd/config.toml.bak.$(date +%Y%m%d)

# 🟢 下载并安装 containerd 2.0
VERSION="2.0.2"
wget https://github.com/containerd/containerd/releases/download/v${VERSION}/containerd-${VERSION}-linux-amd64.tar.gz
tar -xzf containerd-${VERSION}-linux-amd64.tar.gz -C /usr/local

# 🟢 生成新版默认配置（对比差异）
containerd config default > /tmp/config-v2-default.toml
diff /etc/containerd/config.toml /tmp/config-v2-default.toml

# 🟡 重启 containerd（会短暂中断节点上容器）
systemctl daemon-reload
systemctl restart containerd

# 🟢 验证升级
containerd --version
ctr version
kubectl get nodes  # 确认节点 Ready
```

### containerd 2.0 配置示例 (config.toml)

```toml
version = 3  # v2 使用 version = 3 配置格式
root = "/var/lib/containerd"
state = "/run/containerd"

[plugins."io.containerd.cri.v1.runtime"]
  sandbox_image = "registry.k8s.io/pause:3.10"
  enable_selinux = false
  # 默认启用 Seccomp
  enable_unprivileged_ports = false
  enable_unprivileged_icmp = false

  [plugins."io.containerd.cri.v1.runtime".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.cri.v1.runtime".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"  # runc v2 shim
      [plugins."io.containerd.cri.v1.runtime".containerd.runtimes.runc.options]
        SystemdCgroup = true
        BinaryName = "/usr/bin/runc"

  # Transfer Service 配置（镜像加速）
  [plugins."io.containerd.cri.v1.runtime".image_transfer]
    enabled = true

  # NRI 插件配置
  [plugins."io.containerd.nri.v1.nri"]
    disable = false
    plugin_registration_timeout = "5s"
    plugin_request_timeout = "2s"

[plugins."io.containerd.transfer.v1.local"]
  # Lazy Pulling (Stargz/SOCI)
  [plugins."io.containerd.transfer.v1.local".unpack_config]
    platform = "linux/amd64"
```

### NRI 插件示例（GPU 设备注入）

```yaml
# NRI 插件配置示例（/etc/nri/conf.d/gpu-injector.yaml）
apiVersion: nri.containerd.io/v1
kind: NRIPlugin
metadata:
  name: gpu-injector
spec:
  # 在容器创建时注入 GPU 设备
  onCreate:
    - match:
        annotations:
          gpu.nvidia.com/inject: "true"
      actions:
        addDevices:
        - path: /dev/nvidia0
        - path: /dev/nvidiactl
        - path: /dev/nvidia-uvm
        addEnv:
        - name: NVIDIA_VISIBLE_DEVICES
          value: "all"
        addMounts:
        - hostPath: /usr/lib/x86_64-linux-gnu/libnvidia-ml.so
          containerPath: /usr/lib/x86_64-linux-gnu/libnvidia-ml.so
          readOnly: true
```

### Transfer Service 镜像预分发

```bash
# 🟢 使用 Transfer Service 在节点间同步镜像
ctr transfer pull registry.example.com/app:v2.0 localhost:5000/app:v2.0

# 🟢 检查镜像传输状态
ctr transfer ls

# 🟢 Lazy Pulling 验证（Stargz）
ctr images pull --snapshotter=stargz registry.example.com/app:stargz
ctr run --snapshotter=stargz registry.example.com/app:stargz test-container
```

## 运维操作

```bash
# 🟢 检查 containerd 2.0 运行状态
systemctl status containerd
ctr version
ctr plugins ls | grep -E "cri|nri|transfer"

# 🟢 查看容器和沙箱
ctr containers ls
ctr tasks ls
ctr sandboxes ls  # 新增：沙箱一等公民

# 🟢 检查 NRI 插件状态
ctr nri ls
ls /etc/nri/conf.d/

# 🟢 检查镜像和快照
ctr images ls
ctr snapshots ls

# 🟡 清理未使用镜像
ctr images prune --all

# 🟢 查看 containerd 日志
journalctl -u containerd --since "10 min ago" -f
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 升级后节点 NotReady | 配置格式不兼容 | `journalctl -u containerd` | 使用 `containerd config migrate` 迁移配置 |
| Pod 创建失败 | runc v2 shim 缺失 | `ls /usr/local/bin/containerd-shim-runc-v2` | 重新安装完整二进制包 |
| NRI 插件未加载 | 插件注册超时 | `ctr nri ls` | 检查插件配置/增加 timeout |
| Transfer Service 失败 | 网络/Registry 不可达 | `ctr transfer ls` | 检查网络和 Registry 凭据 |
| 容器启动变慢 | Seccomp 默认启用 | `journalctl -u containerd` | 检查 seccomp profile 兼容性 |

### 升级排查流程

```
containerd 2.0 升级异常
├── 服务无法启动？
│   ├── 配置格式错误 → containerd config migrate
│   ├── 二进制缺失 → 重新下载完整包
│   └── 依赖库缺失 → ldd /usr/local/bin/containerd
├── 节点 NotReady？
│   ├── kubelet 连接失败 → 检查 CRI socket 路径
│   ├── 沙箱创建失败 → 检查 pause 镜像
│   └── 运行时不匹配 → 检查 runtime_type 配置
└── 容器行为异常？
    ├── Seccomp 拦截 → 检查默认 profile
    ├── NRI 注入失败 → 检查插件日志
    └── 镜像拉取失败 → 检查 Transfer Service
```

## 生产案例

### 案例1：大规模集群 containerd 2.0 滚动升级

- **场景**：500 节点集群从 containerd 1.7.15 升级到 2.0.1
- **方案**：
  1. 先在 5 个测试节点验证，确认配置迁移和 NRI 插件兼容
  2. 使用 `containerd config migrate` 自动转换 v2→v3 配置
  3. 按可用区分批滚动：每批 50 节点，cordon → drain → 升级 → uncordon
  4. 每批后验证 Pod 调度和运行正常
- **效果**：零停机完成升级，容器启动速度提升 15%

### 案例2：NRI 插件实现 GPU 动态分配

- **场景**：多租户集群需要按 Pod 注解动态分配 GPU 设备
- **方案**：开发 NRI 插件，在容器创建时根据 annotation 注入对应的 /dev/nvidia* 设备和环境变量
- **效果**：替代 Device Plugin 的静态分配，实现更细粒度的 GPU 共享

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| containerd 2.0 | 功能丰富、NRI/Transfer、CNCF标准 | 配置迁移成本 | K8s 1.29+ 新部署 |
| containerd 1.7 (LTS) | 稳定、广泛验证 | 缺少新特性、即将 EOL | 现有生产环境 |
| CRI-O | 专为K8s设计、轻量 | 功能较少、社区较小 | OpenShift/纯K8s |
| Docker + containerd | 开发体验好 | 生产不推荐Docker shim | 开发环境 |

## 检查清单

- [ ] K8s 版本 >= 1.29（containerd 2.0 最低要求）
- [ ] 配置文件已迁移到 v3 格式
- [ ] runc v2 shim 已安装
- [ ] NRI 插件已测试兼容
- [ ] Seccomp 默认策略与应用兼容
- [ ] 滚动升级策略已制定（cordon/drain/uncordon）
- [ ] 回滚方案已准备（保留 1.7 二进制和配置）
- [ ] 监控指标已验证（containerd metrics endpoint）

## Related

- [[k3s]] — k3s 轻量级 Kubernetes
- [[23-实体/02-K8s核心组件/virtual-kubelet.md|kubelet]]]] — Virtual Kubelet
- [[kudo]] — KUDO
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 02-containerd-v2-features


<!-- risk-assessed -->
