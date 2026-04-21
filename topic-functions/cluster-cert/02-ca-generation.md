# CA 证书生成源码分析

## 概述

Kubernetes 集群部署时，kubeadm 首先生成三组 CA 证书。本文档基于官方源码，深入分析 CA 证书的生成逻辑、参数配置及密钥算法选择。

---

## 源码路径

- **CA 生成主控**: `cmd/kubeadm/app/phases/certs/certs.go`
- **PKI 工具函数**: `cmd/kubeadm/app/util/pkiutil/pki_helpers.go`
- **通用证书库**: `staging/src/k8s.io/client-go/util/cert/cert.go`

---

## CA 生成主控流程

### 1. kubeadm 证书阶段入口

```go
// cmd/kubeadm/app/phases/certs/certs.go
func CreatePKIAssets(cfg *kubeadmapi.InitConfiguration) error {
    certificatesDir := cfg.CertificatesDir

    // 定义所有需要生成的证书
    certList := KubeadmCerts

    for _, cert := range certList {
        // 如果 CA 不存在，先生成 CA
        if cert.CAName == "" {
            // 这是根 CA
            if err := cert.CreateFromCA(cfg, nil); err != nil {
                return err
            }
        } else {
            // 这是由 CA 签发的证书
            caCert := GetCert(cert.CAName)
            if err := cert.CreateFromCA(cfg, caCert); err != nil {
                return err
            }
        }
    }
    return nil
}
```

### 2. CA 证书定义列表

```go
// cmd/kubeadm/app/phases/certs/certs.go
var KubeadmCerts = []*KubeadmCert{
    // 根 CA 证书
    KubeadmCertRootCA,      // kubernetes-ca
    KubeadmCertEtcdCA,      // etcd-ca
    KubeadmCertFrontProxyCA, // front-proxy-ca

    // 由 kubernetes-ca 签发的证书
    KubeadmCertApiserver,
    KubeadmCertApiserverKubeletClient,
    KubeadmCertAdmin,
    KubeadmCertControllerManager,
    KubeadmCertScheduler,

    // 由 etcd-ca 签发的证书
    KubeadmCertEtcdServer,
    KubeadmCertEtcdPeer,
    KubeadmCertEtcdHealthcheck,
    KubeadmCertApiserverEtcdClient,

    // 由 front-proxy-ca 签发的证书
    KubeadmCertFrontProxyClient,

    // ServiceAccount 密钥对 (非证书)
    KubeadmCertServiceAccount,
}
```

---

## CA 证书生成源码深度分析

### 1. NewSelfSignedCACert — 核心 CA 生成函数

```go
// staging/src/k8s.io/client-go/util/cert/cert.go
func NewSelfSignedCACert(cfg Config, key crypto.Signer) (*x509.Certificate, error) {
    now := time.Now()

    // 构造 x509 证书模板
    templ := x509.Certificate{
        SerialNumber: new(big.Int).SetInt64(0),
        Subject: pkix.Name{
            CommonName:   cfg.CommonName,
            Organization: cfg.Organization,
        },
        NotBefore:             now.UTC(),
        NotAfter:              now.Add(cfg.ValidityPeriod).UTC(),
        KeyUsage:              x509.KeyUsageKeyEncipherment | x509.KeyUsageDigitalSignature | x509.KeyUsageCertSign,
        BasicConstraintsValid: true,
        IsCA:                  true,
    }

    // 自签名：使用自己的私钥签名
    certDERBytes, err := x509.CreateCertificate(
        rand.Reader,
        &templ,       // 证书模板
        &templ,       // 父证书 (自签即自己)
        key.Public(), // 公钥
        key,          // 签名私钥
    )
    if err != nil {
        return nil, err
    }

    return x509.ParseCertificate(certDERBytes)
}
```

**关键属性分析**：

| 属性 | 值 | 说明 |
|-----|---|------|
| `SerialNumber` | `0` | CA 证书通常每个信任域只有一个，固定序列号可接受 |
| `KeyUsage` | `KeyEncipherment \| DigitalSignature \| CertSign` | CA 必须具有 CertSign 权限 |
| `BasicConstraintsValid` | `true` | 启用基本约束扩展 |
| `IsCA` | `true` | 标记为 CA 证书 |
| `ValidityPeriod` | 10 年 (默认) | 通过 `cfg.ValidityPeriod` 传入 |

**CA 与终端实体证书的序列号策略差异**：
- **CA 证书**：`SerialNumber = 0`，因为每个信任域通常只有一个 CA 证书，不存在冲突
- **服务端/客户端证书**：`NewSignedCert` 使用 `rand.Int(rand.Reader, MaxInt64)` 生成随机序列号，防止同一 CA 签发的多个证书指纹冲突

### 2. kubeadm 对 CA 生成的封装

```go
// cmd/kubeadm/app/util/pkiutil/pki_helpers.go
func NewCertificateAuthority(config *certutil.Config) (*x509.Certificate, crypto.Signer, error) {
    // 1. 生成 RSA 2048 私钥
    key, err := rsa.GenerateKey(cryptorand.Reader, rsaKeySize)
    if err != nil {
        return nil, nil, err
    }

    // 2. 生成自签名 CA 证书
    cert, err := certutil.NewSelfSignedCACert(*config, key)
    if err != nil {
        return nil, nil, err
    }

    return cert, key, nil
}
```

