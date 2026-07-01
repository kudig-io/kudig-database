---
title: '集群新建进阶: 关键机制详解'
description: 'title: 集群新建进阶关键机制详解'
summary: 'title: 集群新建进阶关键机制详解'
category: general
tags:
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- cilium
- calico
- coredns
- containerd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '集群新建进阶: 关键机制详解 是什么'
- '如何 集群新建进阶: 关键机制详解'
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- '集群新建进阶:'
- 关键机制详解
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- cilium-basics
- cni-basics
- etcd-basics
---



title: 集群新建进阶关键机制详解
description: '# 集群新建进阶: 关键机制详解'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- cilium
- calico
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- kubeadm init configuration advance
- InitConfiguration NodeRegistration complete
- kubelet-start phase bootstrap-kubelet.conf
- NodeRestriction admission plugin
- CoreDNS deployment kubeadm
trigger_keywords:
- init
- configuration
- InitConfiguration
- NodeRegistration
- kubelet-start
- bootstrap-kubelet.conf
- kubelet.conf
- NodeRestriction
- CoreDNS
- FeatureGates
- wait-control-plane
- criSocket
related_domains:
- domain-01-cluster-fundamentals
- domain-05-security-compliance
related_topics:
- cluster-create/01-overview
- cluster-create/03-certs
- cluster-create/06-join
- cluster-create/09-upgrade
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

# 集群新建进阶: 关键机制详解

## 源码路径

`cmd/kubeadm/app/cmd/init.go`
`cmd/kubeadm/app/phases/kubelet/`

---

## InitConfiguration 完整结构

```go
type InitConfiguration struct {
    APIEndpoint       APIEndpoint          // API Server 暴露地址
    NodeRegistration  NodeRegistrationOptions  // 节点注册选项
    CertificatesDir   string               // 证书目录 /etc/kubernetes/pki
    DryRun            bool
    FeatureGates      map[string]bool      // 特性门控
    KubernetesVersion string                // K8s 版本
    ControlPlane      *ControlPlane        // 高可用配置
    Networking        Networking           // 网络配置 (PodCIDR/ServiceCIDR/DNS)
    Patches           *Patches             // 配置文件补丁
    LocalAPIEndpoint  APIEndpoint          // 本地 API Server 端点
}
```

---

## kubelet-start 阶段完整流程

```go
// cmd/kubeadm/app/phases/kubelet/config.go
func WriteKubeletConfiguration() error {
    // 1. 写入 /var/lib/kubelet/config.yaml
    //    - cgroup driver (systemd/cgroupfs)
    //    - cgroup version (v1/v2)
    //    - container runtime endpoint
    //    - serverTLSBootstrap: true
    //    - authentication/authorization mode

    // 2. 写入 /etc/kubernetes/bootstrap-kubelet.conf
    //    - 包含 Bootstrap Token
    //    - 包含 API Server CA cert (用于 TLS 验证)

    // 3. 写入 systemd drop-in:
    //    /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
    //    - 设置 --kubeconfig 指向 bootstrap-kubelet.conf
    //    - 设置 --container-runtime-endpoint

    // 4. 启用并启动 kubelet
    //    systemctl enable --now kubelet
}
```

---

## kubelet.conf vs bootstrap-kubelet.conf

```
kubelet 首次启动:
bootstrap-kubelet.conf (含 Bootstrap Token)
    ↓
kubelet 向 API Server 发起 CSR
    ↓
csrapproving controller 自动审批 (基于 Bootstrap Token 的 groups)
    ↓
签发证书写入 /var/lib/kubelet/pki/kubelet-client-*.pem
    ↓
kubelet.conf 替换 bootstrap-kubelet.conf (kubelet 重启后)
```

---

## kubelet 配置文件差异

| 配置文件 | 时机 | 用途 |
|---------|------|------|
| `bootstrap-kubelet.conf` | 首次启动 | 通过 Bootstrap Token 申请正式证书 |
| `kubelet.conf` | 证书申请完成后 | 使用正式证书连接 API Server |
| `config.yaml` | 始终 | kubelet 行为配置 (cgroup driver, 认证授权等) |

---

## NodeRestriction Admission

