---
title: 节点生命周期总览 (topic-code-analysis)
description: 'description: ''## 概述'''
category: general
tags:
- reference
- deep-dive
- apiserver
- kubelet
- containerd
- cri-o
- docker
- pdb
- networkpolicy
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点生命周期总览 是什么
- 如何 节点生命周期总览
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点生命周期总览
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
created: "2026-05-23"
---

title: 节点生命周期总览
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- apiserver
- kubelet
- containerd
- cri-o
- docker
- pdb
- networkpolicy
last_updated: 2026-05-18
difficulty: beginner
reading_level: beginner
audience:
- Kubernetes 初学者
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes node lifecycle overview
- node lifecycle stages phases
- node Ready condition management
- node resource capacity allocatable
- node role management control-plane worker
trigger_keywords:
- node
- lifecycle
- overview
- Ready
- condition
- capacity
- allocatable
- control-plane
- worker
- role
- label
- taint
- cordon
- uncordon
- drain
related_domains:
- domain-01-cluster-fundamentals
related_topics:
- node-create/02-registration
- node-create/03-condition
- node-create/04-drain
- 01-overview
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

# 节点生命周期总览

## 概述

节点（Node）是 Kubernetes 集群中的工作单元，它是运行 Pod 的物理机或虚拟机。每个节点包含运行 Pod 所需的核心服务：kubelet（节点代理）、容器运行时（containerd/cri-o）和网络插件（CNI）。理解节点的完整生命周期——从创建、注册、运行到维护和移除——是有效管理 Kubernetes 集群的基础。

Kubernetes 中的节点管理采用了"声明式"的设计哲学：用户通过 API Server 定义期望的节点状态（如标签、污点、规格），kubelet 和各个控制器负责将实际状态与期望状态对齐。这种设计使得节点管理可以自动化——Cluster Autoscaler 可以根据负载自动增减节点，Node Lifecycle Controller 可以自动检测和隔离问题节点。

本文档作为节点生命周期管理的总览，详细介绍节点的核心组件、生命周期阶段、状态流转机制、Node 对象的结构以及节点角色管理。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubelet 主入口 | `pkg/kubelet/kubelet.go` | kubelet 核心逻辑 |
| 节点状态管理 | `pkg/kubelet/nodestatus/` | 状态上报与同步 |
| 节点注册 | `pkg/kubelet/certificate/` | Bootstrap + CSR |
| 容器运行时 | `pkg/kubelet/cri/` | CRI 接口 |
| PLEG | `pkg/kubelet/pleg/` | Pod 生命周期事件 |
| 卷管理 | `pkg/kubelet/volumemanager/` | 存储卷管理 |
| 驱逐管理 | `pkg/kubelet/eviction/` | 资源压力驱逐 |
| Node Lifecycle Controller | `pkg/controller/nodelifecycle/` | 节点健康监控 |

---

## 一、节点生命周期

### 1.1 完整生命周期流程

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```
节点生命周期:
  ┌─────────────────────────────────────────────────────────────┐
  │  阶段 1: 节点准备                                           │
  │  ├── 创建物理/虚拟机实例                                     │
  │  ├── 安装操作系统（Linux 推荐 Ubuntu/RHEL）                 │
  │  ├── 安装容器运行时 (containerd/cri-o/Docker)               │
  │  ├── 安装 kubelet 二进制 + 配置 systemd 服务                │
  │  ├── 安装 CNI 插件二进制                                     │
  │  └── 配置网络、内核参数、文件系统                             │
  ├─────────────────────────────────────────────────────────────┤
  │  阶段 2: 节点注册                                           │
  │  ├── kubeadm join --token <token>                            │
  │  ├── kubelet 使用 Bootstrap Token 向 API Server 认证         │
  │  ├── 发起 CSR (Certificate Signing Request)                  │
  │  ├── csrapproving controller 自动审批 CSR                    │
  │  ├── 获取正式客户端证书                                       │
  │  ├── 创建 Node 对象                                          │
  │  └── kubelet 开始上报状态 → Ready                            │
  ├─────────────────────────────────────────────────────────────┤
  │  阶段 3: 正常运行                                           │
  │  ├── kubelet 同步 Pod 状态 (syncPod 循环)                   │
  │  ├── 容器健康检查 (liveness/readiness/startup probe)        │
  │  ├── 资源监控上报 (cAdvisor → metrics)                      │
  │  ├── 证书自动轮换 (certificate rotation)                    │
  │  ├── 垃圾回收 (image/container GC)                          │
  │  └── 卷管理 (attach/detach/mount/unmount)                   │
  ├─────────────────────────────────────────────────────────────┤
  │  阶段 4: 节点运维                                           │
  │  ├── 维护: drain → 操作 → uncordon                          │
  │  ├── 升级: kubeadm upgrade node                             │
  │  ├── 弹性伸缩: Cluster Autoscaler 增减节点                   │
  │  └── 故障排查与恢复                                          │
  ├─────────────────────────────────────────────────────────────┤
  │  阶段 5: 节点移除                                           │
  │  ├── drain: 驱逐所有 Pod (遵守 PDB)                          │
  │  ├── delete: 从 API Server 删除 Node 对象                    │
  │  ├── reset: kubeadm reset 清理节点配置                       │  # ⚠️ 清理节点所有 K8s 配置
  │  └── 释放: 云厂商释放实例资源                                 │
  └─────────────────────────────────────────────────────────────┘
```

