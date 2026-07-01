#!/usr/bin/env python3
"""Generate individual glossary term files from k8s-glossary.md terms.

Each term gets its own markdown file with comprehensive content following
the KUDIG dictionary entry template (annotations.md style).
"""

import os
from pathlib import Path

BASE = Path(__file__).resolve().parent.parent / "domain-17-system-foundation" / "topic-dictionary"

# ── Term definitions ──────────────────────────────────────────────
# Format: (filename, title_zh, title_en, category_dir, tags, content_dict)
# content_dict keys: overview, core_concepts, key_mechanisms, use_cases, best_practices, references, related

TERMS = []

def t(filename, title_zh, title_en, cat_dir, tags, overview, core, mechanism, use_cases_and_bp, refs="", related=""):
    TERMS.append({
        "filename": filename,
        "title_zh": title_zh,
        "title_en": title_en,
        "cat_dir": cat_dir,
        "tags": tags,
        "overview": overview,
        "core": core,
        "mechanism": mechanism,
        "use_cases_and_bp": use_cases_and_bp,
        "refs": refs,
        "related": related,
    })

# ═══════════════════════════════════════════════════════════════════
# 1. 架构与组件 — fundamentals/
# ═══════════════════════════════════════════════════════════════════

t("control-plane", "控制平面", "Control Plane", "fundamentals",
  ["k8s", "glossary", "control-plane", "architecture"],
  "控制平面（Control Plane）是 Kubernetes 集群的管理层，负责维护集群状态、处理 API 请求、执行调度和协调所有组件的工作。控制平面由一组核心组件构成，通常部署在专用节点上并采用高可用架构。",
  """### 控制平面的核心组件

- **kube-apiserver**：集群的唯一入口，提供 RESTful API，所有操作（包括用户请求、内部组件通信）都通过 API Server。
- **etcd**：分布式键值存储，保存集群的完整状态数据，是集群的"大脑"。
- **kube-scheduler**：负责将未调度的 Pod 分配到最合适的节点上。
- **kube-controller-manager**：运行一组控制器（如 Deployment Controller、ReplicaSet Controller），维护集群的期望状态。
- **cloud-controller-manager**：将云厂商特定的控制逻辑（节点管理、负载均衡器、路由）从核心控制平面中解耦。

### 高可用架构

生产环境中，控制平面通常采用多副本部署：
- **etcd 集群**：至少 3 个成员，支持多数派写入的容错。
- **多 API Server 实例**：前端配置负载均衡器。
- **Controller Manager / Scheduler**：通过 Leader Election 机制实现主备切换。""",
  """- 控制平面组件通过 [[Leases|Lease]] 对象实现领导者选举。
- API Server 支持水平扩展，通过负载均衡器对外提供服务。
- etcd 的 compaction 和 defragmentation 需要定期执行以保证性能。
- 控制平面节点通常添加 `node-role.kubernetes.io/control-plane` 标签并设置污点以阻止普通工作负载调度。""",
  """- 生产集群应至少部署 3 个控制平面节点以实现高可用。
- etcd 应与 API Server 分开部署或使用专用 SSD，避免 I/O 竞争。
- 定期备份 etcd 数据（使用 `etcdctl snapshot save`）。
- 使用 RBAC 严格控制对控制平面 API 的访问权限。
- 监控控制平面组件的健康状态和资源使用。""",
  "https://kubernetes.io/docs/concepts/architecture/",
  "[[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes-components|Kubernetes 组件]] | [[etcd|etcd]]")

t("kube-apiserver", "API Server", "kube-apiserver", "fundamentals",
  ["k8s", "glossary", "apiserver", "control-plane"],
  "kube-apiserver 是 Kubernetes 控制平面的核心组件，提供 RESTful API 作为集群所有交互的统一入口。用户、集群内部组件和外部工具都通过 API Server 来查询和操作集群状态。",
  """### 核心职责

- **API 网关**：所有请求（包括 kubectl、控制器、kubelet）都通过 API Server 的 HTTP API。
- **认证（Authentication）**：验证请求者身份（支持多种认证方式：证书、Bearer Token、OIDC 等）。
- **授权（Authorization）**：基于 RBAC、ABAC 或 Node 授权决定请求是否被允许。
- **准入控制（Admission Control）**：在对象持久化前执行验证和变更逻辑。
- **持久化**：通过 etcd 存储对象状态，支持 watch 机制实现变更通知。

### API 请求生命周期

```
请求 → 认证 → 授权 → Mutating Admission → Schema 验证 → Validating Admission → 持久化(etcd)
```""",
  """- API Server 支持 **聚合层（Aggregation Layer）**，允许通过 APIService 注册自定义 API Server。
- 支持 **API Priority and Fairness**，对不同类别的请求进行流量控制。
- 所有对象的 watch 事件通过 API Server 分发给订阅者。
- 支持 OpenAPI v3 规范描述所有 API 端点。""",
  """- 使用 `--audit-log-path` 启用审计日志，记录所有 API 请求。
- 配置 `--max-requests-inflight` 防止 API Server 过载。
- 生产环境中部署多个 API Server 实例并使用负载均衡器。
- 启用 `--encryption-provider-config` 加密 etcd 中的 Secret 数据。""",
  "https://kubernetes.io/docs/reference/command-line-tools-reference/kube-apiserver/",
  "[[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api|Kubernetes API]]")

t("kube-scheduler", "调度器", "kube-scheduler", "fundamentals",
  ["k8s", "glossary", "scheduler", "control-plane"],
  "kube-scheduler 是 Kubernetes 控制平面的默认调度器，负责将新创建的 Pod 分配到最合适的节点上运行。调度决策基于资源需求、亲和性约束、污点容忍等多种因素。",
  """### 调度流程

调度器采用两阶段流程：

1. **过滤（Filtering）**：排除不满足 Pod 约束的节点（资源不足、污点不匹配、节点亲和性不满足等）。
2. **打分（Scoring）**：对通过过滤的节点评分，选择得分最高的节点。

### 调度框架

kube-scheduler 基于可插拔的 Scheduling Framework 架构，支持自定义 Filter、Score、Bind 等扩展点。""",
  """- 内置 Filter 插件：NodeResourcesFit、NodeAffinity、TaintToleration、PodTopologySpread 等。
- 内置 Score 插件：InterPodAffinity、LeastRequestedPower、BalancedAllocation 等。
- 支持 **调度器扩展配置（KubeSchedulerConfiguration）** 自定义调度行为。
- 支持多调度器共存，通过 `schedulerName` 字段指定 Pod 使用的调度器。""",
  """- 为关键工作负载配置 Pod Priority，确保高优先级 Pod 能够被调度。
- 使用 topologySpreadConstraints 实现 Pod 跨可用区均匀分布。
- 在大型集群中调整 `percentageOfNodesToScore` 平衡调度速度和准确性。
- 监控 pending Pod 数量和调度延迟指标。""",
  "https://kubernetes.io/docs/reference/command-line-tools-reference/kube-scheduler/",
  "[[references/scheduling-terms|调度术语参考]]")

t("kube-controller-manager", "控制器管理器", "kube-controller-manager", "fundamentals",
  ["k8s", "glossary", "controller-manager", "control-plane"],
  "kube-controller-manager 是 Kubernetes 控制平面中运行各种控制器的组件。每个控制器都是一个独立的控制循环，持续比较集群的当前状态与期望状态，并在偏差时采取纠正措施。",
  """### 内置控制器

kube-controller-manager 运行的核心控制器包括：

- **Node Controller**：监控节点状态，处理节点加入/离开/故障。
- **Replication Controller**：维护 ReplicaSet 中 Pod 的副本数。
- **Deployment Controller**：管理 Deployment 的滚动更新和回滚。
- **ServiceAccount Controller**：为新命名空间创建默认 ServiceAccount。
- **Namespace Controller**：处理命名空间的删除及其资源的级联清理。
- **Job Controller**：管理 Job 的执行。
- **EndpointSlice Controller**：维护 Service 和 Pod 之间的映射关系。

### 工作原理

每个控制器独立运行一个 Reconcile 循环：读取当前状态 → 计算差异 → 执行纠正操作 → 更新状态。""",
  """- 控制器通过 Informer 机制缓存集群状态，减少对 API Server 的压力。
- 支持水平扩展：通过 Leader Election 确保同一时间只有一个活跃的 Controller Manager。
- 控制器之间松耦合，各自独立运行，互不干扰。""",
  """- 监控控制器的 reconcile 延迟和错误率。
- 调整 `--concurrent-*-syncs` 参数控制控制器的并发度。
- 定期检查控制器日志，排查 reconcile 失败的资源。""",
  "https://kubernetes.io/docs/reference/command-line-tools-reference/kube-controller-manager/",
  "[[domain-17-system-foundation/topic-dictionary/fundamentals/controllers|控制器]]")

t("cloud-controller-manager", "云控制器管理器", "cloud-controller-manager", "fundamentals",
  ["k8s", "glossary", "cloud-controller-manager", "control-plane"],
  "Cloud Controller Manager（CCM）是 Kubernetes 控制平面的一个可选组件，它将云厂商特定的控制逻辑嵌入到 Kubernetes 中。CCM 使集群能够与云提供商的 API 交互，实现节点管理、负载均衡器创建和路由配置等云原生能力。",
  """### 核心控制器

CCM 通常包含以下云相关的控制器：

- **Node Controller**：管理节点生命周期，与云平台 API 同步节点状态。
- **Route Controller**：配置云平台网络路由，实现 Pod 网络互通。
- **Service Controller**：创建和管理云厂商负载均衡器（LoadBalancer 类型 Service）。
- **Cloud Node Lifecycle Controller**：处理节点删除时的云资源清理。

### 解耦设计

CCM 将云相关逻辑从 kube-controller-manager 中分离出来，使核心 Kubernetes 项目与云厂商实现解耦。""",
  """- CCM 通过 Cloud Provider Interface 与云平台 API 交互。
- 支持 out-of-tree 模式（推荐）和 in-tree 模式（已弃用）。
- Kubernetes v1.28+ 推荐使用 external cloud provider 方式部署 CCM。""",
  """- 在云环境中部署集群时，应使用对应云厂商的 CCM 实现。
- CCM 需要适当的 IAM 权限来管理云资源。
- 监控 CCM 的 Service 同步状态，确保 LoadBalancer 正常创建。""",
  "https://kubernetes.io/docs/concepts/architecture/cloud-controller/",
  "[[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes-components|Kubernetes 组件]]")

t("etcd", "etcd", "etcd", "fundamentals",
  ["k8s", "glossary", "etcd", "control-plane", "storage"],
  "etcd 是一个高可用的分布式键值存储系统，是 Kubernetes 集群的核心数据存储。集群的所有状态信息（包括 Pod、Service、ConfigMap、Secret 等所有资源对象）都持久化在 etcd 中。",
  """### 核心特性

- **强一致性**：基于 Raft 共识算法，保证所有读取返回最新数据。
- **Watch 机制**：支持对 key 或 key 前缀的变更监听，是 Kubernetes 事件驱动架构的基础。
- **事务支持**：支持多 key 的原子操作。
- **MVCC 存储**：使用多版本并发控制，保留 key 的历史版本。

### 在 Kubernetes 中的角色

API Server 是唯一直接与 etcd 通信的组件。所有 Kubernetes 对象通过 API Server 读写 etcd。etcd 中的数据变更触发控制器和 Informer 的响应。""",
  """- etcd 集群推荐至少 3 个成员以实现容错（可容忍 1 个节点故障）。
- 5 个成员的集群可容忍 2 个节点故障，适合大规模生产环境。
- 需要定期执行 compaction（压缩历史版本）和 defragmentation（回收空间）。
- 备份策略：定期执行 `etcdctl snapshot save` 并存储在异地。""",
  """- **性能**：使用 SSD 存储，避免网络延迟；大规模集群考虑独立 etcd 集群。
- **安全**：启用 TLS 加密所有 etcd 通信（peer 和 client）。
- **备份**：实施自动化备份策略，定期验证备份可恢复性。
- **监控**：关注 WAL fsync 延迟、backend commit 延迟等关键指标。
- **版本**：Kubernetes 对 etcd 版本有严格要求，参见兼容性矩阵。""",
  "https://etcd.io/docs/",
  "[[domain-17-system-foundation/topic-dictionary/fundamentals/storage-versions|存储版本]]")

t("node", "节点", "Node", "fundamentals",
  ["k8s", "glossary", "node", "kubelet"],
  "Node（节点）是 Kubernetes 集群中的工作机器，可以是物理机或虚拟机。节点上运行 kubelet 和容器运行时，负责执行用户的工作负载（Pod）。",
  """### 节点组件

每个节点运行以下核心组件：

- **kubelet**：节点代理，接收来自 API Server 的 Pod 规格，确保 Pod 中的容器正常运行。
- **kube-proxy**：网络代理，维护节点上的网络规则，实现 Service 的负载均衡。
- **Container Runtime**：容器运行时（如 containerd），负责拉取镜像和运行容器。

### 节点状态

节点通过以下状态条件报告健康状况：
- `Ready`：节点就绪，可以接受 Pod。
- `MemoryPressure`：内存压力。
- `DiskPressure`：磁盘压力。
- `PIDPressure`：PID 资源压力。
- `NetworkUnavailable`：网络不可用。""",
  """- 节点通过 Lease 对象向 API Server 发送心跳（默认每 10 秒）。
- 节点注册（Registration）可以是自动的（kubelet 自注册）或由 Controller 创建。
- 节点可以通过标签（Labels）和污点（Taints）进行分类和调度控制。""",
  """- 为节点设置合理的标签，便于使用 nodeSelector 或 nodeAffinity 进行调度。
- 配置 kubelet 资源预留（`--kube-reserved` 和 `--system-reserved`）。
- 监控节点资源使用率和 Pod 容量。
- 使用 Node Problem Detector 自动检测和报告节点异常。""",
  "https://kubernetes.io/docs/concepts/architecture/nodes/",
  "[[domain-17-system-foundation/topic-dictionary/fundamentals/nodes|节点]]")

t("kubelet", "kubelet", "kubelet", "fundamentals",
  ["k8s", "glossary", "kubelet", "node"],
  "kubelet 是运行在每个 Kubernetes 节点上的代理程序。它确保容器按照 PodSpec 中描述的规格运行，是节点上最重要的组件。",
  """### 核心职责

- **Pod 管理**：根据 API Server 下发的 PodSpec 创建、更新和删除容器。
- **健康检查**：执行 Liveness、Readiness 和 Startup 探针。
- **资源监控**：上报节点资源使用情况和 Pod 指标。
- **日志收集**：管理容器日志文件。
- **Volume 管理**：挂载和卸载 Volume。
- **镜像管理**：通过 CRI 拉取容器镜像。

### 通信模式

kubelet 通过 API Server 获取 Pod 配置，同时向 API Server 报告节点状态和 Pod 状态。kubelet 还暴露 `/healthz`、`/metrics` 等端点供监控使用。""",
  """- kubelet 通过 CRI（Container Runtime Interface）与容器运行时通信。
- 支持 Static Pod（通过 manifest 目录或 URL 直接创建，不经过 API Server）。
- kubelet 的 `--config` 参数通过 KubeletConfiguration 进行配置。
- 支持 cgroup v1 和 cgroup v2。""",
  """- 合理配置 `--max-pods` 限制单节点 Pod 数量。
- 设置 `--image-gc-high-threshold` 和 `--image-gc-low-threshold` 管理镜像垃圾回收。
- 配置 `--eviction-hard` 和 `--eviction-soft` 防止节点资源耗尽。
- 定期升级 kubelet 版本，保持与 API Server 的兼容性。""",
  "https://kubernetes.io/docs/reference/command-line-tools-reference/kubelet/",
  "[[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes-components|Kubernetes 组件]]")

