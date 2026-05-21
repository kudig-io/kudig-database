---
title: ServiceAccount 密钥对源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- apiserver
- kubelet
- controller-manager
last_updated: '2026-05-18'
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 管理员
- 安全工程师
- 应用开发者
estimated_read_time: 5min
intent_queries:
- Kubernetes ServiceAccount 密钥对 sa.key sa.pub JWT 签名
- ServiceAccount Token 签发与验证原理
- API Server Controller Manager 密钥共享配置
- SA 密钥轮换挑战 全部 Token 失效
- ServiceAccount TokenRequest API 短期 Token
trigger_keywords:
- ServiceAccount
- sa.key
- sa.pub
- JWT
- Token
- service-account-key-file
- service-account-private-key-file
- TokenRequest
- 短期 Token
- Bound Object
prerequisites:
- kubectl-basics
- pod-lifecycle
related_domains:
- domain-01-cluster-fundamentals
- domain-05-security-compliance
related_topics:
- cluster-cert/pki-architecture
- cluster-cert/apiserver-cert-flags
- cluster-cert/cert-rotation
---

# ServiceAccount 密钥对源码分析

## 概述

ServiceAccount 密钥对（sa.pub / sa.key）是 Kubernetes 集群中用于 ServiceAccount Token 签发与验证的核心凭证。不同于 TLS 证书体系，SA 密钥对使用 **JWT（JSON Web Token）** 机制，为 Pod 提供访问 API Server 的身份凭证。

---

## 源码路径

- **SA 密钥生成**: `cmd/kubeadm/app/phases/certs/certs.go`
- **JWT 签名**: `pkg/serviceaccount/jwt.go`
- **Token 验证**: `pkg/serviceaccount/claims.go`
- **Token 控制器**: `pkg/controlplane/controller.go`
- **Legacy Token**: `pkg/serviceaccount/legacy.go`

---

## SA 密钥对与 TLS 证书的区别

| 特性 | SA 密钥对 | TLS 证书 |
|-----|----------|---------|
| 格式 | RSA 私钥 + 公钥 | X.509 证书 + 私钥 |
| 用途 | JWT 签名/验证 | TLS 握手 |
| 有效期 | 无内置过期 | 1 年（默认） |
| 轮换难度 | **高**（需重新签发所有 Token） | 低 |
| 存储 | `sa.pub`, `sa.key` | `.crt`, `.key` |

---

## SA 密钥生成

### 1. kubeadm 生成 SA 密钥

```go
// cmd/kubeadm/app/phases/certs/certs.go
var KubeadmCertServiceAccount = &KubeadmCert{
    Name:     "sa",
    LongName: "service account signing key",
    BaseName: "sa",
    // 注意：这不是 X.509 证书，而是原始 RSA 密钥对
}

// 实际生成逻辑
func CreateServiceAccountKey(pkiDir string) (crypto.Signer, error) {
    // 生成 RSA 2048 私钥
    privateKey, err := rsa.GenerateKey(cryptorand.Reader, 2048)
    if err != nil {
        return nil, err
    }
    
    // 提取公钥
    publicKey := &privateKey.PublicKey
    
    // 写入私钥: sa.key
    if err := keyutil.WriteKey(filepath.Join(pkiDir, "sa.key"), keyToPem(privateKey)); err != nil {
        return nil, err
    }
    
    // 写入公钥: sa.pub
    if err := keyutil.WriteKey(filepath.Join(pkiDir, "sa.pub"), keyToPem(publicKey)); err != nil {
        return nil, err
    }
    
    return privateKey, nil
}
```

**存储路径**：
- 私钥：`/etc/kubernetes/pki/sa.key` — 用于签名 JWT
- 公钥：`/etc/kubernetes/pki/sa.pub` — 用于验证 JWT

### 2. 密钥格式

```bash
# sa.key (PKCS#1 RSA PRIVATE KEY)
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA0Z3VS5JJcds3xfn/ygWyF8PbnGy0AHB7MAPK7/Eg6I3qSqKb
...
-----END RSA PRIVATE KEY-----

# sa.pub (PUBLIC KEY)
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0Z3VS5JJcds3xfn/ygWy
...
-----END PUBLIC KEY-----
```

---

## SA Token 的签发与验证

