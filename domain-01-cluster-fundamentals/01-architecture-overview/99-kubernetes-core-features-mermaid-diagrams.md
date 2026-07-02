---
title: Kubernetes v1.29-v1.33 核心特性架构图集
description: '# Kubernetes v1.29-v1.33 核心特性架构图集'
summary: 'I1["initContainer: start sidecar<br/>⚠️ 后台启动，不可靠"]'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- kubelet
- scheduler
- controller-manager
- job
- gateway
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v1.29-v1.33 核心特性架构图集 是什么
- 如何 Kubernetes v1.29-v1.33 核心特性架构图集
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- v1.29-v1.33
- 核心特性架构图集
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
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
- type: domain
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---



# [[Kubernetes|Kubernetes]] v1.29-v1.33 核心特性架构图集

> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 核心新特性的 Mermaid 架构图解  
> **目标读者**: 架构师、平台工程师、技术决策者

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [Sidecar 容器架构](#sidecar-容器架构)
- [CEL 准入策略架构](#cel-准入策略架构)
- [DRA 动态资源分配架构](#dra-动态资源分配架构)
- [In-Place Pod Resize 架构](#in-place-pod-resize-架构)
- [nftables kube-proxy 架构](#nftables-kube-proxy-架构)
- [Queueing Hints 调度优化架构](#queueing-hints-调度优化架构)
- [用户命名空间安全架构](#用户命名空间安全架构)
- [协调领导者选举架构](#协调领导者选举架构)

---

<!-- chunk: Sidecar 容器架构 -->
## Sidecar 容器架构

### 启动顺序状态机

```mermaid
stateDiagram-v2
    [*] --> Pending: 创建 Pod
    Pending --> InitRunning: 调度完成

    InitRunning --> SidecarStarted: Sidecar 容器启动
    SidecarStarted --> SidecarRunning: Sidecar 保持运行

    SidecarRunning --> InitCompleted: 普通 initContainer 完成
    InitCompleted --> ContainersReady: 主容器启动

    ContainersReady --> Running: 所有容器就绪
    Running --> Terminating: 收到删除请求

    Terminating --> MainTerminated: 主容器终止
    MainTerminated --> SidecarTerminated: Sidecar 优雅终止
    SidecarTerminated --> Succeeded: Pod 完成

    Succeeded --> [*]
```

### Sidecar 与普通 InitContainer 对比

```mermaid
flowchart TB
    subgraph Before["v1.33 之前 (Hack 方案)"]
        I1["initContainer: start sidecar<br/>⚠️ 后台启动，不可靠"]
        I2["initContainer: wait for sidecar<br/>⚠️ 复杂脚本"]
        C1["main container"]
        S1["sidecar (独立生命周期)<br/>⚠️ Job 无法完成"]
    end

    subgraph After["v1.33 GA (原生支持)"]
        SI["sidecar initContainer<br/>restartPolicy: Always ✅"]
        NI["normal initContainer<br/>完成任务后退出 ✅"]
        MC["main container"]
    end

    I1 --> I2 --> C1
    C1 --> S1

    SI --> NI --> MC

    style After fill:#e8f5e9
    style Before fill:#ffebee
```

### Job 场景中的 Sidecar

```mermaid
sequenceDiagram
    participant Kubelet as Kubelet
    participant Sidecar as Sidecar Container
    participant Init as Init Container
    participant Main as Main Container

    Kubelet->>Sidecar: 创建 Sidecar (restartPolicy: Always)
    Sidecar-->>Kubelet: Running
    Kubelet->>Init: 执行 Init Container
    Init-->>Kubelet: Completed
    Kubelet->>Main: 创建 Main Container
    Main-->>Kubelet: Running
    Main->>Main: 执行业务逻辑
    Main-->>Kubelet: Completed (exit 0)
    Kubelet->>Sidecar: 发送 SIGTERM
    Sidecar-->>Kubelet: Exited
    Kubelet->>Kubelet: Pod Phase = Succeeded
```

---

<!-- chunk: CEL 准入策略架构 -->
## CEL 准入策略架构

### Webhook vs CEL 准入对比

```mermaid
flowchart TB
    subgraph Webhook["传统 Webhook 模式"]
        API1["API Server"]
        W1["Webhook Service"]
        W2["Webhook Service"]
        CERT["证书管理"]
        NET["网络依赖"]

        API1 -->|HTTPS| W1
        API1 -->|HTTPS| W2
        W1 --> CERT
        W2 --> CERT
        W1 --> NET
        W2 --> NET
    end

    subgraph CEL["CEL 原生模式 (v1.30 GA)"]
        API2["API Server"]
        CEL_ENGINE["内置 CEL 引擎"]
        POLICY["ValidatingAdmissionPolicy"]

        API2 --> CEL_ENGINE
        POLICY --> CEL_ENGINE
    end

    style CEL fill:#e8f5e9
    style Webhook fill:#fff8e1
```

### CEL 准入策略执行流程

```mermaid
sequenceDiagram
    participant User as 用户
    participant API as API Server
    participant Auth as 认证/授权
    participant Mutate as Mutating Admission
    participant CEL as CEL 引擎
    participant Validate as Validating Admission
    participant ETCD as etcd

    User->>API: 提交 Pod 创建请求
    API->>Auth: 验证身份与权限
    Auth-->>API: 通过
    API->>Mutate: 执行 Mutating Webhook
    Mutate-->>API: 修改对象
    API->>CEL: 匹配 ValidatingAdmissionPolicy
    CEL->>CEL: 编译并执行 CEL 表达式
    CEL-->>API: 验证结果

    alt 验证失败
        API-->>User: 403 Forbidden + CEL 错误消息
    else 验证通过
        API->>Validate: 执行其他 Validating Webhook
        Validate-->>API: 通过
        API->>ETCD: 持久化对象
        ETCD-->>API: 确认
        API-->>User: 201 Created
    end
```

---

<!-- chunk: DRA 动态资源分配架构 -->
## DRA 动态资源分配架构

### DRA 完整数据流

```mermaid
flowchart TB
    subgraph User["用户层"]
        POD["Pod + ResourceClaimTemplate"]
    end

    subgraph Control["控制平面"]
        API["API Server"]
        SCHED["Scheduler + DRA Plugin"]
        CLASS["ResourceClass"]
        RC["ResourceClaim"]
        POOL["ResourcePool"]
    end

    subgraph Node["节点层"]
        KUBELET["Kubelet"]
        DRIVER["DRA Driver<br/>(如 NVIDIA)"]
        PLUGIN["Device Plugin"]
    end

    subgraph Hardware["硬件层"]
        GPU1["GPU 0"]
        GPU2["GPU 1"]
        FPGA["FPGA"]
    end

    POD -->|引用| RC
    RC -->|模板| CLASS
    API --> SCHED
    SCHED -->|分配决策| RC
    RC -->|Allocated| KUBELET
    KUBELET --> DRIVER
    DRIVER --> PLUGIN
    PLUGIN --> GPU1
    PLUGIN --> GPU2
    PLUGIN --> FPGA

    style Control fill:#e3f2fd
    style Node fill:#e8f5e9
    style Hardware fill:#f3e5f5
```

### DRA 与 Device Plugin 对比

```mermaid
flowchart TB
    subgraph DP["传统 Device Plugin"]
        P1["Pod (resources.limits.nvidia.com/gpu: 1)"]
        S1["Scheduler<br/>Extended Resources"]
        D1["Device Plugin<br/> Allocate()"]
        G1["GPU"]

        P1 --> S1 --> D1 --> G1
    end

    subgraph DRA["DRA (v1.33 GA)"]
        P2["Pod (resourceClaims)"]
        T2["ResourceClaimTemplate"]
        C2["ResourceClaim"]
        S2["Scheduler<br/>DRA Plugin"]
        DR2["DRA Driver"]
        G2["GPU"]

        P2 --> T2 --> C2
        S2 --> C2
        C2 --> DR2 --> G2
    end

    style DRA fill:#e8f5e9
```

---

<!-- chunk: In-Place Pod Resize 架构 -->
## In-Place Pod Resize 架构

### 资源调整状态流转

```mermaid
stateDiagram-v2
    [*] --> Running: Pod 创建
    Running --> Proposed: 用户 PATCH resources

    Proposed --> InProgress: Kubelet 接受调整
    Proposed --> Infeasible: 节点资源不足
    Proposed --> Deferred: 暂不可调整

    InProgress --> Complete: cgroup 更新完成
    InProgress --> Infeasible: 调整失败

    Infeasible --> Proposed: 重新尝试
    Deferred --> InProgress: 条件满足

    Complete --> Running: 继续运行
    Complete --> Proposed: 再次调整

    Running --> [*]: Pod 删除
```

### 原地调整与重建对比

```mermaid
flowchart TB
    subgraph InPlace["原地调整 (v1.33 Alpha)"]
        IP1["Pod Running"]
        IP2["PATCH resources"]
        IP3["Kubelet 更新 cgroup"]
        IP4["Pod 继续运行<br/>✅ 无中断"]

        IP1 --> IP2 --> IP3 --> IP4
    end

    subgraph Recreate["传统重建"]
        R1["Pod Running"]
        R2["删除 Pod"]
        R3["创建新 Pod"]
        R4["重新调度"]
        R5["容器启动<br/>❌ 服务中断"]

        R1 --> R2 --> R3 --> R4 --> R5
    end

    style InPlace fill:#e8f5e9
    style Recreate fill:#ffebee
```

---

<!-- chunk: nftables kube-proxy 架构 -->
## nftables kube-proxy 架构

### 三种代理模式对比

```mermaid
flowchart TB
    subgraph IPTables["iptables 模式"]
        I1["Service IP"]
        I2["iptables rules<br/>O(n) 查找"]
        I3["Pod Endpoints"]

        I1 --> I2 --> I3
    end

    subgraph IPVS["IPVS 模式"]
        V1["Service IP"]
        V2["IPVS virtual server<br/>O(1) 查找"]
        V3["Pod Endpoints"]

        V1 --> V2 --> V3
    end

    subgraph NFTables["nftables 模式 (v1.33 Beta)"]
        N1["Service IP"]
        N2["nftables ruleset<br/>增量更新 + 原生 IPv6"]
        N3["Pod Endpoints"]

        N1 --> N2 --> N3
    end

    style NFTables fill:#e8f5e9
```

### nftables 数据包路径

```mermaid
flowchart LR
    subgraph Node["Worker Node"]
        NIC["Physical NIC"]
        PREROUTING["nftables<br/>PREROUTING"]
        FORWARD["nftables<br/>FORWARD"]
        POSTROUTING["nftables<br/>POSTROUTING"]
        MASQ["MASQUERADE<br/>(SNAT)"]

        subgraph Pod["Target Pod"]
            P1["eth0"]
            P2["Container"]
        end
    end

    NIC --> PREROUTING
    PREROUTING -->|DNAT| FORWARD
    FORWARD --> P1
    P1 --> P2
    P2 --> P1
    P1 --> POSTROUTING
    POSTROUTING --> MASQ --> NIC

    style NFTables fill:#ffe0b2
```

---

<!-- chunk: Queueing Hints 调度优化架构 -->
## Queueing Hints 调度优化架构

### 传统调度队列 vs Queueing Hints

```mermaid
flowchart TB
    subgraph Traditional["传统调度队列"]
        T_POD1["Pod A<br/>需要 GPU"]
        T_POD2["Pod B<br/>需要大内存"]
        T_POD3["Pod C<br/>需要 SSD"]
        T_QUEUE["Unschedulable Queue"]
        T_EVENT["任意集群事件"]
        T_RETRY["全部重试"]

        T_POD1 --> T_QUEUE
        T_POD2 --> T_QUEUE
        T_POD3 --> T_QUEUE
        T_EVENT --> T_RETRY --> T_QUEUE
    end

    subgraph Hints["Queueing Hints (v1.33 Beta)"]
        H_POD1["Pod A<br/>Hint: GPU 相关事件"]
        H_POD2["Pod B<br/>Hint: 内存相关事件"]
        H_POD3["Pod C<br/>Hint: 存储相关事件"]
        H_QUEUE["Unschedulable Queue<br/>+ Hint Registry"]
        H_EVENT1["GPU 节点加入"]
        H_EVENT2["内存释放"]
        H_RETRY1["仅 Pod A 重试"]
        H_RETRY2["仅 Pod B 重试"]

        H_POD1 --> H_QUEUE
        H_POD2 --> H_QUEUE
        H_POD3 --> H_QUEUE
        H_EVENT1 --> H_RETRY1 --> H_POD1
        H_EVENT2 --> H_RETRY2 --> H_POD2
    end

    style Hints fill:#e8f5e9
```

### Queueing Hint 注册机制

```mermaid
sequenceDiagram
    participant Pod as 不可调度 Pod
    participant Scheduler as 调度器
    participant Plugin as 调度插件
    participant Queue as 调度队列
    participant Event as 集群事件

    Pod->>Scheduler: 调度失败
    Scheduler->>Plugin: 获取 QueueingHint
    Plugin->>Plugin: 分析失败原因
    Plugin-->>Scheduler: Hint: 需要 GPU 节点
    Scheduler->>Queue: 注册 Pod + Hint

    Event->>Queue: GPU 节点加入集群
    Queue->>Queue: 匹配 Hint
    Queue->>Scheduler: 唤醒 Pod A
    Scheduler->>Pod: 重新调度
```

---

<!-- chunk: 用户命名空间安全架构 -->
## 用户命名空间安全架构

### UID 映射模型

```mermaid
flowchart TB
    subgraph Container["容器内部"]
        ROOT["root (UID 0)"]
        USER1["user (UID 1000)"]
        USER2["user (UID 1001)"]
    end

    subgraph Mapping["ID 映射层"]
        M0["0 → 65536"]
        M1["1000 → 66536"]
        M2["1001 → 66537"]
    end

    subgraph Host["宿主机"]
        H0["UID 65536<br/>(无特权)"]
        H1["UID 66536"]
        H2["UID 66537"]
    end

    ROOT --> M0 --> H0
    USER1 --> M1 --> H1
    USER2 --> M2 --> H2

    style Container fill:#e3f2fd
    style Mapping fill:#fff8e1
    style Host fill:#c8e6c9
```

### 安全边界增强

```mermaid
flowchart TB
    subgraph Before["传统模式"]
        B_POD["Pod (root)"]
        B_BREAK["容器逃逸"]
        B_HOST["宿主机 root<br/>💥 完全控制"]

        B_POD --> B_BREAK --> B_HOST
    end

    subgraph After["用户命名空间 (v1.33 GA)"]
        A_POD["Pod (root)"]
        A_BREAK["容器逃逸"]
        A_MAP["ID 映射"]
        A_HOST["宿主机 UID 65536+<br/>✅ 无特权"]

        A_POD --> A_BREAK --> A_MAP --> A_HOST
    end

    style After fill:#e8f5e9
    style Before fill:#ffebee
```

---

<!-- chunk: 协调领导者选举架构 -->
## 协调领导者选举架构

### 传统 vs 协调选举对比

```mermaid
flowchart TB
    subgraph Traditional["传统领导者选举"]
        T_SCHED["kube-scheduler"]
        T_KCM["kube-controller-manager"]
        T_CCM["cloud-controller-manager"]
        T_LEASE1["Lease: scheduler"]
        T_LEASE2["Lease: kcm"]
        T_LEASE3["Lease: ccm"]
        T_ETCD["etcd"]

        T_SCHED --> T_LEASE1
        T_KCM --> T_LEASE2
        T_CCM --> T_LEASE3
        T_LEASE1 --> T_ETCD
        T_LEASE2 --> T_ETCD
        T_LEASE3 --> T_ETCD
    end

    subgraph Coordinated["协调领导者选举 (v1.32 Alpha)"]
        C_SCHED["kube-scheduler"]
        C_KCM["kube-controller-manager"]
        C_CCM["cloud-controller-manager"]
        C_CANDIDATE["LeaseCandidate"]
        C_COORD["协调器"]
        C_LEASE["统一 Lease"]
        C_ETCD["etcd"]

        C_SCHED --> C_CANDIDATE
        C_KCM --> C_CANDIDATE
        C_CCM --> C_CANDIDATE
        C_CANDIDATE --> C_COORD --> C_LEASE --> C_ETCD
    end

    style Coordinated fill:#e8f5e9
```

---

<!-- chunk: 附录：全特性架构总览 -->
## 附录：全特性架构总览

```mermaid
flowchart TB
    subgraph v129["v1.29"]
        RWOP["ReadWriteOncePod GA"]
        KMS2["KMS v2 GA"]
        NODELOG["Node Log Query Alpha"]
    end

    subgraph v130["v1.30"]
        CEL["ValidatingAdmissionPolicy GA"]
        BOUND["BoundServiceAccountToken GA"]
        READY["PodSchedulingReadiness GA"]
    end

    subgraph v131["v1.31"]
        GW["Gateway API v1.1"]
        APPARMOR["AppArmor GA"]
        PARALLEL["Parallel Image Pulls"]
        TRACE["KubeletTracing GA"]
    end

    subgraph v132["v1.32"]
        JOBPOLICY["Job Pod Replacement"]
        PODFAIL["Pod Failure Policy"]
        LEADER["Coordinated Leader Election"]
    end

    subgraph v133["v1.33"]
        SIDECAR["Sidecar GA"]
        DRA["DRA GA"]
        RESIZE["In-Place Resize Alpha"]
        NFTABLES["nftables Beta"]
        QUEUE["Queueing Hints Beta"]
        METRICS["KubeletResourceMetrics Beta"]
        USERNS["UserNamespaces GA"]
    end

    v129 --> v130 --> v131 --> v132 --> v133

    style v133 fill:#c8e6c9
```

---

<!-- chunk: 参考链接 -->
## 参考链接

- [Kubernetes 特性门控](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates/)
- [Sidecar KEP](https://github.com/kubernetes/enhancements/tree/master/keps/sig-node/753-sidecar-containers)
- [DRA KEP](https://github.com/kubernetes/enhancements/tree/master/keps/sig-scheduling/3960-dynamic-resource-allocation)
- [Queueing Hints KEP](https://github.com/kubernetes/enhancements/tree/master/keps/sig-scheduling/4247-queueing-hint)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals KUDIG Database — Global MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- index.md|Domain-1 架构基础 — 开源项目索引]]
- Kubernetes 架构全景图
- [[entities/kubernetes.md|kubernetes]]
- 03 - 功能和API表
- structure.md|04 - Kubernetes 源码结构深度解析]]
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)

## See Also

- 99-kubernetes-api-version-matrix
- 99-kubernetes-core-components-v1.29-v1.33-update
- 99-kubernetes-v1.25-v1.33-feature-comparison-table
- 99-kubernetes-v1.29-v1.33-complete-feature-gates-reference