t("kube-proxy", "kube-proxy", "kube-proxy", "fundamentals",
  ["k8s", "glossary", "kube-proxy", "networking"],
  "kube-proxy 是运行在每个节点上的网络代理，负责维护节点上的网络规则以实现 Service 的负载均衡功能。它是 Kubernetes Service 抽象的底层实现。",
  """### 代理模式

kube-proxy 支持多种工作模式：

- **iptables 模式**（默认）：使用 iptables 规则实现流量转发，性能好但规则数随 Service 增长线性增加。
- **IPVS 模式**：使用 Linux IPVS（IP Virtual Server）内核模块，规则数 O(1) 复杂度，适合大规模集群。
- **nftables 模式**（v1.31+ Alpha）：使用 nftables 替代 iptables，提供更好的性能和可维护性。

### 工作原理

kube-proxy 监听 Service 和 Endpoints/EndpointSlice 的变化，自动更新节点上的转发规则。""",
  """- Service 的 ClusterIP 流量通过 kube-proxy 分发的规则转发到后端 Pod。
- IPVS 模式需要节点安装 `ipvsadm` 等工具并加载相应内核模块。
- kube-proxy 支持会话保持（SessionAffinity）。""",
  """- 大规模集群（>1000 Service）建议使用 IPVS 模式。
- 监控 kube-proxy 的同步延迟和错误。
- 使用 `kube-proxy --proxy-mode=ipvs` 切换到 IPVS 模式时需确保内核模块就绪。
- 考虑使用 eBPF 替代方案（如 Cilium）获得更好的性能。""",
  "https://kubernetes.io/docs/reference/command-line-tools-reference/kube-proxy/",
  "[[domain-17-system-foundation/topic-dictionary/networking/service|Service]]")

t("container-runtime", "容器运行时", "Container Runtime", "fundamentals",
  ["k8s", "glossary", "container-runtime", "containerd", "cri"],
  "容器运行时（Container Runtime）是负责在节点上运行和管理容器的软件。它实现了 Kubernetes 的 CRI（Container Runtime Interface）接口，处理镜像拉取、容器创建和网络配置等操作。",
  """### 主要运行时

- **containerd**：从 Docker 中拆分出的轻量级运行时，是当前 Kubernetes 的默认选择。
- **CRI-O**：专为 Kubernetes 设计的运行时，实现了最小化的 CRI 接口。
- **Docker Engine**：通过 dockershim（已移除）或 cri-dockerd 适配器支持 CRI。

### CRI 接口

Kubernetes 通过 CRI（Container Runtime Interface）与容器运行时通信。CRI 定义了一组 gRPC 接口：
- `RuntimeService`：管理容器生命周期。
- `ImageService`：管理容器镜像。""",
  """- 容器运行时分为高层级运行时（管理容器生命周期）和低层级运行时（如 runc，实际执行容器）。
- containerd 支持 OCI 标准镜像格式。
- CRI 使用 Unix domain socket 通信，默认路径 `/run/containerd/containerd.sock`。""",
  """- 生产环境推荐使用 containerd 或 CRI-O。
- Docker 在 K8s v1.24 后不再直接支持，需使用 cri-dockerd 适配器。
- 配置镜像拉取超时和重试策略。
- 启用容器的 seccomp 和 AppArmor 安全配置。""",
  "https://kubernetes.io/docs/setup/production-environment/container-runtimes/",
  "[[entities/containerd|containerd]] | [[entities/cri-o|CRI-O]]")

# ═══════════════════════════════════════════════════════════════════
# 2. 核心资源 — 工作负载 workloads/
# ═══════════════════════════════════════════════════════════════════

t("pod", "Pod", "Pod", "workloads",
  ["k8s", "glossary", "pod", "workload"],
  "Pod 是 Kubernetes 的最小调度单元和计算单元。一个 Pod 封装了一个或多个紧密相关的容器，共享网络和存储资源，并作为一个整体被调度和管理。",
  """### 核心特性

- **共享网络**：Pod 内的容器共享同一个网络命名空间（相同的 IP 和端口空间）。
- **共享存储**：Pod 可以挂载 Volume，容器间共享数据。
- **生命周期**：Pod 经历 Pending → Running → Succeeded/Failed 的生命周期。

### Pod 类型

- **单容器 Pod**：最常见的模式，一个 Pod 运行一个容器。
- **多容器 Pod（Sidecar 模式）**：主容器 + 辅助容器（日志收集、服务网格代理等）。
- **Init Container**：在主容器启动前运行的初始化容器。
- **Static Pod**：由 kubelet 直接管理，不经过 API Server。""",
  """- Pod 的 `restartPolicy` 控制容器重启策略（Always/OnFailure/Never）。
- Pod 的 QoS 类别由资源 Request 和 Limit 决定。
- Pod 可以通过 OwnerReference 关联到上层控制器（Deployment/ReplicaSet 等）。
- Pod Disruption Budget（PDB）限制同时被驱逐的 Pod 数量。""",
  """- 尽量保持一个 Pod 运行一个主容器（single container per Pod 原则）。
- 为容器设置资源 Request 和 Limit。
- 配置 Liveness 和 Readiness 探针确保健康检查。
- 使用 `terminationGracePeriodSeconds` 实现优雅关闭。""",
  "https://kubernetes.io/docs/concepts/workloads/pods/",
  "[[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes-components|Kubernetes 组件]]")

t("deployment", "Deployment", "Deployment", "workloads",
  ["k8s", "glossary", "deployment", "workload"],
  "Deployment 是 Kubernetes 中管理无状态应用的核心工作负载控制器。它通过管理 ReplicaSet 来维护期望的 Pod 副本数和版本，支持声明式更新、滚动发布和回滚。",
  """### 核心能力

- **声明式更新**：修改 Pod 模板后，Deployment 自动执行滚动更新。
- **版本管理**：每次更新创建新的 ReplicaSet，保留历史记录支持回滚。
- **滚动更新策略**：通过 `maxSurge` 和 `maxUnavailable` 控制更新节奏。
- **扩缩容**：修改 `replicas` 字段即可调整副本数。

### 更新流程

```
修改 Pod 模板 → 创建新 ReplicaSet → 逐步增加新 Pod → 逐步减少旧 Pod → 完成更新
```""",
  """- `strategy.type: RollingUpdate` 是最常用的更新策略，保证零停机。
- `strategy.type: Recreate` 先停掉所有旧 Pod 再创建新 Pod，适用于不兼容版本升级。
- `revisionHistoryLimit` 控制保留的历史 ReplicaSet 数量（默认 10）。
- `minReadySeconds` 确保新 Pod 就绪后才继续更新。""",
  """- 生产环境始终使用 Deployment 而非裸 ReplicaSet 管理应用。
- 设置合理的 `maxSurge` 和 `maxUnavailable`（推荐 25%/25%）。
- 使用 `kubectl rollout status` 监控更新进度。
- 配置 Pod 的反亲和性，确保副本分布在不同的节点/可用区。""",
  "https://kubernetes.io/docs/concepts/workloads/controllers/deployment/",
  "[[entities/deployment|Deployment]]")

t("statefulset", "有状态副本集", "StatefulSet", "workloads",
  ["k8s", "glossary", "statefulset", "workload", "storage"],
  "StatefulSet 是 Kubernetes 中管理有状态应用的工作负载控制器。与 Deployment 不同，StatefulSet 为每个 Pod 提供稳定的网络标识、存储和有序的部署/扩缩容/删除顺序。",
  """### 核心特性

- **稳定的网络标识**：每个 Pod 有固定的名称（如 `mysql-0`, `mysql-1`）和对应的 Headless Service DNS。
- **稳定的存储**：每个 Pod 通过 VolumeClaimTemplate 绑定独立的 PVC，Pod 重启/重调度后仍保持绑定。
- **有序操作**：Pod 按序号顺序创建（0→N-1），逆序删除（N-1→0）。
- **有序更新**：RollingUpdate 从高序号向低序号逆序更新。

### 与 Deployment 的对比

| 特性 | Deployment | StatefulSet |
|------|-----------|-------------|
| Pod 标识 | 随机名称 | 固定有序名称 |
| 存储 | 共享或无 | 每 Pod 独立 PVC |
| 创建顺序 | 并行 | 有序（0→N-1） |
| 适用场景 | 无状态应用 | 有状态应用 |""",
  """- `podManagementPolicy: Parallel` 可让 Pod 并行创建/删除。
- `serviceName` 必须指向一个 Headless Service。
- 删除 StatefulSet 不会自动删除关联的 PVC（保护数据安全）。""",
  """- 数据库（MySQL、PostgreSQL）、消息队列（Kafka）、分布式存储等使用 StatefulSet。
- 为每个 Pod 配置独立的 PVC 和 VolumeClaimTemplate。
- 使用 `partition` 字段实现金丝雀更新。
- 考虑使用 Operator 模式管理复杂的有状态应用生命周期。""",
  "https://kubernetes.io/docs/concepts/workloads/controllers/statefulset/",
  "")

t("daemonset", "守护进程集", "DaemonSet", "workloads",
  ["k8s", "glossary", "daemonset", "workload"],
  "DaemonSet 确保集群中每个（或特定）节点上运行一个 Pod 副本。当节点加入集群时自动创建 Pod，节点离开时自动删除 Pod。常用于部署系统级守护进程。",
  """### 典型用途

- **日志收集**：在每个节点上运行 Fluentd/Fluent Bit 收集容器日志。
- **节点监控**：运行 Node Exporter、Datadog Agent 等监控代理。
- **网络插件**：部署 kube-proxy、Cilium、Calico 等网络组件。
- **存储守护进程**：运行 CSI 节点插件。

### 调度方式

DaemonSet 的 Pod 调度由 DaemonSet Controller 负责（K8s v1.12+），而非默认调度器。DaemonSet 使用 nodeSelector 和 tolerations 控制在哪些节点上运行。""",
  """- 更新策略支持 `RollingUpdate` 和 `OnDelete`。
- `maxUnavailable` 控制同时更新的节点数。
- 使用 `updateStrategy.rollingUpdate.maxSurge` 允许在更新时临时多运行一个 Pod。
- DaemonSet Pod 通常需要 tolerations 以容忍控制平面节点的污点。""",
  """- 系统级组件（kube-proxy、CNI 插件、日志收集器）优先使用 DaemonSet。
- 配置 `nodeSelector` 限制 DaemonSet 只在特定节点运行。
- 监控 DaemonSet 的 `Desired`/`Ready`/`Available` 状态。""",
  "https://kubernetes.io/docs/concepts/workloads/controllers/daemonset/",
  "[[entities/daemonset|DaemonSet]]")

t("job", "任务", "Job", "workloads",
  ["k8s", "glossary", "job", "workload"],
  "Job 是 Kubernetes 中用于运行一次性任务的工作负载控制器。它创建一个或多个 Pod 并确保指定数量的 Pod 成功完成后终止。",
  """### 核心特性

- **一次性执行**：Pod 成功完成后不会重启。
- **完成保证**：Job 确保 `completions` 指定数量的 Pod 成功执行。
- **并行控制**：`parallelism` 控制同时运行的 Pod 数量。
- **失败处理**：`backoffLimit` 限制最大重试次数。

### 执行模式

- **Non-parallel Job**：`completions=1`，一个 Pod 成功即完成。
- **Fixed completion count**：`completions=N`，需要 N 个 Pod 成功。
- **Work queue**：`completions` 未设置，由 Pod 自行协调。""",
  """- `activeDeadlineSeconds` 限制 Job 的最大运行时间。
- `ttlSecondsAfterFinished` 自动清理已完成的 Job。
- Job 支持 `suspend` 字段暂停执行。
- 失败后的重试间隔按指数退避（10s → 20s → 40s...最大 6min）。""",
  """- 数据迁移、批处理、机器学习训练等一次性任务使用 Job。
- 设置 `backoffLimit` 防止无限重试。
- 使用 `activeDeadlineSeconds` 避免任务卡死。
- 大规模批处理考虑使用 Argo Workflows 或 Tekton。""",
  "https://kubernetes.io/docs/concepts/workloads/controllers/job/",
  "")

t("cronjob", "定时任务", "CronJob", "workloads",
  ["k8s", "glossary", "cronjob", "workload"],
  "CronJob 按 Cron 时间表定期创建 Job，实现定时任务的自动化执行。类似于 Linux 的 crontab，但运行在 Kubernetes 集群中。",
  """### 核心特性

- **Cron 调度**：使用标准 Cron 表达式定义执行计划（分 时 日 月 周）。
- **并发控制**：`concurrencyPolicy` 控制多个 Job 实例的并发行为。
  - `Allow`（默认）：允许并发。
  - `Forbid`：跳过新执行如果前一个仍在运行。
  - `Replace`：替换当前运行的 Job。
- **历史管理**：`successfulJobsHistoryLimit` 和 `failedJobsHistoryLimit` 控制保留的历史 Job 数量。

### Cron 表达式

```
# ┌───────────── 分钟 (0 - 59)
# │ ┌───────────── 小时 (0 - 23)
# │ │ ┌───────────── 日 (1 - 31)
# │ │ │ ┌───────────── 月 (1 - 12)
# │ │ │ │ ┌───────────── 星期 (0 - 6)
# │ │ │ │ │
# * * * * *
```""",
  """- `startingDeadlineSeconds` 控制错过调度时间的容忍窗口。
- `suspend: true` 暂停 CronJob 的所有后续调度。
- 时区支持（v1.24+）：通过 `timeZone` 字段指定。
- CronJob Controller 每 10 秒检查一次是否有需要执行的 CronJob。""",
  """- 数据库备份、日志清理、报表生成等定期任务使用 CronJob。
- 设置 `successfulJobsHistoryLimit: 3` 避免历史 Job 堆积。
- 使用 `Forbid` 策略防止任务重叠执行。
- 监控 CronJob 的最后执行时间和成功率。""",
  "https://kubernetes.io/docs/concepts/workloads/controllers/cron-jobs/",
  "[[entities/cronjob|CronJob]]")

t("replicaset", "副本集", "ReplicaSet", "workloads",
  ["k8s", "glossary", "replicaset", "workload"],
  "ReplicaSet 确保在任意时刻运行指定数量的 Pod 副本。它通过标签选择器匹配 Pod，并维护期望的副本数。通常不直接使用，而是通过 Deployment 间接管理。",
  """### 核心功能

- **副本维护**：监控 Pod 数量，自动创建/删除 Pod 以维持 `replicas` 指定的数量。
- **标签选择器**：使用 `selector` 字段标识哪些 Pod 属于该 ReplicaSet。
- **Pod 模板**：使用 `template` 字段定义新 Pod 的规格。

### 与 Deployment 的关系

Deployment 通过创建和管理 ReplicaSet 来实现声明式更新。每次更新 Deployment 的 Pod 模板时，会创建新的 ReplicaSet 并逐步缩减旧的。""",
  """- ReplicaSet 只负责维护副本数量，不支持声明式更新。
- 同一个 Pod 不能同时被多个 ReplicaSet 管理。
- `kubectl rollout` 命令操作的是 Deployment，而非 ReplicaSet。""",
  """- 生产环境应使用 Deployment 管理应用，而非直接使用 ReplicaSet。
- Deployment 的 `revisionHistoryLimit` 控制保留的旧 ReplicaSet 数量。
- 不要手动修改 Deployment 管理的 ReplicaSet。""",
  "https://kubernetes.io/docs/concepts/workloads/controllers/replicaset/",
  "")

