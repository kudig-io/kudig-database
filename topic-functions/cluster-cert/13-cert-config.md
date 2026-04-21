# kubeadm 配置对证书生成的影响

## 概述

kubeadm 的 `InitConfiguration` 和 `ClusterConfiguration` 中有多个字段直接影响证书的生成结果。理解这些配置项是正确部署集群、特别是高可用和外部访问场景下确保证书有效的前提。

---

## 源码路径

- **配置定义**: `cmd/kubeadm/app/apis/kubeadm/v1beta3/types.go`
- **配置加载**: `cmd/kubeadm/app/util/config/initconfiguration.go`
- **证书阶段**: `cmd/kubeadm/app/phases/certs/certs.go`

---

## 核心配置字段

### 1. CertificatesDir — 证书存储目录

```go
// cmd/kubeadm/app/apis/kubeadm/v1beta3/types.go
type InitConfiguration struct {
    // 证书和密钥的存储目录
    // 默认: "/etc/kubernetes/pki"
    CertificatesDir string
}
```

**配置示例**：
```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
certificatesDir: /etc/kubernetes/pki
```

**影响**：
- 所有 `.crt`、`.key`、`.conf` 文件的输出目录
- 外部 CA 模式下，kubeadm 从此目录读取预先放置的 CA 证书

---

### 2. API Server 公告地址

```go
type InitConfiguration struct {
    LocalAPIEndpoint APIEndpoint
}

type APIEndpoint struct {
    // API Server 的公告地址（其他组件用于连接 API Server）
    AdvertiseAddress string
    // API Server 绑定端口
    BindPort int32
}
```

**配置示例**：
```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 192.168.1.10
  bindPort: 6443
```

**对证书的影响**：
- `advertiseAddress` 自动加入 `apiserver.crt` 的 **IP SAN** 列表
- 这是 API Server 证书生成逻辑中的默认 SAN 之一

---

### 3. certSANs — 自定义 API Server 证书 SAN

```go
// cmd/kubeadm/app/apis/kubeadm/v1beta3/types.go
type ClusterConfiguration struct {
    APIServer APIServer
}

type APIServer struct {
    // 额外的 Subject Alternative Names
    CertSANs []string
}
```

**配置示例**：
```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
apiServer:
  certSANs:
    - "192.168.1.10"
    - "192.168.1.11"
    - "192.168.1.12"
    - "10.0.0.1"
    - "lb.example.com"
    - "kubernetes"
    - "kubernetes.default"
    - "kubernetes.default.svc"
    - "kubernetes.default.svc.cluster.local"
```

**对证书的影响**：
- 列表中的每个 IP 被加入 `apiserver.crt` 的 `IPAddresses` 字段
- 列表中的每个 DNS 名被加入 `apiserver.crt` 的 `DNSNames` 字段
- **高可用集群必须配置负载均衡 IP/域名**，否则外部访问会报 SAN 不匹配

**源码中的 SAN 收集**：
```go
// cmd/kubeadm/app/phases/certs/certs.go
func GetAPIServerAltNames(cfg *kubeadmapi.InitConfiguration) (*certutil.AltNames, error) {
    altNames := &certutil.AltNames{}
    
    // 自动添加的 SAN:
    // 1. 节点主机名
    // 2. "kubernetes", "kubernetes.default", "kubernetes.default.svc", "kubernetes.default.svc.<DNSDomain>"
    // 3. advertiseAddress
    // 4. Service CIDR 的第一个 IP
    
    // 用户自定义 SAN:
    for _, san := range cfg.APIServer.CertSANs {
        if ip := net.ParseIP(san); ip != nil {
            altNames.IPs = append(altNames.IPs, ip)
        } else {
            altNames.DNSNames = append(altNames.DNSNames, san)
        }
    }
    
    return altNames, nil
}
```

---

### 4. Networking — 网络配置

```go
type ClusterConfiguration struct {
    Networking Networking
}

type Networking struct {
    // Service 子网 CIDR
    // 默认: "10.96.0.0/12"
    ServiceSubnet string
    
    // Pod 子网 CIDR
    PodSubnet string
    
    // 集群 DNS 域名
    // 默认: "cluster.local"
    DNSDomain string
}
```

**配置示例**：
```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "10.244.0.0/16"
  dnsDomain: "cluster.local"
```

**对证书的影响**：
- `ServiceSubnet` 的第一个 IP（如 `10.96.0.1`）自动加入 `apiserver.crt` 的 SAN
- 这是 `kubernetes.default.svc` Service 的 ClusterIP
- `DNSDomain` 影响默认 SAN：`kubernetes.default.svc.cluster.local`

---

### 5. etcd 外部配置

