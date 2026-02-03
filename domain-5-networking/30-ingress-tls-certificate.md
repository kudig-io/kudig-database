# 130 - Ingress TLS 与证书管理

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **相关**: cert-manager v1.13+

---

## 一、TLS 基础概念

### 1.1 TLS 术语表

| 术语 | 英文全称 | 中文 | 说明 |
|-----|---------|------|------|
| **TLS** | Transport Layer Security | 传输层安全 | HTTPS 的加密协议 |
| **SSL** | Secure Sockets Layer | 安全套接层 | TLS 的前身，已弃用 |
| **Certificate** | Certificate | 证书 | 包含公钥和身份信息 |
| **Private Key** | Private Key | 私钥 | 与证书配对，用于解密 |
| **CA** | Certificate Authority | 证书颁发机构 | 签发证书的权威机构 |
| **CSR** | Certificate Signing Request | 证书签名请求 | 申请证书的请求文件 |
| **SAN** | Subject Alternative Name | 主体备用名称 | 证书支持的多个域名 |
| **Wildcard** | Wildcard Certificate | 通配符证书 | 支持子域的证书 (*.example.com) |
| **mTLS** | Mutual TLS | 双向 TLS | 客户端和服务端互相验证 |
| **OCSP** | Online Certificate Status Protocol | 在线证书状态协议 | 实时验证证书有效性 |
| **HSTS** | HTTP Strict Transport Security | HTTP 严格传输安全 | 强制 HTTPS 访问 |
| **SNI** | Server Name Indication | 服务器名称指示 | 单 IP 多证书支持 |

### 1.2 TLS 版本对比

| 版本 | 发布年份 | 安全性 | 性能 | 支持状态 | 建议 |
|-----|---------|-------|------|---------|------|
| SSL 2.0 | 1995 | ❌ 不安全 | - | 已弃用 | 禁用 |
| SSL 3.0 | 1996 | ❌ 不安全 | - | 已弃用 | 禁用 |
| TLS 1.0 | 1999 | ⚠️ 弱 | 低 | 已弃用 | 禁用 |
| TLS 1.1 | 2006 | ⚠️ 弱 | 低 | 已弃用 | 禁用 |
| TLS 1.2 | 2008 | ✅ 安全 | 中 | 当前标准 | 启用 |
| TLS 1.3 | 2018 | ✅ 最安全 | 高 | 最新标准 | 启用 |

### 1.3 Ingress TLS 工作流程

```
                                    TLS 握手流程
┌──────────────────────────────────────────────────────────────────────────────────────┐
│                                                                                       │
│  ┌──────────┐                                                      ┌──────────────┐  │
│  │  Client  │                                                      │  Ingress     │  │
│  │ (浏览器) │                                                      │  Controller  │  │
│  └────┬─────┘                                                      └──────┬───────┘  │
│       │                                                                   │          │
│       │  1. ClientHello (支持的 TLS 版本、加密套件、SNI 主机名)             │          │
│       │ ─────────────────────────────────────────────────────────────────>│          │
│       │                                                                   │          │
│       │                     2. ServerHello (选定 TLS 版本、加密套件)        │          │
│       │                        Certificate (服务器证书)                    │          │
│       │                        ServerKeyExchange (密钥交换参数)            │          │
│       │ <─────────────────────────────────────────────────────────────────│          │
│       │                                                                   │          │
│       │  3. 客户端验证服务器证书                                            │          │
│       │     - 检查证书是否过期                                             │          │
│       │     - 验证 CA 签名                                                 │          │
│       │     - 检查域名匹配 (CN/SAN)                                        │          │
│       │     - 检查证书吊销状态 (OCSP)                                      │          │
│       │                                                                   │          │
│       │  4. ClientKeyExchange (加密的预主密钥)                             │          │
│       │     ChangeCipherSpec                                              │          │
│       │     Finished                                                      │          │
│       │ ─────────────────────────────────────────────────────────────────>│          │
│       │                                                                   │          │
│       │                     5. ChangeCipherSpec                           │          │
│       │                        Finished                                   │          │
│       │ <─────────────────────────────────────────────────────────────────│          │
│       │                                                                   │          │
│       │  6. 加密的应用数据 (HTTPS)                                         │          │
│       │ <════════════════════════════════════════════════════════════════>│          │
│       │                                                                   │          │
└───────┴───────────────────────────────────────────────────────────────────┴──────────┘
```