### 1. Token 签发（Controller Manager）

```go
// pkg/serviceaccount/jwt.go
func (j *jwtTokenGenerator) GenerateToken(claims *jwt.Claims, secret *v1.Secret) (string, error) {
    // 使用 sa.key (RSA 私钥) 对 JWT 签名
    token := jwt.NewWithClaims(jwt.SigningMethodRS256, claims)
    return token.SignedString(j.privateKey)
}
```

**Token 结构**：
```
<header>.<payload>.<signature>

Header: {"alg":"RS256","kid":""}
Payload: {
  "iss": "kubernetes/serviceaccount",
  "sub": "system:serviceaccount:default:my-sa",
  "namespace": "default",
  "name": "my-sa",
  "aud": ["https://kubernetes.default.svc"],
  "exp": 1735689600,
  "iat": 1735686000
}
Signature: RSA-SHA256(header + "." + payload)
```

### 2. Token 验证（API Server）

```go
// pkg/serviceaccount/claims.go
func (v *validator) Validate(ctx context.Context, tokenData string) (*user.DefaultInfo, error) {
    // 1. 解析 JWT (不验证签名)
    token, err := jwt.Parse(tokenData, func(token *jwt.Token) (interface{}, error) {
        // 返回 sa.pub 用于验证 RSA 签名
        return v.publicKeys, nil
    })
    
    // 2. 验证签名
    if !token.Valid {
        return nil, errors.New("invalid token signature")
    }
    
    // 3. 验证 claims
    claims := token.Claims.(jwt.MapClaims)
    if claims["iss"] != "kubernetes/serviceaccount" {
        return nil, errors.New("invalid issuer")
    }
    
    // 4. 返回用户信息
    return &user.DefaultInfo{
        Name:   claims["sub"].(string),
        Groups: []string{"system:serviceaccounts", "system:serviceaccounts:" + claims["namespace"].(string)},
    }, nil
}
```

---

## 密钥不匹配故障诊断

### 故障现象：API Server 与 Controller Manager 使用不同密钥对

当 API Server 的 `--service-account-key-file` 与 Controller Manager 的 `--service-account-private-key-file` 不匹配时：

```
Controller Manager 使用 sa.key_A 签名 Token
           │
           ▼
      Pod 携带 Token
           │
           ▼
    API Server 使用 sa.pub_B 验证
           │
           ▼
    验证失败 → Unauthorized
```

**故障症状**：
- 所有 Pod 无法访问 API Server：`Unauthorized`
- 新创建的 Pod 也无法启动（如果需要访问 API）
- kube-controller-manager 日志正常，API Server 日志出现大量 `invalid token` 错误

**根因排查**：
```bash
# 1. 检查 API Server 使用的公钥
ps aux | grep kube-apiserver | grep service-account-key-file
# 输出: --service-account-key-file=/etc/kubernetes/pki/sa.pub

# 2. 检查 Controller Manager 使用的私钥
ps aux | grep kube-controller-manager | grep service-account-private-key-file
# 输出: --service-account-private-key-file=/etc/kubernetes/pki/sa.key

# 3. 验证两者是否配对
openssl rsa -in /etc/kubernetes/pki/sa.key -pubout -out /tmp/sa.key.pub
diff /etc/kubernetes/pki/sa.pub /tmp/sa.key.pub
# 如果 diff 有输出，说明不匹配

# 4. 检查 kubeconfig 中的证书是否也受影响（某些部署方式）
```

**修复**：
```bash
# 确保两个组件使用同一密钥对
# 方式 1: 从私钥重新导出公钥
sudo openssl rsa -in /etc/kubernetes/pki/sa.key -pubout -out /etc/kubernetes/pki/sa.pub
sudo systemctl restart kubelet  # 重启控制面

# 方式 2: 如果 sa.pub 是正确的，导出匹配的私钥（不太可能）
# 方式 3: 重新生成 SA 密钥对（所有现有 Token 失效）
sudo openssl genrsa -out /etc/kubernetes/pki/sa.key 2048
sudo openssl rsa -in /etc/kubernetes/pki/sa.key -pubout -out /etc/kubernetes/pki/sa.pub
sudo systemctl restart kubelet
# 然后删除所有 Pod 使其重新获取 Token
kubectl delete pods --all -n <namespace>
```

