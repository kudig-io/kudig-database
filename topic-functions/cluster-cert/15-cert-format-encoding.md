# 证书格式与编码详解

## 概述

Kubernetes 集群证书虽然以 `.crt` 和 `.key` 文件形式存在，但其底层涉及多种编码标准和数据格式。理解 PEM、DER、X.509 v3 及 ASN.1 的关系，是深入排查证书异常和手动签发证书的基础。

---

## 核心概念关系

```
┌─────────────────────────────────────────────────────────────────┐
│                    证书格式层次关系                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  X.509 v3  (逻辑标准)                                            │
│  ├─ 定义了证书字段：Subject, Issuer, Validity, PublicKey, ...   │
│  ├─ 定义了标准扩展：BasicConstraints, KeyUsage, SAN, ...        │
│  └─ 用 ASN.1 (Abstract Syntax Notation One) 描述数据结构        │
│                                                                  │
│  ASN.1 定义 ──► DER 编码 (Distinguished Encoding Rules)         │
│  │                      └─ 二进制格式，不可读                    │
│  │                                                              │
│  └─► PEM 编码 (Privacy Enhanced Mail)                           │
│        └─ Base64(DER) + ASCII 头尾标记，人类可读                 │
│                                                                  │
│  Kubernetes 中使用的 .crt/.key 文件均为 PEM 格式                 │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## PEM 格式详解

### 1. 文件扩展名约定

| 扩展名 | 内容 | PEM 标记头 |
|-------|------|-----------|
| `.crt` / `.pem` | 证书 | `-----BEGIN CERTIFICATE-----` |
| `.key` | 私钥 (PKCS#1) | `-----BEGIN RSA PRIVATE KEY-----` |
| `.key` | 私钥 (PKCS#8) | `-----BEGIN PRIVATE KEY-----` |
| `.pub` | 公钥 | `-----BEGIN PUBLIC KEY-----` |
| `.csr` | 证书签名请求 | `-----BEGIN CERTIFICATE REQUEST-----` |
| `.p12` / `.pfx` | PKCS#12 容器 | 二进制，非 PEM |

### 2. PEM 文件结构

```
-----BEGIN CERTIFICATE-----          ← 头标记
MIIDXTCCAkWgAwIBAgIJAJC1HiIAZAiUMA0GCSqGSIb3Qa3BajELMAkGA1UEBhMC
U0cxDzANBgNVBAgTBlNpbmdhcG9yZTEPMA0GA1UEBxMGU2luZ2Fwb3JlMRMwEQYD
... (Base64 编码的 DER 数据) ...
Ja61Po0Yq4A6H8b+uPQYPnVNLzlZmxXz7ljsedQrvqnselmIm4=          ← 无空行
-----END CERTIFICATE-----            ← 尾标记
```

**关键特征**：
- 纯文本格式，可直接用 `cat` / `vim` 查看
- Base64 编码每行 64 字符（标准）
- 一个 PEM 文件可包含多个证书（证书链）
- Kubernetes 中所有证书文件均为 PEM 格式

### 3. 证书链的 PEM 表示

```
-----BEGIN CERTIFICATE-----
... (服务端证书) ...
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
... (中间 CA 证书) ...
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
... (根 CA 证书) ...
-----END CERTIFICATE-----
```

**Kubernetes 中的应用**：
- `/etc/kubernetes/pki/ca.crt` — 单个 CA 证书
- `/etc/kubernetes/pki/apiserver.crt` — 单个服务端证书
- 某些场景下（如 ingress TLS Secret）`tls.crt` 可能包含完整证书链

---

## DER 格式详解

### 1. DER 是 ASN.1 的二进制编码

```go
// Go 标准库中的证书解析
// crypto/x509/x509.go
func ParseCertificate(der []byte) (*Certificate, error) {
    // 输入：DER 编码的二进制数据
    // 输出：解析后的 x509.Certificate 结构体
}
```

**DER 特征**：
- 严格的二进制格式，每条数据只有一种编码方式
- 不可直接阅读，需要工具转换
- 是 PEM 编码的底层数据（PEM = Base64(DER) + 头尾标记）

### 2. PEM 与 DER 互转

```bash
# PEM → DER
cp /etc/kubernetes/pki/ca.crt ca.pem
openssl x509 -in ca.pem -out ca.der -outform DER

# DER → PEM
openssl x509 -in ca.der -inform DER -out ca.pem

# 从 kubeconfig 提取 Base64 证书并解码为 PEM
kubectl config view --raw \
  -o jsonpath='{.clusters[0].cluster.certificate-authority-data}' | \
  base64 -d > ca-from-kubeconfig.crt
