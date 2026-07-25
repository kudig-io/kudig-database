---
title: Cluster API (CAPI) 深度解析
summary: 解析 Kubernetes 原生集群生命周期管理项目 Cluster API 的 CRD 模型、Provider 协作与生产实践。
category: 平台工程
tags:
- cluster-api
- capi
- lifecycle
- multi-cluster
- provider
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 平台架构师
- SRE
estimated_read_time: 25min
intent_queries:
- Cluster API 是什么
- CAPI 如何管理集群生命周期
- Cluster API 与 vcluster 区别
- MachineDeployment 如何工作
trigger_keywords:
- Cluster API
- CAPI
- 集群生命周期
- Machine
- Provider
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Cluster API (CAPI) 深度解析

> **适用版本**: Cluster API v1.7+ / ClusterClass (v1beta1)  
> **Kubernetes 版本**: v1.28 - v1.33  
> **难度**: 高级  
> **最后更新**: 2026-07-23

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、概述](#一概述)
- [二、与其它多集群方案对比](#二与其它多集群方案对比)
- [三、CAPI 架构与 CRD 模型](#三capi-架构与-crd-模型)
- [四、Provider 协作工作机制](#四provider-协作工作机制)
- [五、集群创建与删除生命周期](#五集群创建与删除生命周期)
- [六、MachineDeployment 滚动升级](#六machinedeployment-滚动升级)
- [七、ClusterClass 声明式集群模板](#七clusterclass-声明式集群模板)
- [八、与 GitOps 集成](#八与-gitops-集成)
- [九、生产实践](#九生产实践)
- [十、排障](#十排障)
- [十一、相关文档](#十一相关文档)

---

<!-- chunk: 一、概述 -->
## 一、概述

**Cluster API（CAPI）** 是 Kubernetes SIG Cluster Lifecycle 的核心子项目，它使用 **声明式 CRD + Kubernetes 风格的 controller** 来创建、升级、销毁 Kubernetes 集群本身——把"集群"当作 Kubernetes 资源来管理，即业界常说的 **"用 Kubernetes 管理 Kubernetes（Kubernetes managing Kubernetes）"** 范式。

### 1.1 解决了什么问题

传统集群供给是命令式、粘滞于具体云厂商的：`eksctl create cluster`、`gcloud container clusters create`、`terraform apply`，每条路径彼此割裂，升级与销毁缺乏统一模型。当集群数量从个位数增长到数十、数百乃至上千时，问题集中爆发：

- **供给路径割裂**：AWS / Azure / GCP / 私有数据中心各写一套 IaC，重复且易漂移；
- **生命周期无统一状态机**：创建易、升级难、销毁更难，往往残留云资源造成成本泄漏；
- **GitOps 难以覆盖集群层**：应用层早已 GitOps 化，集群层仍靠手工/脚本；
- **多云缺乏可移植抽象**：换云即重写整套运维工具链。

CAPI 的核心价值在于提供一套**与云无关的集群生命周期 API**：无论底层是 AWS、Azure、GCP、vSphere 还是裸金属，上层都用同一组 CRD（`Cluster` / `Machine` / `MachineDeployment`）表达，由各 Provider 翻译成具体云动作。

### 1.2 两个关键角色

| 角色 | 说明 |
|:---|:---|
| **管理集群（Management Cluster）** | 运行 CAPI core 与各 Provider controller 的 Kubernetes 集群，是"控制集群的集群"。通常独立、高可用、严格备份。 |
| **工作负载集群（Workload Cluster）** | 由管理集群创建并纳管的业务集群，其 API Server、节点 VM 均作为 CR 的"产物"存在于管理集群中。 |

> **关键边界**：管理集群是**单点**。它一旦丢失，工作负载集群仍会运行（CAPI 不接管数据面），但你会失去对它们的声明式控制面——因此管理集群的 etcd 备份与高可用是 CAPI 生产化的第一要务（见 [九、生产实践](#九生产实践)）。

### 1.3 设计哲学

CAPI 遵循三条核心原则：

1. **声明式（Declarative）**：用户描述期望状态（"我要一个 3 控制面 + 5 worker 的 v1.32 集群"），controller 持续 reconcile 实际状态向期望收敛；
2. **可组合（Composable）**：核心 API 只定义 `Cluster` / `Machine` 等抽象，具体"怎么建 VM"、"怎么 bootstrap 节点"、"怎么管控制平面"由三类 Provider 各自实现，自由拼装；
3. **云无关（Cloud-agnostic）**：核心 API 不含任何云特定字段，云细节通过 `infrastructureRef` 委托给对应 InfraProvider。

---

<!-- chunk: 二、与其它多集群方案对比 -->
## 二、与其它多集群方案对比

在多集群领域，CNCF 生态涌现了大量项目，它们的**层级**和**职责**并不相同。理解这一点是选型的基础：CAPI 管的是"集群本身的生命周期"，与应用分发（GitOps）、虚拟集群、联邦调度**正交**，可以并通常是叠加使用的。

### 2.1 核心对比表

| 方案 | 定位 | 抽象层级 | 集群形态 | 典型用途 |
|:---|:---|:---|:---|:---|
| **Cluster API (CAPI)** | 集群生命周期（创建/升级/销毁） | 集群本身（infra） | 真实集群 | 多云多区域集群供给、统一升级 |
| **vcluster** | 虚拟集群（Namespace 内跑轻量控制平面） | 集群（但软隔离） | 虚拟（共享宿主节点） | 多租户、隔离、CI、临时环境 |
| **Fleet / Argo CD ApplicationSet** | 应用分发到多集群 | 应用层（workload） | 不创建集群 | 多集群应用 GitOps |
| **KubeFed**（已归档） | 跨集群资源联邦调度 | 资源层（分发 CR） | 不创建集群 | 已废弃，被 Karmada 等取代 |
| **Karmada** | 跨集群调度与资源分发 | 资源/调度层 | 不创建集群 | 大规模多集群工作负载调度 |
| **Rancher** | 集群管理 UI + 下游 agent | 全栈（含供给+UI） | 真实/导入 | 企业多集群管理（含 RKE2 供给） |
| **Cluster API Provider (CAPA/CAPZ/...)** | CAPI 的具体云实现 | CAPI 的 Infra 层 | 真实集群 | CAPI 在某云上的具体执行 |

### 2.2 按问题域分层理解

一个常见误区是把 CAPI 与 vcluster / Rancher 当成"二选一"的竞品。实际上它们解决的是**不同层次的问题**，正确的认知是分层叠加：

```
┌──────────────────────────────────────────────────────────┐
│  应用分发层（Application Distribution）                   │
│  Argo CD ApplicationSet / Fleet / Flux                   │
│  → "把哪些应用同步到哪些集群"（不创建集群）                │
├──────────────────────────────────────────────────────────┤
│  跨集群调度层（Multi-cluster Scheduling）                 │
│  Karmada / KubeFed(已弃)                                  │
│  → "一个 Deployment 跨集群拆分副本"                        │
├──────────────────────────────────────────────────────────┤
│  集群生命周期层（Cluster Lifecycle）        ★ CAPI 在此   │
│  Cluster API                                              │
│  → "集群本身怎么创建/升级/销毁"                            │
├──────────────────────────────────────────────────────────┤
│  虚拟化/多租户层（Tenancy）                               │
│  vcluster                                                  │
│  → "在一个集群里跑出多个轻量控制平面"                      │
├──────────────────────────────────────────────────────────┤
│  管理平面/UI 层（Management Plane）                       │
│  Rancher / ACM / Anthos                                   │
│  → "统一 UI + RBAC + 应用商店，可含供给能力"               │
└──────────────────────────────────────────────────────────┘
```

### 2.3 三个易混场景的选型

| 场景 | 推荐 | 理由 |
|:---|:---|:---|
| 为开发团队按需供给隔离的真实集群（多区域、多账号） | **CAPI** | 声明式、GitOps 友好、多云一致 |
| 数百个租户需要轻量、秒级、低成本的"集群感" | **vcluster** | 共享宿主节点，不重复跑 etcd/APIServer |
| 同一个应用部署到已存在的多个集群 | **Argo CD ApplicationSet / Fleet** | 应用层 GitOps，CAPI 管不到这里 |
| 想要 GUI + RBAC + 集群供给一站式 | **Rancher**（其 RKE2 集群可由 CAPI 供给） | 全栈，但较重 |

> **叠加是常态**：生产架构常见组合是 **CAPI 供给集群 → Argo CD 同步集群与节点 addon → Karmada/Fleet 做应用分发**。详见 [八、与 GitOps 集成](#八与-gitops-集成)。

---

<!-- chunk: 三、CAPI 架构与 CRD 模型 -->
## 三、CAPI 架构与 CRD 模型

CAPI 由**核心 API**（`cluster.x-k8s.io`）与**三类 Provider**组成。核心 API 只定义抽象，具体动作委托给 Provider，二者通过 `Ref` 字段解耦。

### 3.1 整体架构图

```
                       ┌─────────────────────────────────────────────┐
                       │       管理集群 (Management Cluster)          │
                       │                                             │
                       │  ┌───────────────────────────────────────┐  │
                       │  │  CAPI Core Controllers                │  │
                       │  │  (cluster.x-k8s.io)                   │  │
                       │  │  Cluster / Machine / MachineSet       │  │
                       │  │  MachineDeployment / MachineHealthCheck│ │
                       │  └───────────────┬───────────────────────┘  │
                       │                  │ reconcile                │
                       │     ┌────────────┼────────────┐             │
                       │     ▼            ▼            ▼             │
                       │ ┌───────┐  ┌──────────┐  ┌────────────┐    │
                       │ │Boot-  │  │Control   │  │Infra       │    │
                       │ │strap  │  │Plane     │  │Provider    │    │
                       │ │Prov.  │  │Provider  │  │(CAPA/CAPZ/ │    │
                       │ │(CABPK)│  │(KCP)     │  │ CAPG/CAPV) │    │
                       │ └───┬───┘  └────┬─────┘  └─────┬──────┘    │
                       │     │          │              │            │
                       └─────┼──────────┼──────────────┼────────────┘
                             │          │              │
            cloud-init/      │   etcd/   │   创建 VM/VPC/LB
            kubeadm join     │   apiserver│  (cloud API calls)
                 数据         │  生命周期  │
                 ▼            ▼           ▼
              ┌─────────────────────────────────────────┐
              │   工作负载集群 (Workload Cluster)         │
              │   ┌─────────────────────────────────┐   │
              │   │ Control Plane (etcd+apiserver+  │   │
              │   │  controller-manager+scheduler)  │   │
              │   └────────────┬────────────────────┘   │
              │                │                        │
              │   ┌────────────┴────────────────────┐   │
              │   │ Worker Nodes (kubelet+kube-proxy)│  │
              │   └─────────────────────────────────┘   │
              └─────────────────────────────────────────┘
```

### 3.2 核心 CRD（`cluster.x-k8s.io`）

| CRD | 类比 | 职责 |
|:---|:---|:---|
| **Cluster** | Namespace（集群级） | 集群的"容器"，持有 controlPlaneRef 与 infrastructureRef 两个引用，是生命周期根对象 |
| **Machine** | Pod | 一台节点（控制面或 worker）。每个 Machine 对应一台真实 VM/物理机 |
| **MachineSet** | ReplicaSet | 维持指定数量的 Machine 副本（一般不直接用） |
| **MachineDeployment** | Deployment | 管 MachineSet 的滚动升级，是 worker 节点的管理入口 |
| **MachineHealthCheck** | PodDisruptionBudget / HPA | 检测 Machine 不健康时自动触发 remediation（重建） |
| **ClusterClass** | PodTemplate（集群级） | CAPI 1.0+ 的集群模板，统一字段，配合 `Cluster.spec.topology` 使用 |

#### Cluster 对象示例（裸 CR 方式）

```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: prod-us-east-1
  namespace: clusters
spec:
  clusterNetwork:
    pods:
      cidrBlocks: ["10.244.0.0/16"]
    services:
      cidrBlocks: ["10.96.0.0/12"]
    serviceDomain: "cluster.local"
  controlPlaneEndpoint:     # 集群 API Server 入口（由 InfraProvider 填充 LB）
    host: ""
    port: 6443
  controlPlaneRef:          # 委托控制面管理
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: prod-us-east-1-cp
  infrastructureRef:        # 委托基础设施管理
    apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
    kind: AWSCluster
    name: prod-us-east-1-infra
```

> 这两个 `Ref` 是 CAPI 可组合性的核心：换云只改 `infrastructureRef.kind`（`AWSCluster` → `GCPCluster`），换控制面实现只改 `controlPlaneRef.kind`（`KubeadmControlPlane` → `KamajiControlPlane`）。

#### MachineDeployment 示例

```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: MachineDeployment
metadata:
  name: prod-us-east-1-worker
  namespace: clusters
spec:
  clusterName: prod-us-east-1
  replicas: 5
  selector:
    matchLabels:
      cluster.x-k8s.io/cluster-name: prod-us-east-1
  template:
    spec:
      clusterName: prod-us-east-1
      version: "v1.32.4"          # K8s 版本，升级改这里
      bootstrap:                   # 委托 bootstrap
        configRef:
          apiVersion: bootstrap.cluster.x-k8s.io/v1beta1
          kind: KubeadmConfigTemplate
          name: prod-us-east-1-worker-bootstrap
      infrastructureRef:           # 委托 infra（每个 Machine 一份）
        apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
        kind: AWSMachineTemplate
        name: prod-us-east-1-worker-mt
```

### 3.3 三类 Provider 详解

CAPI 的能力由三类 Provider 拼装而成，每类都有独立的 CRD 与 controller。

#### (1) Bootstrap Provider —— 生成节点引导数据

**职责**：为 Machine 生成首次启动所需的 bootstrap 数据（通常是 cloud-init 配置），包含 kubeadm join token、kubelet 配置、CA 证书等。

| 实现 | 说明 |
|:---|:---|
| **CABPK**（Kubeadm Bootstrap Provider） | 官方默认实现，生成 `kubeadm init/join` 的 cloud-init，CRD 为 `KubeadmConfig` / `KubeadmConfigTemplate` |
| Talos Bootstrap | 生成 Talos Linux 的 machine config |
| MicroK8s / K3s Bootstrap | 针对轻量发行版的 bootstrap |

```yaml
apiVersion: bootstrap.cluster.x-k8s.io/v1beta1
kind: KubeadmConfigTemplate
metadata:
  name: prod-us-east-1-worker-bootstrap
spec:
  template:
    spec:
      joinConfiguration:
        nodeRegistration:
          kubeletExtraArgs:
            cloud-provider: aws
            register-with-taints: "node-role.kubernetes.io/worker=:NoSchedule"
      files: []
      preKubeadmCommands: []
```

#### (2) Control Plane Provider —— 管理控制平面

**职责**：创建并维护控制平面（etcd / kube-apiserver / controller-manager / scheduler），处理控制面的滚动升级与 etcd 备份。

| 实现 | 说明 |
|:---|:---|
| **KCP**（KubeadmControlPlane） | 官方默认，用 kubeadm 在静态 VM 上堆叠式部署 etcd+apiserver，CRD 为 `KubeadmControlPlane` |
| **Kamaji** | 提供"托管控制平面（Hosted Control Plane）"，etcd 集中部署，多集群共享 |
| **etcdadm** | 用 etcdadm 单独管理 etcd 成员 |

```yaml
apiVersion: controlplane.cluster.x-k8s.io/v1beta1
kind: KubeadmControlPlane
metadata:
  name: prod-us-east-1-cp
spec:
  replicas: 3                       # 控制面副本数（建议奇数）
  version: "v1.32.4"                # 控制面 K8s 版本
  machineTemplate:
    infrastructureRef:
      apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
      kind: AWSMachineTemplate
      name: prod-us-east-1-cp-mt
  kubeadmConfigSpec:
    initConfiguration:
      nodeRegistration:
        kubeletExtraArgs: {cloud-provider: aws}
    clusterConfiguration:
      apiServer:
        extraArgs: {profile: "true"}
  rollback: true                    # 升级失败自动回滚（v1.7+）
```

#### (3) Infrastructure Provider —— 供给基础设施

**职责**：实际调用云/裸金属 API 创建计算、网络、负载均衡。这是 CAPI 的"手脚"。

| 实现 | 云/平台 | CRD（Cluster 级 / Machine 级） |
|:---|:---|:---|
| **CAPA** | AWS | `AWSCluster` / `AWSMachine` / `AWSMachineTemplate` |
| **CAPZ** | Azure | `AzureCluster` / `AzureMachine` |
| **CAPG** | GCP | `GCPCluster` / `GCPMachine` |
| **CAPV** | vSphere | `VSphereCluster` / `VSphereMachine` |
| **CAPMVM** | 裸金属（microvm / Flintlock） | `MicrovmCluster` / `MicrovmMachine` |
| **CAPD** | Docker（仅测试/演示） | `DockerCluster` / `DockerMachine` |
| **CAPMVM/CAPH** | Bare metal（Tinkerbell） | `TinkerbellCluster` |

```yaml
apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
kind: AWSCluster
metadata:
  name: prod-us-east-1-infra
  namespace: clusters
spec:
  region: us-east-1
  sshKeyName: platform-prod
  network:
    vpc:
      id: vpc-0abc123              # 留空则自动新建 VPC
  controlPlaneLoadBalancer:
    loadBalancerType: nlb           # API Server 入口 LB
```

> **Provider 注册表**：完整列表见 https://cluster-api-aws.sigs.k8s.io 等 SIG 子项目。每个 Provider 有独立版本与兼容矩阵（见 [9.2](#92-provider-版本兼容矩阵)）。

---

<!-- chunk: 四、Provider 协作工作机制 -->
## 四、Provider 协作工作机制

理解 CAPI 的关键，是理解三类 Provider 如何**通过 status 字段接力**协作。它们之间没有直接调用，而是通过观察彼此写入 `status` 的字段来触发自己的 reconcile。

### 4.1 数据流：bootstrap 数据如何流动

一个 Machine 的 bootstrap 数据流经三个对象，是一条单向接力链：

```
   Machine (core)
       │ 1. controller 创建 Machine，置 Ready=false
       │ 2. 委托 Bootstrap：status.bootstrapReady=false
       ▼
   KubeadmConfig (bootstrap)
       │ 3. CABPK 生成 cloud-init（含 join token）
       │ 4. 写回 .status.ready=true, .status.dataReady=true
       ▼
   InfraMachine (infra, e.g. AWSMachine)
       │ 5. CAPA 看到 bootstrap.dataReady=true，把 cloud-init 注入 user-data
       │ 6. 调 EC2 RunInstances 起 VM
       │ 7. VM 首次启动 → cloud-init 跑 kubeadm join
       │ 8. kubelet 注册成功 → Node 出现
       ▼
   Machine (core) status.ready=true
       9. NodeProvider 标记 Machine Ready，ControlPlane/MD 完成计数
```

### 4.2 完整集群创建时序图

```
用户 apply Cluster CR
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│ T1  Cluster Controller                                      │
│     创建 InfraCluster (AWSCluster)                          │
│     创建 ControlPlane (KubeadmControlPlane)                 │
└─────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│ T2  InfraProvider (CAPA)                                    │
│     reconcile AWSCluster →                                  │
│     创建 VPC / 子网 / 安全组 / 路由表 / NLB                  │
│     填充 AWSCluster.status.ready=true                       │
│     回填 Cluster.spec.controlPlaneEndpoint.host = NLB DNS   │
└─────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│ T3  ControlPlane Provider (KCP)                             │
│     reconcile KubeadmControlPlane →                         │
│     为第 1 台控制面创建 Machine + KubeadmConfig + AWSMachine│
│     bootstrap 生成 kubeadm init cloud-init                  │
└─────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│ T4  Bootstrap (CABPK) + Infra (CAPA)                        │
│     CABPK 写 KubeadmConfig.status.dataReady=true            │
│     CAPA 起 EC2，注入 cloud-init（含 join token）            │
│     VM 启动 → cloud-init 跑 kubeadm init                    │
│     etcd / apiserver 启动 → 控制面就绪                       │
└─────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│ T5  KCP 重复 T3-T4，按序创建第 2、3 台控制面（串行 etcd join）│
│     全部 Ready 后 KubeadmControlPlane.status.ready=true     │
│     Cluster.status.controlPlaneReady=true                   │
└─────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│ T6  用户/CD 创建 MachineDeployment（worker）                │
│     MachineDeployment Controller 创建 MachineSet → Machines │
│     每个 worker Machine 走与 T4 相同的 bootstrap+infra 流程  │
│     但 cloud-init 是 kubeadm join（控制面已存在）            │
└─────────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│ T7  CAPI Core                                               │
│     所有 Machine Ready → Cluster.status.ready=true          │
│     集群可供使用                                              │
└─────────────────────────────────────────────────────────────┘
```

### 4.3 关键协调点（Reconcile Triggers）

| 触发 | 观察者 | 动作 |
|:---|:---|:---|
| `Cluster` 创建 | Cluster Controller | 创建 Infra/ControlPlane 子对象 |
| `InfraCluster.status.ready=true` | Cluster Controller | 把 LB 地址写回 Cluster.controlPlaneEndpoint |
| `ControlPlane.status.ready=true` | Cluster Controller | 置 `controlPlaneReady`，允许 MD 创建 worker |
| `Machine.spec.bootstrap.dataReady=true` | InfraProvider | 起 VM 并注入 cloud-init |
| `InfraMachine.status.ready=true` | Node/Machine Controller | 标记 Machine Ready |
| `MachineHealthCheck` 不健康 | MachineDeployment/KCP | 触发 remediation 重建 Machine |

> **设计要点**：Provider 之间**没有同步 RPC 调用**，全部通过 watch etcd 中彼此的 status 异步推进。这使得整个系统天然契合 Kubernetes 的最终一致性模型，任何一步失败都会在下次 reconcile 重试。

---

<!-- chunk: 五、集群创建与删除生命周期 -->
## 五、集群创建与删除生命周期

### 5.1 Cluster 状态机

```
        ┌──────────┐  apply CR
        │ (none)   │ ─────────► ┌───────────┐
        └──────────┘            │  Pending  │
                                └─────┬─────┘
                                      │ InfraProvider 开始建云资源
                                      ▼
                                ┌───────────┐
                                │Provisioning│
                                └─────┬─────┘
                                      │ InfraCluster.ready + 控制面就绪
                                      ▼
                                ┌───────────┐
                                │ Provisioned│ ──►  ┌──────┐
                                └─────┬─────┘       │ Ready│ ◄── 稳态
                                      │             └──────┘
                                      │ delete Cluster
                                      ▼
                                ┌───────────┐
                                │ Deleting  │  ◄── finalizer 阻挡
                                └─────┬─────┘      级联删除所有子对象
                                      │ 全部清理完
                                      ▼
                                ┌───────────┐
                                │  Deleted  │  CR 被移除
                                └───────────┘
```

### 5.2 创建阶段（Pending → Ready）

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get cluster -n clusters prod-us-east-1 -o yaml | \
  grep -E "phase|controlPlaneReady|infrastructureReady"
```

关键字段在 `Cluster.status`：
- `infrastructureReady`：InfraProvider 已建好 VPC/LB；
- `controlPlaneReady`：KCP 报告控制面 3 台就绪；
- `phase`：粗粒度阶段（`Pending` / `Provisioned` / `Deleting` / `Deleted`）。

> ⚠️ `phase` 是**派生字段，反映过去**，不能作为唯一判据；应看 `Ready` condition 与各子对象 status。

### 5.3 删除阶段（Deleting → Deleted）—— 级联与 finalizer

删除 `Cluster` 是 CAPI 最容易出错也最关键的环节。它依赖 **ownerReferences + finalizer** 实现严格级联：

1. 用户 `kubectl delete cluster prod-us-east-1`；
2. Cluster 持有 finalizer `cluster.x-k8s.io`，CR 不会立即消失，进入 `Terminating`；
3. Cluster Controller 先删除 MachineDeployment → MachineSet → Machine（worker 先撤）；
4. 再删 ControlPlane（KCP）→ 删除控制面 Machine；
5. 最后删 InfraCluster（AWSCluster）→ **InfraProvider 此时才真正销毁云资源（EC2/VPC/LB）**；
6. 所有子对象消失后，finalizer 移除，Cluster CR 被回收。

> 🔴 **级联失败 = 云资源泄漏**：若 InfraProvider 凭证失效或云配额问题导致 `AWSCluster` 删不掉，finalizer 永不移除，Cluster 卡在 `Terminating`，云上 EC2/VPC 持续计费。排障见 [十、排障](#十排障)。

```bash
# 🔴 高风险：触发云资源销毁，不可逆
kubectl delete cluster prod-us-east-1 -n clusters
```

### 5.4 安全删除前检查清单

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认集群内无残留工作负载（在 workload cluster 上下文）
kubectl --context=prod-us-east-1 get ns
kubectl --context=prod-us-east-1 get pods -A

# 2. 确认该集群未被 ApplicationSet/ClusterGroup 引用
kubectl get appproj,appset -A -o yaml | grep prod-us-east-1

# 3. 确认管理集群中该 Cluster 的子对象规模
clusterctl describe cluster prod-us-east-1 -n clusters
```

---

<!-- chunk: 六、MachineDeployment 滚动升级 -->
## 六、MachineDeployment 滚动升级

CAPI 的升级模型与 K8s 的 Deployment 高度同构，但分**控制面升级（KCP）**和**worker 升级（MachineDeployment）**两条独立路径，且**必须先升控制面、再升 worker**。

### 6.1 升级总流程

```
   当前: 控制面 v1.31.6  +  worker v1.31.6
                │
                ▼  改 KCP.spec.version = v1.32.4
   ┌─────────────────────────────────────────┐
   │ KCP 滚动升级（一次一台，串行 etcd join） │
   │ cp-0 v1.31.6 → cp-0 v1.32.4 (etcd 迁移) │
   │ cp-1 v1.31.6 → cp-1 v1.32.4             │
   │ cp-2 v1.31.6 → cp-2 v1.32.4             │
   └─────────────────────────────────────────┘
                │ 控制面全部 v1.32.4
                ▼  改 MachineDeployment.spec.template.spec.version = v1.32.4
   ┌─────────────────────────────────────────┐
   │ MachineDeployment 滚动升级（可并行）     │
   │ 新建 MachineSet(v1.32.4) → 扩；旧 MS 缩  │
   │ 按 maxSurge/maxUnavailable 控制          │
   └─────────────────────────────────────────┘
                ▼
   最终: 控制面 v1.32.4  +  worker v1.32.4
```

### 6.2 控制面升级（KCP rolling out）

KCP 的升级比 worker 谨慎得多，因为 etcd 是有状态的：

- **一次只升一台**：逐台替换，确保 etcd quorum（多数派）不丢；
- **先 join 新成员、再移除旧成员**：保持 3 副本始终可用；
- **preflight check**（v1.7+）：升级前检查 etcd 健康、磁盘空间、组件版本兼容；
- **自动回滚**：`spec.rollback=true` 时，若新控制面无法在超时内就绪，KCP 自动回退。

```bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 触发控制面升级（改 KCP version 字段）
kubectl patch kubeadmcontrolplane prod-us-east-1-cp -n clusters \
  --type=merge -p '{"spec":{"version":"v1.32.4"}}'

# 🟢 视察升级进度
kubectl get kubeadmcontrolplane prod-us-east-1-cp -n clusters \
  -o jsonpath='{.status.version}{"\n"}{.status.readyReplicas}{"\n"}'
```

升级期间观察 KCP conditions：
- `Ready`：当前是否满足期望副本；
- `MachinesReady`：所有 Machine 是否就绪；
- `EtcdClusterHealthy`：etcd 集群健康（升级中关键指标）；
- `ControlPlaneComponentsHealthy`：apiserver 等组件健康。

### 6.3 Worker 升级（MachineDeployment rolling out）

Worker 升级与 K8s Deployment 完全同构，参数也一致：

```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: MachineDeployment
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1           # 最多多建几台（同 Deployment）
      maxUnavailable: 0     # 滚动时不允许 fewer than replicas
  template:
    spec:
      version: "v1.32.4"    # 改这里即触发滚动
```

```bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 触发 worker 滚动升级
kubectl patch machinedeployment prod-us-east-1-worker -n clusters \
  --type=merge -p '{"spec":{"template":{"spec":{"version":"v1.32.4"}}}}'

# 🟢 观察新旧 MachineSet 副本变化
kubectl get machineset -n clusters -l cluster.x-k8s.io/cluster-name=prod-us-east-1
```

### 6.4 暂停与恢复滚动

```bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 暂停 MachineDeployment（停止 reconcile，便于排障）
kubectl patch machinedeployment prod-us-east-1-worker -n clusters \
  -p '{"spec":{"paused":true}}'

# 恢复
kubectl patch machinedeployment prod-us-east-1-worker -n clusters \
  -p '{"spec":{"paused":false}}' --type=merge
```

> ⚠️ **暂停不会停止已运行的滚动**，只是冻结当前状态。要"中止"升级需手动把 version 改回旧值并删除新 MachineSet。

### 6.5 MachineHealthCheck 自动修复

MachineHealthCheck 监测 Machine 对应 Node 的 condition，不健康时触发重建（remediation）：

```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: MachineHealthCheck
metadata:
  name: prod-us-east-1-worker-mhc
  namespace: clusters
spec:
  clusterName: prod-us-east-1
  maxUnhealthy: 40%            # 超过 40% 不健康则停止修复（防雪崩）
  nodeStartupTimeout: 10m      # VM 启动超时
  selector:
    matchLabels:
      cluster.x-k8s.io/deployment-name: prod-us-east-1-worker
  unhealthyConditions:
  - type: Ready
    status: Unknown
    timeout: 300s
  - type: Ready
    status: "False"
    timeout: 300s
```

---

<!-- chunk: 七、ClusterClass 声明式集群模板 -->
## 七、ClusterClass 声明式集群模板

**ClusterClass** 是 CAPI 1.0+ 引入的关键能力，它把"一份完整的集群定义"抽象成可复用的**模板**，避免每个 Cluster 都重复声明 KCP/MD/Infra 一堆 CR。

### 7.1 为什么需要 ClusterClass

不用 ClusterClass 时，创建一个集群要 apply **5~7 个 CR**（Cluster / AWSCluster / KubeadmControlPlane / AWSMachineTemplate / KubeadmConfigTemplate / MachineDeployment / ...），且每个集群都要复制粘贴改名字，维护成本高、易漂移。

ClusterClass 把这些模板打包成一份，每个真实 Cluster 只需声明"用哪个 ClusterClass + 拓扑变量"即可。

### 7.2 ClusterClass 定义（示例骨架）

```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: ClusterClass
metadata:
  name: aws-prod-standard
  namespace: clusters
spec:
  controlPlane:
    ref:
      apiVersion: controlplane.cluster.x-k8s.io/v1beta1
      kind: KubeadmControlPlaneTemplate
      name: aws-prod-cp-template
    machineInfrastructure:
      ref:
        apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
        kind: AWSMachineTemplate
        name: aws-prod-cp-mt
  infrastructure:
    ref:
      apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
      kind: AWSClusterTemplate
      name: aws-prod-infra-template
  workers:
    machineDeployments:
    - class: default-worker
      template:
        bootstrap:
          ref:
            apiVersion: bootstrap.cluster.x-k8s.io/v1beta1
            kind: KubeadmConfigTemplate
            name: aws-prod-worker-bootstrap
        infrastructure:
          ref:
            apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
            kind: AWSMachineTemplate
            name: aws-prod-worker-mt
  variables:
  - name: region
    required: true
    schema:
      openAPIV3Schema:
        type: string
  - name: kubernetesVersion
    required: true
    schema:
      openAPIV3Schema:
        type: string
  patches:
  - name: region-patch
    definitions:
    - selector:
        apiVersion: infrastructure.cluster.x-k8s.io/v1beta1
        kind: AWSClusterTemplate
        matchResources:
          infrastructureCluster: true
      jsonPatches:
      - op: add
        path: /spec/template/spec/region
        valueFrom:
          variable: region
```

### 7.3 用 ClusterClass 创建集群（极简）

```yaml
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: prod-eu-west-1
  namespace: clusters
spec:
  topology:
    class: aws-prod-standard          # 引用 ClusterClass
    version: v1.32.4
    variables:
    - name: region
      value: eu-west-1
    workers:
      machineDeployments:
      - class: default-worker
        replicas: 5
    controlPlane:
      replicas: 3
```

只需十几行即可创建一个完整集群。升级时改 `topology.version`，CAPI 自动协调 KCP 与 MD。

```bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 用 ClusterClass 升级（改 version 即触发 KCP+MD 全量滚动）
kubectl patch cluster prod-eu-west-1 -n clusters --type=merge \
  -p '{"spec":{"topology":{"version":"v1.33.1"}}}'
```

### 7.4 ClusterClass 价值小结

| 维度 | 不用 ClusterClass | 用 ClusterClass |
|:---|:---|:---|
| 创建集群的 CR 数 | 5~7 个 | 1 个（仅 Cluster） |
| 版本升级 | 分别改 KCP + MD | 改 `topology.version` 一处 |
| 一致性 | 易漂移 | 模板强制对齐 |
| 变量差异化 | 手改每个 CR | `variables` + `patches` 注入 |

---

<!-- chunk: 八、与 GitOps 集成 -->
## 八、与 GitOps 集成

CAPI 是声明式 CRD，天然适合 GitOps。典型模式是用 Argo CD / Flux 把 `Cluster` / `MachineDeployment` / `ClusterClass` 等 CR 同步到管理集群，由 CAPI controller 协调执行。

### 8.1 GitOps 集成架构

```
   ┌──────────────┐   push    ┌──────────────────┐
   │  Git 仓库    │ ────────► │ Argo CD (管理集群)│
   │ clusters/    │           │  ApplicationSet   │
   │  ├── prod/   │           └────────┬──────────┘
   │  ├── staging/│                    │ apply CR
   │  └── dev/    │                    ▼
   └──────────────┘           ┌──────────────────────┐
                              │ 管理集群 (CAPI)       │
                              │ Cluster/Machine CR    │
                              │ cluster.x-k8s.io      │
                              └────────┬─────────────┘
                                       │ reconcile
                                       ▼
                              ┌──────────────────────┐
                              │ InfraProvider (CAPA)  │
                              │ → 调云 API 建 EC2/VPC  │
                              └──────────────────────┘
```

### 8.2 Argo CD ApplicationSet 多集群供给示例

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: capi-clusters
  namespace: argocd
spec:
  generators:
  - git:
      repoURL: https://github.com/org/capi-fleet
      revision: main
      directories:
      - path: clusters/*
  template:
    metadata:
      name: '{{path.basename}}'
    spec:
      project: cluster-lifecycle
      source:
        repoURL: https://github.com/org/capi-fleet
        targetRevision: main
        path: '{{path}}'
      destination:
        server: https://kubernetes.default.svc   # 部署到管理集群自身
        namespace: clusters
      syncPolicy:
        automated:
          prune: true       # Git 删除 CR → CAPI 删除集群（注意！）
          selfHeal: true
        syncOptions:
        - CreateNamespace=true
        - ServerSideApply=true
```

> 🔴 **prune + CAPI = 危险组合**：`prune: true` 会在 Git 删除 Cluster CR 时触发**真实云资源销毁**。生产环境强烈建议对 `Cluster` 资源关闭 prune，或用 Argo CD `IgnoreDifferences` / 资源钩子保护。

### 8.3 Flux 集成示例

```yaml
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: capi-fleet
  namespace: flux-system
spec:
  url: https://github.com/org/capi-fleet
  ref:
    branch: main
---
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: clusters
  namespace: flux-system
spec:
  sourceRef:
    kind: GitRepository
    name: capi-fleet
  path: ./clusters
  prune: false                # 关键：保护 Cluster 不被误删
  wait: true
  interval: 5m
```

### 8.4 clusterctl move（GitOps 迁移管理权）

当工作负载集群需要"自立门户"（例如让管理集群自身也成为一个被 CAPI 管理的对象，或迁移管理集群）时，使用 `clusterctl move` 把对象的 owner 关系从源管理集群搬到目标管理集群：

```bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 把 src-mgmt 中的对象 ownership 移到 dst-mgmt
clusterctl move \
  --from-kubeconfig=src-mgmt.kubeconfig \
  --to-kubeconfig=dst-mgmt.kubeconfig \
  --namespace=clusters
```

> `move` 不重建云资源，只是改 CR 的 ownerReferences，使新管理集群接管 reconcile。常用于管理集群升级/灾备演练。

---

<!-- chunk: 九、生产实践 -->
## 九、生产实践

### 9.1 管理集群高可用与备份（CAPI 的单点防线）

管理集群是整个 fleet 的"控制之控制"，它的丢失不致命（工作负载集群继续运行），但会让所有声明式能力失效。三条铁律：

| 铁律 | 实践 |
|:---|:---|
| **etcd 多副本** | 管理集群控制面至少 3 副本 etcd，跨 AZ 部署 |
| **etcd 定期备份** | 用 Velero / etcdctl snapshot 定期备份；管理集群是 CAPI 单点，etcd 备份 = fleet 备份 |
| **etcd 异地副本** | 关键 fleet 考虑 etcd 跨 region 副本或定期把 snapshot 推到对象存储 |

```bash
# 🟢 低风险：只读/信息收集，通常无副作用（snapshot 本身只读，不改动 etcd）
# etcd snapshot 备份
ETCDCTL_API=3 etcdctl snapshot save /backup/mgmt-$(date +%F).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/etcd/pki/ca.crt \
  --cert=/etc/etcd/pki/peer.crt \
  --key=/etc/etcd/pki/peer.key

# 推到对象存储（示例）
aws s3 cp /backup/mgmt-$(date +%F).db s3://capi-etcd-backup/
```

```bash
# 🔴 高风险：恢复 etcd 会覆盖管理集群全部状态，仅在灾备场景执行
ETCDCTL_API=3 etcdctl snapshot restore /backup/mgmt-2026-07-23.db \
  --data-dir=/var/lib/etcd-restored
```

### 9.2 Provider 版本兼容矩阵

CAPI core 与各 Provider 是**独立版本**的，组合前必须查兼容矩阵（每版 CAPI 有 supported Provider 版本范围）。升级时务必**先升 CAPI core，再逐个升 Provider**，并查阅 release notes 的 contract。

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前管理集群已安装的 Provider 与版本
clusterctl init --list
# 或
kubectl get providers -A
```

经验法则：
- CAPI minor 升级（1.7→1.8）通常支持前一个 minor 的 Provider；
- Provider 落后 CAPI 一个 minor 通常可用，但放弃新特性；
- 永远不要让 Provider 领先 CAPI core。

### 9.3 与 Karpenter 的关系

CAPI 与 Karpenter 是**不同层级**的弹性供给，可叠加：

| 工具 | 管什么 | 触发源 | 颗粒度 |
|:---|:---|:---|:---|
| **CAPI** | 集群本身（建/升/删整集群） | 用户/GitOps | 集群级 |
| **Karpenter** | 集群**内**的节点（按 Pod 需求建 EC2） | 调度器/Pod pending | 节点级 |

典型叠加：CAPI 供给一个"空"集群 → Karpenter 接管该集群内的节点弹性扩缩。即 **CAPI 决定"集群存在"，Karpenter 决定"集群内节点数"**。详见 [[10-平台工程/02-运维/99-karpenter-node-autoscaling-guide|Karpenter 节点弹性]]。

> ⚠️ 二者**不要在同一集群同时管 worker 节点生命周期**：若 MachineDeployment 与 Karpenter NodePool 都在建节点，会重复供给。约定：CAPI 只建控制面 + 最小 worker，其余 worker 交给 Karpenter。

### 9.4 clusterctl 工具集

`clusterctl` 是 CAPI 的瑞士军刀，覆盖初始化、描述、升级、迁移四大场景：

```bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 初始化管理集群（安装 CAPI core + 指定 Provider）
clusterctl init \
  --infrastructure aws \
  --control-plane kubeadm \
  --bootstrap kubeadm

# 🟢 低风险：只读/信息收集，通常无副作用
# 树状拓扑：展示 Cluster 下所有 Machine 与状态
clusterctl describe cluster prod-us-east-1 -n clusters

# 🟢 低风险：只读/信息收集，通常无副作用
# 查看已安装 Provider
clusterctl init --list

# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 升级 CAPI core / Provider 到兼容版本
clusterctl upgrade apply --contract v1beta1

# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 生成一个新集群的 CR 清单（不直接 apply）
clusterctl generate cluster dev-1 \
  --kubernetes-version=v1.32.4 \
  --control-plane-machine-count=3 \
  --worker-machine-count=3 \
  --infrastructure=aws > dev-1.yaml
```

### 9.5 日常只读观察命令

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出所有集群
kubectl get cluster -A

# 列出所有 Machine 及其版本/状态
kubectl get machine -A \
  -o custom-columns=NAME:.metadata.name,CLUSTER:.spec.clusterName,\
VERSION:.spec.version,READY:.status.ready,STATE:.status.phase

# 列出 MachineDeployment 及其副本/版本
kubectl get machinedeployment -A

# 查看 ClusterClass
kubectl get clusterclass -A
```

### 9.6 生产清单速查

| 维度 | 建议 |
|:---|:---|
| 管理集群 | 控制面 ≥3 副本、跨 AZ、etcd 定期备份 |
| Provider 版本 | 升级前查兼容矩阵，core 先升 |
| ClusterClass | 强烈建议启用，统一模板降低漂移 |
| GitOps | 同步 CR，但 Cluster 资源关 prune |
| 凭证管理 | 用 ExternalSecret / IRSA，不硬编码 cloud creds |
| 网络 | 规划好每集群 Pod/Service CIDR，避免冲突 |
| 监控 | 监控 Machine Provisioning 卡住、InfraProvider 错误率 |
| 与 Karpenter | 划清边界，不同时管 worker |

---

<!-- chunk: 十、排障 -->
## 十、排障

CAPI 排障的核心思路：**自顶向下**——从 Cluster status 下钻到 ControlPlane/MD，再到 Machine，最后到 InfraProvider 的具体云错误。

### 10.1 排障总流程

```
kubectl get cluster (status.phase / conditions)
        │ 不健康
        ▼
kubectl describe cluster (看哪个 Ready=false)
        │ controlPlaneReady=false → 看 KCP
        │ infrastructureReady=false → 看 InfraCluster
        ▼
kubectl describe <子对象> (KCP / MachineDeployment / AWSCluster)
        │ 看 conditions / events
        ▼
kubectl describe machine <name> (bootstrapReady? infraReady?)
        ▼
kubectl logs <provider controller> -n capi-system / capi-provider-aws
        ▼
云控制台核对（EC2/VPC/凭证）
```

### 10.2 关键诊断命令

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. Cluster 整体状态
kubectl describe cluster prod-us-east-1 -n clusters | \
  grep -A20 "Conditions:"

# 2. 树状拓扑（最直观）
clusterctl describe cluster prod-us-east-1 -n clusters

# 3. 单台 Machine 状态
kubectl describe machine prod-us-east-1-worker-xxxxx -n clusters

# 4. 看 KCP 升级进度与 etcd 健康
kubectl get kubeadmcontrolplane -n clusters -o yaml | \
  grep -A15 "conditions:"

# 5. 看 InfraProvider 是否报云错误
kubectl get awscluster -n clusters -o yaml | grep -A5 "conditions:"
kubectl get awsmachine -n clusters -o yaml | grep -A5 "conditions:"

# 6. Provider controller 日志
kubectl logs deploy/capa-controller -n capa-system --tail=200 | grep -i error
kubectl logs deploy/capi-controller-manager -n capi-system --tail=200
kubectl logs deploy/cabpk-controller-manager -n cabpk-system --tail=200
kubectl logs deploy/kubeadm-control-plane -n capi-kubeadm-control-plane-system --tail=200
```

### 10.3 常见问题与对策

#### 问题 1：Machine 卡在 Provisioning

**现象**：`kubectl get machine` 显示 `phase=Provisioning`，长时间不变，`status.ready=false`。

**排查**：
```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe machine <name> -n clusters
# 重点看：
# - status.bootstrapReady   (false → bootstrap 没生成 cloud-init)
# - status.infrastructureReady (false → InfraProvider 没建 VM)
# - events 段
```

**常见根因**：
| 现象 | 根因 | 对策 |
|:---|:---|:---|
| bootstrapReady=false | CABPK 没收到 Machine / 配置错误 | 看 cabpk controller 日志 |
| infrastructureReady=false | InfraProvider 凭证错 / 配额满 / AMI 找不到 | 看 capa controller 日志 + 云控制台 |
| 都 ready 但 Node 不出现 | VM 起了但 cloud-init 失败 / 网络不通 | 看 VM console / cloud-init 日志 |

#### 问题 2：Cluster 卡在 Terminating（云资源泄漏风险）

**现象**：`kubectl delete cluster` 后 CR 长期 `Terminating`。

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 找出哪个 finalizer 卡住
kubectl get cluster <name> -n clusters -o jsonpath='{.metadata.finalizers}'
# 找出哪个子对象没删干净
kubectl get awscluster,kubeadmcontrolplane,machine -n clusters \
  -l cluster.x-k8s.io/cluster-name=<name>
```

**根因**：通常是 InfraProvider 删云资源失败（凭证失效、依赖资源如 LB/NAT 没清）。

> 🔴 **慎用强制移除 finalizer**：直接 `kubectl patch ... -p '{"metadata":{"finalizers":[]}}'` 会让 CR 消失，但**云上资源会残留持续计费**。正确做法是先在云控制台人工清理依赖，再让 controller 自然完成。

```bash
# 🔴 高风险：可能造成云资源残留与持续计费，仅当确认云资源已人工清理后使用
# 仅作最后手段，强制移除 finalizer
kubectl patch awsmachine <name> -n clusters --type=merge \
  -p '{"metadata":{"finalizers":[]}}'
```

#### 问题 3：InfraProvider 凭证错误

**现象**：`AWSCluster.status.conditions` 中 `Ready=False`，message 含 `UnauthorizedOperation` / `AccessDenied`。

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Provider 使用的凭证（IRSA / secret）
kubectl get secret -n capa-system
kubectl get pod -n capa-system -o yaml | grep -A5 AWS_
```

**对策**：核对 IRSA role 的 trust policy、IAM policy 权限范围（CAPA 需 EC2/IAM/ELB 大量权限）。

#### 问题 4：bootstrap cloud-init 失败

**现象**：Machine 的 VM 已起但 Node 始终不出现。

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 KubeadmConfig 生成的 cloud-init
kubectl get kubeadmconfig <name> -n clusters -o yaml

# 在云控制台/串口日志看 VM 上 cloud-init 输出
# 或在 VM 内：
# sudo journalctl -u cloud-final --no-pager | tail -100
```

**常见根因**：CA 证书过期、kubeadm 配置语法错、所需镜像拉不下来（私有镜像仓库鉴权）。

#### 问题 5：KCP 升级卡住

**现象**：控制面升级中途停止，某台 Machine `Provisioning` 不前。

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe kubeadmcontrolplane <name> -n clusters
# 重点看 conditions:
# - EtcdClusterHealthy (etcd quorum 是否还在)
# - CertificatesAvailable
# - MachinesReady
```

**根因**：etcd 成员未正确 join（quorum 风险）、新版本 kubelet 镜像拉取失败、preflight check 失败。`spec.rollback=true` 可让 KCP 自动回退到旧版本。

### 10.4 排障速查表

| 症状 | 第一检查点 | 第二检查点 |
|:---|:---|:---|
| Cluster 长时间 Pending | InfraCluster conditions | InfraProvider 日志 |
| 控制面起不来 | KCP conditions / EtcdClusterHealthy | KCP controller 日志 |
| worker 不出现 | Machine bootstrapReady / infraReady | cloud-init 串口日志 |
| Cluster 删不掉 | finalizers / 残留子对象 | 云控制台依赖资源 |
| 升级卡住 | KCP/MachineDeployment conditions | etcd 健康 + 镜像拉取 |

---

<!-- chunk: 十一、相关文档 -->
## 十一、相关文档

### 域内相关

- [[10-平台工程/02-运维/13-multi-cluster-management|多集群管理]] — 多集群架构模式与工具对比，CAPI 与 Karmada/Rancher 的定位
- [[10-平台工程/02-运维/25-virtual-clusters|虚拟集群 vcluster]] — Namespace 内的轻量控制平面，与 CAPI 真实集群的对比与叠加
- [[10-平台工程/02-运维/99-karpenter-node-autoscaling-guide|Karpenter 节点弹性]] — 节点级弹性供给，与 CAPI 集群级供给的边界划分
- [[10-平台工程/02-运维/02-cluster-lifecycle-management|集群生命周期管理]] — 集群从创建到退役的通用流程

### GitOps 与应用分发

- [[11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops|Argo CD GitOps]] — 用 ApplicationSet 同步 CAPI Cluster CR 的实践
- [[11-发布变更/01-GitOps/08-fleet-gitops-operations-guide|Fleet GitOps 运维]] — Rancher Fleet 的多集群应用分发

### 知识字典

- [[17-系统基础/06-知识字典/platform-engineering/cluster-api-and-fleet-management|Cluster API 与 Fleet]] — CAPI 与集群舰队管理的概述性入门

### 官方资源

- Cluster API 官方文档: https://cluster-api.sigs.k8s.io/
- Cluster API Book（含 Provider 注册表）: https://cluster-api.sigs.k8s.io/user/quick-start
- Cluster API GitHub: https://github.com/kubernetes-sigs/cluster-api
- Provider 兼容矩阵: https://cluster-api.sigs.k8s.io/developer/providers/implemented-providers

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

<!-- risk-assessed -->