---

## 二、Kubernetes TLS Secret

### 2.1 TLS Secret 结构

| 字段 | 说明 | 要求 |
|-----|------|------|
| `type` | Secret 类型 | 必须为 `kubernetes.io/tls` |
| `data.tls.crt` | 证书 (Base64 编码) | PEM 格式，包含完整证书链 |
| `data.tls.key` | 私钥 (Base64 编码) | PEM 格式，未加密 |
| `data.ca.crt` | CA 证书 (可选) | 用于 mTLS 客户端验证 |

### 2.2 创建 TLS Secret

```yaml
# 方式1: YAML 定义
apiVersion: v1
kind: Secret
metadata:
  name: example-tls
  namespace: default
type: kubernetes.io/tls
data:
  # base64 编码的证书
  tls.crt: LS0tLS1CRUdJTi...
  # base64 编码的私钥
  tls.key: LS0tLS1CRUdJTi...
```

```bash
# 方式2: kubectl 命令创建
kubectl create secret tls example-tls \
  --cert=./tls.crt \
  --key=./tls.key \
  -n default

# 方式3: 从 PFX/P12 文件创建 (需要先转换)
openssl pkcs12 -in certificate.pfx -out tls.crt -clcerts -nokeys
openssl pkcs12 -in certificate.pfx -out tls.key -nocerts -nodes
kubectl create secret tls example-tls --cert=tls.crt --key=tls.key

# 方式4: 创建包含 CA 的 Secret (用于 mTLS)
kubectl create secret generic mtls-secret \
  --from-file=tls.crt=./server.crt \
  --from-file=tls.key=./server.key \
  --from-file=ca.crt=./ca.crt \
  -n default

# 查看 Secret 内容
kubectl get secret example-tls -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout
```

### 2.3 证书链顺序

```
# 正确的证书链顺序 (从叶子到根)
-----BEGIN CERTIFICATE-----
[服务器证书 / Server Certificate]
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
[中间证书1 / Intermediate CA 1]
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
[中间证书2 / Intermediate CA 2]
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
[根证书 / Root CA] (可选)
-----END CERTIFICATE-----
```

---

## 三、Ingress TLS 配置

### 3.1 基础 TLS 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tls-ingress
  namespace: default
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls-secret
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

### 3.2 多域名 TLS 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: multi-domain-tls
  namespace: default
spec:
  ingressClassName: nginx
  tls:
  # 方式1: 多个单域名证书
  - hosts:
    - www.example.com
    secretName: www-tls
  - hosts:
    - api.example.com
    secretName: api-tls
  - hosts:
    - admin.example.com
    secretName: admin-tls
  
  # 方式2: 一个 SAN 证书覆盖多个域名
  # - hosts:
  #   - www.example.com
  #   - api.example.com
  #   - admin.example.com
  #   secretName: multi-domain-san-tls
  
  rules:
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: www-service
            port:
              number: 80
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
  - host: admin.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: admin-service
            port:
              number: 3000
```

### 3.3 通配符证书配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: wildcard-tls
  namespace: default
spec:
  ingressClassName: nginx
  tls:
  # 通配符证书 *.example.com
  - hosts:
    - "*.example.com"
    secretName: wildcard-tls-secret
  rules:
  - host: app1.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app1-service
            port:
              number: 80
  - host: app2.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app2-service
            port:
              number: 80
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

### 3.4 TLS 透传 (Passthrough)

```yaml
# TLS 透传 - 不在 Ingress 终止 TLS，直接转发到后端
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tls-passthrough
  namespace: default
  annotations:
    nginx.ingress.kubernetes.io/ssl-passthrough: "true"
    nginx.ingress.kubernetes.io/backend-protocol: "HTTPS"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - secure.example.com
    # 不需要 secretName，TLS 在后端处理
  rules:
  - host: secure.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: secure-backend
            port:
              number: 443