### 1.2 节点状态流转

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```
                   ┌──────────────┐
                   │ NotRegistered│  节点未注册
                   └──────┬───────┘
                          │ kubelet 启动 + Bootstrap
                          ▼
                   ┌──────────────┐
                   │  Registered  │  Node 对象已创建
                   └──────┬───────┘
                          │ kubelet 状态上报
                          ▼
                ┌──────────────────────┐
                │        Ready         │  节点正常
                └──────┬───────┬───────┘
                       │       │
              kubelet 正常    kubelet 异常/资源压力
                       │       │
                       ▼       ▼
                ┌──────────┐ ┌──────────────┐
                │   Ready  │ │  NotReady    │
                │ (持续)    │ │ (Conditions  │
                └──────────┘ │  异常)        │
                             └──────┬───────┘
                                    │ kubelet 恢复
                                    ▼
                             ┌──────────────┐
                             │    Ready     │
                             └──────────────┘
                                    │
                          长时间 NotReady
                          (超过 pod-eviction-timeout)
                                    │
                                    ▼
                             ┌──────────────┐
                             │   Evicted    │  Pod 被驱逐
                             └──────┬───────┘
                                    │ kubeadm reset  # ⚠️ 清理节点所有 K8s 配置
                                    ▼
                             ┌──────────────┐
                             │   Removed    │  节点已移除
                             └──────────────┘
```

---

## 二、节点核心组件

### 2.1 kubelet

kubelet 是每个节点上运行的核心代理程序，它是 Kubernetes 控制面与节点之间的桥梁：

```bash
# kubelet 核心职责:
# 1. 节点注册
#    - 使用 Bootstrap Token 向 API Server 认证
#    - 发起 CSR 获取正式证书
#    - 创建和更新 Node 对象
#
# 2. Pod 生命周期管理
#    - 监听 API Server 获取分配到本节点的 Pod
#    - 通过 CRI 调用容器运行时创建/更新/删除容器
#    - 管理 Pod 的挂载卷
#    - 执行容器健康检查
#
# 3. 状态上报
#    - 定期向 API Server 上报节点 Conditions
#    - 上报节点资源容量和可分配量
#    - 上报 Pod 和容器状态
#
# 4. 资源管理
#    - cgroup 管理（CPU/内存限制）
#    - 驱逐管理（资源不足时主动驱逐 Pod）
#    - 垃圾回收（镜像/容器清理）
#
# 5. 安全管理
#    - 证书自动轮换
#    - 服务端 TLS 管理
#    - 认证与授权
```

### 2.2 kube-proxy

kube-proxy 维护节点上的网络规则，实现 Service 的负载均衡：

```bash
# kube-proxy 核心职责:
# 1. 监听 API Server 中的 Service 和 Endpoints 变化
# 2. 维护节点网络规则:
#    - iptables 模式: 维护 KUBE-SERVICES 链
#    - ipvs 模式: 维护虚拟服务器和真实服务器映射
#    - nftables 模式: 维护 nft 规则集 (v1.29+)
# 3. 实现 Service ClusterIP 负载均衡
# 4. 实现 NodePort 和 LoadBalancer 流量转发
# 5. 连接跟踪 (conntrack) 管理
```

### 2.3 CNI 插件

CNI（Container Network Interface）负责 Pod 网络配置：

```bash
# CNI 核心职责:
# 1. Pod 网络命名空间创建和管理
# 2. veth pair 创建和配置
# 3. IP 地址分配 (IPAM)
# 4. 路由配置 (同节点/跨节点通信)
# 5. 网络策略实现 (NetworkPolicy)
# 6. DNS 配置
```

---

## 三、Node 对象结构

### 3.1 Node 对象完整结构

```yaml
apiVersion: v1
kind: Node
metadata:
  name: node-1
  labels:
    kubernetes.io/arch: amd64
    kubernetes.io/os: linux
    kubernetes.io/hostname: node-1
    node-role.kubernetes.io/control-plane: ""
    topology.kubernetes.io/zone: us-east-1a
  annotations:
    node.alpha.kubernetes.io/ttl: "0"
    volumes.kubernetes.io/controller-managed-attach-detach: "true"
  uid: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
spec:
  podCIDR: 10.244.0.0/24              # 分配给此节点的 Pod IP 范围
  podCIDRs:
  - 10.244.0.0/24
  providerID: aws:///us-east-1a/i-xxx  # 云厂商实例 ID
  taints:                              # 污点列表
  - key: node-role.kubernetes.io/control-plane
    effect: NoSchedule
  unschedulable: false                  # 是否禁止调度
status:
  conditions: [...]                     # 节点状态 (见下文)
  addresses:
  - type: InternalIP
    address: 192.168.1.10
  - type: Hostname
    address: node-1
  capacity:
    cpu: "4"
    memory: 8Gi
    ephemeral-storage: 100Gi
    pods: "110"
  allocatable:
    cpu: "3800m"
    memory: 7Gi
    ephemeral-storage: 90Gi
    pods: "110"
  nodeInfo:
    architecture: amd64
    containerRuntimeVersion: containerd://1.7.0
    kernelVersion: 5.15.0-91-generic
    kubeProxyVersion: v1.28.0
    kubeletVersion: v1.28.0
    operatingSystem: linux
    osImage: Ubuntu 22.04.3 LTS
  images: [...]                         # 节点上的镜像列表
  volumesInUse: [...]                   # 正在使用的卷
  volumesAttached: [...]                # 已挂载的卷
```

