---
title: '高可用进阶: 负载均衡与证书分发 [cluster-create]'
description: 'description: // 1. CA 公钥/私钥 (加密存储)'
summary: 'description: // 1. CA 公钥/私钥 (加密存储)'
category: general
tags:
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '高可用进阶: 负载均衡与证书分发 是什么'
- '如何 高可用进阶: 负载均衡与证书分发'
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- '高可用进阶:'
- 负载均衡与证书分发
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---



title: '高可用进阶: 负载均衡与证书分发'
description: // 1. CA 公钥/私钥 (加密存储)
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
- daemonset
last_updated: '2026-05-18'
difficulty: expert
reading_level: expert
audience:
- Kubernetes开发者
- DevOps工程师
- SRE
- 云架构师
estimated_read_time: 5min
intent_queries:
- Kubernetes HA kube-vip load balancer configuration
- kubeadm certificate upload download AES-256-GCM encryption
- etcd HA stacked vs external topology comparison
- Kubernetes API server endpoint load balancing health check
- kubeadm join control-plane certificate key generation
trigger_keywords:
- kube-vip
- HA
- certificate-key
- upload-certs
- stacked etcd
- external etcd
- load balancer
- TLS passthrough
- leader election
- API Server endpoint
- certificate
- AES-256-GCM
related_domains:
- domain-01-cluster-fundamentals
- domain-01-cluster-fundamentals
- domain-10-troubleshooting-diagnostics
related_topics:
- kubeadm init
- kubeadm join
- etcd
- API Server
- certificate management
- HA cluster
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

# 高可用进阶: 负载均衡与证书分发

## 源码路径

`cmd/kubeadm/app/cmd/phases/controlplane/`
`cmd/kubeadm/app/cmd/join.go` (control-plane 部分)
`cmd/kubeadm/app/phases/uploadconfig/`

---

## upload-certs 阶段

```go
// cmd/kubeadm/app/phases/uploadconfig/uploadconfig.go
func UploadConfiguration(cfg *kubeadmapi.InitConfiguration) error {
    // 将以下内容加密后存入 ConfigMap: kube-system/kubeadm-config
    // 1. CA 公钥/私钥 (加密存储)
    // 2. etcd CA 公钥/私钥 (加密存储)
    // 3. Service Account 签名密钥
    // 4. 所有控制面组件的证书/私钥

    // 用于: 新节点 join 时解密获取完整证书集
}
```

**加密方式**: AES-256-GCM，密钥来自 `--certificate-key` 参数。

---

## --certificate-key 生成与使用

```bash
# 生成 certificate-key (24 字符，base64)
openssl rand -base64 24

# init 时指定
kubeadm init \
  --control-plane-endpoint=loadbalancer:6443 \
  --certificate-key=<生成的key>

# join 时使用同一 key 解密
kubeadm join loadbalancer:6443 \
  --control-plane \
  --certificate-key=<同一个key>
```

---

## kube-vip 配置

kube-vip 是常用的 Kubernetes 高可用方案:

```yaml
# kube-vip DaemonSet (部署在所有 control-plane 节点)
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kube-vip
  namespace: kube-system
spec:
  containers:
  - name: kube-vip
    image: ghcr.io/kube-vip/kube-vip:v0.6.0
    args:
    - manager
    env:
    - name: vip_arp
      value: "true"
    - name: vip_interface
      value: "eth0"
    - name: address
      value: "192.168.1.100"  # 虚拟 IP (VIP)
    - name: port
      value: "6443"
    - name: vip_cidr
      value: "32"
```

---

## kube-vip 工作模式

```
         ┌─────────────────────────────────────┐
         │           kube-vip                   │
         │  (每节点一个，通过 ARP/NDP 争抢 VIP) │
         └─────────────────────────────────────┘
                           │
         ┌─────────────────┼─────────────────┐
         ▼                 ▼                 ▼
    ┌──────────┐    ┌──────────┐    ┌──────────┐
    │ node-1   │    │ node-2   │    │ node-3   │
    │(active)  │    │(standby) │    │(standby) │
    └──────────┘    └──────────┘    └──────────┘
```

