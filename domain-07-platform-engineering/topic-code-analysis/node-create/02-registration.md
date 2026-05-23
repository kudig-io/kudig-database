---
title: 节点注册流程 — TLS Bootstrap 源码分析
description: 'description: ''## 概述'''
category: general
tags:
- reference
- apiserver
- kubelet
- controller-manager
- containerd
- webhook
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点注册流程 — TLS Bootstrap 源码分析 是什么
- 如何 节点注册流程 — TLS Bootstrap 源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点注册流程
- TLS
- Bootstrap
- 源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
created: "2026-05-23"
---

title: 节点注册流程 TLS Bootstrap 源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- apiserver
- kubelet
- controller-manager
- containerd
- webhook
- rag
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes TLS Bootstrap node registration
- kubelet CSR certificate signing request
- Bootstrap Token node join flow
- node registration kubeadm join
- csrapproving controller auto approve
trigger_keywords:
- registration
- TLS Bootstrap
- Bootstrap Token
- CSR
- kubeadm join
- certificate
- node
- kubelet
- csrapproving
- csrsigning
- system:nodes
- system:node: null
- nodeRegistration
- podCIDR
related_domains:
- domain-01-cluster-fundamentals
- domain-05-security-compliance
related_topics:
- cluster-create/06-join
- cluster-create/03-certs
- cluster-create/12-join-advanced
- node-create/06-certificate
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

# 节点注册流程 — TLS Bootstrap 源码分析

## 概述

节点注册是 Kubernetes 节点生命周期的起点。当一台新的机器准备好加入集群时，它需要通过一系列认证和授权步骤才能被集群正式接纳。这个过程称为 TLS Bootstrap（TLS 引导），它允许 kubelet 在没有预先生成证书的情况下，通过 Bootstrap Token 向 API Server 认证，然后发起 CSR（Certificate Signing Request）获取正式的客户端证书。

TLS Bootstrap 的设计目标是简化节点加入集群的流程。在早期版本中，管理员需要手动为每个节点生成证书和 kubeconfig 文件，这在管理数百个节点的集群时极其繁琐。Bootstrap Token 机制通过一个临时的、有限权限的 Token 来引导节点的初始认证，然后自动完成证书签发，极大地简化了节点管理。

完整的节点注册流程涉及多个组件的协作：kubeadm 负责创建 Bootstrap Token，kubelet 负责发起 CSR，csrapproving controller 负责自动审批，csrsigning controller 负责签发证书。本文档从源码层面深入分析这个完整流程。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubelet 主入口 | `pkg/kubelet/kubelet.go` | kubelet 启动 |
| 节点状态上报 | `pkg/kubelet/nodestatus/` | Node 对象管理 |
| 证书管理 | `pkg/kubelet/certificate/` | CSR 和证书轮换 |
| Bootstrap Token | `cmd/kubeadm/app/phases/bootstraptoken/` | Token 管理 |
| CSR 审批 | `pkg/controller/certificates/approval/` | 自动审批 |
| CSR 签发 | `pkg/controller/certificates/` | 证书签发 |
| Node Controller | `pkg/controller/nodelifecycle/` | 节点生命周期 |

---

## 一、节点注册完整流程

### 1.1 流程全景图

```
物理机/虚拟机准备
        │
        ▼
安装 containerd (容器运行时)
        │
        ▼
安装 kubelet 二进制 + systemd 服务
        │
        ▼
kubeadm join --token <token> --discovery-token-ca-cert-hash sha256:<hash>
        │
        ├── 1. 写入 /var/lib/kubelet/config.yaml (kubelet 配置)
        ├── 2. 写入 /etc/kubernetes/bootstrap-kubelet.conf (Bootstrap kubeconfig)
        │
        ▼
kubelet 启动
        │
        ├── 3. 读取 bootstrap-kubelet.conf (含 Bootstrap Token)
        │       Token 格式: <token-id>.<token-secret>
        │       路径: /etc/kubernetes/bootstrap-kubelet.conf
        │
        ▼
kubelet 向 API Server 认证 (使用 Bootstrap Token)
        │
        ▼
kubelet 发起 CSR (CertificateSigningRequest)
        │
        ├── 4. CSR 包含:
        │   - Subject: O=system:nodes, CN=system:node:<hostname>
        │   - SignerName: kubernetes.io/kube-apiserver-client-kubelet
        │   - Usages: client auth
        │
        ▼
csrapproving controller 自动审批 CSR
        │
        ├── 5. 审批条件:
        │   - 请求者有 node-bootstrapper 权限
        │   - CSR Organization 包含 system:nodes
        │   - CSR CommonName 以 system:node: 开头
        │
        ▼
csrsigning controller 使用 CA 私钥签发证书
        │
        ▼
签发证书写入 /var/lib/kubelet/pki/kubelet-client-<timestamp>.pem
        │
        ▼
kubelet 创建正式 kubeconfig: /etc/kubernetes/kubelet.conf
        │
        ▼
kubelet 创建 Node 对象
        │
        ├── 6. Node 对象包含:
        │   - labels: hostname, instance-type, zone, region
        │   - addresses: InternalIP, Hostname
        │   - capacity/allocatable: CPU, memory, pods
        │   - conditions: Ready, MemoryPressure, ...
        │   - nodeInfo: kernel, OS, container runtime version
        │
        ▼
kubelet 定期上报状态 → Node Ready
```

