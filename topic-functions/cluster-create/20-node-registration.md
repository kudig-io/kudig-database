# Node 注册与 kubeadm token 详解

## 源码路径

`cmd/kubeadm/app/phases/bootstraptoken/`
`cmd/kubeadm/app/cmd/token/`
`pkg/kubelet/` (node registration)

---

## kubeadm token 家族

```bash
# 创建新的 Bootstrap Token
kubeadm token create

# 带 TTL 的 Token (默认 24h)
kubeadm token create --ttl=2h

# 不自动创建描述
kubeadm token create --description="For node-3 join"

# 列出所有有效 Token
kubeadm token list

# 删除 Token
kubeadm token delete <token-id>

# 生成 join 命令
kubeadm token create --print-join-command

# 完整输出:
# kubeadm join <api-server>:6443 --token xxx --discovery-token-ca-cert-hash sha256:xxx
```

---

## Token 存储

```bash
# Token 存储在 kube-system secret 中
kubectl get secrets -n kube-system | grep bootstrap-token

# 查看 Secret 内容
kubectl get secret bootstrap-token-abc123 -n kube-system -o yaml

# 解码后内容:
# apiVersion: v1
# kind: Secret
# metadata:
#   name: bootstrap-token-abc123
#   namespace: kube-system
# type: bootstrap.kubernetes.io/token
# data:
#   token-id: <base64>
#   token-secret: <base64>
#   expiration: <base64 (ISO8601)>
#   usage-bootstrap-authentication: <base64 (true)>
#   usage-bootstrap-signing: <base64 (true)>
#   auth-extra-groups: <base64 (system:bootstrappers:kubeadm:default-node-token)>
```

---

## Token TTL 与续期

```bash
# 默认 Token 有效期 24 小时
# TTL 到期后 Token 自动失效

# 创建 1 小时有效期的 Token
kubeadm token create --ttl=1h

# 查看 Token 过期时间
kubeadm token list

# 输出:
# TOKEN                     TTL         EXPIRES
# abc123.def456789         23h         2024-01-01T12:00:00Z
# xyz789.uvw123            59m         2024-01-01T00:59:00Z

# Token 过期后需要重新创建
# 过期 Token 无法用于 join
```

---

## --discovery-token-ca-cert-hash 详解

```bash
# 获取当前集群的 CA cert hash
openssl x509 -pubkey -in /etc/kubernetes/pki/ca.crt | \
  openssl rsa -pubin -outform der 2>/dev/null | \
  openssl dgst -sha256 -hex | sed 's/^.* //'

# 或者用 kubeadm 获取
kubeadm init phase certs ca --cert-dir=/etc/kubernetes/pki

# join 时验证
kubeadm join <api-server>:6443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash>
```

**作用**: 确保节点只 join 到正确的集群，防止中间人攻击。

---

## Node 注册选项

### --node-name

```bash
# 指定节点名称 (默认使用 hostname)
kubeadm join --node-name=node-3

# kubelet 会注册为: node-3
kubectl get nodes | grep node-3
```

### --hostname-override

```bash
# 覆盖主机名 (用于云厂商/虚拟化环境)
kubeadm join --hostname-override=node-custom-name

# 使用场景:
# - 云主机 hostname 与期望不符
# - 动态 IP 环境
# - 强制统一命名
```

### --node-ip

```bash
# 指定节点 IP (用于多网卡环境)
kubeadm join --node-ip=192.168.1.100

# kubelet 会在 Node.status.addresses 中注册此 IP
kubectl get node <node> -o jsonpath='{.status.addresses}'
```

### --register-node

```bash
# 是否向 API Server 注册节点 (默认 true)
kubeadm join --register-node=false

# 使用场景:
# - 手动管理 Node 对象
# - 节点只运行静态 Pod
# - 调试 kubelet
```

---

## Node 对象

```yaml
# kubectl get node <node-name> -o yaml
apiVersion: v1
kind: Node
metadata:
  labels:
    beta.kubernetes.io/arch: amd64
    beta.kubernetes.io/os: linux
    kubernetes.io/arch: amd64
    kubernetes.io/hostname: node-1
    kubernetes.io/os: linux
    node-role.kubernetes.io/control-plane: ""
    node.kubernetes.io/exclude-from-external-load-balancers: ""
  name: node-1
spec:
  podCIDR: 10.244.0.0/24           # kubeadm 分配的 Pod CIDR
  podCIDRs: 10.244.0.0/24
  taints:                          # control-plane 污点
  - effect: NoSchedule
    key: node-role.kubernetes.io/control-plane
status:
  addresses:
  - address: 192.168.1.1
    type: InternalIP
  - address: node-1
    type: Hostname
  allocatable:
    cpu: "4"
    memory: 8Gi
  capacity:
    cpu: "4"
    memory: 8Gi
  nodeInfo:
    architecture: amd64
    bootID: xxx
    containerRuntimeVersion: containerd://1.7.x
    kernelVersion: 5.15.0
    kubeProxyVersion: v1.28.0
    kubeletVersion: v1.28.0
    machineID: xxx
    operatingSystem: linux
    osImage: Ubuntu 22.04
```

---

## PodCIDR 分配

```bash
# kubeadm init 时指定 Pod CIDR
kubeadm init --pod-network-cidr=10.244.0.0/16

# 每个节点加入时分配一个子网:
# node-1: 10.244.0.0/24
# node-2: 10.244.1.0/24
# node-3: 10.244.2.0/24

# 分配由 kube-controller-manager 的 node-controller 完成:
# --node-monitor-period=5s
# --node-monitor-grace-period=40s
# --pod-eviction-timeout=5m

# 查看节点 PodCIDR
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'
```

---

## provider-id

云厂商为节点分配的唯标识:

```yaml
# AWS EC2
providerID: aws:///us-east-1a/i-0abc123def456

# GCP GCE
providerID: gce://project-id/us-east1-b/gke-node-pool-xxx

# Azure
providerID: azure:///subscriptions/xxx/resourceGroups/xxx/providers/Microsoft.Compute/virtualMachines/node-xxx
```

kubelet 在节点注册时设置 `--provider-id` 或从云元数据服务获取。

---

## 节点污点与标签

```bash
# control-plane 节点标签
kubectl get nodes --show-labels | grep node-role
# node-role.kubernetes.io/control-plane

# control-plane 节点污点 (防止调度普通 Pod)
kubectl get nodes -o jsonpath='{.items[*].spec.taints}'
# [{"effect":"NoSchedule","key":"node-role.kubernetes.io/control-plane"}]

# worker 节点污点 (可自定义)
kubectl taint node <node> node-role.kubernetes.io/worker:NoSchedule

# 容忍污点的 Pod
spec:
  tolerations:
  - key: node-role.kubernetes.io/worker
    effect: NoSchedule
    operator: Exists
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| Token 已过期 | `--ttl=24h` 到期 | `kubeadm token create` 新建 |
| `node name mismatch` | hostname-override 与实际不符 | 检查 `/etc/hostname` |
| `node pod cidr unassigned` | podCIDR 未分配 | 检查 kube-controller-manager 日志 |
| `node not found` | kubelet 未注册 | 检查 kubelet 日志、CSR 状态 |
| 多网卡 node-ip 问题 | kubelet 选择了错误的 IP | 显式指定 `--node-ip` |
