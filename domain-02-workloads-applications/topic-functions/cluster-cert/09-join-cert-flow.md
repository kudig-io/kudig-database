---
title: kubeadm join 证书分发流程 (topic-code-analysis)
description: 'description: ''## 概述'''
summary: 'description: ''## 概述'''
category: general
tags:
- reference
- etcd
- apiserver
- kubelet
- scheduler
- rbac
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- kubeadm join 证书分发流程 是什么
- 如何 kubeadm join 证书分发流程
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- kubeadm
- join
- 证书分发流程
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: kubeadm join 证书分发流程
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- scheduler
- rbac
last_updated: '2026-05-18'
difficulty: intermediate
reading_level: intermediate
audience:
- Kubernetes 管理员
- 集群运维人员
- SRE 工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes kubeadm join 证书获取流程 Bootstrap Token
- 节点加入集群 CA 发现 discovery-token-ca-cert-hash
- kubeadm join Control Plane 高可用 证书复制
- kubelet Bootstrap kubeconfig 生成流程
- CSR 自动审批 node-autoapprove-clusterrolebinding
trigger_keywords:
- kubeadm join
- Bootstrap Token
- cluster-info
- CA 发现
- certificate-key
- kubeadm-certs
- Control Plane
- HA
- CSR 自动审批
- node-autoapprove
related_domains:
- domain-01-cluster-fundamentals
- domain-4-nodes
related_topics:
- cluster-cert/pki-architecture
- cluster-cert/kubelet-cert
- cluster-cert/cert-rotation
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# kubeadm join 证书分发流程

## 概述

kubeadm join 是新节点加入 Kubernetes 集群的标准流程。与 `kubeadm init` 生成所有证书不同，`join` 的核心任务是**安全地获取节点运行所需的凭证**，而非自行生成。理解这一流程对排查节点加入失败、证书分发异常至关重要。

---

## 源码路径

- **join 命令入口**: `cmd/kubeadm/app/cmd/join.go`
- **Bootstrap Token 阶段**: `cmd/kubeadm/app/phases/bootstraptoken/node/tls.go`
- **kubelet 引导配置**: `cmd/kubeadm/app/phases/kubelet/config.go`
- **CSR 自动审批**: `pkg/controller/certificates/`

---

## join 流程总览

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
│         │                                │                  │       │
│         │ 4. 创建 Bootstrap kubeconfig   │                  │       │
│         │    (/etc/kubernetes/bootstrap-  │                  │       │
│         │     kubelet.conf)              │                  │       │
│         │                                │                  │       │
│         │ 5. 使用 Bootstrap Token 创建 CSR│                  │       │
│         ├───────────────────────────────►│                  │       │
│         │                                │ 6. CSR 自动审批   │       │
│         │                                │    (Node Auto-    │       │
│         │                                │     Approval)     │       │
│         │◄────────────────────────────────│                  │       │
│         │                                │                  │       │
│         │ 7. 下载签发的客户端证书          │                  │       │
│         │    写入 /var/lib/kubelet/pki/   │                  │       │
│         │                                │                  │       │
│         │ 8. 生成正式 kubeconfig           │                  │       │
│         │    (/etc/kubernetes/kubelet.conf)│                │       │
│         │                                │                  │       │
│         │ 9. 加入集群完成                  │                  │       │
│         │                                │                  │       │
└─────────┴────────────────────────────────┴──────────────────┴───────┘
```

---

## 阶段详解

### 阶段 1: 准备 Bootstrap Token

```bash
# 在 Master 节点生成 join 命令
kubeadm token create --print-join-command

