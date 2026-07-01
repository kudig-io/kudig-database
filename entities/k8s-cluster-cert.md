---
title: Kubernetes 集群证书管理操作指南
description: '# Kubernetes 集群证书管理操作指南'
category: references
tags:
- k8s
- operations
- cluster-cert
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- rbac
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes 集群证书管理操作指南 是什么
- 如何 Kubernetes 集群证书管理操作指南
trigger_keywords:
- Kubernetes
- 集群证书管理操作指南
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# Kubernetes 集群证书管理操作指南

### 01 Pki Architecture

#### 概述

Kubernetes 集群采用多 CA 架构，将不同信任域的证书隔离管理。整个 PKI 体系由 **三组独立的 CA** 构成，分别服务于控制面组件通信、etcd 集群通信和 API 聚合层扩展。本文档从源码层面全面分析 PKI 架构的设计原理、证书签发链路、组件加载顺序以及安全最佳实践。

---

#### 函数签名

```go
func CreatePKIAssets(cfg *kubeadmapi.InitConfiguration) error

func NewKubernetesCA() *KubeadmCert

func NewEtcdCA() *KubeadmCert

func NewFrontProxyCA() *KubeadmCert

func (k *KubeadmCert) CreateFromCA(cfg *kubeadmapi.InitConfiguration, caCert *KubeadmCert) error

func CreateServiceAccountKeyPair(cfg *kubeadmapi.InitConfiguration) error

func UsingExternalCA(cfg *kubeadmapi.InitConfiguration) (bool, error)

func ValidateCertPeriod(cert *x509.Certificate, key string) error
```

---

#### 源码位置

| 功能 | 文件路径 |
|------|---------|
| 证书阶段主控 | `cmd/kubeadm/app/phases/certs/certs.go` |
| CA 证书定义 | `cmd/kubeadm/app/phases/certs/certs.go` |
| 证书工具封装 | `cmd/kubeadm/app/util/pkiutil/pki_helpers.go` |
| 通用证书生成 | `staging/src/k8s.io/client-go/util/cert/cert.go` |
| CSR 签名控制器 | `pkg/controller/certificates/signer/signer.go` |
| kubelet 证书管理 | `pkg/kubelet/certificate/kubelet.go` |
| 常量定义 | `cmd/kubeadm/app/constants/constants.go` |

---

---

### 02 Ca Generation

#### 函数签名

