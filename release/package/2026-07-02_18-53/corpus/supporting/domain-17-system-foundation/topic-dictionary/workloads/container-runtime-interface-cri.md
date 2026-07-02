---
title: 容器运行时接口（Container Runtime Interface, CRI）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- containerd
- cri-o
- docker
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器运行时接口（Container Runtime Interface, CRI） 是什么
- 如何 容器运行时接口（Container Runtime Interface, CRI）
trigger_keywords:
- 容器运行时接口
- Container
- Runtime
- Interface
- CRI
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 容器运行时接口（[[concepts/container-runtime.md|Container Runtime]] Interface, CRI）

## 概述

容器运行时接口（CRI）是一个插件接口，它使 [[kubelet|kubelet]] 能够使用多种不同的容器运行时，而无需重新编译集群组件。CRI 是 kubelet 与容器运行时之间的主要通信协议，采用 [[gRPC|gRPC]] 定义。

## 核心概念/原理

### 为什么需要 CRI

在 [[Kubernetes|Kubernetes]] 早期，kubelet 与容器运行时（如 Docker）紧密耦合。为了支持更多运行时（如 containerd、CRI-O），Kubernetes 引入了 CRI，将 kubelet 与具体的容器运行时解耦，使得：

- 社区和厂商可以独立开发新的容器运行时
- kubelet 无需为每种运行时做定制化开发
- 用户可以根据需求选择最适合的容器运行时

### CRI API（v1 Stable）

自 Kubernetes v1.23 起，CRI v1 API 进入 Stable 状态。

- kubelet 作为客户端，通过 gRPC 连接到容器运行时
- 容器运行时必须提供运行时服务（Runtime Service）和镜像服务（Image Service）端点
- kubelet 通过 `--container-runtime-endpoint` 命令行标志配置 CRI 端点

从 Kubernetes v1.26 开始，kubelet 要求容器运行时**必须支持 CRI v1 API**。如果不支持，kubelet 将无法注册该节点。

### 通信模型

```
┌─────────┐      gRPC (CRI)      ┌─────────────────┐
│ kubelet │  ◄────────────────►  │ Container Runtime│
│ (client)│                     │ (server)         │
└─────────┘                     └─────────────────┘
```

CRI 定义了两类核心服务：

1. **RuntimeService**：负责 Pod 和容器的生命周期管理（创建、启动、停止、删除、状态查询等）
2. **ImageService**：负责镜像的拉取、查看和删除等操作

## 关键机制或特性

### 升级兼容性

- 升级节点上的 Kubernetes 版本时，kubelet 会重启
- 如果容器运行时不支持 CRI v1 API，kubelet 将无法注册节点并报错
- 如果容器运行时升级后需要重新建立 gRPC 连接，运行时也必须支持 CRI v1 API，连接才能成功
- 在某些情况下，正确配置容器运行时后可能需要重启 kubelet

### 支持的容器运行时

目前主流支持 CRI 的容器运行时包括：

- **containerd**：CNCF 毕业项目，Docker 的核心运行时，轻量且性能优异
- **CRI-O**：专为 Kubernetes 设计的轻量级容器运行时，与 OCI 兼容
- **Docker（通过 cri-dockerd）**：Docker Engine 不再被 kubelet 直接支持，需要通过 cri-dockerd 适配 CRI

## 使用场景

- **标准化容器运行时接入**：任何实现了 CRI 的容器运行时都可以被 Kubernetes 使用
- **选择轻量级运行时**：在高密度或边缘计算场景中，使用 containerd 或 CRI-O 替代完整的 Docker 引擎
- **安全增强运行时**：通过 CRI 接入基于 VM 的沙箱运行时（如 Kata Containers、gVisor）
- **混合运行时集群**：结合 RuntimeClass，在同一集群中同时使用多种容器运行时

## 最佳实践/注意事项

- **确保容器运行时支持 CRI v1**：在 Kubernetes v1.26+ 的集群中，必须确认运行时支持 CRI v1 API，否则节点无法就绪
- **正确配置 CRI 端点**：检查 kubelet 的 `--container-runtime-endpoint` 配置是否指向正确的 socket 路径（如 `unix:///run/containerd/containerd.sock`）
- **升级时先升级运行时或确认兼容性**：在进行 Kubernetes 大版本升级前，确认当前容器运行时的 CRI 支持情况
- **监控节点注册状态**：若节点长时间 NotReady，可检查 kubelet 日志中是否存在 CRI 连接或版本不相关的错误

## 生产 YAML 示例

### kubelet CRI 端点配置

```yaml
# /var/lib/kubelet/config.yaml（kubelet 配置文件）
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
containerRuntimeEndpoint: unix:///run/containerd/containerd.sock
# 或 CRI-O：unix:///var/run/crio/crio.sock
# 或 cri-dockerd：unix:///var/run/cri-dockerd.sock
imageServiceEndpoint: ""       # 为空时使用与 containerRuntimeEndpoint 相同的值
```

### containerd 多 handler 配置

```toml
# /etc/containerd/config.toml
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"

  # 默认 runc handler
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
      SystemdCgroup = true

  # Kata Containers handler（沙箱运行时）
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
    runtime_type = "io.containerd.kata.v2"
    privileged_without_host_devices = true

  # gVisor handler
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.gvisor]
    runtime_type = "io.containerd.runsc.v1"
```

### CRI-O 运行时配置