# 输出:
# kubeadm join 192.168.1.10:6443 \
#   --token abcdef.0123456789abcdef \
#   --discovery-token-ca-cert-hash sha256:1234abcd...
```

**Token 格式**：
- `abcdef.0123456789abcdef`
- 前半部分（`abcdef`）是 Token ID，用于标识
- 后半部分（`0123456789abcdef`）是 Secret，用于认证

**Token 生命周期**：
- 默认有效期：**24 小时**
- 过期后无法用于 join，需重新创建
- 查看现有 Token：`kubeadm token list`
- 删除过期 Token：`kubeadm token delete <token-id>`
- 创建新 Token：`kubeadm token create --print-join-command`

### 阶段 2: 发现集群 CA

```go
// cmd/kubeadm/app/discovery/token/token.go
func RetrieveValidatedConfigInfo(...) (*clientcmdapi.Config, error) {
    // 1. 创建 Insecure TLS 客户端（尚未验证 CA）
    insecureClient := &http.Client{
        Transport: &http.Transport{
            TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
        },
    }
    
    // 2. 从 kube-public/cluster-info ConfigMap 获取 CA 证书
    response, err := insecureClient.Get("https://<api-server>:6443/api/v1/namespaces/kube-public/configmaps/cluster-info")
    
    // 3. 验证 CA 指纹
    caCertHash := hashCAX509Cert(caCert)
    if caCertHash != expectedHash {
        return nil, errors.New("CA hash mismatch")
    }
    
    // 4. 返回安全的 kubeconfig
    return buildSecureConfig(caCert), nil
}
```

**cluster-info ConfigMap**：
```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-info
  namespace: kube-public
  # 该 ConfigMap 对所有用户（包括未认证）可读
data:
  kubeconfig: |
    apiVersion: v1
    kind: Config
    clusters:
    - cluster:
        certificate-authority-data: <base64-ca-cert>
        server: https://192.168.1.10:6443
      name: ""
```

**安全设计**：
- 通过 `discovery-token-ca-cert-hash` 防止中间人攻击
- 即使 `kube-public/cluster-info` 被篡改，哈希不匹配会中断加入流程

### 阶段 3: 创建 Bootstrap kubeconfig

```go
// cmd/kubeadm/app/phases/kubelet/config.go
func WriteKubeletBootstrapConfigFile(bootstrapConfigFile string, ...) error {
    bootstrapConfig := &clientcmdapi.Config{
        Clusters: map[string]*clientcmdapi.Cluster{
            "kubernetes": {
                Server:                   apiServerEndpoint,
                CertificateAuthorityData: caCertData,
            },
        },
        AuthInfos: map[string]*clientcmdapi.AuthInfo{
            "kubelet-bootstrap": {
                Token: bootstrapToken,
            },
        },
        CurrentContext: "kubelet-bootstrap-context",
        Contexts: map[string]*clientcmdapi.Context{
            "kubelet-bootstrap-context": {
                Cluster:  "kubernetes",
                AuthInfo: "kubelet-bootstrap",
            },
        },
    }
    return clientcmd.WriteToFile(*bootstrapConfig, bootstrapConfigFile)
}
```

**生成的文件**：
- `/etc/kubernetes/bootstrap-kubelet.conf`
- 仅包含 Token，不包含客户端证书

### 阶段 4: kubelet 使用 Bootstrap Token 申请证书

```go
// pkg/kubelet/certificate/bootstrap.go
func LoadClientCert(...) error {
    // 1. 读取 bootstrap-kubelet.conf
    bootstrapConfig, err := clientcmd.LoadFromFile(bootstrapPath)
    
    // 2. 使用 Token 创建临时客户端
    bootstrapClient, err := kubernetes.NewForConfig(bootstrapConfig)
    
    // 3. 生成私钥和 CSR
    privateKey, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
    csrPEM, _, err := generateCSR(privateKey, nodeName)
    
    // 4. 提交 CertificateSigningRequest
    csr := &certificates.CertificateSigningRequest{
        ObjectMeta: metav1.ObjectMeta{
            Name: csrName,
        },
        Spec: certificates.CertificateSigningRequestSpec{
            Request:    csrPEM,
            SignerName: "kubernetes.io/kube-apiserver-client-kubelet",
            Usages: []certificates.KeyUsage{
                certificates.UsageDigitalSignature,
                certificates.UsageKeyEncipherment,
                certificates.UsageClientAuth,
            },
        },
    }
    
    _, err = bootstrapClient.CertificatesV1().CertificateSigningRequests().Create(ctx, csr, metav1.CreateOptions{})
    
    // 5. 等待 CSR 被批准并获取证书
    // 轮询 CSR 状态直到 status.certificate 非空
}
```

### 阶段 5: CSR 自动审批

kubeadm init 时创建了自动审批的 ClusterRoleBinding：

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: kubeadm:node-autoapprove-bootstrap
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:certificates.k8s.io:certificatesigningrequests:nodeclient
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: Group
  name: system:bootstrappers:kubeadm:default-node-token
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: kubeadm:node-autoapprove-certificate-rotation
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:certificates.k8s.io:certificatesigningrequests:selfnodeclient
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: Group
  name: system:nodes
```

