---
title: 32 - Lease / Event / Node YAML 配置参考
description: '# 32 - Lease / Event / Node YAML 配置参考'
summary: 'Lease(租约)是 Kubernetes 中的**分布式锁机制**,主要用于:'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- calico
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Lease / Event / Node YAML 配置参考 是什么
- 如何 Lease / Event / Node YAML 配置参考
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- Lease
- Event
- Node
- YAML
- 配置参考
- yaml
- manifests
prerequisites:
- kubectl-basics
- prometheus-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../故障诊断/FTA故障树/list/node-fta.md
  label: '故障树: node'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 32 - Lease / Event / Node YAML 配置参考

> **适用版本**: [[kubernetes|Kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-02

**本文档全面覆盖 Lease(租约)、Event(事件)、Node(节点)的 YAML 配置**,包括完整字段说明、Lease 在 Leader Election 中的应用、Event 事件类型、Node 节点管理、生产实践案例等。

---

<!-- chunk: 📋 目录 -->## 📋 目录

1. [Lease 租约配置](#1-lease-租约配置)
2. [Event 事件配置](#2-event-事件配置)
3. [Node 节点配置](#3-node-节点配置)
4. [生产案例](#4-生产案例)
5. [故障排查](#5-故障排查)

---

<!-- chunk: 1. Lease 租约配置 -->## 1. Lease 租约配置

## 1.1 Lease 基础概念

Lease(租约)是 Kubernetes 中的**分布式锁机制**,主要用于:

- **节点心跳**: [[kubelet|Kubelet]] 通过更新 Lease 向控制平面报告节点存活状态(取代 Node Status 更新,减少 [[etcd|etcd]] 压力)
- **Leader Election**: 控制器通过竞争 Lease 实现 Leader 选举(确保同一时间只有一个实例运行)
- **API Server Identity**: API Server 实例通过 Lease 标识身份(v1.26+)

## 1.2 Lease 完整字段

```yaml
apiVersion: coordination.k8s.io/v1
kind: Lease
metadata:
  # Lease 名称
  name: my-controller-leader
  # 命名空间(Lease 是命名空间级资源)
  namespace: kube-system
  # 标签(可选,用于标识用途)
  labels:
    app: my-controller
    component: leader-election
spec:
  # === 核心字段 ===
  
  # 持有者标识(当前持有 Lease 的实例 ID)
  holderIdentity: "my-controller-pod-abc123"
  
  # 租约有效期(秒,推荐 10-15 秒)
  leaseDurationSeconds: 15
  
  # 获取时间(首次获得 Lease 的时间戳,RFC3339 格式)
  acquireTime: "2026-02-10T10:00:00.123456Z"
  
  # 续约时间(最后一次续约的时间戳,RFC3339 格式)
  renewTime: "2026-02-10T10:00:05.654321Z"
  
  # 租约转换次数(Leader 切换次数,用于检测频繁切换)
  leaseTransitions: 3
  
  # === 高级字段(v1.27+) ===
  
  # 优先使用的 Leader(可选,用于提示下次 Leader 选举优先选择该实例)
  # 注意: 这是一个"提示"字段,不是强制要求
  preferredHolder: "my-controller-pod-xyz789"
  
  # 策略(可选,v1.27 Alpha,定义 Leader Election 策略)
  strategy: OldestEmulationVersion  # 或 nil
```

## 1.3 Lease 用途详解

## 1.3.1 节点心跳(kube-node-lease)

从 Kubernetes v1.14 开始,Kubelet 使用 Lease 替代 Node Status 更新作为心跳机制:

```yaml
# 节点心跳 Lease(由 Kubelet 自动创建和维护)
apiVersion: coordination.k8s.io/v1
kind: Lease
metadata:
  # 名称与节点名一致
  name: node1
  # 固定命名空间
  namespace: kube-node-lease
  # Kubelet 自动添加的标签
  labels:
    kubernetes.io/hostname: node1
spec:
  # 持有者为节点名
  holderIdentity: "node1"
  # 默认 40 秒(与 --node-status-update-frequency 相关)
  leaseDurationSeconds: 40
  # Kubelet 每 10 秒更新一次(--node-lease-renew-interval-seconds)
  renewTime: "2026-02-10T10:00:30Z"
  acquireTime: "2026-02-10T09:00:00Z"
  leaseTransitions: 0  # 节点心跳不涉及切换
```

**优势**:

- **减少 etcd 压力**: Node Status 对象较大(包含 conditions, addresses, capacity 等),而 Lease 仅 ~200 字节
- **降低网络开销**: Lease 更新频率可独立配置,不影响 Node Status 更新
- **提高检测速度**: 更频繁的心跳(默认 10 秒)使节点故障检测更快

**相关 Kubelet 参数**:

```bash
# kubelet 启动参数
--node-lease-duration-seconds=40           # Lease 有效期
--node-lease-renew-interval-seconds=10     # 续约间隔
--node-status-update-frequency=10s         # Node Status 更新频率(保留用于状态变更)
```

## 1.3.2 Leader Election(控制器)

控制器通过竞争 Lease 实现 Leader 选举:

```yaml
# Leader Election Lease(由 client-go 自动管理)
apiVersion: coordination.k8s.io/v1
kind: Lease
metadata:
  name: my-controller
  namespace: default
spec:
  # 当前 Leader 的 Pod 名称
  holderIdentity: "my-controller-7d8f9b5c6-abc123"
  # 租约有效期(推荐 10-15 秒)
  leaseDurationSeconds: 15
  # 首次获得 Leader 的时间
  acquireTime: "2026-02-10T10:00:00Z"
  # 最后一次续约时间
  renewTime: "2026-02-10T10:01:30Z"
  # Leader 切换次数
  leaseTransitions: 2
```

**工作流程**:

```
多个 Pod 副本(Replica 1, 2, 3)
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. 尝试获取 Lease(通过 CREATE 请求)                             │
│    - Replica 1: CREATE Lease/my-controller → 成功(成为 Leader)  │
│    - Replica 2: CREATE Lease/my-controller → 失败(已存在)       │
│    - Replica 3: CREATE Lease/my-controller → 失败(已存在)       │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. Leader 定期续约(UPDATE 请求)                                 │
│    - Replica 1(Leader): 每 10 秒更新 renewTime                  │
│    - Replica 2/3(Follower): 监听 Lease 变化                     │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. Leader 问题 → Follower 接管                                  │
│    - Replica 1 Crash,停止续约                                   │
│    - Replica 2 检测到 Lease 过期(renewTime + 15s < now)         │
│    - Replica 2 尝试更新 Lease.holderIdentity = "replica-2"      │
│      * 使用 Optimistic Concurrency(resourceVersion 检查)        │
│      * 如果成功 → 成为新 Leader                                 │
│      * 如果失败(Replica 3 先抢到)→ 继续等待                    │
└─────────────────────────────────────────────────────────────────┘
```

**client-go 代码示例**(Go):

```go
import (
    "context"
    "time"
    
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/client-go/kubernetes"
    "k8s.io/client-go/tools/leaderelection"
    "k8s.io/client-go/tools/leaderelection/resourcelock"
)

func runLeaderElection(clientset *kubernetes.Clientset) {
    // 创建 Lease 锁
    lock := &resourcelock.LeaseLock{
        LeaseMeta: metav1.ObjectMeta{
            Name:      "my-controller",
            Namespace: "default",
        },
        Client: clientset.CoordinationV1(),
        LockConfig: resourcelock.ResourceLockConfig{
            Identity: "my-controller-pod-abc123",  // 当前 Pod 名称
        },
    }
    
    // 配置 Leader Election
    leaderelection.RunOrDie(context.Background(), leaderelection.LeaderElectionConfig{
        Lock:            lock,
        LeaseDuration:   15 * time.Second,  // 租约有效期
        RenewDeadline:   10 * time.Second,  // 续约截止时间
        RetryPeriod:     2 * time.Second,   // 重试间隔
        Callbacks: leaderelection.LeaderCallbacks{
            // 成为 Leader 时调用
            OnStartedLeading: func(ctx context.Context) {
                fmt.Println("I am the leader!")
                // 启动控制器逻辑...
            },
            // 失去 Leader 时调用
            OnStoppedLeading: func() {
                fmt.Println("I lost leadership!")
                // 停止控制器逻辑...
            },
            // 新 Leader 产生时调用(所有副本都会收到)
            OnNewLeader: func(identity string) {
                fmt.Printf("New leader: %s\n", identity)
            },
        },
    })
}
```

## 1.3.3 API Server Identity(v1.26+)

API Server 使用 Lease 标识自己的身份,用于协调和监控:

```yaml
# API Server Identity Lease(由 kube-apiserver 自动创建)
apiVersion: coordination.k8s.io/v1
kind: Lease
metadata:
  name: kube-apiserver-xxx  # API Server 实例 ID
  namespace: kube-system
  labels:
    apiserver.kubernetes.io/identity: kube-apiserver
spec:
  holderIdentity: "kube-apiserver-xxx_xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  leaseDurationSeconds: 3600  # 1 小时
  renewTime: "2026-02-10T10:00:00Z"
```

**用途**:

- **监控**: 统计当前运行的 API Server 实例数量
- **协调**: 用于 API Server 之间的协调操作(如 Storage Version 迁移)

---

<!-- chunk: 2. Event 事件配置 -->## 2. Event 事件配置

## 2.1 Event 基础概念

Event(事件)是 Kubernetes 中的**审计和调试机制**,记录集群中发生的重要操作:

- **资源生命周期**: Pod 创建、调度、启动、失败等
- **系统告警**: 节点资源不足、镜像拉取失败、卷挂载失败等
- **控制器行为**: Deployment 滚动更新、HPA 扩缩容、PVC 绑定等

## 2.2 Event 完整字段

## 2.2.1 events.k8s.io/v1 (推荐,v1.19+)

```yaml
apiVersion: events.k8s.io/v1
kind: Event
metadata:
  name: my-pod.17e2a1b2c3d4e5f6  # 自动生成: <对象名>.<hash>
  namespace: default
  # 事件的创建时间
  creationTimestamp: "2026-02-10T10:00:00Z"
spec:
  # === 关联对象 ===
  
  # 事件涉及的主要对象
  regarding:
    apiVersion: v1
    kind: Pod
    name: my-pod
    namespace: default
    uid: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
    resourceVersion: "12345"
  
  # 相关对象(可选,如 PVC 挂载事件中的 PV)
  related:
    apiVersion: v1
    kind: PersistentVolume
    name: pv-abc123
  
  # === 事件内容 ===
  
  # 事件原因(简短标识,如 FailedScheduling, Pulled, Started)
  reason: FailedScheduling
  
  # 事件消息(详细描述)
  message: "0/3 nodes are available: 3 Insufficient cpu."
  
  # 事件类型: Normal(正常) 或 Warning(告警)
  type: Warning
  
  # === 时间信息 ===
  
  # 首次发生时间
  eventTime: "2026-02-10T10:00:00.123456Z"
  
  # === 事件来源 ===
  
  # 报告者(生成事件的组件)
  reportingController: "default-scheduler"
  
  # 报告实例(组件的具体实例)
  reportingInstance: "default-scheduler-node1"
  
  # === 重复事件处理 ===
  
  # 动作(Binding, Started, Killing 等)
  action: "Scheduling"
  
  # 重复次数(相同事件发生次数)
  series:
    count: 5              # 重复次数
    lastObservedTime: "2026-02-10T10:00:30Z"  # 最后发生时间
  
  # === 备注 ===
  
  # 备注信息(可选,扩展信息)
  note: "Pod triggered scale-up"
```

## 2.2.2 v1/Event (旧版本,已弃用但仍兼容)

```yaml
apiVersion: v1
kind: Event
metadata:
  name: my-pod.17e2a1b2c3d4e5f6
  namespace: default
# 涉及的对象
involvedObject:
  apiVersion: v1
  kind: Pod
  name: my-pod
  namespace: default
  uid: "xxx"
  resourceVersion: "12345"
# 事件原因
reason: FailedScheduling
# 事件消息
message: "0/3 nodes are available: 3 Insufficient cpu."
# 事件类型
type: Warning
# 首次发生时间
firstTimestamp: "2026-02-10T10:00:00Z"
# 最后发生时间
lastTimestamp: "2026-02-10T10:00:30Z"
# 重复次数
count: 5
# 事件来源
source:
  component: default-scheduler
  host: node1
```

## 2.3 常见 Event Reason 列表

## Pod 生命周期事件

| Reason | Type | 描述 |
|--------|------|------|
| **Scheduled** | Normal | Pod 已成功调度到节点 |
| **FailedScheduling** | Warning | 调度失败(资源不足、污点不匹配等) |
| **Pulling** | Normal | 开始拉取容器镜像 |
| **Pulled** | Normal | 镜像拉取成功 |
| **Failed** | Warning | 镜像拉取失败(镜像不存在、认证失败等) |
| **BackOff** | Warning | 容器启动失败,处于 BackOff 状态 |
| **Created** | Normal | 容器已创建 |
| **Started** | Normal | 容器已启动 |
| **Killing** | Normal | 正在终止容器 |
| **Preempting** | Normal | Pod 被抢占(优先级更高的 Pod 到达) |
| **Unhealthy** | Warning | 健康检查失败 |

## 节点事件

| Reason | Type | 描述 |
|--------|------|------|
| **NodeReady** | Normal | 节点变为 Ready 状态 |
| **NodeNotReady** | Warning | 节点变为 NotReady 状态 |
| **NodeSchedulable** | Normal | 节点变为可调度 |
| **NodeNotSchedulable** | Warning | 节点被标记为不可调度 |
| **RegisteredNode** | Normal | 新节点注册到集群 |
| **RemovingNode** | Warning | 节点正在被移除 |

## 存储事件

| Reason | Type | 描述 |
|--------|------|------|
| **SuccessfulAttachVolume** | Normal | 卷成功挂载到节点 |
| **FailedAttachVolume** | Warning | 卷挂载失败 |
| **SuccessfulMountVolume** | Normal | 卷成功挂载到 Pod |
| **FailedMount** | Warning | 卷挂载到 Pod 失败 |
| **VolumeResizeFailed** | Warning | 卷扩容失败 |
| **VolumeResizeSuccessful** | Normal | 卷扩容成功 |

## 控制器事件

| Reason | Type | 描述 |
|--------|------|------|
| **SuccessfulCreate** | Normal | Deployment/ReplicaSet 成功创建 Pod |
| **FailedCreate** | Warning | 创建 Pod 失败(配额超限等) |
| **ScalingReplicaSet** | Normal | ReplicaSet 扩缩容 |
| **SuccessfulDelete** | Normal | 成功删除 Pod |
| **FailedDelete** | Warning | 删除 Pod 失败 |

---

<!-- chunk: 3. Node 节点配置 -->## 3. Node 节点配置

## 3.1 Node 基础概念

Node(节点)是 Kubernetes 集群的**工作负载载体**,运行 Kubelet 和容器:

- **自动注册**: Kubelet 启动时自动向 API Server 注册节点
- **状态同步**: Kubelet 定期上报节点状态(Ready/NotReady、资源容量等)
- **标签管理**: 通过标签实现节点选择器、亲和性调度
- **污点管理**: 通过污点(Taint)驱逐或阻止 Pod 调度

## 3.2 Node 完整字段

```yaml
apiVersion: v1
kind: Node
metadata:
  name: node1
  # 标签(用于 nodeSelector, affinity)
  labels:
    kubernetes.io/hostname: node1
    kubernetes.io/os: linux
    kubernetes.io/arch: amd64
    node-role.kubernetes.io/control-plane: ""  # 控制平面节点
    node-role.kubernetes.io/worker: ""         # 工作节点
    # 自定义标签
    environment: production
    zone: us-west-1a
    instance-type: c5.2xlarge
  # 注解
  annotations:
    kubeadm.alpha.kubernetes.io/cri-socket: unix:///var/run/containerd/containerd.sock
    node.alpha.kubernetes.io/ttl: "0"
    volumes.kubernetes.io/controller-managed-attach-detach: "true"
spec:
  # === 基础配置 ===
  
  # Pod CIDR(节点上 Pod 的 IP 地址范围,由 CNI 分配)
  podCIDR: 10.244.1.0/24
  
  # Pod CIDRs(多协议栈支持,v1.16+)
  podCIDRs:
    - 10.244.1.0/24        # IPv4
    - fd00:10:244:1::/64   # IPv6
  
  # 云提供商 ID(如 AWS: aws:///us-west-1a/i-0abcd1234efgh5678)
  providerID: "aws:///us-west-1a/i-0abcd1234efgh5678"
  
  # === 污点(Taint)配置 ===
  
  taints:
    # 污点 1: 禁止调度(NoSchedule)
    - key: node-role.kubernetes.io/control-plane
      effect: NoSchedule
    
    # 污点 2: 优先驱逐(PreferNoSchedule)
    - key: example.com/maintenance
      value: "true"
      effect: PreferNoSchedule
    
    # 污点 3: 立即驱逐(NoExecute)
    - key: node.kubernetes.io/unreachable
      effect: NoExecute
      timeAdded: "2026-02-10T10:00:00Z"
  
  # === 调度控制 ===
  
  # 不可调度标记(true = 不调度新 Pod,但不驱逐现有 Pod)
  unschedulable: false
  
  # === 配置源(Kubelet 配置来源) ===
  
  configSource:
    configMap:
      name: kubelet-config
      namespace: kube-system
      uid: "xxx"
      kubeletConfigKey: kubelet

status:
  # === 资源容量 ===
  
  # 节点总容量
  capacity:
    cpu: "8"               # 8 核 CPU
    memory: "32Gi"         # 32 GB 内存
    ephemeral-storage: "100Gi"  # 100 GB 临时存储
    hugepages-1Gi: "0"
    hugepages-2Mi: "0"
    pods: "110"            # 最多 110 个 Pod(--max-pods)
  
  # 可分配资源(扣除系统预留)
  allocatable:
    cpu: "7500m"           # 7.5 核(预留 0.5 核给系统)
    memory: "30Gi"         # 30 GB(预留 2 GB 给系统)
    ephemeral-storage: "90Gi"
    pods: "110"
  
  # === 节点状态条件 ===
  
  conditions:
    # 节点就绪状态
    - type: Ready
      status: "True"       # True, False, Unknown
      lastHeartbeatTime: "2026-02-10T10:05:00Z"
      lastTransitionTime: "2026-02-10T09:00:00Z"
      reason: KubeletReady
      message: "kubelet is posting ready status"
    
    # 内存压力
    - type: MemoryPressure
      status: "False"
      lastHeartbeatTime: "2026-02-10T10:05:00Z"
      lastTransitionTime: "2026-02-10T09:00:00Z"
      reason: KubeletHasSufficientMemory
      message: "kubelet has sufficient memory available"
    
    # 磁盘压力
    - type: DiskPressure
      status: "False"
      lastHeartbeatTime: "2026-02-10T10:05:00Z"
      lastTransitionTime: "2026-02-10T09:00:00Z"
      reason: KubeletHasNoDiskPressure
      message: "kubelet has no disk pressure"
    
    # PID 压力
    - type: PIDPressure
      status: "False"
      lastHeartbeatTime: "2026-02-10T10:05:00Z"
      lastTransitionTime: "2026-02-10T09:00:00Z"
      reason: KubeletHasSufficientPID
      message: "kubelet has sufficient PID available"
    
    # 网络不可用(仅在 CNI 未就绪时为 True)
    - type: NetworkUnavailable
      status: "False"
      lastHeartbeatTime: "2026-02-10T10:05:00Z"
      lastTransitionTime: "2026-02-10T09:00:00Z"
      reason: RouteCreated
      message: "RouteController created a route"
  
  # === 节点地址 ===
  
  addresses:
    - type: InternalIP       # 内部 IP(集群内通信)
      address: 10.0.1.100
    - type: ExternalIP       # 外部 IP(可选,公网 IP)
      address: 203.0.113.50
    - type: Hostname         # 主机名
      address: node1
    - type: InternalDNS      # 内部 DNS(可选)
      address: node1.internal.example.com
    - type: ExternalDNS      # 外部 DNS(可选)
      address: node1.example.com
  
  # === 守护进程端点 ===
  
  daemonEndpoints:
    # Kubelet 端点
    kubeletEndpoint:
      Port: 10250  # Kubelet API 端口
  
  # === 节点信息 ===
  
  nodeInfo:
    # 操作系统
    operatingSystem: linux
    architecture: amd64
    
    # 内核版本
    kernelVersion: "5.15.0-60-generic"
    
    # 操作系统镜像
    osImage: "Ubuntu 22.04.1 LTS"
    
    # 容器运行时版本
    containerRuntimeVersion: "containerd://1.7.13"
    
    # Kubelet 版本
    kubeletVersion: "v1.32.0"
    
    # Kube-Proxy 版本
    kubeProxyVersion: "v1.32.0"
    
    # Machine ID
    machineID: "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    
    # System UUID
    systemUUID: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
    
    # Boot ID
    bootID: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  
  # === 镜像列表(节点上已缓存的镜像) ===
  
  images:
    - names:
        - "registry.k8s.io/pause:3.9"
        - "registry.k8s.io/pause@sha256:xxx"
      sizeBytes: 514000
    - names:
        - "nginx:1.21"
      sizeBytes: 142000000
  
  # === 卷信息(已挂载的卷) ===
  
  volumesInUse:
    - kubernetes.io/csi/ebs.csi.aws.com^vol-0abcd1234efgh5678
  
  volumesAttached:
    - name: kubernetes.io/csi/ebs.csi.aws.com^vol-0abcd1234efgh5678
      devicePath: /dev/xvda
```

## 3.3 Node Condition 详解

| Condition Type | Status=True 含义 | Status=False 含义 |
|---------------|-----------------|------------------|
| **Ready** | 节点健康,可接收 Pod | 节点异常,不可接收 Pod |
| **MemoryPressure** | 内存压力过大,可能驱逐 Pod | 内存充足 |
| **DiskPressure** | 磁盘压力过大,可能驱逐 Pod | 磁盘充足 |
| **PIDPressure** | 进程数过多,可能驱逐 Pod | 进程数正常 |
| **NetworkUnavailable** | 网络未配置(CNI 未就绪) | 网络正常 |

**触发条件**(Kubelet 参数):

```bash
# 内存压力阈值
--eviction-hard=memory.available<100Mi
--eviction-soft=memory.available<300Mi
--eviction-soft-grace-period=memory.available=1m30s

# 磁盘压力阈值
--eviction-hard=nodefs.available<10%,nodefs.inodesFree<5%
--eviction-soft=nodefs.available<15%,nodefs.inodesFree<10%

# PID 压力阈值
--eviction-hard=pid.available<1000
```

---

<!-- chunk: 4. 生产案例 -->## 4. 生产案例

## 4.1 案例 1: Leader Election 高可用控制器

**场景**: 部署一个自定义控制器,使用 Leader Election 确保单实例运行

```yaml
# controller-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-controller
  namespace: default
spec:
  replicas: 3  # 部署 3 个副本(高可用)
  selector:
    matchLabels:
      app: my-controller
  template:
    metadata:
      labels:
        app: my-controller
    spec:
      serviceAccountName: my-controller
      containers:
        - name: controller
          image: myregistry.com/my-controller:v1.0.0
          args:
            # 启用 Leader Election
            - --leader-elect=true
            - --leader-elect-lease-duration=15s
            - --leader-elect-renew-deadline=10s
            - --leader-elect-retry-period=2s
            - --leader-elect-resource-name=my-controller
            - --leader-elect-resource-namespace=default
          env:
            - name: POD_NAME
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: POD_NAMESPACE
              valueFrom:
                fieldRef:
                  fieldPath: metadata.namespace
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 1000m
              memory: 512Mi

---
# controller-rbac.yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: my-controller
  namespace: default

---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: my-controller-leader-election
  namespace: default
rules:
  # 允许创建和更新 Lease
  - apiGroups: ["coordination.k8s.io"]
    resources: ["leases"]
    verbs: ["get", "create", "update", "patch"]
  # 旧版本控制器可能使用 ConfigMap/Endpoints(向后兼容)
  - apiGroups: [""]
    resources: ["configmaps", "endpoints"]
    verbs: ["get", "create", "update", "patch"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: my-controller-leader-election
  namespace: default
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: my-controller-leader-election
subjects:
  - kind: ServiceAccount
    name: my-controller
    namespace: default

---
# 控制器业务逻辑的其他 RBAC(示例)
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: my-controller
rules:
  - apiGroups: [""]
    resources: ["pods", "services"]
    verbs: ["get", "list", "watch"]
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["get", "list", "watch", "update", "patch"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: my-controller
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: my-controller
subjects:
  - kind: ServiceAccount
    name: my-controller
    namespace: default
```

**验证 Leader Election**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Lease 状态
kubectl get lease my-controller -n default -o yaml

# 输出示例:
# spec:
#   holderIdentity: "my-controller-7d8f9b5c6-abc123"  # 当前 Leader
#   leaseDurationSeconds: 15
#   renewTime: "2026-02-10T10:05:30Z"
#   leaseTransitions: 1

# 查看控制器日志
kubectl logs -n default -l app=my-controller --tail=20

# 输出示例:
# Pod my-controller-7d8f9b5c6-abc123: I am the leader!
# Pod my-controller-7d8f9b5c6-def456: Waiting for leader election...
# Pod my-controller-7d8f9b5c6-ghi789: Waiting for leader election...

# 模拟 Leader 问题
kubectl delete pod my-controller-7d8f9b5c6-abc123 -n default

# 观察新 Leader 产生(通常 10-15 秒内)
kubectl logs -n default -l app=my-controller --tail=20 -f
# 输出:
# Pod my-controller-7d8f9b5c6-def456: Became the leader!
```
## 4.2 案例 2: 通过 Event 调试 Pod 启动失败

**场景**: Pod 一直处于 Pending 状态

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 查看 Pod 基本信息
kubectl get pod my-app -n production
# 输出:
# NAME     READY   STATUS    RESTARTS   AGE
# my-app   0/1     Pending   0          5m

# 2. 查看 Pod Events
kubectl describe pod my-app -n production | grep -A 10 "Events:"
# 输出:
# Events:
#   Type     Reason            Age   From               Message
#   ----     ------            ----  ----               -------
#   Warning  FailedScheduling  3m    default-scheduler  0/5 nodes are available: 
#                                                         2 node(s) had taint {node-role.kubernetes.io/control-plane: }, that the pod didn't tolerate, 
#                                                         3 Insufficient cpu.

# 3. 使用 kubectl get events 查看详细信息
kubectl get events -n production --field-selector involvedObject.name=my-app --sort-by='.lastTimestamp'

# 输出:
# LAST SEEN   TYPE      REASON             OBJECT      MESSAGE
# 5m          Warning   FailedScheduling   Pod/my-app  0/5 nodes are available: 3 Insufficient cpu.

# 4. 分析问题
# 问题: CPU 资源不足,3 个工作节点的可用 CPU 都不满足 Pod 的请求(spec.resources.requests.cpu)

# 5. 解决方案
# 方案 1: 降低 Pod 的 CPU 请求
kubectl edit pod my-app -n production
# 修改: resources.requests.cpu: 1000m → 500m

# 方案 2: 扩容集群节点
# 或 方案 3: 驱逐低优先级 Pod
```
## 4.3 案例 3: 节点维护 - 添加污点驱逐 Pod

**场景**: 需要维护 node2,将所有 Pod 迁移到其他节点

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl taint nodes`：变更污点影响 Pod 调度

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 标记节点为不可调度(禁止新 Pod 调度)
kubectl cordon node2
# 输出:
# node/node2 cordoned

# 2. 添加 NoExecute 污点(驱逐现有 Pod)
kubectl taint nodes node2 maintenance=true:NoExecute
# 输出:
# node/node2 tainted

# 3. 观察 Pod 迁移
kubectl get pods -A -o wide --field-selector spec.nodeName=node2 --watch
# 输出:
# NAMESPACE   NAME      READY   STATUS        RESTARTS   NODE
# default     pod1      1/1     Terminating   0          node2
# default     pod2      1/1     Terminating   0          node2
# (等待 Pod 终止并在其他节点重建)

# 4. 执行维护操作
# (如升级内核、更换硬件等)

# 5. 维护完成后,移除污点并恢复调度
kubectl taint nodes node2 maintenance=true:NoExecute-  # 移除污点
kubectl uncordon node2                                  # 恢复调度
# 输出:
# node/node2 untainted
# node/node2 uncordoned
```
**优雅驱逐(Drain)**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 使用 kubectl drain 命令(相当于 cordon + taint + 等待 Pod 终止)
kubectl drain node2 --ignore-daemonsets --delete-emptydir-data
# 参数说明:
# --ignore-daemonsets: 忽略 DaemonSet(无法迁移)
# --delete-emptydir-data: 删除 emptyDir 卷数据
# --grace-period=60: 优雅终止等待时间(默认 30 秒)
# --timeout=5m: 总超时时间

# 恢复调度
kubectl uncordon node2
```
## 4.4 案例 4: 节点标签管理 - 按硬件类型调度

**场景**: 集群有 GPU 节点和 CPU 节点,AI 训练 Pod 需要调度到 GPU 节点

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 为 GPU 节点添加标签
kubectl label nodes gpu-node1 accelerator=nvidia-tesla-v100
kubectl label nodes gpu-node2 accelerator=nvidia-tesla-v100

# 2. 为 CPU 节点添加标签
kubectl label nodes cpu-node1 accelerator=none
kubectl label nodes cpu-node2 accelerator=none

# 3. 查看节点标签
kubectl get nodes --show-labels | grep accelerator

# 4. AI 训练 Pod 使用 nodeSelector
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: ai-training
spec:
  nodeSelector:
    accelerator: nvidia-tesla-v100  # 仅调度到 GPU 节点
  containers:
    - name: training
      image: nvidia/cuda:12.0-runtime
      resources:
        limits:
          nvidia.com/gpu: 1  # 请求 1 个 GPU
EOF

# 5. 验证调度结果
kubectl get pod ai-training -o wide
# 输出:
# NAME          READY   STATUS    NODE
# ai-training   1/1     Running   gpu-node1
```
**使用 Node Affinity(更灵活)**:

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ai-training-advanced
spec:
  affinity:
    nodeAffinity:
      # 必须满足的条件(硬亲和性)
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
          - matchExpressions:
              # 必须是 GPU 节点
              - key: accelerator
                operator: In
                values:
                  - nvidia-tesla-v100
                  - nvidia-tesla-a100
      # 优先满足的条件(软亲和性)
      preferredDuringSchedulingIgnoredDuringExecution:
        # 优先选择 A100(权重 100)
        - weight: 100
          preference:
            matchExpressions:
              - key: accelerator
                operator: In
                values:
                  - nvidia-tesla-a100
        # 其次选择 us-west 区域(权重 50)
        - weight: 50
          preference:
            matchExpressions:
              - key: topology.kubernetes.io/zone
                operator: In
                values:
                  - us-west-1a
  containers:
    - name: training
      image: nvidia/cuda:12.0-runtime
```

## 4.5 案例 5: 监控节点心跳 Lease

**场景**: 监控节点是否在线(通过 Lease 比 Node Status 更实时)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点的 Lease
kubectl get leases -n kube-node-lease

# 输出:
# NAME    HOLDER   AGE
# node1   node1    30d
# node2   node2    30d
# node3   node3    30d

# 查看特定节点的 Lease 详情
kubectl get lease node1 -n kube-node-lease -o yaml

# 关键字段:
# spec:
#   renewTime: "2026-02-10T10:05:30Z"  # 最后心跳时间
#   leaseDurationSeconds: 40            # 心跳有效期

# 计算节点是否在线:
# 当前时间 - renewTime < leaseDurationSeconds → 在线
# 当前时间 - renewTime >= leaseDurationSeconds → 离线(NotReady)
```
**Prometheus 监控查询(假设有 kube-state-metrics)**:

```promql
# 节点最后心跳时间(Unix 时间戳)
kube_lease_renew_time{namespace="kube-node-lease"}

# 节点离线时间(秒)
time() - kube_lease_renew_time{namespace="kube-node-lease"}

# 节点离线告警(超过 60 秒未心跳)
(time() - kube_lease_renew_time{namespace="kube-node-lease"}) > 60
```

---

<!-- chunk: 5. 故障排查 -->## 5. 故障排查

## 5.1 Leader Election 频繁切换

**症状**: Lease.spec.leaseTransitions 不断增加,控制器日志频繁出现 "lost leadership"

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Lease 切换次数
kubectl get lease my-controller -n default -o jsonpath='{.spec.leaseTransitions}'
# 输出: 50 (异常高,正常应该 < 5)

# 可能原因:
# 1. Leader Pod 频繁重启
kubectl get pods -n default -l app=my-controller
# 检查 RESTARTS 列

# 2. 网络问题导致续约失败
kubectl logs -n default -l app=my-controller | grep -i "lost leadership|network"

# 3. Lease 配置不合理(续约间隔过短)
kubectl get lease my-controller -n default -o jsonpath='{.spec.leaseDurationSeconds}'
# 推荐配置: leaseDuration=15s, renewDeadline=10s, retryPeriod=2s

# 解决方案:
# 1. 增加 Lease 有效期
# 2. 检查 Pod 健康检查配置
# 3. 检查 API Server 负载(是否限流)
```
## 5.2 节点 NotReady 但 Lease 正常

**症状**: Node Condition 为 NotReady,但 Lease 仍在更新

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点状态
kubectl get nodes
# 输出:
# NAME    STATUS     ROLES    AGE   VERSION
# node1   NotReady   <none>   30d   v1.32.0

# 查看 Lease(发现仍在更新)
kubectl get lease node1 -n kube-node-lease -o yaml
# spec:
#   renewTime: "2026-02-10T10:06:00Z"  # 仍在更新!

# 可能原因:
# Kubelet 仍在运行(能续约 Lease),但节点其他组件异常(如 CNI、容器运行时)

# 排查步骤:
# 1. 查看节点 Conditions
kubectl describe node node1 | grep -A 5 "Conditions:"
# 输出:
#   Ready            False   ... KubeletNotReady  container runtime network not ready

# 2. 检查容器运行时
ssh node1
systemctl status containerd  # 或 docker

# 3. 检查 CNI
kubectl logs -n kube-system -l app=calico-node --field-selector spec.nodeName=node1

# 4. 检查 Kubelet 日志
journalctl -u kubelet -n 100
```
## 5.3 Event 过多导致 etcd 压力

**症状**: etcd 存储空间增长过快,大量 Event 对象

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看 Event 数量
kubectl get events --all-namespaces | wc -l
# 输出: 10000 (过多!)

# 查看 etcd 大小
kubectl exec -n kube-system etcd-xxx -- etcdctl endpoint status --write-out=table

# 解决方案:
# 1. 减少 Event TTL(默认 1 小时)
# 在 kube-apiserver 启动参数中添加:
--event-ttl=10m  # 缩短为 10 分钟

# 2. 限制 Event 速率(v1.27+)
# 创建 EventRateLimit Admission 配置:
apiVersion: eventratelimit.admission.k8s.io/v1alpha1
kind: Configuration
limits:
  - type: Namespace
    qps: 50
    burst: 100
    cacheSize: 2000

# 3. 清理历史 Event(临时方案)
kubectl delete events --all-namespaces --field-selector reason=FailedScheduling,type=Warning  # ⚠️ 批量删除，波及面大
```
## 5.4 Pod 调度失败 - 无可用节点

**症状**: Pod Pending,Event 显示 "0/5 nodes are available"

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Event 详细信息
kubectl describe pod my-pod -n default | grep -A 10 "Events:"
# 输出:
# Events:
#   Type     Reason            Message
#   ----     ------            -------
#   Warning  FailedScheduling  0/5 nodes are available:
#                                2 node(s) had taint {node-role.kubernetes.io/control-plane: }, that the pod didn't tolerate,
#                                3 Insufficient memory.

# 分析:
# 1. 2 个节点是控制平面(有污点,Pod 无容忍)
# 2. 3 个工作节点内存不足

# 解决方案:
# 方案 1: 添加容忍(允许调度到控制平面,不推荐)
spec:
  tolerations:
    - key: node-role.kubernetes.io/control-plane
      effect: NoSchedule

# 方案 2: 降低内存请求
spec:
  resources:
    requests:
      memory: 2Gi  # 降低到合理值

# 方案 3: 扩容集群节点
```
## 5.5 调试技巧

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 实时监听 Event
kubectl get events --all-namespaces --watch

# 2. 过滤特定类型 Event
kubectl get events --all-namespaces --field-selector type=Warning

# 3. 按时间排序 Event
kubectl get events --all-namespaces --sort-by='.lastTimestamp'

# 4. 查看特定对象的 Event
kubectl get events --field-selector involvedObject.name=my-pod,involvedObject.namespace=default

# 5. 监控 Lease 变化
kubectl get leases -n kube-node-lease --watch

# 6. 查看节点资源分配情况
kubectl describe nodes | grep -A 5 "Allocated resources:"

# 7. 模拟调度(检查 Pod 为何无法调度)
kubectl get pod my-pod -o yaml | kubectl apply --dry-run=server -f -

```
---

<!-- chunk: 📚 参考资源 -->## 📚 参考资源

- **官方文档**:
  - [Lease API Reference](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/lease-v1/)
  - [Event API Reference](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/event-v1/)
  - [Node API Reference](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/node-v1/)
  - [Leader Election](https://kubernetes.io/blog/2016/01/simple-leader-election-with-kubernetes/)
- **client-go Leader Election**: https://pkg.go.dev/k8s.io/client-go/tools/leaderelection

---

**最佳实践总结**:

## Lease 最佳实践:

1. **Leader Election 配置**: 使用推荐值 `leaseDuration=15s, renewDeadline=10s, retryPeriod=2s`
2. **监控切换次数**: 监控 `leaseTransitions`,异常增长说明稳定性问题
3. **避免多个 Lease**: 每个控制器使用唯一的 Lease 名称,避免冲突
4. **命名规范**: Lease 名称与控制器名称一致,便于排查

## Event 最佳实践:

1. **使用 events.k8s.io/v1**: 优先使用新版本 Event API(v1.19+)
2. **控制 Event 数量**: 配置合理的 `--event-ttl` 和 EventRateLimit
3. **结构化日志**: 关键操作同时记录日志和 Event,便于审计
4. **避免高频 Event**: 使用 Event Series 机制聚合重复事件

## Node 最佳实践:

1. **标签规范化**: 使用标准标签 `topology.kubernetes.io/zone`, `node.kubernetes.io/instance-type` 等
2. **污点管理**: 维护时使用 `kubectl drain`,避免手动删除 Pod
3. **资源预留**: 配置 `--system-reserved` 和 `--kube-reserved`,避免节点资源耗尽
4. **监控 Lease**: 通过 Lease 监控节点心跳,比 Node Status 更实时

---

🚀 **Lease、Event、Node 是 Kubernetes 集群运维的基础组件,掌握它们是集群稳定运行的关键!**

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-32-yaml-manifests MOC
- [[03-清单模式/README.md|Domain-32: Kubernetes YAML 配置完整参考手册]]
- Domain-32 YAML 清单 — 开源项目索引
- 01 - YAML 语法基础与 Kubernetes 资源通用规范
- 02 - Namespace / ResourceQuota / LimitRange YAML 配置参考
- 03 - Pod 完整规格说明书
- 04 - Deployment / ReplicaSet YAML 配置参考
- 05 - StatefulSet YAML 配置参考
- 06 - DaemonSet YAML 配置参考
- 07 - Job / CronJob YAML 配置参考
- 08 - Service 全类型 YAML 配置参考
- 09 - Endpoints / EndpointSlice YAML 配置参考

## See Also

- 30-apiservice-aggregation
- 31-api-priority-fairness
- 33-kubeadm-cluster-bootstrap
- 34-component-configuration

## Related

- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/node-index.md|Node 知识图谱索引]]

```

<!-- risk-assessed -->
