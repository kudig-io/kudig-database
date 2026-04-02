# Kubernetes 中的代理

## 概述

在 Kubernetes 中，用户和集群管理员可能会遇到多种不同类型的代理。理解这些代理的区别和用途，对于正确访问集群服务、调试网络问题以及设计集群架构非常重要。

## 核心概念/原理

Kubernetes 中有五种主要的代理类型：

1. **kubectl proxy**
2. **apiserver proxy**
3. **kube-proxy**
4. **apiserver 前端的代理/负载均衡器**
5. **外部服务的云负载均衡器**

普通 Kubernetes 用户通常只需关注前两种类型，而集群管理员则需要确保后几种类型的正确配置。

## 关键机制或特性

### 1. kubectl proxy

- 运行在用户的桌面或某个 Pod 中。
- 将本地 localhost 地址代理到 Kubernetes apiserver。
- 客户端到代理使用 HTTP。
- 代理到 apiserver 使用 HTTPS。
- 自动定位 apiserver 并添加认证头。
- 常用于本地安全访问 Kubernetes API。

### 2. apiserver proxy

- 内置于 apiserver 中的堡垒代理。
- 将集群外部的用户连接到集群内部可能无法直接访问的 Cluster IP。
- 运行在 apiserver 进程内部。
- 客户端到代理使用 HTTPS（如果 apiserver 允许，也可以使用 HTTP）。
- 代理到目标可能使用 HTTP 或 HTTPS，由代理根据可用信息自动选择。
- 可用于访问 Node、Pod 或 Service，访问 Service 时会进行负载均衡。

### 3. kube-proxy

- 运行在每个节点上。
- 代理 UDP、TCP 和 SCTP 流量。
- 不理解 HTTP 协议。
- 提供负载均衡功能。
- **仅用于访问 Service**。
- 通过 iptables/IPVS/nftables 等机制实现 Service 的网络转发。

### 4. apiserver 前端的代理/负载均衡器

- 存在和实现方式因集群而异（如 nginx、云厂商负载均衡器）。
- 位于所有客户端和一个或多个 apiservers 之间。
- 当有多个 apiserver 时，充当负载均衡器。
- 用于提供高可用性和外部访问入口。

### 5. 外部服务的云负载均衡器

- 由部分云提供商提供（如 AWS ELB、Google Cloud Load Balancer）。
- 当 Kubernetes Service 的类型为 `LoadBalancer` 时自动创建。
- 通常仅支持 UDP/TCP。
- SCTP 支持取决于云提供商的负载均衡器实现。
- 具体实现因云提供商而异。

### 请求重定向

代理已经取代了重定向（redirect）功能。重定向已被弃用。

## 使用场景

- **本地 API 调试**：使用 `kubectl proxy` 安全地访问 Kubernetes API，无需直接处理证书和认证。
- **访问集群内部资源**：通过 `apiserver proxy` 从外部访问特定的 Pod、Service 或节点端口，用于调试和测试。
- **Service 负载均衡**：`kube-proxy` 为集群内部和外部的 Service 访问提供透明的负载均衡。
- **高可用控制平面**：在多个 apiserver 实例前部署负载均衡器，提供故障转移和流量分发。
- **暴露服务到公网**：使用云负载均衡器将集群内的 Service 暴露到互联网。

## 最佳实践/注意事项

- 普通用户日常使用中主要接触 `kubectl proxy` 和 `apiserver proxy`，应理解两者的安全模型和适用场景。
- `kube-proxy` 工作在四层（UDP/TCP/SCTP），不解析 HTTP，因此不适用于基于 HTTP 内容的路由。
- 设计集群入口时，明确区分 `apiserver` 前端的负载均衡器（面向控制平面）与云负载均衡器（面向工作负载 Service）。
- 使用 `apiserver proxy` 访问 Service 时，代理会自动进行负载均衡，但不会保留源 IP（取决于具体配置）。
- 重定向功能已被弃用，新的设计和实现应优先使用代理机制。

## 参考链接

- [Proxies in Kubernetes - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/proxies/)
