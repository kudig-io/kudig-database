# 证书阶段 (Certificate Generation)

## 源码路径

`cmd/kubeadm/app/phases/certs/certs.go`

---

## 生成证书列表

| 证书 | 用途 | 存储路径 |
|------|------|---------|
| `ca.crt/key` | Kubernetes CA | `/etc/kubernetes/pki/` |
| `apiserver.crt/key` | API Server 证书 (节点通信) | `/etc/kubernetes/pki/` |
| `apiserver-kubelet-client.crt/key` | API Server 连接 kubelet (认证) | `/etc/kubernetes/pki/` |
| `apiserver-kubelet-serving.crt/key` | kubelet 暴露 metrics (kubelet 自己做 server) | `/etc/kubernetes/pki/` |
| `front-proxy-ca.crt/key` | Front Proxy CA (aggregator) | `/etc/kubernetes/pki/` |
| `front-proxy-client.crt/key` | Front Proxy Client (API Server → extension apiserver) | `/etc/kubernetes/pki/` |
| `etcd/ca.crt/key` | etcd CA | `/etc/kubernetes/pki/etcd/` |
| `etcd/server.crt/key` | etcd Server | `/etc/kubernetes/pki/etcd/` |
| `etcd/peer.crt/key` | etcd Peer (节点间通信) | `/etc/kubernetes/pki/etcd/` |
| `etcd/healthcheck-client.crt/key` | etcd 健康检查 | `/etc/kubernetes/pki/etcd/` |
| `sa.pub/key` | Service Account 签名密钥 | `/etc/kubernetes/pki/` |

---

## 证书有效期

| 证书类型 | 有效期 | 续期方式 |
|---------|--------|---------|
| CA (ca.crt) | 10 年 | 不续期 |
| API Server 证书 | 1 年 | `kubeadm alpha certs renew apiserver` |
| kubelet Client 证书 | 1 年 | kubelet 自动续期 |
| kubelet Serving 证书 | 1 年 | kubelet 自动续期 |
| etcd 证书 | 1 年 | `kubeadm alpha certs renew etcd-server` |
| Front Proxy 证书 | 1 年 | `kubeadm alpha certs renew front-proxy-client` |
| SA 签名密钥 | 10 年 | `kubeadm alpha certs renew sa` |

---

## apiserver-kubelet-client vs apiserver-kubelet-serving

这两个证书容易混淆:

```
apiserver-kubelet-client:
  用途: API Server 作为客户端，向 kubelet 的 API (10250) 发请求 (如 exec/attach/logs)
  认证: API Server 用此证书客户端证书连接 kubelet
  持有者: API Server

apiserver-kubelet-serving:
  用途: kubelet 作为服务端，暴露 /metrics /stats 等端点供其他组件读取
  认证: 其他组件用 CA 验证 kubelet server 证书
  持有者: kubelet
```

---

## 核心代码

```go
// cmd/kubeadm/app/phases/certs/certs.go
func CreatePKIAssets(cfg *kubeadmapi.InitConfiguration) error {
    caCfg := certs.CACertConfig{
        CommonName:   "kubernetes-ca",
        Organization: []string{"kubernetes/kubernetes"},
    }
    // 生成 CA
    if err := certs.GenerateCACert(caCfg, cfg.CertificatesDir); err != nil {
        return err
    }
    // 生成 API Server 证书
    apiServerCfg := certs.APIServerCertConfig{
        CertConfig: certs.CertConfig{
            CommonName:   "kube-apiserver",
            Organization: []string{"system:masters"},
        },
        ServiceCIDR: cfg.Networking.ServiceSubnet,
        DNSDomain:   cfg.Networking.DNSDomain,
    }
    return certs.GenerateAPIServerCert(apiServerCfg, cfg.CertificatesDir)
}
```

---

## API Server 证书 SAN

API Server 证书包含以下 Subject Alternative Names (SAN):

```go
[]string{
    "kubernetes",
    "kubernetes.default",
    "kubernetes.default.svc",
    "kubernetes.default.svc.cluster.local",
    // Service CIDR 首 IP
    "10.0.0.1",
    // localhost
    "127.0.0.1",
    // API Server 节点 IP
    "192.168.1.10",
    // 负载均衡器 IP (如果配置了 HA)
    "192.168.1.100",
}
```

---

## 证书轮换

```bash
# 查看证书过期时间
kubeadm alpha certs check-expiration

# 轮换所有证书
kubeadm alpha certs renew all

# 轮换特定证书
kubeadm alpha certs renew apiserver
kubeadm alpha certs renew etcd-server
kubeadm alpha certs renew front-proxy-client
kubeadm alpha certs renew sa

# 仅重新生成 CA (危险，会导致所有证书失效)
kubeadm alpha certs renew ca
```

---

## 证书路径结构

```
/etc/kubernetes/pki/
├── ca.crt
├── ca.key
├── apiserver.crt
├── apiserver.key
├── apiserver-kubelet-client.crt
├── apiserver-kubelet-client.key
├── apiserver-kubelet-serving.crt     # kubelet serving 证书
├── apiserver-kubelet-serving.key
├── front-proxy-ca.crt
├── front-proxy-ca.key
├── front-proxy-client.crt
├── front-proxy-client.key
├── sa.pub
├── sa.key
└── etcd/
    ├── ca.crt
    ├── ca.key
    ├── server.crt
    ├── server.key
    ├── peer.crt
    ├── peer.key
    ├── healthcheck-client.crt
    └── healthcheck-client.key
```
