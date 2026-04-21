# 节点注册流程

## 源码路径

`pkg/kubelet/kubelet.go`
`pkg/kubelet/nodestatus/`
`pkg/kubelet/certificate/`

---

## 节点注册完整流程

```
物理机/虚拟机准备
    ↓
安装 containerd/Docker
    ↓
安装 kubelet
    ↓
kubeadm join --token <token> --discovery-token-ca-cert-hash <hash>
    ↓
写入 /var/lib/kubelet/config.yaml
写入 /etc/kubernetes/bootstrap-kubelet.conf
    ↓
kubelet 启动
    ↓
读取 bootstrap-kubelet.conf (含 Bootstrap Token)
    ↓
向 API Server 发起 CSR (CertificateSigningRequest)
    ↓
csrapving controller 自动 approve CSR
    ↓
签发证书写入 /var/lib/kubelet/pki/kubelet-client-*.pem
    ↓
kubelet 重启 (使用正式证书)
    ↓
创建 Node 对象
    ↓
节点 Ready
```

---

## Bootstrap Token

```bash
# Token 格式: <token-id>.<token-secret>
# 有效期默认 24 小时

# 在 control-plane 创建 Token
kubeadm token create

# 列出所有有效 Token
kubeadm token list

# 生成完整 join 命令
kubeadm token create --print-join-command
```

---

## CSR (Certificate Signing Request)

```bash
# 查看节点 CSR
kubectl get csr

# 查看 CSR 详情
kubectl describe csr <csr-name>

# CSR 状态:
# Pending → Approved → Issued
#        → Denied

# 手动 approve (如果自动审批失败)
kubectl certificate approve <csr-name>
```

---

## kubelet 配置文件

### /var/lib/kubelet/config.yaml

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
address: 0.0.0.0
port: 10250
readOnlyPort: 10255
cgroupDriver: systemd
cgroupVersion: 2
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock
serverTLSBootstrap: true
rotateCertificates: true
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
    cacheTTL: 2h0m0s
  bootstrap:
    enabled: true
authorization:
  mode: Webhook
runtimeRequestTimeout: 2m0s
```

---

## Node 对象创建

```yaml
# kubelet 注册时创建的 Node 对象
apiVersion: v1
kind: Node
metadata:
  labels:
    kubernetes.io/hostname: <hostname>
    node.kubernetes.io/instance-type: <type>
    topology.kubernetes.io/region: <region>
    topology.kubernetes.io/zone: <zone>
  name: <node-name>
spec:
  podCIDR: 10.244.0.0/24           # kubelet 分配的 Pod CIDR
  podCIDRs: 10.244.0.0/24
  taints:                          # control-plane 有污点
  - effect: NoSchedule
    key: node-role.kubernetes.io/control-plane
  unschedulable: false
status:
  addresses:
  - address: <node-ip>
    type: InternalIP
  - address: <hostname>
    type: Hostname
  allocatable:
    cpu: "4"
    ephemeral-storage: "100Gi"
    memory: 8Gi
    pods: "110"
  capacity:
    cpu: "4"
    ephemeral-storage: "100Gi"
    memory: 8Gi
    pods: "110"
  conditions:
  - type: Ready
    status: "True"
  - type: MemoryPressure
    status: "False"
  - type: DiskPressure
    status: "False"
  - type: PIDPressure
    status: "False"
  - type: NetworkUnavailable
    status: "False"
  nodeInfo:
    architecture: amd64
    bootID: xxx
    containerRuntimeVersion: containerd://1.7.0
    kernelVersion: 5.15.0
    kubeProxyVersion: v1.28.0
    kubeletVersion: v1.28.0
    operatingSystem: linux
    osImage: Ubuntu 22.04
```

---

## PodCIDR 分配

```bash
# kubeadm init 时指定 --pod-network-cidr
kubeadm init --pod-network-cidr=10.244.0.0/16

# kube-controller-manager 的 node-controller 分配 PodCIDR:
# --pod-cidr-mask-size: 24 (默认每个节点 /24)
# --node-monitor-period: 5s
# --node-monitor-grace-period: 40s

# 查看节点 PodCIDR
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'
```

---

## --node-name 与 --hostname-override

```bash
# 指定节点名称
kubelet --hostname-override=node-3

# 查看节点 hostname
hostname

# 节点名称与 hostname 不匹配时需要 --hostname-override
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| CSR Pending | csrapproving controller 未运行 | 手动 `kubectl certificate approve` |
| Token 过期 | 24h TTL | `kubeadm token create` 新建 |
| Node 已存在 | 重复 join | `kubectl delete node <node>` |
| PodCIDR 未分配 | node-controller 未分配 | 检查 kube-controller-manager 日志 |