- kube-vip 在每个节点运行
- 通过 ARP/NDP 竞争谁响应 VIP 的 ARP 请求
- 只有一个节点是 active 的

---

## 高可用 etcd 架构

### stacked etcd (同节点)

```
node-1: etcd-1 + apiserver-1
node-2: etcd-2 + apiserver-2
node-3: etcd-3 + apiserver-3

raft 共识:
- 写操作需要多数节点确认 (3节点 -> 2节点确认)
- 允许 1 节点问题
```

### external etcd

```
# 分开部署的 3 节点 etcd 集群
node-etcd-1: etcd-1
node-etcd-2: etcd-2
node-etcd-3: etcd-3

# kubeadm init 时指定:
kubeadm init \
  --control-plane-endpoint=lb:6443 \
  --etcd-servers=https://etcd-1:2379,https://etcd-2:2379,https://etcd-3:2379
```

---

## API Server 高可用

```
                    ┌────────────────────┐
                    │   负载均衡器 (LB)   │
                    │   (kube-vip/LB)    │
                    │   192.168.1.100    │
                    └────────┬───────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        ▼                    ▼                    ▼
  ┌──────────┐        ┌──────────┐        ┌──────────┐
  │ apiserver│        │ apiserver│        │ apiserver│
  │ node-1   │        │ node-2   │        │ node-3   │
  │ :6443    │        │ :6443    │        │ :6443    │
  └──────────┘        └──────────┘        └──────────┘
```

所有节点的 `kubelet.conf` 和 `admin.conf` 中的 `server` 指向同一个 VIP。

---

## 负载均衡器健康检查

```yaml
# 健康检查配置:
# TCP 6443 端口
# 检查: API Server /healthz 返回 200
# 间隔: 10s
# 超时: 5s
# 不健康阈值: 3
# 健康阈值: 1
```

---

## 新增 control-plane 节点流程详解

```
1. kubeadm join --control-plane --certificate-key <key>
   ↓
2. 解密 ConfigMap 中的证书
   ↓
3. 写入 /etc/kubernetes/pki/ (CA + 所有组件证书)
   ↓
4. 生成 /etc/kubernetes/manifests/kube-apiserver.yaml
   ↓
5. kubelet 启动 apiserver
   ↓
6. 生成 kube-controller-manager.yaml, kube-scheduler.yaml
   ↓
7. kubelet 启动 etcd (如果是 stacked)
   ↓
8. 向 etcd 集群添加新成员
   ↓
9. 更新 API Server endpoints (包含新节点)
   ↓
10. 标记节点为 control-plane (node label + taint)
```

---

## API Server Endpoints 更新

```bash
# 查看 API Server endpoints (所有 apiserver 的 PodIP + port)
kubectl get endpoints kube-apiserver -n kube-system -o yaml

# 新增节点后:
# endpoints 包含:
# - 10.0.0.1:6443 (node-1)
# - 10.0.0.2:6443 (node-2)
# - 10.0.0.3:6443 (node-3)
```

---

## Leader Election 机制

Controller Manager 和 Scheduler 使用 Kubernetes 内置的 leader election:

```go
// kube-controller-manager 配置:
--leader-elect=true
--leader-elect-resource-name=kube-controller-manager
--leader-elect-resource-namespace=kube-system

// 选举端点:
// endpoints/kube-controller-manager
// 包含:
// - holderIdentity: 当前 leader 的 node name
// - leaseDurationSeconds: 15
// - renewTime: 上次更新时间
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 节点加入后 etcd 失败 | 新节点 IP 不在证书 SAN | 使用 `kubeadm init --apiserver-cert-extra-sans` |
| API Server 连不上 LB | TLS passthrough 未配置 | 配置 LB 透传 TLS |
| leader election 频繁切换 | 网络延迟高/选举超时太短 | 增大 `--lease-duration` |
| etcd 成员数不对 | 成员添加失败 | 检查 etcd 日志，手动 `member add` |

## Related

- [[reference|#reference Hub]] — tag hub

- [[log|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/kube-vip.md|kube-vip]]