```go
func CreatePKIAssets(cfg *kubeadmapi.InitConfiguration) error

func NewCertificateAuthority(config *certutil.Config) (*x509.Certificate, crypto.Signer, error)

func NewSelfSignedCACert(cfg Config, key crypto.Signer) (*x509.Certificate, error)

func NewSignedCert(cfg certutil.Config, key crypto.Signer, caCert *x509.Certificate, caKey crypto.Signer) (*x509.Certificate, error)

func WriteCertAndKey(pkiPath string, name string, cert *x509.Certificate, key crypto.Signer) error

func CertOrKeyExist(pkiPath string, name string) bool

func UsingExternalCA(cfg *kubeadmapi.InitConfiguration) (bool, error)
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| CA 生成主控 | `cmd/kubeadm/app/phases/certs/certs.go` | CreatePKIAssets、KubeadmCerts 列表 |
| PKI 工具函数 | `cmd/kubeadm/app/util/pkiutil/pki_helpers.go` | NewCertificateAuthority、WriteCertAndKey |
| 通用证书库 | `staging/src/k8s.io/client-go/util/cert/cert.go` | NewSelfSignedCACert、NewSignedCert |
| 配置定义 | `cmd/kubeadm/app/phases/certs/certs.go` | KubeadmCert 结构体 |
| 证书验证 | `cmd/kubeadm/app/util/pkiutil/pki_helpers.go` | 证书已存在检查 |

#### KubeadmCerts 完整列表

| 证书名 | CA 来源 | CN | 类型 |
|--------|--------|-----|------|
| `ca` | 自签名 | `kubernetes-ca` | 根 CA |
| `etcd/ca` | 自签名 | `etcd-ca` | etcd CA |
| `front-proxy-ca` | 自签名 | `front-proxy-ca` | Front Proxy CA |
| `apiserver` | kubernetes-ca | `kube-apiserver` | 服务端证书 |
| `apiserver-kubelet-client` | kubernetes-ca | `kube-apiserver-kubelet-client` | 客户端证书 |
| `admin` | kubernetes-ca | `kubernetes-admin` | 客户端证书 |
| `controller-manager` | kubernetes-ca | `system:kube-controller-manager` | 客户端证书 |
| `scheduler` | kubernetes-ca | `system:kube-scheduler` | 客户端证书 |
| `etcd/server` | etcd-ca | `etcd-server` | 服务端证书 |
| `etcd/peer` | etcd-ca | `etcd-peer` | 服务端+客户端证书 |
| `etcd/healthcheck-client` | etcd-ca | `kube-etcd-healthcheck-client` | 客户端证书 |
| `apiserver-etcd-client` | etcd-ca | `kube-apiserver-etcd-client` | 客户端证书 |
| `front-proxy-client` | front-proxy-ca | `front-proxy-client` | 客户端证书 |
| `sa` | 无 (密钥对) | 无 | 公钥/私钥 |

---

### 03 Apiserver Cert

#### 概述

API Server 证书是 Kubernetes 集群中最重要的服务端证书，它不仅需要包含正确的 SAN（Subject Alternative Name）以支持集群内外的多种访问方式，还需要配置正确的扩展密钥用途（EKU）。本文档基于 kubeadm 源码，深入分析 API Server 证书的生成逻辑、SAN 动态收集机制、CA 签发流程以及多场景下的证书验证实践。

---

#### 函数签名

```go
func GetAPIServerAltNames(cfg *kubeadmapi.InitConfiguration) (*certutil.AltNames, error)

func NewSignedCert(cfg Config, key crypto.Signer, caCert *x509.Certificate, caKey crypto.Signer) (*x509.Certificate, error)

func (k *KubeadmCert) CreateFromCA(cfg *kubeadmapi.InitConfiguration, caCert *KubeadmCert) error

func NewPrivateKey() (*rsa.PrivateKey, error)

func TryLoadCertAndKeyFromDisk(certPath, keyPath string) (*x509.Certificate, crypto.Signer, error)

func WriteCertAndKey(pkiPath, baseName string, cert *x509.Certificate, key *rsa.PrivateKey) error

func NewCertAndKey(caCert *x509.Certificate, caKey crypto.Signer, config *certutil.Config, key *rsa.PrivateKey) (*x509.Certificate, error)

func GetIndexedIP(cidr *net.IPNet, index int) (net.IP, error)
```

---

#### 源码位置

| 功能 | 文件路径 |
|------|---------|
| API Server 证书定义 | `cmd/kubeadm/app/phases/certs/certs.go` |
| SAN 收集逻辑 | `cmd/kubeadm/app/phases/certs/certs.go` |
| 证书生成工具 | `cmd/kubeadm/app/util/pkiutil/pki_helpers.go` |
| 通用证书库 | `staging/src/k8s.io/client-go/util/cert/cert.go` |
| 证书写入磁盘 | `cmd/kubeadm/app/util/pkiutil/pki_helpers.go` |
| PKI 资产创建入口 | `cmd/kubeadm/app/phases/certs/certs.go` |

---

---

### 04 Etcd Cert

#### 函数签名

```go
func GetEtcdAltNames(cfg *kubeadmapi.InitConfiguration) (*certutil.AltNames, error)

func NewCertificateAuthority(config *certutil.Config) (*x509.Certificate, crypto.Signer, error)

func NewSignedCert(cfg certutil.Config, key crypto.Signer, caCert *x509.Certificate, caKey crypto.Signer) (*x509.Certificate, error)

func (k *KubeadmCert) CreateFromCA(cfg *kubeadmapi.InitConfiguration, caCert *KubeadmCert) error

