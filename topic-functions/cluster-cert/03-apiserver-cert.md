# API Server 证书生成源码分析

## 概述

API Server 证书是 Kubernetes 集群中最重要的服务端证书，它不仅需要包含正确的 SAN（Subject Alternative Name）以支持集群内外的多种访问方式，还需要配置正确的扩展密钥用途（EKU）。本文档基于 kubeadm 源码，深入分析 API Server 证书的生成逻辑。

---

## 源码路径

- **API Server 证书定义**: `cmd/kubeadm/app/phases/certs/certs.go`
- **证书生成工具**: `cmd/kubeadm/app/util/pkiutil/pki_helpers.go`
- **通用证书库**: `staging/src/k8s.io/client-go/util/cert/cert.go`

---

## API Server 证书定义

```go
// cmd/kubeadm/app/phases/certs/certs.go
var KubeadmCertApiserver = &KubeadmCert{
    Name:     "apiserver",
    LongName: "certificate for serving the Kubernetes API",
    BaseName: KubeadmCertApiserver,
    CAName:   "ca",  // 由 kubernetes-ca 签发
    Config: certutil.Config{
        CommonName:   "kube-apiserver",
        Organization: []string{"system:masters"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
    },
}
```

**关键属性**：

| 属性 | 值 | 说明 |
|-----|---|------|
| `CommonName` | `kube-apiserver` | 证书主体名称 |
| `Organization` | `system:masters` | 所属组织，RBAC 中该组具有 cluster-admin 权限 |
| `Usages` | `ExtKeyUsageServerAuth` | 仅用于服务端认证 |
| `CAName` | `ca` | 由 kubernetes-ca 签发 |

---

## SAN 生成逻辑

API Server 证书的 SAN 列表是证书生成中最复杂的部分，它决定了哪些地址可以通过 TLS 验证连接到 API Server。

### 1. SAN 收集函数

```go
// cmd/kubeadm/app/phases/certs/certs.go
func GetAPIServerAltNames(cfg *kubeadmapi.InitConfiguration) (*certutil.AltNames, error) {
    altNames := &certutil.AltNames{
        DNSNames: []string{
            cfg.NodeRegistration.Name,                    // 节点主机名
            "kubernetes",                                 // 内部 DNS 名
            "kubernetes.default",                         // default 命名空间服务名
            "kubernetes.default.svc",                     // svc 服务名
            "kubernetes.default.svc." + cfg.Networking.DNSDomain,  // 完整服务域名
        },
        IPs: []net.IP{
            cfg.LocalAPIEndpoint.AdvertiseAddress,        // API Server 公告地址
        },
    }

    // 添加 Service CIDR 的第一个 IP (通常是 10.96.0.1)
    // 这是 kubernetes.default.svc 的 ClusterIP
    svcSubnetCIDR, err := net.ParseCIDR(cfg.Networking.ServiceSubnet)
    if err == nil {
        internalAPIServerVirtualIP, err := utilsnet.GetIndexedIP(svcSubnetCIDR, 1)
        if err == nil {
            altNames.IPs = append(altNames.IPs, internalAPIServerVirtualIP)
        }
    }

    // 添加用户通过 certSANs 自定义的 SAN
    for _, altname := range cfg.APIServer.CertSANs {
        if ip := net.ParseIP(altname); ip != nil {
            altNames.IPs = append(altNames.IPs, ip)
        } else {
            altNames.DNSNames = append(altNames.DNSNames, altname)
        }
    }

    return altNames, nil
}
```

### 2. 默认 SAN 列表

```go
// 默认生成的 SAN 包括:
DNSNames: [
    "<node-hostname>",
    "kubernetes",
    "kubernetes.default",
    "kubernetes.default.svc",
    "kubernetes.default.svc.cluster.local",
]
IPs: [
    <API Server AdvertiseAddress>,  // 如 192.168.1.10
    <Service CIDR 第一个 IP>,       // 如 10.96.0.1
]
```

### 3. 自定义 certSANs 配置

```yaml
# kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
apiServer:
  certSANs:
    - "192.168.1.100"          # 新增 IP
    - "k8s.example.com"        # 新增域名
    - "api.k8s.internal"       # 内部负载均衡域名
    - "10.0.0.50"              # 外部负载均衡 IP
```

---

## API Server 证书生成源码

### 1. NewSignedCert — CA 签发证书的核心函数

```go
// staging/src/k8s.io/client-go/util/cert/cert.go
func NewSignedCert(cfg Config, key crypto.Signer, caCert *x509.Certificate, caKey crypto.Signer) (*x509.Certificate, error) {
    serial, err := rand.Int(rand.Reader, new(big.Int).SetInt64(math.MaxInt64))
    if err != nil {
        return nil, err
    }

    certTmpl := x509.Certificate{
        SerialNumber: serial,
        Subject: pkix.Name{
            CommonName:   cfg.CommonName,
            Organization: cfg.Organization,
        },
        DNSNames:     cfg.AltNames.DNSNames,
        IPAddresses:  cfg.AltNames.IPs,
        NotBefore:    time.Now().Add(-5 * time.Minute).UTC(),
        NotAfter:     time.Now().Add(cfg.ValidityPeriod).UTC(),
        KeyUsage:     x509.KeyUsageKeyEncipherment | x509.KeyUsageDigitalSignature,
        ExtKeyUsage:  cfg.Usages,
    }

    // 使用 CA 私钥签名
    certDERBytes, err := x509.CreateCertificate(rand.Reader, &certTmpl, caCert, key.Public(), caKey)
    if err != nil {
        return nil, err
    }

    return x509.ParseCertificate(certDERBytes)
}
```