**密钥算法**：
- 默认使用 **RSA 2048**（`rsaKeySize = 2048`）
- 不采用 ECDSA 的原因：保持与旧版本 openssl 客户端的最大兼容性

### 3. CA 生成时的 Subject 配置

```go
// cmd/kubeadm/app/phases/certs/certs.go
// kubernetes-ca 配置
&KubeadmCert{
    Name:     "ca",
    BaseName: "ca",
    Config: certutil.Config{
        CommonName: "kubernetes-ca",
    },
}

// etcd-ca 配置
&KubeadmCert{
    Name:     "etcd-ca",
    BaseName: "ca",
    Config: certutil.Config{
        CommonName: "etcd-ca",
    },
}

// front-proxy-ca 配置
&KubeadmCert{
    Name:     "front-proxy-ca",
    BaseName: "front-proxy-ca",
    Config: certutil.Config{
        CommonName: "front-proxy-ca",
    },
}
```

---

## 证书文件读写逻辑

### 1. 写证书到磁盘

```go
// cmd/kubeadm/app/util/pkiutil/pki_helpers.go
func WriteCertAndKey(pkiPath string, name string, cert *x509.Certificate, key crypto.Signer) error {
    // 写入证书: <name>.crt
    if err := WriteCert(pkiPath, name, cert); err != nil {
        return err
    }
    // 写入私钥: <name>.key
    if err := WriteKey(pkiPath, name, key); err != nil {
        return err
    }
    return nil
}

func WriteCert(pkiPath string, name string, cert *x509.Certificate) error {
    certificatePath := pathForCert(pkiPath, name)
    // 证书使用 0644 权限 (只读，所有人可读)
    if err := certutil.WriteCert(certificatePath, certToPem(cert)); err != nil {
        return errors.Wrapf(err, "unable to write certificate to file %s", certificatePath)
    }
    return nil
}

func WriteKey(pkiPath string, name string, key crypto.Signer) error {
    privateKeyPath := pathForKey(pkiPath, name)
    // 私钥使用 0600 权限 (仅所有者可读写)
    if err := keyutil.WriteKey(privateKeyPath, keyToPem(key)); err != nil {
        return errors.Wrapf(err, "unable to write private key to file %s", privateKeyPath)
    }
    return nil
}
```

**文件权限设计**：

| 文件类型 | 权限 | 说明 |
|---------|------|------|
| `.crt` (证书) | `0644` | 所有人可读，用于组件验证 |
| `.key` (私钥) | `0600` | 仅 root 可读写，保护密钥安全 |

### 2. 证书已存在时的处理

```go
// cmd/kubeadm/app/phases/certs/certs.go
func (k *KubeadmCert) CreateFromCA(cfg *kubeadmapi.InitConfiguration, caCert *KubeadmCert) error {
    // 检查证书是否已存在
    if certutil.CertOrKeyExist(pkiDir, k.BaseName) {
        // 证书已存在，跳过生成
        // 这是幂等性设计：重复执行 kubeadm init 不会覆盖已有证书
        return nil
    }
    // ... 生成证书
}
```

**设计要点**：
- kubeadm 证书生成是**幂等**的
- 已存在的证书不会被覆盖，防止意外替换有效证书
- 如需重新生成，需手动删除旧证书

---

## 外部 CA 支持

kubeadm 支持使用外部 CA，即由外部 PKI 系统预先提供 CA 证书和密钥。

### 外部 CA 模式检测

```go
// cmd/kubeadm/app/phases/certs/certs.go
func UsingExternalCA(cfg *kubeadmapi.InitConfiguration) (bool, error) {
    // 检查 CA 证书和密钥是否存在
    caCertExists := certutil.CertOrKeyExist(cfg.CertificatesDir, "ca")
    caKeyExists := certutil.CertOrKeyExist(cfg.CertificatesDir, "ca")

    // 如果证书存在但密钥不存在，说明使用外部 CA
    if caCertExists && !caKeyExists {
        return true, nil
    }
    return false, nil
}
```

### 外部 CA 时的证书生成行为

```go
// 使用外部 CA 时，kubeadm 只生成需要私钥的证书
// 如果外部 CA 未提供密钥，kubeadm 无法签发新证书
// 此时需要外部 PKI 系统预先签发所有组件证书
```

**外部 CA 要求提供的文件**：
- `ca.crt` — 必须
- `ca.key` — 可选（如不提供，kubeadm 无法自动签发新证书）
- `etcd/ca.crt` / `etcd/ca.key` — 如果使用外部 etcd CA
- `front-proxy-ca.crt` / `front-proxy-ca.key` — 如果使用外部 Front Proxy CA

---

## CA 证书的关键属性验证

```bash
# 查看 kubernetes-ca 证书详情
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -text

# 关键输出示例:
# Subject: CN = kubernetes-ca
# Issuer: CN = kubernetes-ca
#          (自签名: Subject == Issuer)
# 
# X509v3 Basic Constraints: critical
#     CA:TRUE
# 
# X509v3 Key Usage: critical
#     Certificate Sign
```

**必须验证的属性**：
1. `CA:TRUE` — 确认是 CA 证书
2. `Certificate Sign` — 确认具有签发证书的权限
3. `Subject == Issuer` — 确认是自签名根证书
4. `Not After` — 确认有效期