func WriteCertAndKey(pkiPath string, name string, cert *x509.Certificate, key crypto.Signer) error
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| etcd 证书定义 | `cmd/kubeadm/app/phases/certs/certs.go` | 所有 etcd 证书的 CN/O/Usages 定义 |
| etcd 本地启动 | `cmd/kubeadm/app/phases/etcd/local.go` | etcd 静态 Pod manifest 生成 |
| PKI 工具 | `cmd/kubeadm/app/util/pkiutil/pki_helpers.go` | 证书生成、写入、验证 |
| 通用证书库 | `staging/src/k8s.io/client-go/util/cert/cert.go` | NewSelfSignedCACert、NewSignedCert |
| SAN 计算 | `cmd/kubeadm/app/phases/certs/certs.go` | GetEtcdAltNames |

#### etcd 证书定义参数

| 证书名 | CommonName | Organization | Usages | CA |
|--------|-----------|--------------|--------|-----|
| etcd-ca | `etcd-ca` | 无 | CertSign | 自签名 |
| etcd-server | `etcd-server` | 无 | ServerAuth, ClientAuth | etcd-ca |
| etcd-peer | `etcd-peer` | 无 | ServerAuth, ClientAuth | etcd-ca |
| etcd-healthcheck-client | `kube-etcd-healthcheck-client` | `system:masters` | ClientAuth | etcd-ca |
| apiserver-etcd-client | `kube-apiserver-etcd-client` | `system:masters` | ClientAuth | etcd-ca |

---

### 05 Kubelet Cert

#### 概述

kubelet 证书管理是 Kubernetes 集群证书体系中最复杂的部分。与其他控制面组件不同，kubelet 采用 **引导证书（Bootstrap Token）+ CSR（Certificate Signing Request）自动签发** 机制，使节点能够自动加入集群并管理自身证书。

---

#### 源码路径

- **kubelet 证书管理器**: `pkg/kubelet/certificate/kubelet.go`
- **kubelet 证书轮换**: `pkg/kubelet/certificate/rotation.go`
- **CSR 控制器**: `pkg/controller/certificates/signer/signer.go`
- **kubeadm 引导**: `cmd/kubeadm/app/phases/kubelet/config.go`
- **Bootstrap Token**: `cmd/kubeadm/app/phases/bootstraptoken/node/tls.go`

---

#### kubelet 证书类型

kubelet 需要两类证书：

| 证书类型 | 用途 | 签发方式 |
|---------|------|---------|
| **客户端证书** | kubelet -> API Server 的身份认证 | Bootstrap / CSR |
| **服务端证书** | kubelet 提供 metrics/logs API | Bootstrap / CSR |

---

---

### 06 Cert Rotation

#### 概述

Kubernetes 集群证书具有默认 1 年的有效期（CA 10 年），需要定期轮换以保障集群安全。Kubernetes 提供两种轮换机制：
1. **kubeadm 手动/自动轮换** — 控制面证书
2. **kubelet 自动轮换** — 节点证书（基于 CSR）

---

#### 源码路径

- **kubeadm 轮换命令**: `cmd/kubeadm/app/cmd/phases/certs/renew.go`
- **kubeadm 轮换实现**: `cmd/kubeadm/app/phases/certs/renew.go`
- **kubelet 轮换**: `pkg/kubelet/certificate/rotation.go`
- **证书有效期检查**: `cmd/kubeadm/app/phases/certs/certs.go`

---

#### 1. 轮换命令入口

```go
// cmd/kubeadm/app/cmd/phases/certs/renew.go
func newCmdRenewAll() *cobra.Command {
    return &cobra.Command{
        Use:   "all",
        Short: "Renew all available certificates",
        RunE: func(cmd *cobra.Command, args []string) error {
            // 轮换所有证书
            return renewCerts(renewAllCerts)
        },
    }
}

func newCmdRenewApiserver() *cobra.Command {
    return &cobra.Command{
        Use:   "apiserver",
        Short: "Renew the certificate for serving the Kubernetes API",
        RunE: func(cmd *cobra.Command, args []string) error {
            return renewCerts([]*certs.KubeadmCert{certs.KubeadmCertApiserver})
        },
    }
}
```

