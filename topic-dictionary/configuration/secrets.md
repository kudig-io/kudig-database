# Secrets

## 概述

Secret 是 Kubernetes 中用于存储敏感数据（如密码、令牌、密钥等）的 API 对象。使用 Secret 可以避免将机密信息硬编码到 Pod 规约或容器镜像中，从而降低在创建、查看和编辑 Pod 过程中泄露敏感数据的风险。

## 核心概念/原理

- **与 ConfigMap 的对比**：Secret 与 ConfigMap 类似，但专门用于保存机密数据。Kubernetes 对 Secret 对象会施加额外的保护措施。
- **默认存储状态**：默认情况下，Secret 以未加密形式存储在 API Server 的后端数据存储（etcd）中。任何拥有 API 访问权限或 etcd 访问权限的人都可以读取或修改 Secret。
- **访问控制**：在该命名空间中拥有创建 Pod 权限的用户，可以间接读取该命名空间下的所有 Secret（例如通过 Deployment）。因此，必须配合 RBAC 进行严格授权。
- **数据字段**：
  - `data`：值为 base64 编码的字符串。
  - `stringData`：接受明文字符串，创建时自动合并到 `data` 中（注意与 server-side apply 兼容性较差）。
  - 单个 Secret 大小不能超过 1 MiB。

## 关键机制或特性

### 内置 Secret 类型

| 内置类型 | 用途 |
|---------|------|
| `Opaque` | 任意用户自定义数据（默认类型） |
| `kubernetes.io/service-account-token` | ServiceAccount 令牌（v1.22+ 推荐使用 TokenRequest API 获取短期令牌） |
| `kubernetes.io/dockercfg` | 序列化的 `~/.dockercfg` 文件（旧版） |
| `kubernetes.io/dockerconfigjson` | 序列化的 `~/.docker/config.json` 文件 |
| `kubernetes.io/basic-auth` | HTTP Basic Auth 凭据（需包含 `username` 和 `password`） |
| `kubernetes.io/ssh-auth` | SSH 认证凭据（需包含 `ssh-privatekey`） |
| `kubernetes.io/tls` | TLS 证书和密钥（需包含 `tls.crt` 和 `tls.key`） |
| `bootstrap.kubernetes.io/token` | 节点引导（bootstrap）令牌 |

### 在 Pod 中使用 Secret

1. **卷挂载**：将 Secret 挂载为只读文件，Secret 更新后卷中的文件会最终一致地同步。
2. **环境变量**：通过 `env.valueFrom.secretKeyRef` 注入，但环境变量中的 Secret 不会自动更新。
3. **镜像拉取凭据**：通过 `imagePullSecrets` 将 Docker 注册表凭据传递给 kubelet，用于拉取私有镜像。
4. **可选 Secret**：在卷中设置 `optional: true`，当 Secret 不存在时 Pod 仍可启动。

### 安全机制

- **节点分发限制**：Secret 仅会被发送到需要它的节点。
- **tmpfs 存储**：kubelet 将 Secret 数据写入 tmpfs，避免写入持久化存储；Pod 删除后，本地副本也会被清除。
- **不可变 Secret（v1.21+）**：设置 `immutable: true` 可防止意外修改，减少 API Server 的 watch 负载。

## 使用场景

- 存储数据库密码、API 密钥、TLS 证书等敏感配置。
- 为 Pod 提供私有镜像仓库的拉取凭据（`imagePullSecrets`）。
- 为 Ingress 或 Service Mesh 配置 TLS 加密通信。
- 节点引导过程中使用 bootstrap token 自动化节点注册。

## 最佳实践/注意事项

- **最小权限原则**：仅授予 Pod 运行所需的最小 Secret 访问权限，避免授予命名空间级别的 `list`/`watch` Secret 权限。
- **启用静态加密**：在 etcd 中启用 Secret 的静态数据加密（Encryption at Rest），提升数据安全性。
- **优先使用短期令牌**：对于 ServiceAccount 令牌，优先使用 TokenRequest API 获取自动轮转的短期令牌，而非长期存储在 Secret 中。
- **定期轮转**：建立 Secret 内容的定期轮换机制，降低凭证泄露后的风险窗口。
- **隔离命名空间**：利用命名空间隔离不同团队或应用的 Secret 访问范围。
- **替代方案**：对于高安全要求场景，可结合外部密钥管理系统（如 Vault、AWS Secrets Manager、Azure Key Vault）或 CSI 驱动动态注入 Secret。

## 参考链接

- [Kubernetes 官方文档 - Secrets](https://kubernetes.io/docs/concepts/configuration/secret/)
