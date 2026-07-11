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

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/secrets-management.md|Kubernetes 密钥管理最佳实践]]
- [[概念/pods.md|Pod]] — Secret 的消费者
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
