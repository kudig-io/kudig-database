# kubeconfig 中的证书嵌入逻辑

## 概述

kubeadm 在证书阶段不仅生成 `.crt`/`.key` 文件，还会生成多个 kubeconfig 文件。这些 kubeconfig 文件将 **CA 证书和客户端证书以 Base64 编码直接嵌入**，使组件无需依赖外部证书文件即可建立 TLS 连接。本文档基于 `cmd/kubeadm/app/phases/kubeconfig` 源码，分析证书嵌入的完整逻辑。

---

## 源码路径

- **kubeconfig 生成主控**: `cmd/kubeadm/app/phases/kubeconfig/kubeconfig.go`
- **kubeconfig 工具**: `cmd/kubeadm/app/util/kubeconfig/kubeconfig.go`
- **客户端证书创建**: `cmd/kubeadm/app/phases/certs/certs.go`

---

## kubeadm 生成的 kubeconfig 列表

| kubeconfig 文件 | 使用者 | 证书来源 | 存储路径 |
|----------------|-------|---------|---------|
| `admin.conf` | 集群管理员 (kubectl) | `kubernetes-ca` 签发 | `/etc/kubernetes/admin.conf` |
| `controller-manager.conf` | kube-controller-manager | `kubernetes-ca` 签发 | `/etc/kubernetes/controller-manager.conf` |
| `scheduler.conf` | kube-scheduler | `kubernetes-ca` 签发 | `/etc/kubernetes/scheduler.conf` |
| `kubelet.conf` | kubelet (正式) | CSR 动态签发 | `/etc/kubernetes/kubelet.conf` |

**注意**：`kubelet.conf` 在 `kubeadm init` 阶段**不生成**，而是由 kubelet 通过 Bootstrap 机制在首次启动时自动创建。

---

## kubeconfig 证书嵌入源码分析

### 1. 生成 kubeconfig 的主控函数

```go
// cmd/kubeadm/app/phases/kubeconfig/kubeconfig.go
func CreateJoinControlPlaneKubeConfigFiles(...) error {
    // 定义需要生成的 kubeconfig 列表
    files := []string{
        kubeadmconstants.AdminKubeConfigFileName,
        kubeadmconstants.ControllerManagerKubeConfigFileName,
        kubeadmconstants.SchedulerKubeConfigFileName,
    }
    
    for _, file := range files {
        // 生成每个 kubeconfig
        if err := createKubeConfigFile(...); err != nil {
            return err
        }
    }
    return nil
}
```

### 2. 构建 kubeconfig 的核心逻辑

```go
// cmd/kubeadm/app/phases/kubeconfig/kubeconfig.go
func buildKubeConfigFromSpec(spec *kubeConfigSpec) (*clientcmdapi.Config, error) {
    // 1. 读取或生成客户端证书
    clientCert, clientKey, err := getOrCreateClientCert(spec)
    
    // 2. 读取 CA 证书
    caCert, err := os.ReadFile(spec.CAPath)
    
    // 3. 构建 kubeconfig 结构
    config := &clientcmdapi.Config{
        Clusters: map[string]*clientcmdapi.Cluster{
            spec.ClusterName: {
                Server:                   spec.APIServerEndpoint,
                CertificateAuthorityData: caCert,  // ← CA 证书嵌入
            },
        },
        AuthInfos: map[string]*clientcmdapi.AuthInfo{
            spec.ClientName: {
                ClientCertificateData: clientCert,  // ← 客户端证书嵌入
                ClientKeyData:         clientKey,   // ← 客户端私钥嵌入
            },
        },
        Contexts: map[string]*clientcmdapi.Context{
            spec.ClientName + "@" + spec.ClusterName: {
                Cluster:  spec.ClusterName,
                AuthInfo: spec.ClientName,
            },
        },
        CurrentContext: spec.ClientName + "@" + spec.ClusterName,
    }
    
    return config, nil
}
```

**关键设计**：
- `CertificateAuthorityData` — CA 证书以 **Base64 编码** 嵌入
- `ClientCertificateData` — 客户端证书以 **Base64 编码** 嵌入
- `ClientKeyData` — 客户端私钥以 **Base64 编码** 嵌入
- 组件启动时无需挂载 hostPath 证书卷，只需读取 kubeconfig 文件

---

## 证书嵌入 vs 文件引用的对比

### 嵌入模式（kubeadm 默认）

```yaml
apiVersion: v1
kind: Config
clusters:
- cluster:
    certificate-authority-data: LS0tLS1CRUdJTi...  # Base64 编码的 CA
    server: https://192.168.1.10:6443
  name: kubernetes
users:
- name: kubernetes-admin
  user:
    client-certificate-data: LS0tLS1CRUdJTi...    # Base64 编码的证书
    client-key-data: LS0tLS1CRUdJTi...            # Base64 编码的私钥
```

### 引用模式（手动配置时可选）

```yaml
clusters:
- cluster:
    certificate-authority: /etc/kubernetes/pki/ca.crt  # 文件路径引用
    server: https://192.168.1.10:6443
  name: kubernetes
users:
- name: kubernetes-admin
  user:
    client-certificate: /etc/kubernetes/pki/admin.crt
    client-key: /etc/kubernetes/pki/admin.key
```

**对比**：