---

## 组件间的密钥共享

### API Server 与 Controller Manager 的密钥配置

```
┌─────────────────────┐          ┌─────────────────────────────┐
│  Controller Manager │          │       API Server            │
│                     │          │                             │
│  --service-account- │          │  --service-account-key-file │
│   private-key-file  │          │    =/etc/kubernetes/pki/    │
│   =/etc/kubernetes/ │          │     sa.pub                  │
│    pki/sa.key       │          │                             │
│                     │          │  使用公钥验证 Token         │
│  使用私钥签名 Token │          │                             │
└─────────────────────┘          └─────────────────────────────┘
```

**启动参数**：
```bash
# Controller Manager
--service-account-private-key-file=/etc/kubernetes/pki/sa.key

# API Server
--service-account-key-file=/etc/kubernetes/pki/sa.pub
```

**关键约束**：
- 两个组件必须使用**配对的**密钥对
- 如果密钥不匹配，所有 ServiceAccount Token 验证将失败
- Pod 将无法访问 API Server

---

## SA Token 的版本演进

### v1.24 之前的 Legacy Token

```yaml
# Secret 中存储的 JWT Token (永不过期)
apiVersion: v1
kind: Secret
metadata:
  name: default-token-abc123
  annotations:
    kubernetes.io/service-account.name: "default"
type: kubernetes.io/service-account-token
data:
  ca.crt: <base64-ca>
  namespace: <base64-namespace>
  token: <base64-jwt>  # 使用 sa.key 签名，永不过期
```

### v1.20+ 的 ServiceAccountIssuerDiscovery（OIDC 兼容）

```go
// API Server 启动参数
--service-account-issuer=https://kubernetes.default.svc.cluster.local
--service-account-jwks-uri=https://kubernetes.default.svc.cluster.local/openid/v1/jwks
--service-account-signing-key-file=/etc/kubernetes/pki/sa.key
```

**特性说明**：
- API Server 暴露 OIDC 发现端点 (`/.well-known/openid-configuration`)
- 外部系统（如 Vault、AWS IAM）可通过 JWKS 验证 SA Token
- 支持 **多个公钥文件**：`--service-account-key-file` 可指定多次
- 轮换 SA 密钥时，新旧公钥可同时存在于 JWKS 中，实现零停机轮换

### v1.24+ 的 TokenRequest API（短期 Token）

```go
// pkg/serviceaccount/claims.go
func (v *validator) NewValidateFunc() ValidateTokenFunc {
    return func(ctx context.Context, tokenData string) (*user.DefaultInfo, error) {
        // 支持短期 Token（1 小时默认有效期）
        // 通过 TokenRequest API 动态签发
    }
}
```

**短期 Token 特性**：
- 默认有效期：**1 小时**
- 由 API Server 使用 sa.key 直接签发
- 无需持久化 Secret
- 支持 **Bound Object**（绑定到 Pod/Secret）

---

## SA 密钥轮换

### 轮换的挑战

SA 密钥轮换是 Kubernetes 证书体系中最困难的操作，因为：

1. **所有已签发 Token 同时失效** — 使用旧 sa.key 签发的 Token 将无法通过新 sa.pub 验证
2. **需要滚动更新所有 Pod** — 让所有 Pod 重新获取新 Token
3. **API Server 和 Controller Manager 必须同步切换** — 否则签发和验证不匹配

### 手动轮换步骤

```bash
# 1. 备份当前密钥
sudo cp /etc/kubernetes/pki/sa.key /etc/kubernetes/pki/sa.key.backup
sudo cp /etc/kubernetes/pki/sa.pub /etc/kubernetes/pki/sa.pub.backup

# 2. 生成新密钥对
openssl genrsa -out /etc/kubernetes/pki/sa.key 2048
openssl rsa -in /etc/kubernetes/pki/sa.key -pubout -out /etc/kubernetes/pki/sa.pub

# 3. 同时更新所有控制面节点
# 将新 sa.key / sa.pub 同步到所有 master 节点的 /etc/kubernetes/pki/

# 4. 重启 Controller Manager 和 API Server
# 确保两者使用新密钥对
sudo systemctl restart kubelet

# 5. 删除所有 ServiceAccount Secret，强制重新生成 Token
kubectl delete secret --all -n <namespace>

# 6. 滚动重启所有 Pod，获取新 Token
kubectl rollout restart deployment/<name> -n <namespace>

# 7. 验证
kubectl auth can-i --list --as=system:serviceaccount:default:default
```

