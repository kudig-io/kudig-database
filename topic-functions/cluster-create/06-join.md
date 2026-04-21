# 节点加入流程 (kubeadm join)

## 源码路径

`cmd/kubeadm/app/cmd/join.go`
`cmd/kubeadm/app/phases/join`

---

## join 完整流程

```
┌─────────────────────────────────────────────────────────────┐
│                    kubeadm join                              │
├─────────────────────────────────────────────────────────────┤
│  1. preflight           预检 (同 init)                       │
│  2. tls-bootstrap       TLS Bootstrapping 握手               │
│  3. discovery           集群发现 (token/certificate-authority)│
│  4. kubelet-start        写入 kubelet 配置 + 启动             │
│  5. control-plane-join   (可选)如果是 control-plane 节点      │
└─────────────────────────────────────────────────────────────┘
```

---

## TLS Bootstrapping 流程

这是理解 join 的核心。当新节点上 kubelet 启动时，它没有任何证书：

```
kubelet 启动
    ↓
读取 /etc/kubernetes/bootstrap-kubelet.conf (含 Bootstrap Token)
    ↓
向 API Server 发送 Certificate Signing Request (CSR)
    ↓
API Server 验证 Bootstrap Token 有效
    ↓
Controller Manager 签发证书 (通过 csrapproving controller)
    ↓
证书写入 /var/lib/kubelet/pki/kubelet-client-current.pem
    ↓
kubelet 使用正式证书连接 API Server
```

---

## --discovery-token-ca-cert-hash

这是 join 命令中最关键的安安全参数:

```bash
# 完整 join 命令
kubeadm join <api-server-ip>:6443 \
  --token <token-id>.<token-secret> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --control-plane \
  --certificate-key <key>
```

**`--discovery-token-ca-cert-hash`** 用于验证连接的是正确的 API Server:

```go
// 验证过程:
// 1. 从 Bootstrap Token 获取 CA cert (从 ConfigMap 下载或本地已有)
// 2. 计算 CA cert 的 SHA256 哈希
// 3. 与 --discovery-token-ca-cert-hash 比对
// 4. 如果不匹配，拒绝连接 (防止中间人攻击)
```

```bash
# 获取 hash (在已初始化的节点上)
openssl x509 -pubkey -in /etc/kubernetes/pki/ca.crt | \
  openssl rsa -pubin -outform der 2>/dev/null | \
  openssl dgst -sha256 -hex | sed 's/^.* //'
```

---

## Bootstrap Token

Token 格式: `<token-id>.<token-secret>`

```go
// 创建 Bootstrap Token
// cmd/kubeadm/app/phases/bootstraptoken/node/token.go
func NewBootstrapToken() *bootstrapapi.BootstrapToken {
    return &bootstrapapi.BootstrapToken{
        Token:       &bootstrapapi.BootstrapTokenDiscovery{Token: "abc123.def456..."},
        TTL:         &metav1.Duration{Duration: 24 * time.Hour},
        Usages:      []string{"signing", "authentication"},
        Groups:      []string{"system:bootstrappers:kubeadm:default-node-token"},
    }
}
```

**关键 Groups**:
- `system:bootstrappers:kubeadm:default-node-token` — 临时组，证书批准后自动失效
- `system:nodes` — 正式节点组

---

## 核心代码: join.go

```go
// cmd/kubeadm/app/cmd/join.go
func RunJoin(cmd *cobra.Command, args []string) error {
    // 1. 解析 join 参数
    joinCfg, err := NewJoinConfiguration(args)

    // 2. 执行 TLS Bootstrapping
    if err := kubeletphase.TryRunKubeletStart(); err != nil {
        return err
    }

    // 3. 如果是 control-plane 节点
    if joinCfg.ControlPlane != nil {
        return joinControlPlane(joinCfg)
    }
    return nil
}
```

---

## discovery 阶段

节点需要发现集群并验证 API Server 身份:

```go
// 两种 discovery 模式:
// 1. Token discovery (默认)
//    - 使用 Bootstrap Token 认证
//    - 从 API Server 获取 .kubeconfig (含 CA cert)
// 2. TLS discovery
//    - 直接使用 CA cert 验证
//    - 用于离线环境
```

---

## kubelet-start 阶段

```go
// cmd/kubeadm/app/phases/kubelet/config.go
func WriteKubeletConfiguration() error {
    // 1. 生成 /var/lib/kubelet/config.yaml
    // 2. 如果是第一个节点，写入 /etc/kubernetes/bootstrap-kubelet.conf
    // 3. 写入 /usr/lib/systemd/system/kubelet.service.d/10-kubeadm.conf
}
```

---

## control-plane-join (可选)

如果是 master 节点额外执行:

```
1. 上传 etcd CA/cert/key 到 ConfigMap
2. 生成新的 etcd 静态 Pod manifest
3. 等待 etcd 成员添加成功
4. 等待 API Server 就绪
5. 更新 API Server endpoints
6. 标记节点为 control-plane
```

---

## crictl 故障排查

join 过程中 kubelet 启动失败时，使用 crictl 排查:

```bash
# 查看所有容器
crictl ps -a

# 查看 static pod 容器 (由 kubelet 直接管理)
crictl ps --name etcd
crictl ps --name kube-apiserver
crictl ps --name kube-controller-manager
crictl ps --name kube-scheduler

# 查看容器日志
crictl logs <container-id>

# 查看镜像
crictl images

# 查看 pod 详情
crictl pods

# 手动删除并重启 static pod
crictl stop <pod-id> && crictl rm <pod-id>
# kubelet 会自动重新创建
```

---

## 常见问题

| 错误 | 原因 | 解决 |
|------|------|------|
| `connection refused` | API Server 未启动 | 等待 init 完成后再 join |
| `TLS handshake timeout` | 防火墙/网络 | 检查 6443 端口 |
| `node xxx not found` | CSR 未批准 | 检查 csrpending |
| `invalid token` | Token 过期 | 重新生成 `kubeadm token create` |
| `invalid discovery token ca cert hash` | hash 不匹配 | 重新获取正确的 sha256 hash |
