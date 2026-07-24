---
title: Kubernetes 节点管理操作指南
description: '# Kubernetes 节点管理操作指南'
summary: '节点（Node）是 Kubernetes 集群中的工作单元，它是运行 Pod 的物理机或虚拟机。每个节点包含运行 Pod 所需的核心服务：kubelet（节点代理）、容器运行时（containerd/cri-o）和网络插件（CNI）。理解节点的完整生命周期——从创建、注册、运行到维护和移除——是有效管理 Kubernetes 集群的基础。'
category: references
tags:
- k8s
- operations
- node-create
- etcd
- apiserver
- kubelet
- scheduler
- containerd
- cri-o
- docker
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes 节点管理操作指南 是什么
- 如何 Kubernetes 节点管理操作指南
trigger_keywords:
- Kubernetes
- 节点管理操作指南
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 节点管理操作指南

### 01 Overview

#### 概述

节点（Node）是 Kubernetes 集群中的工作单元，它是运行 Pod 的物理机或虚拟机。每个节点包含运行 Pod 所需的核心服务：kubelet（节点代理）、容器运行时（containerd/cri-o）和网络插件（CNI）。理解节点的完整生命周期——从创建、注册、运行到维护和移除——是有效管理 Kubernetes 集群的基础。

Kubernetes 中的节点管理采用了"声明式"的设计哲学：用户通过 API Server 定义期望的节点状态（如标签、污点、规格），kubelet 和各个控制器负责将实际状态与期望状态对齐。这种设计使得节点管理可以自动化——Cluster Autoscaler 可以根据负载自动增减节点，Node Lifecycle Controller 可以自动检测和隔离问题节点。

本文档作为节点生命周期管理的总览，详细介绍节点的核心组件、生命周期阶段、状态流转机制、Node 对象的结构以及节点角色管理。

---

#### 源码路径

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