**关键属性分析**：

| 属性 | 值 | 说明 |
|-----|---|------|
| `SerialNumber` | 随机生成 | 防止证书指纹冲突 |
| `KeyUsage` | `KeyEncipherment \| DigitalSignature` | 非 CA 证书不需要 CertSign |
| `ExtKeyUsage` | `ServerAuth` | 仅用于服务端 TLS 认证 |
| `NotBefore` | 当前时间 - 5 分钟 | 允许轻微的时钟偏差，确保证书立即可用 |
| `NotAfter` | 当前时间 + 1 年 | 默认有效期 |

### 2. kubeadm 的封装调用链

```go
// cmd/kubeadm/app/phases/certs/certs.go
func (k *KubeadmCert) CreateFromCA(cfg *kubeadmapi.InitConfiguration, caCert *KubeadmCert) error {
    // 获取 SAN
    altNames, err := k.GetConfig(cfg)
    
    // 生成 RSA 私钥
    key, err := pkiutil.NewPrivateKey()
    
    // 构造证书配置
    certConfig := certutil.Config{
        CommonName:   k.Config.CommonName,
        Organization: k.Config.Organization,
        AltNames:     *altNames,
        Usages:       k.Config.Usages,
        ValidityPeriod: cfg.CertificateValidityPeriod,
    }
    
    // 加载 CA 证书和私钥
    caCertFile, caKeyFile := caCert.PathsForCertificateAndKey(cfg.CertificatesDir)
    caCertificate, caKey, err := pkiutil.TryLoadCertAndKeyFromDisk(caCertFile, caKeyFile)
    
    // 生成由 CA 签名的证书
    cert, err := pkiutil.NewCertAndKey(caCertificate, caKey, &certConfig, key)
    
    // 写入磁盘
    return pkiutil.WriteCertAndKey(cfg.CertificatesDir, k.BaseName, cert, key)
}
```

---

## API Server 客户端证书验证

API Server 作为服务端，使用 `kubernetes-ca` 验证所有客户端证书：

```go
// API Server 启动参数
--client-ca-file=/etc/kubernetes/pki/ca.crt
--requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
```

**验证逻辑**：
1. 普通客户端（kubectl、kubelet）使用 `--client-ca-file` 指定的 CA 验证
2. API 聚合层请求（metrics-server）使用 `--requestheader-client-ca-file` 指定的 CA 验证
3. 从客户端证书中提取 `Subject.CommonName` 作为用户名
4. 从客户端证书中提取 `Subject.Organization` 作为用户组

---

## API Server 客户端证书

API Server 不仅需要服务端证书，还需要多个客户端证书用于连接其他组件。

### 1. API Server -> kubelet 客户端证书

```go
// cmd/kubeadm/app/phases/certs/certs.go
var KubeadmCertApiserverKubeletClient = &KubeadmCert{
    Name:     "apiserver-kubelet-client",
    LongName: "certificate for the API server to connect to kubelet",
    BaseName: KubeadmCertApiserverKubeletClient,
    CAName:   "ca",
    Config: certutil.Config{
        CommonName:   "kube-apiserver-kubelet-client",
        Organization: []string{"system:masters"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    },
}
```

**关键差异**：
- `Usages: ExtKeyUsageClientAuth` — 仅用于客户端认证
- `Organization: system:masters` — 具有 kubelet 的完全访问权限

### 2. API Server -> etcd 客户端证书

```go
var KubeadmCertApiserverEtcdClient = &KubeadmCert{
    Name:     "apiserver-etcd-client",
    LongName: "certificate for the API server to connect to etcd",
    BaseName: KubeadmCertApiserverEtcdClient,
    CAName:   "etcd-ca",  // 注意：由 etcd-ca 签发
    Config: certutil.Config{
        CommonName:   "kube-apiserver-etcd-client",
        Organization: []string{"system:masters"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    },
}
```

**关键差异**：
- `CAName: etcd-ca` — 由 etcd 独立 CA 签发
- etcd 使用自己的 CA 验证 API Server 的客户端身份

---

## 证书验证实践

```bash
# 1. 查看 API Server 证书完整信息
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text

# 2. 查看 SAN 列表
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -ext subjectAltName

# 3. 验证证书链
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt

# 4. 验证证书与私钥匹配
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -modulus | md5sum
openssl rsa -in /etc/kubernetes/pki/apiserver.key -noout -modulus | md5sum

# 5. 远程检查 API Server 证书
echo | openssl s_client -connect <api-server-ip>:6443 -servername kubernetes 2>/dev/null | openssl x509 -noout -text
```

---

## API Server 证书问题排查

| 问题 | 现象 | 根因 | 解决 |
|-----|------|------|------|
| SAN 缺失 | `x509: certificate is valid for X, not Y` | 新 IP/域名未加入 certSANs | 更新 kubeadm-config，重新生成证书 |
| 证书过期 | `certificate has expired` | 超过 1 年有效期 | `kubeadm certs renew apiserver` |
| CA 不信任 | `signed by unknown authority` | 使用错误的 CA 验证 | 确认客户端使用正确的 ca.crt |
| 密钥不匹配 | `private key does not match public key` | 证书与私钥不配对 | 重新生成匹配的证书密钥对 |