**支持的轮换目标**：

| 命令 | 证书 |
|-----|------|
| `kubeadm certs renew all` | 所有证书 |
| `kubeadm certs renew apiserver` | API Server 服务端证书 |
| `kubeadm certs renew apiserver-kubelet-client` | API Server -> kubelet 客户端证书 |
| `kubeadm certs renew apiserver-etcd-client` | API Server -> etcd 客户端证书 |
| `kubeadm certs renew front-proxy-client` | Front Proxy 客户端证书 |
| `kubeadm certs renew etcd-server` | etcd 服务端证书 |
| `kubeadm certs renew etcd-peer` | etcd Peer 证书 |
| `kubeadm certs renew etcd-healthcheck-client` | etcd 健康检查客户端证书 |
| `kubeadm certs renew admin.conf` | 管理员 kubeconfig |
| `kubeadm certs renew controller-manager.conf` | Controller Manager kubeconfig |
| `kubeadm certs renew scheduler.conf` | Scheduler kubeconfig |

---

### 07 Service Account Keys

#### 概述

ServiceAccount 密钥对（sa.pub / sa.key）是 Kubernetes 集群中用于 ServiceAccount Token 签发与验证的核心凭证。不同于 TLS 证书体系，SA 密钥对使用 **JWT（JSON Web Token）** 机制，为 Pod 提供访问 API Server 的身份凭证。

---

#### 源码路径

- **SA 密钥生成**: `cmd/kubeadm/app/phases/certs/certs.go`
- **JWT 签名**: `pkg/serviceaccount/jwt.go`
- **Token 验证**: `pkg/serviceaccount/claims.go`
- **Token 控制器**: `pkg/controlplane/controller.go`
- **Legacy Token**: `pkg/serviceaccount/legacy.go`

---

#### SA 密钥对与 TLS 证书的区别

| 特性 | SA 密钥对 | TLS 证书 |
|-----|----------|---------|
| 格式 | RSA 私钥 + 公钥 | X.509 证书 + 私钥 |
| 用途 | JWT 签名/验证 | TLS 握手 |
| 有效期 | 无内置过期 | 1 年（默认） |
| 轮换难度 | **高**（需重新签发所有 Token） | 低 |
| 存储 | `sa.pub`, `sa.key` | `.crt`, `.key` |

---

---

### 08 Rbac Mapping

#### 函数签名

```go
func (a *Authenticator) AuthenticateRequest(req *http.Request) (*authenticator.Response, bool, error)

func NewSelfSignedCACert(cfg Config, key crypto.Signer) (*x509.Certificate, error)

func (k *KubeadmCert) CreateFromCA(cfg *kubeadmapi.InitConfiguration, caCert *KubeadmCert) error
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| X509 认证插件 | `staging/src/k8s.io/apiserver/pkg/authentication/request/x509/x509.go` | 证书身份提取 |
| RBAC 鉴权 | `plugin/pkg/auth/authorizer/rbac/` | RBAC 授权逻辑 |
| kubeadm 证书配置 | `cmd/kubeadm/app/phases/certs/certs.go` | 各组件证书的 CN/O 定义 |
| Node 鉴权器 | `plugin/pkg/auth/authorizer/node/` | 节点级别的特殊鉴权 |
| ABAC 鉴权 | `plugin/pkg/auth/authorizer/abac/` | 基于属性的访问控制 |

#### X509 Authenticator 参数

| 参数名 | 类型 | 说明 |
|--------|------|------|
| `a.caBundle` | `*x509.CertPool` | 信任的 CA 证书池，对应 `--client-ca-file` |
| `a.opts` | `*x509.VerifyOptions` | 证书验证选项（含 KeyUsages） |
| `req.TLS.PeerCertificates` | `[]*x509.Certificate` | TLS 握手中的客户端证书链 |

---

### 09 Join Cert Flow

#### 概述

kubeadm join 是新节点加入 Kubernetes 集群的标准流程。与 `kubeadm init` 生成所有证书不同，`join` 的核心任务是**安全地获取节点运行所需的凭证**，而非自行生成。理解这一流程对排查节点加入失败、证书分发异常至关重要。

---

#### 源码路径

- **join 命令入口**: `cmd/kubeadm/app/cmd/join.go`
- **Bootstrap Token 阶段**: `cmd/kubeadm/app/phases/bootstraptoken/node/tls.go`
- **kubelet 引导配置**: `cmd/kubeadm/app/phases/kubelet/config.go`
- **CSR 自动审批**: `pkg/controller/certificates/`

---

#### join 流程总览

```
┌─────────────────────────────────────────────────────────────────────┐
│                      kubeadm join 证书获取流程                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  Worker 节点                              Control Plane             │
│  ┌──────────────┐                        ┌──────────────────┐       │
│  │ 1. 拥有      │                        │                  │       │
│  │ Bootstrap    │                        │                  │       │
│  │ Token + CA   │                        │                  │       │
│  │ Hash         │                        │                  │       │
│  └──────┬───────┘                        │                  │       │
│         │                                │                  │       │
│         │ 2. HTTPS GET /api/v1/namespaces/ │                  │       │
│         │    kube-public/configmaps/       │                  │       │
│         │    cluster-info                  │                  │       │
│         ├───────────────────────────────►│                  │       │
│         │                                │                  │       │
│         │ 3. 验证 CA 证书指纹              │                  │       │
│         │    (discovery-token-ca-cert-hash)│                 │       │
│         │◄────────────────────────────────│                  │       │
│         │                                │               
...(截断)