# ═══════════════════════════════════════════════════════════════════
# 3. 核心资源 — 服务发现与网络 networking/
# ═══════════════════════════════════════════════════════════════════

t("service", "服务", "Service", "networking",
  ["k8s", "glossary", "service", "networking"],
  "Service 是 Kubernetes 中为一组 Pod 提供稳定网络访问入口的抽象资源。它通过标签选择器找到后端 Pod，并提供负载均衡和稳定的 ClusterIP/DNS 名称。",
  """### Service 类型

- **ClusterIP**（默认）：仅集群内部可访问的虚拟 IP。
- **NodePort**：在每个节点上暴露一个端口（30000-32767），外部可通过 `NodeIP:NodePort` 访问。
- **LoadBalancer**：在云环境中自动创建外部负载均衡器。
- **ExternalName**：将 Service 映射到外部 DNS 名称（CNAME 记录）。

### 核心概念

- **Selector**：通过标签选择后端 Pod。
- **Endpoints/EndpointSlice**：实际的后端 Pod IP 和端口列表。
- **Headless Service**：`clusterIP: None`，不分配 ClusterIP，DNS 直接返回后端 Pod IP。""",
  """- Service 的负载均衡由 kube-proxy 实现（iptables/IPVS 模式）。
- `sessionAffinity: ClientIP` 实现基于客户端 IP 的会话保持。
- `externalTrafficPolicy: Local` 保留客户端源 IP。
- `internalTrafficPolicy: Local` 限制流量只在同节点的 Pod 间路由。""",
  """- 使用 Headless Service + StatefulSet 实现有状态应用的稳定寻址。
- 对于 HTTP 服务，优先使用 Ingress/Gateway API 而非 NodePort/LoadBalancer。
- 监控 EndpointSlice 同步状态，确保后端 Pod 及时注册。
- 大规模场景考虑使用 IPVS 模式替代 iptables。""",
  "https://kubernetes.io/docs/concepts/services-networking/service/",
  "[[domain-17-system-foundation/topic-dictionary/networking/service|Service]]")

t("ingress", "入口", "Ingress", "networking",
  ["k8s", "glossary", "ingress", "networking", "http"],
  "Ingress 是 Kubernetes 中管理集群外部 HTTP/HTTPS 访问的 API 资源。它定义了路由规则，将外部请求根据主机名和路径转发到集群内部的不同 Service。",
  """### 核心功能

- **HTTP 路由**：基于 Host 和 Path 将流量路由到不同后端 Service。
- **TLS 终止**：配置 TLS 证书实现 HTTPS 访问。
- **路径重写**：通过注解实现路径重写和自定义路由逻辑。

### Ingress 资源示例

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
spec:
  tls:
  - hosts: [example.com]
    secretName: tls-secret
  rules:
  - host: example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 80
```""",
  """- Ingress 本身只是规则定义，需要 Ingress Controller 来实现实际的路由逻辑。
- 支持 pathType: `Exact`、`Prefix`、`ImplementationSpecific`。
- 多个 Ingress 规则可以共享同一个 Ingress Controller。
- Ingress 正在被更强大的 Gateway API 逐步替代。""",
  """- 使用 Nginx Ingress Controller 或 Traefik 等成熟的 Ingress Controller。
- 配置 TLS 终止确保 HTTPS 安全访问。
- 对于复杂路由需求，考虑迁移到 Gateway API。
- 监控 Ingress Controller 的请求延迟和错误率。""",
  "https://kubernetes.io/docs/concepts/services-networking/ingress/",
  "[[domain-17-system-foundation/topic-dictionary/networking/ingress|Ingress]]")

t("endpoints", "端点", "Endpoints", "networking",
  ["k8s", "glossary", "endpoints", "networking"],
  "Endpoints 是 Service 后端 Pod 的 IP 地址和端口组合。当 Service 使用 selector 时，Kubernetes 自动创建对应的 Endpoints 对象，记录匹配 Pod 的网络信息。",
  """### 核心概念

- **自动管理**：Service 的 selector 匹配 Pod 后，Endpoints Controller 自动更新 Endpoints。
- **手动 Endpoints**：不使用 selector 的 Service 可以手动指定 Endpoints，指向外部服务。
- **EndpointSlice**：Endpoints 的替代方案，将端点分片存储，适合大规模集群。

### Endpoints vs EndpointSlice

| 特性 | Endpoints | EndpointSlice |
|------|-----------|---------------|
| 容量 | 单对象存储所有端点 | 分片存储，每片最多 100 个 |
| 性能 | 大规模时 API Server 压力大 | 显著减少 API Server 负载 |
| 推荐 | 小规模 | 生产推荐 |""",
  """- EndpointSlice 从 K8s v1.21 起成为默认方案。
- Endpoints 对象仍可使用但不推荐在大规模集群中使用。
- Headless Service 的 DNS 查询直接返回 Endpoints 中的 Pod IP。""",
  """- 大规模集群确保启用 EndpointSlice API。
- 排查 Service 不通时检查 Endpoints 是否包含正确的后端 Pod。
- 使用 `kubectl get endpointslices` 查看分片的端点信息。""",
  "https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/",
  "[[domain-17-system-foundation/topic-dictionary/networking/endpointslices|EndpointSlices]]")

t("ingress-controller", "入口控制器", "Ingress Controller", "networking",
  ["k8s", "glossary", "ingress", "networking", "nginx"],
  "Ingress Controller 是实现 Ingress 规则的实际组件。Kubernetes 本身不包含 Ingress Controller 实现，需要用户部署第三方控制器来处理 HTTP/HTTPS 路由。",
  """### 主流 Ingress Controller

- **Nginx Ingress Controller**：最流行，基于 Nginx，功能丰富。
- **Traefik**：自动服务发现，支持多种协议。
- **Kong Ingress Controller**：基于 Kong API 网关。
- **HAProxy Ingress**：基于 HAProxy 的高性能方案。

### 工作原理

Ingress Controller 监听 Ingress 资源的变化，动态更新自身的反向代理配置，将外部流量路由到对应的后端 Service。""",
  """- Ingress Controller 通常以 Deployment + Service（LoadBalancer/NodePort）方式部署。
- 支持通过注解（annotations）扩展路由能力（限流、CORS、重写等）。
- 一个集群可以部署多个 Ingress Controller，通过 `ingressClassName` 区分。""",
  """- 生产环境部署高可用的 Ingress Controller（多副本 + PDB）。
- 使用 HPA 根据流量自动扩缩 Ingress Controller。
- 配置请求限流和 WAF 规则保护后端服务。
- 考虑迁移到 Gateway API 获得更强大的路由能力。""",
  "https://kubernetes.io/docs/concepts/services-networking/ingress-controllers/",
  "[[domain-17-system-foundation/topic-dictionary/networking/ingress-controllers|Ingress Controllers]]")

t("network-policy", "网络策略", "NetworkPolicy", "networking",
  ["k8s", "glossary", "network-policy", "security", "networking"],
  "NetworkPolicy 是 Kubernetes 中用于控制 Pod 之间以及 Pod 与外部网络之间流量访问的网络安全资源。它基于标签选择器定义允许/拒绝的入站和出站规则。",
  """### 核心概念

- **默认策略**：Kubernetes 默认允许所有 Pod 之间的通信（无隔离）。
- **策略生效**：为 Pod 配置 NetworkPolicy 后，未明确允许的流量将被拒绝。
- **三要素**：
  - `podSelector`：选择策略适用的 Pod。
  - `ingress`：定义允许的入站规则。
  - `egress`：定义允许的出站规则。

### 示例：限制只允许特定 Pod 访问

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
spec:
  podSelector:
    matchLabels:
      app: database
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: backend
    ports:
    - port: 5432
```""",
  """- NetworkPolicy 需要 CNI 插件支持（Calico、Cilium 等），部分 CNI 不支持。
- 策略基于 IP CIDR 和标签选择器，不直接支持 FQDN（域名）策略。
- 空 `ingress: []` 表示拒绝所有入站，空 `egress: []` 表示拒绝所有出站。""",
  """- 生产环境应为所有应用配置 NetworkPolicy 实现最小权限网络访问。
- 从默认拒绝策略开始，逐步添加允许规则。
- 使用 Cilium 的 FQDN Policy 实现基于域名的出站控制。
- 定期审计 NetworkPolicy 覆盖情况，确保无遗漏。""",
  "https://kubernetes.io/docs/concepts/services-networking/network-policies/",
  "[[domain-17-system-foundation/topic-dictionary/networking/network-policies|Network Policies]]")

t("cni", "容器网络接口", "CNI (Container Network Interface)", "networking",
  ["k8s", "glossary", "cni", "networking"],
  "CNI（Container Network Interface）是容器网络插件的标准接口规范。它定义了容器网络配置、创建和删除的标准化流程，使 Kubernetes 能够使用各种网络插件实现 Pod 间通信。",
  """### 核心概念

- **CNI 插件**：实现 CNI 规范的网络软件，如 Calico、Cilium、Flannel、Weave 等。
- **CNI 配置**：通过 JSON 配置文件定义网络拓扑和 IP 分配策略。
- **CNI 执行流程**：
  1. kubelet 通过 CRI 创建容器。
  2. 容器运行时调用 CNI 插件。
  3. CNI 插件配置网络接口和路由。

### 主流 CNI 插件

| 插件 | 数据面 | 网络策略 | 特点 |
|------|--------|---------|------|
| Calico | BGP/VXLAN | 支持 | 纯三层路由，性能优秀 |
| Cilium | eBPF | 支持 | 内核旁路，高性能 |
| Flannel | VXLAN | 不支持 | 简单轻量 |
| Weave | mesh | 支持 | 加密通信 |""",
  """- CNI 由 CNCF 维护，是容器网络的事实标准。
- CNI 配置文件位于 `/etc/cni/net.d/` 目录。
- 一个节点可以有多个 CNI 配置，按文件名排序选择。""",
  """- 生产环境推荐 Calico 或 Cilium，功能完整且性能优秀。
- 大规模集群考虑 Cilium 的 eBPF 数据面获得更好性能。
- 确保 CNI 版本与 Kubernetes 版本兼容。
- 监控 CNI 的 IP 分配情况和网络延迟。""",
  "https://www.cni.dev/",
  "[[entities/cilium|Cilium]] | [[entities/cni-plugins|CNI Plugins]]")

t("dns", "域名服务", "DNS", "networking",
  ["k8s", "glossary", "dns", "coredns", "networking"],
  "Kubernetes DNS 是集群内部的域名解析服务，为 Service 和 Pod 提供自动的 DNS 记录。CoreDNS 是 Kubernetes 的默认 DNS 实现。",
  """### DNS 记录格式

- **Service**：`<service-name>.<namespace>.svc.cluster.local`
- **Headless Service**：返回所有后端 Pod 的 IP 地址。
- **Pod**：`<pod-ip-dashed>.<namespace>.pod.cluster.local`
- **StatefulSet Pod**：`<pod-name>.<headless-service>.<namespace>.svc.cluster.local`

### CoreDNS

CoreDNS 是 CNCF 毕业项目，通过 Kubernetes 插件机制部署。它支持丰富的插件生态，包括缓存、转发、日志等。""",
  """- `ndots` 配置影响 DNS 查询行为（默认 5，可能导致额外查询）。
- DNS 缓存（NodeLocal DNSCache）可显著减少 CoreDNS 负载。
- CoreDNS 的 `forward` 插件可将外部域名转发到上游 DNS。
- `dnsConfig` 字段允许自定义 Pod 的 DNS 配置。""",
  """- 生产环境部署 NodeLocal DNSCache 减少 CoreDNS 压力。
- 调整 `ndots: 2` 减少不必要的 DNS 查询。
- 监控 CoreDNS 的 QPS、延迟和缓存命中率。
- 为外部服务配置 ExternalName Service 或 CoreDNS rewrite 规则。""",
  "https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/",
  "[[entities/coredns|CoreDNS]]")

# ═══════════════════════════════════════════════════════════════════
# 4. 核心资源 — 存储 storage/
# ═══════════════════════════════════════════════════════════════════

t("persistent-volume", "持久化卷", "PersistentVolume (PV)", "storage",
  ["k8s", "glossary", "storage", "pv"],
  "PersistentVolume（PV）是 Kubernetes 中集群级别的存储资源，由管理员预先创建或通过 StorageClass 动态供给。它代表集群中的一块实际存储（如云磁盘、NFS 共享等），独立于 Pod 的生命周期。",
  """### 核心概念

- **PV 属性**：容量（capacity）、访问模式（access modes）、回收策略（reclaim policy）、存储类。
- **访问模式**：
  - `ReadWriteOnce (RWO)`：单节点读写。
  - `ReadOnlyMany (ROX)`：多节点只读。
  - `ReadWriteMany (RWX)`：多节点读写。
  - `ReadWriteOncePod (RWOP)`：单 Pod 读写（v1.22+）。
- **回收策略**：`Retain`（保留数据）、`Delete`（删除存储）、`Recycle`（已弃用）。

### PV 生命周期

```
Available → Bound → Released → (Available/Delete)
```""",
  """- PV 与 PVC 是一对一绑定关系。
- `persistentVolumeReclaimPolicy: Retain` 确保数据不会被意外删除。
- 静态供给需要管理员预先创建 PV 对象。""",
  """- 生产环境优先使用动态供给（StorageClass）。
- 为关键数据使用 `Retain` 回收策略。
- 监控 PV 的状态（Available/Bound/Released）。""",
  "https://kubernetes.io/docs/concepts/storage/persistent-volumes/",
  "")

t("persistent-volume-claim", "持久化卷声明", "PersistentVolumeClaim (PVC)", "storage",
  ["k8s", "glossary", "storage", "pvc"],
  "PersistentVolumeClaim（PVC）是用户对存储资源的请求。类似于 Pod 消耗节点资源，PVC 消耗 PV 资源。用户通过 PVC 指定所需的存储大小和访问模式。",
  """### 核心概念

- **PVC 请求参数**：存储容量、访问模式、StorageClass。
- **绑定过程**：PVC 与满足条件的 PV 自动绑定（静态供给）或通过 StorageClass 动态创建 PV（动态供给）。
- **使用方式**：PVC 作为 Volume 挂载到 Pod 中。

### PVC 示例

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: standard
  resources:
    requests:
      storage: 10Gi
```""",
  """- PVC 创建后可以扩容（如果 StorageClass 的 `allowVolumeExpansion: true`）。
- PVC 不能缩减容量。
- 使用 `volumeMode: Block` 获取原始块设备而非文件系统。""",
  """- 根据应用需求选择合适的访问模式和 StorageClass。
- 为关键应用使用 ReadWriteOnce + Retain 策略。
- 定期检查 PVC 的状态和使用率。""",
  "https://kubernetes.io/docs/concepts/storage/persistent-volumes/",
  "")