### 1.2 各步骤对应的源码

```go
// Step 1-2: kubeadm join
// cmd/kubeadm/app/cmd/join.go
func (j *Join) Run(cmd *cobra.Command, args []string) error {
    // 1. Preflight 预检
    // 2. 获取 discovery info (CA 证书 hash 验证)
    // 3. 写入 bootstrap-kubelet.conf
    // 4. 写入 kubelet 配置
    // 5. 启动 kubelet
}

// Step 3-6: kubelet bootstrap
// pkg/kubelet/certificate/certificate_manager.go
func (m *Manager) Start() error {
    // 3. 使用 bootstrap-kubelet.conf 连接 API Server
    // 4. 创建 CSR
    // 5. 等待审批和签发
    // 6. 更新 kubelet.conf
}

// Node 对象创建:
// pkg/kubelet/kubelet.go
func (kl *Kubelet) initializeNodeStatus() {
    // 设置节点标签
    // 设置节点地址
    // 设置节点容量
    // 注册 Node 对象到 API Server
}
```

---

## 二、Bootstrap Token

### 2.1 Token 格式与生命周期

```bash
# Bootstrap Token 格式: <token-id>.<token-secret>
# token-id:     6 位 alphanumeric (如 abcdef)
# token-secret: 16 位 alphanumeric (如 0123456789abcdef)
# 完整 Token:   abcdef.0123456789abcdef

# Token 存储在 kube-system 命名空间的 Secret 中
kubectl get secrets -n kube-system -l kubernetes.io/token-type=bootstrap.kubernetes.io/token

# 查看 Token 列表
kubeadm token list
# TOKEN                     TTL         EXPIRES
# abcdef.0123456789abcdef   23h         2024-01-02T00:00:00Z

# Token 默认 24 小时过期
# 过期后节点无法使用该 Token 加入集群
```

### 2.2 Token 管理

```bash
# 创建新 Token
kubeadm token create
# 输出: xxxxxx.xxxxxxxxxxxxxxxx

# 创建永不过期的 Token (不推荐)
kubeadm token create --ttl 0

# 生成完整 join 命令 (包含 Token 和 CA hash)
kubeadm token create --print-join-command
# 输出:
# kubeadm join 192.168.1.1:6443 \
#   --token abcdef.0123456789abcdef \
#   --discovery-token-ca-cert-hash sha256:1234567890abcdef...

# 获取 CA 证书 hash
openssl x509 -pubkey -in /etc/kubernetes/pki/ca.crt | \
  openssl rsa -pubin -outform der 2>/dev/null | \
  openssl dgst -sha256 -hex | sed 's/^.* //'

# 删除 Token
kubeadm token delete <token-id>
```

### 2.3 Bootstrap Token 源码

```go
// cmd/kubeadm/app/phases/bootstraptoken/create/create.go
func NewBootstrapToken(token string, ttl time.Duration, description string) *kubeadm.BootstrapToken {
    return &kubeadm.BootstrapToken{
        Token: &kubeadm.BootstrapTokenString{
            ID:     strings.Split(token, ".")[0],    // token-id
            Secret: strings.Split(token, ".")[1],    // token-secret
        },
        TTL:                  &metav1.Duration{Duration: ttl},
        Description:          description,
        Usages: []string{
            "authentication",   // 用于 API Server 认证
            "signing",          // 用于 CSR 签名
        },
        Groups: []string{
            "system:bootstrappers:kubeadm:default-node-token",
        },
    }
}
```