---

### 10 Front Proxy Workflow

#### 概述

Front Proxy（请求头代理）是 Kubernetes API 聚合层（Aggregation Layer）的安全基石。metrics-server、custom metrics API、service catalog 等扩展 API Server 均依赖此机制。与常规 TLS 双向认证不同，Front Proxy 采用 **"受信代理 + 请求头传递"** 的模式，使扩展 API Server 能够识别原始调用者身份。

---

#### 源码路径

- **请求头认证**: `staging/src/k8s.io/apiserver/pkg/authentication/request/headerrequest/`
- **API Server 聚合配置**: `pkg/kubeapiserver/options/authentication.go`
- **kubeadm front-proxy 证书**: `cmd/kubeadm/app/phases/certs/certs.go`
- **API Service 注册**: `pkg/kubeaggregator/`

---

#### 架构与信任模型

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        Front Proxy 信任模型                               │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│   用户/kubectl                                                            │
│      │                                                                    │
│      │ 1. 使用个人证书 (由 kubernetes-ca 签发)                            │
│      ▼                                                                    │
│   ┌──────────────┐                                                        │
│   │  API Server  │                                                        │
│   │              │ 2. 验证用户证书 (client-ca-file=ca.crt)                 │
│   │  --client-ca │                                                        │
│   │   -file      │ 3. 确定用户身份: CN=user, O=group                      │
│   │              │                                                        │
│   │  4. 连接 metrics-server 时使用 front-proxy-client.crt                 │
│   │     添加 HTTP Header:                                                 │
│   │     X-Remote-User: user                                               │
│   │     X-Remote-Group: group                                             │
│   └──────┬───────┘                                                        │
│          │                                     
...(截断)

---

### 11 Apiserver Cert Flags

#### 函数签名

```go
func CreateServerOptions() (*options.ServerOptions, error)

func (s *ServerOptions) Complete() error
func (s *ServerOptions) Validate() []error
func (s *ServerOptions) RunServer(stopCh <-chan struct{}) error

func buildGenericConfig(s *options.ServerOptions) (*genericapiserver.Config, error)

func createKubeAPIServerConfig(
    s *options.ServerOptions,
    kubeAPIServerStorageConfig *storagebackend.Config,
) (

---
(内容截断，完整内容见源文件) ---

## 相关链接

- [[skills/ts-security-auth.md|安全认证排查]]
- [[skills/backup-restore-etcd.md|etcd 备份恢复]]
- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[entities/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[entities/kubelet.md|kubelet]] — kubelet
- [[entities/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
