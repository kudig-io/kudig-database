# kubeconfig 阶段 (Kubeconfig Generation)

## 源码路径

`cmd/kubeadm/app/phases/kubeconfig/kubeconfig.go`

---

## 生成 kubeconfig 列表

| 文件 | 用途 | 使用者 |
|------|------|--------|
| `admin.conf` | 集群管理员 | kubectl, helm |
| `kubelet.conf` | Node 节点连接 API Server | kubelet |
| `controller-manager.conf` | Controller Manager 连接 API Server | kube-controller-manager |
| `scheduler.conf` | Scheduler 连接 API Server | kube-scheduler |

---

## 各组件 kubeconfig 用途

### admin.conf

```yaml
# kubectl helm 等工具使用
# 持有者: kubernetes-admin (system:masters)
clusters:
- cluster:
    certificate-authority-data: <base64(ca.crt)>
    server: https://<api-server>:6443
contexts:
- context:
    cluster: kubernetes
    user: kubernetes-admin
current-context: kubernetes-admin@kubernetes
users:
- name: kubernetes-admin
  user:
    client-certificate-data: <base64(admin.crt)>
    client-key-data: <base64(admin.key)>
```

### controller-manager.conf

```yaml
# kube-controller-manager 启动时指定:
# --kubeconfig=/etc/kubernetes/controller-manager.conf
# 用途: 访问 API Server 获取资源变化、创建 Service/Ingress、更新 Endpoint
clusters:
- cluster:
    server: https://<api-server>:6443
    certificate-authority-data: <base64(ca.crt)>
contexts:
- context:
    cluster: kubernetes
    user: system:kube-controller-manager  # 注意: 是 system:kube-controller-manager 组
current-context: system@kube-controller-manager
users:
- name: system:kube-controller-manager
  user:
    client-certificate-data: <base64(controller-manager.crt)>
    client-key-data: <base64(controller-manager.key)>
```

### scheduler.conf

```yaml
# kube-scheduler 启动时指定:
# --kubeconfig=/etc/kubernetes/scheduler.conf
# 用途: 访问 API Server 获取未调度 Pod、绑定 Pod 到节点
clusters:
- cluster:
    server: https://<api-server>:6443
    certificate-authority-data: <base64(ca.crt)>
contexts:
- context:
    cluster: kubernetes
    user: system:kube-scheduler  # 注意: 是 system:kube-scheduler 组
current-context: system@kube-scheduler
users:
- name: system:kube-scheduler
  user:
    client-certificate-data: <base64(scheduler.crt)>
    client-key-data: <base64(scheduler.key)>
```

---

## 核心代码

```go
// cmd/kubeadm/app/phases/kubeconfig/kubeconfig.go
func BuildKubeconfig(kubeconfigFile string, endpoint string, caCert []byte, clientKey []byte, clientCert []byte) error {
    config := &clientcmdapi.Config{
        Clusters: map[string]*clientcmdapi.Cluster{
            "kubernetes": {
                Server:                   endpoint,
                CertificateAuthorityData: caCert,
            },
        },
        AuthInfos: map[string]*clientcmdapi.AuthInfo{
            "default": {
                ClientCertificateData: clientCert,
                ClientKeyData:         clientKey,
            },
        },
        Contexts: map[string]*clientcmdapi.Context{
            "default": {
                Cluster:  "kubernetes",
                AuthInfo: "default",
            },
        },
        CurrentContext: "default",
    }
    return clientcmd.WriteToFile(*config, kubeconfigFile)
}
```

---

## kubelet.conf 生成逻辑

kubelet.conf 是节点加入集群后最重要的配置文件:

```go
// kubelet 启动时使用:
// 1. 如果存在 /etc/kubernetes/kubelet.conf，直接使用
// 2. 如果存在 bootstrap-kubelet.conf，使用它通过 Bootstrap Token 向 API Server 申请正式证书
// 3. 申请到的证书存储在 /var/lib/kubelet/pki/
```

---

## kubeconfig 路径结构

```
/etc/kubernetes/
├── admin.conf           # 管理员 kubeconfig
├── kubelet.conf         # kubelet 连接 API Server (正式证书)
├── controller-manager.conf
├── scheduler.conf
└── bootstrap-kubelet.conf  # Bootstrap Token 配置 (首次启动用)
```

---

## 关键: 各组件的 system: 组

| 组件 | Group | 内置 ClusterRole |
|------|-------|-----------------|
| admin | `system:masters` | cluster-admin |
| kubelet | `system:nodes` | system:node |
| controller-manager | `system:kube-controller-manager` | system:kube-controller-manager |
| scheduler | `system:kube-scheduler` | system:kube-scheduler |

这些组在证书 CN (Common Name) 中指定，API Server 根据组授予权限。

---

## admin.conf 权限

```go
// admin.conf 的 client-certificate-data 包含以下组:
Organization: []string{"system:masters"}
// system:masters 是一个内置 clusterrolebinding，绑定到 cluster-admin clusterrole
```

---

## kubeconfig 合并与切换

```bash
# 查看当前 kubeconfig
kubectl config current-context

# 查看所有 kubeconfig
kubectl config get-contexts

# 合并多个 kubeconfig
KUBECONFIG=~/.kube/config:/path/to/other/config kubectl config view --flatten

# 切换上下文
kubectl config use-context <context-name>
```
