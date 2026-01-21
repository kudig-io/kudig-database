# cloud-controller-manager 深度解析 (CCM Deep Dive)

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-01 | **文档类型**: 生产级深度解析

cloud-controller-manager (CCM) 是 Kubernetes 与云提供商集成的核心组件，负责管理云特定的控制逻辑，实现节点管理、负载均衡、路由配置等云平台集成功能。

---

## 目录

- [1. 架构概述](#1-架构概述-architecture-overview)
- [2. 核心控制器详解](#2-核心控制器详解-core-controllers)
- [3. Cloud Provider Interface](#3-cloud-provider-interface)
- [4. 阿里云 CCM 深度解析](#4-阿里云-ccm-深度解析-alibaba-cloud)
- [5. AWS CCM 配置详解](#5-aws-ccm-配置详解)
- [6. Azure CCM 配置详解](#6-azure-ccm-配置详解)
- [7. GCP CCM 配置详解](#7-gcp-ccm-配置详解)
- [8. 生产环境部署](#8-生产环境部署-production-deployment)
- [9. 监控与可观测性](#9-监控与可观测性-monitoring--observability)
- [10. 故障排查](#10-故障排查-troubleshooting)
- [11. 生产环境 Checklist](#11-生产环境-checklist)
- [12. 版本兼容性矩阵](#12-版本兼容性矩阵)

---

## 1. 架构概述 (Architecture Overview)

### 1.1 CCM 设计背景与演进

| 版本 | 里程碑 | 变化说明 |
|:---|:---|:---|
| v1.6 | CCM Alpha | 首次引入out-of-tree cloud provider概念 |
| v1.11 | CCM Beta | 主流云厂商开始迁移 |
| v1.20 | in-tree废弃 | 正式废弃in-tree cloud provider |
| v1.26 | in-tree移除 | 完全移除in-tree代码 |
| v1.28+ | 成熟稳定 | 所有主流云厂商完成迁移 |

### 1.2 传统模式 vs CCM 模式对比

| 维度 | 传统模式 (in-tree) | CCM模式 (out-of-tree) |
|:---|:---|:---|
| **云逻辑位置** | 内嵌在kube-controller-manager | 独立组件运行 |
| **发布周期** | 与K8s核心版本绑定 | 可独立发布,快速迭代 |
| **维护责任** | K8s社区统一维护 | 云提供商各自维护 |
| **代码耦合** | 高耦合,改动影响核心 | 低耦合,隔离变更 |
| **升级灵活性** | 需等待K8s版本发布 | 可独立升级CCM |
| **功能扩展** | 受限于K8s发布周期 | 云厂商可快速添加新功能 |
| **二进制大小** | 单体二进制膨胀 | 按需部署,精简二进制 |
| **测试隔离** | 云测试影响核心CI | 独立测试流水线 |

### 1.3 整体架构图

```
                              ┌────────────────────────────────────────────────────────────────┐
                              │                  cloud-controller-manager                       │
                              │                                                                 │
                              │  ┌───────────────────────────────────────────────────────────┐ │
                              │  │                Cloud Provider Interface                    │ │
                              │  │                                                            │ │
                              │  │  ┌─────────────────┐  ┌──────────────────────────────────┐│ │
                              │  │  │    Instances    │  │        LoadBalancer              ││ │
                              │  │  │    Interface    │  │        Interface                 ││ │
                              │  │  │                 │  │                                  ││ │
                              │  │  │ - NodeAddresses │  │ - EnsureLoadBalancer             ││ │
                              │  │  │ - InstanceID    │  │ - UpdateLoadBalancer             ││ │
                              │  │  │ - InstanceType  │  │ - EnsureLoadBalancerDeleted      ││ │
                              │  │  │ - InstanceExists│  │ - GetLoadBalancer                ││ │
                              │  │  └─────────────────┘  └──────────────────────────────────┘│ │
                              │  │  ┌─────────────────┐  ┌──────────────────────────────────┐│ │
                              │  │  │     Routes      │  │          Zones                   ││ │
                              │  │  │    Interface    │  │        Interface                 ││ │
                              │  │  │                 │  │                                  ││ │
                              │  │  │ - ListRoutes    │  │ - GetZone                        ││ │
                              │  │  │ - CreateRoute   │  │ - GetZoneByProviderID            ││ │
                              │  │  │ - DeleteRoute   │  │ - GetZoneByNodeName              ││ │
                              │  │  └─────────────────┘  └──────────────────────────────────┘│ │
                              │  └───────────────────────────────────────────────────────────┘ │
                              │                              │                                  │
                              │  ┌───────────────────────────┴───────────────────────────────┐ │
                              │  │                       Controllers                          │ │
                              │  │  ┌──────────────────┐  ┌────────────────────────────────┐ │ │
                              │  │  │  Node Controller │  │    Service Controller          │ │ │
                              │  │  │                  │  │    (LoadBalancer)              │ │ │
                              │  │  │ - 初始化节点     │  │                                │ │ │
                              │  │  │ - 同步地址       │  │ - 创建/更新/删除云LB           │ │ │
                              │  │  │ - 设置标签       │  │ - 同步后端实例                 │ │ │
                              │  │  │ - 管理污点       │  │ - 配置健康检查                 │ │ │
                              │  │  └──────────────────┘  └────────────────────────────────┘ │ │
                              │  │  ┌──────────────────┐                                     │ │
                              │  │  │ Route Controller │                                     │ │
                              │  │  │                  │                                     │ │
                              │  │  │ - 配置Pod路由    │                                     │ │
                              │  │  │ - 同步VPC路由表  │                                     │ │
                              │  │  └──────────────────┘                                     │ │
                              │  └───────────────────────────────────────────────────────────┘ │
                              └────────────────────────────┬───────────────────────────────────┘
                                                           │
                           ┌───────────────────────────────┼───────────────────────────────┐
                           │                               │                               │
                           ▼                               ▼                               ▼
                    ┌─────────────────┐            ┌─────────────────┐            ┌─────────────────┐
                    │  Cloud Compute  │            │   Cloud Load    │            │   Cloud VPC     │
                    │    Instances    │            │    Balancer     │            │    Routes       │
                    │   (ECS/EC2/VM)  │            │  (SLB/ELB/ALB)  │            │  (Route Table)  │
                    └─────────────────┘            └─────────────────┘            └─────────────────┘
```

### 1.4 从 KCM 分离的控制器详解

| 控制器 | CCM接管部分 | KCM保留部分 | 分离原因 |
|:---|:---|:---|:---|
| **Node Controller** | 云实例信息获取、地址同步、ProviderID设置 | 节点生命周期、心跳检测、污点管理 | 节点元数据来自云API |
| **Service Controller** | 完整的LoadBalancer管理 | ClusterIP/NodePort | LB完全依赖云服务 |
| **Route Controller** | 完整的VPC路由管理 | - | 路由配置是云特定的 |
| **Volume Controller** | Attach/Detach操作(已迁移至CSI) | PV/PVC绑定、Provisioner | 存储操作需云API |

### 1.5 CCM 内部工作流

```
CCM 启动流程:
┌─────────────────────────────────────────────────────────────────────────────┐
│  1. 初始化阶段                                                               │
│     ├─ 加载云配置文件 (--cloud-config)                                      │
│     ├─ 初始化云提供商客户端                                                  │
│     ├─ 验证云API凭证                                                        │
│     └─ 启动健康检查端点                                                      │
│                                                                              │
│  2. Leader 选举                                                              │
│     ├─ 创建/获取 Lease 对象                                                  │
│     ├─ 竞争成为 Leader                                                       │
│     └─ 非Leader实例进入等待状态                                              │
│                                                                              │
│  3. 控制器启动 (Leader)                                                      │
│     ├─ 启动 Node Controller                                                  │
│     │   ├─ 初始化 SharedInformer (Node)                                     │
│     │   └─ 启动 Worker Goroutines                                           │
│     ├─ 启动 Service Controller                                              │
│     │   ├─ 初始化 SharedInformer (Service, Node, Endpoints)                 │
│     │   └─ 启动 Worker Goroutines                                           │
│     └─ 启动 Route Controller                                                │
│         ├─ 初始化 SharedInformer (Node)                                     │
│         └─ 启动定期同步协程                                                  │
│                                                                              │
│  4. 事件处理循环                                                             │
│     ├─ Watch API Server 资源变化                                            │
│     ├─ 入队工作项                                                            │
│     ├─ Worker 处理工作项                                                    │
│     └─ 调用云API执行操作                                                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 核心控制器详解 (Core Controllers)

### 2.1 Node Controller

#### 2.1.1 职责概述

| 功能 | 说明 | 触发条件 | 云API调用 |
|:---|:---|:---|:---|
| **初始化节点** | 添加云特定信息到新节点 | 新节点注册 | GetInstance, GetZone |
| **更新节点地址** | 同步云实例IP地址 | 定期同步/实例变化 | GetNodeAddresses |
| **添加拓扑标签** | 设置region/zone等标签 | 节点初始化 | GetZone |
| **设置Provider ID** | 云实例唯一标识符 | 节点初始化 | GetInstanceID |
| **检测节点删除** | 同步已删除的云实例 | 定期检查 | InstanceExists |
| **管理初始化污点** | 控制节点调度状态 | 初始化完成时 | - |

#### 2.1.2 节点初始化详细流程

```
                    节点初始化流程详解
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│   1. Kubelet 启动 (--cloud-provider=external)                               │
│      │                                                                       │
│      └─▶ 向 API Server 注册节点                                             │
│          │                                                                   │
│          └─▶ API Server 自动添加 Taint:                                     │
│              node.cloudprovider.kubernetes.io/uninitialized=true:NoSchedule │
│                                                                              │
│   2. CCM Node Controller 检测到新节点 (Watch)                               │
│      │                                                                       │
│      └─▶ 入队工作项 (WorkQueue)                                             │
│                                                                              │
│   3. Worker 处理初始化                                                       │
│      │                                                                       │
│      ├─▶ 调用 cloud.InstancesV2().InstanceMetadata()                        │
│      │   └─ 获取: ProviderID, InstanceType, NodeAddresses, Zone             │
│      │                                                                       │
│      ├─▶ 更新 Node.Spec                                                     │
│      │   └─ spec.providerID = "alicloud:///cn-hangzhou/i-xxx"               │
│      │                                                                       │
│      ├─▶ 更新 Node.Status.Addresses                                         │
│      │   ├─ InternalIP: 172.16.0.10                                         │
│      │   ├─ ExternalIP: 47.xxx.xxx.xxx (如有)                               │
│      │   └─ Hostname: iZxxx                                                 │
│      │                                                                       │
│      ├─▶ 添加 Node Labels                                                   │
│      │   ├─ topology.kubernetes.io/region: cn-hangzhou                      │
│      │   ├─ topology.kubernetes.io/zone: cn-hangzhou-h                      │
│      │   ├─ node.kubernetes.io/instance-type: ecs.g6.xlarge                 │
│      │   └─ (云厂商特定标签)                                                 │
│      │                                                                       │
│      └─▶ 移除初始化 Taint                                                    │
│          └─ 节点可以开始调度 Pod                                             │
│                                                                              │
│   4. 持续同步 (定期/事件触发)                                                │
│      ├─ 检查实例是否存在                                                     │
│      ├─ 同步IP地址变化                                                       │
│      └─ 更新实例状态                                                         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 2.1.3 CCM 添加的节点信息示例

```yaml
apiVersion: v1
kind: Node
metadata:
  name: cn-hangzhou.172.16.0.10
  labels:
    # CCM 添加的拓扑标签
    topology.kubernetes.io/region: cn-hangzhou
    topology.kubernetes.io/zone: cn-hangzhou-h
    node.kubernetes.io/instance-type: ecs.g6.xlarge
    
    # 阿里云特定标签
    alibabacloud.com/nodepool-id: np-xxxxx
    alibabacloud.com/instance-type-family: ecs.g6
    
    # 系统标签
    kubernetes.io/os: linux
    kubernetes.io/arch: amd64
spec:
  # CCM 设置的 Provider ID
  providerID: alicloud:///cn-hangzhou-h/i-bp1xxxxxxxxx
  
  # Pod CIDR (如果由CCM分配)
  podCIDR: 10.244.1.0/24
  podCIDRs:
  - 10.244.1.0/24
  
status:
  # CCM 更新的地址信息
  addresses:
  - type: InternalIP
    address: 172.16.0.10
  - type: ExternalIP
    address: 47.xxx.xxx.xxx
  - type: Hostname
    address: cn-hangzhou.172.16.0.10
    
  # 节点容量 (部分由云信息补充)
  capacity:
    cpu: "4"
    memory: 16384Mi
    pods: "110"
    
  # 节点状态
  conditions:
  - type: Ready
    status: "True"
    lastHeartbeatTime: "2026-01-21T10:00:00Z"
    lastTransitionTime: "2026-01-20T08:00:00Z"
    
  nodeInfo:
    machineID: 20f5xxxx
    systemUUID: ECS-INST-xxxxx
    bootID: xxxxxxxx
    kernelVersion: 5.10.134-16.al8.x86_64
    osImage: Alibaba Cloud Linux 3
    containerRuntimeVersion: containerd://1.6.28
    kubeletVersion: v1.30.4
    kubeProxyVersion: v1.30.4
    architecture: amd64
    operatingSystem: linux
```

### 2.2 Service Controller (LoadBalancer)

#### 2.2.1 职责概述

| 操作 | 触发条件 | 云API调用 | 结果 |
|:---|:---|:---|:---|
| **创建LB** | Service type=LoadBalancer 创建 | CreateLoadBalancer | 分配LB IP/DNS |
| **更新LB** | Service ports/selector变化 | UpdateLoadBalancer | 同步配置 |
| **删除LB** | Service删除 | DeleteLoadBalancer | 释放云资源 |
| **同步后端** | Node Ready状态变化 | UpdateTargetGroup | 更新后端实例 |
| **健康检查** | 配置变化 | UpdateHealthCheck | 同步探测配置 |

#### 2.2.2 Service Controller 工作流

```
Service Controller 详细工作流:
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│   Watch: Service (type=LoadBalancer), Node, Endpoints                       │
│                      │                                                       │
│                      ▼                                                       │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                    Service 事件处理                                  │   │
│   │                                                                      │   │
│   │   CREATE Service (type=LoadBalancer)                                │   │
│   │   │                                                                  │   │
│   │   └─▶ EnsureLoadBalancer()                                          │   │
│   │       │                                                              │   │
│   │       ├─▶ [1] 解析 Service Annotations                              │   │
│   │       │   ├─ 负载均衡器类型 (CLB/NLB/ALB)                            │   │
│   │       │   ├─ 网络类型 (公网/内网)                                    │   │
│   │       │   ├─ 规格/带宽/计费                                          │   │
│   │       │   └─ 健康检查/会话保持等                                     │   │
│   │       │                                                              │   │
│   │       ├─▶ [2] 创建/获取 LoadBalancer                                │   │
│   │       │   ├─ 检查是否指定已有LB ID                                   │   │
│   │       │   ├─ 调用云API创建LB实例                                     │   │
│   │       │   └─ 等待LB就绪                                              │   │
│   │       │                                                              │   │
│   │       ├─▶ [3] 配置监听器 (Listeners)                                │   │
│   │       │   ├─ 根据 Service.Spec.Ports 创建监听器                      │   │
│   │       │   ├─ 配置协议 (TCP/UDP/HTTP/HTTPS)                          │   │
│   │       │   └─ 配置端口映射                                            │   │
│   │       │                                                              │   │
│   │       ├─▶ [4] 配置后端服务器组                                       │   │
│   │       │   ├─ 获取 Ready 状态的 Node 列表                             │   │
│   │       │   ├─ 过滤: 排除 Master/Cordoned 节点                         │   │
│   │       │   ├─ 应用 externalTrafficPolicy                              │   │
│   │       │   └─ 创建/更新后端服务器组                                    │   │
│   │       │                                                              │   │
│   │       ├─▶ [5] 配置健康检查                                           │   │
│   │       │   ├─ 检查类型 (TCP/HTTP)                                     │   │
│   │       │   ├─ 端口/路径/间隔/阈值                                     │   │
│   │       │   └─ 应用健康检查配置                                         │   │
│   │       │                                                              │   │
│   │       └─▶ [6] 更新 Service Status                                   │   │
│   │           └─ status.loadBalancer.ingress[].ip/hostname              │   │
│   │                                                                      │   │
│   │   UPDATE Service                                                    │   │
│   │   │                                                                  │   │
│   │   └─▶ UpdateLoadBalancer()                                          │   │
│   │       ├─ 对比当前配置与期望配置                                       │   │
│   │       ├─ 更新监听器/后端/健康检查                                     │   │
│   │       └─ 更新 Service Status                                         │   │
│   │                                                                      │   │
│   │   DELETE Service                                                    │   │
│   │   │                                                                  │   │
│   │   └─▶ EnsureLoadBalancerDeleted()                                   │   │
│   │       ├─ 检查 preserve-lb-on-delete 注解                             │   │
│   │       ├─ 删除监听器                                                  │   │
│   │       ├─ 删除后端服务器组                                            │   │
│   │       └─ 删除 LB 实例 (如果不保留)                                   │   │
│   │                                                                      │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│   Node 事件处理                                                             │
│   │                                                                          │
│   └─▶ 触发关联 Service 的后端同步                                           │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 2.2.3 externalTrafficPolicy 详解

| Policy | 后端选择 | 源IP保留 | 负载分布 | 适用场景 |
|:---|:---|:---|:---|:---|
| **Cluster** (默认) | 所有Ready节点 | 否 (SNAT) | 均匀分布 | 高可用,不需要源IP |
| **Local** | 仅有Pod的节点 | 是 | 可能不均匀 | 需要源IP,如日志审计 |

```yaml
# externalTrafficPolicy: Cluster (默认)
# 流量路径: Client → LB → 任意Node → Pod (可能跨节点)
#
# 优点: 负载均匀,高可用
# 缺点: 额外一跳,源IP被SNAT

---
# externalTrafficPolicy: Local
# 流量路径: Client → LB → 有Pod的Node → Pod (同节点)
#
# 优点: 保留源IP,减少网络跳数
# 缺点: 负载可能不均匀,需要足够副本分布

apiVersion: v1
kind: Service
metadata:
  name: nginx-local-policy
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  # 健康检查端口 (Cluster模式不需要)
  healthCheckNodePort: 30000
  selector:
    app: nginx
  ports:
  - port: 80
    targetPort: 80
```

### 2.3 Route Controller

#### 2.3.1 职责与限制

| 功能 | 说明 | 适用场景 |
|:---|:---|:---|
| **创建路由** | 为每个节点的PodCIDR创建VPC路由 | Flannel host-gw/云原生网络 |
| **删除路由** | 节点删除时清理路由 | 节点下线 |
| **同步路由** | 定期检查并修复路由 | 路由漂移修复 |

```
Route Controller 工作流:
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│   目标: 确保每个节点的 PodCIDR 在云VPC路由表中可达                          │
│                                                                              │
│   Watch Node 变化                                                           │
│         │                                                                    │
│         ▼                                                                    │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                     路由同步处理                                     │   │
│   │                                                                      │   │
│   │   1. 新节点加入                                                     │   │
│   │      └─▶ CreateRoute()                                              │   │
│   │          ├─ 获取节点 PodCIDR (spec.podCIDR)                         │   │
│   │          ├─ 创建路由规则:                                           │   │
│   │          │   Destination: 10.244.1.0/24 (PodCIDR)                   │   │
│   │          │   NextHop: i-xxxx (Node Instance ID)                     │   │
│   │          └─ 更新云VPC路由表                                          │   │
│   │                                                                      │   │
│   │   2. 节点删除                                                       │   │
│   │      └─▶ DeleteRoute()                                              │   │
│   │          └─ 删除对应的路由规则                                       │   │
│   │                                                                      │   │
│   │   3. 定期同步 (--route-reconciliation-period)                       │   │
│   │      └─▶ reconcile()                                                │   │
│   │          ├─ ListRoutes() - 获取云端当前路由                          │   │
│   │          ├─ 计算期望路由 (基于Node列表)                              │   │
│   │          └─ 同步差异:                                               │   │
│   │              ├─ 创建缺失的路由                                       │   │
│   │              ├─ 删除多余的路由                                       │   │
│   │              └─ 更新错误的路由                                       │   │
│   │                                                                      │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│   VPC 路由表示例 (阿里云):                                                  │
│   ┌───────────────────────────────────────────────────────────────────────┐ │
│   │  目标网段           │   下一跳类型    │   下一跳          │  状态     │ │
│   ├───────────────────────────────────────────────────────────────────────┤ │
│   │  172.16.0.0/16      │   Local        │   -               │  Active   │ │
│   │  10.244.0.0/24      │   ECS Instance │   i-node1         │  Active   │ │
│   │  10.244.1.0/24      │   ECS Instance │   i-node2         │  Active   │ │
│   │  10.244.2.0/24      │   ECS Instance │   i-node3         │  Active   │ │
│   │  0.0.0.0/0          │   NAT Gateway  │   nat-xxxx        │  Active   │ │
│   └───────────────────────────────────────────────────────────────────────┘ │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 2.3.2 Route Controller 注意事项

| 注意事项 | 说明 | 建议 |
|:---|:---|:---|
| **路由表限制** | 云VPC路由表条目有上限 | AWS: 50条, 阿里云: 48条(可申请提升) |
| **CNI兼容性** | 仅部分CNI需要 | Flannel host-gw需要,Calico IPIP/VXLAN不需要 |
| **启用条件** | 需显式启用 | `--configure-cloud-routes=true` |
| **CIDR分配** | 需配合CIDR分配 | `--allocate-node-cidrs=true` |

---

## 3. Cloud Provider Interface

### 3.1 核心接口定义

```go
// cloudprovider.Interface - 主接口
type Interface interface {
    // Initialize 初始化云提供商,在控制器启动前调用
    Initialize(clientBuilder cloudprovider.ControllerClientBuilder, stop <-chan struct{})
    
    // LoadBalancer 返回LoadBalancer接口实现
    LoadBalancer() (LoadBalancer, bool)
    
    // Instances 返回Instances接口实现 (已废弃,推荐InstancesV2)
    Instances() (Instances, bool)
    
    // InstancesV2 返回InstancesV2接口实现 (v1.20+推荐)
    InstancesV2() (InstancesV2, bool)
    
    // Zones 返回Zones接口实现
    Zones() (Zones, bool)
    
    // Clusters 返回Clusters接口实现 (很少使用)
    Clusters() (Clusters, bool)
    
    // Routes 返回Routes接口实现
    Routes() (Routes, bool)
    
    // ProviderName 返回云提供商名称
    ProviderName() string
    
    // HasClusterID 是否支持集群ID
    HasClusterID() bool
}

// LoadBalancer 接口 - 负载均衡管理
type LoadBalancer interface {
    // GetLoadBalancer 获取LB当前状态
    GetLoadBalancer(ctx context.Context, clusterName string, service *v1.Service) (*v1.LoadBalancerStatus, bool, error)
    
    // GetLoadBalancerName 返回LB名称
    GetLoadBalancerName(ctx context.Context, clusterName string, service *v1.Service) string
    
    // EnsureLoadBalancer 创建/更新LB以匹配期望状态
    EnsureLoadBalancer(ctx context.Context, clusterName string, service *v1.Service, nodes []*v1.Node) (*v1.LoadBalancerStatus, error)
    
    // UpdateLoadBalancer 更新LB后端
    UpdateLoadBalancer(ctx context.Context, clusterName string, service *v1.Service, nodes []*v1.Node) error
    
    // EnsureLoadBalancerDeleted 删除LB
    EnsureLoadBalancerDeleted(ctx context.Context, clusterName string, service *v1.Service) error
}

// InstancesV2 接口 - 实例信息查询 (v1.20+)
type InstancesV2 interface {
    // InstanceExists 检查实例是否存在
    InstanceExists(ctx context.Context, node *v1.Node) (bool, error)
    
    // InstanceShutdown 检查实例是否关机
    InstanceShutdown(ctx context.Context, node *v1.Node) (bool, error)
    
    // InstanceMetadata 获取实例元数据
    InstanceMetadata(ctx context.Context, node *v1.Node) (*InstanceMetadata, error)
}

// InstanceMetadata 实例元数据结构
type InstanceMetadata struct {
    ProviderID    string          // 云实例唯一ID
    InstanceType  string          // 实例规格
    NodeAddresses []v1.NodeAddress // 网络地址列表
    Zone          string          // 可用区
    Region        string          // 地域
}

// Routes 接口 - 路由管理
type Routes interface {
    // ListRoutes 列出所有路由
    ListRoutes(ctx context.Context, clusterName string) ([]*cloudprovider.Route, error)
    
    // CreateRoute 创建路由
    CreateRoute(ctx context.Context, clusterName string, nameHint string, route *cloudprovider.Route) error
    
    // DeleteRoute 删除路由
    DeleteRoute(ctx context.Context, clusterName string, route *cloudprovider.Route) error
}

// Zones 接口 - 可用区信息
type Zones interface {
    // GetZone 获取当前节点所在Zone
    GetZone(ctx context.Context) (Zone, error)
    
    // GetZoneByProviderID 通过ProviderID获取Zone
    GetZoneByProviderID(ctx context.Context, providerID string) (Zone, error)
    
    // GetZoneByNodeName 通过节点名获取Zone
    GetZoneByNodeName(ctx context.Context, nodeName types.NodeName) (Zone, error)
}
```

### 3.2 主流云提供商实现对比

| 云提供商 | 项目仓库 | 最新版本 | LoadBalancer | Routes | InstancesV2 |
|:---|:---|:---|:---|:---|:---|
| **阿里云** | kubernetes/cloud-provider-alibaba-cloud | v2.12.4 | CLB/NLB/ALB | VPC Routes | ECS |
| **AWS** | kubernetes/cloud-provider-aws | v1.32.0 | NLB/ALB/CLB | VPC Routes | EC2 |
| **Azure** | kubernetes-sigs/cloud-provider-azure | v1.32.11 | Azure LB | Route Tables | VMSS/VM |
| **GCP** | kubernetes/cloud-provider-gcp | v32.0.0 | TCP/HTTP LB | VPC Routes | GCE |
| **腾讯云** | TencentCloud/cloud-provider-tencent | v1.28.0 | CLB | VPC Routes | CVM |
| **华为云** | kubernetes-sigs/cloud-provider-huaweicloud | v0.28.0 | ELB | VPC Routes | ECS |
| **OpenStack** | kubernetes/cloud-provider-openstack | v1.32.0 | Octavia | Neutron | Nova |

---

## 4. 阿里云 CCM 深度解析 (Alibaba Cloud)

### 4.1 版本兼容性矩阵

| CCM 版本 | K8s 版本 | 功能特性 | 发布日期 | 状态 |
|:---|:---|:---|:---|:---|
| **v2.12.4** | v1.24-v1.32 | ReadinessGate默认开启,NLB服务器组清理修复 | 2025-12 | 推荐 |
| **v2.12.1** | v1.24-v1.31 | CLB计费默认PayByCLCU | 2025-09 | 稳定 |
| **v2.11.2** | v1.24-v1.30 | NLB IP型后端支持 | 2025-06 | 稳定 |
| **v2.10.0** | v1.24-v1.30 | ReadinessGate,标签修改,删除保留LB | 2024-10 | LTS |
| **v2.9.1** | v1.22-v1.29 | 资源组继承,IPv6双栈后端 | 2024-05 | 维护中 |

### 4.2 负载均衡器类型对比

| 特性 | CLB (传统型) | NLB (网络型) | ALB (应用型) |
|:---|:---|:---|:---|
| **工作层级** | L4 (TCP/UDP) | L4 (TCP/UDP/TCPSSL) | L7 (HTTP/HTTPS/gRPC) |
| **配置方式** | Service annotations | Service + loadBalancerClass | Ingress + AlbConfig CRD |
| **性能** | 中等 | 超高性能,100万QPS | 高性能 |
| **计费模式** | PayBySpec/PayByCLCU | 按量付费 | 按量付费 |
| **会话保持** | 支持 | 支持 | 支持 (Cookie/Header) |
| **健康检查** | TCP/HTTP | TCP/HTTP | HTTP/HTTPS/gRPC |
| **双栈支持** | 部分 | IPv4/IPv6 | IPv4/IPv6 |
| **适用场景** | 通用TCP/UDP服务 | 高性能四层服务 | HTTP/HTTPS应用 |

### 4.3 CLB (传统型负载均衡) 完整配置

#### 4.3.1 CLB 注解速查表

| 注解 (省略前缀 service.beta.kubernetes.io/) | 值类型 | 默认值 | 说明 |
|:---|:---|:---|:---|
| **基础配置** ||||
| `alibaba-cloud-loadbalancer-address-type` | string | internet | 网络类型: internet/intranet |
| `alibaba-cloud-loadbalancer-id` | string | - | 复用已有SLB实例ID |
| `alibaba-cloud-loadbalancer-spec` | string | slb.s1.small | 实例规格 |
| `alibaba-cloud-loadbalancer-name` | string | auto | 实例名称 |
| `alibaba-cloud-loadbalancer-vswitch-id` | string | - | 内网SLB所在交换机 |
| **计费配置** ||||
| `alibaba-cloud-loadbalancer-instance-charge-type` | string | PayByCLCU | PayBySpec/PayByCLCU |
| `alibaba-cloud-loadbalancer-charge-type` | string | paybytraffic | paybytraffic/paybybandwidth |
| `alibaba-cloud-loadbalancer-bandwidth` | int | 50 | 带宽上限(Mbps) |
| **监听器配置** ||||
| `alibaba-cloud-loadbalancer-protocol-port` | string | - | 协议端口映射,如 "https:443,http:80" |
| `alibaba-cloud-loadbalancer-scheduler` | string | wrr | 调度算法: wrr/rr/wlc/sch/tch |
| `alibaba-cloud-loadbalancer-persistence-timeout` | int | 0 | 会话保持超时(秒),0禁用 |
| `alibaba-cloud-loadbalancer-sticky-session` | string | off | 会话保持: on/off |
| `alibaba-cloud-loadbalancer-sticky-session-type` | string | insert | insert/server |
| `alibaba-cloud-loadbalancer-cookie-timeout` | int | 86400 | Cookie超时(秒) |
| **健康检查配置** ||||
| `alibaba-cloud-loadbalancer-health-check-flag` | string | on | 健康检查开关: on/off |
| `alibaba-cloud-loadbalancer-health-check-type` | string | tcp | tcp/http |
| `alibaba-cloud-loadbalancer-health-check-connect-port` | int | - | 检查端口 |
| `alibaba-cloud-loadbalancer-health-check-connect-timeout` | int | 5 | 响应超时(秒) |
| `alibaba-cloud-loadbalancer-health-check-interval` | int | 2 | 检查间隔(秒) |
| `alibaba-cloud-loadbalancer-healthy-threshold` | int | 3 | 健康阈值 |
| `alibaba-cloud-loadbalancer-unhealthy-threshold` | int | 3 | 不健康阈值 |
| `alibaba-cloud-loadbalancer-health-check-uri` | string | / | HTTP检查路径 |
| `alibaba-cloud-loadbalancer-health-check-domain` | string | - | HTTP检查域名 |
| `alibaba-cloud-loadbalancer-health-check-method` | string | head | head/get |
| **SSL/TLS配置** ||||
| `alibaba-cloud-loadbalancer-cert-id` | string | - | SSL证书ID |
| `alibaba-cloud-loadbalancer-tls-cipher-policy` | string | tls_cipher_policy_1_0 | TLS安全策略 |
| `alibaba-cloud-loadbalancer-forward-port` | string | - | HTTP→HTTPS转发,如 "80:443" |
| **后端配置** ||||
| `alibaba-cloud-loadbalancer-backend-label` | string | - | 后端节点标签选择器 |
| `backend-type` | string | - | eni/ecs 后端类型 |
| **高级配置** ||||
| `alibaba-cloud-loadbalancer-delete-protection` | string | on | 删除保护: on/off |
| `alibaba-cloud-loadbalancer-modification-protection` | string | ConsoleProtection | 修改保护 |
| `alibaba-cloud-loadbalancer-force-override-listeners` | string | false | 强制覆盖监听器 |
| `alibaba-cloud-loadbalancer-additional-resource-tags` | string | - | 附加标签: "k1=v1,k2=v2" |
| `alibaba-cloud-loadbalancer-resource-group-id` | string | - | 资源组ID |
| `alibaba-cloud-loadbalancer-ip-version` | string | ipv4 | IP版本: ipv4/ipv6 |

#### 4.3.2 CLB 生产环境配置示例

```yaml
# 生产级 CLB 配置 - 内网 HTTPS 服务
apiVersion: v1
kind: Service
metadata:
  name: production-api
  namespace: production
  annotations:
    # === 基础配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-vswitch-id: "vsw-bp1xxxxxxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s3.large"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-name: "prod-api-slb"
    
    # === 计费配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-instance-charge-type: "PayByCLCU"
    
    # === 监听器配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "https:443"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-scheduler: "wrr"
    
    # === SSL证书 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cert-id: "1234567890-cn-hangzhou"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-tls-cipher-policy: "tls_cipher_policy_1_2"
    
    # === 健康检查 (严格配置) ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-timeout: "5"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "2"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "3"
    
    # === 会话保持 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-sticky-session: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-sticky-session-type: "insert"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cookie-timeout: "1800"
    
    # === 安全配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-delete-protection: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-modification-protection: "ConsoleProtection"
    
    # === 资源管理 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-additional-resource-tags: "env=prod,team=platform"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-resource-group-id: "rg-xxxxx"
    
    # === 后端配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-backend-label: "app.kubernetes.io/name=api-server"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留源IP
  selector:
    app: api-server
  ports:
  - name: https
    port: 443
    targetPort: 8443
    protocol: TCP
---
# HTTP → HTTPS 重定向配置
apiVersion: v1
kind: Service
metadata:
  name: production-api-redirect
  namespace: production
  annotations:
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-id: "${CLB_INSTANCE_ID}"  # 复用同一个CLB
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "http:80"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-forward-port: "80:443"
spec:
  type: LoadBalancer
  selector:
    app: api-server
  ports:
  - name: http
    port: 80
    targetPort: 8080
```

### 4.4 NLB (网络型负载均衡) 完整配置

#### 4.4.1 NLB 注解速查表

| 注解 (省略前缀 service.beta.kubernetes.io/) | 值类型 | 默认值 | 说明 |
|:---|:---|:---|:---|
| **实例配置** ||||
| `alibaba-cloud-loadbalancer-zone-maps` | string | - | 可用区映射(必需) |
| `alibaba-cloud-loadbalancer-address-type` | string | internet | internet/intranet |
| `alibaba-cloud-loadbalancer-ip-version` | string | ipv4 | ipv4/DualStack |
| `alibaba-cloud-loadbalancer-ipv6-address-type` | string | intranet | IPv6地址类型 |
| `alibaba-cloud-loadbalancer-name` | string | auto | 实例名称(2-128字符) |
| `alibaba-cloud-loadbalancer-id` | string | - | 复用已有NLB |
| **带宽配置** ||||
| `alibaba-cloud-loadbalancer-bandwidth-package-id` | string | - | 共享带宽包ID |
| **监听器配置** ||||
| `alibaba-cloud-loadbalancer-protocol-port` | string | - | 协议:端口,如 "TCP:80,TCPSSL:443" |
| `alibaba-cloud-loadbalancer-cps` | int | - | 每秒最大连接数 |
| `alibaba-cloud-loadbalancer-idle-timeout` | int | 900 | 空闲超时(10-900秒) |
| `alibaba-cloud-loadbalancer-proxy-protocol` | string | off | Proxy Protocol: on/off |
| **健康检查** ||||
| `alibaba-cloud-loadbalancer-health-check-flag` | string | on | on/off |
| `alibaba-cloud-loadbalancer-health-check-type` | string | tcp | tcp/http |
| `alibaba-cloud-loadbalancer-health-check-uri` | string | / | HTTP检查路径 |
| `alibaba-cloud-loadbalancer-health-check-domain` | string | $SERVER_IP | 检查域名 |
| `alibaba-cloud-loadbalancer-health-check-connect-port` | int | 0 | 检查端口,0=后端端口 |
| `alibaba-cloud-loadbalancer-health-check-connect-timeout` | int | 5 | 超时(1-300秒) |
| `alibaba-cloud-loadbalancer-healthy-threshold` | int | 2 | 健康阈值(2-10) |
| `alibaba-cloud-loadbalancer-unhealthy-threshold` | int | 2 | 不健康阈值(2-10) |
| `alibaba-cloud-loadbalancer-health-check-interval` | int | 10 | 间隔(1-50秒) |
| `alibaba-cloud-loadbalancer-health-check-method` | string | GET | GET/HEAD |
| **后端服务器组** ||||
| `alibaba-cloud-loadbalancer-scheduler` | string | wrr | wrr/rr/sch/tch/wlc |
| `alibaba-cloud-loadbalancer-connection-drain` | string | off | 连接排空: on/off |
| `alibaba-cloud-loadbalancer-connection-drain-timeout` | int | 10 | 排空超时(10-900秒) |
| `alibaba-cloud-loadbalancer-preserve-client-ip` | string | off | 保留源IP: on/off |
| `alibaba-cloud-loadbalancer-server-group-type` | string | Instance | Instance/Ip |
| `alibaba-cloud-loadbalancer-weight` | int | 100 | 默认权重(1-100) |
| **SSL/TLS配置** ||||
| `alibaba-cloud-loadbalancer-cert-id` | string | - | 服务器证书ID |
| `alibaba-cloud-loadbalancer-cacert-id` | string | - | CA证书ID |
| `alibaba-cloud-loadbalancer-cacert` | string | off | 双向认证: on/off |
| `alibaba-cloud-loadbalancer-tls-cipher-policy` | string | - | TLS策略ID |
| `alibaba-cloud-loadbalancer-alpn` | string | off | ALPN: on/off |
| `alibaba-cloud-loadbalancer-alpn-policy` | string | HTTP1Only | HTTP1Only/HTTP2Preferred 等 |
| **安全配置** ||||
| `alibaba-cloud-loadbalancer-delete-protection` | string | on | 删除保护: on/off |
| `alibaba-cloud-loadbalancer-modification-protection` | string | ConsoleProtection | 修改保护 |
| `alibaba-cloud-loadbalancer-security-group-ids` | string | - | 安全组ID列表 |
| **其他配置** ||||
| `alibaba-cloud-loadbalancer-force-override-listeners` | string | false | 强制覆盖监听器 |
| `alibaba-cloud-loadbalancer-preserve-lb-on-delete` | string | false | 删除时保留NLB |
| `alibaba-cloud-loadbalancer-additional-resource-tags` | string | - | 附加标签 |
| `alibaba-cloud-loadbalancer-resource-group-id` | string | - | 资源组ID |

#### 4.4.2 NLB 生产环境配置示例

```yaml
# 生产级 NLB 配置 - 高性能四层服务
apiVersion: v1
kind: Service
metadata:
  name: production-grpc-service
  namespace: production
  annotations:
    # === 实例配置 ===
    # 指定可用区映射 (关键配置,保证多可用区高可用)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-zone-maps: |
      cn-hangzhou-h:vsw-bp1aaaa,cn-hangzhou-i:vsw-bp1bbbb,cn-hangzhou-j:vsw-bp1cccc
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-name: "prod-grpc-nlb"
    
    # IPv4/IPv6 双栈配置 (可选)
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-ip-version: "DualStack"
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-ipv6-address-type: "intranet"
    
    # === 监听器配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-protocol-port: "TCP:50051,TCPSSL:50052"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-idle-timeout: "900"
    # 每秒最大新建连接数
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cps: "100000"
    
    # === TCPSSL 证书配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cert-id: "cert-xxxxxxxx"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-tls-cipher-policy: "tls_cipher_policy_1_2_strict"
    # 双向TLS认证 (mTLS)
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cacert-id: "cacert-xxxxxxx"
    # service.beta.kubernetes.io/alibaba-cloud-loadbalancer-cacert: "on"
    
    # === 健康检查 (精细配置) ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "tcp"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-port: "50051"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-connect-timeout: "5"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "3"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-healthy-threshold: "2"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-unhealthy-threshold: "3"
    
    # === 后端服务器组配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-scheduler: "wrr"
    # 连接排空 (优雅下线)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-connection-drain-timeout: "30"
    # IP型后端 (ENI模式,直通Pod)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-server-group-type: "Ip"
    
    # === 安全配置 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-delete-protection: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-modification-protection: "ConsoleProtection"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-security-group-ids: "sg-bp1xxxxxxxxxx"
    
    # === 资源管理 ===
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-additional-resource-tags: "env=prod,service=grpc,team=platform"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-resource-group-id: "rg-xxxxx"
    
    # === 高级配置 ===
    # 删除Service时保留NLB实例 (用于蓝绿发布等场景)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-preserve-lb-on-delete: "false"
spec:
  type: LoadBalancer
  # 使用 loadBalancerClass 显式指定NLB (K8s 1.24+)
  loadBalancerClass: "alibabacloud.com/nlb"
  externalTrafficPolicy: Local
  selector:
    app: grpc-server
  ports:
  - name: grpc
    port: 50051
    targetPort: 50051
    protocol: TCP
  - name: grpc-tls
    port: 50052
    targetPort: 50051
    protocol: TCP
```

### 4.5 ALB (应用型负载均衡) 配置

> ALB 通过 Ingress Controller 和 AlbConfig CRD 配置,不直接使用 Service LoadBalancer

#### 4.5.1 AlbConfig CRD 示例

```yaml
# AlbConfig - 定义 ALB 实例配置
apiVersion: alibabacloud.com/v1
kind: AlbConfig
metadata:
  name: production-alb
  namespace: kube-system
spec:
  config:
    # ALB 实例名称
    name: "prod-alb"
    # 地址类型
    addressType: Intranet
    # 可用区映射 (至少两个可用区)
    zoneMappings:
    - zoneId: cn-hangzhou-h
      vSwitchId: vsw-bp1aaaa
    - zoneId: cn-hangzhou-i
      vSwitchId: vsw-bp1bbbb
    # 访问日志
    accessLogConfig:
      logStore: alb-access-logs
      logProject: k8s-logs
    # 删除保护
    deletionProtectionEnabled: true
    # 修改保护
    modificationProtectionConfig:
      status: ConsoleProtection
    # 标签
    tags:
    - key: env
      value: prod
---
# 使用 ALB Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: production-ingress
  namespace: production
  annotations:
    # 关联 AlbConfig
    alb.ingress.kubernetes.io/albconfig.order: "production-alb"
    # HTTPS 监听
    alb.ingress.kubernetes.io/listen-ports: '[{"HTTPS": 443}]'
    # SSL 证书
    alb.ingress.kubernetes.io/certificate-id: "cert-xxxxxxxx"
    # 后端协议
    alb.ingress.kubernetes.io/backend-protocol: "HTTP"
    # 健康检查
    alb.ingress.kubernetes.io/healthcheck-enabled: "true"
    alb.ingress.kubernetes.io/healthcheck-path: "/health"
    alb.ingress.kubernetes.io/healthcheck-interval-seconds: "5"
    alb.ingress.kubernetes.io/healthy-threshold-count: "2"
    alb.ingress.kubernetes.io/unhealthy-threshold-count: "3"
spec:
  ingressClassName: alb
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 80
```

### 4.6 阿里云 CCM 部署配置

#### 4.6.1 云配置文件

```yaml
# ConfigMap: cloud-config
apiVersion: v1
kind: ConfigMap
metadata:
  name: cloud-config
  namespace: kube-system
data:
  cloud-config.conf: |
    {
      "Global": {
        "routeTableIDs": "vtb-xxxxxxxx",
        "vpcid": "vpc-xxxxxxxx",
        "vswitchid": "vsw-xxxxxxxx",
        "region": "cn-hangzhou",
        "clusterID": "c-xxxxxxxxxx",
        "zoneID": "cn-hangzhou-h",
        "serviceAccountName": "alibaba-cloud-controller-manager"
      },
      "CloudControllerManager": {
        "cloudConfigPath": "/etc/kubernetes/cloud-config.conf",
        "clusterCIDR": "10.244.0.0/16"
      }
    }
```

#### 4.6.2 RRSA 认证配置 (推荐)

```yaml
# 使用 RRSA (RAM Roles for Service Account) 替代 AK/SK
apiVersion: v1
kind: ServiceAccount
metadata:
  name: alibaba-cloud-controller-manager
  namespace: kube-system
  annotations:
    # 关联 RAM 角色
    pod-identity.alibabacloud.com/role-arn: "acs:ram::123456789012:role/ack-cloud-controller-manager"
```

### 4.7 ReadinessGate 配置 (v2.10.0+)

```yaml
# 启用 ReadinessGate,确保 Pod Ready 前 LB 已配置完成
apiVersion: v1
kind: Service
metadata:
  name: service-with-readiness
  annotations:
    # 自动注入 ReadinessGate (v2.12.4+ 默认启用)
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-readiness-gate: "true"
spec:
  type: LoadBalancer
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
---
# Pod 将自动添加 ReadinessGate
# status.conditions 中会出现:
# - type: cloud.alibabacloud.com/load-balancer-backend-ready
#   status: "True"
```

---

## 5. AWS CCM 配置详解

### 5.1 版本兼容性

| CCM 版本 | K8s 版本 | EKS 版本 | 发布日期 |
|:---|:---|:---|:---|
| v1.32.0 | v1.32.x | 1.32 | 2024-12 |
| v1.31.4 | v1.31.x | 1.31 | 2024-11 |
| v1.30.5 | v1.30.x | 1.30 | 2024-10 |
| v1.29.6 | v1.29.x | 1.29 | 2024-09 |

### 5.2 AWS 负载均衡器类型

| 类型 | Controller | 说明 | 适用场景 |
|:---|:---|:---|:---|
| **CLB** | CCM in-tree | Classic Load Balancer | 遗留应用 |
| **NLB** | CCM | Network Load Balancer | 高性能L4 |
| **ALB** | AWS LB Controller | Application Load Balancer | HTTP/HTTPS |

### 5.3 AWS NLB 完整配置

```yaml
# AWS NLB 生产配置
apiVersion: v1
kind: Service
metadata:
  name: production-nlb
  namespace: production
  annotations:
    # === 负载均衡器类型 ===
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    # 目标类型: instance (NodePort) 或 ip (直通Pod,需要VPC CNI)
    service.beta.kubernetes.io/aws-load-balancer-nlb-target-type: "ip"
    
    # === 网络配置 ===
    service.beta.kubernetes.io/aws-load-balancer-internal: "true"
    service.beta.kubernetes.io/aws-load-balancer-subnets: "subnet-xxx,subnet-yyy,subnet-zzz"
    
    # === IP 地址配置 ===
    # service.beta.kubernetes.io/aws-load-balancer-eip-allocations: "eipalloc-xxx"
    # service.beta.kubernetes.io/aws-load-balancer-private-ipv4-addresses: "10.0.1.100,10.0.2.100"
    
    # === 安全配置 ===
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internal"
    
    # === SSL/TLS ===
    service.beta.kubernetes.io/aws-load-balancer-ssl-cert: "arn:aws:acm:us-west-2:xxx:certificate/xxx"
    service.beta.kubernetes.io/aws-load-balancer-ssl-ports: "443"
    service.beta.kubernetes.io/aws-load-balancer-ssl-negotiation-policy: "ELBSecurityPolicy-TLS13-1-2-2021-06"
    
    # === 健康检查 ===
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-protocol: "HTTP"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-path: "/health"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-port: "traffic-port"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-interval: "10"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-timeout: "5"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-healthy-threshold: "2"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-unhealthy-threshold: "3"
    
    # === 跨区负载均衡 ===
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    
    # === Proxy Protocol ===
    service.beta.kubernetes.io/aws-load-balancer-proxy-protocol: "*"
    
    # === 访问日志 ===
    service.beta.kubernetes.io/aws-load-balancer-access-log-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-name: "my-nlb-logs"
    service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-prefix: "prod-nlb"
    
    # === 标签 ===
    service.beta.kubernetes.io/aws-load-balancer-additional-resource-tags: "Environment=Production,Team=Platform"
spec:
  type: LoadBalancer
  loadBalancerClass: service.k8s.aws/nlb
  externalTrafficPolicy: Local
  selector:
    app: api-server
  ports:
  - name: https
    port: 443
    targetPort: 8443
```

### 5.4 AWS 云配置

```yaml
# AWS cloud-config
apiVersion: v1
kind: ConfigMap
metadata:
  name: cloud-config
  namespace: kube-system
data:
  cloud.conf: |
    [Global]
    Zone=us-west-2a
    VPC=vpc-xxxxxxxxx
    SubnetID=subnet-xxxxxxxxx
    RouteTableID=rtb-xxxxxxxxx
    KubernetesClusterTag=kubernetes.io/cluster/my-cluster
    KubernetesClusterID=my-cluster
    DisableSecurityGroupIngress=false
    ElbSecurityGroup=sg-xxxxxxxxx
    
    [ServiceOverride "ec2"]
    Service=ec2
    Region=us-west-2
    URL=https://ec2.us-west-2.amazonaws.com
    SigningRegion=us-west-2
```

---

## 6. Azure CCM 配置详解

### 6.1 版本兼容性

| CCM 版本 | K8s 版本 | AKS 版本 | 发布日期 |
|:---|:---|:---|:---|
| v1.32.11 | v1.32.x | 1.32 | 2024-11 |
| v1.31.12 | v1.31.x | 1.31 | 2024-11 |
| v1.30.13 | v1.30.x | 1.30 | 2024-11 |
| v1.29.14 | v1.29.x | 1.29 | 2024-10 |

### 6.2 Azure LB 完整配置

```yaml
# Azure Standard LB 生产配置
apiVersion: v1
kind: Service
metadata:
  name: production-lb
  namespace: production
  annotations:
    # === SKU 配置 ===
    service.beta.kubernetes.io/azure-load-balancer-sku: "standard"
    
    # === 内网配置 ===
    service.beta.kubernetes.io/azure-load-balancer-internal: "true"
    service.beta.kubernetes.io/azure-load-balancer-internal-subnet: "internal-subnet"
    
    # === IP 配置 ===
    # service.beta.kubernetes.io/azure-load-balancer-ipv4: "10.0.0.100"
    # service.beta.kubernetes.io/azure-pip-name: "my-pip"
    
    # === DNS ===
    service.beta.kubernetes.io/azure-dns-label-name: "my-service"
    
    # === 健康检查 ===
    service.beta.kubernetes.io/azure-load-balancer-health-probe-protocol: "Http"
    service.beta.kubernetes.io/azure-load-balancer-health-probe-request-path: "/health"
    service.beta.kubernetes.io/azure-load-balancer-health-probe-interval: "10"
    service.beta.kubernetes.io/azure-load-balancer-health-probe-num-of-probe: "2"
    
    # === 后端池配置 ===
    service.beta.kubernetes.io/azure-load-balancer-mode: "auto"
    
    # === 出站规则 ===
    service.beta.kubernetes.io/azure-load-balancer-disable-tcp-reset: "false"
    service.beta.kubernetes.io/azure-load-balancer-enable-high-availability-ports: "true"
    
    # === 资源组 ===
    service.beta.kubernetes.io/azure-load-balancer-resource-group: "mc_myresourcegroup_mycluster_westus2"
    
    # === 标签 ===
    service.beta.kubernetes.io/azure-additional-public-ips: ""
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  selector:
    app: api-server
  ports:
  - name: https
    port: 443
    targetPort: 8443
```

### 6.3 Azure 云配置

```yaml
# azure.json 配置
{
  "cloud": "AzurePublicCloud",
  "tenantId": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "subscriptionId": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "resourceGroup": "my-rg",
  "location": "westus2",
  
  "vnetName": "my-vnet",
  "vnetResourceGroup": "my-vnet-rg",
  "subnetName": "default",
  "securityGroupName": "my-nsg",
  "routeTableName": "my-route-table",
  
  "primaryAvailabilitySetName": "",
  "primaryScaleSetName": "vmss-nodes",
  "vmType": "vmss",
  
  "useManagedIdentityExtension": true,
  "userAssignedIdentityID": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  
  "cloudProviderBackoff": true,
  "cloudProviderBackoffRetries": 6,
  "cloudProviderBackoffExponent": 1.5,
  "cloudProviderBackoffDuration": 5,
  "cloudProviderBackoffJitter": 1,
  
  "cloudProviderRateLimit": true,
  "cloudProviderRateLimitQPS": 6,
  "cloudProviderRateLimitBucket": 10,
  
  "useInstanceMetadata": true,
  "loadBalancerSku": "standard",
  "excludeMasterFromStandardLB": true
}
```

---

## 7. GCP CCM 配置详解

### 7.1 版本兼容性

| CCM 版本 | K8s 版本 | GKE 版本 | 发布日期 |
|:---|:---|:---|:---|
| v32.0.0 | v1.32.x | 1.32 | 2024-12 |
| v31.0.0 | v1.31.x | 1.31 | 2024-08 |
| v30.0.0 | v1.30.x | 1.30 | 2024-05 |

### 7.2 GCP LB 完整配置

```yaml
# GCP Internal TCP LB 配置
apiVersion: v1
kind: Service
metadata:
  name: production-ilb
  namespace: production
  annotations:
    # === 内网负载均衡 ===
    cloud.google.com/load-balancer-type: "Internal"
    
    # === 子网配置 ===
    networking.gke.io/internal-load-balancer-subnet: "gke-subnet"
    
    # === 全局访问 (跨区域) ===
    networking.gke.io/internal-load-balancer-allow-global-access: "true"
    
    # === 共享 VPC ===
    # networking.gke.io/internal-load-balancer-project: "host-project-id"
    
    # === NEG 配置 (直通 Pod) ===
    cloud.google.com/neg: '{"ingress": true, "exposed_ports": {"80":{}}}'
    
    # === 后端配置 ===
    # cloud.google.com/backend-config: '{"default": "my-backend-config"}'
    
    # === 健康检查 ===
    # 通过 BackendConfig CRD 配置
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  selector:
    app: api-server
  ports:
  - name: http
    port: 80
    targetPort: 8080
---
# BackendConfig for health check
apiVersion: cloud.google.com/v1
kind: BackendConfig
metadata:
  name: my-backend-config
  namespace: production
spec:
  healthCheck:
    type: HTTP
    requestPath: /health
    port: 8080
    checkIntervalSec: 10
    timeoutSec: 5
    healthyThreshold: 2
    unhealthyThreshold: 3
```

### 7.3 GCP 云配置

```ini
# gce.conf
[Global]
project-id = my-gcp-project
network-name = my-vpc
network-project-id = host-project-id  # 共享 VPC
subnetwork-name = gke-subnet
node-tags = gke-my-cluster-node
node-instance-prefix = gke-my-cluster
multizone = true
regional = true

[VMSSCFG]
vmss-name = my-vmss
vmss-zone = us-central1-a
```

---

## 8. 生产环境部署 (Production Deployment)

### 8.1 DaemonSet 部署 (推荐)

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: cloud-controller-manager
  namespace: kube-system
  labels:
    k8s-app: cloud-controller-manager
spec:
  selector:
    matchLabels:
      k8s-app: cloud-controller-manager
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
  template:
    metadata:
      labels:
        k8s-app: cloud-controller-manager
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "10258"
        prometheus.io/path: "/metrics"
    spec:
      # 仅运行在控制平面节点
      nodeSelector:
        node-role.kubernetes.io/control-plane: ""
      tolerations:
      # 允许调度到未初始化节点
      - key: node.cloudprovider.kubernetes.io/uninitialized
        value: "true"
        effect: NoSchedule
      # 允许调度到控制平面节点
      - key: node-role.kubernetes.io/control-plane
        effect: NoSchedule
      - key: node-role.kubernetes.io/master
        effect: NoSchedule
      # 允许调度到 NotReady 节点
      - key: node.kubernetes.io/not-ready
        effect: NoSchedule
      
      serviceAccountName: cloud-controller-manager
      priorityClassName: system-cluster-critical
      hostNetwork: true  # 使用主机网络,确保云API可达
      
      securityContext:
        runAsNonRoot: true
        runAsUser: 65534
        seccompProfile:
          type: RuntimeDefault
      
      containers:
      - name: cloud-controller-manager
        image: registry.k8s.io/cloud-controller-manager/cloud-controller-manager:v1.30.0
        imagePullPolicy: IfNotPresent
        command:
        - /cloud-controller-manager
        args:
        # 云提供商配置
        - --cloud-provider=$(CLOUD_PROVIDER)
        - --cloud-config=/etc/kubernetes/cloud.conf
        
        # API Server 连接
        - --kubeconfig=/etc/kubernetes/cloud-controller-manager.conf
        - --authentication-kubeconfig=/etc/kubernetes/cloud-controller-manager.conf
        - --authorization-kubeconfig=/etc/kubernetes/cloud-controller-manager.conf
        
        # Leader 选举
        - --leader-elect=true
        - --leader-elect-lease-duration=15s
        - --leader-elect-renew-deadline=10s
        - --leader-elect-retry-period=2s
        - --leader-elect-resource-name=cloud-controller-manager
        - --leader-elect-resource-namespace=kube-system
        
        # 控制器配置
        - --controllers=*
        - --configure-cloud-routes=true
        - --allocate-node-cidrs=true
        - --cluster-cidr=10.244.0.0/16
        - --cluster-name=$(CLUSTER_NAME)
        
        # 并发配置
        - --concurrent-service-syncs=5
        - --node-monitor-period=5s
        - --route-reconciliation-period=10s
        
        # 安全配置
        - --use-service-account-credentials=true
        - --bind-address=0.0.0.0
        - --secure-port=10258
        - --tls-cert-file=/etc/kubernetes/pki/cloud-controller-manager.crt
        - --tls-private-key-file=/etc/kubernetes/pki/cloud-controller-manager.key
        
        # 日志配置
        - --v=2
        - --logging-format=json
        
        env:
        - name: CLOUD_PROVIDER
          value: "alicloud"  # aws/azure/gce/alicloud
        - name: CLUSTER_NAME
          valueFrom:
            configMapKeyRef:
              name: cluster-info
              key: cluster-name
        
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 1000m
            memory: 512Mi
        
        livenessProbe:
          httpGet:
            path: /healthz
            port: 10258
            scheme: HTTPS
          initialDelaySeconds: 15
          periodSeconds: 10
          timeoutSeconds: 15
          successThreshold: 1
          failureThreshold: 3
        
        readinessProbe:
          httpGet:
            path: /healthz
            port: 10258
            scheme: HTTPS
          initialDelaySeconds: 5
          periodSeconds: 5
          timeoutSeconds: 5
        
        volumeMounts:
        - name: cloud-config
          mountPath: /etc/kubernetes/cloud.conf
          subPath: cloud.conf
          readOnly: true
        - name: kubeconfig
          mountPath: /etc/kubernetes/cloud-controller-manager.conf
          subPath: cloud-controller-manager.conf
          readOnly: true
        - name: pki
          mountPath: /etc/kubernetes/pki
          readOnly: true
        # 云提供商凭证 (如果不使用 IRSA/RRSA)
        # - name: cloud-credentials
        #   mountPath: /etc/kubernetes/cloud-credentials
        #   readOnly: true
        
        ports:
        - name: https
          containerPort: 10258
          protocol: TCP
      
      volumes:
      - name: cloud-config
        configMap:
          name: cloud-config
      - name: kubeconfig
        secret:
          secretName: cloud-controller-manager-kubeconfig
      - name: pki
        hostPath:
          path: /etc/kubernetes/pki
          type: DirectoryOrCreate
      # - name: cloud-credentials
      #   secret:
      #     secretName: cloud-credentials
```

### 8.2 RBAC 配置

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: cloud-controller-manager
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: system:cloud-controller-manager
rules:
# Node 管理
- apiGroups: [""]
  resources: ["nodes"]
  verbs: ["get", "list", "watch", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["nodes/status"]
  verbs: ["patch", "update"]

# Service 管理
- apiGroups: [""]
  resources: ["services"]
  verbs: ["get", "list", "watch", "patch", "update"]
- apiGroups: [""]
  resources: ["services/status"]
  verbs: ["patch", "update"]

# ServiceAccount Token 创建 (用于控制器身份)
- apiGroups: [""]
  resources: ["serviceaccounts/token"]
  verbs: ["create"]
- apiGroups: [""]
  resources: ["serviceaccounts"]
  verbs: ["create", "get"]

# Endpoints 管理
- apiGroups: [""]
  resources: ["endpoints"]
  verbs: ["create", "get", "list", "watch", "update"]
- apiGroups: ["discovery.k8s.io"]
  resources: ["endpointslices"]
  verbs: ["get", "list", "watch"]

# Events
- apiGroups: [""]
  resources: ["events"]
  verbs: ["create", "patch", "update"]

# Configmaps (Leader 选举 - 旧版本)
- apiGroups: [""]
  resources: ["configmaps"]
  verbs: ["get", "create", "update"]

# Leases (Leader 选举 - 推荐)
- apiGroups: ["coordination.k8s.io"]
  resources: ["leases"]
  verbs: ["get", "create", "update"]

# Secrets (云凭证访问)
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get", "list", "watch"]

# PersistentVolumes (CSI 迁移相关)
- apiGroups: [""]
  resources: ["persistentvolumes"]
  verbs: ["get", "list", "watch", "update"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: system:cloud-controller-manager
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:cloud-controller-manager
subjects:
- kind: ServiceAccount
  name: cloud-controller-manager
  namespace: kube-system
---
# RoleBinding for leader election in kube-system namespace
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: system:cloud-controller-manager:extension-apiserver-authentication-reader
  namespace: kube-system
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: extension-apiserver-authentication-reader
subjects:
- kind: ServiceAccount
  name: cloud-controller-manager
  namespace: kube-system
```

### 8.3 KCM 和 Kubelet 配置调整

```bash
# kube-controller-manager 配置
# 禁用已移至 CCM 的控制器
--cloud-provider=external
--controllers=*,-cloud-node-lifecycle,-route,-service

# kubelet 配置
# 使用外部云提供商
--cloud-provider=external
# 节点将带有 node.cloudprovider.kubernetes.io/uninitialized taint
# 等待 CCM 初始化后移除
```

---

## 9. 监控与可观测性 (Monitoring & Observability)

### 9.1 关键指标表

| 指标名称 | 类型 | 说明 | 告警阈值建议 |
|:---|:---|:---|:---|
| **可用性指标** ||||
| `up{job="cloud-controller-manager"}` | Gauge | CCM 进程存活 | == 0 告警 |
| `leader_election_master_status` | Gauge | Leader 状态 | sum == 0 告警 |
| **性能指标** ||||
| `cloudprovider_*_api_request_duration_seconds` | Histogram | 云API请求延迟 | P99 > 10s 告警 |
| `cloudprovider_*_api_request_errors_total` | Counter | 云API错误数 | rate > 0.1 告警 |
| `workqueue_depth{name="service"}` | Gauge | Service队列深度 | > 100 告警 |
| `workqueue_depth{name="node"}` | Gauge | Node队列深度 | > 50 告警 |
| `workqueue_retries_total` | Counter | 重试次数 | rate > 1 告警 |
| `workqueue_longest_running_processor_seconds` | Gauge | 最长处理时间 | > 300s 告警 |
| **业务指标** ||||
| `node_collector_zone_health` | Gauge | Zone 健康状态 | < 1 告警 |
| `node_collector_zone_size` | Gauge | Zone 节点数量 | 监控趋势 |
| `service_controller_sync_total` | Counter | Service 同步次数 | 监控趋势 |

### 9.2 Prometheus 告警规则

```yaml
groups:
- name: cloud-controller-manager
  rules:
  # CCM 进程不可用
  - alert: CloudControllerManagerDown
    expr: absent(up{job="cloud-controller-manager"} == 1)
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "cloud-controller-manager is down"
      description: "CCM has been down for more than 5 minutes"
      runbook_url: "https://runbooks.example.com/ccm-down"
  
  # 无 Leader
  - alert: CloudControllerManagerNoLeader
    expr: sum(leader_election_master_status{job="cloud-controller-manager"}) == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "cloud-controller-manager has no leader"
      description: "No CCM instance is currently the leader"
  
  # 云 API 错误率高
  - alert: CloudAPIErrorRateHigh
    expr: |
      sum(rate(cloudprovider_aws_api_request_errors_total[5m])) 
      / sum(rate(cloudprovider_aws_api_request_duration_seconds_count[5m])) > 0.1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Cloud API error rate is high (> 10%)"
      description: "Cloud provider API is experiencing elevated error rates"
  
  # 云 API 延迟高
  - alert: CloudAPILatencyHigh
    expr: |
      histogram_quantile(0.99, 
        sum(rate(cloudprovider_aws_api_request_duration_seconds_bucket[5m])) by (le, request)
      ) > 10
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Cloud API P99 latency > 10s"
      description: "Cloud provider API latency is degraded"
  
  # LoadBalancer 同步失败
  - alert: LoadBalancerSyncFailed
    expr: increase(workqueue_retries_total{name="service",job="cloud-controller-manager"}[1h]) > 20
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "LoadBalancer sync is failing repeatedly"
      description: "Service controller has retried {{ $value }} times in the last hour"
  
  # 工作队列积压
  - alert: CCMWorkQueueBacklog
    expr: workqueue_depth{job="cloud-controller-manager"} > 100
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "CCM work queue has backlog"
      description: "Queue {{ $labels.name }} has {{ $value }} items pending"
  
  # 节点初始化超时
  - alert: NodeInitializationTimeout
    expr: |
      count(kube_node_spec_taint{key="node.cloudprovider.kubernetes.io/uninitialized"} == 1) > 0
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Nodes pending CCM initialization"
      description: "{{ $value }} nodes have been waiting for CCM initialization for > 10 minutes"
  
  # Zone 健康状态
  - alert: CloudZoneUnhealthy
    expr: node_collector_zone_health < 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Cloud zone is unhealthy"
      description: "Zone {{ $labels.zone }} health is degraded"
```

### 9.3 Grafana Dashboard 配置

```json
{
  "title": "Cloud Controller Manager Overview",
  "panels": [
    {
      "title": "CCM Availability",
      "type": "stat",
      "targets": [
        {
          "expr": "sum(up{job=\"cloud-controller-manager\"})",
          "legendFormat": "Running Instances"
        }
      ]
    },
    {
      "title": "Leader Status",
      "type": "stat",
      "targets": [
        {
          "expr": "sum(leader_election_master_status{job=\"cloud-controller-manager\"})",
          "legendFormat": "Active Leaders"
        }
      ]
    },
    {
      "title": "Cloud API Request Rate",
      "type": "graph",
      "targets": [
        {
          "expr": "sum(rate(cloudprovider_*_api_request_duration_seconds_count[5m])) by (request)",
          "legendFormat": "{{ request }}"
        }
      ]
    },
    {
      "title": "Cloud API Error Rate",
      "type": "graph",
      "targets": [
        {
          "expr": "sum(rate(cloudprovider_*_api_request_errors_total[5m])) by (request)",
          "legendFormat": "{{ request }}"
        }
      ]
    },
    {
      "title": "Cloud API Latency (P99)",
      "type": "graph",
      "targets": [
        {
          "expr": "histogram_quantile(0.99, sum(rate(cloudprovider_*_api_request_duration_seconds_bucket[5m])) by (le, request))",
          "legendFormat": "{{ request }}"
        }
      ]
    },
    {
      "title": "Work Queue Depth",
      "type": "graph",
      "targets": [
        {
          "expr": "workqueue_depth{job=\"cloud-controller-manager\"}",
          "legendFormat": "{{ name }}"
        }
      ]
    },
    {
      "title": "Nodes by Zone",
      "type": "graph",
      "targets": [
        {
          "expr": "node_collector_zone_size",
          "legendFormat": "{{ zone }}"
        }
      ]
    }
  ]
}
```

---

## 10. 故障排查 (Troubleshooting)

### 10.1 常见问题诊断表

| 症状 | 可能原因 | 诊断方法 | 解决方案 |
|:---|:---|:---|:---|
| **节点未初始化** | CCM未运行/认证失败/云API超时 | 检查节点Taint和CCM日志 | 确保CCM运行且云凭证有效 |
| **LB未创建** | 云API失败/配额不足/权限缺失 | `kubectl describe svc` 检查Events | 检查云API权限和配额 |
| **LB IP未分配** | 创建中/EIP不足/网络配置错误 | 检查云控制台LB状态 | 等待或检查网络配置 |
| **节点地址未更新** | 云实例信息不同步/元数据服务故障 | 检查Node status | 重启CCM或检查元数据服务 |
| **路由未创建** | 路由控制器未启用/VPC路由表满 | 检查云路由表 | 启用路由控制器或清理路由 |
| **Provider ID为空** | 初始化失败/云API错误 | `kubectl get node -o yaml` | 检查CCM日志和云API |
| **LB后端不同步** | 节点标签不匹配/CCM延迟 | 检查LB后端组 | 检查节点标签和CCM状态 |
| **健康检查失败** | 配置错误/端口不通 | 云控制台检查探测日志 | 修正健康检查配置 |

### 10.2 诊断命令

```bash
# ========== CCM 状态检查 ==========

# 检查 CCM Pod 状态
kubectl get pods -n kube-system -l k8s-app=cloud-controller-manager -o wide

# 检查 CCM 日志
kubectl logs -n kube-system -l k8s-app=cloud-controller-manager -f --tail=100

# 检查 CCM 日志 (过滤错误)
kubectl logs -n kube-system -l k8s-app=cloud-controller-manager | grep -i "error\|failed\|warning"

# 检查 Leader 状态
kubectl get lease -n kube-system cloud-controller-manager -o yaml

# ========== 节点状态检查 ==========

# 检查节点初始化状态
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,\
PROVIDER:.spec.providerID,\
INTERNAL-IP:.status.addresses[?(@.type==\"InternalIP\")].address,\
TAINTS:.spec.taints[*].key

# 检查未初始化的节点
kubectl get nodes -o json | jq -r '
  .items[] | 
  select(.spec.taints[]? | .key == "node.cloudprovider.kubernetes.io/uninitialized") | 
  .metadata.name'

# 检查节点标签
kubectl get nodes --show-labels | grep -E "topology.kubernetes.io|node.kubernetes.io/instance-type"

# ========== Service/LoadBalancer 检查 ==========

# 检查 LoadBalancer Service 状态
kubectl get svc -A -o wide | grep LoadBalancer

# 检查 Service 详情
kubectl describe svc <service-name> -n <namespace>

# 检查 Service Events
kubectl get events --field-selector involvedObject.kind=Service -A --sort-by='.lastTimestamp'

# 检查 Service 的 Endpoints
kubectl get endpoints <service-name> -n <namespace> -o yaml

# ========== CCM 指标检查 ==========

# 访问 CCM 指标 (需要 port-forward)
kubectl port-forward -n kube-system svc/cloud-controller-manager 10258:10258 &
curl -k https://localhost:10258/metrics | grep -E "cloudprovider|workqueue"

# ========== 云API相关检查 ==========

# 检查云凭证 (阿里云)
kubectl get secret -n kube-system cloud-config -o yaml

# 检查 IRSA/RRSA 配置
kubectl get sa -n kube-system cloud-controller-manager -o yaml
```

### 10.3 常见日志模式

```bash
# ========== 正常日志 ==========

# Leader 获取成功
I0121 10:00:00.000000   1 leaderelection.go:248] successfully acquired lease kube-system/cloud-controller-manager

# 节点初始化成功
I0121 10:00:01.000000   1 node_controller.go:232] Initializing node node-1 with cloud provider
I0121 10:00:02.000000   1 node_controller.go:245] Successfully initialized node node-1

# LoadBalancer 创建成功
I0121 10:00:03.000000   1 service_controller.go:456] Ensuring load balancer for service default/my-service
I0121 10:00:05.000000   1 service_controller.go:502] Successfully created load balancer lb-xxxxxxxx

# ========== 警告日志 ==========

# 节点不可达
W0121 10:00:00.000000   1 node_controller.go:340] Node node-1 is unreachable, adding taint

# 重试
W0121 10:00:00.000000   1 reflector.go:324] failed to list *v1.Service: Get https://...: dial tcp: i/o timeout

# ========== 错误日志 ==========

# LoadBalancer 同步失败
E0121 10:00:00.000000   1 service_controller.go:567] Error syncing load balancer: failed to ensure load balancer: API error

# 节点初始化失败
E0121 10:00:00.000000   1 node_controller.go:234] Failed to get instance info: instance not found

# 云API认证失败
E0121 10:00:00.000000   1 cloud.go:123] Failed to call cloud API: InvalidAccessKeyId.NotFound

# 路由创建失败
E0121 10:00:00.000000   1 route_controller.go:189] Failed to create route: quota exceeded

# ========== 诊断关键词 ==========

# 常见错误关键词
grep -E "error|failed|Error|Failed|timeout|Timeout|refused|denied|quota|limit" <ccm-log>

# 云API相关
grep -E "api|API|request|response|status" <ccm-log>

# 特定资源
grep -E "node|Node|service|Service|loadbalancer|LoadBalancer|route|Route" <ccm-log>
```

### 10.4 问题修复流程

```
节点未初始化问题排查流程:
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│  1. 检查节点 Taint                                                          │
│     kubectl get nodes -o json | jq '.items[].spec.taints'                   │
│     │                                                                        │
│     └─ 有 node.cloudprovider.kubernetes.io/uninitialized=true:NoSchedule?   │
│        │                                                                     │
│        ├─ 是 → 继续步骤 2                                                   │
│        └─ 否 → 节点已初始化,问题在其他地方                                   │
│                                                                              │
│  2. 检查 CCM 状态                                                           │
│     kubectl get pods -n kube-system -l k8s-app=cloud-controller-manager     │
│     │                                                                        │
│     └─ Pod Running?                                                          │
│        │                                                                     │
│        ├─ 否 → 检查 Pod 状态: kubectl describe pod <ccm-pod>                │
│        │       常见原因: 镜像拉取失败, 资源不足, 节点亲和性                  │
│        │                                                                     │
│        └─ 是 → 继续步骤 3                                                   │
│                                                                              │
│  3. 检查 CCM 日志                                                           │
│     kubectl logs -n kube-system <ccm-pod> | grep -i "error\|fail"           │
│     │                                                                        │
│     └─ 常见错误:                                                             │
│        ├─ "InvalidAccessKeyId" → 检查云凭证                                 │
│        ├─ "instance not found" → 检查实例是否存在                           │
│        ├─ "no leader" → 检查 Leader 选举                                    │
│        └─ "timeout" → 检查网络/云API可达性                                  │
│                                                                              │
│  4. 检查 Leader 选举                                                        │
│     kubectl get lease -n kube-system cloud-controller-manager -o yaml       │
│     │                                                                        │
│     └─ 确认 holderIdentity 和 renewTime 正常                                │
│                                                                              │
│  5. 检查云API可达性                                                         │
│     kubectl exec -n kube-system <ccm-pod> -- curl <cloud-metadata-endpoint> │
│     │                                                                        │
│     └─ 如果失败,检查网络策略和防火墙                                         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 11. 生产环境 Checklist

### 11.1 部署前检查

| 检查项 | 验证方法 | 状态 |
|:---|:---|:---:|
| **基础设施** |||
| 云凭证配置正确 | 验证云API访问 | [ ] |
| 云API权限充足 | 检查IAM/RAM策略 | [ ] |
| 网络可达 (云元数据服务) | curl metadata endpoint | [ ] |
| VPC/子网配置正确 | 云控制台验证 | [ ] |
| **CCM配置** |||
| CCM镜像版本与K8s兼容 | 检查版本矩阵 | [ ] |
| RBAC权限配置 | kubectl auth can-i | [ ] |
| 云配置文件正确 | 验证配置语法 | [ ] |
| Leader选举启用 | 检查启动参数 | [ ] |
| **其他组件配置** |||
| KCM已禁用云控制器 | 检查--cloud-provider=external | [ ] |
| Kubelet已配置external | 检查--cloud-provider=external | [ ] |
| kube-proxy配置正确 | 检查配置 | [ ] |

### 11.2 部署后验证

| 验证项 | 验证方法 | 预期结果 | 状态 |
|:---|:---|:---|:---:|
| CCM Pod运行 | `kubectl get pods -n kube-system` | Running | [ ] |
| Leader选举成功 | `kubectl get lease` | 有活跃holder | [ ] |
| 节点初始化完成 | 检查节点Taint | 无uninitialized | [ ] |
| Provider ID设置 | `kubectl get nodes -o yaml` | 非空 | [ ] |
| 节点标签正确 | 检查topology标签 | region/zone正确 | [ ] |
| LoadBalancer创建 | 创建测试Service | IP分配成功 | [ ] |
| 健康检查正常 | 云控制台检查 | 后端健康 | [ ] |
| 路由创建(如启用) | 云VPC路由表 | 路由存在 | [ ] |
| 指标暴露 | curl /metrics | 指标可访问 | [ ] |

### 11.3 安全最佳实践

| 安全项 | 说明 | 实施状态 |
|:---|:---|:---:|
| **凭证管理** |||
| 使用IRSA/RRSA/Workload Identity | 避免长期AK/SK | [ ] |
| 凭证定期轮换 | 自动化轮换机制 | [ ] |
| 最小权限原则 | 仅授予必要的云API权限 | [ ] |
| **网络安全** |||
| 限制CCM网络出口 | NetworkPolicy/安全组 | [ ] |
| 使用内网端点 | 云API走内网 | [ ] |
| **审计与监控** |||
| 启用云API审计日志 | CloudTrail/ActionTrail | [ ] |
| CCM日志持久化 | 日志收集到中央存储 | [ ] |
| 监控告警配置 | Prometheus + AlertManager | [ ] |
| **运行时安全** |||
| 非root用户运行 | securityContext配置 | [ ] |
| 只读文件系统(如可能) | readOnlyRootFilesystem | [ ] |
| 资源限制 | resources.limits配置 | [ ] |

### 11.4 运维检查项

| 运维项 | 频率 | 检查内容 |
|:---|:---|:---|
| CCM健康状态 | 实时监控 | Pod状态、指标 |
| 云API错误率 | 每5分钟 | error rate < 1% |
| 云API延迟 | 每5分钟 | P99 < 10s |
| Leader状态 | 每分钟 | 有活跃Leader |
| 节点初始化队列 | 每分钟 | 队列深度 < 10 |
| 云资源配额 | 每天 | LB/EIP配额充足 |
| 凭证过期时间 | 每天 | 证书/token未过期 |
| 版本更新 | 每月 | 检查新版本和CVE |

---

## 12. 版本兼容性矩阵

### 12.1 Kubernetes 版本与 CCM 版本对应

| K8s 版本 | 阿里云 CCM | AWS CCM | Azure CCM | GCP CCM |
|:---|:---|:---|:---|:---|
| v1.32 | v2.12.x | v1.32.x | v1.32.x | v32.x.x |
| v1.31 | v2.12.x | v1.31.x | v1.31.x | v31.x.x |
| v1.30 | v2.11.x | v1.30.x | v1.30.x | v30.x.x |
| v1.29 | v2.10.x | v1.29.x | v1.29.x | v29.x.x |
| v1.28 | v2.9.x | v1.28.x | v1.28.x | v28.x.x |

### 12.2 功能可用性矩阵

| 功能 | 阿里云 | AWS | Azure | GCP |
|:---|:---:|:---:|:---:|:---:|
| **LoadBalancer** |||||
| L4 TCP/UDP | CLB/NLB | NLB | Standard LB | TCP LB |
| L7 HTTP/HTTPS | ALB(Ingress) | ALB(AWSLBC) | AppGW(Ingress) | HTTP LB |
| 内网LB | Yes | Yes | Yes | Yes |
| 双栈IPv6 | Yes | Yes | Yes | Yes |
| IP型后端 | NLB | NLB | Standard LB | NEG |
| **Routes** |||||
| VPC路由管理 | Yes | Yes | Yes | Yes |
| **节点管理** |||||
| InstancesV2 | Yes | Yes | Yes | Yes |
| 拓扑标签 | Yes | Yes | Yes | Yes |
| Provider ID | Yes | Yes | Yes | Yes |
| **高级功能** |||||
| ReadinessGate | v2.10.0+ | AWSLBC | N/A | NEG |
| 连接排空 | NLB | NLB | Yes | Backend Service |
| Proxy Protocol | NLB | NLB | N/A | N/A |

### 12.3 升级注意事项

| 版本变化 | 影响 | 迁移步骤 |
|:---|:---|:---|
| **阿里云 v2.11 → v2.12** | ReadinessGate默认开启 | 确认应用兼容 |
| **阿里云 v2.10 → v2.11** | CLB默认PayByCLCU | 检查计费配置 |
| **AWS v1.30+ deprecation** | CLB支持废弃 | 迁移到NLB |
| **Azure v1.30+ standard** | 默认Standard SKU | 更新配置 |

---

## 参考资源

| 资源 | 链接 |
|:---|:---|
| Kubernetes CCM 概念 | https://kubernetes.io/docs/concepts/architecture/cloud-controller/ |
| 阿里云 CCM | https://github.com/kubernetes/cloud-provider-alibaba-cloud |
| 阿里云 ACK 文档 | https://www.alibabacloud.com/help/en/ack |
| AWS CCM | https://github.com/kubernetes/cloud-provider-aws |
| Azure CCM | https://github.com/kubernetes-sigs/cloud-provider-azure |
| GCP CCM | https://github.com/kubernetes/cloud-provider-gcp |

---

> **文档版本**: v2.0 | **维护者**: Platform Team | **审核日期**: 2026-01