#### 1.1 完整生命周期流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
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
  │  └── 卷管理 (attach/det
...(截断)

---

### 02 Registration

#### 概述

节点注册是 Kubernetes 节点生命周期的起点。当一台新的机器准备好加入集群时，它需要通过一系列认证和授权步骤才能被集群正式接纳。这个过程称为 TLS Bootstrap（TLS 引导），它允许 kubelet 在没有预先生成证书的情况下，通过 Bootstrap Token 向 API Server 认证，然后发起 CSR（Certificate Signing Request）获取正式的客户端证书。

TLS Bootstrap 的设计目标是简化节点加入集群的流程。在早期版本中，管理员需要手动为每个节点生成证书和 kubeconfig 文件，这在管理数百个节点的集群时极其繁琐。Bootstrap Token 机制通过一个临时的、有限权限的 Token 来引导节点的初始认证，然后自动完成证书签发，极大地简化了节点管理。

完整的节点注册流程涉及多个组件的协作：kubeadm 负责创建 Bootstrap Token，kubelet 负责发起 CSR，csrapproving controller 负责自动审批，csrsigning controller 负责签发证书。本文档从源码层面深入分析这个完整流程。

---

#### 源码路径

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

#### 1.1 流程全景图

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
  
...(截断)

---

### 03 Condition

#### 概述

节点状态（Node Conditions）是 Kubernetes 调度器和管理控制器判断节点健康状态的核心机制。每个节点维护一组 Conditions，包括 Ready、MemoryPressure、DiskPressure、PIDPressure 和 NetworkUnavailable，它们分别反映了节点在不同维度的健康状况。

kubelet 通过定期的状态上报（Status Update）将节点的 Conditions 同步到 API Server。调度器在为 Pod 选择节点时，会检查这些 Conditions 来决定是否将 Pod 调度到该节点。Node Lifecycle Controller 盾牌持续监控节点 Conditions，当节点长时间 NotReady 时触发 Pod 驱逐。

理解 Node Conditions 的工作原理对于以下场景至关重要：

- **调度决策**：理解为什么 Pod 没有被调度到某个节点
- **故障排查**：快速定位节点不健康的根本原因
- **容量管理**：通过资源状态监控实现主动扩容
- **驱逐策略**：理解 Pod 驱逐与节点状态的关系

本文档详细分析每种 Condition 的含义、触发机制、对调度的影响以及源码实现。

---

#### 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 节点状态上报 | `pkg/kubelet/nodestatus/` | Conditions 更新 |
| 驱逐管理 | `pkg/kubelet/eviction/` | 资源压力检测 |
| PLEG | `pkg/kubelet/pleg/` | Pod 生命周期事件 |
| 卷管理 | `pkg/kubelet/volumemanager/` | 卷状态管理 |
| 调度器 | `pkg/scheduler/` | 节点选择 |
| Node Lifecycle Controller | `pkg/controller/nodelifecycle/` | 节点健康监控 |

---

#### 1.1 Condition 结构

```go
// k8s.io/api/core/v1/types.go
type NodeCondition struct {
    Type               NodeConditionType    // Condition 类型
    Status             ConditionStatus      // True / False / Unknown
    LastHeartbeatTime  metav1.Time          // 最后一次上报时间
    LastTransitionTime metav1.Time          // 最后一次状态变化时间
    Reason             string               // 状态变化原因
    Message            string               // 详细描述
}
```

---

### 04 Drain

#### 函数/流程签名

```go
func RunDrain(o *DrainOptions, args []string) error
func (o *DrainOptions) RunCordon() error
func (o *DrainOptions) RunUncordon() error
func (o *DrainOptions) deleteOrEvictPods(pods []corev1.Pod) error
func (o *DrainOptions) evictPod(pod corev1.Pod) error
func (o *DrainOptions) deletePod(pod corev1.Pod) error
func (o *DrainOptions) getPodsForDeletion(nodeName string) ([]corev1.Pod, error)
```

#### 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubectl/pkg/cmd/drain/drain.go` | L50-L200 | `DrainOptions` 结构体定义 |
| `cmd/kubectl/pkg/cmd/drain/drain.go` | L201-L350 | `RunDrain` 主入口 |
| `cmd/kubectl/pkg/cmd/drain/drain.go` | L351-L500 | `deleteOrEvictPods` 驱逐逻辑 |
| `pkg/apis/core/install/versioned.go` | - | Pod eviction API 注册 |
| `pkg/api/legacyscheme/scheme.go` | - | API scheme 注册 |
| `staging/src/k8s.io/api/core/v1/types.go` | L3500-L3600 | Pod 结构体定义 |

#### DrainOptions 参数

| 参数名 | 类型 | 说明 | 验证规则 |
|--------|------|------|---------|
| `nodeName` | `string` | 目标节点名称 | 必须是已存在的节点 |
| `gracePeriodSeconds` | `int` | Pod 优雅终止宽限期 (秒) | -1=使用 Pod 默认值，默认 30 |
| `timeout` | `time.Duration` | drain 超时时间 | 默认 0 (无限等待) |
| `deleteEmptydirData` | `bool` | 允许删除 emptyDir 卷数据的 Pod | 必须显式设置 |
| `ignoreDaemonsets` | `bool` | 忽略 DaemonSet Pod | 必须设置，否则拒绝 drain |
| `disableEviction` | `bool` | 使用 delete 而非 eviction API | 默认 false (优先 eviction) |
| `selector` | `string` | Label selector 过滤 Pod | 标准 label selector 语法 |
| `podSelector` | `string` | Pod label selector | 标准 label selector 语法 |
| `force` | `bool` | 继续即使 Pod 管理器不存在 | 默认 false |
| `dryRun` | `bool` | 只打印不执行 | 默认 false |

---

### 05 Upgrade

#### 概述

节点升级是 Kubernetes 集群版本管理中最重要的运维操作之一。Kubernetes 社区大约每三个月发布一个 minor 版本，每个版本都有约一年的维护支持期。保持集群版本的及时升级对于获取安全补丁、新功能和性能优化至关重要。

Kubernetes 的升级遵循严格的顺序：先升级控制面节点（API Server、Controller Manager、Scheduler、etcd），再升级工作节点。工作节点的升级包括 kubelet 二进制、kubeadm 配置文件和容器运行时组件的更新。升级过程中需要确保工作负载的连续性——通过 drain/uncordon 机制将 Pod 从待升级节点迁移到其他节点。

kubeadm 提供了 `kubeadm upgrade node` 命令来简化工作节点的升级过程。它自动处理配置文件更新、kubelet 服务重启等操作。本文档详细分析节点升级的完整流程、kubeadm 的源码实现、各组件的升级顺序以及常见问题的排查方法。

---

#### 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubeadm upgrade 命令 | `cmd/kubeadm/app/cmd/upgrade/` | 升级命令入口 |
| upgrade node 实现 | `cmd/kubeadm/app/cmd/upgrade/node.go` | 节点升级逻辑 |
| kubelet 升级 | `pkg/kubelet/` | kubelet 相关 |
| 配置更新 | `cmd/kubeadm/app/phases/kubelet/` | kubelet 配置更新 |
| 静态 Pod 更新 | `cmd/kubeadm/app/phases/controlplane/` | 控制面 manifest 更新 |

---

#### 1.1 升级类型

```
集群升级类型:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. Minor 版本升级 (如 1.28 → 1.29)                         │
  │     - 新功能、API 变化、弃用警告                              │
  │     - 需要详细阅读 Release Notes                             │
  │     - 只能逐版本升级 (1.28 → 1.29，不能跳版本)              │
  ├─────────────────────────────────────────────────────────────┤
  │  2. Patch 版本升级 (如 1.28.0 → 1.28.3)                     │
  │     - Bug 修复、安全补丁                                     │
  │     - 无 API 变化                                            │
  │     - 风险较低                                               │
  ├─────────────────────────────────────────────────────────────┤
  │  3. kubelet 单独升级                                        │
  │     - 仅升级 kubelet 二进制                                  │
  │     - 不涉及控制面变更                                        │
  │     - 支持 ±1 个 minor 版本的偏差                            │
  ├─────────────────────────────────────────────────────────────┤
  │  4. OS/内核升级                                              │
  │     - 节点操作系统和内核更新                                  │
  │     - 需要重启节点                                           │
  │     - 不直接影响 Kubernetes 版本                              │
  └─────────────────────────────────────────────────────────────┘
```

---

### 06 Certificate

#### 概述

在 Kubernetes 集群中，每个 kubelet 都需要持有有效的客户端证书才能与 API Server 进行安全通信。证书是有有效期的，过期后 kubelet 将无法连接 API Server，导致节点上的 Pod 无法被管理、日志无法采集、状态无法上报。因此，证书的自动轮换机制对集群稳定性至关重要。

kubelet 的证书管理分为两个阶段：**Bootstrap 阶段**和**正式证书阶段**。在 Bootstrap 阶段，kubelet 使用 Bootstrap Token 向 API Server 发起 CSR（Certificate Signing Request），获取正式的客户端证书。在正式证书阶段，kubelet 会监控证书的有效期，在证书即将过期时自动发起新的 CSR 来续期证书。

这个自动轮换机制从 Kubernetes v1.19 起默认启用，是生产环境中 kubelet 证书管理的标准方案。本文档从源码层面深入分析 kubelet 证书轮换的完整流程、CSR 审批机制、常见问题及解决方案。

---

#### 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubelet 证书管理 | `pkg/kubelet/certificate/` | 证书存储、轮换、CSR 创建 |
| kubelet 主入口 | `pkg/kubelet/kubelet.go` | 证书管理器初始化 |
| CSR 审批控制器 | `pkg/controller/certificates/approval/` | 自动审批 CSR |
| CSR 签发控制器 | `pkg/controller/certificates/` | 证书签发 |
| kubeadm bootstrap | `cmd/kubeadm/app/phases/kubelet/` | Bootstrap Token 逻辑 |
| 证书工具 | `staging/src/k8s.io/client-go/util/cert/` | 证书解析工具 |

---

#### 1.1 两个 kubeconfig 文件

kubelet 在证书管理过程中使用两个不同的 kubeconfig 文件：

```bash
# 1. bootstrap-kubelet.conf (首次启动用)
#    包含 Bootstrap Token，用于向 API Server 发起 CSR
#    路径: /etc/kubernetes/bootstrap-kubelet.conf
#    Token 格式: <token-id>.<token-secret>
#    有效期: 默认 24 小时

# 2. kubelet.conf (正式证书)
#    包含签发后的客户端证书，用于正常的 API Server 通信
#    路径: /etc/kubernetes/kubelet.conf
#    证书路径: /var/lib/kubelet/pki/kubelet-client-current.pem
```

---

### 07 Autoscaling

#### 概述

节点弹性伸缩是 Kubernetes 集群实现成本优化和弹性容量的核心能力。Cluster Autoscaler（CA）是 Kubernetes 官方提供的节点自动伸缩组件，它通过监控集群中不可调度的 Pod（unschedulable Pod）来动态增加节点，通过检测空闲节点来减少节点，从而实现集群容量的自动调整。

Cluster Autoscaler 的设计哲学是"按需伸缩"——当有 Pod 因为资源不足无法调度时，自动扩容节点；当节点上的资源利用率持续低于阈值时，自动缩容节点。这种机制特别适用于以下场景：

- **突发流量**：业务流量突增导致 Pod 需要快速扩容
- **成本优化**：在低峰期自动释放空闲节点以降低云资源成本
- **批处理任务**：临时创建大量节点处理批处理任务，完成后自动释放

Cluster Autoscaler 本身运行在集群中作为一个 Deployment，它通过云厂商的 API（如 AWS ASG、GCP MIG、Azure VMSS）来管理节点的生命周期。本文档详细分析 Cluster Autoscaler 的工作原理、配置方法、各云厂商的集成方式以及常见故障排查。

---

#### 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| Cluster Autoscaler | `kubernetes/autoscaler/cluster-autoscaler/` | CA 核心逻辑 |
| Node Lifecycle Controller | `pkg/controller/nodelifecycle/` | 节点生命周期管理 |
| Cloud Provider 接口 | `pkg/cloudprovider/` | 云厂商接口定义 |
| AWS Cloud Provider | `k8s.io/cloud-provider-aws/` | AWS 实现 |
| GCP Cloud Provider | `k8s.io/cloud-provider-gcp/` | GCP 实现 |
| Azure Cloud Provider | `k8s.io/legacy-cloud-providers/azure/` | Azure 实现 |

---

#### 1.1 核心扩缩流程

```
Cluster Autoscaler 工作循环:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. 定期扫描集群状态 (默认每 10 秒)                          │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  2. 检查是否有 unschedulable Pod                             │
  │     (Pod 的 condition 中有 PodScheduled=False,               │
  │      reason=Unschedulable)                                   │
  └─────────────────────────────────────────────────────────────┘
                            │
               ┌────────────┴────────────┐
               │ 有 unschedulable Pod     │ 无 unschedulable Pod
               ▼                          ▼
  ┌─────────────────────┐   ┌─────────────────────────────────┐
  │ 3a. 扩容流程        │   │ 3b. 缩容流程                     │
  │ - 计算需要的资源    │   │ - 检查空闲节点                   │
  │ - 选择合适的节点组  │   │ - 检查 Pod 可驱逐性              │
  │ - 调用云 API 扩容   │   │ - 调用云 API 缩容               │
  └─────────────────────┘   └─────────────────────────────────┘
```

---

### 08 Troubleshooting

#### 概述

节点故障排查是 Kubernetes 运维中最常见且最关键的任务之一。节点问题可能表现为多种症状：节点 NotReady、Pod 无法启动、网络不通、磁盘满、内存不足等。这些问题的根本原因可能涉及 kubelet 配置错误、证书过期、容器运行时异常、网络插件问题、系统资源耗尽等多个层面。

有效的节点故障排查需要系统化的方法论：

1. **分层排查**：从底层（硬件/OS）向上排查（容器运行时 → kubelet → 控制面 → 网络）
2. **日志分析**：通过 systemd 日志、kubelet 日志、容器日志定位问题
3. **状态检查**：通过 kubectl 命令和 API 查询获取节点和 Pod 的当前状态
4. **对比分析**：将问题节点与正常节点对比，找出差异

本文档提供了全面的节点故障排查指南，涵盖 NotReady 节点、kubelet 启动失败、容器异常、网络问题、磁盘问题和 OOM 等常见场景的排查流程和解决方案。

---

#### 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubelet 核心 | `pkg/kubelet/` | kubelet 主逻辑 |
| kubelet 工具 | `pkg/kubelet/util/` | 工具函数 |
| PLEG | `pkg/kubelet/pleg/` | Pod 生命周期事件 |
| 容器运行时 | `pkg/kubelet/cri/` | CRI 接口 |
| 驱逐管理 | `pkg/kubelet/eviction/` | 驱逐逻辑 |
| 节点状态 | `pkg/kubelet/nodestatus/` | 状态上报 |

---

#### 1.1 排查流程图

```
节点 NotReady 排查:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. kubectl get nodes                                       │
  │     确认节点状态为 NotReady                                   │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  2. syste

---
(内容截断，完整内容见源文件) ---

## 相关链接

- [[技能/节点/node/诊断排障/troubleshoot-node-issues.md|节点故障排查]]
- [[技能/节点/node/诊断排障/ts-node-components.md|节点组件排查]]
- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[实体/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/node-lifecycle-management.md|node-lifecycle-management]] — 节点生命周期管理


<!-- risk-assessed -->