**两个审批绑定**：

| Binding | 目标 | 条件 |
|---------|------|------|
| `node-autoapprove-bootstrap` | 首次加入的节点 | 使用 Bootstrap Token，SignerName 为 `kube-apiserver-client-kubelet` |
| `node-autoapprove-certificate-rotation` | 证书轮换 | 已加入的节点（属于 `system:nodes` 组）申请新客户端证书 |

### 阶段 6: 生成正式 kubeconfig

```go
// cmd/kubeadm/app/phases/kubelet/config.go
func UpdateKubeletConfigFile(kubeletConfigFile string, ...) error {
    // 1. 读取签发的客户端证书
    clientCert, err := os.ReadFile("/var/lib/kubelet/pki/kubelet-client-current.pem")
    
    // 2. 构建正式 kubeconfig（使用证书而非 Token）
    kubeletConfig := &clientcmdapi.Config{
        Clusters: map[string]*clientcmdapi.Cluster{
            "kubernetes": {
                Server:                   apiServerEndpoint,
                CertificateAuthorityData: caCertData,
            },
        },
        AuthInfos: map[string]*clientcmdapi.AuthInfo{
            "system:node:<nodename>": {
                ClientCertificateData: clientCert,
                ClientKeyData:         privateKey,
            },
        },
    }
    
    // 3. 写入 /etc/kubernetes/kubelet.conf
    return clientcmd.WriteToFile(*kubeletConfig, kubeletConfigFile)
}
```

**最终节点上的证书文件**：
```
/etc/kubernetes/
├── bootstrap-kubelet.conf      # 引导配置（含 Token，join 后可删除）
├── kubelet.conf                # 正式配置（含客户端证书）
└── pki/
    └── ca.crt                  # 集群 CA（从 cluster-info 下载）

/var/lib/kubelet/pki/
├── kubelet-client-current.pem  # 当前客户端证书
└── kubelet-server-current.pem  # 当前服务端证书（如启用 serverTLSBootstrap）
```

---

## 高可用场景下的 join

### Control Plane 节点加入（HA）

```bash
kubeadm join <load-balancer>:6443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --control-plane \
  --certificate-key <certificate-key>
```

**与 Worker 加入的区别**：

| 维度 | Worker 加入 | Control Plane 加入 |
|-----|-----------|-------------------|
| 需要 `--control-plane` | 否 | 是 |
| 需要 `--certificate-key` | 否 | 是 |
| 从 Secret 下载的证书 | 无 | etcd CA、front-proxy CA、sa.pub |
| 生成静态 Pod manifests | 否 | 是（API Server、etcd、Controller Manager、Scheduler） |

**证书密钥（Certificate Key）**：
- kubeadm init 时使用 `--upload-certs` 上传加密证书到 Secret
- `kubeadm init phase upload-certs --upload-certs` 生成随机密钥
- 新 Control Plane 节点使用此密钥解密并下载共享证书

```go
// cmd/kubeadm/app/phases/copycerts/copycerts.go
func DownloadCerts(...) error {
    // 从 kubeadm-certs Secret 下载加密的证书包
    secret, err := client.CoreV1().Secrets("kube-system").Get(ctx, "kubeadm-certs", metav1.GetOptions{})
    
    // 使用 certificate-key 解密
    decryptedCerts, err := decryptCerts(secret.Data, certificateKey)
    
    // 写入 /etc/kubernetes/pki/
    for name, data := range decryptedCerts {
        os.WriteFile(filepath.Join(pkiDir, name), data, 0600)
    }
}
```

---

# 节点加入失败排查