| 特性 | 嵌入模式 | 引用模式 |
|-----|---------|---------|
| 文件数量 | 单个 kubeconfig 文件 | kubeconfig + 多个证书文件 |
| 分发便利性 | 高（单文件即可） | 低（需同步多个文件） |
| 证书更新 | 需重新生成 kubeconfig | 替换证书文件即可 |
| 安全性 | 私钥在文件中（风险较高） | 私钥可单独设置权限 |
| kubeadm 默认 | **是** | 否 |

---

## 各组件 kubeconfig 的证书配置

### 1. admin.conf

```go
// cmd/kubeadm/app/phases/kubeconfig/kubeconfig.go
// admin kubeconfig 的证书配置
&KubeadmCertAdmin.Config
// CommonName: "kubernetes-admin"
// Organization: ["system:masters"]
```

**用途**：
- kubectl 默认使用 `~/.kube/config`（通常从 admin.conf 复制）
- 具有集群完全控制权限

### 2. controller-manager.conf

```go
// CommonName: "system:kube-controller-manager"
// Organization: ["system:kube-controller-manager"]
```

**Controller Manager 启动参数**：
```bash
--kubeconfig=/etc/kubernetes/controller-manager.conf
--authentication-kubeconfig=/etc/kubernetes/controller-manager.conf
--authorization-kubeconfig=/etc/kubernetes/controller-manager.conf
```

### 3. scheduler.conf

```go
// CommonName: "system:kube-scheduler"
// Organization: ["system:kube-scheduler"]
```

**Scheduler 启动参数**：
```bash
--kubeconfig=/etc/kubernetes/scheduler.conf
--authentication-kubeconfig=/etc/kubernetes/scheduler.conf
--authorization-kubeconfig=/etc/kubernetes/scheduler.conf
```

---

## kubeconfig 证书轮换的特殊性

### kubeadm certs renew 对 kubeconfig 的处理

```go
// cmd/kubeadm/app/phases/certs/renew.go
func renewKubeConfigCert(cfg *kubeadmapi.InitConfiguration, cert *certs.KubeadmCert) error {
    // 1. 生成新客户端证书
    newCert, newKey, err := generateNewClientCert(cfg, cert)
    
    // 2. 读取现有 kubeconfig
    kubeConfig, err := clientcmd.LoadFromFile(cert.KubeConfigFile)
    
    // 3. 替换嵌入的证书和私钥
    authInfo := kubeConfig.AuthInfos[cert.ClientName]
    authInfo.ClientCertificateData = certToPEM(newCert)
    authInfo.ClientKeyData = keyToPEM(newKey)
    
    // 4. 写回文件
    return clientcmd.WriteToFile(*kubeConfig, cert.KubeConfigFile)
}
```

**关键区别**：
- 轮换 `.crt`/`.key` 文件时，只需重写证书文件
- 轮换 kubeconfig 时，需要 **解析 YAML → 替换 Base64 字段 → 重写 YAML**
- kubeadm 会自动处理这一逻辑

---

## 手动更新 kubeconfig 中的证书

```bash
# 1. 提取 kubeconfig 中的证书信息
kubectl config view --raw

# 2. 提取并解码 CA 证书
kubectl config view --raw -o jsonpath='{.clusters[0].cluster.certificate-authority-data}' | base64 -d > ca-from-kubeconfig.crt

# 3. 提取并解码客户端证书
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d > client-from-kubeconfig.crt

# 4. 查看客户端证书有效期
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -enddate

# 5. 更新 kubeconfig 中的 CA（如果需要）
kubectl config set-cluster kubernetes \
  --certificate-authority=/etc/kubernetes/pki/ca.crt \
  --embed-certs=true

# 6. 更新 kubeconfig 中的客户端证书
kubectl config set-credentials kubernetes-admin \
  --client-certificate=/etc/kubernetes/pki/admin.crt \
  --client-key=/etc/kubernetes/pki/admin.key \
  --embed-certs=true
```

---

## 高可用集群中的 kubeconfig 证书

### 外部负载均衡场景

```yaml
# admin.conf 中的 server 地址
clusters:
- cluster:
    server: https://lb.example.com:6443  # 负载均衡地址
```

**证书要求**：
- `apiserver.crt` 的 SAN 必须包含 `lb.example.com`
- 否则 kubectl 会报告 `x509: certificate is valid for ...`

**修复方式**：
```bash
# 在 kubeadm-config 中添加 certSANs，然后重新生成 API Server 证书
kubeadm init phase certs apiserver --config kubeadm-config.yaml
```

---

## 故障排查

| 问题 | 现象 | 排查 |
|-----|------|------|
| kubeconfig 证书过期 | `Unable to connect to the server: x509: certificate has expired` | `kubectl config view` 解码证书检查有效期 |
| CA 不匹配 | `x509: certificate signed by unknown authority` | 对比 kubeconfig 中的 CA 与 `/etc/kubernetes/pki/ca.crt` |
| 证书链不完整 | `x509: certificate has expired or is not yet valid` | 检查系统时间与证书有效期 |
| 私钥不匹配 | `tls: private key does not match public key` | 验证 modulus 一致性 |
| 权限不足 | `Error from server (Forbidden)` | 检查证书中的 CN/O 对应的 RBAC 权限 |