---

## 四、节点角色管理

### 4.1 节点角色标签

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 查看控制面节点
kubectl get nodes -l node-role.kubernetes.io/control-plane

# 查看工作节点
kubectl get nodes -l '!node-role.kubernetes.io/control-plane'

# 查看旧版 master 标签
kubectl get nodes -l node-role.kubernetes.io/master

# 添加角色标签
kubectl label node <node> node-role.kubernetes.io/worker=worker

# 添加自定义标签
kubectl label node <node> environment=production
kubectl label node <node> tier=frontend
```

### 4.2 污点管理

```bash
# 查看污点
kubectl describe node <node> | grep Taints
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.taints}{"\n"}{end}'

# 添加污点
kubectl taint node <node> key=value:NoSchedule
kubectl taint node <node> key=value:NoExecute
kubectl taint node <node> key=value:PreferNoSchedule

# 删除污点
kubectl taint node <node> key:NoSchedule-

# 常见污点:
# node-role.kubernetes.io/control-plane:NoSchedule    # 控制面节点
# node.kubernetes.io/not-ready:NoExecute              # 节点不就绪
# node.kubernetes.io/unreachable:NoExecute            # 节点不可达
# node.kubernetes.io/network-unavailable:NoSchedule   # 网络不可用
```

---

## 五、节点资源容量

### 5.1 Capacity 与 Allocatable

```bash
# Capacity: 节点总资源
# Allocatable: 可分配给 Pod 的资源 = Capacity - Reserved

# 查看节点资源
kubectl get node <node> -o jsonpath='{.status.capacity}'
kubectl get node <node> -o jsonpath='{.status.allocatable}'

# 资源预留公式:
# Allocatable = Capacity - KubeReserved - SystemReserved - EvictionHard
```

### 5.2 资源预留配置

```bash
# kubelet 启动参数:
--kube-reserved=cpu=500m,memory=1Gi,ephemeral-storage=2Gi
--system-reserved=cpu=500m,memory=1Gi,ephemeral-storage=2Gi
--eviction-hard=memory.available<500Mi,nodefs.available<10%
--enforce-node-allocatable=pods,kube-reserved,system-reserved
```

---

## 六、节点常用命令速查

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 查看所有节点
kubectl get nodes -o wide

# 查看节点详情
kubectl describe node <node>

# 查看节点资源使用
kubectl top nodes

# 查看节点事件
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node>

# 节点维护
kubectl cordon <node>       # 标记不可调度
kubectl drain <node>        # 驱逐所有 Pod
kubectl uncordon <node>     # 恢复调度

# 节点标签
kubectl label node <node> key=value
kubectl label node <node> key-   # 删除标签

# 节点污点
kubectl taint node <node> key=value:NoSchedule
kubectl taint node <node> key:NoSchedule-   # 删除污点

# 节点调试
kubectl debug node/<node> -it --image=busybox
```

---

## 七、常见问题

| 问题 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| 节点 NotReady | kubelet 未启动/网络问题 | `systemctl status kubelet` | 启动 kubelet，检查网络 |
| 节点 NetworkUnavailable | CNI 配置错误 | `ls /etc/cni/net.d/` | 安装 CNI 插件 |
| 节点磁盘不足 | 镜像/日志占用 | `df -h; du -sh /var/lib/*` | 清理磁盘或扩容 |
| 节点内存不足 | 工作负载过多 | `kubectl top nodes` | 扩容或减少 Pod |
| 节点无法注册 | Token 过期/网络不通 | `kubeadm token list` | 创建新 Token |
| Pod 无法调度 | 节点资源不足/污点 | `kubectl describe pod` | 扩容或添加容忍 |

---

## 相关函数

| 函数 | 源码位置 | 说明 |
|------|---------|------|
| `NewMainKubelet` | `pkg/kubelet/kubelet.go` | kubelet 初始化 |
| `syncNodeStatus` | `pkg/kubelet/kubelet.go` | 节点状态同步 |
| `NodeReadyCondition` | `pkg/kubelet/nodestatus/setters.go` | Ready 状态判定 |
| `syncPod` | `pkg/kubelet/kubelet.go` | Pod 同步 |
| `registerWithAPIServer` | `pkg/kubelet/kubelet.go` | 节点注册 |
| `TryUpdateNodeStatus` | `pkg/kubelet/nodestatus/` | 状态更新 |

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[concepts/node-lifecycle-management.md|node-lifecycle-management]]

```