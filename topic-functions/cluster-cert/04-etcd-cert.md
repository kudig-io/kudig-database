# etcd 证书体系源码分析

## 概述

etcd 作为 Kubernetes 的数据存储后端，拥有独立的 PKI 体系。etcd 集群内部节点间通过 Peer 证书通信，外部客户端（如 API Server）通过 etcd Client 证书访问。本文档基于 kubeadm 源码，分析 etcd 证书的设计与生成逻辑。

---

## 源码路径

- **etcd 证书定义**: `cmd/kubeadm/app/phases/certs/certs.go`
- **etcd 本地启动**: `cmd/kubeadm/app/phases/etcd/local.go`
- **PKI 工具**: `cmd/kubeadm/app/util/pkiutil/pki_helpers.go`

---

## etcd 证书体系架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      etcd 证书体系                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────┐                                            │
│  │    etcd-ca      │                                            │
│  │   (独立根 CA)    │                                            │
│  └────────┬────────┘                                            │
│           │                                                      │
│     ┌─────┼─────┐                                                │
│     ▼     ▼     ▼                                                │
│  ┌─────┐┌─────┐┌─────────────┐                                 │
│  │etcd ││etcd ││  API Server │                                 │
│  │Peer ││Server││  (etcd client)│                                │
│  │cert ││cert ││   cert      │                                 │
│  └──┬──┘└──┬──┘└──────┬──────┘                                 │
│     │      │          │                                         │
│     └──────┴──────────┘                                         │
│            │                                                      │
│     ┌──────┴──────┐                                              │
│     ▼             ▼                                              │
│  etcd 节点间通信   外部客户端访问                                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## etcd 证书定义

### 1. etcd CA

```go
// cmd/kubeadm/app/phases/certs/certs.go
var KubeadmCertEtcdCA = &KubeadmCert{
    Name:     "etcd-ca",
    LongName: "etcd certificate authority",
    BaseName: "ca",
    Config: certutil.Config{
        CommonName: "etcd-ca",
    },
}
```

**存储路径**: `/etc/kubernetes/pki/etcd/ca.crt` / `ca.key`

### 2. etcd Server 证书

```go
var KubeadmCertEtcdServer = &KubeadmCert{
    Name:     "etcd-server",
    LongName: "certificate for serving the etcd API",
    BaseName: "server",
    CAName:   "etcd-ca",
    Config: certutil.Config{
        CommonName: "etcd-server",
        Usages:     []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth, x509.ExtKeyUsageClientAuth},
    },
}
```

**关键差异**：
- `Usages: ServerAuth | ClientAuth` — etcd server 同时需要服务端和客户端认证用途
- 原因：etcd 在 Peer 通信中也可能作为客户端发起连接

**SAN 生成**：
```go
func GetEtcdAltNames(cfg *kubeadmapi.InitConfiguration) (*certutil.AltNames, error) {
    altNames := &certutil.AltNames{
        DNSNames: []string{
            cfg.NodeRegistration.Name,
            "localhost",
        },
        IPs: []net.IP{
            cfg.LocalAPIEndpoint.AdvertiseAddress,
            net.IPv4(127, 0, 0, 1),
            net.IPv6loopback,
        },
    }
    // 添加用户自定义 SAN
    for _, san := range cfg.Etcd.Local.ExtraArgs["listen-client-urls"] {
        // 解析 URL 中的 IP
    }
    return altNames, nil
}
```

### 3. etcd Peer 证书

```go
var KubeadmCertEtcdPeer = &KubeadmCert{
    Name:     "etcd-peer",
    LongName: "certificate for etcd nodes to communicate with each other",
    BaseName: "peer",
    CAName:   "etcd-ca",
    Config: certutil.Config{
        CommonName: "etcd-peer",
        Usages:     []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth, x509.ExtKeyUsageClientAuth},
    },
}
```

**用途**：
- etcd 集群节点间的 Raft 通信
- 节点加入集群时的身份验证
- 同样需要 `ServerAuth | ClientAuth` 双用途

### 4. etcd Healthcheck Client 证书