```go
type ClusterConfiguration struct {
    Etcd Etcd
}

type Etcd struct {
    // 本地 etcd（kubeadm 管理）
    Local *LocalEtcd
    // 外部 etcd（用户自行管理）
    External *ExternalEtcd
}
```

**外部 etcd 配置示例**：
```yaml
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

**对证书的影响**：
- 外部 etcd 模式下，kubeadm **不生成 etcd CA 和 etcd 服务端证书**
- 只生成 `apiserver-etcd-client` 证书（用于 API Server 连接外部 etcd）
- etcd 集群的证书由外部系统管理

---

### 6. ControlPlaneEndpoint — 高可用控制面端点

```go
type ClusterConfiguration struct {
    // 控制面的稳定端点（负载均衡地址）
    ControlPlaneEndpoint string
}
```

**配置示例**：
```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
controlPlaneEndpoint: "lb.example.com:6443"
```

**对证书的影响**：
- 自动加入 `apiserver.crt` 的 DNS SAN（如果值是域名）
- 如果是 IP，自动加入 IP SAN
- **注意**：与 `certSANs` 不同，此字段还会被写入各组件的 kubeconfig `server` 字段

---

## 配置验证实践

```bash
# 1. 查看当前 kubeadm 配置
cat /etc/kubernetes/kubeadm-config.yaml

# 2. 从 ConfigMap 导出配置
kubectl get cm kubeadm-config -n kube-system -o yaml

# 3. 导出 kubelet 配置
kubectl get cm kubelet-config -n kube-system -o yaml

# 3. 验证 certSANs 是否生效
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -ext subjectAltName

# 4. 使用 kubeadm 验证配置
kubeadm config validate --config kubeadm-config.yaml

# 5. 查看配置中的证书有效期设置（如自定义）
kubectl get cm kubeadm-config -n kube-system -o jsonpath='{.data.ClusterConfiguration}' | grep -A5 certificate
```

---

## kubeadm upgrade 与证书配置

`kubeadm upgrade` 会读取升级后的 kubeadm 配置，但不会自动重新生成证书。如果配置变更涉及证书相关字段（如新增 `certSANs`），需要**手动触发证书重生成**。

```bash
# upgrade 后如发现 SAN 缺失，需要：
# 1. 更新 kubeadm-config ConfigMap
kubectl edit cm kubeadm-config -n kube-system

# 2. 重新生成受影响的证书
kubeadm init phase certs apiserver --config /etc/kubernetes/kubeadm-config.yaml

# 3. 重启组件
systemctl restart kubelet
```

**注意**：`kubeadm upgrade` 不会覆盖已有证书（幂等性），因此配置变更不会自动反映到证书上。

---

## 修改配置后的证书重生成

### 场景：添加新的 SAN

```bash
# 1. 备份现有证书
sudo cp -r /etc/kubernetes/pki /etc/kubernetes/pki.backup.$(date +%Y%m%d)
sudo cp /etc/kubernetes/*.conf /etc/kubernetes/conf.backup.$(date +%Y%m%d)

# 2. 修改 kubeadm-config.yaml，添加新的 certSANs
sudo vi /etc/kubernetes/kubeadm-config.yaml

# 3. 重新生成 API Server 证书
sudo kubeadm init phase certs apiserver --config /etc/kubernetes/kubeadm-config.yaml

# 4. 重新生成 admin kubeconfig（因为证书变了）
sudo kubeadm init phase kubeconfig admin --config /etc/kubernetes/kubeadm-config.yaml

# 5. 更新本地 kubectl 配置
sudo cp /etc/kubernetes/admin.conf ~/.kube/config
sudo chown $(id -u):$(id -g) ~/.kube/config

# 6. 重启 API Server
sudo systemctl restart kubelet

# 7. 验证新 SAN
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -ext subjectAltName
```

---

## 完整配置模板（生产级）

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 192.168.1.10
  bindPort: 6443
certificatesDir: /etc/kubernetes/pki
nodeRegistration:
  name: master-1
  criSocket: unix:///var/run/containerd/containerd.sock
---
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
clusterName: production-cluster
kubernetesVersion: v1.32.0
controlPlaneEndpoint: "lb.example.com:6443"
networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "10.244.0.0/16"
  dnsDomain: "cluster.local"
apiServer:
  certSANs:
    - "192.168.1.10"
    - "192.168.1.11"
    - "192.168.1.12"
    - "10.96.0.1"
    - "lb.example.com"
    - "kubernetes"
    - "kubernetes.default"
    - "kubernetes.default.svc"
    - "kubernetes.default.svc.cluster.local"
  extraArgs:
    audit-log-path: "/var/log/kubernetes/audit.log"
scheduler:
  extraArgs:
    bind-address: "0.0.0.0"
controllerManager:
  extraArgs:
    bind-address: "0.0.0.0"
---
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
rotateCertificates: true
serverTLSBootstrap: true
```