---

## SA 密钥的安全最佳实践

### 1. 使用短期 Token（推荐）

```yaml
# Pod 中挂载短期 Token
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: app
    volumeMounts:
    - name: token
      mountPath: /var/run/secrets/kubernetes.io/serviceaccount
  volumes:
  - name: token
    projected:
      sources:
      - serviceAccountToken:
          path: token
          expirationSeconds: 3600  # 1 小时
```

### 2. 定期轮换 SA 密钥

- 建议频率：**每年一次**
- 在维护窗口执行
- 提前通知所有应用团队

### 3. 监控 Token 验证失败

```bash
# API Server 日志中监控 Token 验证失败
kubectl logs -n kube-system kube-apiserver-<node> | grep "invalid token"

# 监控 metrics
# apiserver_authentication_token_cache_active_fetch_count
# authentication_duration_seconds
```

### 4. 使用外部 KMS 保护 sa.key

```bash
# 将 sa.key 存储在硬件安全模块 (HSM) 或云 KMS 中
# 配置 API Server 和 Controller Manager 使用 KMS 进行签名/验证
```

---

## 验证 SA 密钥一致性

```bash
# 1. 验证公钥是否从私钥派生
openssl rsa -in /etc/kubernetes/pki/sa.key -pubout -out /tmp/sa.pub.check
diff /etc/kubernetes/pki/sa.pub /tmp/sa.pub.check

# 2. 查看 API Server 使用的公钥
ps aux | grep kube-apiserver | grep service-account-key-file

# 3. 查看 Controller Manager 使用的私钥
ps aux | grep kube-controller-manager | grep service-account-private-key-file

# 4. 验证 Token 签名
TOKEN=$(kubectl get secret default-token -o jsonpath='{.data.token}' | base64 -d)
echo "$TOKEN" | cut -d. -f2 | base64 -d | jq .

# 5. 手动验证 JWT 签名
# 提取 JWT 的 header.payload，使用 sa.pub 验证 signature
```

---

## 故障排查

| 现象 | 根因 | 解决 |
|-----|------|------|
| Pod 无法访问 API Server (`Unauthorized`) | sa.key / sa.pub 不匹配 | 确认两组件使用同一密钥对 |
| 所有 ServiceAccount Token 失效 | SA 密钥被替换但未同步 | 回滚密钥或重新签发所有 Token |
| Token 验证性能下降 | API Server 公钥缓存失效 | 检查 `service-account-key-file` 配置 |
| Legacy Token 安全风险 | 永不过期 Token 泄露 | 升级到短期 Token，删除 Legacy Secret |

---

## 进阶：多集群 SA 密钥管理

### 场景：多集群环境下的 SA 密钥一致性

在多集群环境中，如果使用相同的镜像和配置，需要确保各集群的 SA 密钥不同：

```
┌─────────────────────────────────────────────────────────────┐
│              多集群 SA 密钥隔离设计                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  问题：同一镜像在不同集群部署时使用相同的 ServiceAccount     │
│        如果 SA 密钥相同，一个集群的 Pod Token 可用于         │
│        其他集群的 API Server 验证                           │
│                                                              │
│  解决：                                                      │
│  1. 每个集群使用独立的 SA 密钥对                             │
│  2. 通过 Secret 注入而非镜像内置                            │
│  3. 集群级别使用云 KMS 加密存储                              │
│                                                              │
│  配置示例：                                                   │
│  apiVersion: v1                                              │
│  kind: Pod                                                  │
│  spec:                                                       │
│    serviceAccountName: my-sa                                 │
│    volumes:                                                  │
│    - name: sa-token                                          │
│      projected:                                             │
│        sources:                                              │
│        - serviceAccountToken:                                │
│            path: token                                       │
│            expirationSeconds: 3600                           │
│            audience: kubernetes.default.svc.cluster.local    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Related

- [[domain-19-landscape-references/topic-index/cert-index|Certificate / TLS 证书知识图谱索引]]
