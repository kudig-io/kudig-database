# Kubernetes API Server 绕过风险

## 概述

Kubernetes API server 是外部用户和服务与集群交互的主要入口。作为这一角色，API server 具有多项关键的内置安全控制，例如审计日志和准入控制器。然而，存在一些可以修改集群配置或内容的方式，能够绕过这些控制。本文描述了 API server 内置安全控制可能被绕过的途径，以便集群运营人员和安全架构师能够确保这些绕过途径得到适当限制。

## 核心概念/原理

攻击者如果能够直接访问某些组件或配置，就可以在不经过 API server 的情况下创建、修改或删除集群中的资源，从而绕过 API server 的审计和准入控制。主要风险点包括：Static Pods、Kubelet API、etcd API 和容器运行时套接字。

## 关键机制或特性

### Static Pods（静态 Pod）

每个节点上的 kubelet 会直接加载并管理存储在指定目录或从特定 URL 获取的 Pod 清单，这些 Pod 被称为**静态 Pod**。API server 不管理这些静态 Pod。

- **风险**：具有该目录或 URL 写入权限的攻击者可以修改现有静态 Pod 的配置或引入新的静态 Pod。
- **影响**：静态 Pod 虽然无法访问 Kubernetes API 中的其他对象（如 Secret），但可以执行其他敏感操作，例如通过 `hostPath` 挂载访问底层节点文件系统。
- **隐藏风险**：默认情况下 kubelet 会创建 mirror pod 使静态 Pod 在 API 中可见。但如果攻击者使用无效的命名空间名称，则该 Pod 在 Kubernetes API 中不可见，只能通过访问受影响主机上的工具发现。
- **缓解措施**：
  - 仅在节点需要时启用静态 Pod 清单功能。
  - 限制对静态 Pod 清单目录/URL 的文件系统访问。
  - 限制对 kubelet 配置文件和参数的访问，防止攻击者设置静态 Pod 路径。
  - 定期审计并集中报告对静态 Pod 目录和 kubelet 配置文件的访问。

### Kubelet API

Kubelet 在集群工作节点上通常通过 TCP **10250** 端口暴露 HTTP API（某些发行版也会在控制平面节点上暴露）。

- **风险**：直接访问该 API 可披露节点上运行的 Pod 信息、Pod 日志，并在节点上每个容器中执行命令。
  - 某些端点支持通过 HTTP `GET` 请求的 Websocket 协议，且使用 `get` 动词授权。因此，`nodes/proxy` 的 `get` 权限**不是只读权限**，它授权访问可用于在任何容器中执行命令的端点。
- **影响**：直接访问 kubelet API 不受 Kubernetes 准入控制约束，也不被 Kubernetes 审计日志记录。
- **缓解措施**：
  - 使用 RBAC 严格限制对 `Node` 对象子资源的访问，仅在需要时（如监控服务）授予。
  - 避免授予 `nodes/proxy` 的通配权限，即使只有 `get` 动词。
  - 在网络层面限制对 kubelet 端口的访问，仅允许指定的可信 IP 范围。
  - 确保 kubelet 认证设置为 webhook 或证书模式，禁用未认证的“只读” kubelet 端口。

### etcd API

Kubernetes 集群使用 etcd 作为数据存储。etcd 服务通常在 TCP **2379** 端口监听。唯一需要访问的客户端是 Kubernetes API server 和备份工具。

- **风险**：直接访问 etcd API 可披露或修改集群中保存的任何数据。
- **影响**：直接访问 etcd 不受 Kubernetes 准入控制，也不被 Kubernetes 审计日志记录。攻击者如果读取了 API server 的 etcd 客户端证书私钥，或能创建新的受信任客户端证书，即可通过访问集群 Secret 或修改访问规则获得集群管理员权限。
- **缓解措施**：
  - 确保 etcd 信任的 CA 仅用于该服务的认证。
  - 控制对 etcd 服务器证书私钥以及 API server 客户端证书和私钥的访问。
  - 在网络层面考虑限制对 etcd 端口的访问，仅允许指定的可信 IP 范围。

### 容器运行时套接字（Container Runtime Socket）

在 Kubernetes 集群的每个节点上，与容器交互的访问权限由容器运行时控制。运行时通常暴露一个 Unix 套接字供 kubelet 访问。

- **风险**：能够访问该套接字的攻击者可以启动新容器或与正在运行的容器交互。
- **影响**：在集群层面，这种访问的影响取决于 compromised 节点上的容器是否能够访问 Secret 或其他机密数据，这些数据可能被用于将权限提升到其他工作节点或控制平面组件。
- **缓解措施**：
  - 严格控制对容器运行时套接字的文件系统访问，尽可能仅允许 `root` 用户访问。
  - 使用 Linux 内核命名空间等机制将 kubelet 与节点上其他组件隔离。
  - 限制或禁止挂载包含容器运行时套接字的 `hostPath` 卷，或将 `hostPath` 卷设置为只读。
  - 限制用户对节点的访问，尤其是限制对节点的超级用户访问。

## 使用场景

- 集群运营人员和安全架构师评估集群中的 API server 绕过风险。
- 制定节点和组件加固策略，防止攻击者绕过中央审计和准入控制。
- 进行安全审计和渗透测试，识别潜在的权限提升路径。

## 最佳实践/注意事项

- 认识到 API server 不是唯一可以影响集群状态的入口；kubelet、etcd 和容器运行时套接字都是需要重点保护的组件。
- 对所有这些绕过路径实施**纵深防御**：文件系统访问控制、网络隔离、RBAC 限制和定期审计。
- 确保 etcd 使用独立的 CA 和 mTLS，且网络访问受到严格限制。
- 监控对 kubelet 和容器运行时套接字的不寻常访问模式。

## 参考链接

- https://kubernetes.io/docs/concepts/security/api-server-bypass-risks/