t("storage-class", "存储类", "StorageClass", "storage",
  ["k8s", "glossary", "storage", "storageclass"],
  "StorageClass 是 Kubernetes 中定义存储类别的资源。它使管理员能够描述不同质量级别的存储（如 SSD/HDD、性能等级），并实现存储的动态供给。",
  """### 核心属性

- **provisioner**：指定使用哪个 CSI 驱动或内置供给器创建存储。
- **parameters**：传递给存储供给器的特定参数（如磁盘类型、IOPS）。
- **reclaimPolicy**：动态创建的 PV 的回收策略。
- **allowVolumeExpansion**：是否允许 PVC 扩容。
- **volumeBindingMode**：`Immediate`（立即绑定）或 `WaitForFirstConsumer`（等待 Pod 调度后绑定）。

### 示例

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```""",
  """- `WaitForFirstConsumer` 模式避免创建存储时不知道 Pod 会调度到哪个节点的问题。
- 可以设置默认 StorageClass（通过注解 `storageclass.kubernetes.io/is-default-class: "true"`）。
- CSI 驱动的 StorageClass 比 in-tree 的更灵活。""",
  """- 为不同工作负载定义不同级别的 StorageClass（如 fast-ssd、standard-hdd）。
- 生产环境使用 `WaitForFirstConsumer` 绑定模式。
- 启用 `allowVolumeExpansion` 以支持在线扩容。""",
  "https://kubernetes.io/docs/concepts/storage/storage-classes/",
  "")

t("volume", "卷", "Volume", "storage",
  ["k8s", "glossary", "storage", "volume"],
  "Volume 是 Kubernetes 中为 Pod 容器提供文件系统访问的存储抽象。容器内的文件默认是临时的，Volume 解决了数据持久化和容器间共享存储的需求。",
  """### Volume 类型

Kubernetes 支持多种 Volume 类型：

- **emptyDir**：临时目录，Pod 删除后数据丢失。
- **hostPath**：挂载节点文件系统的特定路径。
- **configMap/secret**：将 ConfigMap/Secret 作为文件挂载。
- **persistentVolumeClaim**：挂载持久化存储。
- **projected**：将多个 Volume 源合并为一个目录。
- **csi**：直接使用 CSI 驱动提供的卷。
- **nfs**：挂载 NFS 共享。""",
  """- Volume 的生命周期与 Pod 绑定（emptyDir）或独立于 Pod（PV）。
- 容器崩溃（非 Pod 删除）时，emptyDir 数据不丢失。
- Volume 可以以只读或读写方式挂载。""",
  """- 临时数据使用 emptyDir。
- 配置文件使用 ConfigMap Volume。
- 敏感信息使用 Secret Volume。
- 持久化数据使用 PVC。
- 避免使用 hostPath（安全风险和可移植性问题）。""",
  "https://kubernetes.io/docs/concepts/storage/volumes/",
  "")

t("emptydir", "空目录卷", "emptyDir", "storage",
  ["k8s", "glossary", "storage", "emptydir"],
  "emptyDir 是一种临时存储卷，在 Pod 被分配到节点时创建，Pod 从节点移除时数据永久丢失。适用于 Pod 内容器间的临时数据共享。",
  """### 核心特性

- **生命周期**：与 Pod 绑定。Pod 删除 → emptyDir 数据丢失。
- **容器间共享**：Pod 中多个容器可以挂载同一个 emptyDir。
- **内存模式**：`medium: Memory` 使用 tmpfs（RAM），速度更快但受内存限制。

### 示例

```yaml
volumes:
- name: scratch
  emptyDir:
    sizeLimit: 1Gi
- name: cache
  emptyDir:
    medium: Memory
    sizeLimit: 512Mi
```""",
  """- emptyDir 的默认存储介质是节点的本地磁盘。
- `sizeLimit` 限制 emptyDir 的最大容量。
- Memory 类型的 emptyDir 计入容器的内存使用。""",
  """- 用作多容器 Pod 的共享工作区。
- 存放崩溃恢复所需的检查点数据。
- 大文件处理的暂存目录。
- 不要用于需要持久化的数据。""",
  "https://kubernetes.io/docs/concepts/storage/volumes/#emptydir",
  "")

t("hostpath", "主机路径卷", "hostPath", "storage",
  ["k8s", "glossary", "storage", "hostpath"],
  "hostPath 卷将节点文件系统的文件或目录挂载到 Pod 中。它提供了对节点文件系统的直接访问，但存在安全和可移植性风险。",
  """### 核心特性

- **直接访问**：Pod 可以直接读写节点文件系统的指定路径。
- **类型检查**：`type` 字段可以指定挂载前需要进行的检查（如 DirectoryExists、FileOrCreate）。

### 安全风险

- Pod 可以访问节点上的敏感文件（如 `/etc/shadow`）。
- 不同节点的文件系统结构可能不同，导致 Pod 不可移植。
- 恶意 Pod 可能修改节点关键文件。""",
  """- hostPath 是少数几种允许容器访问宿主机的 Volume 类型。
- 与 `privileged: true` 组合使用时风险更高。
- PodSecurityStandards 的 `baseline` 和 `restricted` 级别限制 hostPath 使用。""",
  """- **仅在系统级 DaemonSet 中使用**（如日志收集、监控代理）。
- 应用工作负载不应使用 hostPath。
- 使用 `readOnly: true` 减少安全风险。
- 考虑使用 PV/PVC 替代 hostPath。""",
  "https://kubernetes.io/docs/concepts/storage/volumes/#hostpath",
  "")

t("csi", "容器存储接口", "CSI (Container Storage Interface)", "storage",
  ["k8s", "glossary", "storage", "csi"],
  "CSI（Container Storage Interface）是存储插件的标准接口规范。它定义了存储系统如何与容器编排系统集成的标准化方式，取代了 Kubernetes 早期的 in-tree 存储插件。",
  """### 核心概念

- **CSI Driver**：实现 CSI 接口的存储驱动程序，通常由存储厂商提供。
- **Controller Plugin**：处理卷的创建/删除/快照等控制操作（运行在任意节点）。
- **Node Plugin**：处理卷的挂载/卸载和格式化（运行在每个节点，通常以 DaemonSet 部署）。

### CSI 操作

- `CreateVolume` / `DeleteVolume`：卷的生命周期管理。
- `ControllerPublishVolume` / `ControllerUnpublishVolume`：卷的附加/分离。
- `NodeStageVolume` / `NodePublishVolume`：卷的格式化/挂载。
- `CreateSnapshot` / `DeleteSnapshot`：快照管理。""",
  """- CSI 驱动通过 `CSIDriver` 和 `CSINode` 对象注册到 Kubernetes。
- 支持动态供给、卷快照、卷克隆、卷扩容等高级功能。
- CSI 驱动与 Kubernetes 版本有兼容性要求。""",
  """- 选择经过认证的 CSI 驱动，确保与 Kubernetes 版本兼容。
- 测试 CSI 驱动在高负载下的性能和稳定性。
- 监控 CSI 操作的延迟和成功率。""",
  "https://kubernetes-csi.github.io/docs/",
  "[[entities/csi-drivers|CSI Drivers]]")

# ═══════════════════════════════════════════════════════════════════
# 5. 核心资源 — 配置与安全 configuration/ & security/
# ═══════════════════════════════════════════════════════════════════

t("configmap", "配置映射", "ConfigMap", "configuration",
  ["k8s", "glossary", "configmap", "configuration"],
  "ConfigMap 是 Kubernetes 中用于存储非敏感配置数据的 API 资源。它将配置与容器镜像解耦，使应用配置可以被集中管理和动态更新。",
  """### 使用方式

1. **环境变量**：通过 `envFrom` 或 `env` 注入容器环境变量。
2. **命令行参数**：作为容器启动命令的参数。
3. **文件挂载**：作为 Volume 挂载到容器中的文件。

### 示例

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
data:
  database_host: "mysql.default.svc"
  log_level: "info"
  config.yaml: |
    server:
      port: 8080
```""",
  """- ConfigMap 大小限制 1MB。
- 通过 Volume 挂载的 ConfigMap 更新会自动传播（有延迟）。
- 通过环境变量注入的 ConfigMap 更新需要重启 Pod。
- `subPath` 挂载单个文件时不会自动更新。""",
  """- 敏感数据使用 Secret 而非 ConfigMap。
- 使用 Kustomize 或 Helm 管理 ConfigMap 的版本。
- 为 ConfigMap 设置合理的标签便于管理。
- 考虑使用外部配置中心（如 Apollo、Nacos）管理动态配置。""",
  "https://kubernetes.io/docs/concepts/configuration/configmap/",
  "[[domain-17-system-foundation/topic-dictionary/configuration/configmaps|ConfigMaps]]")

t("secret", "密钥", "Secret", "security",
  ["k8s", "glossary", "secret", "security"],
  "Secret 是 Kubernetes 中用于存储敏感数据的 API 资源，如密码、Token、TLS 证书等。它提供了比 ConfigMap 更强的安全控制机制。",
  """### Secret 类型

- **Opaque**：通用 Secret（默认类型）。
- **kubernetes.io/tls**：TLS 证书和私钥。
- **kubernetes.io/dockerconfigjson**：容器镜像仓库认证。
- **kubernetes.io/basic-auth**：基本认证凭据。
- **kubernetes.io/ssh-auth**：SSH 认证密钥。
- **kubernetes.io/service-account-token**：ServiceAccount Token。

### 安全措施

- etcd 加密：启用 EncryptionConfiguration 加密 Secret 数据。
- RBAC：限制对 Secret 资源的访问权限。
- 外部密钥管理：集成 Vault、AWS Secrets Manager 等。""",
  """- Secret 数据以 Base64 编码存储（非加密），需配合 etcd 加密。
- Secret 大小限制 1MB。
- 使用 `stringData` 字段可以用明文方式创建 Secret（自动转换为 Base64）。
- Volume 挂载的 Secret 更新会自动传播。""",
  """- 生产环境使用 External Secrets Operator 集成外部密钥管理系统。
- 启用 etcd 加密确保 Secret 数据安全。
- 通过 RBAC 严格控制 Secret 的访问权限。
- 避免将 Secret 硬编码在 YAML 文件中并提交到 Git。""",
  "https://kubernetes.io/docs/concepts/configuration/secret/",
  "[[domain-17-system-foundation/topic-dictionary/configuration/secrets|Secrets]]")

t("service-account", "服务账号", "ServiceAccount", "security",
  ["k8s", "glossary", "service-account", "rbac", "security"],
  "ServiceAccount 是 Kubernetes 中为 Pod 提供身份标识的资源。Pod 通过关联的 ServiceAccount 向 API Server 认证身份，获取访问集群资源的权限。",
  """### 核心概念

- **默认 ServiceAccount**：每个命名空间自动创建 `default` ServiceAccount。
- **Token 注入**：kubelet 自动将 ServiceAccount Token 挂载到 Pod 中（Projected Volume）。
- **Token 特性**：
  - 有界的（bound to Pod）。
  - 有过期时间（默认 1 小时，自动轮转）。
  - 观众限制（audience-restricted）。

### RBAC 集成

通过 RoleBinding 或 ClusterRoleBinding 将权限授予 ServiceAccount，实现 Pod 级别的权限控制。""",
  """- Token Request API（v1.20+）提供时间有界、受众受限的 Token。
- `automountServiceAccountToken: false` 可以禁止自动挂载 Token。
- `boundServiceAccountTokenVolume` 特性确保 Token 安全。""",
  """- 为每个应用创建独立的 ServiceAccount，避免使用 default。
- 遵循最小权限原则，只授予必要的 RBAC 权限。
- 对不需要 API 访问的 Pod，禁用自动 Token 挂载。
- 使用 TokenRequest API 为外部服务生成短期 Token。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/service-accounts-admin/",
  "")

t("role", "角色", "Role", "security",
  ["k8s", "glossary", "role", "rbac", "security"],
  "Role 是 Kubernetes RBAC 中命名空间级别的权限定义资源。它定义了一组允许的操作（verbs）和可操作的资源（resources）。",
  """### 核心概念

- **verbs**：允许的操作（`get`, `list`, `watch`, `create`, `update`, `patch`, `delete`）。
- **resources**：可操作的资源类型（`pods`, `services`, `deployments` 等）。
- **resourceNames**：限定特定资源实例名称。
- **apiGroups**：API 组（`""` 表示核心组，`apps` 表示 apps 组等）。

### 示例

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: pod-reader
  namespace: default
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list", "watch"]
```""",
  """- Role 仅在命名空间内生效。
- ClusterRole 在集群范围生效。
- Role 和 ClusterRole 都通过 Binding 关联到用户/组/ServiceAccount。""",
  """- 遵循最小权限原则。
- 优先使用 ClusterRole + RoleBinding 的模式减少重复定义。
- 定期审计 RBAC 配置，清理不再使用的 Role。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/rbac/",
  "")

t("clusterrole", "集群角色", "ClusterRole", "security",
  ["k8s", "glossary", "clusterrole", "rbac", "security"],
  "ClusterRole 是 Kubernetes RBAC 中集群级别的权限定义资源。与 Role 不同，ClusterRole 不受命名空间限制，可以授予集群范围和跨命名空间的权限。",
  """### 核心概念

- **集群范围权限**：可以访问集群级别资源（Node、PV、ClusterRole 等）。
- **跨命名空间权限**：通过 ClusterRoleBinding 可以在所有命名空间生效。
- **命名空间范围使用**：ClusterRole 也可以通过 RoleBinding 限制在特定命名空间内使用。
- **聚合 ClusterRole**：使用 `aggregationRule` 自动合并多个 ClusterRole 的规则。

### 内置 ClusterRole

Kubernetes 预定义了一些常用的 ClusterRole：
- `cluster-admin`：完全管理员权限。
- `admin`：命名空间管理员。
- `edit`：命名空间内读写。
- `view`：命名空间内只读。""",
  """- 聚合 ClusterRole 会自动包含匹配标签的其他 ClusterRole 的规则。
- `admin`、`edit`、`view` 是推荐的预定义角色。
- ClusterRole 可以授予对非资源 URL（如 `/healthz`）的访问权限。""",
  """- 避免过度使用 `cluster-admin`，优先使用最小权限的自定义 ClusterRole。
- 使用预定义的 `view`/`edit`/`admin` 角色简化权限管理。
- 定期使用 `kubectl auth can-i --list` 审计权限。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/rbac/",
  "")

t("rolebinding", "角色绑定", "RoleBinding", "security",
  ["k8s", "glossary", "rolebinding", "rbac", "security"],
  "RoleBinding 将 Role 或 ClusterRole 的权限授予命名空间内的用户、组或 ServiceAccount。它是 RBAC 中连接权限定义和权限主体的桥梁。",
  """### 核心概念

- **subjects**：权限接收者（User、Group、ServiceAccount）。
- **roleRef**：引用的 Role 或 ClusterRole。
- **命名空间范围**：RoleBinding 仅在创建它的命名空间内生效。

### ClusterRole + RoleBinding 模式

一个常见的模式是使用 ClusterRole 定义权限，然后通过 RoleBinding 限制在特定命名空间内授予：

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: read-pods
  namespace: development
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: view
subjects:
- kind: ServiceAccount
  name: dev-app
  namespace: development
```""",
  """- RoleBinding 引用 Role 时，权限限制在该命名空间。
- RoleBinding 引用 ClusterRole 时，权限仍限制在 RoleBinding 所在的命名空间。
- 删除 RoleBinding 不会删除关联的 Role/ClusterRole。""",
  """- 为每个应用创建独立的 ServiceAccount 并通过 RoleBinding 授权。
- 避免在 RoleBinding 中引用 `cluster-admin`。
- 定期审计命名空间的 RoleBinding 配置。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/rbac/",
  "")

