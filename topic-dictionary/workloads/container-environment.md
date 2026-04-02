# 容器环境（Container Environment）

## 概述

Kubernetes 容器环境为容器提供了若干重要资源，包括文件系统、容器自身信息，以及集群中其他对象的信息。了解这些资源有助于开发人员在容器内正确获取运行时的上下文信息。

## 核心概念/原理

### 文件系统

容器可用的文件系统由两部分组成：
- **容器镜像层**：包含应用程序和预置的静态文件
- **卷（Volumes）**：在 Pod 级别挂载到容器中的持久化或临时存储

### 容器自身信息

- **主机名（Hostname）**：容器的主机名即其所在 Pod 的名称。可通过 `hostname` 命令或 libc 的 `gethostname` 函数调用获取
- **Pod 名称和命名空间**：通过 Downward API 以环境变量的形式注入到容器中
- **用户定义的环境变量**：在 Pod 定义中通过 `env` 或 `envFrom` 指定的环境变量，以及容器镜像构建时静态设置的环境变量，均对容器可见

### 集群信息

当容器创建时，Kubernetes 会将同一命名空间内所有正在运行的 Service 信息以环境变量的形式注入到该容器中。对于名为 `foo` 的 Service，会设置如下环境变量：

```
FOO_SERVICE_HOST=<服务所在的主机地址>
FOO_SERVICE_PORT=<服务暴露的端口>
```

Service 拥有独立的 IP 地址，如果集群启用了 DNS 插件，容器也可以通过 DNS 名称访问这些服务。

> **注意**：这种通过环境变量注入的服务发现方式仅限于容器创建时已经存在的同命名空间 Service 以及 Kubernetes 控制平面服务。

## 关键机制或特性

- **Downward API**：允许将 Pod 和节点的元数据（如 Pod 名称、命名空间、标签、IP 等）以环境变量或卷文件的形式暴露给容器
- **Service 环境变量注入**：在容器启动时自动完成，若后续新增 Service，已运行的容器不会自动获得新的环境变量
- **DNS 服务发现**：比环境变量更灵活的服务发现方式，推荐在大多数场景下使用 DNS 而非依赖环境变量

## 使用场景

- 容器内应用需要知道自身 Pod 名称或所在命名空间以进行日志标记或配置分区
- 在容器启动脚本中需要动态获取同一命名空间内其他服务的地址和端口
- 通过环境变量向容器传递配置参数、密钥引用或运行时上下文

## 最佳实践/注意事项

- **优先使用 DNS 进行服务发现**：Service 环境变量仅在容器创建时注入，后续新增的 Service 对已运行容器不可见；DNS 则没有此限制
- **合理利用 Downward API**：避免在镜像中硬编码 Pod 信息，使用 Downward API 动态注入
- **注意环境变量顺序和覆盖规则**：Pod 中定义的环境变量可以覆盖镜像中静态设置的环境变量
- **跨命名空间访问需使用 FQDN**：若通过 DNS 访问其他命名空间的服务，应使用完整域名（如 `my-service.other-namespace.svc.cluster.local`）

## 参考链接

- [Kubernetes 官方文档：容器环境](https://kubernetes.io/docs/concepts/containers/container-environment/)
- [Kubernetes Downward API 文档](https://kubernetes.io/docs/concepts/workloads/pods/downward-api/)
- [Kubernetes Service 与 DNS](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)