```

### 3.5 后端 HTTPS (Re-encrypt)

```yaml
# Ingress 终止前端 TLS，与后端建立新的 TLS 连接
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: reencrypt-tls
  namespace: default
  annotations:
    nginx.ingress.kubernetes.io/backend-protocol: "HTTPS"
    # 可选: 验证后端证书
    nginx.ingress.kubernetes.io/proxy-ssl-verify: "on"
    nginx.ingress.kubernetes.io/proxy-ssl-verify-depth: "2"
    nginx.ingress.kubernetes.io/proxy-ssl-secret: "default/backend-ca"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: frontend-tls
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: https-backend
            port:
              number: 443
```

---

## 四、mTLS 双向认证

### 4.1 mTLS 配置概览

| 验证方向 | 配置组件 | 证书 |
|---------|---------|------|
| **客户端验证服务端** | 浏览器/客户端 | 服务器证书 (CA 签发) |
| **服务端验证客户端** | Ingress Controller | 客户端证书 (指定 CA 签发) |

### 4.2 mTLS 完整配置

```yaml
# 1. 创建 CA Secret (用于验证客户端证书)
apiVersion: v1
kind: Secret
metadata:
  name: client-ca-secret
  namespace: default
type: Opaque
data:
  ca.crt: <base64-encoded-ca-cert>
---
# 2. 配置 mTLS Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: mtls-ingress
  namespace: default
  annotations:
    # 启用客户端证书验证
    nginx.ingress.kubernetes.io/auth-tls-secret: "default/client-ca-secret"
    # 验证模式: on (强制) / optional (可选) / optional_no_ca (可选但不验证 CA)
    nginx.ingress.kubernetes.io/auth-tls-verify-client: "on"
    # 证书链验证深度
    nginx.ingress.kubernetes.io/auth-tls-verify-depth: "2"
    # 是否将客户端证书传递给后端
    nginx.ingress.kubernetes.io/auth-tls-pass-certificate-to-upstream: "true"
    # 可选: 错误页面
    nginx.ingress.kubernetes.io/auth-tls-error-page: "https://error.example.com/cert-error"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - secure.example.com
    secretName: server-tls
  rules:
  - host: secure.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: secure-service
            port:
              number: 80
```

### 4.3 生成 mTLS 证书

```bash
# 1. 创建 CA 私钥和证书
openssl genrsa -out ca.key 4096
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt \
  -subj "/CN=My CA/O=MyOrg/C=CN"

# 2. 创建服务器证书
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr \
  -subj "/CN=secure.example.com/O=MyOrg/C=CN"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days 365 \
  -extfile <(echo "subjectAltName=DNS:secure.example.com")

# 3. 创建客户端证书
openssl genrsa -out client.key 2048
openssl req -new -key client.key -out client.csr \
  -subj "/CN=client1/O=MyOrg/C=CN"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out client.crt -days 365

# 4. 创建 Kubernetes Secrets
kubectl create secret tls server-tls --cert=server.crt --key=server.key
kubectl create secret generic client-ca-secret --from-file=ca.crt=ca.crt