t("clusterrolebinding", "集群角色绑定", "ClusterRoleBinding", "security",
  ["k8s", "glossary", "clusterrolebinding", "rbac", "security"],
  "ClusterRoleBinding 将 ClusterRole 的权限授予集群范围的主体。与 RoleBinding 不同，ClusterRoleBinding 的权限在整个集群内生效。",
  """### 核心概念

- **集群范围生效**：授权主体可以在所有命名空间执行授权的操作。
- **主体类型**：User、Group、ServiceAccount。
- **使用场景**：集群管理员权限、跨命名空间权限、集群级资源访问。

### 示例

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: cluster-readers
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: view
subjects:
- apiGroup: rbac.authorization.k8s.io
  kind: Group
  name: developers
```""",
  """- ClusterRoleBinding 一旦创建，授权主体在整个集群范围内拥有相应权限。
- 谨慎使用 ClusterRoleBinding，遵循最小权限原则。
- 删除 ClusterRoleBinding 不影响关联的 ClusterRole。""",
  """- 仅对需要集群范围权限的场景使用 ClusterRoleBinding。
- 优先使用 Group 而非单独 User 来管理集群级权限。
- 定期使用 `kubectl get clusterrolebindings` 审计集群级授权。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/rbac/",
  "")

t("rbac", "基于角色的访问控制", "RBAC (Role-Based Access Control)", "security",
  ["k8s", "glossary", "rbac", "security"],
  "RBAC（Role-Based Access Control）是 Kubernetes 的权限管理机制，通过角色（Role/ClusterRole）和绑定（RoleBinding/ClusterRoleBinding）来控制用户、组和 ServiceAccount 对集群资源的访问权限。",
  """### RBAC 四大资源

| 资源 | 范围 | 作用 |
|------|------|------|
| Role | 命名空间 | 定义权限规则 |
| ClusterRole | 集群 | 定义权限规则 |
| RoleBinding | 命名空间 | 将权限授予主体 |
| ClusterRoleBinding | 集群 | 将权限授予主体 |

### 授权流程

```
用户请求 → API Server → RBAC Authorizer → 匹配 Role/ClusterRole 规则 → 允许/拒绝
```

### RBAC 决策规则

- **默认拒绝**：没有明确允许的操作都会被拒绝。
- **权限叠加**：多个 RoleBinding 的权限取并集。
- **不可拒绝**：RBAC 只支持"允许"，不支持显式"拒绝"。""",
  """- RBAC 从 K8s v1.8 起成为稳定特性。
- 支持 `*` 通配符匹配所有 verbs/resources/apiGroups。
- 支持自定义动词（如 `bind`、`escalate`）。""",
  """- 始终启用 RBAC（禁用 `--authorization-mode=AlwaysAllow`）。
- 遵循最小权限原则，避免过度授权。
- 使用 `kubectl auth can-i` 验证权限配置。
- 定期运行 RBAC 审计工具（如 rakkess、rbac-lookup）。
- 为每个应用创建独立的 ServiceAccount 并绑定最小权限。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/rbac/",
  "")

# ═══════════════════════════════════════════════════════════════════
# 6. 调度 scheduling/
# ═══════════════════════════════════════════════════════════════════

t("node-selector", "节点选择器", "nodeSelector", "scheduling",
  ["k8s", "glossary", "scheduling", "node-selector"],
  "nodeSelector 是 Kubernetes 中最简单的 Pod 调度约束方式。它通过键值对标签匹配，将 Pod 限制在具有特定标签的节点上运行。",
  """### 基本用法

```yaml
spec:
  nodeSelector:
    disktype: ssd
    zone: us-east-1a
```

Pod 只会被调度到同时具有 `disktype=ssd` 和 `zone=us-east-1a` 标签的节点。

### 局限性

- 只支持等值匹配（不支持 In、NotIn、Exists 等操作符）。
- 无法表达"偏好"（soft requirement），只有"必须"（hard requirement）。
- 复杂场景应使用 nodeAffinity。""",
  """- nodeSelector 是 nodeAffinity 的简化版本。
- 空 `nodeSelector: {}` 表示不限制。
- 可以与其他调度约束（Affinity、Taint/Toleration）组合使用。""",
  """- 简单的节点约束场景使用 nodeSelector（语法简洁）。
- 复杂场景使用 nodeAffinity。
- 为节点设置规范的标签体系，便于调度管理。""",
  "https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/",
  "")

t("affinity", "亲和性", "Affinity", "scheduling",
  ["k8s", "glossary", "scheduling", "affinity"],
  "Affinity（亲和性）是 Kubernetes 中表达 Pod 对节点或其他 Pod 调度偏好的机制。它比 nodeSelector 更灵活，支持多种操作符和软硬约束。",
  """### 亲和性类型

#### Node Affinity（节点亲和性）

- **requiredDuringSchedulingIgnoredDuringExecution**：硬性要求（等同于增强的 nodeSelector）。
- **preferredDuringSchedulingIgnoredDuringExecution**：软性偏好（调度器尽量满足，但不保证）。

```yaml
affinity:
  nodeAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
      nodeSelectorTerms:
      - matchExpressions:
        - key: topology.kubernetes.io/zone
          operator: In
          values: [us-east-1a, us-east-1b]
```

#### Pod Affinity（Pod 亲和性）

将 Pod 调度到与特定 Pod 相同拓扑域的位置。""",
  """- 支持的操作符：`In`、`NotIn`、`Exists`、`DoesNotExist`、`Gt`、`Lt`。
- Pod Affinity/Anti-Affinity 可以基于 `topologyKey` 指定拓扑域。
- 软性偏好通过 `weight` 字段控制优先级。""",
  """- 使用 Node Affinity 将工作负载调度到特定硬件或区域的节点。
- 使用 Pod Affinity 将相关服务调度到一起减少延迟。
- 软性偏好（preferred）在无法满足时不会阻止 Pod 调度。""",
  "https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/",
  "")

t("anti-affinity", "反亲和性", "Anti-Affinity", "scheduling",
  ["k8s", "glossary", "scheduling", "anti-affinity"],
  "Anti-Affinity（反亲和性）表达 Pod 不希望与某些 Pod 调度到同一拓扑域的约束。它是实现高可用和故障隔离的关键调度策略。",
  """### 核心概念

- **Pod Anti-Affinity**：确保 Pod 不与特定 Pod 运行在同一拓扑域（如同一节点、同一可用区）。
- **硬性约束**：`requiredDuringSchedulingIgnoredDuringExecution` — 必须满足。
- **软性约束**：`preferredDuringSchedulingIgnoredDuringExecution` — 尽量满足。

### 示例：确保副本分布在不同节点

```yaml
affinity:
  podAntiAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
    - labelSelector:
        matchLabels:
          app: web
      topologyKey: kubernetes.io/hostname
```""",
  """- `topologyKey` 定义拓扑域：`kubernetes.io/hostname`（节点级）、`topology.kubernetes.io/zone`（可用区级）。
- 硬性反亲和性可能导致 Pod 无法调度（如果没有足够的拓扑域）。
- 软性反亲和性通过 `weight` 控制优先级。""",
  """- 使用反亲和性确保应用副本分布在不同的节点或可用区。
- 优先使用软性反亲和性，避免过于严格的约束导致调度失败。
- 结合 topologySpreadConstraints 获得更均匀的分部效果。""",
  "https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/",
  "")

t("taint", "污点", "Taint", "scheduling",
  ["k8s", "glossary", "scheduling", "taint"],
  "Taint（污点）是应用在节点上的标记，表示该节点不应接受没有对应容忍度（Toleration）的 Pod。污点与容忍度配合工作，实现节点的调度控制。",
  """### 污点效果

- **NoSchedule**：新 Pod 不会调度到该节点（已运行的 Pod 不受影响）。
- **PreferNoSchedule**：尽量不调度新 Pod（软性约束）。
- **NoExecute**：驱逐已运行但没有对应容忍度的 Pod。

### 设置污点

```bash
kubectl taint nodes node1 key1=value1:NoSchedule
```

### 常见系统污点

- `node.kubernetes.io/not-ready`：节点未就绪。
- `node.kubernetes.io/unreachable`：节点不可达。
- `node.kubernetes.io/memory-pressure`：内存压力。
- `node.kubernetes.io/disk-pressure`：磁盘压力。""",
  """- 控制平面节点默认有 `node-role.kubernetes.io/control-plane:NoSchedule` 污点。
- 节点问题（NotReady、内存/磁盘压力）会自动添加系统污点。
- `NoExecute` 支持 `tolerationSeconds` 设置驱逐延迟。""",
  """- 为专用节点（如 GPU 节点）添加污点，只有特定工作负载才能调度。
- 使用 `PreferNoSchedule` 作为软性约束。
- 监控系统污点的添加和移除情况。""",
  "https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/",
  "")

t("toleration", "容忍", "Toleration", "scheduling",
  ["k8s", "glossary", "scheduling", "toleration"],
  "Toleration（容忍）是应用在 Pod 上的属性，允许 Pod 被调度到具有匹配污点（Taint）的节点上。它与污点配合工作，控制 Pod 的调度行为。",
  """### 基本语法

```yaml
tolerations:
- key: "gpu"
  operator: "Equal"
  value: "true"
  effect: "NoSchedule"
- key: "node.kubernetes.io/not-ready"
  operator: "Exists"
  effect: "NoExecute"
  tolerationSeconds: 300
```

### 操作符

- **Equal**（默认）：key 和 value 都匹配。
- **Exists**：只匹配 key，忽略 value。
- 空 key + Exists 操作符：匹配所有污点。""",
  """- DaemonSet Pod 通常自动添加系统污点的容忍度。
- `tolerationSeconds` 仅在 `NoExecute` 效果下有效。
- 多个 Toleration 可以匹配多个 Taint。""",
  """- 关键系统组件（如监控代理）添加 `NoExecute` 容忍以确保始终运行。
- 为 `not-ready` 和 `unreachable` 设置合理的 `tolerationSeconds`。
- 不要为普通应用添加过于宽泛的容忍度。""",
  "https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/",
  "")

# ═══════════════════════════════════════════════════════════════════
# 7. 调度 — 资源模型 scheduling/
# ═══════════════════════════════════════════════════════════════════

t("resource-request", "资源请求", "Resource Request", "scheduling",
  ["k8s", "glossary", "scheduling", "resource"],
  "Resource Request 是容器声明需要的最小资源量。调度器根据 Request 值决定 Pod 应该调度到哪个节点，kubelet 保证容器至少能获得 Request 数量的资源。",
  """### 核心概念

```yaml
resources:
  requests:
    cpu: "250m"      # 0.25 核 CPU
    memory: "256Mi"  # 256 MiB 内存
```

- **CPU Request**：调度器确保节点有足够的 CPU 容量。1 CPU = 1000m（millicores）。
- **Memory Request**：调度器确保节点有足够的内存。kubelet 通过 cgroup 保证内存分配。
- **Ephemeral Storage Request**：本地临时存储的请求量。

### 调度影响

调度器使用 Request 值进行调度决策：只有当节点的（总容量 - 已分配 Request）≥ Pod Request 时，Pod 才能被调度到该节点。""",
  """- Request 是调度的依据，Limit 是运行时的上限。
- 未设置 Request 时默认值为 0（BestEffort QoS）。
- 设置过高的 Request 会浪费资源，过低可能导致 OOM 或 CPU 节流。""",
  """- 基于实际监控数据设置合理的 Request 值。
- CPU Request 应覆盖应用的平均负载。
- Memory Request 应覆盖应用的稳态内存使用。
- 使用 VPA（Vertical Pod Autoscaler）推荐值作为参考。""",
  "https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/",
  "")

t("resource-limit", "资源限制", "Resource Limit", "scheduling",
  ["k8s", "glossary", "scheduling", "resource"],
  "Resource Limit 是容器允许使用的最大资源量。当容器使用量超过 Limit 时，CPU 会被节流（throttle），内存超出则容器会被 OOM Kill。",
  """### 核心概念

```yaml
resources:
  requests:
    cpu: "250m"
    memory: "256Mi"
  limits:
    cpu: "500m"
    memory: "512Mi"
```

- **CPU Limit**：超出 Limit 时 CPU 被节流（不是 Kill），性能下降但不会重启。
- **Memory Limit**：超出 Limit 时容器被 OOM Kill（Out of Memory），根据 restartPolicy 可能重启。
- **规则**：Limit 必须 ≥ Request。""",
  """- CPU Limit 通过 CFS（Completely Fair Scheduler）配额实现。
- Memory Limit 通过 cgroup 的 memory.limit_in_bytes 实现。
- `LimitRange` 可以为命名空间设置默认的 Request/Limit。""",
  """- 始终设置 Memory Limit 防止容器耗尽节点内存。
- CPU Limit 的设置需要谨慎：过低的 CPU Limit 会导致请求延迟增加。
- 使用 `requests == limits` 实现 Guaranteed QoS（关键工作负载）。
- 监控 OOMKill 事件和 CPU 节流指标。""",
  "https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/",
  "")

t("qos", "服务质量", "QoS (Quality of Service)", "scheduling",
  ["k8s", "glossary", "scheduling", "qos"],
  "QoS（Quality of Service）是 Kubernetes 对 Pod 的优先级分类机制。当节点资源不足时，kubelet 根据 QoS 类别决定驱逐 Pod 的顺序，优先级低的 Pod 先被驱逐。",
  """### QoS 类别

| 类别 | 条件 | 驱逐优先级 |
|------|------|-----------|
| **Guaranteed** | 所有容器都设置了 requests == limits | 最低（最后被驱逐） |
| **Burstable** | 至少一个容器设置了 requests 或 limits | 中等 |
| **BestEffort** | 所有容器都没有设置 requests 和 limits | 最高（最先被驱逐） |

### 判定规则

- **Guaranteed**：每个容器的 cpu/memory 的 requests 和 limits 都相等（或只设置 limits 未设置 requests，此时 requests 默认等于 limits）。
- **BestEffort**：所有容器的 cpu/memory 都未设置 requests 和 limits。
- **Burstable**：不满足以上两种条件的 Pod。""",
  """- QoS 类别在 Pod 创建时确定，运行期间不可更改。
- 可以通过 `kubectl get pod -o jsonpath='{.status.qosClass}'` 查看。
- 节点压力驱逐时，优先驱逐 BestEffort → Burstable → Guaranteed。""",
  """- 关键生产工作负载使用 Guaranteed QoS。
- 批处理和低优先级任务可以使用 Burstable。
- 避免在生产环境中使用 BestEffort（随时可能被驱逐）。
- 使用 `priorityClassName` 进一步细化驱逐优先级。""",
  "https://kubernetes.io/docs/tasks/configure-pod-container/quality-service-pod/",
  "")

t("limitrange", "限制范围", "LimitRange", "scheduling",
  ["k8s", "glossary", "scheduling", "limitrange"],
  "LimitRange 是 Kubernetes 中用于限制命名空间内每个容器/Pod 资源使用范围的策略资源。它为命名空间中的资源使用设定上下限。",
  """### 核心功能

- **默认值**：为未设置 resources 的容器提供默认的 Request/Limit。
- **最小值**：容器必须至少请求的资源量。
- **最大值**：容器允许设置的最大资源量。
- **比例限制**：Limit 与 Request 的最大比例。

### 示例

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
spec:
  limits:
  - default:        # 默认 Limit
      cpu: 500m
      memory: 512Mi
    defaultRequest: # 默认 Request
      cpu: 200m
      memory: 256Mi
    max:
      cpu: "2"
      memory: 2Gi
    min:
      cpu: 100m
      memory: 128Mi
    type: Container
```""",
  """- LimitRange 仅对新创建的 Pod 生效。
- 可以针对 Container、Pod、PersistentVolumeClaim 三种类型设置。
- 如果 Pod 的资源设置超出 LimitRange 范围，创建请求会被拒绝。""",
  """- 为每个命名空间创建 LimitRange 防止资源滥用。
- 结合 ResourceQuota 实现命名空间级别的资源总量控制。
- 设置合理的默认值，避免未配置 resources 的 Pod 影响节点稳定性。""",
  "https://kubernetes.io/docs/concepts/policy/limit-range/",
  "")

