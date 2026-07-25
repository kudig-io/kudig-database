---
title: Secrets
summary: Secrets：Secret 是 Kubernetes 中用于存储和管理敏感信息（如密码、令牌、密钥）的对象。
category: concepts
tags:
- core-concept
- k8s
- security
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# Secrets

## 概述

Secret 是 Kubernetes 中专门用于存储敏感数据（密码、Token、TLS 证书、SSH 私钥等）的对象。与普通 ConfigMap 不同，Secret 在 API 层面有更严格的访问控制，可挂载为独立 Volume 或注入为环境变量，并能配合 etcd 静态加密（Encryption at Rest）进一步保护数据。需要注意的是：Secret 的 value 仅做 **base64 编码**而非加密，所以**必须叠加 etcd 加密 + RBAC + 外部 KMS** 才算真正安全。

## 架构与工作原理

```
┌────────── etcd（EncryptionConfiguration 加密）──────────┐
│   Secret (data: base64)  ←  AESCBC / secretbox / KMS     │
└───────────────┬──────────────────────────────────────────┘
                │ Watch / Get
                ▼
   ┌──── kubelet (节点) ────┐
   │  1. 挂载为 tmpfs Volume │  → /etc/secrets/db-password (明文)
   │  2. 注入为环境变量       │  → $DB_PASSWORD
   └─────────────────────────┘
                │
                ▼
            应用容器（读取明文使用）
```

**Secret 类型（type）**：

| 类型 | 用途 |
|------|------|
| `Opaque` | 通用键值对（最常用） |
| `kubernetes.io/tls` | 存放 TLS 证书 + 私钥 |
| `kubernetes.io/dockerconfigjson` | 私有镜像仓库鉴权 |
| `kubernetes.io/service-account-token` | SA Token（自动） |
| `kubernetes.io/basic-auth` | 用户名/密码 |
| `bootstrap.kubernetes.io/token` | 节点 bootstrap |

**消费方式**：
- **Volume 挂载**（推荐）：以 tmpfs（内存）形式挂载，更新自动同步；容器重启才能感知 env 更新。
- **环境变量**：简单但易在日志/崩溃栈泄露。
- **imagePullSecrets**：为 Pod 提供私有仓库拉取凭据。

## 关键组件与特性

| 特性 | 说明 |
|------|------|
| base64 编码 | data 字段值必须 base64；stringData 可写明文自动转码 |
| etcd 静态加密 | EncryptionConfiguration，使用 aescbc/secretbox/KMS |
| RBAC 绑定 | Secret 应配最小权限 Role，避免默认可读 |
| 不可变 Secret | `immutable: true`（1.21+）降低 apiserver 负载 |
| 外部同步 | External Secrets Operator 从 Vault/AWS SM 拉取并生成 Secret |
| Sealed Secrets | GitOps 友好：仓库存密文，集群内解密为 Secret |

## 配置示例

```yaml
---
# 1. 创建 Secret（stringData 自动转 base64）
apiVersion: v1
kind: Secret
metadata:
  name: db-creds
  namespace: production
type: Opaque
stringData:
  username: appuser
  password: S3cret!Passw0rd
---
# 2. 挂载为 Volume + 环境变量
apiVersion: apps/v1
kind: Deployment
metadata:
  name: webapp
  namespace: production
spec:
  replicas: 2
  selector:
    matchLabels: {app: webapp}
  template:
    metadata:
      labels: {app: webapp}
    spec:
      containers:
      - name: webapp
        image: webapp:v1
        env:
        - name: DB_USER
          valueFrom:
            secretKeyRef: {name: db-creds, key: username}
        - name: DB_PASS
          valueFrom:
            secretKeyRef: {name: db-creds, key: password}
        volumeMounts:
        - name: secrets
          mountPath: /etc/secrets
          readOnly: true
      volumes:
      - name: secrets
        secret:
          secretName: db-creds
          defaultMode: 0400
---
# 3. etcd 静态加密（EncryptionConfiguration）
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources: ["secrets"]
  providers:
  - aescbc:
      keys:
      - name: key1
        secret: <base64-32-byte-key>
  - identity: {}      # 兜底
```

## 常用操作与命令

```bash
# 创建
kubectl create secret generic db-creds \
  --from-literal=username=appuser \
  --from-literal=password='S3cret!' \
  --from-file=ca.crt=./ca.pem

# 查看（base64 解码）
kubectl get secret db-creds -o jsonpath='{.data.password}' | base64 -d ; echo

# TLS Secret
kubectl create secret tls webapp-tls --cert=tls.crt --key=tls.key

# 镜像仓库 Secret
kubectl create secret docker-registry regcred \
  --docker-server=registry.example.com \
  --docker-username=robot \
  --docker-password=$TOKEN \
  --docker-email=ci@example.com

# 触发全量重新加密（更换 key 后）
for ns in $(kubectl get ns -o name); do
  for s in $(kubectl -n "${ns#namespace/}" get secrets -o name); do
    kubectl -n "${ns#namespace/}" get "$s" -o yaml | kubectl replace -f -
  done
done
```

## 最佳实践

1. **必开 etcd 静态加密**：否则 etcd 备份/泄露即等于凭据泄露，用 KMS 管理主密钥更佳。
2. **优先挂 Volume 而非 env**：Volume 更新可同步，env 需重启 Pod；且 env 易在 `kubectl describe` 中看到。
3. **外部密钥管理**：生产环境用 External Secrets Operator + Vault/AWS Secrets Manager，避免在 Git 仓库存敏感值。
4. **RBAC 最小权限**：为应用 SA 只授权读特定 Secret（resourceNames），杜绝 `secrets.*` 全权限。
5. **GitOps 用 Sealed Secrets / SOPS**：把密文纳入 Git，Controller 在集群内解密。
6. **轮换机制**：通过 CronJob + External Secrets `refreshInterval` 定期更新，缩短凭据生命周期。
7. **immutable: true**：只读 Secret 减少 apiserver watch 压力，适合证书类。