# 结果直接是 PEM 格式，无需转换
```

---

## X.509 v3 扩展字段

### 1. Kubernetes 证书中常见的扩展

```bash
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text
```

**典型输出解析**：

```
Certificate:
    Data:
        Version: 3 (0x2)                          ← X.509 v3
        Serial Number: 123456789 (0x75bcd15)       ← 序列号
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN = kubernetes-ca                 ← 签发者
        Validity
            Not Before: Jan 15 08:30:00 2025 GMT   ← 生效时间
            Not After : Jan 15 08:30:00 2026 GMT   ← 过期时间
        Subject: CN = kube-apiserver               ← 证书主体
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (2048 bit)          ← 密钥长度
        X509v3 extensions:
            X509v3 Key Usage: critical              ← 密钥用途
                Digital Signature, Key Encipherment
            X509v3 Extended Key Usage:              ← 扩展密钥用途
                TLS Web Server Authentication
            X509v3 Basic Constraints: critical      ← 基本约束
                CA:FALSE
            X509v3 Subject Alternative Name:         ← SAN
                DNS:kubernetes, DNS:kubernetes.default,
                DNS:kubernetes.default.svc,
                IP Address:10.96.0.1, IP Address:192.168.1.10
```

### 2. 各扩展字段在 Kubernetes 中的意义

| 扩展字段 | OID | Kubernetes 应用 |
|---------|-----|----------------|
| `Key Usage` | 2.5.29.15 | CA 证书必须有 `Certificate Sign`；服务端证书需 `Digital Signature, Key Encipherment` |
| `Extended Key Usage` | 2.5.29.37 | `ServerAuth`（API Server、etcd）、`ClientAuth`（kubelet、controller-manager） |
| `Basic Constraints` | 2.5.29.19 | CA 证书 `CA:TRUE`；终端实体证书 `CA:FALSE` |
| `Subject Alternative Name` | 2.5.29.17 | API Server 必须包含所有访问入口的 DNS/IP |
| `Authority Key Identifier` | 2.5.29.35 | 标识签发证书的 CA 公钥，用于构建证书链 |
| `Subject Key Identifier` | 2.5.29.14 | 标识证书本身的公钥，用于证书链匹配 |

---

## Kubernetes 证书的特殊格式

### 1. kubeconfig 中的 Base64 内嵌证书

```yaml
# kubeconfig 中的证书不是 PEM，而是 Base64(PEM)
# 即：Base64(Base64(DER) + 头尾标记)
users:
- name: kubernetes-admin
  user:
    client-certificate-data: LS0tLS1CRUdJTi...  # ← Base64(PEM)
```

**解码过程**：
```bash
# Base64 解码 → 得到 PEM
echo "LS0tLS1CRUdJTi..." | base64 -d
# 输出:
# -----BEGIN CERTIFICATE-----
# ...
# -----END CERTIFICATE-----
```

### 2. CSR (Certificate Signing Request) 格式

```bash
# 查看 Kubernetes CSR 资源中的请求内容
kubectl get csr <csr-name> -o jsonpath='{.spec.request}' | base64 -d

# 解析 CSR 内容
kubectl get csr <csr-name> -o jsonpath='{.spec.request}' | base64 -d | openssl req -noout -text
```

**CSR 结构**：
```
Certificate Request:
    Data:
        Version: 1 (0x0)
        Subject: O = system:nodes, CN = system:node:worker-1
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
        Attributes:
            a0:0
    Signature Algorithm: sha256WithRSAEncryption
```

**注意**：CSR 本身不包含 SAN（v1.0 格式），但 Kubernetes 的 CSR API 通过额外的元数据字段支持传递 SAN。

---

## 证书指纹与哈希

### 1. 指纹算法

```bash
# SHA-256 指纹（最常用）
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -fingerprint -sha256

# SHA-1 指纹（旧系统兼容）
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -fingerprint -sha1
```

**指纹计算**：
```
Fingerprint = SHA256(DER_encoded_certificate)
```

### 2. 在 Kubernetes 中的应用

```bash
# kubeadm join 使用的 discovery-token-ca-cert-hash
# 计算方式: SHA256(ca.crt) 的前 16 字节
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -pubkey | \
  openssl rsa -pubin -outform DER | \
  sha256sum | head -c 64

# 或者直接使用 kubeadm 提供的命令
kubeadm token create --print-join-command
```

---

## 编码相关故障排查

| 问题 | 现象 | 根因 | 解决 |
|-----|------|------|------|
| `unable to load certificate` | openssl 无法解析 | 文件是 DER 格式但按 PEM 读取 | `openssl -inform DER` |
| `bad base64 decode` | kubectl kubeconfig 解析失败 | `certificate-authority-data` 包含换行符或空格 | 清理 Base64 字符串 |
| `PEM 头尾标记不匹配` | 解析失败 | 手动编辑时破坏了标记 | 确保证书内容完整，头尾匹配 |
| `证书链不完整` | `x509: certificate signed by unknown authority` | 缺少中间 CA | 将中间 CA 追加到证书文件 |
| `UTF-8 编码问题` | Subject 显示乱码 | 非 ASCII 字符的编码方式不同 | 使用 BMPString/UTF8String 一致性 |