t("resource-quota", "资源配额", "ResourceQuota", "scheduling",
  ["k8s", "glossary", "scheduling", "resource-quota"],
  "ResourceQuota 是 Kubernetes 中限制命名空间总资源使用的策略资源。它控制一个命名空间中所有对象消耗的计算资源、存储资源和对象数量的总和。",
  """### 配额类型

- **计算资源配额**：限制命名空间的总 CPU/内存 Request 和 Limit。
- **存储资源配额**：限制命名空间的总存储请求量和 PVC 数量。
- **对象数量配额**：限制命名空间中特定类型对象的数量（如 Pod、Service、ConfigMap 数量）。

### 示例

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-quota
spec:
  hard:
    requests.cpu: "10"
    requests.memory: 20Gi
    limits.cpu: "20"
    limits.memory: 40Gi
    pods: "50"
    services: "20"
    persistentvolumeclaims: "10"
```""",
  """- ResourceQuota 超出配额时，创建请求会被 API Server 拒绝。
- 启用 ResourceQuota 后，创建 Pod 时必须指定 resources.requests/limits（或使用 LimitRange 默认值）。
- 可以基于 PriorityClass 设置作用域（Scope），只为特定优先级设置配额。""",
  """- 为每个团队/项目的命名空间设置 ResourceQuota。
- 结合 LimitRange 确保单个 Pod 也有资源限制。
- 监控 ResourceQuota 的使用率，及时扩容或优化。
- 使用 `kubectl describe resourcequota` 查看配额使用情况。""",
  "https://kubernetes.io/docs/concepts/policy/resource-quotas/",
  "")

# ═══════════════════════════════════════════════════════════════════
# 8. 探针与健康检查 configuration/
# ═══════════════════════════════════════════════════════════════════

t("liveness-probe", "存活探针", "Liveness Probe", "configuration",
  ["k8s", "glossary", "probe", "health-check"],
  "Liveness Probe（存活探针）用于检测容器是否处于运行状态。如果探测失败，kubelet 会终止容器并根据 restartPolicy 决定是否重启。",
  """### 探测方式

- **HTTP GET**：发送 HTTP 请求，200-399 为成功。
- **TCP Socket**：尝试建立 TCP 连接。
- **Exec**：在容器内执行命令，退出码 0 为成功。
- **gRPC**：执行 gRPC Health Check（v1.24+ stable）。

### 配置参数

```yaml
livenessProbe:
  httpGet:
    path: /healthz
    port: 8080
  initialDelaySeconds: 15
  periodSeconds: 10
  timeoutSeconds: 5
  failureThreshold: 3
  successThreshold: 1
```""",
  """- `initialDelaySeconds`：容器启动后等待多久开始探测。
- `failureThreshold`：连续失败多少次才判定为失败。
- Liveness Probe 失败会导致容器重启。
- 不要将依赖外部服务的检查放入 Liveness Probe（会导致级联重启）。""",
  """- Liveness Probe 应只检查容器自身的健康状态。
- 设置合理的 `initialDelaySeconds` 避免启动期间误杀。
- 避免在 Liveness Probe 中执行重量级检查。
- 对于慢启动应用，使用 Startup Probe 替代。""",
  "https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/",
  "[[domain-17-system-foundation/topic-dictionary/configuration/liveness-readiness-and-startup-probes|Probes]]")

t("readiness-probe", "就绪探针", "Readiness Probe", "configuration",
  ["k8s", "glossary", "probe", "health-check"],
  "Readiness Probe（就绪探针）用于检测容器是否准备好接受流量。如果探测失败，Pod 会被从 Service 的 Endpoints 中移除，不再接收请求。",
  """### 与 Liveness Probe 的区别

| 特性 | Liveness Probe | Readiness Probe |
|------|---------------|-----------------|
| 失败行为 | 重启容器 | 从 Service 移除 |
| 用途 | 检测死锁/卡住 | 检测是否就绪 |
| successThreshold | 始终为 1 | 可配置 |

### 典型场景

- 应用启动时需要加载大量数据。
- 依赖的外部服务暂时不可用。
- 应用需要预热缓存后才能接受流量。""",
  """- Readiness Probe 失败不会重启容器，只是暂停接收流量。
- `successThreshold` 默认为 1，可以设置更大的值确保稳定后再接受流量。
- Pod 中的所有 Readiness Probe 都成功后，Pod 才被视为 Ready。""",
  """- 为所有面向流量的服务配置 Readiness Probe。
- Readiness Probe 的检查路径应反映应用的真实就绪状态。
- 避免 Readiness Probe 和 Liveness Probe 使用相同的检查逻辑。
- 设置合理的 `periodSeconds` 平衡检测灵敏度和开销。""",
  "https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/",
  "")

t("startup-probe", "启动探针", "Startup Probe", "configuration",
  ["k8s", "glossary", "probe", "health-check"],
  "Startup Probe（启动探针）用于检测容器是否已完成启动。在 Startup Probe 成功之前，Liveness 和 Readiness Probe 会被禁用，防止慢启动应用被误杀。",
  """### 核心价值

对于启动时间较长的应用（如 Java 应用加载 JVM、加载大量数据），Startup Probe 提供了一个"启动宽限期"：

```yaml
startupProbe:
  httpGet:
    path: /healthz
    port: 8080
  failureThreshold: 30
  periodSeconds: 10
  # 最多等待 30 × 10 = 300 秒启动
```

### 工作流程

1. 容器启动 → Startup Probe 开始探测。
2. Startup Probe 成功 → 启用 Liveness 和 Readiness Probe。
3. Startup Probe 失败（达到 failureThreshold）→ 容器被终止。""",
  """- Startup Probe 从 K8s v1.20 起达到 stable。
- `failureThreshold × periodSeconds` 定义了最大启动等待时间。
- 一旦 Startup Probe 成功一次，就不再执行。""",
  """- 启动时间不确定的应用（Java、大型数据库）应配置 Startup Probe。
- 设置足够大的 `failureThreshold` 容纳最坏情况的启动时间。
- 避免将 Startup Probe 的检查逻辑设置得过于复杂。""",
  "https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/",
  "")

t("graceful-shutdown", "优雅关闭", "Graceful Shutdown", "configuration",
  ["k8s", "glossary", "configuration", "graceful-shutdown"],
  "Graceful Shutdown（优雅关闭）是 Kubernetes 在终止 Pod 时允许容器执行清理操作的机制。它确保应用在退出前有机会完成进行中的请求、关闭连接和释放资源。",
  """### 关闭流程

```
1. Pod 被标记删除
2. Pod 状态变为 Terminating
3. API Server 发送 SIGTERM 信号给容器主进程
4. 等待 terminationGracePeriodSeconds（默认 30 秒）
5. 超时后发送 SIGKILL 强制终止
6. Pod 被删除
```

### 关键配置

```yaml
spec:
  terminationGracePeriodSeconds: 60  # 给予 60 秒的清理时间
  containers:
  - name: app
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 10"]  # 等待负载均衡器更新
```""",
  """- `preStop` hook 在 SIGTERM 之前执行，可用于延迟和通知。
- `terminationGracePeriodSeconds` 包括 preStop + 应用清理的总时间。
- kube-proxy 需要同步时间更新 iptables/IPVS 规则，`preStop` sleep 可以避免流量丢失。""",
  """- 所有生产服务都应实现优雅关闭。
- 应用应监听 SIGTERM 信号并执行清理逻辑。
- 设置 `preStop: sleep 5-10` 避免负载均衡器未及时更新导致的请求失败。
- `terminationGracePeriodSeconds` 应大于预期的清理时间。""",
  "https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/#pod-termination",
  "")

# ═══════════════════════════════════════════════════════════════════
# 9. API 与工具 platform-engineering/ & tooling/
# ═══════════════════════════════════════════════════════════════════

t("api-group", "API 组", "API Group", "platform-engineering",
  ["k8s", "glossary", "api", "platform"],
  "API Group 是 Kubernetes 中将相关 API 资源组织在一起的逻辑分组机制。不同的功能模块通过不同的 API Group 暴露，便于管理和版本控制。",
  """### 核心概念

- **核心组（Core Group）**：`""` 或 `v1`，包含 Pod、Service、ConfigMap、Secret 等基础资源。
- **命名组（Named Groups）**：如 `apps/v1`（Deployment、StatefulSet）、`batch/v1`（Job、CronJob）、`networking.k8s.io/v1`（Ingress、NetworkPolicy）。

### 常用 API Group

| API Group | 资源示例 |
|-----------|---------|
| `""` (core) | Pod, Service, ConfigMap, Secret, Node |
| `apps` | Deployment, StatefulSet, DaemonSet, ReplicaSet |
| `batch` | Job, CronJob |
| `networking.k8s.io` | Ingress, NetworkPolicy, IngressClass |
| `rbac.authorization.k8s.io` | Role, ClusterRole, RoleBinding |
| `storage.k8s.io` | StorageClass, CSIDriver |""",
  """- 通过 `kubectl api-resources` 查看所有可用的 API Group 和资源。
- API Group 支持多个版本共存（如 `v1`、`v1beta1`）。
- CRD 使用自定义的 API Group。""",
  """- 创建 CRD 时使用 `yourcompany.io` 格式的 API Group。
- 了解 API Group 有助于正确编写 RBAC 规则和 Manifest。""",
  "https://kubernetes.io/docs/reference/using-api/",
  "")

t("manifest", "清单", "Manifest", "platform-engineering",
  ["k8s", "glossary", "manifest", "yaml"],
  "Manifest 是 Kubernetes 资源的声明式定义文件，通常使用 YAML 或 JSON 格式。它描述了资源的期望状态，Kubernetes 会持续将当前状态向期望状态调整。",
  """### 基本结构

```yaml
apiVersion: apps/v1       # API 版本
kind: Deployment           # 资源类型
metadata:                  # 元数据（名称、命名空间、标签）
  name: my-app
  namespace: default
spec:                      # 期望状态（规格）
  replicas: 3
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: app
        image: nginx:1.25
```

### 管理方式

- **命令式**：`kubectl run`、`kubectl create`。
- **声明式**：`kubectl apply -f manifest.yaml`（推荐）。
- **Kustomize**：通过 overlay 管理 Manifest 变体。
- **Helm**：通过模板和 Values 动态生成 Manifest。""",
  """- `kubectl apply` 使用 Server-Side Apply（SSA）或 Client-Side Apply 管理资源。
- Manifest 应纳入版本控制（GitOps 理念）。
- `kubectl diff` 可以预览 Manifest 变更。""",
  """- 始终使用声明式管理（`kubectl apply`）而非命令式。
- 将 Manifest 存储在 Git 仓库中。
- 使用 Kustomize 或 Helm 管理多环境差异。
- 为 Manifest 添加完整的 labels 和 annotations。""",
  "https://kubernetes.io/docs/concepts/overview/working-with-objects/",
  "")

t("kubectl", "kubectl", "kubectl", "tooling",
  ["k8s", "glossary", "kubectl", "tooling"],
  "kubectl 是 Kubernetes 的官方命令行工具，通过与 API Server 通信来管理集群资源。它是 Kubernetes 用户和运维人员最常用的工具。",
  """### 常用命令分类

| 类别 | 命令示例 |
|------|---------|
| 查看资源 | `kubectl get pods`, `kubectl describe pod <name>` |
| 创建/更新 | `kubectl apply -f`, `kubectl create` |
| 调试 | `kubectl logs`, `kubectl exec`, `kubectl port-forward` |
| 集群管理 | `kubectl drain`, `kubectl cordon`, `kubectl top` |
| 配置 | `kubectl config use-context`, `kubectl config set-cluster` |

### 高级功能

- **Dry-run**：`kubectl apply --dry-run=client/server` 预览变更。
- **Server-side Apply**：`kubectl apply --server-side` 使用服务端合并。
- **输出格式**：`-o json`, `-o yaml`, `-o jsonpath`, `-o custom-columns`。""",
  """- kubectl 通过 kubeconfig 文件连接集群。
- 支持插件机制（通过 PATH 中的 `kubectl-*` 二进制）。
- `kubectl explain` 查看资源的文档说明。
- `kubectl api-resources` 和 `kubectl api-versions` 查看可用 API。""",
  """- 使用 `kubectl` 别名和自动补全提高效率。
- 生产环境操作前使用 `--dry-run=server` 验证。
- 使用 `kubectl auth can-i` 验证权限。
- 安装常用插件（krew 管理）：stern、kubens、kubectx。""",
  "https://kubernetes.io/docs/reference/kubectl/",
  "")

t("kubeadm", "kubeadm", "kubeadm", "tooling",
  ["k8s", "glossary", "kubeadm", "tooling"],
  "kubeadm 是 Kubernetes 官方提供的集群初始化和升级工具。它简化了集群的引导过程，是快速搭建 Kubernetes 集群的推荐方式。",
  """### 核心命令

| 命令 | 用途 |
|------|------|
| `kubeadm init` | 初始化控制平面节点 |
| `kubeadm join` | 将工作节点加入集群 |
| `kubeadm upgrade` | 升级集群版本 |
| `kubeadm reset` | 重置节点，移除 kubeadm 安装的组件 |
| `kubeadm token` | 管理加入令牌 |

### 初始化流程

```
kubeadm init → 生成证书 → 启动 etcd → 启动 API Server →
启动 Controller Manager → 启动 Scheduler → 安装 CNI → 就绪
```""",
  """- kubeadm 只负责引导集群，不负责 CNI 插件安装（需用户自行部署）。
- 支持配置文件（`kubeadm-config.yaml`）自定义集群参数。
- `kubeadm upgrade` 支持安全的集群版本升级。""",
  """- 学习/开发环境使用 kubeadm 快速搭建集群。
- 生产环境使用 kubeadm 配合自动化工具（Ansible/Terraform）。
- 升级前使用 `kubeadm upgrade plan` 检查兼容性。
- 始终备份 etcd 数据后再执行升级。""",
  "https://kubernetes.io/docs/reference/setup-tools/kubeadm/",
  "")

# ═══════════════════════════════════════════════════════════════════
# 10. 运维概念 operations/
# ═══════════════════════════════════════════════════════════════════

t("cordon", "封锁节点", "Cordon", "operations",
  ["k8s", "glossary", "operations", "node"],
  "Cordon 是将节点标记为不可调度的操作。被封锁的节点不会接受新的 Pod 调度，但已运行的 Pod 不受影响。",
  """### 命令

```bash
# 封锁节点
kubectl cordon <node-name>

# 查看节点状态（SchedulingDisabled 表示已封锁）
kubectl get nodes
```

### 节点状态

被封锁的节点会显示 `SchedulingDisabled` 状态，调度器不会再将 Pod 分配到该节点。""",
  """- Cordon 只影响调度，不影响已运行的 Pod。
- 节点上会添加 `node.kubernetes.io/unschedulable` 污点。
- Uncordon 可以恢复节点的可调度状态。""",
  """- 维护节点前先执行 Cordon 阻止新 Pod 调度。
- 配合 Drain 完成节点上 Pod 的安全迁移。
- 使用 `kubectl get nodes` 确认节点状态后再进行维护。""",
  "https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/",
  "")