---

## 三、CSR（Certificate Signing Request）

### 3.1 CSR 对象结构

```yaml
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: csr-abc123
  labels:
    kubernetes.io/kubelet-serving: ""
spec:
  request: <base64-encoded-csr-pem>      # PEM 编码的 CSR
  signerName: kubernetes.io/kube-apiserver-client-kubelet
  usages:
  - digital signature
  - key encipherment
  - client auth                           # 客户端认证
  expirationSeconds: 31536000             # 1 年有效期
status:
  conditions:
  - type: Approved                        # 已审批
    status: "True"
    reason: AutoApproved
  certificate: <base64-encoded-cert>      # 签发后的证书
```

### 3.2 CSR 管理

```bash
# 查看所有 CSR
kubectl get csr
# NAME        AGE   SIGNERNAME                                    REQUESTOR          CONDITION
# csr-abc12   1m    kubernetes.io/kube-apiserver-client-kubelet   system:node:node1  Approved,Issued
# csr-def34   30s   kubernetes.io/kube-apiserver-client-kubelet   system:node:node2  Pending

# 查看 CSR 详情
kubectl describe csr <csr-name>

# CSR 状态流转:
# Pending → Approved → Issued (正常)
# Pending → Denied (被拒绝)

# 手动审批 CSR (自动审批失败时)
kubectl certificate approve <csr-name>

# 拒绝 CSR
kubectl certificate deny <csr-name>

# 查看 CSR 中的请求内容
kubectl get csr <csr-name> -o jsonpath='{.spec.request}' | \
  base64 -d | openssl req -text -noout
```

### 3.3 自动审批源码

```go
// pkg/controller/certificates/approval/sarapproval.go
func (a *csrApprovingController) handle(ctx context.Context, csr *certificatesv1.CertificateSigningRequest) error {
    // 自动审批条件:
    // 1. SignerName == "kubernetes.io/kube-apiserver-client-kubelet"
    // 2. 请求者是 system:node:<name> 或有 node-bootstrapper 权限
    // 3. CSR Subject.Organizations 包含 "system:nodes"
    // 4. CSR Subject.CommonName 以 "system:node:" 开头
    // 5. CSR usages 包含 "client auth"
    
    if approved {
        csr.Status.Conditions = append(csr.Status.Conditions, certificatesv1.CertificateSigningRequestCondition{
            Type:    certificatesv1.CertificateApproved,
            Status:  v1.ConditionTrue,
            Reason:  "AutoApproved",
            Message: "Auto-approved by csrapproving controller",
        })
    }
}
```

---

## 四、kubelet 配置文件

### 4.1 /var/lib/kubelet/config.yaml

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
address: 0.0.0.0
port: 10250
readOnlyPort: 10255                    # 已废弃
cgroupDriver: systemd
cgroupVersion: 2
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock
serverTLSBootstrap: true               # 服务端证书通过 CSR
rotateCertificates: true               # 自动轮换客户端证书
authentication:
  anonymous:
    enabled: false                     # 禁止匿名访问
  webhook:
    enabled: true                      # API Server 认证
    cacheTTL: 2h0m0s
  bootstrap:
    enabled: true                      # 启用 Bootstrap
authorization:
  mode: Webhook                        # API Server 授权
runtimeRequestTimeout: 2m0s
clusterDNS:
  - 10.96.0.10
clusterDomain: cluster.local
```

### 4.2 /etc/kubernetes/bootstrap-kubelet.conf

```yaml
# Bootstrap kubeconfig (首次启动用)
apiVersion: v1
kind: Config
clusters:
- cluster:
    certificate-authority-data: <ca.crt>           # CA 证书
    server: https://<api-server>:6443              # API Server
  name: kubernetes
contexts:
- context:
    cluster: kubernetes
    user: tls-bootstrap-token-user
  name: tls-bootstrap-token-user@kubernetes
current-context: tls-bootstrap-token-user@kubernetes
users:
- name: tls-bootstrap-token-user
  user:
    token: <token-id>.<token-secret>               # Bootstrap Token
```

---

## 五、Node 对象创建

### 5.1 完整 Node 对象

```yaml
apiVersion: v1
kind: Node
metadata:
  labels:
    kubernetes.io/arch: amd64
    kubernetes.io/hostname: node-1
    kubernetes.io/os: linux
    node.kubernetes.io/instance-type: t3.medium
    topology.kubernetes.io/region: us-east-1
    topology.kubernetes.io/zone: us-east-1a
  name: node-1
