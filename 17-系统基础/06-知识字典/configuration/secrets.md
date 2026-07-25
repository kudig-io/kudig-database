---
title: Secrets
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- kubelet
- docker
- opa
- ingress
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Secrets 是什么
- 如何 Secrets
trigger_keywords:
- Secrets
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Secrets

## 概述

Secret 是 [[Kubernetes|Kubernetes]] 中用于存储敏感数据（如密码、令牌、密钥等）的 API 对象。使用 Secret 可以避免将机密信息硬编码到 Pod 规约或容器镜像中，从而降低在创建、查看和编辑 Pod 过程中泄露敏感数据的风险。

## 核心概念/原理

- **与 ConfigMap 的对比**：Secret 与 ConfigMap 类似，但专门用于保存机密数据。Kubernetes 对 Secret 对象会施加额外的保护措施。
- **默认存储状态**：默认情况下，Secret 以未加密形式存储在 API Server 的后端数据存储（[[17-系统基础/06-知识字典/fundamentals/etcd.md|etcd]]）中。任何拥有 API 访问权限或 etcd 访问权限的人都可以读取或修改 Secret。
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
3. **镜像拉取凭据**：通过 `imagePullSecrets` 将 Docker 注册表凭据传递给 [[kubelet|kubelet]]，用于拉取私有镜像。
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

## 生产 YAML 示例

### TLS Secret + 镜像拉取凭据

```yaml
# 1. TLS Secret（Ingress HTTPS 证书）
apiVersion: v1
kind: Secret
metadata:
  name: tls-wildcard-example
  namespace: production
type: kubernetes.io/tls
data:
  tls.crt: <base64-encoded-cert>
  tls.key: <base64-encoded-key>
immutable: true                            # 证书更新时创建新 Secret + 更新 Ingress 引用
---
# 2. 私有镜像仓库凭据
apiVersion: v1
kind: Secret
metadata:
  name: registry-credentials
  namespace: production
type: kubernetes.io/dockerconfigjson
data:
  .dockerconfigjson: <base64-encoded-docker-config>
---
# 3. 应用数据库密码（Opaque 类型）
apiVersion: v1
kind: Secret
metadata:
  name: db-credentials
  namespace: production
  labels:
    app: order-service
type: Opaque
stringData:                                # stringData 接受明文，创建时自动 base64 编码
  DB_USERNAME: "app_user"
  DB_PASSWORD: "s3cur3-p@ssw0rd!"
```

### Pod 安全使用 Secret

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: order-service
  template:
    metadata:
      labels:
        app: order-service
    spec:
      imagePullSecrets:
        - name: registry-credentials       # 镜像拉取凭据
      containers:
        - name: app
          image: registry.example.com/order:v3.0
          env:
            - name: DB_USERNAME
              valueFrom:
                secretKeyRef:
                  name: db-credentials
                  key: DB_USERNAME
            - name: DB_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: db-credentials
                  key: DB_PASSWORD
          volumeMounts:
            - name: tls-certs
              mountPath: /etc/tls
              readOnly: true
          resources:
            requests:
              cpu: "500m"
              memory: 512Mi
      volumes:
        - name: tls-certs
          secret:
            secretName: tls-wildcard-example
            defaultMode: 0400              # 严格文件权限
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 启动失败，镜像拉取 ImagePullBackOff | imagePullSecrets 缺失或凭据过期 | `kubectl get secret registry-credentials -o yaml`；重新创建凭据 |
| Secret 卷挂载后文件内容为空 | Secret data 字段为空或 key 不存在 | `kubectl get secret <name> -o jsonpath='{.data}'` 检查内容 |
| base64 解码后内容包含多余换行 | 编码时包含了尾部换行符 | `echo -n 'value' | base64`（注意 `-n` 避免换行） |
| 环境变量中 Secret 未更新 | 环境变量注入不自动更新 | 重启 Pod 或改用卷挂载方式 |
| etcd 中 Secret 明文可见 | 未启用静态加密 | 配置 EncryptionConfiguration 启用 Secret 加密 |

## 生产检查清单

- [ ] 启用 etcd 静态加密（Encryption at Rest）
- [ ] 配置 RBAC 最小权限：仅授予 Pod 运行所需的 Secret 访问
- [ ] 不授予命名空间级别的 `list` / `watch` Secret 权限
- [ ] 使用 `stringData` 而非手动 base64 编码（减少错误）
- [ ] 证书类 Secret 设置 `immutable: true`
- [ ] 建立 Secret 定期轮转机制
- [ ] 高安全场景集成外部密钥管理（Vault / AWS Secrets Manager / CSI driver）
- [ ] 卷挂载 Secret 文件权限设为 0400 或 0440
- [ ] 不将 Secret YAML 提交到版本控制系统

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 从字面量创建 Opaque Secret
kubectl create secret generic db-credentials \
  --from-literal=DB_USERNAME=app_user \
  --from-literal=DB_PASSWORD='s3cur3-p@ssw0rd!' \
  -n production

# 创建 TLS Secret
kubectl create secret tls tls-wildcard-example \
  --cert=tls.crt --key=tls.key -n production

# 创建镜像拉取凭据
kubectl create secret docker-registry registry-credentials \
  --docker-server=registry.example.com \
  --docker-username=user --docker-password=pass \
  -n production

# 查看 Secret（解码值）
kubectl get secret db-credentials -n production -o jsonpath='{.data.DB_PASSWORD}' | base64 -d

# 检查 Secret 类型
kubectl get secrets -n production -o custom-columns='NAME:.metadata.name,TYPE:.type'

# 查看引用 Secret 的 Pod
kubectl get pods -n production -o json | jq '.items[] | select(.spec.volumes[]?.secret.secretName == "db-credentials") | .metadata.name'
```
## 交叉引用

- [ConfigMaps](./configmaps.md) — 非机密配置存储
- [存活、就绪和启动探针](./liveness-readiness-and-startup-probes.md) — 探针可验证密钥/证书加载
- [使用 kubeconfig 文件组织集群访问](./organizing-cluster-access-using-kubeconfig-files.md) — kubeconfig 中的认证凭据

## 参考链接

- [Kubernetes 官方文档 - Secrets](https://kubernetes.io/docs/concepts/configuration/secret/)

## Related
- [[21-生态参考/03-领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]


<!-- risk-assessed -->