```go
var KubeadmCertEtcdHealthcheck = &KubeadmCert{
    Name:     "etcd-healthcheck-client",
    LongName: "certificate for liveness probes to healthcheck etcd",
    BaseName: "healthcheck-client",
    CAName:   "etcd-ca",
    Config: certutil.Config{
        CommonName: "kube-etcd-healthcheck-client",
        Organization: []string{"system:masters"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    },
}
```

**用途**：
- kubeadm/kubectl 对 etcd 进行健康检查
- `Organization: system:masters` — 确保具有 etcd 访问权限

---

## etcd 证书与 API Server 的关系

### API Server 作为 etcd 客户端

```go
var KubeadmCertApiserverEtcdClient = &KubeadmCert{
    Name:     "apiserver-etcd-client",
    CAName:   "etcd-ca",  // 由 etcd-ca 签发
    Config: certutil.Config{
        CommonName:   "kube-apiserver-etcd-client",
        Organization: []string{"system:masters"},
        Usages:       []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth},
    },
}
```

**信任链**：
```
API Server ──► etcd
  携带: apiserver-etcd-client.crt
  etcd 使用: etcd/ca.crt 验证
```

**API Server 启动参数中的 etcd TLS 配置**：
```
--etcd-cafile=/etc/kubernetes/pki/etcd/ca.crt
--etcd-certfile=/etc/kubernetes/pki/apiserver-etcd-client.crt
--etcd-keyfile=/etc/kubernetes/pki/apiserver-etcd-client.key
--etcd-servers=https://127.0.0.1:2379
```

---

## etcd 静态 Pod 中的证书挂载

kubeadm 生成的 etcd 静态 Pod manifest 中，证书以 hostPath 卷挂载：

```yaml
# /etc/kubernetes/manifests/etcd.yaml (片段)
spec:
  containers:
  - name: etcd
    command:
    - etcd
    - --cert-file=/etc/kubernetes/pki/etcd/server.crt
    - --key-file=/etc/kubernetes/pki/etcd/server.key
    - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --client-cert-auth=true
    - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
    - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
    - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --peer-client-cert-auth=true
    volumeMounts:
    - mountPath: /etc/kubernetes/pki/etcd
      name: etcd-certs
      readOnly: true
  volumes:
  - hostPath:
      path: /etc/kubernetes/pki/etcd
      type: DirectoryOrCreate
    name: etcd-certs
```

---

## etcd 证书验证实践

```bash
# 1. 查看 etcd Server 证书
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -text

# 2. 验证 etcd 证书链
openssl verify -CAfile /etc/kubernetes/pki/etcd/ca.crt \
  /etc/kubernetes/pki/etcd/server.crt

# 3. 检查 etcd 健康 (使用客户端证书)
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 4. 检查 etcd 集群成员
ETCDCTL_API=3 etcdctl member list \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
```

---

## 外部 etcd 集群的证书管理

当使用外部 etcd 集群时，kubeadm 不管理 etcd 证书：

```yaml
# kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
etcd:
  external:
    endpoints:
      - https://192.168.1.10:2379
      - https://192.168.1.11:2379
      - https://192.168.1.12:2379
    caFile: /etc/kubernetes/pki/etcd/ca.crt
    certFile: /etc/kubernetes/pki/apiserver-etcd-client.crt
    keyFile: /etc/kubernetes/pki/apiserver-etcd-client.key
```

**外部 etcd 模式下**：
- kubeadm 只生成 `apiserver-etcd-client` 证书
- etcd CA 和 etcd 服务端证书由外部系统管理
- 需要手动确保证书同步和轮换

---

## etcd 证书常见问题

| 问题 | 现象 | 根因 | 解决 |
|-----|------|------|------|
| etcd 证书过期 | `authentication handshake failed: x509: certificate has expired` | 超过 1 年有效期 | `kubeadm certs renew etcd-server` |
| Peer 通信失败 | `remote error: tls: bad certificate` | Peer 证书 SAN 不包含节点 IP | 重新生成包含所有节点 IP 的 Peer 证书 |
| API Server 连不上 etcd | `etcdserver: request timed out` | apiserver-etcd-client 证书问题 | 检查证书有效期和 CA 匹配 |
| etcd 数据损坏 | 集群无法启动 | 证书与密钥不匹配 | 从备份恢复或使用 kubeadm 重新生成 |
