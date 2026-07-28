---
title: youki [entities]
description: '## 概述'
summary: 'youki 是一个用 Rust 实现的 OCI 容器运行时，作为 runc 的替代品。'
category: entities
tags:
- k8s
- cncf
- runtime
- youki
- containerd
- cri-o
- crd
- operator
- wasm
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- youki 是什么
- 如何 youki
trigger_keywords:
- youki
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# youki

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

youki 是一个用 Rust 实现的 OCI 容器运行时（OCI Runtime），作为 runc 的替代品，2022 年加入 CNCF 沙箱。它完全兼容 OCI Runtime Specification，同时利用 Rust 语言的内存安全特性减少潜在的安全漏洞（如 buffer overflow、use-after-free 等 C 语言常见问题）。youki 可与 containerd、CRI-O、Podman 等高级容器运行时集成，作为底层容器执行引擎。youki 还实验性支持 Wasm 运行时特性，可以在同一运行时中运行传统 Linux 容器和 WebAssembly 模块。作为 Rust 实现的运行时，youki 还具有优秀的并发性能和更低的资源开销。

## 核心能力

- **OCI 兼容**: 完全兼容 OCI Runtime Specification，可作为 runc 直接替代
- **内存安全**: 利用 Rust 的所有权系统，消除 buffer overflow、data race 等内存安全漏洞
- **高性能**: Rust 的零成本抽象和优秀的并发模型，性能与 runc 相当或更优
- **Rootless 模式**: 支持非特权用户运行容器（rootless containers）
- **Wasm 支持**: 实验性支持通过 Wasm 运行时运行 WebAssembly 模块
- **cgroups v2**: 完整支持 cgroups v2 资源管理

## 架构

youki 作为底层 OCI Runtime，遵循 OCI 规范设计：

- **youki 二进制**: 替代 runc 的容器运行时二进制，实现 OCI Runtime CLI 接口
- **libcontainer**: youki 的核心库，管理容器生命周期（create/start/kill/delete）
- **Namespaces**: 利用 Linux namespace 实现容器隔离（pid/net/mnt/uts/ipc/user）
- **Cgroups**: 通过 cgroups v1/v2 管理资源限制（CPU/内存/IO/PID）
- **Linux Capabilities**: 精细化的 Linux capability 权限控制
- **Seccomp**: 系统调用过滤，限制容器可用的 syscall

容器生命周期：`containerd/CRI-O → youki create → youki start → youki kill → youki delete`

## K8s 集成

youki 作为 OCI Runtime 与 Kubernetes 集成。在节点上配置 containerd 或 CRI-O 使用 youki 替代默认的 runc（在 containerd config.toml 中设置 `runtime = "youki"`）。youki 处理容器的创建、启动、停止和删除操作，由上层 CRI（containerd/CRI-O）调用。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的容器运行时接口（CRI）完全兼容——替换 youki 不需要修改任何 Pod 或 Deployment 配置。

## 生产场景

1. **安全敏感环境**: 利用 Rust 内存安全减少容器逃逸漏洞风险
2. **Rootless 容器**: 在无 root 权限环境中运行安全容器
3. **边缘轻量运行时**: 边缘设备上使用 Rust 运行时获得更好的资源效率
4. **Wasm + 容器混合**: 在同一节点上运行传统容器和 Wasm 模块

## 安装与配置

```bash
# 从源码安装 youki
git clone https://github.com/containers/youki.git
cd youki && make youki
sudo mv youki /usr/local/bin/

# 或使用包管理器（Fedora）
sudo dnf install youki

# 验证安装
youki --version
youki info

# 运行测试容器
mkdir -p /tmp/container-bundle/rootfs
cd /tmp/container-bundle
youki spec  # 生成 config.json
sudo youki create -b /tmp/container-bundle test-container
sudo youki start test-container
sudo youki kill test-container
sudo youki delete test-container
```

```toml
# containerd 配置使用 youki (/etc/containerd/config.toml)
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.youki]
  runtime_type = "io.containerd.runc.v2"
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.youki.options]
    BinaryName = "/usr/local/bin/youki"

# 设置为默认运行时（可选）
[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "youki"
```

```yaml
# K8s RuntimeClass 配置（按 Pod 选择运行时）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: youki
handler: youki
scheduling:
  nodeSelector:
    runtime: youki
---
# 使用 youki 运行时的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  runtimeClassName: youki
  containers:
  - name: app
    image: nginx:latest
    securityContext:
      runAsNonRoot: true
      allowPrivilegeEscalation: false
```

## 运维操作