## 常见陷阱

- **误以为 base64 是加密**：任何人有读权限即可解码，必须叠加 etcd 加密 + RBAC。
- **环境变量泄露**：崩溃转储、`kubectl describe pod`、APM 上报都可能输出 env，敏感数据用 Volume 更稳妥。
- **未配置 imagePullSecrets**：私有镜像一直 ImagePullBackOff，需要在 SA 上 automount。
- **更换加密 key 后老 Secret 仍明文**：必须重新写入所有 Secret 才会按新 key 加密。
- **超大 Secret**：1.5MB 上限，且全量写回 etcd 压力大，证书链尤其注意。
- **跨命名空间共享**：Secret 是 Namespace 作用域，跨 NS 需复制或用 ExternalSecrets 引用。

## 源码实现分析

### Secret 存储与加密机制

```go
// k8s.io/kubernetes/pkg/registry/core/secret/storage/storage.go
// Secret 写入 etcd 的加密路径
func (r *REST) Create(ctx context.Context, obj runtime.Object) (runtime.Object, error) {
    secret := obj.(*api.Secret)
    // 1. Secret 数据在 apiserver 内存中为明文
    // 2. 写入 etcd 前经过 EncryptionConfiguration 处理
    // 3. 默认无加密（base64 不是加密！）
    // 4. 配置 aescbc/aesgcp/kms provider 后真正加密
    return r.Store.Create(ctx, obj)
}

// k8s.io/apiserver/pkg/server/options/encryptionconfig/config.go
// EncryptionConfiguration 解析
// apiVersion: apiserver.config.k8s.io/v1
// kind: EncryptionConfiguration
// resources:
//   - resources: [secrets]
//     providers:
//       - aescbc:          # 静态密钥加密
//           keys: [{name: key1, secret: <base64>}]
//       - identity: {}     # 回退：无加密
```

```
┌─────────────────────────────────────────────────────────┐
│     Secret 安全架构                                    │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  应用 Pod                                               │
│    │ volume mount / env var                             │
│    ▼                                                    │
│  kubelet (tmpfs 内存文件系统)                          │
│    │ gRPC                                               │
│    ▼                                                    │
│  kube-apiserver                                         │
│    │ EncryptionConfiguration                            │
│    ▼                                                    │
│  etcd (加密存储 / 默认明文 base64)                    │
│                                                         │
│  安全增强方案:                                         │
│  1. etcd 加密 (EncryptionConfiguration)                │
│  2. 外部密钥管理 (Vault/External Secrets)             │
│  3. RBAC 限制 Secret 读取权限                         │
│  4. Sealed Secrets (加密后存 Git)                     │
└─────────────────────────────────────────────────────────┘
```

### 生产运维：Secret 安全管理

```bash
# 🟢 检查 Secret 列表（不显示内容）
kubectl get secrets -A
kubectl get secret <name> -n <ns> -o jsonpath='{.type}'

# 🟡 查看 Secret 内容（敏感操作）
kubectl get secret <name> -n <ns> -o jsonpath='{.data.password}' | base64 -d
# 🔴 生产环境避免直接查看 Secret，使用审计日志记录访问

# 🟢 检查 etcd 加密配置
kubectl get pods -n kube-system -l component=kube-apiserver -o yaml | grep encryption-provider-config

# 🟡 轮换 Secret（触发 Pod 重启）
kubectl create secret generic <name> --from-literal=key=new-value \
  --dry-run=client -o yaml | kubectl apply -f -
kubectl rollout restart deployment/<app> -n <ns>
```

## 面试要点

1. **K8s Secret 默认是否加密？**
   - 默认不加密！base64 只是编码，不是加密
   - etcd 中存储的是 base64 编码的明文
   - 必须配置 EncryptionConfiguration 才能加密存储
   - 生产必须启用 etcd 加密 + RBAC 限制访问

2. **Secret 的 type 有哪些？**
   - Opaque：通用密钥（默认）
   - kubernetes.io/tls：TLS 证书
   - kubernetes.io/dockerconfigjson：镜像拉取凭证
   - kubernetes.io/service-account-token：SA token（已弃用）

3. **External Secrets Operator 如何工作？**
   - 定义 ExternalSecret CRD 引用外部密钥存储（Vault/AWS SSM/GCP SM）
   - Operator Watch ExternalSecret → 从外部拉取 → 创建 K8s Secret
   - 支持自动轮换、审计日志、细粒度权限
   - 避免 Secret 明文存 Git 或 etcd

4. **如何安全地在 CI/CD 中使用 Secret？**
   - 不要硬编码在 YAML 中，使用 Sealed Secrets 或 SOPS
   - CI 中使用 OIDC 短期 token 而非长期凭证
   - Vault Agent Injector 动态注入 Secret 到 Pod
   - 启用审计日志记录所有 Secret 访问

## 相关概念

- [[22-概念/01-核心架构/kubernetes.md|Kubernetes]]
- [[22-概念/05-安全/secrets-management.md|Kubernetes 密钥管理最佳实践]]
- [[22-概念/02-工作负载/pods.md|Pod]] — Secret 的消费者
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