API Server 启用插件: `--enable-admission-plugins=NodeRestriction`

```go
// NodeRestriction 限制 kubelet 只能:
type NodeRestrictionAdmission struct {
    // 1. 只能创建/修改自己节点的 Node 和 Pod
    // 2. 不能修改其他节点的资源
    // 3. 不能创建/修改 kube-system 命名空间的 Pod
    // 4. 只能设置 node.kubernetes.io/* 注解
}
```

这确保了 kubelet 只能管理自己的节点，不能伪造其他节点身份。

---

## CoreDNS 部署 (kubeadm 安装)

```go
// kubeadm 初始化完成后自动部署:
// 1. 读取 ConfigMap: kube-system/kube-dns
// 2. 创建 kube-dns Deployment (2副本)
// 3. 创建 kube-dns Service (ClusterIP: 10.96.0.10)
// 4. 更新 /etc/resolv.conf (节点 DNS 指向 CoreDNS)
```

```bash
# CoreDNS Pod 规格:
kubectl -n kube-system get pods -l k8s-app=kube-dns
# 输出:
# NAME                       READY   STATUS    RESTARTS   AGE
# coredns-5d7c7b8b5d-abcde   1/1     Running   0          10m
# coredns-5d7c7b8b5d-fghij   1/1     Running   0          10m
```

---

## CoreDNS 配置文件

```yaml
# /etc/resolv.conf (节点上)
nameserver 10.96.0.10
search default.svc.cluster.local svc.cluster.local cluster.local
options ndots:5
```

CoreDNS 负责将 `svc.cluster.local` 域名解析为 ClusterIP。

---

## wait-control-plane 超时时间

```go
// cmd/kubeadm/app/phases/controlplane/wait.go
const (
    defaultRetryInterval = 5 * time.Second
    defaultTimeout       = 5 * time.Minute  // 默认超时 5 分钟
)

// 等待检查:
for {
    1. 检查 /healthz 端点 (API Server)
    2. 检查 etcd /health 端点
    3. crictl ps 检查静态 Pod 是否 Running
}
```

如果 5 分钟内未就绪，kubeadm init 失败。

---

## FeatureGates 示例

```yaml
# kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
spec:
  featureGates:
    # 1.27+ 默认开启
    EtcdLearnerMode: true
    # 1.28+ 默认开启
    NodeLease: true
```

---

## 常用 kubeadm 配置示例

```yaml
# init-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
spec:
  nodeRegistration:
    criSocket: /var/run/containerd/containerd.sock  # 指定 containerd
    name: master-1
    taints: null  # 不打污点，允许调度
  networking:
    podSubnet: 10.244.0.0/16    # Calico/Cilium CIDR
    serviceSubnet: 10.96.0.0/12  # Service CIDR
    dnsDomain: cluster.local
  kubernetesVersion: v1.28.0
```

---

## 集群新建完整文件树

```
/etc/kubernetes/
├── manifests/
│   ├── etcd.yaml
│   ├── kube-apiserver.yaml
│   ├── kube-controller-manager.yaml
│   └── kube-scheduler.yaml
├── pki/
│   ├── ca.crt / ca.key
│   ├── apiserver.crt / apiserver.key
│   ├── apiserver-kubelet-client.crt / apiserver-kubelet-client.key
│   ├── front-proxy-ca.crt / front-proxy-ca.key
│   ├── front-proxy-client.crt / front-proxy-client.key
│   ├── sa.pub / sa.key
│   └── etcd/
│       ├── ca.crt / ca.key
│       ├── server.crt / server.key
│       ├── peer.crt / peer.key
│       └── healthcheck-client.crt / healthcheck-client.key
├── admin.conf
├── kubelet.conf
├── controller-manager.conf
├── scheduler.conf
└── bootstrap-kubelet.conf

/var/lib/kubelet/
├── config.yaml
└── pki/
    └── kubelet-client-*.pem

/var/lib/etcd/
└── member/
    ├── wal/
    └── snap/

/etc/systemd/system/kubelet.service.d/
└── 10-kubeadm.conf
```

## Related

- [[reference|#reference Hub]] — tag hub

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/coredns.md|coredns]]
- [[entities/cilium.md|Cilium]]