t("drain", "驱逐", "Drain", "operations",
  ["k8s", "glossary", "operations", "node"],
  "Drain 是安全地将节点上的 Pod 迁移到其他节点的操作。它会先 Cordon 节点，然后逐个驱逐节点上的 Pod（尊重 PDB），确保应用的可用性。",
  """### 命令

```bash
# 驱逐节点（自动 cordon + 驱逐 Pod）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 常用参数
--ignore-daemonsets    # 忽略 DaemonSet Pod（它们会被重新创建）
--delete-emptydir-data # 删除使用 emptyDir 的 Pod
--force                # 强制删除不受控制器管理的 Pod
--grace-period=30      # 优雅关闭等待时间
--timeout=5m           # 驱逐超时时间
```""",
  """- Drain 会尊重 PodDisruptionBudget（PDB），不会同时驱逐过多 Pod。
- DaemonSet Pod 不会被驱逐（除非使用 `--force`）。
- 没有控制器管理的裸 Pod 不会被驱逐（除非使用 `--force`）。""",
  """- 节点维护前始终执行 Drain。
- 使用 `--ignore-daemonsets` 避免 DaemonSet Pod 阻塞 Drain。
- 监控 Drain 进度，确保 PDB 允许驱逐。
- 大规模集群中批量 Drain 时要注意 PDB 和资源容量。""",
  "https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/",
  "")

t("uncordon", "解封节点", "Uncordon", "operations",
  ["k8s", "glossary", "operations", "node"],
  "Uncordon 是恢复节点可调度状态的操作。解封后，节点可以重新接受 Pod 调度。",
  """### 命令

```bash
# 解封节点
kubectl uncordon <node-name>
```

### 使用场景

- 节点维护完成后恢复使用。
- 节点问题修复后重新加入调度。""",
  """- Uncordon 只是恢复调度能力，不会主动将 Pod 调度到该节点。
- 解封后调度器会根据集群状态自然地将 Pod 调度到该节点。""",
  """- 维护完成后确认节点健康状态再执行 Uncordon。
- 大量节点维护后分批 Uncordon，避免 Pod 大量迁移导致集群不稳定。""",
  "https://kubernetes.io/docs/reference/generated/kubectl/kubectl-commands#uncordon",
  "")

t("rolling-update", "滚动更新", "Rolling Update", "operations",
  ["k8s", "glossary", "operations", "deployment"],
  "Rolling Update（滚动更新）是 Kubernetes 中逐步替换旧版本 Pod 为新版本 Pod 的部署策略。它确保在更新过程中始终保持应用可用，实现零停机发布。",
  """### 工作原理

```
1. 创建新 ReplicaSet，启动新 Pod（数量受 maxSurge 限制）
2. 新 Pod 就绪后，减少旧 ReplicaSet 的 Pod（数量受 maxUnavailable 限制）
3. 重复步骤 1-2，直到所有 Pod 更新完成
```

### 配置参数

```yaml
strategy:
  type: RollingUpdate
  rollingUpdate:
    maxSurge: 25%        # 最多超出期望副本数的 Pod 数量
    maxUnavailable: 25%  # 最多不可用的 Pod 数量
```""",
  """- `maxSurge` 和 `maxUnavailable` 控制更新速度。
- `minReadySeconds` 确保新 Pod 稳定运行后才继续更新。
- 可以通过 `kubectl rollout pause/resume` 暂停和恢复更新。
- `kubectl rollout undo` 回滚到上一个版本。""",
  """- 生产环境使用 Rolling Update 确保零停机。
- 设置 `maxUnavailable: 0` 实现严格可用（更新更慢但更安全）。
- 使用 `minReadySeconds` 验证新 Pod 稳定性。
- 配合 Readiness Probe 确保新 Pod 就绪后才继续。""",
  "https://kubernetes.io/docs/concepts/workloads/controllers/deployment/#rolling-update-deployment",
  "")

t("rollback", "回滚", "Rollback", "operations",
  ["k8s", "glossary", "operations", "deployment"],
  "Rollback（回滚）是将 Deployment 恢复到之前版本的操作。当新版本出现问题时，可以快速回退到已知的工作版本。",
  """### 命令

```bash
# 查看更新历史
kubectl rollout history deployment/<name>

# 回滚到上一个版本
kubectl rollout undo deployment/<name>

# 回滚到指定版本
kubectl rollout undo deployment/<name> --to-revision=3

# 查看回滚状态
kubectl rollout status deployment/<name>
```""",
  """- `revisionHistoryLimit` 控制保留的历史 ReplicaSet 数量（默认 10）。
- 超出 `revisionHistoryLimit` 的旧 ReplicaSet 会被删除，无法回滚。
- 回滚操作本身也会创建一个新的 revision。""",
  """- 设置合理的 `revisionHistoryLimit`（生产建议 5-10）。
- 更新前记录变更内容，便于排查需要回滚的原因。
- 回滚后验证应用功能和指标是否正常。""",
  "https://kubernetes.io/docs/concepts/workloads/controllers/deployment/#rolling-back-a-deployment",
  "")

t("scale", "扩缩容", "Scale", "operations",
  ["k8s", "glossary", "operations", "autoscaling"],
  "Scale（扩缩容）是调整 Kubernetes 工作负载副本数量的操作。包括手动扩缩容和基于指标的自动扩缩容。",
  """### 扩缩容方式

#### 手动扩缩容

```bash
# 设置副本数
kubectl scale deployment/my-app --replicas=5

# 通过 apply 修改
kubectl apply -f deployment.yaml
```

#### 自动扩缩容

- **HPA（Horizontal Pod Autoscaler）**：基于 CPU/内存/自定义指标自动调整 Pod 副本数。
- **VPA（Vertical Pod Autoscaler）**：自动调整 Pod 的资源 Request/Limit。
- **Cluster Autoscaler / Karpenter**：自动调整节点数量。

### HPA 示例

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```""",
  """- HPA 依赖 Metrics Server 提供的指标数据。
- HPA 的扩缩容有冷却时间（默认缩容 5 分钟，扩容 3 分钟）。
- PDB 限制缩容时的最小可用 Pod 数。""",
  """- 为生产服务配置 HPA 实现自动扩缩容。
- 设置合理的 minReplicas 和 maxReplicas 边界。
- 使用自定义指标（如 QPS、延迟）实现更精准的扩缩容。
- 定期审查 ResourceQuota 确保有足够空间扩容。""",
  "https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/",
  "")

# ═══════════════════════════════════════════════════════════════════
# 11. 监控与日志 observability/
# ═══════════════════════════════════════════════════════════════════

t("metrics-server", "指标服务器", "Metrics Server", "observability",
  ["k8s", "glossary", "observability", "metrics"],
  "Metrics Server 是 Kubernetes 集群的资源指标聚合器，收集节点和 Pod 的 CPU/内存使用数据。它是 HPA、VPA 和 `kubectl top` 命令的数据源。",
  """### 核心功能

- **节点指标**：CPU 和内存使用率。
- **Pod 指标**：每个 Pod/容器的 CPU 和内存使用率。
- **API 暴露**：通过 `metrics.k8s.io` API Group 提供指标数据。

### 使用方式

```bash
# 查看节点资源使用
kubectl top nodes

# 查看 Pod 资源使用
kubectl top pods -n <namespace>

# 查看容器级别
kubectl top pods -n <namespace> --containers
```""",
  """- Metrics Server 不是 Kubernetes 核心组件，需要单独安装。
- 数据每 15 秒采集一次（可配置）。
- 仅保留最近的数据点，不提供历史查询。
- 使用 Summary API 从 kubelet 获取指标。""",
  """- 每个集群都应部署 Metrics Server（HPA 依赖）。
- 配置 `--kubelet-insecure-tls` 仅用于开发环境。
- 生产环境需要配置正确的 TLS 证书。
- 监控 Metrics Server 自身的可用性和延迟。""",
  "https://github.com/kubernetes-sigs/metrics-server",
  "")

t("prometheus", "Prometheus", "Prometheus", "observability",
  ["k8s", "glossary", "observability", "prometheus", "monitoring"],
  "Prometheus 是 CNCF 毕业项目，是 Kubernetes 生态中最主流的监控系统。它采用 Pull 模型采集指标数据，支持强大的 PromQL 查询语言和告警机制。",
  """### 核心架构

- **Prometheus Server**：采集和存储时间序列指标数据。
- **ServiceMonitor/PodMonitor**：定义采集目标（通过 Prometheus Operator CRD）。
- **Alertmanager**：处理告警规则，发送通知（邮件/Slack/PagerDuty 等）。
- **Grafana**：可视化指标数据的仪表盘工具。
- **PromQL**：Prometheus 查询语言，支持复杂的指标计算。

### 在 Kubernetes 中的集成

- **kube-prometheus-stack**：一键部署 Prometheus + Grafana + Alertmanager 的 Helm Chart。
- **Prometheus Operator**：通过 CRD 声明式管理 Prometheus 实例和采集规则。""",
  """- Prometheus 使用 Pull 模型通过 HTTP 抓取 `/metrics` 端点。
- 指标数据以时间序列存储，每个序列由 metric name + labels 标识。
- 支持 Recording Rules 预计算常用查询。
- Federation 支持多 Prometheus 实例的指标聚合。""",
  """- 生产环境使用 Prometheus Operator 管理 Prometheus 实例。
- 配置合理的 scrape_interval（默认 15s，关键指标可调为 5s）。
- 使用 Recording Rules 优化频繁使用的 PromQL 查询。
- 实施告警分级，避免告警疲劳。""",
  "https://prometheus.io/docs/",
  "[[entities/prometheus|Prometheus]]")

t("grafana", "Grafana", "Grafana", "observability",
  ["k8s", "glossary", "observability", "grafana", "monitoring"],
  "Grafana 是开源的数据可视化平台，支持丰富的图表类型和数据源集成。在 Kubernetes 生态中，Grafana 通常与 Prometheus 搭配使用，用于展示监控指标和构建运维仪表盘。",
  """### 核心功能

- **仪表盘**：支持时间序列图、表格、热力图、拓扑图等多种可视化。
- **数据源**：Prometheus、Loki、Tempo、Elasticsearch、MySQL 等 100+ 数据源。
- **告警**：内置告警引擎，支持多通道通知。
- **Dashboard as Code**：仪表盘可以用 JSON 定义并纳入版本控制。

### Kubernetes 常用仪表盘

- Kubernetes / Compute Resources / Cluster
- Kubernetes / Compute Resources / Namespace (Pods)
- Node Exporter / Nodes
- CoreDNS
- kube-state-metrics""",
  """- Grafana 支持 Provisioning 自动化配置数据源和仪表盘。
- 社区提供大量预制的 Kubernetes 仪表盘（Grafana Labs 官方库）。
- Grafana OnCall 集成告警管理和值班调度。""",
  """- 使用 kube-prometheus-stack 一键部署 Grafana + Prometheus。
- 导入社区推荐的 Kubernetes 仪表盘。
- 配置 Provisioning 自动化仪表盘管理。
- 设置关键 SLI 的告警仪表盘。""",
  "https://grafana.com/docs/",
  "")

t("alertmanager", "告警管理器", "Alertmanager", "observability",
  ["k8s", "glossary", "observability", "alertmanager"],
  "Alertmanager 是 Prometheus 生态中的告警处理组件。它接收来自 Prometheus 的告警，执行分组、抑制、静默和路由逻辑，最终通过多种渠道发送通知。",
  """### 核心功能

- **分组（Grouping）**：将相关告警合并为一条通知。
- **抑制（Inhibition）**：当某个告警触发时，抑制相关的衍生告警。
- **静默（Silencing）**：临时静默特定告警（维护期间使用）。
- **路由（Routing）**：基于标签将告警发送到不同的通知渠道。

### 通知渠道

支持 Email、Slack、PagerDuty、Webhook、OpsGenie、VictorOps 等。""",
  """- 告警规则定义在 Prometheus 中，告警处理在 Alertmanager 中。
- 支持多 Alertmanager 实例的集群模式（去重）。
- `amtool` CLI 工具用于管理静默和查看告警。""",
  """- 配置合理的分组规则避免告警风暴。
- 使用抑制规则减少噪音告警。
- 为 P0 告警配置 PagerDuty/电话通知。
- 定期审查告警规则，清理无效告警。""",
  "https://prometheus.io/docs/alerting/latest/alertmanager/",
  "")

# ═══════════════════════════════════════════════════════════════════
# 12. 安全概念 security/
# ═══════════════════════════════════════════════════════════════════

t("admission-controller", "准入控制器", "Admission Controller", "security",
  ["k8s", "glossary", "security", "admission"],
  "Admission Controller 是 Kubernetes API Server 中的插件机制，在对象持久化之前拦截和处理 API 请求。它可以验证和修改请求中的对象，是实施集群策略和安全控制的关键组件。",
  """### 类型

- **Validating（验证型）**：只验证请求是否合规，不修改对象。如 `ValidatingAdmissionWebhook`。
- **Mutating（变更型）**：可以修改请求中的对象。如 `MutatingAdmissionWebhook`。

### 内置准入控制器

- **LimitRanger**：检查资源是否超出 LimitRange。
- **ResourceQuota**：检查资源是否超出 ResourceQuota。
- **PodSecurity**：强制执行 Pod 安全标准（替代 PSP）。
- **NodeRestriction**：限制 kubelet 可以修改的 API 对象。
- **AlwaysPullImages**：强制每次都拉取镜像。""",
  """- 准入控制链：Mutating → Object Validation → Validating。
- Mutating 控制器可以修改对象，可能需要多次执行（收敛）。
- Webhook 超时或失败时，`failurePolicy` 决定是拒绝（Fail）还是允许（Ignore）。""",
  """- 使用 `ValidatingAdmissionWebhook` 实施自定义策略（如镜像白名单）。
- 使用 OPA Gatekeeper 或 Kyverno 实现声明式策略管理。
- 配置 Webhook 的 `failurePolicy: Ignore` 避免 Webhook 故障导致集群不可用。
- 为 Webhook 配置 `namespaceSelector` 排除系统命名空间。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/admission-controllers/",
  "[[domain-17-system-foundation/topic-dictionary/platform-engineering/admission-webhook-good-practices|Admission Webhook 最佳实践]]")

t("webhook", "Webhook", "Webhook", "security",
  ["k8s", "glossary", "security", "webhook"],
  "Webhook 是 Kubernetes 中允许外部服务介入 API 请求处理流程的回调机制。通过 Webhook，可以将认证、授权和准入控制逻辑委托给外部服务。",
  """### Webhook 类型

- **MutatingAdmissionWebhook**：在对象持久化前修改对象。
- **ValidatingAdmissionWebhook**：在对象持久化前验证对象。
- **Authentication Webhook**：自定义认证逻辑（Token Review）。
- **Authorization Webhook**：自定义授权逻辑（SubjectAccessReview）。

### 工作原理

```
API Request → API Server → Webhook (HTTPS) → External Service → Response
```""",
  """- Webhook 服务需要通过 TLS 加密通信。
- 支持 `caBundle` 或 `service` 引用配置 Webhook 服务。
- Webhook 的性能直接影响 API Server 的请求延迟。""",
  """- 实现 Webhook 时确保低延迟和高可用。
- 配置合理的超时时间（默认 10 秒）。
- 使用 `namespaceSelector` 限制 Webhook 的作用范围。
- 测试 Webhook 的故障场景（超时/不可达）。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/webhook/",
  "")