# 5. 测试 mTLS
curl -v --cert client.crt --key client.key --cacert ca.crt https://secure.example.com
```

---

## 五、cert-manager 自动化证书管理

### 5.1 cert-manager 架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                            cert-manager 架构                                    │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │                              Issuers                                       │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐            │  │
│  │  │  ClusterIssuer  │  │     Issuer      │  │    Issuer       │            │  │
│  │  │  (Let's Encrypt)│  │  (Self-signed)  │  │   (CA/Vault)    │            │  │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘            │  │
│  └───────────┼────────────────────┼────────────────────┼─────────────────────┘  │
│              │                    │                    │                         │
│              └────────────────────┼────────────────────┘                         │
│                                   │                                              │
│                                   ▼                                              │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │                          Certificate CRD                                   │  │
│  │                                                                            │  │
│  │  apiVersion: cert-manager.io/v1                                           │  │
│  │  kind: Certificate                                                         │  │
│  │  spec:                                                                     │  │
│  │    secretName: app-tls                                                     │  │
│  │    issuerRef: letsencrypt-prod                                            │  │
│  │    dnsNames: [app.example.com]                                            │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                   │                                              │
│                                   │ cert-manager controller                      │
│                                   ▼                                              │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │                         Challenge Solvers                                  │  │
│  │  ┌────────────────────┐  ┌────────────────────┐  ┌────────────────────┐   │  │
│  │  │  HTTP-01 Solver    │  │  DNS-01 Solver     │  │  Self-signed       │   │  │
│  │  │  (Ingress 验证)    │  │  (DNS TXT 记录)    │  │  (直接签发)        │   │  │
│  │  └────────────────────┘  └────────────────────┘  └────────────────────┘   │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                   │                                              │
│                                   ▼                                              │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │                          TLS Secret                                        │  │
│  │                                                                            │  │
│  │  kind: Secret                                                              │  │
│  │  type: kubernetes.io/tls                                                   │  │
│  │  data:                                                                     │  │
│  │    tls.crt: ...                                                            │  │
│  │    tls.key: ...                                                            │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                   │                                              │
│                                   ▼                                              │
│  ┌───────────────────────────────────────────────────────────────────────────┐  │
│  │                            Ingress                                         │  │
│  │                                                                            │  │
│  │  spec:                                                                     │  │
│  │    tls:                                                                    │  │
│  │    - hosts: [app.example.com]                                             │  │
│  │      secretName: app-tls  <── 自动关联                                     │  │
│  └───────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 安装 cert-manager

```bash
# 方式1: Helm 安装
helm repo add jetstack https://charts.jetstack.io
helm repo update
helm install cert-manager jetstack/cert-manager \
  --namespace cert-manager \
  --create-namespace \
  --set installCRDs=true \
  --set prometheus.enabled=true

# 方式2: kubectl 安装
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.14.0/cert-manager.yaml

# 验证安装
kubectl get pods -n cert-manager
kubectl get crds | grep cert-manager
```

### 5.3 配置 Issuer/ClusterIssuer

```yaml
# Let's Encrypt 生产环境 ClusterIssuer
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    # Let's Encrypt 生产服务器
    server: https://acme-v02.api.letsencrypt.org/directory
    # 邮箱地址（用于证书过期通知）
    email: admin@example.com
    # 存储 ACME 账户私钥的 Secret
    privateKeySecretRef:
      name: letsencrypt-prod-account-key
    # 验证方式
    solvers:
    # HTTP-01 验证（推荐用于 Ingress）
    - http01:
        ingress:
          class: nginx
    # DNS-01 验证（用于通配符证书）
    # - dns01:
    #     cloudflare:
    #       email: admin@example.com
    #       apiTokenSecretRef:
    #         name: cloudflare-api-token
    #         key: api-token
---
# Let's Encrypt 测试环境 ClusterIssuer
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-staging
spec:
  acme:
    server: https://acme-staging-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-staging-account-key
    solvers:
    - http01:
        ingress:
          class: nginx
---
# 自签名 Issuer
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: selfsigned-issuer
spec:
  selfSigned: {}
---
# CA Issuer（使用自己的 CA）
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: ca-issuer
spec:
  ca:
    secretName: ca-key-pair
```

### 5.4 配置 Certificate

```yaml
# 方式1: 显式创建 Certificate 资源
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: app-tls-cert
  namespace: default
spec:
  # 生成的 Secret 名称
  secretName: app-tls
  # 证书有效期
  duration: 2160h  # 90 天
  # 提前多久续期
  renewBefore: 360h  # 15 天
  # 使用的 Issuer
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
    group: cert-manager.io
  # 证书通用名称
  commonName: app.example.com
  # 证书包含的域名
  dnsNames:
  - app.example.com
  - www.app.example.com
  # 可选: 私钥配置
  privateKey:
    algorithm: RSA
    size: 2048
    # 或使用 ECDSA
    # algorithm: ECDSA
    # size: 256
  # 可选: 用途
  usages:
  - server auth
  - client auth