```bash
# 🟢 查看 youki 版本和能力
youki --version
youki info

# 🟢 查看运行中的容器
sudo youki list

# 🟢 查看容器详细信息
sudo youki state <container-id>

# 🟡 重启 containerd 以应用 youki 配置
sudo systemctl restart containerd

# 🟢 验证 K8s 节点使用 youki
kubectl get nodes -o custom-columns=NAME:.metadata.name,RUNTIME:.status.nodeInfo.containerRuntimeVersion

# 🟡 切换默认运行时回 runc（回滚）
sudo sed -i 's/default_runtime_name = "youki"/default_runtime_name = "runc"/' /etc/containerd/config.toml
sudo systemctl restart containerd

# 🔴 强制删除卡死的容器
sudo youki delete --force <container-id>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 容器创建失败 | youki 二进制不存在或权限不足 | `which youki && youki --version` | 确认二进制路径和可执行权限 |
| containerd 启动失败 | config.toml 语法错误 | `containerd config dump` | 检查 TOML 格式和 runtime 配置 |
| Pod 调度到错误节点 | RuntimeClass nodeSelector 不匹配 | `kubectl describe pod` | 确认节点标签与 RuntimeClass 一致 |
| 容器内网络异常 | namespace 配置问题 | `youki state <id>` 查看 namespace | 检查 CNI 插件配置 |
| cgroup 限制不生效 | cgroup v1/v2 不匹配 | `stat -fc %T /sys/fs/cgroup/` | 确认 youki 编译时启用的 cgroup 版本 |

```
排查流程：
├── 容器无法启动
│   ├── youki --version 确认二进制可用
│   ├── 检查 config.json 是否有效
│   ├── 查看 containerd 日志: journalctl -u containerd
│   └── 确认 Linux 内核版本 >= 5.4
├── K8s 集成问题
│   ├── kubectl get runtimeclass 确认 RuntimeClass 存在
│   ├── 检查节点标签是否匹配
│   ├── crictl info 查看 CRI 配置
│   └── 确认 containerd 已重启加载新配置
└── 性能问题
    ├── 对比 youki vs runc 启动时间
    ├── 检查 cgroup 配置是否正确
    └── 确认 seccomp profile 不会过度限制
```

## 生产案例

### 案例 1：安全敏感环境容器运行时替换

- **场景**：金融机构 K8s 集群，安全团队要求消除 C 语言运行时的内存安全漏洞风险（CVE 历史）
- **排查**：runc 历史 CVE 中多个涉及 buffer overflow 和 use-after-free，安全审计不通过
- **方案**：将节点运行时替换为 youki（Rust 实现），通过 RuntimeClass 渐进式迁移，先非生产后生产
- **效果**：安全审计通过，容器启动性能与 runc 持平，无兼容性问题，CVE 风险显著降低

### 案例 2：边缘设备 Rootless 容器

- **场景**：IoT 边缘网关无 root 权限，需要运行容器化应用，传统 runc 需要特权
- **排查**：runc rootless 模式配置复杂且稳定性不足，经常遇到 namespace 权限问题
- **方案**：使用 youki rootless 模式，利用 Rust 的精确权限控制，配合 user namespace 映射
- **效果**：无 root 权限稳定运行容器，内存占用比 runc 低 15%，边缘设备运行稳定 6 个月无故障

## 对比

| 特性 | youki | runc | crun | runsc (gVisor) | 适用场景 |
|------|-------|------|------|----------------|----------|
| 语言 | Rust | Go | C | Go | 安全偏好 |
| 内存安全 | ✅ | ⚠️ | ❌ | ⚠️ | 安全敏感环境 |
| 性能 | 高 | 高 | 高 | 中（开销） | 性能要求 |
| OCI 兼容 | ✅ | ✅ | ✅ | ✅ | 无缝替换 |
| 生产成熟度 | 中 | 高 | 中 | 高 | 稳定性要求 |

## 架构定位

在 CNCF 生态中，youki 属于 **Runtime** 类别，为云原生应用提供内存安全的容器运行时能力。

## 参考链接

- [[containerd]]
- [[pod-lifecycle]]

## Related

- [[kairos]] — Kairos
- [[kaito]] — KAITO
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd

- youki
- [[22-概念/container-runtime-comparison.md|[[22-概念/15-运行时与系统/container-runtime|Container Runtime]]me Comparison|Container Runtime Comparison]]]] — Cross-reference
- [[22-概念/docker-architecture.md|[[22-概念/15-运行时与系统/docker-architecture|Docker Architecture and Container Runtime]]]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
