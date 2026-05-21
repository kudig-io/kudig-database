Kubernetes 网络是该系统中最复杂、最核心的子系统之一。本文从 CNI 容器网络接口出发，逐层向上剖析 **Service 四层负载均衡**、**Ingress 七层路由**、**Gateway API 新一代流量管理标准**，直至 **多集群网络互联** 与 **Service Mesh 服务网格**，为高级开发者提供一套完整的网络知识图谱与生产级实践参考。本页覆盖 `domain-03-networking-traffic/` 目录下 38 篇文档的精华要点，结合 `domain-01-cluster-fundamentals/23-container-network-deep-dive.md` 的 CNI 规范解析与 `domain-03-networking-traffic/03-cilium-cni-architecture.md` 的 eBPF 数据面深度，构建从内核到应用层的全栈网络认知。

Sources: [README.md](domain-03-networking-traffic/README.md#L1-L139), [01-network-architecture-overview.md](domain-03-networking-traffic/01-network-architecture-overview.md#L1-L58)

## 一、Kubernetes 网络架构全景

Kubernetes 网络遵循三条基本模型原则：**所有 Pod 可直接通信（无需 NAT）**、**节点可直接与所有 Pod 通信**、**Pod 看到的自身 IP 与其他 Pod 看到的一致**。在这个模型之上，网络栈被划分为四个清晰的层次——CNI 层负责 Pod IP 分配与跨节点通信，kube-proxy 层实现 Service 虚拟 IP 到 Pod IP 的 DNAT 转发，Service 层提供稳定的抽象入口，而 Ingress/Gateway API 层则管理从集群外部到内部服务的 L7 路由。

```mermaid
graph TB
    subgraph "外部流量入口"
        Internet[Internet / CDN / WAF]
        CloudLB[Cloud Load Balancer<br/>SLB / ALB / NLB]
    end
    
    subgraph "L7 路由层"
        Ingress[Ingress Controller<br/>Nginx / Traefik]
        GW[Gateway API<br/>Envoy Gateway / Istio]
    end
    
    subgraph "Service 四层负载均衡"
        CIP[ClusterIP<br/>内部 LB]
        NP[NodePort<br/>端口映射]
        LB[LoadBalancer<br/>云 LB 集成]
        HS[Headless<br/>直连 Pod]
    end
    
    subgraph "kube-proxy 数据面"
        iptables[iptables<br/>O(n) 链遍历]
        IPVS[IPVS<br/>O(1) 哈希查找]
        eBPF[eBPF<br/>Cilium 内核级]
    end
    
    subgraph "CNI 网络层"
        Calico[Calico<br/>BGP / VXLAN / eBPF]
        Cilium[Cilium<br/>eBPF 原生]
        Flannel[Flannel<br/>VXLAN Overlay]
        Terway[Terway<br/>VPC ENI]
    end
    
    subgraph "物理网络"
        VPC[VPC / VxLAN / BGP]
    end
    
    Internet --> CloudLB
    CloudLB --> Ingress & GW
    Ingress & GW --> CIP & NP & LB
    CIP & NP & LB --> iptables & IPVS & eBPF
    HS --> iptables & IPVS & eBPF
    iptables & IPVS & eBPF --> Calico & Cilium & Flannel & Terway
    Calico & Cilium & Flannel & Terway --> VPC
```

**网络分层职责速查**：

| 层次 | 核心职责 | 关键组件 | 关注点 |
|------|---------|---------|-------|
| **L7 路由层** | 域名/路径路由、TLS 终止、限流认证 | Ingress Controller、Gateway API | 南北向流量入口 |
| **Service 层** | 稳定虚拟 IP、四层负载均衡、服务发现 | ClusterIP / NodePort / LoadBalancer / Headless | 东西向服务通信 |
| **kube-proxy 层** | DNAT 规则同步、连接追踪、负载均衡算法 | iptables / IPVS / nftables / eBPF | 数据面转发效率 |
| **CNI 层** | Pod IP 分配、跨节点隧道、网络策略执行 | Calico / Cilium / Flannel / Terway | Pod 网络连通性 |

Sources: [02-cni-architecture-fundamentals.md](domain-03-networking-traffic/02-cni-architecture-fundamentals.md#L7-L57), [01-network-architecture-overview.md](domain-03-networking-traffic/01-network-architecture-overview.md#L1-L58)

## 二、CNI 容器网络接口

### 2.1 CNI 规范与 Pod 网络创建流程

CNI（Container Network Interface）是 Kubernetes 与网络插件之间的标准接口协议。当 kubelet 创建 Pod 的 sandbox 容器后，通过 CRI 调用 CNI 插件执行 **ADD**（创建网络命名空间、分配 IP、配置路由）、**DEL**（清理资源、释放 IP）、**CHECK**（健康检查）、**VERSION**（版本查询）四种操作。CNI 配置文件为 JSON 格式，位于 `/etc/cni/net.d/` 目录，支持 **插件链（Chaining）** 模式——主插件（如 calico）负责网络连通，IPAM 插件负责 IP 分配，元插件（如 portmap、bandwidth）提供端口映射和带宽限制等附加能力。

Pod 网络初始化的完整链路为：API Server 调度 Pod → kubelet 创建 sandbox → 调用 CNI ADD → IPAM 分配 IP → 创建 veth pair → 配置路由规则 → 返回 IP 配置 → 启动业务容器。CNI 配置中的环境变量（`CNI_COMMAND`、`CNI_CONTAINERID`、`CNI_NETNS`、`CNI_IFNAME`）由容器运行时传入，插件根据这些参数完成网络配置。

Sources: [02-cni-architecture-fundamentals.md](domain-03-networking-traffic/02-cni-architecture-fundamentals.md#L59-L165), [23-container-network-deep-dive.md](domain-01-cluster-fundamentals/23-container-network-deep-dive.md#L44-L100)

### 2.2 CNI 插件全景对比

选择 CNI 插件是集群网络设计的首要决策。下表从网络模式、安全策略、可观测性、多集群支持等维度对主流 CNI 进行全面对比：

| 能力维度 | Calico | Cilium | Flannel | Terway (ACK) | Antrea |
|---------|--------|--------|---------|-------------|--------|
| **网络模式** | VXLAN / IPIP / BGP | VXLAN / Native / eBPF | VXLAN / host-gw | VPC / ENIIP | VXLAN / Geneve |
| **NetworkPolicy** | ✅ 完整 L3/L4 | ✅ 完整 + L7 (HTTP/gRPC) | ❌ | ✅ 完整 | ✅ 完整 |
| **eBPF 数据面** | ✅ 可选启用 | ✅ 原生核心 | ❌ | ✅ | ❌ |
| **Service Mesh** | ❌ | ✅ Cilium Mesh | ❌ | ASM 集成 | ❌ |
| **多集群互联** | ✅ | ✅ ClusterMesh | ❌ | ACK One | ✅ |
| **加密传输** | WireGuard | WireGuard / IPsec | ❌ | ✅ | IPsec |
| **可观测性** | 基础 | ✅ Hubble 全景 | 基础 | ARMS 集成 | ✅ |
| **Windows 支持** | ✅ | ⚠️ Beta | ✅ | ✅ | ✅ |

**生产环境选型建议**：通用生产首选 **Calico**（成熟稳定、BGP 原生支持）；高性能与安全并重选 **Cilium**（eBPF 原生、L7 策略、kube-proxy 替代）；阿里云环境选 **Terway**（VPC 原生性能最优）；简单测试场景选 **Flannel**（配置极简）。Cilium 作为 CNCF 毕业项目，已拥有 5000+ 生产集群部署，其 eBPF 数据面以 O(1) hash map 查找取代传统 iptables 的 O(n) 链遍历，在大规模集群（>5000 Service）中性能优势显著。

Sources: [03-cni-plugins-comparison.md](domain-03-networking-traffic/03-cni-plugins-comparison.md#L1-L200), [03-cilium-cni-architecture.md](domain-03-networking-traffic/03-cilium-cni-architecture.md#L25-L58)

### 2.3 Overlay 与 Native 网络模式

CNI 的网络连通性实现分为两大类：**Overlay 网络**通过封装（VXLAN / IPIP / Geneve）在现有物理网络之上构建虚拟二层网络，配置简单但对性能有一定损耗；**Native 网络**（BGP 直连 / VPC ENI）直接利用底层网络路由，性能最优但要求网络基础设施支持。以 Flannel VXLAN 为例，跨节点 Pod 通信时，原始数据包（Pod Eth + Pod IP + TCP + Payload）被封装为 VXLAN 包（Outer Eth + Outer IP + UDP:4789 + VXLAN Header + 原始数据包），在物理网络中传输，对底层网络完全透明。Calico BGP 模式则通过 BGP 协议在节点间传播 Pod CIDR 路由，数据包无需封装，吞吐量更高、延迟更低。

Sources: [02-cni-architecture-fundamentals.md](domain-03-networking-traffic/02-cni-architecture-fundamentals.md#L185-L230)

## 三、Service 四层负载均衡

### 3.1 Service 类型矩阵

Service 是 Kubernetes 中最核心的网络抽象，为一组 Pod 提供稳定的虚拟 IP 和 DNS 名称。五种 Service 类型各有明确的使用场景：

| 类型 | 访问范围 | 核心特征 | 生产推荐度 | 典型场景 |
|------|---------|---------|-----------|---------|
| **ClusterIP** | 集群内部 | 虚拟 IP + 内部 DNS，默认类型 | ⭐⭐⭐⭐⭐ | 微服务间通信 |
| **NodePort** | 集群外部 | 30000-32767 端口映射到节点 | ⭐⭐ | 测试环境、物理 LB 后端 |
| **LoadBalancer** | 集群外部 | 自动创建云厂商 LB | ⭐⭐⭐⭐ | 单服务对外暴露 |
| **ExternalName** | 集群内部 | CNAME 映射到外部域名 | ⭐⭐⭐ | 引用集群外部服务 |
| **Headless** (clusterIP: None) | 集群内部 | 无虚拟 IP，直接返回 Pod IP | ⭐⭐⭐⭐⭐ | StatefulSet、数据库集群 |

Service 的工作流程为四步：① `kubectl apply` 提交 Service 定义到 API Server → ② EndpointSlice Controller Watch Pod 变化、更新端点信息 → ③ kube-proxy Watch Service 和 EndpointSlice、同步转发规则到内核 → ④ 客户端请求 Service IP 被 DNAT 为 Pod IP。其中 EndpointSlice（v1.21+ GA）取代了早期的 Endpoints 对象，通过分片存储（每 slice 最多 100 个 endpoint）解决了大规模集群的性能瓶颈。

Sources: [06-service-concepts-types.md](domain-03-networking-traffic/06-service-concepts-types.md#L1-L200), [01-network-architecture-overview.md](domain-03-networking-traffic/01-network-architecture-overview.md#L62-L200)

### 3.2 kube-proxy 模式与性能

kube-proxy 是 Service 数据面的实现组件，运行在每个节点上，有四种工作模式：

| 特性 | iptables | IPVS | nftables | eBPF (Cilium) |
|------|----------|------|----------|--------------|
| **状态** | 默认 | 推荐 | Alpha (v1.29+) | 生产就绪 |
| **查找复杂度** | O(n) | O(1) | O(1) | O(1) |
| **大规模支持** | < 5000 Service | > 10000 Service | > 10000 Service | > 10000 Service |
| **负载均衡算法** | 随机 | rr/wrr/lc/wlc/sh 等 10 种 | 多种 | Maglev / DSR |
| **规则更新** | 全量刷新 | 增量更新 | 增量更新 | 增量更新 |

**iptables 模式**的规则链结构为 `PREROUTING → KUBE-SERVICES → KUBE-SVC-xxx → KUBE-SEP-xxx`，每条 Service 对应一条 SVC 链，每个 Endpoint 对应一条 SEP 链（DNAT 规则）。当 Service 数量超过 5000 时，规则链遍历的 O(n) 开销会导致显著延迟。**IPVS 模式**利用内核 ip_vs 模块的哈希表实现 O(1) 查找，支持 rr（轮询）、wrr（加权轮询）、lc（最少连接）、sh（源哈希）等 10 种调度算法，是大规模集群的推荐选择。**eBPF 模式**（Cilium kube-proxy replacement）在 socket 层直接完成路由决策，完全绕过 iptables/netfilter 栈，配合 DSR（Direct Server Return）可实现请求路径最优化。

Sources: [09-kube-proxy-modes-performance.md](domain-03-networking-traffic/09-kube-proxy-modes-performance.md#L1-L150)

### 3.3 流量策略与拓扑感知

**externalTrafficPolicy** 控制外部流量的处理方式：`Cluster`（默认）模式下流量可在节点间转发、负载均衡均匀但源 IP 被 SNAT 丢失；`Local` 模式仅转发到本节点 Pod、保留源 IP 但可能导致负载不均。**internalTrafficPolicy**（v1.26+ GA）将相同机制应用于集群内部流量，`Local` 模式可减少跨节点跳数、降低延迟。

**拓扑感知路由（Topology Aware Hints）**（v1.27+ GA）是一项重要的成本优化特性。通过在 Service 上设置注解 `service.kubernetes.io/topology-aware-hints: "auto"`，kube-proxy 自动优先将流量路由到同一可用区的 Endpoint，减少跨 AZ 流量。生产实测数据显示延迟降低 40-60%，跨 AZ 流量成本降低 70%，特别适用于跨多可用区部署的缓存、数据库等延迟敏感型服务。

Sources: [10-service-advanced-features.md](domain-03-networking-traffic/10-service-advanced-features.md#L1-L100), [01-network-architecture-overview.md](domain-03-networking-traffic/01-network-architecture-overview.md#L136-L160)

## 四、Ingress 七层流量管理

### 4.1 Ingress 架构与核心概念

Ingress 是 Kubernetes 管理集群外部 HTTP/HTTPS 访问的 API 对象（v1.19+ GA），提供基于域名和路径的 L7 路由、TLS 终止、虚拟主机等功能。Ingress 资源本身仅是声明式配置，实际流量处理由 **Ingress Controller**（如 Nginx、Traefik、Envoy）完成。Controller 通过 Watch Ingress、Service、EndpointSlice、Secret 等资源的变化，动态生成代理配置（如 nginx.conf），由代理引擎执行实际的 TLS 终止、路由匹配、负载均衡和限流等操作。

**Ingress 解决的核心问题**：没有 Ingress 时，每个对外服务都需要独立的 LoadBalancer（成本高）、无法按域名/路径路由、TLS 需要在每个服务单独配置、无法实现金丝雀发布等高级流量管理。Ingress 通过统一入口实现多服务共享、集中 TLS 管理、灵活路由规则和安全策略。

| 方案对比 | ClusterIP | NodePort | LoadBalancer | **Ingress** | **Gateway API** |
|---------|-----------|----------|-------------|-------------|----------------|
| 协议层 | L4 | L4 | L4 | **L7** | **L4/L7** |
| 路由能力 | 无 | 无 | 无 | Host/Path | Host/Path/Header/Query |
| TLS 终止 | 无 | 无 | 可选 | ✅ | ✅ |
| 成本 | 无 | 无 | 每服务一个 LB | **共享控制器** | **共享网关** |
| 适用场景 | 内部通信 | 测试 | 单服务暴露 | **多服务 HTTP 路由** | **复杂多协议** |

Sources: [19-ingress-fundamentals.md](domain-03-networking-traffic/19-ingress-fundamentals.md#L1-L200)

### 4.2 Ingress Controller 选型与生产实践

主流 Ingress Controller 的对比选择直接决定了集群南北向流量的治理能力：

| 控制器 | 代理引擎 | 性能 | Gateway API 支持 | 特色能力 |
|-------|---------|------|-----------------|---------|
| **Nginx Ingress** | Nginx | 高 | 部分 | 成熟稳定、注解丰富（注意：2026 年宣布退役计划） |
| **Envoy Gateway** | Envoy | 很高 | ✅ 官方实现 | Gateway API 一等公民、社区标准 |
| **Traefik** | Traefik | 中-高 | ✅ | 动态配置、中间件生态 |
| **ALB Ingress** | 阿里云 ALB | 很高 | ✅ | ACK 原生、免运维、自动弹性 |
| **Istio Gateway** | Envoy | 高 | ✅ | 服务网格深度集成 |

Nginx Ingress 通过注解实现高级功能——`canary: "true"` + `canary-weight: "10"` 实现金丝雀发布，`limit-rps: "100"` 实现限流，`rewrite-target` 实现 URL 重写，`ssl-redirect: "true"` 强制 HTTPS。但注解方式的碎片化问题（不同控制器注解语法完全不同）正是 Gateway API 要解决的核心痛点。

生产环境最佳实践包括：IngressClass 指定控制器类型、独立命名空间部署 Controller、配置 DefaultBackend 处理 404、启用 Prometheus 指标采集、配置 access-log 集中分析、使用 cert-manager 自动化证书管理。

Sources: [36-api-gateway-patterns.md](domain-03-networking-traffic/36-api-gateway-patterns.md#L1-L138), [19-ingress-fundamentals.md](domain-03-networking-traffic/19-ingress-fundamentals.md#L124-L161)

## 五、Gateway API：新一代流量管理标准

### 5.1 从 Ingress 到 Gateway API 的演进

2026 年 3 月，NGINX Ingress Controller 官方宣布退役计划，标志着 Kubernetes 正式进入 **Gateway API 时代**。Ingress API 自 2015 年引入以来停留在 `networking.k8s.io/v1`，从未进入 v2 迭代——配置不可移植（各厂商注解语法不同）、权限模型混乱（集群级 Ingress 与命名空间级 Service 混合）、缺乏标准化状态反馈、多租户支持薄弱。

Gateway API 从 v1.0（2023-10 GA）到 v1.4（2025-11），经历了系统性的演进：v1.0 核心 CRD（GatewayClass、Gateway、HTTPRoute）GA；v1.1 GRPCRoute GA + BackendTLSPolicy；v1.2 BackendLBPolicy 实验；v1.3 **GAMMA**（Gateway API for Mesh Management and Administration）Mesh 路由标准化；v1.4 BackendTLSPolicy GA。

**核心设计哲学是三层角色分离**：

| 角色 | 管理的资源 | 职责范围 | 典型人员 |
|------|----------|---------|---------|
| **基础设施管理员** | GatewayClass | 定义底层网关实现类型（Istio/Nginx/Envoy） | 平台团队 |
| **集群/平台管理员** | Gateway | 定义流量入口（IP、端口、TLS 证书、允许路由的命名空间） | 运维团队 |
| **应用开发者** | HTTPRoute / GRPCRoute 等 | 定义具体路由规则、流量分割、请求过滤 | 业务开发 |

Sources: [19-kubernetes-gateway-api-modern-traffic-management.md](domain-19-landscape-references/19-kubernetes-gateway-api-modern-traffic-management.md#L1-L100), [35-gateway-api-overview.md](domain-03-networking-traffic/35-gateway-api-overview.md#L1-L75)

### 5.2 Gateway API CRD 体系与核心配置

Gateway API 的 CRD 体系围绕四个核心资源构建：**GatewayClass**（集群作用域，定义网关实现控制器）→ **Gateway**（命名空间作用域，定义监听器配置）→ **HTTPRoute** / GRPCRoute / TLSRoute / TCPRoute（命名空间作用域，定义路由规则）→ **ReferenceGrant**（跨命名空间安全引用许可）。

Gateway API 相比 Ingress 的关键能力跃升：

| 特性 | Ingress | Gateway API |
|------|---------|-------------|
| **角色分离** | ❌ 混合权限 | ✅ 三层角色、各司其职 |
| **多协议** | HTTP(S) 仅 | HTTP/HTTPS/TCP/UDP/gRPC/TLS 全覆盖 |
| **流量分割** | ❌ 注解 hack | ✅ 原生 weight 权重分配 |
| **请求修改** | 注解依赖 | ✅ 标准 Filter 机制（URLRewrite / RequestHeaderModifier / RequestRedirect） |
| **跨命名空间** | ❌ 不安全 | ✅ ReferenceGrant 安全控制 |
| **网格治理 (GAMMA)** | ❌ | ✅ East-West 流量标准化 |

GAMMA 是 Gateway API 的一个重要扩展方向——将 HTTPRoute 直接绑定到 Service（而非 Gateway），实现服务网格内部 East-West 流量的标准化治理。例如通过 `parentRefs: [{kind: Service, name: users-service}]` 将 `/v2` 路径路由到 `users-v2` 服务，无需任何 Sidecar 注入。

Sources: [35-gateway-api-overview.md](domain-03-networking-traffic/35-gateway-api-overview.md#L1-L200)

## 六、NetworkPolicy 与网络安全

### 6.1 零信任网络策略设计

Kubernetes NetworkPolicy 是集群内网络隔离的标准机制，但默认情况下所有 Pod 间通信完全开放。生产环境必须采用**零信任安全模型**——从默认拒绝所有流量开始，按需开放最小权限。

```mermaid
graph LR
    subgraph "第一层：默认拒绝"
        A[default-deny-all<br/>podSelector: {}<br/>Ingress + Egress 全部拒绝]
    end
    
    subgraph "第二层：命名空间隔离"
        B[namespace-isolation<br/>仅允许同命名空间 Ingress]
    end
    
    subgraph "第三层：应用间最小权限"
        C[frontend-to-backend<br/>仅允许 frontend → backend:8080]
        D[backend-to-database<br/>仅允许 backend → database:5432]
    end
    
    subgraph "第四层：出站控制"
        E[egress-control<br/>仅允许 DNS + 指定外部 API]
    end
    
    A --> B --> C & D --> E
```

NetworkPolicy 选择器的组合逻辑需特别注意：同一 `from` 条目内的 `namespaceSelector` 和 `podSelector` 是 **AND 关系**（必须同时满足）；多个 `from` 条目之间是 **OR 关系**（满足任一即可）。生产环境常见错误是将不同命名空间的 Pod 选择器写在同一 `from` 条目内，导致策略永远无法匹配。

Sources: [16-networkpolicy-deep-practice.md](domain-03-networking-traffic/16-networkpolicy-deep-practice.md#L1-L120)

### 6.2 Egress 出站流量管理

Egress 流量管理是网络安全中容易被忽视的环节。出站控制方案从 L3 到 L7 形成完整的防御纵深：**NetworkPolicy**（L3/L4 IP+端口级）→ **Egress Gateway**（统一出口 IP，用于 IP 白名单场景）→ **NAT Gateway**（云原生 NAT，VPC 级控制）→ **Service Mesh**（L7 URL/Header 级精细控制）→ **Cilium eBPF**（L3-L7 全栈高性能控制）。对于需要固定出口 IP 的合规场景（如第三方 API 白名单），Istio Egress Gateway 或 Cilium Egress Gateway 是推荐方案。

Sources: [29-egress-traffic-management.md](domain-03-networking-traffic/29-egress-traffic-management.md#L1-L122)

## 七、Service Mesh 服务网格

Service Mesh 在 Service 之上提供了更精细的流量治理能力——mTLS 加密、熔断、重试、超时、金丝雀发布、可观测性等。当前两大架构模式并存：**Sidecar 模式**（传统）在每个 Pod 注入 Envoy 代理，细粒度控制但资源开销大（每 Pod +50MB）；**Ambient 模式**（未来趋势）采用分层架构——ztunnel 节点级 DaemonSet 处理 L4 转发，Waypoint Proxy 按需部署处理 L7 治理，资源开销降低 70%+，且可独立升级不干扰应用。

Istio 的流量拦截机制通过 `istio-init` 容器设置 iptables 规则，将所有入站流量重定向到 15006 端口（Envoy）、出站流量重定向到 15001 端口。Cilium Service Mesh 则采用完全不同的路径——利用 eBPF 在内核 socket 层完成路由决策，无需 iptables 规则注入，也无需 Sidecar 容器，实现了"无 Sidecar 的内核级服务网格"。

Sources: [30-service-mesh-deep-dive.md](domain-03-networking-traffic/30-service-mesh-deep-dive.md#L1-L150)

## 八、多集群网络互联

### 8.1 多集群网络架构模式

多集群网络的核心挑战是跨集群 Pod-to-Pod 连通性和跨集群服务发现。四种基础架构模式各有取舍：

| 模式 | 适用场景 | 优势 | 劣势 |
|------|---------|------|------|
| **扁平网络** | 同云同区域 | 配置简单、性能最优 | 网络隔离差 |
| **VPC 对等连接** | 同云跨区域 | 原生性能、成本较低 | 配置复杂 |
| **VPN 隧道** | 混合云/跨云 | 安全性强、兼容性好 | 性能损耗 |
| **专用线路** | 金融/政企 | 性能最优、安全最高 | 成本极高 |

CIDR 规划是多集群网络的基础——每个集群必须分配不重叠的 Pod CIDR 和 Service CIDR。例如三集群方案：cluster1 使用 `10.244.0.0/18` + `10.96.0.0/14`，cluster2 使用 `10.244.64.0/18` + `10.100.0.0/14`，cluster3 使用 `10.244.128.0/18` + `10.104.0.0/14`。

Sources: [31-multi-cluster-federation.md](domain-03-networking-traffic/31-multi-cluster-federation.md#L1-L100)

### 8.2 跨集群服务发现与流量治理

**Karmada** 已成为 CNCF 孵化项目，是多集群资源分发的事实标准。通过 `PropagationPolicy` 定义资源分发策略（如 2:1 权重分配到 cluster-east 和 cluster-west），再结合 `OverridePolicy` 进行集群差异化配置。**MCS（Multi-cluster Service）API** 是 Kubernetes 官方的跨集群服务标准：在源集群创建 `ServiceExport` 导出服务，消费集群自动生成 `ServiceImport`，通过 `svc.clusterset.local` 域名实现跨集群服务发现。

**Cilium ClusterMesh** 提供了更优雅的多集群互联方案：通过 `cilium clustermesh connect` 建立集群间连接，为 Service 添加 `service.cilium.io/global: "true"` 注解即可实现全局服务发现，流量自动在所有集群的后端 Pod 间负载均衡。**Submariner** 则专注于跨集群 L3 网络连通，通过 `ServiceExport` + `svc.clusterset.local` 域名机制暴露跨集群服务。

| 方案 | 互联技术 | 服务发现 | 适用场景 |
|------|---------|---------|---------|
| Submariner | 隧道 | MCS ServiceExport | 混合云 |
| Cilium ClusterMesh | eBPF 隧道 | 全局 Service 注解 | 同质集群 |
| Istio 多集群 | 服务网格 | DNS 自动发现 | 服务治理 |
| Skupper | 应用层 | 服务暴露 | 简单互联 |
| Karmada | 控制面联邦 | PropagationPolicy | 多集群编排 |

Sources: [32-multi-cluster-networking.md](domain-03-networking-traffic/32-multi-cluster-networking.md#L1-L150), [31-multi-cluster-federation.md](domain-03-networking-traffic/31-multi-cluster-federation.md#L104-L170)

## 九、网络排障与性能调优

### 9.1 网络故障排查方法论

网络排障遵循自底向上的分层诊断原则：① 物理网络连通性（ping 节点 IP）→ ② CNI 层 Pod IP 分配与跨节点通信（`kubectl describe pod` 查看 IP、跨节点 ping Pod IP）→ ③ DNS 解析（`dig svc.ns.svc.cluster.local`）→ ④ Service 端点（`kubectl get endpoints`）→ ⑤ kube-proxy 规则（`iptables -t nat -L KUBE-SERVICES`）→ ⑥ Ingress/Gateway 路由（Controller 日志 + 资源 status）。

| 问题现象 | 诊断方向 | 关键命令 |
|---------|---------|---------|
| Pod 无 IP | IPAM 配置 | `kubectl describe pod` |
| 跨节点 Pod 不通 | BGP/隧道状态 | `calicoctl node status` / `cilium status` |
| DNS 解析失败 | CoreDNS | `kubectl exec -- nslookup` |
| Service 不可达 | Endpoint/kube-proxy | `kubectl get endpoints` / `iptables -t nat -L` |
| Ingress 502/503 | 后端健康检查 | Controller 日志 + `kubectl describe ingress` |

Sources: [27-cni-troubleshooting-optimization.md](domain-03-networking-traffic/27-cni-troubleshooting-optimization.md#L1), [33-network-troubleshooting.md](domain-03-networking-traffic/33-network-troubleshooting.md#L1)

### 9.2 生产环境最佳实践总结

| 维度 | 最佳实践 |
|------|---------|
| **CNI 选型** | 通用: Calico；高性能: Cilium；阿里云: Terway |
| **Service** | 内部通信用 ClusterIP；对外用 LoadBalancer + Ingress 共享 |
| **kube-proxy** | >1000 Service 切换 IPVS；>5000 考虑 Cilium eBPF |
| **Ingress/Gateway** | 新部署优先 Gateway API；存量 Nginx 渐进迁移 |
| **NetworkPolicy** | 零信任默认拒绝 + 分层策略 + 最小权限 |
| **DNS** | CoreDNS + NodeLocal DNSCache 减少 DNS 延迟 |
| **可观测性** | Prometheus 指标 + Hubble/Cilium 流量可视化 |
| **多集群** | CIDR 提前规划 + Karmada 编排 + Cilium ClusterMesh |
| **证书** | cert-manager 自动化 + Gateway API BackendTLSPolicy |

Sources: [README.md](domain-03-networking-traffic/README.md#L81-L100), [34-network-performance-tuning.md](domain-03-networking-traffic/34-network-performance-tuning.md#L1)

## 十、知识域导航与学习路径

本文覆盖了 Kubernetes 网络体系的核心知识，从 CNI 底层到多集群顶层。以下是基于知识图谱的推荐学习路径：

**渐进式学习路径**：

| 阶段 | 聚焦领域 | 推荐阅读顺序 |
|------|---------|------------|
| **入门（网络基础）** | 网络架构全景 + CNI 基础 | [01-network-architecture-overview](domain-03-networking-traffic/01-network-architecture-overview.md) → [02-cni-architecture-fundamentals](domain-03-networking-traffic/02-cni-architecture-fundamentals.md) → [04-flannel-complete-guide](domain-03-networking-traffic/04-flannel-complete-guide.md) |
| **进阶（Service 与 DNS）** | Service 类型 + kube-proxy + CoreDNS | [06-service-concepts-types](domain-03-networking-traffic/06-service-concepts-types.md) → [09-kube-proxy-modes-performance](domain-03-networking-traffic/09-kube-proxy-modes-performance.md) → [11-dns-service-discovery-coredns](domain-03-networking-traffic/11-dns-service-discovery-coredns.md) |
| **高级（Ingress 与 Gateway）** | L7 路由 + Gateway API | [19-ingress-fundamentals](domain-03-networking-traffic/19-ingress-fundamentals.md) → [35-gateway-api-overview](domain-03-networking-traffic/35-gateway-api-overview.md) → [19-kubernetes-gateway-api-modern-traffic-management](domain-19-landscape-references/19-kubernetes-gateway-api-modern-traffic-management.md) |
| **专家（安全与多集群）** | NetworkPolicy + 多集群 + Service Mesh | [16-networkpolicy-deep-practice](domain-03-networking-traffic/16-networkpolicy-deep-practice.md) → [31-multi-cluster-federation](domain-03-networking-traffic/31-multi-cluster-federation.md) → [30-service-mesh-deep-dive](domain-03-networking-traffic/30-service-mesh-deep-dive.md) |

**关联知识域跳转**：

- 深入 eBPF 数据面原理：[eBPF 技术：Cilium CNI 架构与 eBPF 数据面详解](27-ebpf-ji-zhu-ping-tai-gong-cheng-bian-yuan-ji-suan-yu-webassembly) 中的 Cilium 章节
- 云厂商托管网络实现：[云厂商托管 Kubernetes 服务全景对比](22-yun-han-shang-tuo-guan-kubernetes-fu-wu-quan-jing-dui-bi-13-jia-han-shang) 中 Terway / ALB / SLB 部分
- 存储网络对接：[存储体系：PV/PVC、StorageClass、CSI 驱动与灾备恢复](10-cun-chu-ti-xi-pv-pvc-storageclass-csi-qu-dong-yu-zai-bei-hui-fu)
- 安全合规纵深：[安全合规：RBAC、网络安全策略、运行时安全与零信任架构](11-an-quan-he-gui-rbac-wang-luo-an-quan-ce-lue-yun-xing-shi-an-quan-yu-ling-xin-ren-jia-gou)
- YAML 配置参考：[YAML 配置清单](29-yaml-pei-zhi-qing-dan-kubernetes-quan-zi-yuan-zi-duan-can-kao-shou-ce) 中 Service、Ingress、Gateway API、NetworkPolicy 全字段参考