---
# 方式2: 通过 Ingress 注解自动创建证书
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: auto-tls-ingress
  namespace: default
  annotations:
    # 指定 ClusterIssuer
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    # 或使用 Issuer
    # cert-manager.io/issuer: "letsencrypt-prod"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls  # cert-manager 自动创建此 Secret
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

### 5.5 通配符证书 (DNS-01 验证)

```yaml
# Cloudflare DNS-01 验证
apiVersion: v1
kind: Secret
metadata:
  name: cloudflare-api-token
  namespace: cert-manager
type: Opaque
stringData:
  api-token: <your-cloudflare-api-token>
---
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod-dns
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-prod-dns-account-key
    solvers:
    - dns01:
        cloudflare:
          apiTokenSecretRef:
            name: cloudflare-api-token
            key: api-token
---
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: wildcard-cert
  namespace: default
spec:
  secretName: wildcard-tls
  duration: 2160h
  renewBefore: 360h
  issuerRef:
    name: letsencrypt-prod-dns
    kind: ClusterIssuer
  commonName: "*.example.com"
  dnsNames:
  - "*.example.com"
  - "example.com"
```

### 5.6 DNS-01 验证器支持

| DNS 提供商 | 配置字段 | 说明 |
|-----------|---------|------|
| **Cloudflare** | `cloudflare` | API Token 或 API Key |
| **阿里云 DNS** | `alidns` | AccessKey ID/Secret |
| **AWS Route53** | `route53` | IAM 角色或 Access Key |
| **Google Cloud DNS** | `cloudDNS` | Service Account |
| **Azure DNS** | `azureDNS` | Service Principal |
| **腾讯云 DNS** | `tencentcloud` (webhook) | SecretId/SecretKey |
| **ACME DNS** | `acmeDNS` | 通用 ACME DNS 服务 |
| **Webhook** | `webhook` | 自定义 DNS 提供商 |

```yaml
# 阿里云 DNS 配置示例
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-alidns
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-alidns-account-key
    solvers:
    - dns01:
        webhook:
          groupName: acme.yourcompany.com
          solverName: alidns
          config:
            regionId: cn-hangzhou
            accessKeyIdRef:
              name: alidns-credentials
              key: access-key-id
            accessKeySecretRef:
              name: alidns-credentials
              key: access-key-secret
```

---

## 六、证书运维操作

### 6.1 证书查看与验证

```bash
# 查看证书 Secret
kubectl get secrets -l cert-manager.io/certificate-name

# 查看证书详情
kubectl describe certificate app-tls-cert

# 查看证书内容
kubectl get secret app-tls -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout

# 验证证书有效期
kubectl get secret app-tls -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -dates -noout

# 验证证书链
kubectl get secret app-tls -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl verify -CAfile ca.crt

# 检查 Certificate 资源状态
kubectl get certificate -A
kubectl describe certificate app-tls-cert -n default

# 查看证书请求
kubectl get certificaterequest -A
kubectl describe certificaterequest app-tls-cert-xxxxx

# 查看 ACME Challenge
kubectl get challenges -A
kubectl describe challenge app-tls-cert-xxxxx
```

### 6.2 证书续期

```bash
# 手动触发证书续期
kubectl cert-manager renew app-tls-cert -n default

# 批量续期所有证书
kubectl cert-manager renew --all -n default

# 查看续期状态
kubectl describe certificate app-tls-cert | grep -A5 "Status:"

# 监控证书过期时间
kubectl get certificate -A -o custom-columns=\
'NAMESPACE:.metadata.namespace,NAME:.metadata.name,READY:.status.conditions[?(@.type=="Ready")].status,EXPIRY:.status.notAfter,RENEWAL:.status.renewalTime'
```

### 6.3 证书问题排查

