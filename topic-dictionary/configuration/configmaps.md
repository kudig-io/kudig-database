# ConfigMaps

## 概述

ConfigMap 是 Kubernetes 中用于存储非机密数据的 API 对象，以键值对（key-value）形式保存。Pod 可以将 ConfigMap 用作环境变量、命令行参数，或者作为卷中的配置文件。通过 ConfigMap，你可以将环境相关的配置与容器镜像解耦，使应用更易于移植。

## 核心概念/原理

- **键值对存储**：ConfigMap 使用 `data` 字段存储 UTF-8 字符串，使用 `binaryData` 字段存储 base64 编码的二进制数据。
- **命名规范**：ConfigMap 的名称必须是合法的 DNS 子域名；`data` 和 `binaryData` 中的键名只能包含字母、数字、`-`、`_` 或 `.`，且两个字段中的键不能重复。
- **大小限制**：ConfigMap 的数据总量不能超过 1 MiB，不适合存储大块数据。
- **不可变 ConfigMap**：从 v1.19 开始，可以设置 `immutable: true` 创建不可变 ConfigMap，防止意外修改并降低 API Server 的负载。

## 关键机制或特性

Pod 中使用 ConfigMap 的四种方式：

1. **容器命令和参数**：在 `command` 或 `args` 中引用 ConfigMap 的值。
2. **环境变量**：通过 `env.valueFrom.configMapKeyRef` 或 `envFrom.configMapRef` 将键值注入为环境变量。
3. **只读卷挂载**：将 ConfigMap 挂载为卷中的文件，供应用读取。
4. **Kubernetes API 读取**：在 Pod 内通过代码直接调用 Kubernetes API 读取 ConfigMap，可订阅变更事件，也能访问其他命名空间的 ConfigMap。

**自动更新机制**：
- 通过卷挂载的 ConfigMap 在更新后会自动同步到 Pod（ eventual consistency，延迟取决于 kubelet 同步周期和缓存策略）。
- 通过环境变量注入的 ConfigMap 不会自动更新，需要重启 Pod 才能生效。
- 使用 `subPath` 挂载的 ConfigMap 不会接收更新。

## 使用场景

- 将开发环境（`localhost`）和生产环境（Kubernetes Service）的配置分离，例如数据库主机地址。
- 为同一应用在不同命名空间或集群中提供不同的配置，而无需重新构建镜像。
- 存储小型配置文件（如 `.properties`、`.conf`），供应用启动时读取。

## 最佳实践/注意事项

- **不存储机密数据**：ConfigMap 不提供加密或保密能力，敏感信息应使用 Secret 或第三方加密工具管理。
- **控制数据大小**：超过 1 MiB 的数据应使用持久卷、对象存储或数据库。
- **静态 Pod 限制**：静态 Pod（Static Pod）的 spec 不能引用 ConfigMap 或其他 API 对象。
- **使用不可变 ConfigMap**：对于大规模使用的 ConfigMap，建议标记为 `immutable`，以避免意外更新导致的应用中断，并提升集群性能。
- **键名合法性**：确保键名符合环境变量命名规则，否则部分键可能无法注入为环境变量。

## 参考链接

- [Kubernetes 官方文档 - ConfigMaps](https://kubernetes.io/docs/concepts/configuration/configmap/)