## 完整排查流程图

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────┐
│                 kubeadm join 失败排查流程                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────▼─────────────┐                                   │
│  │ Step 1: 检查 Bootstrap Token │                                  │
│  └─────────────┬─────────────┘                                   │
│                │                                                  │
│         Token 有效？                                              │
│         ┌──────┴──────┐                                          │
│         ▼             ▼                                          │
│      [ 是 ]        [ 否 ]                                        │
│         │             │                                          │
│         │        kubeadm token list                              │
│         │        kubeadm token create --print-join-command       │
│         │                                                                  │
│  ┌──────▼─────────────┐                                         │
│  │ Step 2: 检查 CA 发现  │                                        │
│  └──────┬─────────────┘                                         │
│         │                                                        │
│    curl -k https://<api>:6443/api/v1/namespaces/kube-public/    │
│              configmaps/cluster-info                             │
│         │                                                        │
│    CA 指纹匹配？                                                  │
│    ┌────┴────┐                                                  │
│    ▼         ▼                                                  │
│ [ 是 ]    [ 否 ]                                                │
│    │         │                                                  │
│    │    获取正确的 discovery-token-ca-cert-hash:                 │
│    │    openssl x509 -in /etc/kubernetes/pki/ca.crt            │
│    │      -noout -pubkey | openssl rsa -pubin -outform DER      │
│    │      | sha256sum                                            │
│    │                                                                  │
│  ┌──▼─────────────┐                                             │
│  │ Step 3: 检查 CSR │                                            │
│  └──┬─────────────┘                                              │
│     │                                                             │
│     kubectl get csr                                              │
│     │                                                             │
│     CSR 存在但未批准？                                            │
│     ┌───┴───┐                                                    │
│     ▼       ▼                                                    │
│  [ 是 ]   [ 否 ]                                                 │
│     │       │                                                    │
│     │   检查 kubelet 日志                                         │
│     │   journalctl -u kubelet -f                                 │
│     │                                                             │
│     kubectl certificate approve <csr-name>                        │
│     │                                                             │
│  ┌──▼─────────────┐                                             │
│  │ Step 4: 检查证书下载 │                                        │
│  └──┬─────────────┘                                              │
│     │                                                             │
│     ls -la /var/lib/kubelet/pki/                                 │
│     │                                                             │
│     kubelet-client-current.pem 存在？                             │
│     ┌───┴───┐                                                    │
│     ▼       ▼                                                    │
│  [ 是 ]   [ 否 ]                                                 │
│     │       │                                                    │
│     │   kubelet 未成功从 CSR 下载证书                            │
│     │   检查 API Server 日志                                      │
│     │   kubectl logs -n kube-system kube-apiserver-<node>        │
│     │       | grep -i "csr|certificate"                        │
│     │                                                             │
│  ┌──▼─────────────┐                                             │
│  │ Step 5: 网络连通性 │                                           │
│  └──┬─────────────┘                                              │
│     │                                                             │
│     curl -k https://<api>:6443/healthz                           │
│     │                                                             │
│     连接成功？                                                    │
│     ┌───┴───┐                                                    │
│     ▼       ▼                                                    │
│  [ 是 ]   [ 否 ]                                                 │
│     │       │                                                    │
│     │   检查防火墙/安全组/负载均衡器                              │
│     │   检查 6443 端口是否可达                                    │
│     │                                                             │
│  ✅ 排查完成                                                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```
| 阶段 | 现象 | 排查命令 | 解决方案 |
|-----|------|---------|----------|
| CA 发现失败 | `failed to retrieve CA cert` | `kubectl get cm cluster-info -n kube-public` | 确保 API Server 可达，检查 Token |
| Token 无效 | `invalid bearer token` | `kubeadm token list` | 重新创建 Token |
| CA 指纹不匹配 | `CA hash mismatch` | `kubeadm token create --print-join-command` | 使用正确的 --discovery-token-ca-cert-hash |
| CSR 未审批 | `CSR pending approval` | `kubectl get csr` | `kubectl certificate approve <csr-name>` |
| 证书下载失败 | `unable to fetch client cert` | `journalctl -u kubelet` | 检查 API Server 日志 |
| 网络不通 | `connection refused` | `curl -k https://<api-server>:6443/healthz` | 检查网络/防火墙/负载均衡器 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 join 过程中的 CSR
kubectl get csr -w

# 手动批准 CSR
kubectl certificate approve <csr-name>

# 查看节点加入日志
journalctl -u kubelet -f

# 验证 Bootstrap Token
kubeadm token list

# 重新生成 join 命令
kubeadm token create --print-join-command
```
## Related

- [[reference|#reference Hub]] — tag hub

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/nodes.md|nodes]]


<!-- risk-assessed -->