t("pod-security-standards", "Pod 安全标准", "Pod Security Standards (PSS)", "security",
  ["k8s", "glossary", "security", "pss"],
  "Pod Security Standards（PSS）是 Kubernetes 定义的一组 Pod 安全配置级别，用于替代已弃用的 Pod Security Policy（PSP）。它定义了三个递进的安全级别。",
  """### 三个安全级别

| 级别 | 描述 | 安全程度 |
|------|------|---------|
| **Privileged** | 无限制，允许所有权限 | 最低 |
| **Baseline** | 最小限制，阻止已知的特权提升 | 中等 |
| **Restricted** | 严格限制，遵循当前 Pod 安全最佳实践 | 最高 |

### 实施方式

通过 **Pod Security Admission**（内置准入控制器）实施：

```yaml
# 命名空间标签控制
apiVersion: v1
kind: Namespace
metadata:
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```""",
  """- Pod Security Admission 从 K8s v1.25 起替代 PSP。
- 支持三种模式：`enforce`（强制执行）、`audit`（审计记录）、`warn`（警告用户）。
- 可以为不同模式设置不同的级别（如 enforce=baseline, audit=restricted）。""",
  """- 新集群直接使用 Restricted 级别。
- 已有集群从 Baseline 开始，逐步迁移到 Restricted。
- 使用 `audit` 和 `warn` 模式先评估影响再 `enforce`。
- 为系统命名空间添加豁免标签。""",
  "https://kubernetes.io/docs/concepts/security/pod-security-standards/",
  "")

t("security-context", "安全上下文", "SecurityContext", "security",
  ["k8s", "glossary", "security", "security-context"],
  "SecurityContext 是 Kubernetes 中为 Pod 或容器定义安全配置的字段集合。它控制容器的权限、身份和安全行为，是实现容器安全加固的核心机制。",
  """### 关键配置

```yaml
securityContext:
  runAsNonRoot: true           # 禁止以 root 用户运行
  runAsUser: 1000              # 指定 UID
  runAsGroup: 3000             # 指定 GID
  fsGroup: 2000                # 挂载卷的文件组
  readOnlyRootFilesystem: true # 只读根文件系统
  allowPrivilegeEscalation: false # 禁止提权
  capabilities:
    drop: ["ALL"]              # 删除所有 Linux 能力
  seccompProfile:
    type: RuntimeDefault       # 使用默认 seccomp 配置
```

### Pod 级 vs 容器级

- **Pod SecurityContext**：应用于 Pod 中所有容器和卷。
- **Container SecurityContext**：仅应用于特定容器，可覆盖 Pod 级设置。""",
  """- `allowPrivilegeEscalation: false` 阻止 setuid/setgid 二进制提权。
- `capabilities.drop: ALL` 移除所有 Linux 能力，按需添加。
- `seccompProfile` 限制容器可以执行的系统调用。
- `AppArmor` / `SELinux` 提供额外的 MAC（强制访问控制）层。""",
  """- 所有生产容器都应配置 SecurityContext。
- 始终设置 `runAsNonRoot: true` 和 `readOnlyRootFilesystem: true`。
- 使用 `capabilities.drop: ALL` 并根据需要添加最小能力。
- 配合 Pod Security Standards 的 Restricted 级别强制执行。""",
  "https://kubernetes.io/docs/tasks/configure-pod-container/security-context/",
  "")

t("service-account-token", "服务账号令牌", "ServiceAccount Token", "security",
  ["k8s", "glossary", "security", "service-account"],
  "ServiceAccount Token 是 Kubernetes 为 Pod 自动颁发的认证令牌，允许 Pod 向 API Server 证明身份。从 K8s v1.21 起使用 TokenRequest API 颁发有界、过期的 Token。",
  """### Token 特性

- **有界（Bound）**：Token 绑定到特定的 Pod 和 ServiceAccount。
- **过期（Expiring）**：默认 1 小时过期，kubelet 自动轮转。
- **受众限制（Audience-restricted）**：Token 只能用于特定的 API 受众。

### Token 注入

kubelet 通过 Projected Volume 自动将 Token 注入 Pod：

```yaml
# 自动注入（无需手动配置）
volumes:
- name: kube-api-access
  projected:
    sources:
    - serviceAccountToken:
        expirationSeconds: 3600
        path: token
```""",
  """- 旧版 Secret-based Token（非过期）已弃用。
- TokenRequest API 提供短期、有界的 Token。
- `automountServiceAccountToken: false` 可以禁用自动 Token 注入。""",
  """- 不需要 API 访问的 Pod 禁用自动 Token 挂载。
- 使用 TokenRequest API 为外部服务生成短期 Token。
- 审计 ServiceAccount Token 的使用情况。""",
  "https://kubernetes.io/docs/reference/access-authn-authz/service-accounts-admin/#tokenrequest-api",
  "")

t("certificate", "证书", "Certificate", "security",
  ["k8s", "glossary", "security", "certificate", "tls"],
  "Certificate（证书）是 Kubernetes 中用于 TLS 加密通信的数字凭证。集群内部组件之间的通信、API Server 的 HTTPS 端点和 Ingress TLS 终止都依赖证书。",
  """### 证书用途

- **组件间通信**：API Server、etcd、kubelet 之间的 mTLS。
- **API Server HTTPS**：对外提供 HTTPS 服务。
- **Ingress TLS**：终止 HTTPS 流量。
- **Webhook**：准入 Webhook 的 TLS 认证。

### 证书管理方式

- **kubeadm 自动生成**：集群初始化时自动生成所有证书。
- **cert-manager**：CNCF 项目，自动化证书生命周期管理。
- **手动管理**：使用 cfssl 或 openssl 生成。""",
  """- Kubernetes 使用 PKI（Public Key Infrastructure）管理证书。
- CA（Certificate Authority）是根证书，签发其他证书。
- 证书有有效期，需要定期轮转。
- cert-manager 支持 Let's Encrypt、Vault 等多种 Issuer。""",
  """- 使用 cert-manager 自动化证书管理（推荐）。
- 监控证书过期时间，设置过期前告警。
- 为 Ingress TLS 使用 Let's Encrypt 免费证书。
- 定期检查证书链完整性。""",
  "https://kubernetes.io/docs/tasks/tls/managing-tls-in-a-cluster/",
  "")

# ═══════════════════════════════════════════════════════════════════
# 13. 网络概念 networking/
# ═══════════════════════════════════════════════════════════════════

t("clusterip", "集群 IP", "ClusterIP", "networking",
  ["k8s", "glossary", "networking", "service"],
  "ClusterIP 是 Kubernetes Service 的默认类型，为 Service 分配一个集群内部的虚拟 IP 地址。只有集群内部的 Pod 可以通过 ClusterIP 访问该 Service。",
  """### 核心概念

- **虚拟 IP**：ClusterIP 不绑定任何网络接口，由 kube-proxy 通过 iptables/IPVS 规则实现流量转发。
- **分配范围**：由 `--service-cluster-ip-range` 参数指定（如 `10.96.0.0/12`）。
- **无头服务**：设置 `clusterIP: None` 创建 Headless Service，DNS 直接返回后端 Pod IP。

### 使用场景

- 集群内部服务间的通信（如 API 服务调用数据库）。
- 不
需要外部访问的服务使用 NodePort/LoadBalancer/Ingress。""",
  """- ClusterIP 由 kube-proxy 通过 iptables/IPVS 实现，不依赖实际网络接口。
- 分配范围避免与 Pod CIDR 或 Node 网络冲突。
- Headless Service 适合 StatefulSet 和有状态应用的服务发现。""",
  """- 大多数内部服务使用 ClusterIP（默认类型）。
- 需要稳定 DNS 解析到单个 Pod 时使用 Headless Service。
- 监控 ClusterIP 分配池的使用率。""",
  "https://kubernetes.io/docs/concepts/services-networking/service/#type-clusterip",
  "")

t("nodeport", "节点端口", "NodePort", "networking",
  ["k8s", "glossary", "networking", "service"],
  "NodePort 是 Service 的一种类型，在每个节点上暴露一个固定端口（默认 30000-32767），外部流量可以通过 `NodeIP:NodePort` 访问集群内部服务。",
  """### 核心概念

- **端口范围**：默认 30000-32767，通过 `--service-node-port-range` 参数调整。
- **自动分配**：不指定 `nodePort` 时自动分配。
- **流量路径**：`客户端 → NodeIP:NodePort → kube-proxy → ClusterIP → Pod`。

### 示例

```yaml
apiVersion: v1
kind: Service
spec:
  type: NodePort
  ports:
  - port: 80
    targetPort: 8080
    nodePort: 30080
```""",
  """- NodePort 在所有节点上暴露相同端口。
- `externalTrafficPolicy: Local` 保留客户端源 IP。
- NodePort 是 LoadBalancer 的基础（LoadBalancer 类型自动创建 NodePort）。""",
  """- 开发/测试环境快速暴露服务。
- 生产环境优先使用 LoadBalancer 或 Ingress。
- 注意端口冲突和安全风险（暴露节点端口到外部）。""",
  "https://kubernetes.io/docs/concepts/services-networking/service/#type-nodeport",
  "")

t("loadbalancer", "负载均衡器", "LoadBalancer", "networking",
  ["k8s", "glossary", "networking", "service"],
  "LoadBalancer 是 Service 的一种类型，通过云厂商的负载均衡器将服务暴露到集群外部。它自动创建云平台的 LB 资源并配置外部 IP。",
  """### 工作原理

```
创建 LoadBalancer Service → CCM 调用云 API → 创建 LB → 分配外部 IP → 配置转发规则
```

### 注解

不同云厂商通过 Service 注解自定义 LB 行为：
- AWS: `service.beta.kubernetes.io/aws-load-balancer-*`
- GCP: `cloud.google.com/load-balancer-type`
- Azure: `service.beta.kubernetes.io/azure-load-balancer-*`""",
  """- 依赖 Cloud Controller Manager（CCM）和云平台 API。
- 每个 LoadBalancer Service 通常创建一个独立的 LB 实例（成本较高）。
- `loadBalancerClass`（v1.24+）支持指定自定义 LB 实现。""",
  """- 需要外部访问时使用 LoadBalancer。
- 大量服务考虑使用 Ingress/Gateway API 共享一个 LB。
- 监控 LB 的健康状态和成本。
- 使用 `--allocate-node-ports=false`（v1.24+）避免暴露 NodePort。""",
  "https://kubernetes.io/docs/concepts/services-networking/service/#loadbalancer",
  "")

t("externalname", "外部名称", "ExternalName", "networking",
  ["k8s", "glossary", "networking", "service"],
  "ExternalName 是 Service 的一种特殊类型，它将集群内部的 DNS 名称映射到外部的 DNS 名称（CNAME 记录），而不是将流量转发到 Pod。",
  """### 核心概念

```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-db
spec:
  type: ExternalName
  externalName: db.example.com
```

查询 `external-db.default.svc.cluster.local` 会返回 `db.example.com` 的 CNAME 记录。

### 使用场景

- 将外部数据库映射为集群内部名称。
- 引用其他集群中的服务。
- 渐进式迁移：从外部服务迁移到集群内部服务时，只需修改 Service 类型。""",
  """- ExternalName 不创建 Endpoints，不进行流量转发。
- CoreDNS 直接返回 CNAME 记录。
- 不支持端口映射，客户端使用 `externalName` 的默认端口。""",
  """- 使用 ExternalName 统一管理外部服务的访问方式。
- 迁移外部服务到集群内时只需修改 Service 类型。""",
  "https://kubernetes.io/docs/concepts/services-networking/service/#externalname",
  "")

t("vxlan", "VXLAN", "VXLAN", "networking",
  ["k8s", "glossary", "networking", "vxlan"],
  "VXLAN（Virtual Extensible LAN）是一种网络虚拟化技术，通过在 UDP 报文中封装二层以太网帧，实现跨三层的虚拟网络。Kubernetes CNI 插件广泛使用 VXLAN 实现 Pod 间的跨节点通信。",
  """### 核心概念

- **VTEP（VXLAN Tunnel Endpoint）**：封装和解封装的端点（通常是节点上的虚拟网络设备）。
- **VNI（VXLAN Network Identifier）**：24 位的网络标识，支持最多 1600 万个虚拟网络。
- **封装方式**：原始 Pod 数据包 → 以太网帧 → UDP（端口 4789）→ 外层 IP 包。

### 在 Kubernetes 中的应用

- **Flannel**：VXLAN 后端是最常用的模式。
- **Calico**：支持 VXLAN 封装模式。
- **Cilium**：支持 VXLAN 隧道模式。""",
  """- VXLAN 增加了约 50 字节的头部开销。
- 相比 IPIP 封装，VXLAN 支持跨三层的虚拟网络。
- 硬件卸载（checksum offload）可以提升 VXLAN 性能。""",
  """- 大规模集群中 VXLAN 的封装开销需要考虑。
- 高性能场景考虑使用 eBPF（Cilium）替代 VXLAN。
- 确保 UDP 4789 端口在节点间可达。""",
  "https://datatracker.ietf.org/doc/html/rfc7348",
  "")

# ═══════════════════════════════════════════════════════════════════
# Generator function
# ═══════════════════════════════════════════════════════════════════

def generate_file(term):
    """Generate a single glossary term markdown file."""
    target_dir = BASE / term["cat_dir"]
    target_dir.mkdir(parents=True, exist_ok=True)
    filepath = target_dir / f"{term['filename']}.md"

    if filepath.exists():
        print(f"  SKIP (exists): {filepath.relative_to(BASE.parent.parent)}")
        return False

    tags_yaml = "\n".join(f"- {t}" for t in term["tags"])

    content = f"""---
title: {term['title_zh']}
description: '{term["overview"][:80]}...'
category: dictionary
tags:
{tags_yaml}
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- {term['title_zh']} 是什么
- {term['title_en']} 详解
trigger_keywords:
{chr(10).join(f'- {kw}' for kw in dict.fromkeys([term['title_zh'], term['title_en'], 'dictionary']))}
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# {term['title_zh']}

> **英文名**: {term['title_en']}

## 概述

{term['overview']}

## 核心概念/原理

{term['core']}

## 关键机制或特性

{term['mechanism']}

## 使用场景与最佳实践

{term['use_cases_and_bp']}

## 参考链接

- [{term['title_en']} - Official Documentation]({term['refs']})

## Related

{term['related']}
"""

    filepath.write_text(content, encoding="utf-8")
    print(f"  CREATED: {filepath.relative_to(BASE.parent.parent)}")
    return True


def main():
    print(f"Generating {len(TERMS)} glossary term files...")
    print(f"Target: {BASE}\n")

    created = 0
    skipped = 0
    by_category = {}

    for term in TERMS:
        cat = term["cat_dir"]
        by_category.setdefault(cat, []).append(term["filename"])
        if generate_file(term):
            created += 1
        else:
            skipped += 1

    print(f"\n{'='*60}")
    print(f"Summary: {created} created, {skipped} skipped, {len(TERMS)} total")
    print(f"\nBy category:")
    for cat, files in sorted(by_category.items()):
        print(f"  {cat}/: {len(files)} terms")
        for f in files:
            print(f"    - {f}.md")


if __name__ == "__main__":
    main()
