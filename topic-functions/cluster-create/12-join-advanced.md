# 节点加入进阶: CSR 与 Bootstrap 机制详解

## 源码路径

`cmd/kubeadm/app/phases/bootstraptoken/`
`pkg/controller/certificates/`
`cmd/kubeadm/app/phases/kubelet/config.go`

---

## Bootstrap Token 完整生命周期

```go
// 创建 Bootstrap Token (init 阶段)
type BootstrapToken struct {
    Token       string   // <token-id>.<token-secret>
    TTL         time.Duration  // 默认 24h
    Usages      []string  // ["signing", "authentication"]
    Groups      []string  // ["system:bootstrappers:kubeadm:default-node-token"]
    SecretName  string    // 存储在 kube-system secret 中
}
```

**默认 Token 24 小时后过期**，过期后节点无法再加入集群。

---

## Token 存储位置

```bash
# 存储为 Secret
kubectl get secrets -n kube-system
# 输出:
# NAME                     TYPE                      DATA   AGE
# bootstrap-token-abc123    bootstrap.kubernetes.io/token   6     24h

# 查看 Token 详情
kubectl get secret bootstrap-token-abc123 -n kube-system -o yaml
```

---

## CSR 自动审批机制

当 kubelet 使用 Bootstrap Token 发起 CSR 时，`csrapproving` controller 自动处理:

```go
// pkg/controller/certificates/approval.go
func ReconcileCSRReviewed(csr *CertificatesV1.CertificateSigningRequest) error {
    // 1. 检查 CSR 的 signerName 是否为 kubernetes.io/kube-apiserver-client
    // 2. 检查 CSR 请求的 identity 是否来自有效的 Bootstrap Token
    // 3. 检查请求的 groups 是否包含 system:bootstrappers:kubeadm:default-node-token
    // 4. 自动 approve CSR
    // 5. 签发证书，有效期 1 年
}
```

---

## CSR 状态流转

```
Pending
  ↓ (csrapproving controller)
Approved
  ↓ (controller manager)
Issued
  ↓
kubelet 收到证书，存储到 /var/lib/kubelet/pki/kubelet-client-current.pem
```

---

## Bootstrap Token Groups 解析

| Group | 含义 |
|-------|------|
| `system:bootstrappers:kubeadm:default-node-token` | kubeadm 创建的临时组，用于 Bootstrap 阶段 |
| `system:nodes` | 正式节点组，证书签发后 kubelet 的主要身份 |

**安全设计**: Bootstrap Token 只在节点加入时使用，签发正式证书后，Token 可以安全地删除或过期。

---

## kubelet 首次启动完整流程

```
1. kubelet 读取 /etc/kubernetes/bootstrap-kubelet.conf
   ↓
2. 包含 API Server 地址 + CA cert + Bootstrap Token
   ↓
3. 向 API Server 发送 CSR (CertificateSigningRequest)
   ↓
4. csrapproving controller 验证 Token 有效，自动 approve
   ↓
5. kubelet 收到签发的证书
   ↓
6. 证书写入 /var/lib/kubelet/pki/kubelet-client-current.pem
   ↓
7. kubelet 重启，使用正式证书
   ↓
8. 读取 /etc/kubernetes/kubelet.conf (不再需要 bootstrap)
```

---

## Node 对象创建

当 kubelet 获得正式证书后，向 API Server 注册节点:

```go
// kubelet 注册时创建 Node 对象:
node := &v1.Node{
    Spec: v1.NodeSpec{
        PodCIDR:  <node-pod-cidr>,
        Taints:   []v1.Taint{*masterTaint},
    },
    Status: v1.NodeStatus{
        Addresses: []v1.NodeAddress{
            {Type: v1.NodeInternalIP, Address: <node-ip>},
            {Type: v1.NodeHostName, Address: <hostname>},
        },
        NodeInfo: v1.NodeSystemInfo{
            KubeletVersion:  v1.28.0,
            ContainerRuntimeVersion: containerd://1.7.x,
            OperatingSystem: linux,
            OSImage: Ubuntu 22.04,
        },
    },
}
```

---

## NodeRestriction 对 kubelet 的限制

NodeRestriction 插件确保 kubelet 只能管理自己的节点:

```go
// kubelet 只能:
// ✅ 创建/更新自己节点的 Node 资源
// ✅ 创建/更新自己节点的 Pod 资源
// ✅ 设置 node.kubernetes.io/* 注解
// ✅ 设置 kubelet.kubernetes.io/* 注解

// kubelet 不能:
// ❌ 创建/修改其他节点的 Node
// ❌ 创建/修改 kube-system 命名空间的 Pod
// ❌ 设置其他节点特有的注解
```

---

## 手动批准 CSR

如果 csrapproving controller 未自动批准，可以手动:

```bash
# 查看待批准的 CSR
kubectl get csr

# 手动批准
kubectl certificate approve <csr-name>

# 拒绝 CSR
kubectl certificate deny <csr-name>
```

---

## 证书轮换

kubelet 自动处理证书轮换:

```go
// 当证书剩余有效期 < 80% 时，kubelet 自动发起新的 CSR
// 旧证书在所有引用结束后自动失效
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| CSR 一直是 Pending | csrapproving 未运行 | 检查 controller manager 日志 |
| `invalid token` | Token 过期 | `kubeadm token create` 生成新 Token |
| `node not found` | kubelet 未完成注册 | 检查 kubelet 日志 |
| `certificate signed by unknown authority` | CA 证书不匹配 | 检查 /etc/kubernetes/pki/ca.crt |
