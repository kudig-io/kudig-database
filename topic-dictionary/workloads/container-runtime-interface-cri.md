# 容器运行时接口（Container Runtime Interface, CRI）

## 概述

容器运行时接口（CRI）是一个插件接口，它使 kubelet 能够使用多种不同的容器运行时，而无需重新编译集群组件。CRI 是 kubelet 与容器运行时之间的主要通信协议，采用 gRPC 定义。

## 核心概念/原理

### 为什么需要 CRI

在 Kubernetes 早期，kubelet 与容器运行时（如 Docker）紧密耦合。为了支持更多运行时（如 containerd、CRI-O），Kubernetes 引入了 CRI，将 kubelet 与具体的容器运行时解耦，使得：

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

## 参考链接

- [Kubernetes 官方文档：容器运行时接口（CRI）](https://kubernetes.io/docs/concepts/containers/cri/)
- [CRI 协议定义（GitHub）](https://github.com/kubernetes/cri-api/)
- [containerd 官方文档](https://containerd.io/)
- [CRI-O 官方文档](https://cri-o.io/)
