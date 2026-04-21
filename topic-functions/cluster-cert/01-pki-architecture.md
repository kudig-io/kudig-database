# Kubernetes 集群 PKI 架构总览

## 概述

Kubernetes 集群采用多 CA 架构，将不同信任域的证书隔离管理。整个 PKI 体系由 **三组独立的 CA** 构成，分别服务于控制面组件通信、etcd 集群通信和 API 聚合层扩展。

---

## 源码入口

**kubeadm 证书阶段入口**：
```go
// cmd/kubeadm/app/phases/certs/certs.go
func CreatePKIAssets(cfg *kubeadmapi.InitConfiguration) error {
    // 1. 生成 Kubernetes CA
    // 2. 生成 etcd CA
    // 3. 生成 Front Proxy CA
    // 4. 生成各组件证书
    // 5. 生成 ServiceAccount 密钥对
}
```

---

## PKI 架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Kubernetes 集群 PKI 架构                           │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐     │
│  │  kubernetes-ca  │  │    etcd-ca      │  │  front-proxy-ca │     │
│  │   (集群根 CA)    │  │  (etcd 独立 CA)  │  │  (聚合层 CA)     │     │
│  │   默认 10 年     │  │   默认 10 年     │  │   默认 10 年     │     │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘     │
│           │                    │                    │              │
│           ▼                    ▼                    ▼              │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                     kubernetes-ca 签发                        │  │
│  │  ├─ apiserver.crt         (API Server 服务端证书)             │  │
│  │  ├─ apiserver-kubelet-client.crt (API Server -> kubelet)     │  │
│  │  ├─ apiserver-etcd-client.crt    (API Server -> etcd)        │  │
│  │  ├─ admin.conf            (管理员 kubeconfig)                 │  │
│  │  ├─ controller-manager.conf (kube-controller-manager)        │  │
│  │  ├─ scheduler.conf        (kube-scheduler)                   │  │
│  │  └─ (kubelet 客户端证书通过 CSR 动态签发)                    │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                     etcd-ca 签发                              │  │
│  │  ├─ etcd/server.crt       (etcd 服务端证书)                   │  │
│  │  ├─ etcd/peer.crt         (etcd 对等证书, 集群间通信)         │  │
│  │  └─ etcd/healthcheck-client.crt (etcd 健康检查客户端证书)     │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                  front-proxy-ca 签发                          │  │
│  │  ├─ front-proxy-client.crt  (API 聚合层客户端证书)            │  │
│  │  │   用于: metrics-server, custom metrics API 等              │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                                      │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                  ServiceAccount 密钥对                        │  │
│  │  ├─ sa.pub  (公钥, API Server 验证 JWT Token)                 │  │
│  │  └─ sa.key  (私钥, Controller Manager 签名 Token)             │  │
│  └─────────────────────────────────────────────────────────────┘  │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 三组 CA 的独立性与设计意图

### 1. kubernetes-ca（集群根 CA）

**源码定义**：
```go
// cmd/kubeadm/app/phases/certs/certs.go
func NewKubernetesCA() *KubeadmCert {
    return &KubeadmCert{
        Name:     "ca",
        LongName: "certificate authority",
        BaseName: KubeadmCertRootCABaseName,
        Config: certutil.Config{
            CommonName: "kubernetes-ca",
        },
    }
}
```

**设计意图**：
- 作为整个 Kubernetes 控制面的信任根
- 签发 API Server、组件客户端证书
- 与 etcd CA 分离，允许独立轮换 etcd 证书而不影响控制面

### 2. etcd-ca（etcd 独立 CA）

**源码定义**：
```go
// cmd/kubeadm/app/phases/certs/certs.go
func NewEtcdCA() *KubeadmCert {
    return &KubeadmCert{
        Name:     "etcd-ca",
        LongName: "etcd certificate authority",
        BaseName: "ca",
        Config: certutil.Config{
            CommonName: "etcd-ca",
        },
    }
}
```

**设计意图**：
- etcd 作为独立分布式存储，拥有自己的 PKI 体系
- 允许将 etcd 部署在独立主机上，由独立团队管理
- 支持外部 etcd 集群（不通过 kubeadm 管理 etcd 证书）

### 3. front-proxy-ca（API 聚合层 CA）

**源码定义**：
```go
// cmd/kubeadm/app/phases/certs/certs.go
func NewFrontProxyCA() *KubeadmCert {
    return &KubeadmCert{
        Name:     "front-proxy-ca",
        LongName: "front-proxy certificate authority",
        BaseName: KubeadmCertFrontProxyCA,
        Config: certutil.Config{
            CommonName: "front-proxy-ca",
        },
    }
}
```

**设计意图**：
- 隔离 API 聚合层（Aggregation Layer）的信任链
- API Server 使用 front-proxy-client 证书连接扩展 API Server
- 扩展 API Server 使用 front-proxy-ca 验证请求身份
- 与 kubernetes-ca 分离，避免聚合层证书问题影响核心控制面

---

## 证书路径与命名规范

**kubeadm 默认存储路径**：`/etc/kubernetes/pki/`

