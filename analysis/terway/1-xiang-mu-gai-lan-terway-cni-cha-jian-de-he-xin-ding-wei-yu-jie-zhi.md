Terway 是阿里云容器服务 ACK（Alibaba Cloud Kubernetes）自研的 **容器网络接口（CNI）插件**，基于阿里云**弹性网卡（ENI）**技术构建容器网络，使每个 Pod 拥有独立的网络栈和 VPC IP 地址。它消除了传统 Overlay 网络方案中 VxLAN 隧道封装的开销——同 ECS 内 Pod 直通、跨 ECS Pod 通过 VPC 弹性网卡直接转发——从而实现接近原生 VPC 的网络性能。Terway 支持多种网络模式、eBPF 协议栈加速、Kubernetes NetworkPolicy、IPv4/IPv6 双栈以及灵活的安全组配置，是阿里云上运行 Kubernetes 工作负载的推荐网络方案。

Sources: [README.md](README.md#L1-L53) · [README-zh_CN.md](README-zh_CN.md#L1-L52)

---

## 核心设计哲学

Terway 的设计哲学可以概括为三个关键词：**原生性能**、**资源高效**、**云原生兼容**。

**原生性能**体现在 Terway 完全不依赖 VxLAN 等隧道封装技术。Pod 的 IP 地址直接来源于 VPC 的地址空间，报文通过 VPC 的弹性网卡直接转发。这意味着跨节点的 Pod 通信无需经历额外的封装/解封装过程，网络延迟与裸 ECS 节点之间的通信基本一致。同 ECS 内的 Pod 通信则通过节点内部转发完成，路径更短、延迟更低。

Sources: [README.md](README.md#L15-L19) · [docs/design.md](docs/design.md#L29-L31)

**资源高效**通过**池化机制**实现。阿里云网络资源（ENI、辅助 IP）的创建和释放需要调用 OpenAPI，存在延迟。Terway 在每个节点上维护了一个**资源池**，低于最小水位时自动补充、高于最大水位时自动释放，确保 Pod 创建时能快速获得 IP 地址而无需等待 API 调用完成。同时，Terway 通过 **DevicePlugin** 机制向 Kubernetes 调度器上报节点的网络资源配额，避免调度超出节点承载能力的 Pod。

Sources: [docs/design.md](docs/design.md#L88-L101)

**云原生兼容**意味着 Terway 遵循标准 CNI 接口规范，支持 Kubernetes 原生的 NetworkPolicy、Pod 注解（带宽限速 `kubernetes.io/ingress-bandwidth` / `kubernetes.io/egress-bandwidth`）以及标准的 Service 机制，用户无需学习额外的 API 或配置方式。

Sources: [docs/design.md](docs/design.md#L24-L28)

---

## 架构全景

Terway 的整体架构遵循业界标准的 **CNI 分层设计**，由三个核心组件协同工作：运行在每个节点上的 **Terway Daemon**、被 Kubelet 调用的 **Terway CNI Binary**，以及集群级别的 **Terway ControlPlane**。下方的架构图展示了这些组件之间的交互关系：

```mermaid
%%{init:{"theme":"base","themeVariables":{"primaryColor","#e6f3ff","primaryBorderColor","#4088d0","primaryTextColor","#1a1a2e","lineColor","#4088d0","tertiaryColor","#f0f7ff"}}}--
graph TB
    subgraph cluster["Kubernetes 集群"]
        subgraph cp["控制平面"]
            cp1["Terway ControlPlane<br/>CRD 管理 · 控制器 · Webhook"]
            api["阿里云 OpenAPI<br/>ECS · VPC"]
            crd["自定义资源 CRD<br/>PodENI · PodNetworking<br/>Node · NetworkInterface"]
        end

        subgraph node1["节点 A"]
            kubelet1["Kubelet"]
            bin1["Terway CNI Binary<br/>skel.PluginMain<br/>cmdAdd / cmdDel"]
            daemon1["Terway Daemon<br/>gRPC Server · ENI 资源管理<br/>IP 池化 · 策略路由配置"]
            pod1["Pod<br/>VPC IP · 独立网络栈"]
        end

        subgraph node2["节点 B"]
            kubelet2["Kubelet"]
            bin2["Terway CNI Binary"]
            daemon2["Terway Daemon"]
            pod2["Pod"]
        end
    end

    kubelet1 -- "1. 创建 Sandbox" --> bin1
    bin1 -- "2. gRPC: AllocIP" --> daemon1
    daemon1 -- "3. 分配 ENI/IP" --> api
    daemon1 -- "4. 返回网络配置" --> bin1
    bin1 -- "5. 配置网络命名空间" --> pod1
    cp1 -- "管理 CRD" --> crd
    cp1 -- "调用 OpenAPI" --> api
    daemon1 -- "同步资源状态" --> crd
    pod1 -- "VPC 直通" --> pod2
```

上图展示了 Terway 处理 Pod 网络的完整流程。当一个 Pod 被调度到某个节点时，Kubelet 首先创建 Sandbox 容器来持有网络命名空间，然后通过 CNI 接口调用 Terway Binary。Binary 并不直接处理资源分配，而是通过 Unix Domain Socket 上的 **gRPC 调用**将请求转发给同节点的 Terway Daemon。Daemon 负责调用阿里云 OpenAPI 分配 ENI 或辅助 IP，配置策略路由和网络连通性，最后将结果返回给 Binary，由其完成容器网络命名空间的最终配置。

Sources: [cmd/terway/main.go](cmd/terway/main.go#L44-L76) · [plugin/terway/cni.go](plugin/terway/cni.go#L41-L43) · [daemon/server.go](daemon/server.go#L73-L143) · [docs/design.md](docs/design.md#L38-L53)

---

## 三大核心组件

### Terway Daemon——节点级网络资源管家

Terway Daemon 以 **DaemonSet** 方式运行在每个 Kubernetes 节点上，是节点级网络资源管理的核心。它的主要职责包括：

- **gRPC 服务端**：监听 Unix Domain Socket `/var/run/eni/eni.socket`，为 CNI Binary 提供 `AllocIP`、`ReleaseIP`、`GetIPInfo` 等 RPC 接口
- **ENI 资源管理**：通过阿里云 OpenAPI 创建/释放弹性网卡和辅助 IP，维护本地资源池
- **网络配置**：配置策略路由、Veth pair、IPVlan 子接口等数据路径，打通 Pod 与 VPC 的网络连通性
- **资源回收**：定期执行垃圾回收（每 5 分钟），清理残留的网络资源和配置
- **监控指标**：通过 Debug Server 暴露 Prometheus 指标（RPC 延迟、资源池状态等）

Sources: [daemon/server.go](daemon/server.go#L37-L143) · [daemon/daemon.go](daemon/daemon.go#L63-L100)

### Terway CNI Binary——Kubelet 的网络代理

Terway CNI Binary 是一个符合标准 CNI 规范的可执行文件，被 Kubelet 直接调用。它本身不执行复杂的资源管理逻辑，而是充当 **Kubelet 与 Daemon 之间的桥梁**。Binary 支持三个标准 CNI 操作：`cmdAdd`（创建 Pod 网络时调用）、`cmdDel`（删除 Pod 网络时调用）和 `cmdCheck`（检查 Pod 网络状态）。Binary 接收到请求后，通过 gRPC 与 Daemon 通信获取网络资源信息，然后根据返回的配置（如 Veth pair、IPVlan、策略路由等）在容器网络命名空间中完成最终的网络设备创建和路由配置。

Sources: [plugin/terway/cni.go](plugin/terway/cni.go#L40-L99) · [docs/design.md](docs/design.md#L44-L53)

### Terway ControlPlane——集群级控制器

Terway ControlPlane 是集群级别的控制平面组件，负责 CRD 管理、多类型控制器协调和 Webhook 准入控制。它管理五种自定义资源（CRD）和七种控制器，具体如下：

| CRD 名称 | 用途 |
|---|---|
| `PodENI` | 将 Pod 与底层弹性网卡资源关联，支持独立安全组配置 |
| `PodNetworking` | 定义 Pod 网络配置模板（网络模式、安全组、vSwitch 等） |
| `Node` | 记录节点维度的网络状态与资源信息 |
| `NodeRuntime` | 管理节点运行时层面的网络配置 |
| `NetworkInterface` | 管理弹性网卡的全生命周期 |

| 控制器 | 职责 |
|---|---|
| ENI 控制器 | 管理弹性网卡的创建、绑定和状态同步 |
| Multi-IP Node 控制器 | 节点维度的多 IP 资源协调 |
| Multi-IP Pod 控制器 | Pod 维度的多 IP 分配与绑定 |
| Node 控制器 | 节点网络状态管理 |
| Pod 控制器 | Pod 网络资源的生命周期管理 |
| PodENI 控制器 | Pod 与 ENI 资源的关联管理 |
| PodNetworking 控制器 | Pod 网络配置模板的渲染与分发 |

Sources: [cmd/terway-controlplane/terway-controlplane.go](cmd/terway-controlplane/terway-controlplane.go#L1-L200) · [pkg/apis/crds/register.go](pkg/apis/crds/register.go#L21-L29) · [pkg/controller/all/all.go](pkg/controller/all/all.go#L22-L29)

---

## 网络模式总览

Terway 支持多种网络模式，以适应不同规模和性能要求的场景。网络模式决定了 Pod 如何获得 IP 地址、报文如何在节点之间转发，以及 Pod 的网络连通路径。

| 网络模式 | IP 来源 | 网段关系 | 数据路径技术 | 性能特征 | 状态 |
|---|---|---|---|---|---|
| **ENI 多 IP** | ENI 辅助 IP | Pod 与节点同网段 | Veth + 策略路由 或 IPVlan L2 | 高性能，高密度 | ✅ 推荐 |
| **ENI 独占** | 整个 ENI 直通 Pod | Pod 与节点同网段 | ENI 直通 | 最高性能，密度受限于 ENI 数量 | ✅ 当前版本（通过节点池配置） |
| **VPC 路由** | 节点 PodCIDR | Pod 独立网段 | Veth + VPC 路由表 | 标准 Overlay 性能 | ⚠️ 已废弃 |

**ENI 多 IP 模式**是当前最推荐的模式。它利用了阿里云 ENI 支持多个辅助 IP 的特性——单个 ENI 根据实例规格可分配 6 至 20 个辅助 IP——大幅提升了 Pod 部署的规模和密度。在该模式下，Terway 支持两种数据路径实现：**Veth + 策略路由**（兼容性好，支持 3.10 内核）和 **IPVlan L2**（需要 4.2+ 内核，性能更优）。

Sources: [types/daemon/types.go](types/daemon/types.go#L9-L26) · [docs/design.md](docs/design.md#L55-L122)

---

## 技术特性一览

下表汇总了 Terway 支持的核心技术特性：

| 特性领域 | 具体能力 | 说明 |
|---|---|---|
| **网络性能** | eBPF 加速 | 使用 eBPF 技术加速协议栈，降低延迟、提升吞吐量 |
| **安全策略** | NetworkPolicy | 通过集成 Felix（Calico）或 Cilium 实现 Kubernetes NetworkPolicy |
| **安全策略** | 安全组 | Pod 维度的独立安全组配置，支持 Trunk 模式 |
| **IP 协议** | IPv4/IPv6 双栈 | 同时支持 IPv4 和 IPv6 协议栈 |
| **流量控制** | TC 带宽限速 | 通过 Pod 注解 `kubernetes.io/ingress-bandwidth` / `egress-bandwidth` 控制 |
| **高可用** | 资源池化 | 预热 ENI 和 IP 资源，水位自动调控，保障 Pod 快速获取 IP |
| **IP 保持** | 固定 IP | StatefulSet Pod 在更新过程中 IP 地址保持不变 |
| **多网卡** | Multi-Network | 支持一个 Pod 配置多个网络平面 |
| **RDMA** | eRDMA | 支持阿里云 eRDMA 网卡，提供高性能 RDMA 通信 |
| **异构计算** | 灵骏 (EFLO) | 支持阿里云智能计算灵骏平台 |
| **诊断工具** | Terway CLI | 提供资源映射、元数据查询与问题诊断能力 |
| **可观测** | Prometheus 指标 | 暴露 RPC 延迟、资源池状态、OpenAPI 延迟等指标 |

Sources: [README.md](README.md#L21-L34) · [daemon/server.go](daemon/server.go#L238-L251)

---

## 项目结构导览

Terway 采用经典的 Go 项目布局，核心代码按功能职责组织为以下主要目录：

| 目录 | 职责 |
|---|---|
| `cmd/terway/` | **Daemon 入口**——解析命令行参数，启动 Terway Daemon 进程 |
| `cmd/terway-cli/` | **诊断工具入口**——Terway CLI，用于资源映射、元数据查询与网络诊断 |
| `cmd/terway-controlplane/` | **控制平面入口**——启动 CRD 管理器、控制器和 Webhook |
| `daemon/` | **Daemon 核心逻辑**——gRPC 服务、IP 分配/释放、资源管理、配置加载 |
| `plugin/terway/` | **CNI Binary**——标准 CNI 接口实现（cmdAdd/cmdDel/cmdCheck） |
| `plugin/datapath/` | **数据路径逻辑**——策略路由、独占 ENI、IPVlan、VLAN 等网络配置 |
| `plugin/driver/` | **底层驱动**——Veth、IPVlan、VLAN、NIC、VF 等网络设备驱动 |
| `pkg/eni/` | **ENI 资源管理器**——IP 池化、水位控制、本地/远程 IPAM、Trunk 管理 |
| `pkg/controller/` | **控制器**——ENI、Multi-IP、Node、Pod 等全部控制器的实现 |
| `pkg/aliyun/` | **阿里云 API 封装**——ECS 客户端、凭证管理、元数据服务 |
| `pkg/apis/` | **CRD 定义**——PodENI、PodNetworking、Node、NetworkInterface 等 API 类型 |
| `rpc/` | **gRPC 协议**——Protobuf 定义（AllocIP、ReleaseIP、GetIPInfo 等） |
| `types/` | **共享类型**——全局常量、配置结构体、Daemon/ControlPlane 配置类型 |
| `policy/` | **网络策略补丁**——Cilium 和 Felix（Calico）的定制化补丁集 |
| `tests/` | **E2E 测试**——连通性验证、Prefix 测试、压力测试、升级测试 |

Sources: [cmd/terway/main.go](cmd/terway/main.go#L1-L76) · [plugin/terway/cni.go](plugin/terway/cni.go#L40-L43) · [pkg/apis/crds/register.go](pkg/apis/crds/register.go#L21-L29) · [rpc/rpc.proto](rpc/rpc.proto#L1-L20)

---

## gRPC 通信协议概览

Daemon 与 CNI Binary 之间的通信基于 **gRPC over Unix Domain Socket**，协议定义在 `rpc/rpc.proto` 中。核心接口包含四个 RPC 方法：

```protobuf
service TerwayBackend {
  rpc AllocIP (AllocIPRequest) returns (AllocIPReply) {}
  rpc ReleaseIP (ReleaseIPRequest) returns (ReleaseIPReply) {}
  rpc GetIPInfo (GetInfoRequest) returns (GetInfoReply) {}
  rpc RecordEvent (EventRequest) returns (EventReply) {}
}
```

- **AllocIP**：Pod 创建时调用，请求包含 Pod 名称、命名空间、容器 ID 和网络命名空间路径。返回值包含分配的 IP 地址、网关、子网 CIDR、ENI 信息（MAC、VLAN ID、是否 Trunk）以及带宽限制配置
- **ReleaseIP**：Pod 删除时调用，请求包含 Pod 标识和 IP 类型。Daemon 根据租期决定是否立即释放资源（支持固定 IP 场景）
- **GetIPInfo**：查询已缓存的 Pod 网络配置信息，用于 CNI Check 操作
- **RecordEvent**：向 Kubernetes 记录事件，支持 Node 级别和 Pod 级别的事件上报

Sources: [rpc/rpc.proto](rpc/rpc.proto#L6-L19) · [daemon/daemon.go](daemon/daemon.go#L107-L186)

---

## 技术栈与依赖

Terway 使用 **Go 1.24** 构建，核心依赖包括：

| 依赖 | 用途 |
|---|---|
| `containernetworking/cni` | CNI 规范接口定义与 skel 框架 |
| `containernetworking/plugins` | 标准 CNI 插件工具库 |
| `sigs.k8s.io/controller-runtime` | Kubernetes 控制器运行时框架（Envtest、Manager） |
| `google.golang.org/grpc` | gRPC 通信框架 |
| `alibabacloud-go/ecs-*` | 阿里云 ECS OpenAPI SDK |
| `alibabacloud-go/eflo-*` | 阿里云灵骏 EFLO SDK |
| `prometheus/client_golang` | Prometheus 监控指标导出 |
| `go.opentelemetry.io/otel` | 分布式追踪（OpenTelemetry） |
| `google/nftables` | Linux nftables 防火墙规则管理 |
| `agiledragon/gomonkey` | 单元测试函数打桩 |

构建系统基于 **Makefile**，支持 `make test-quick`（单元测试）、`make e2e-test`（E2E 测试）、`make manifests`（CRD 生成）和 `make generate`（代码生成）等标准目标。

Sources: [go.mod](go.mod#L1-L30) · [Makefile](Makefile#L1-L80)

---

## 版本与部署

Terway 当前版本为 **v1.13.0**，通过 Helm Chart 方式部署。ACK 托管集群和自建集群使用相同的代码版本，唯一的差异是 **Trunk 功能**在自建集群中不可用（依赖 ACK 的集群管理能力）。

部署架构遵循 Kubernetes 最佳实践：
- **Terway Daemon** 以 DaemonSet 形式运行，确保每个节点上有一个实例
- **Terway ControlPlane** 以 Deployment 形式运行，通过 leader election 保证高可用
- **CNI Binary** 随 DaemonSet 的 Init Container 安装到节点的 CNI 二进制目录

Sources: [charts/terway/Chart.yaml](charts/terway/Chart.yaml#L1-L7) · [README-zh_CN.md](README-zh_CN.md#L35-L38)

---

## 阅读路线建议

根据本文档目录结构，建议按以下顺序深入理解 Terway：

**第一步：快速上手**  
→ [快速开始：构建、测试与运行 Terway](2-kuai-su-kai-shi-gou-jian-ce-shi-yu-yun-xing-terway)  
→ [开发规范与贡献指南](3-kai-fa-gui-fan-yu-gong-xian-zhi-nan)

**第二步：理解架构**  
→ [整体架构设计：Daemon、CNI Binary 与控制平面的协作机制](4-zheng-ti-jia-gou-she-ji-daemon-cni-binary-yu-kong-zhi-ping-mian-de-xie-zuo-ji-zhi)  
→ [gRPC 通信协议：Daemon 与 CNI Binary 的接口定义](5-grpc-tong-xin-xie-yi-daemon-yu-cni-binary-de-jie-kou-ding-yi)

**第三步：深入网络模式与数据路径**  
→ [网络模式全解析：VPC、ENI、ENI 多 IP 与 Trunk 模式](6-wang-luo-mo-shi-quan-jie-xi-vpc-eni-eni-duo-ip-yu-trunk-mo-shi)  
→ [数据路径驱动层：Veth、IPVlan、VLAN、NIC 与 VF 驱动实现](7-shu-ju-lu-jing-qu-dong-ceng-veth-ipvlan-vlan-nic-yu-vf-qu-dong-shi-xian)