```ini
# /etc/crio/crio.conf.d/10-runtimes.conf
[crio.runtime]
default_runtime = "runc"

[crio.runtime.runtimes.runc]
runtime_path = "/usr/bin/runc"
runtime_type = "oci"
monitor_path = "/usr/bin/conmon"

[crio.runtime.runtimes.kata]
runtime_path = "/usr/bin/kata-runtime"
runtime_type = "oci"
privileged_without_host_devices = true
```

### 结合 RuntimeClass 使用

```yaml
# RuntimeClass 对象映射到 CRI handler
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: gvisor                  # 对应 containerd 配置中的 handler 名称
---
# Pod 使用该 RuntimeClass
apiVersion: v1
kind: Pod
metadata:
  name: untrusted-workload
spec:
  runtimeClassName: gvisor       # 通过 CRI 调用 gvisor handler
  containers:
  - name: app
    image: nginx:1.27
```

## 运行时对比矩阵

| 维度 | containerd | CRI-O | cri-dockerd |
|------|-----------|-------|-------------|
| 定位 | 通用容器运行时 | 专为 K8s 设计 | Docker 适配层 |
| CNCF 状态 | 毕业项目 | 孵化项目 | 社区维护 |
| 性能 | 优秀 | 优秀 | 额外开销（经过 Docker） |
| 镜像构建 | 需搭配 BuildKit/nerdctl | 不支持（需外部工具） | 支持 docker build |
| 默认 socket | `/run/containerd/containerd.sock` | `/var/run/crio/crio.sock` | `/var/run/cri-dockerd.sock` |
| 多 handler | 支持（config.toml） | 支持（crio.conf） | 不支持 |
| 推荐场景 | 通用生产集群 | Red Hat/OpenShift 生态 | 仅限迁移过渡期 |

## CRI 架构流程

```
                    kubelet
                      │
                      │ gRPC (CRI v1)
                      ▼
              ┌───────────────┐
              │   CRI Server  │ (containerd / CRI-O)
              └───┬───────┬───┘
                  │       │
         RuntimeService  ImageService
              │               │
    ┌─────────┴──────┐       │
    │  OCI Runtime   │   Pull/List/Remove
    │ (runc/kata/    │   Images
    │  runsc/...)    │
    └────────────────┘

RuntimeService 操作：
  - RunPodSandbox / StopPodSandbox / RemovePodSandbox
  - CreateContainer / StartContainer / StopContainer / RemoveContainer
  - ListContainers / ContainerStatus
  - ExecSync / Exec / Attach / PortForward

ImageService 操作：
  - PullImage / ListImages / RemoveImage / ImageStatus
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 节点 NotReady，kubelet 日志报 CRI 连接失败 | CRI socket 路径不正确或运行时服务未启动 | `systemctl status containerd`；检查 socket 文件是否存在 |
| kubelet 报 "CRI v1 runtime API is not implemented" | 运行时版本过旧，不支持 CRI v1 | 升级 containerd ≥ 1.6 或 CRI-O ≥ 1.24 |
| 升级 K8s 后节点无法注册 | 运行时不支持新版 CRI API | 先升级容器运行时，再升级 kubelet |
| Pod 创建超时 | 运行时响应慢或 hang | `crictl pods`/`crictl ps` 检查运行时状态；查看 containerd/cri-o 日志 |
| 镜像拉取失败但网络正常 | CRI ImageService 配置问题 | `crictl pull <image>` 直接测试；检查 mirror/proxy 配置 |

## 生产检查清单

- [ ] 容器运行时支持 CRI v1 API（containerd ≥ 1.6，CRI-O ≥ 1.24）
- [ ] kubelet 的 `containerRuntimeEndpoint` 指向正确的 socket 路径
- [ ] 运行时配置了 systemd cgroup driver（与 kubelet 一致）
- [ ] 多 handler 场景下各 handler 配置正确并已测试
- [ ] 运行时服务设置为开机自启（`systemctl enable containerd`）
- [ ] 升级 Kubernetes 前先确认运行时版本兼容性
- [ ] 监控运行时进程的 CPU/内存使用和 gRPC 延迟

## 命令快速参考

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 检查运行时版本
containerd --version
crio --version

# 使用 crictl 诊断（CRI 命令行工具）
crictl info                            # 运行时基本信息
crictl pods                            # 列出 sandbox
crictl ps -a                           # 列出所有容器
crictl images                          # 列出镜像
crictl pull registry.example.com/app:v1   # 测试镜像拉取
crictl stats                           # 容器资源使用统计

# 检查 kubelet 的 CRI 配置
ps aux | grep kubelet | grep container-runtime-endpoint

# 检查 socket 文件
ls -la /run/containerd/containerd.sock
ls -la /var/run/crio/crio.sock

# 查看运行时日志
journalctl -u containerd -f --no-pager
journalctl -u crio -f --no-pager

# 重启运行时（影响节点上所有容器）
sudo systemctl restart containerd
```
## 交叉引用

- [RuntimeClass](runtime-class.md) — 如何通过 RuntimeClass 选择不同 CRI handler
- [容器镜像](images.md) — ImageService 的镜像拉取策略
- [高级 Pod 配置](advanced-pod-configuration.md) — 安全运行时与隔离配置

## 参考链接

- [Kubernetes 官方文档：容器运行时接口（CRI）](https://kubernetes.io/docs/concepts/containers/cri/)
- [CRI 协议定义（GitHub）](https://github.com/kubernetes/cri-api/)
- [containerd 官方文档](https://containerd.io/)
- [CRI-O 官方文档](https://cri-o.io/)

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