spec:
  podCIDR: 10.244.0.0/24
  podCIDRs:
  - 10.244.0.0/24
  taints:
  - effect: NoSchedule
    key: node-role.kubernetes.io/control-plane
  unschedulable: false
status:
  addresses:
  - address: 192.168.1.10
    type: InternalIP
  - address: node-1
    type: Hostname
  allocatable:
    cpu: "3800m"
    memory: 7Gi
    ephemeral-storage: 90Gi
    pods: "110"
  capacity:
    cpu: "4"
    memory: 8Gi
    ephemeral-storage: 100Gi
    pods: "110"
  conditions:
  - type: Ready
    status: "True"
    reason: KubeletReady
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
    containerRuntimeVersion: containerd://1.7.0
    kernelVersion: 5.15.0-91-generic
    kubeProxyVersion: v1.28.0
    kubeletVersion: v1.28.0
    operatingSystem: linux
    osImage: Ubuntu 22.04.3 LTS
```

---

## 六、PodCIDR 分配

### 6.1 CIDR 分配机制

```bash
# kubeadm init 时指定 Pod CIDR
kubeadm init --pod-network-cidr=10.244.0.0/16

# kube-controller-manager 的 node-cidr-manager 分配 PodCIDR:
# --cluster-cidr=10.244.0.0/16          # 总 CIDR 范围
# --node-cidr-mask-size-ipv4=24          # 每个节点 /24 (254 IP)

# 查看节点 PodCIDR
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'
# node-1    10.244.0.0/24
# node-2    10.244.1.0/24
# node-3    10.244.2.0/24
```

### 6.2 --node-name 与 --hostname-override

```bash
# 指定节点名称 (默认使用 hostname)
kubelet --hostname-override=node-3

# 节点名称必须与证书 CN 匹配:
# CN=system:node:node-3 → --hostname-override=node-3

# 查看节点 hostname
hostname

# 如果 hostname 与期望的节点名不匹配:
# 1. 设置 --hostname-override
# 2. 或修改系统 hostname
```

---

## 七、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| CSR 一直是 Pending | csrapproving controller 未运行 | `kubectl get csr; kubectl logs -n kube-system -l component=kube-controller-manager` | 手动 `kubectl certificate approve` |
| Token 过期 | Token TTL 到期 (默认 24h) | `kubeadm token list` | `kubeadm token create` 新建 |
| Node 已存在 | 重复 join | `kubectl get nodes` | `kubectl delete node <node>` 后重新 join |
| PodCIDR 未分配 | node-cidr-manager 问题 | `kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'` | 检查 kube-controller-manager 日志 |
| `discovery-token-ca-cert-hash` 不匹配 | CA 证书变更 | 重新获取 CA hash | `openssl x509 -pubkey -in /etc/kubernetes/pki/ca.crt \| openssl rsa -pubin -outform der \| openssl dgst -sha256 -hex` |
| `cannot join: node already registered` | 节点已注册但 kubelet 重启 | `kubectl get node <name>` | 删除旧 Node 对象或使用已有 kubeconfig |
| CSR 被拒绝 | csrapproving controller 安全策略 | `kubectl describe csr <name>` | 检查 CSR 内容，确认节点身份 |

---

## 相关函数

| 函数 | 源码位置 | 说明 |
|------|---------|------|
| `NewCmdJoin` | `cmd/kubeadm/app/cmd/join.go` | join 命令入口 |
| `NewManager` | `pkg/kubelet/certificate/certificate_manager.go` | 证书管理器初始化 |
| `createCertificateSigningRequest` | `pkg/kubelet/certificate/certificate_manager.go` | 创建 CSR |
| `handle` | `pkg/controller/certificates/approval/sarapproval.go` | CSR 自动审批 |
| `initializeNodeStatus` | `pkg/kubelet/kubelet.go` | Node 对象初始化 |
| `registerWithAPIServer` | `pkg/kubelet/kubelet.go` | 节点注册 |
| `setNodeAddress` | `pkg/kubelet/nodestatus/` | 设置节点地址 |
| `AllocatePodCIDR` | `pkg/controller/node/ipam/` | PodCIDR 分配 |

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s|k8s]]
- [[entities/kubernetes|kubernetes]]
- [[entities/containerd|containerd]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/nodes|nodes]]