```
/etc/kubernetes/pki/
├── ca.crt                          # Kubernetes CA 证书
├── ca.key                          # Kubernetes CA 私钥
├── apiserver.crt                   # API Server 服务端证书
├── apiserver.key                   # API Server 服务端私钥
├── apiserver-kubelet-client.crt    # API Server -> kubelet 客户端证书
├── apiserver-kubelet-client.key    # API Server -> kubelet 客户端私钥
├── apiserver-etcd-client.crt       # API Server -> etcd 客户端证书
├── apiserver-etcd-client.key       # API Server -> etcd 客户端私钥
├── front-proxy-ca.crt              # Front Proxy CA 证书
├── front-proxy-ca.key              # Front Proxy CA 私钥
├── front-proxy-client.crt          # Front Proxy 客户端证书
├── front-proxy-client.key          # Front Proxy 客户端私钥
├── sa.pub                          # ServiceAccount 公钥
├── sa.key                          # ServiceAccount 私钥
└── etcd/
    ├── ca.crt                      # etcd CA 证书
    ├── ca.key                      # etcd CA 私钥
    ├── server.crt                  # etcd 服务端证书
    ├── server.key                  # etcd 服务端私钥
    ├── peer.crt                    # etcd Peer 证书
    ├── peer.key                    # etcd Peer 私钥
    ├── healthcheck-client.crt      # etcd 健康检查客户端证书
    └── healthcheck-client.key      # etcd 健康检查客户端私钥
```

---

## 证书有效期配置

**源码定义**：
```go
// cmd/kubeadm/app/constants/constants.go
const (
    // CertificateValidityPeriod 定义证书默认有效期 (1 年)
    CertificateValidityPeriod = time.Hour * 24 * 365
    
    // CAValidityPeriod 定义 CA 证书默认有效期 (10 年)
    CAValidityPeriod = time.Hour * 24 * 365 * 10
)
```

| 证书类型 | 默认有效期 | 配置项 |
|---------|----------|--------|
| CA 证书 | 10 年 | `CAValidityPeriod` |
| 服务端/客户端证书 | 1 年 | `CertificateValidityPeriod` |
| kubelet 客户端证书 | 1 年 (默认) | 通过 CSR 动态签发 |
| kubelet 服务端证书 | 1 年 (默认) | 通过 CSR 动态签发 |

---

## 信任链验证关系

```
客户端 (kubectl/kubelet)                    API Server
     │                                         │
     │  1. 使用 ca.crt 验证 API Server 证书    │
     │◄────────────────────────────────────────┤
     │                                         │
     │  2. 使用客户端证书 (admin.conf) 认证    │
     ├────────────────────────────────────────►│
     │                                         │
     │         API Server 内部验证             │
     │     使用 kubernetes-ca 验证客户端证书   │
     │                                         │
     │                                         │
API Server ──────► etcd                      etcd
使用 etcd/ca.crt 验证 etcd 证书      使用 etcd/ca.crt 验证 API Server 客户端证书
```

---

## 组件启动时的证书加载顺序

```
kubelet 启动
    │
    ├─ 1. 读取 /etc/kubernetes/kubelet.conf（或 bootstrap-kubelet.conf）
    ├─ 2. 如果无有效客户端证书，使用 Bootstrap Token 创建 CSR
    ├─ 3. 等待 CSR 批准，将证书写入 /var/lib/kubelet/pki/
    ├─ 4. 加载服务端证书（如启用 serverTLSBootstrap）
    └─ 5. 启动 kubelet 服务

API Server 启动
    │
    ├─ 1. 读取 --tls-cert-file/--tls-private-key-file
    ├─ 2. 读取 --client-ca-file（验证客户端证书）
    ├─ 3. 读取 --etcd-cafile/--etcd-certfile/--etcd-keyfile
    ├─ 4. 读取 --service-account-key-file（验证 SA Token）
    ├─ 5. 读取 --proxy-client-cert-file/--proxy-client-key-file
    └─ 6. 启动 HTTPS 服务

Controller Manager 启动
    │
    ├─ 1. 读取 --service-account-private-key-file（签名 SA Token）
    ├─ 2. 读取 kubeconfig（连接 API Server）
    └─ 3. 启动控制器循环
```

**关键观察**：
- 所有组件在启动时**一次性加载证书文件到内存**
- 证书文件被替换后，组件不会自动感知（kubelet 除外，它使用证书管理器轮询）
- 因此证书轮换后必须重启组件才能使用新证书

---

## 关键源码文件索引

| 功能 | 源码路径 |
|-----|---------|
| 证书阶段主控 | `cmd/kubeadm/app/phases/certs/certs.go` |
| 证书工具封装 | `cmd/kubeadm/app/util/pkiutil/pki_helpers.go` |
| 通用证书生成 | `staging/src/k8s.io/client-go/util/cert/cert.go` |
| CSR 签名控制器 | `pkg/controller/certificates/signer/signer.go` |
| kubelet 证书管理 | `pkg/kubelet/certificate/kubelet.go` |