| 问题 | 症状 | 诊断 | 解决方案 |
|-----|------|------|---------|
| **证书未创建** | Secret 不存在 | `kubectl describe certificate` | 检查 Issuer 配置 |
| **HTTP-01 验证失败** | Challenge 卡住 | `kubectl describe challenge` | 检查 Ingress 是否可访问 |
| **DNS-01 验证失败** | Challenge 失败 | 检查 DNS TXT 记录 | 验证 DNS API 权限 |
| **证书不受信任** | 浏览器警告 | 检查证书链 | 确保包含中间证书 |
| **证书过期** | HTTPS 错误 | 检查证书日期 | 手动触发续期 |
| **私钥不匹配** | TLS 握手失败 | 比较证书和私钥 | 重新签发证书 |

```bash
# 调试 cert-manager
kubectl logs -n cert-manager -l app=cert-manager -f

# 检查 Certificate 事件
kubectl get events --field-selector involvedObject.name=app-tls-cert

# 验证 Ingress 控制器证书配置
kubectl exec -n ingress-nginx <pod> -- cat /etc/nginx/nginx.conf | grep -A5 "ssl_certificate"

# 测试 HTTPS 连接
curl -vI https://app.example.com 2>&1 | grep -E "SSL|certificate|subject|issuer|expire"

# 检查证书链
echo | openssl s_client -connect app.example.com:443 -servername app.example.com 2>/dev/null | openssl x509 -text -noout
```

---

## 七、TLS 最佳实践

### 7.1 配置检查清单

| 检查项 | 说明 | 状态 |
|-------|------|------|
| 使用 TLS 1.2+ | 禁用 TLS 1.0/1.1 | ☐ |
| 强制 HTTPS | 配置 SSL 重定向 | ☐ |
| 启用 HSTS | 防止降级攻击 | ☐ |
| 证书链完整 | 包含中间证书 | ☐ |
| 自动续期 | 配置 cert-manager | ☐ |
| 监控过期 | 配置告警 | ☐ |
| 安全加密套件 | 禁用弱加密 | ☐ |
| OCSP Stapling | 加速验证 | ☐ |

### 7.2 推荐的 TLS 配置

```yaml
# ConfigMap 全局 TLS 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
data:
  # TLS 版本
  ssl-protocols: "TLSv1.2 TLSv1.3"
  
  # 加密套件 (按优先级排序)
  ssl-ciphers: "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384"
  
  # 优先使用服务器加密套件
  ssl-prefer-server-ciphers: "true"
  
  # SSL 会话缓存
  ssl-session-cache: "true"
  ssl-session-cache-size: "50m"
  ssl-session-timeout: "10m"
  
  # 禁用 Session Tickets (提高前向保密性)
  ssl-session-tickets: "false"
  
  # HSTS
  hsts: "true"
  hsts-max-age: "31536000"
  hsts-include-subdomains: "true"
  hsts-preload: "true"
  
  # OCSP Stapling
  enable-ocsp: "true"
```

### 7.3 证书监控告警

```yaml
# Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cert-manager-alerts
  namespace: monitoring
spec:
  groups:
  - name: cert-manager
    rules:
    # 证书即将过期告警 (14 天内)
    - alert: CertificateExpiringSoon
      expr: |
        certmanager_certificate_expiration_timestamp_seconds - time() < 1209600
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "证书即将过期: {{ $labels.name }}"
        description: "证书 {{ $labels.namespace }}/{{ $labels.name }} 将在 {{ $value | humanizeDuration }} 后过期"
    
    # 证书即将过期紧急告警 (7 天内)
    - alert: CertificateExpiringCritical
      expr: |
        certmanager_certificate_expiration_timestamp_seconds - time() < 604800
      for: 1h
      labels:
        severity: critical
      annotations:
        summary: "证书即将过期 (紧急): {{ $labels.name }}"
        description: "证书 {{ $labels.namespace }}/{{ $labels.name }} 将在 {{ $value | humanizeDuration }} 后过期，请立即处理！"
    
    # 证书签发失败
    - alert: CertificateNotReady
      expr: |
        certmanager_certificate_ready_status{condition="False"} == 1
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "证书未就绪: {{ $labels.name }}"
        description: "证书 {{ $labels.namespace }}/{{ $labels.name }} 未处于就绪状态"
```

---

**TLS 配置原则**: 使用最新 TLS 版本 → 强制 HTTPS → 自动化证书管理 → 完善监控告警

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
