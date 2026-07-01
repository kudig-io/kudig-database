#!/usr/bin/env python3
"""Generate remaining glossary term files - Round 2.

Covers:
1. k8s-glossary.md remaining terms not yet expanded (32 terms)
2. FTA fault diagnosis glossary terms (23 terms)
"""

import os
from pathlib import Path

BASE_K8S = Path(__file__).resolve().parent.parent / "domain-17-system-foundation" / "topic-dictionary"
BASE_FTA = Path(__file__).resolve().parent.parent / "domain-10-troubleshooting-diagnostics" / "topic-fta" / "glossary"

TERMS = []

def t(filename, title_zh, title_en, cat_dir, tags, base, overview, core, mechanism, use_cases_bp, refs, related=""):
    TERMS.append({
        "filename": filename, "title_zh": title_zh, "title_en": title_en,
        "cat_dir": cat_dir, "tags": tags, "base": base,
        "overview": overview, "core": core, "mechanism": mechanism,
        "use_cases_bp": use_cases_bp, "refs": refs, "related": related,
    })

# ═══════════════════════════════════════════════════════════════════
# k8s-glossary 剩余术语
# ═══════════════════════════════════════════════════════════════════

t("worker-node", "工作节点", "Worker Node", "fundamentals",
  ["k8s", "glossary", "node", "worker"], BASE_K8S,
  "Worker Node（工作节点）是 Kubernetes 集群中运行用户工作负载的机器。它是集群中实际执行 Pod 的节点，通常数量远多于控制平面节点。",
  """### 核心特性
- 运行 kubelet、kube-proxy 和容器运行时。
- 通过标签标识角色（如 `node-role.kubernetes.io/worker`）。
- 由控制平面管理，但本身不运行控制平面组件。

### 与控制平面节点的对比

| 特性 | Worker Node | Control Plane Node |
|------|------------|-------------------|
| 运行组件 | kubelet, kube-proxy, 容器 | API Server, etcd, Scheduler |
| 主要职责 | 运行用户工作负载 | 管理集群状态 |
| 数量 | 可水平扩展（数十至数千） | 通常 3-5 个（高可用） |
| 污点 | 通常无 | 有控制平面污点 |""",
  """- Worker Node 通过 kubelet 向 API Server 注册自身。
- 节点容量（Capacity）和可分配资源（Allocatable）决定了可运行的 Pod 数量。
- `--max-pods` 参数限制单节点最大 Pod 数（默认 110）。""",
  """- 根据工作负载类型对 Worker Node 进行分类（如 GPU 节点、高内存节点）。
- 使用节点池（Node Pool）管理不同规格的 Worker Node。
- 监控 Worker Node 的资源利用率和 Pod 密度。
- 配置节点自动扩缩容（Cluster Autoscaler / Karpenter）应对负载波动。""",
  "https://kubernetes.io/docs/concepts/architecture/nodes/")

t("master-node", "主节点", "Master Node / Control Plane Node", "fundamentals",
  ["k8s", "glossary", "control-plane", "node"], BASE_K8S,
  "Master Node（主节点）是运行 Kubernetes 控制平面组件的节点。在现代 Kubernetes 术语中，更推荐使用 Control Plane Node 来称呼。主节点负责集群的管理、调度和状态维护。",
  """### 运行的组件
- **kube-apiserver**：集群 API 入口。
- **etcd**：集群状态存储。
- **kube-scheduler**：Pod 调度决策。
- **kube-controller-manager**：控制器运行。
- **cloud-controller-manager**（可选）：云平台集成。

### 高可用部署
生产环境通常部署 3 或 5 个控制平面节点：
- 3 节点：容忍 1 个节点故障。
- 5 节点：容忍 2 个节点故障。
- 通过 `node-role.kubernetes.io/control-plane` 标签和 NoSchedule 污点隔离。""",
  """- 从 K8s v1.20 起，官方弃用 `master` 术语，改用 `control-plane`。
- kubeadm 初始化的控制平面节点自动添加污点 `node-role.kubernetes.io/control-plane:NoSchedule`。
- 控制平面节点可以是专用的（dedicated）或与 Worker Node 共享（不推荐生产）。""",
  """- 生产环境使用专用的控制平面节点。
- 控制平面节点应部署在不同的故障域（可用区）。
- 监控控制平面节点的资源使用和 etcd 健康状态。""",
  "https://kubernetes.io/docs/concepts/architecture/")

t("topology", "拓扑", "Topology", "scheduling",
  ["k8s", "glossary", "scheduling", "topology"], BASE_K8S,
  "Topology（拓扑）在 Kubernetes 中表示节点在物理或逻辑上的位置关系，如区域（Region）、可用区（Zone）、机架（Rack）等。拓扑信息用于调度决策，以实现高可用和故障隔离。",
  """### 拓扑域

Kubernetes 通过标签表示拓扑信息：

| 标签 | 含义 | 示例 |
|------|------|------|
| `topology.kubernetes.io/region` | 区域 | us-east-1 |
| `topology.kubernetes.io/zone` | 可用区 | us-east-1a |
| `kubernetes.io/hostname` | 节点 | node-1 |
| `topology.kubernetes.io/node` | 节点（推荐） | node-1 |

### 拓扑感知调度

调度器利用拓扑信息实现：
- **Pod 拓扑分布约束**（topologySpreadConstraints）：控制 Pod 在拓扑域间的均匀分布。
- **拓扑感知路由**（Topology Aware Routing）：将流量路由到同拓扑域的后端。
- **存储拓扑感知**：将 PVC 绑定到与 Pod 同区域的存储卷。""",
  """- 云厂商自动为节点添加区域和可用区标签。
- 自定义拓扑域可以通过节点标签实现（如机架标签）。
- `topology.kubernetes.io/*` 标签取代了旧的 `failure-domain.beta.kubernetes.io/*`。""",
  """- 为高可用应用配置跨可用区分布。
- 使用拓扑感知路由减少跨区域流量成本。
- 在有状态应用中考虑存储与 Pod 的拓扑对齐。""",
  "https://kubernetes.io/docs/reference/labels-annotations-taints/")

t("topology-spread-constraints", "拓扑分布约束", "topologySpreadConstraints", "scheduling",
  ["k8s", "glossary", "scheduling", "topology"], BASE_K8S,
  "Pod Topology Spread Constraints（拓扑分布约束）用于控制 Pod 在集群中的分布方式，使其跨故障域（如可用区、节点）均匀分布。这是实现高可用部署的关键调度机制。",
  """### 核心参数

```yaml
topologySpreadConstraints:
- maxSkew: 1                # 最大倾斜度（拓扑域间 Pod 数量最大差值）
  topologyKey: topology.kubernetes.io/zone  # 拓扑域标签
  whenUnsatisfiable: DoNotSchedule  # 不满足时的行为
  labelSelector:
    matchLabels:
      app: web
```

### maxSkew 的含义

maxSkew=1 表示任意两个拓扑域中匹配的 Pod 数量差不超过 1。

### whenUnsatisfiable

- `DoNotSchedule`（默认）：硬性约束，不满足则不调度。
- `ScheduleAnyway`：软性约束，尽量满足但不阻止调度。""",
  """- 从 K8s v1.24 起达到 stable。
- 支持多个约束组合（如同时按 zone 和 hostname 分布）。
- `minDomains` 参数（v1.25+）指定最小拓扑域数量。
- `matchLabelKeys`（v1.27+）可以基于 Pod 标签动态分组。""",
  """- 生产服务配置跨可用区的均匀分布（maxSkew=1, topologyKey=zone）。
- 结合 Pod Anti-Affinity 实现更精细的分布控制。
- 使用 `ScheduleAnyway` 作为降级策略，避免无法满足时 Pod 无法调度。""",
  "https://kubernetes.io/docs/concepts/scheduling-eviction/topology-spread-constraints/")

t("probe", "探针", "Probe", "configuration",
  ["k8s", "glossary", "probe", "health-check"], BASE_K8S,
  "Probe（探针）是 Kubernetes 中容器健康检查机制的统称。kubelet 通过定期执行探针来检测容器的运行状态，决定容器是否需要重启、是否可以接收流量、或是否已完成启动。",
  """### 三种探针类型

| 探针 | 作用 | 失败行为 |
|------|------|---------|
| Liveness Probe | 检测容器是否存活 | 终止并重启容器 |
| Readiness Probe | 检测容器是否就绪 | 从 Service Endpoints 移除 |
| Startup Probe | 检测容器是否启动完成 | 禁用其他探针直到成功 |

### 四种探测方式

| 方式 | 说明 | 成功条件 |
|------|------|---------|
| httpGet | 发送 HTTP GET 请求 | 状态码 200-399 |
| tcpSocket | 尝试建立 TCP 连接 | 连接成功 |
| exec | 执行命令 | 退出码 0 |
| grpc | gRPC Health Check | Health 状态 SERVING |

### 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| initialDelaySeconds | 0 | 容器启动后等待时间 |
| periodSeconds | 10 | 探测间隔 |
| timeoutSeconds | 1 | 探测超时 |
| successThreshold | 1 | 连续成功次数 |
| failureThreshold | 3 | 连续失败次数判定 |""",
  """- 探针从 K8s v1.0 开始支持，Startup Probe 在 v1.20 达到 stable。
- 探针的成功和失败由 kubelet 在节点本地执行，不经过 API Server。
- 每个探针的结果会记录在 Pod 的 Events 中。""",
  """- 所有生产容器至少配置 Readiness Probe。
- 慢启动应用使用 Startup Probe 防止启动期间被误杀。
- 探针检查路径应反映应用的真实健康状态，避免过于简单的检查（如仅检查端口开放）。
- 合理设置探测间隔和阈值，平衡检测灵敏度和资源开销。""",
  "https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/#container-probes",
  "[[domain-17-system-foundation/topic-dictionary/configuration/liveness-probe|Liveness Probe]] | [[domain-17-system-foundation/topic-dictionary/configuration/readiness-probe|Readiness Probe]]")

t("api-version", "API 版本", "API Version", "platform-engineering",
  ["k8s", "glossary", "api", "platform"], BASE_K8S,
  "API Version（API 版本）是 Kubernetes API 的版本标识，表示资源的 API 演进阶段。Kubernetes 使用版本化来管理 API 的变更和兼容性。",
  """### 版本阶段

| 阶段 | 格式 | 稳定性 |
|------|------|--------|
| Alpha | `v1alpha1`, `v2beta1` | 实验性，可能随时变更 |
| Beta | `v1beta1`, `v2beta2` | 预发布，API 基本稳定 |
| Stable/GA | `v1`, `v2` | 稳定版本，保证向后兼容 |

### 版本格式

API 版本由 API Group + Version 组成：
- 核心组：`v1`（如 Pod、Service）
- 命名组：`apps/v1`（如 Deployment）、`batch/v1`（如 Job）

### API 版本转换

Kubernetes 支持同一资源的多版本存储和自动转换：
```yaml
# 存储版本（etcd 中的版本）
storage: apps/v1
# 请求版本（客户端请求的版本）
request: apps/v1beta1 → 自动转换为 v1 返回
```""",
  """- 使用 `kubectl api-versions` 查看集群支持的所有 API 版本。
- 使用 `kubectl explain <resource>.spec` 查看资源的 API 版本和字段说明。
- 已弃用的 API 版本会在后续版本中被移除。""",
  """- 始终使用 stable 版本的 API（避免 alpha/beta）。
- 升级集群前检查是否有已弃用的 API 版本在使用。
- 使用 `pluto` 或 `kubent` 工具检测已弃用的 API 版本。
- Manifest 中的 `apiVersion` 必须与资源类型匹配。""",
  "https://kubernetes.io/docs/reference/using-api/")

t("kind", "类型", "Kind", "platform-engineering",
  ["k8s", "glossary", "api", "platform"], BASE_K8S,
  "Kind（类型）是 Kubernetes 资源对象的类型标识。每个 Manifest 的 `kind` 字段指定了要创建的资源类型，如 Pod、Deployment、Service 等。",
  """### Kind 的作用

Kind 在 Manifest 中标识资源类型：

```yaml
apiVersion: apps/v1
kind: Deployment    # 资源类型
metadata:
  name: my-app
```

### Kind 分类

| 类别 | Kind 示例 |
|------|----------|
| 工作负载 | Pod, Deployment, StatefulSet, DaemonSet, Job, CronJob |
| 服务发现 | Service, Ingress, Endpoints |
| 存储 | PersistentVolume, PersistentVolumeClaim, StorageClass |
| 配置 | ConfigMap, Secret |
| 安全 | Role, ClusterRole, RoleBinding, ServiceAccount |
| 集群管理 | Namespace, Node, ResourceQuota, LimitRange |""",
  """- 每个 Kind 属于一个 API Group，通过 `apiVersion` 指定。
- 使用 `kubectl api-resources` 查看所有 Kind 及其对应的 API Group。
- CRD（CustomResourceDefinition）允许创建自定义 Kind。""",
  """- 了解常用 Kind 的 API Group 和版本。
- 使用 `kubectl explain <kind>` 查看 Kind 的详细字段说明。
- 创建 CRD 时遵循 Kind 命名规范（PascalCase，单数/复数）。""",
  "https://kubernetes.io/docs/reference/kubernetes-api/")

t("kubectx", "kubectx", "kubectx", "tooling",
  ["k8s", "glossary", "tooling", "kubectl"], BASE_K8S,
  "kubectx 是一个用于快速切换 Kubernetes 集群上下文的命令行工具。当需要管理多个集群时，kubectx 可以显著简化集群切换操作。",
  """### 核心命令

```bash
# 列出所有上下文
kubectx

# 切换到指定集群
kubectx <context-name>

# 切换到上一个集群
kubectx -

# 重命名上下文
kubectx <new-name>=<old-name>

# 删除上下文
kubectx -d <context-name>
```

### 工作原理

kubectx 操作 kubeconfig 文件（`~/.kube/config`），修改 `current-context` 字段来切换集群。""",
  """- 等价于 `kubectl config use-context`，但更简洁。
- 支持 fzf 模糊搜索（安装 fzf 后自动启用交互式选择）。
- 可与 kubens 配合使用，实现集群+命名空间的快速切换。""",
  """- 多集群环境中必备工具。
- 使用有意义的上下文名称（如 `prod-us-east`, `staging-eu`）。
- 配合 kubens 使用提升效率。""",
  "https://github.com/ahmetb/kubectx")

t("kubens", "kubens", "kubens", "tooling",
  ["k8s", "glossary", "tooling", "kubectl"], BASE_K8S,
  "kubens 是一个用于快速切换 Kubernetes 命名空间的命令行工具。它简化了在多个命名空间之间切换的操作。",
  """### 核心命令

```bash
# 列出所有命名空间
kubens

# 切换到指定命名空间
kubens <namespace>

# 切换到上一个命名空间
kubens -
```

### 工作原理

kubens 修改 kubeconfig 中当前上下文的 `namespace` 字段。""",
  """- 等价于 `kubectl config set-context --current --namespace=<ns>`。
- 支持 fzf 模糊搜索（交互式选择）。
- 与 kubectx 属于同一项目（kubectx/kubens）。""",
  """- 在频繁切换命名空间的场景中非常有用。
- 配合 kubectx 使用，实现集群+命名空间的快速切换。""",
  "https://github.com/ahmetb/kubectx")

t("k9s", "k9s", "k9s", "tooling",
  ["k8s", "glossary", "tooling", "ui"], BASE_K8S,
  "k9s 是一个基于终端的 Kubernetes 集群管理 UI 工具。它提供了实时的资源浏览、日志查看、Shell 进入和交互式操作能力，是 kubectl 的强力补充。",
  """### 核心功能

- **资源浏览**：实时查看所有 Kubernetes 资源（Pod、Service、Deployment 等）。
- **日志查看**：实时流式查看 Pod/容器日志。
- **Shell 进入**：直接进入容器 Shell。
- **编辑/删除**：交互式编辑和删除资源。
- **端口转发**：一键设置端口转发。
- **资源使用**：实时显示 CPU/内存使用率。

### 常用快捷键

| 快捷键 | 功能 |
|--------|------|
| `:pods` | 查看 Pod 列表 |
| `:svc` | 查看 Service 列表 |
| `l` | 查看日志 |
| `s` | 进入 Shell |
| `d` | Describe 资源 |
| `e` | 编辑资源 |
| `ctrl-a` | 查看所有资源 |""",
  """- k9s 使用 kubeconfig 连接集群。
- 支持插件和别名自定义。
- 配置文件位于 `~/.config/k9s/`。
- 支持只读模式（`--readonly`）。""",
  """- 日常运维和调试的必备工具。
- 使用 `--readonly` 模式防止误操作。
- 配置自定义皮肤和布局。
- 配合 stern 工具进行高级日志跟踪。""",
  "https://k9scli.io/")

t("stern", "stern", "stern", "tooling",
  ["k8s", "glossary", "tooling", "logging"], BASE_K8S,
  "stern 是一个多 Pod 日志聚合跟踪工具。它可以同时跟踪多个 Pod 的日志输出，并以不同颜色区分，非常适合调试微服务架构。",
  """### 核心命令

```bash
# 跟踪匹配名称的所有 Pod 日志
stern my-app

# 跟踪特定命名空间
stern my-app -n production

# 使用正则匹配
stern "app-.*"

# 跟踪特定容器
stern my-app -c sidecar

# 显示时间戳
stern my-app --timestamps

# 跟踪最近的日志（类似 tail -n）
stern my-app --tail 100
```""",
  """- stern 使用正则表达式匹配 Pod 名称。
- 自动发现新创建的 Pod 并加入跟踪。
- 支持 `--output` 指定输出格式（default, raw, json）。
- stern 是原项目（wercker/stern）的社区维护分支（stern/stern）。""",
  """- 调试微服务时同时跟踪多个 Pod 的日志。
- 使用 `--since` 限制日志时间范围。
- 配合 `grep` 过滤关键日志信息。
- 使用 `--template` 自定义日志格式。""",
  "https://github.com/stern/stern")

t("etcdctl", "etcdctl", "etcdctl", "tooling",
  ["k8s", "glossary", "tooling", "etcd"], BASE_K8S,
  "etcdctl 是 etcd 的官方命令行客户端工具，用于直接与 etcd 集群交互。在 Kubernetes 运维中，etcdctl 常用于集群健康检查、数据备份和恢复操作。",
  """### 核心命令

```bash
# 检查集群健康状态
etcdctl endpoint health --cluster

# 查看集群成员
etcdctl member list --write-out=table

# 查看集群状态
etcdctl endpoint status --cluster --write-out=table

# 备份数据
etcdctl snapshot save /backup/etcd-snapshot.db

# 恢复数据
etcdctl snapshot restore /backup/etcd-snapshot.db

# 查看 key（仅用于调试）
etcdctl get /registry/pods --prefix --keys-only
```

### 环境变量

```bash
export ETCDCTL_API=3
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/peer.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/peer.key
```""",
  """- etcdctl v3 是推荐版本（`ETCDCTL_API=3`）。
- 访问 Kubernetes 的 etcd 需要 TLS 证书认证。
- `snapshot save` 是备份 etcd 数据的标准方法。
- 不建议直接修改 etcd 中的 Kubernetes 数据。""",
  """- 定期使用 etcdctl 检查集群健康状态。
- 实施自动化 etcd 备份策略（每天至少一次）。
- 升级或迁移前必须执行 `snapshot save`。
- 监控 etcd 的 WAL fsync 延迟和 DB size。""",
  "https://etcd.io/docs/latest/op-guide/")

t("upgrade", "升级", "Upgrade", "operations",
  ["k8s", "glossary", "operations", "upgrade"], BASE_K8S,
  "Upgrade（升级）是指将 Kubernetes 集群或组件从旧版本升级到新版本的过程。Kubernetes 支持滚动升级策略，确保升级过程中集群持续可用。",
  """### 升级路径

Kubernetes 支持跨一个小版本升级（如 1.30 → 1.31），不支持跨多个版本。

```
推荐路径：1.30.x → 1.31.x → 1.32.x
不推荐：1.30.x → 1.32.x（跳版本）
```

### 升级顺序

```
1. 升级控制平面组件（API Server → Controller Manager → Scheduler）
2. 升级 kubelet（逐个节点 cordon + drain + 升级 + uncordon）
3. 升级 CoreDNS、kube-proxy 等系统组件
4. 验证集群健康状态
```

### kubeadm 升级

```bash
# 检查可用版本
kubeadm upgrade plan

# 升级控制平面
kubeadm upgrade apply v1.32.0

# 升级节点
kubeadm upgrade node
```""",
  """- 升级前必须备份 etcd 数据。
- 控制平面先升级，kubelet 后升级（kubelet 版本不能高于 API Server）。
- 升级期间 kubelet 版本可以比 API Server 低 2 个小版本（版本偏差策略）。
- 云厂商托管集群（EKS、ACK、GKE）通常提供自动化升级。""",
  """- 在非生产环境充分测试升级后再在生产环境执行。
- 制定详细的升级计划和回滚方案。
- 选择维护窗口执行升级，减少对业务的影响。
- 监控升级过程中的关键指标（Pod 状态、API Server 延迟、etcd 健康）。""",
  "https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-upgrade/")

t("kubernetes-events", "Kubernetes 事件", "Kubernetes Events", "observability",
  ["k8s", "glossary", "observability", "events"], BASE_K8S,
  "Kubernetes Events（事件）是集群中发生的操作和状态变更的记录。事件由各个控制器和 kubelet 生成，提供了集群运行的实时视图，是排查问题的重要信息来源。",
  """### 事件结构

每个 Event 包含：
- **Type**：`Normal` 或 `Warning`。
- **Reason**：事件原因（如 `Scheduled`, `Pulled`, `FailedScheduling`）。
- **Message**：详细描述。
- **Source**：事件来源组件。
- **Count**：事件发生次数。
- **FirstTimestamp/LastTimestamp**：首次和最后发生时间。

### 查看事件

```bash
# 查看命名空间事件
kubectl get events -n default --sort-by='.lastTimestamp'

# 查看特定 Pod 的事件
kubectl describe pod <pod-name> -n default

# 使用 fieldSelector 过滤
kubectl get events --field-selector type=Warning

# 使用 --watch 实时监控
kubectl get events -w
```""",
  """- Event 默认 TTL 为 1 小时（可通过 `--event-ttl` 调整）。
- Event 存储在 etcd 中，大量 Event 可能影响 etcd 性能。
- Event API（`events.k8s.io/v1`）替代了旧的 core/v1 Event API。
- Event 不会持久化（TTL 过期后自动删除）。""",
  """- 排查问题时首先查看相关资源的事件。
- 使用 `--field-selector type=Warning` 快速定位异常事件。
- 配置 Event 导出工具（如 k8s-event-exporter）将事件发送到外部系统。
- 使用 `kubectl get events --watch` 实时监控集群变更。""",
  "https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/event-v1/")

t("pod-security-policy", "Pod 安全策略", "Pod Security Policy (PSP)", "security",
  ["k8s", "glossary", "security", "psp"], BASE_K8S,
  "Pod Security Policy（PSP）是 Kubernetes 早期用于控制 Pod 安全配置的集群级资源。**PSP 在 K8s v1.21 中被弃用，v1.25 中被移除**，已被 Pod Security Standards（PSS）+ Pod Security Admission 替代。",
  """### PSP 的历史

- **K8s v1.3-v1.20**：PSP 是控制 Pod 安全的主要机制。
- **K8s v1.21**：PSP 被标记为弃用（deprecated）。
- **K8s v1.25**：PSP 被完全移除。

### PSP 的功能（已弃用）

PSP 可以控制：
- 特权容器（privileged）
- 宿主机命名空间（hostNetwork, hostPID, hostIPC）
- 宿主机端口范围
- 卷类型
- 文件系统组
- 用户/组范围
- 允许的能力（capabilities）
- SELinux 上下文""",
  """- PSP 是集群级资源，通过 RBAC 控制谁可以使用哪些 PSP。
- PSP 的复杂性导致难以正确配置，是弃用的主要原因之一。
- 替代方案 PSS（Pod Security Standards）通过命名空间标签实施，更简洁。""",
  """- 如果集群仍在使用 PSP（K8s < v1.25），应计划迁移到 PSS。
- 迁移步骤：1) 审计现有 PSP 规则 → 2) 映射到 PSS 级别 → 3) 在命名空间上应用 PSS 标签 → 4) 验证 → 5) 删除 PSP。
- 新集群直接使用 Pod Security Admission。""",
  "https://kubernetes.io/docs/concepts/security/pod-security-standards/",
  "[[domain-17-system-foundation/topic-dictionary/security/pod-security-standards|Pod Security Standards]]")

t("certificate-authority", "证书颁发机构", "Certificate Authority (CA)", "security",
  ["k8s", "glossary", "security", "certificate", "tls"], BASE_K8S,
  "Certificate Authority（CA，证书颁发机构）是负责签发和管理数字证书的受信任实体。在 Kubernetes 中，CA 是集群 PKI（Public Key Infrastructure）的根，所有组件间的 TLS 通信都依赖 CA 签发的证书。",
  """### Kubernetes PKI 结构

Kubernetes 集群的证书层次：

```
Root CA (ca.crt / ca.key)
├── API Server Certificate (apiserver.crt / apiserver.key)
├── API Server Kubelet Client (apiserver-kubelet-client.crt / .key)
├── Front Proxy CA (front-proxy-ca.crt / .key)
│   └── Front Proxy Client (front-proxy-client.crt / .key)
├── etcd CA (etcd/ca.crt / etcd/ca.key)
│   ├── etcd Server (etcd/server.crt / .key)
│   ├── etcd Peer (etcd/peer.crt / .key)
│   └── etcd Healthcheck Client (etcd/healthcheck-client.crt / .key)
└── ServiceAccount Key (sa.key / sa.pub)
```

### CA 文件位置（kubeadm 集群）

所有证书默认位于 `/etc/kubernetes/pki/` 目录。""",
  """- Kubernetes 使用自签名的 Root CA（非公共 CA）。
- CA 证书的有效期默认 10 年。
- 组件间通信通过验证对方的证书是否由同一个 CA 签发来建立信任。
- `--client-ca-file` 和 `--tls-cert-file` 等参数配置 API Server 的证书。""",
  """- 保护 CA 私钥的安全（限制文件权限，不提交到 Git）。
- 定期检查证书过期时间（`kubeadm certs check-expiration`）。
- 使用 cert-manager 自动化证书轮转。
- 实施证书过期监控告警。
- CA 轮转时需要所有组件同步更新证书。""",
  "https://kubernetes.io/docs/tasks/tls/managing-tls-in-a-cluster/")

t("hpa", "水平 Pod 自动扩缩容", "HPA (Horizontal Pod Autoscaler)", "scheduling",
  ["k8s", "glossary", "scheduling", "autoscaling", "hpa"], BASE_K8S,
  "HPA（Horizontal Pod Autoscaler，水平 Pod 自动扩缩容）是 Kubernetes 中根据观测到的指标自动调整 Pod 副本数量的控制器。它通过增加或减少副本数来应对负载变化。",
  """### 核心机制

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
```

### 支持的指标类型

- **Resource**：CPU/内存利用率。
- **Pods**：自定义 Pod 级指标（如 QPS）。
- **Object**：与特定对象关联的指标。
- **External**：集群外部指标。
- **Container Resource**：容器级资源指标。""",
  """- HPA 依赖 Metrics Server（Resource 指标）或 Prometheus Adapter（自定义指标）。
- 默认同步周期 15 秒（`--horizontal-pod-autoscaler-sync-period`）。
- 扩缩容有冷却时间：缩容默认 5 分钟，扩容默认 3 分钟。
- `behavior` 字段（v1.23+）提供精细的扩缩容行为控制。""",
  """- 为生产服务配置 HPA 实现自动扩缩容。
- 设置合理的 minReplicas（至少 2 保证高可用）和 maxReplicas。
- 结合自定义指标（如请求延迟、队列长度）实现更精准的扩缩。
- 配置 `behavior.scaleDown.stabilizationWindowSeconds` 防止频繁缩容抖动。
- 监控 HPA 的当前状态和扩缩容历史。""",
  "https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/")

t("vpa", "垂直 Pod 自动扩缩容", "VPA (Vertical Pod Autoscaler)", "scheduling",
  ["k8s", "glossary", "scheduling", "autoscaling", "vpa"], BASE_K8S,
  "VPA（Vertical Pod Autoscaler，垂直 Pod 自动扩缩容）是 Kubernetes 中自动调整 Pod 的资源 Request/Limit 的工具。它根据容器的历史资源使用情况推荐或自动调整资源配置。",
  """### 工作模式

| 模式 | 行为 |
|------|------|
| `Off` | 只推荐，不自动调整 |
| `Initial` | 仅在 Pod 创建时设置资源 |
| `Recreate` | 自动调整，需要重启 Pod |
| `Auto` | 自动调整（目前等同于 Recreate） |

### 示例

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Auto"
```""",
  """- VPA 是 `autoscaling.k8s.io` API Group 下的资源。
- VPA 和 HPA 不能同时作用于同一个 Pod 的同一指标（CPU/Memory）。
- VPA 使用 Recommender 组件分析历史数据并推荐资源值。
- `Recreate` 模式需要重启 Pod 以应用新资源值。""",
  """- 使用 VPA 的 `Off` 模式获取资源推荐值，手动调整后应用。
- 与 HPA 结合使用：VPA 调整单 Pod 资源，HPA 调整副本数。
- 对于不能重启的关键服务，使用 `Initial` 模式。
- 监控 VPA 推荐值与实际使用值的偏差。""",
  "https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler")

t("pdb", "Pod 中断预算", "PDB (Pod Disruption Budget)", "operations",
  ["k8s", "glossary", "operations", "pdb", "reliability"], BASE_K8S,
  "Pod Disruption Budget（PDB，Pod 中断预算）是 Kubernetes 中用于限制同时被自愿中断的 Pod 数量的策略资源。它确保在节点维护、集群升级等操作期间，应用始终保持最低可用水平。",
  """### 核心概念

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-pdb
spec:
  minAvailable: 2         # 至少保持 2 个 Pod 可用
  # 或
  maxUnavailable: 1       # 最多允许 1 个 Pod 不可用
  selector:
    matchLabels:
      app: web
```

### 自愿中断 vs 非自愿中断

- **自愿中断**：Drain、节点维护、集群升级（受 PDB 保护）。
- **非自愿中断**：节点故障、OOM Kill、驱逐（不受 PDB 保护）。

### PDB 的作用时机

PDB 在以下场景中阻止 Pod 被驱逐：
- `kubectl drain` 操作。
- 集群升级过程中的 Pod 迁移。
- Cluster Autoscaler 缩容节点。""",
  """- PDB 从 K8s v1.21 起达到 stable。
- `minAvailable` 和 `maxUnavailable` 不能同时设置。
- 支持百分比和绝对数字（如 `minAvailable: 50%`）。
- `unhealthyPodEvictionPolicy`（v1.27+）允许驱逐不健康的 Pod 即使 PDB 不满足。""",
  """- 为所有生产 Deployment/StatefulSet 配置 PDB。
- `maxUnavailable: 1` 适合大多数场景（允许 1 个 Pod 中断）。
- 确保 `minAvailable` 不超过实际副本数减 1。
- 结合滚动更新策略实现零停机部署和维护。
- 监控 PDB 的 `currentHealthy` 和 `disruptionsAllowed` 状态。""",
  "https://kubernetes.io/docs/tasks/run-application/configure-pdb/")

t("server-side-apply", "服务器端应用", "SSA (Server-Side Apply)", "platform-engineering",
  ["k8s", "glossary", "platform", "ssa"], BASE_K8S,
  "Server-Side Apply（SSA，服务器端应用）是 Kubernetes 中管理资源对象的声明式方式。与 Client-Side Apply 不同，SSA 在 API Server 端执行合并逻辑，支持多管理者（managers）协作管理同一资源。",
  """### 核心概念

- **Field Manager**：标识管理资源的客户端（如 `kubectl`, `helm`, `argocd`）。
- **Managed Fields**：记录每个字段由哪个 Manager 管理。
- **冲突检测**：当多个 Manager 修改同一字段时检测冲突。

### 使用方式

```bash
# 服务器端 Apply
kubectl apply --server-side -f deployment.yaml

# 强制覆盖冲突字段
kubectl apply --server-side --force-conflicts -f deployment.yaml

# 在 Manifest 中标识管理者
# 通过 managedFields 自动记录
```""",
  """- SSA 从 K8s v1.22 起达到 stable。
- Client-Side Apply（`kubectl apply`）使用客户端的 last-applied 注解。
- SSA 使用 `managedFields` 替代 `last-applied-configuration` 注解。
- SSA 更适合多工具协作的场景（GitOps、Operator 等）。""",
  """- 新项目和 CI/CD 流水线优先使用 `--server-side`。
- 使用 SSA 的冲突检测避免配置漂移。
- 在 Helm/Kustomize 中启用 SSA 模式。
- 迁移到 SSA 时注意清理旧的 `last-applied-configuration` 注解。""",
  "https://kubernetes.io/docs/reference/using-api/server-side-apply/")

t("cncf", "云原生计算基金会", "CNCF (Cloud Native Computing Foundation)", "fundamentals",
  ["k8s", "glossary", "cncf", "cloud-native"], BASE_K8S,
  "CNCF（Cloud Native Computing Foundation，云原生计算基金会）是 Linux Foundation 旗下的开源基金会，致力于推动云原生计算的普及。它托管了 Kubernetes、Prometheus、Envoy 等核心云原生项目。",
  """### 项目成熟度

CNCF 项目分为三个成熟度级别：

| 级别 | 含义 | 代表项目 |
|------|------|---------|
| **Sandbox** | 实验阶段 | 早期探索性项目 |
| **Incubating** | 孵化阶段 | 活跃开发，社区增长中 |
| **Graduated** | 毕业阶段 | 生产就绪，广泛采用 |

### 毕业项目（部分）

- **Kubernetes**：容器编排平台。
- **Prometheus**：监控系统。
- **Envoy**：服务代理。
- **CoreDNS**：DNS 服务。
- **containerd**：容器运行时。
- **Fluentd**：日志收集。
- **Jaeger**：分布式追踪。
- **Vitess**：MySQL 水平扩展。""",
  """- CNCF 成立于 2015 年，由 Google 捐赠 Kubernetes 项目而发起。
- 截至 2026 年，CNCF 托管 200+ 个开源项目。
- CNCF Landscape 是了解云原生生态的权威参考。
- TOC（Technical Oversight Committee）负责项目评审和技术方向。""",
  """- 技术选型时参考 CNCF 项目成熟度级别。
- 关注 CNCF 毕业项目，优先用于生产环境。
- 使用 CNCF Landscape 了解云原生生态全景。
- 关注新兴项目（如 eBPF、Wasm 相关项目）。""",
  "https://www.cncf.io/")

t("cri", "容器运行时接口", "CRI (Container Runtime Interface)", "fundamentals",
  ["k8s", "glossary", "cri", "container-runtime"], BASE_K8S,
  "CRI（Container Runtime Interface，容器运行时接口）是 Kubernetes 定义的一组 gRPC 接口标准，用于 kubelet 与容器运行时之间的通信。CRI 使 Kubernetes 能够支持多种容器运行时实现。",
  """### CRI 接口组成

| 服务 | 职责 |
|------|------|
| **RuntimeService** | 管理容器生命周期（创建、启动、停止、删除） |
| **ImageService** | 管理容器镜像（拉取、检查、删除） |

### 通信方式

```
kubelet ──gRPC──> /run/containerd/containerd.sock ──> containerd
           │
           └──> /run/crio/crio.sock ──> CRI-O
```

### 支持的运行时

- **containerd**：K8s 默认运行时。
- **CRI-O**：专为 Kubernetes 设计的最小化运行时。
- **cri-dockerd**：Docker 的 CRI 适配器（Docker 已从 K8s 1.24 起不再直接支持）。""",
  """- CRI 使用 Unix domain socket 通信。
- CRI 版本与 Kubernetes 版本有兼容性要求。
- `crictl` 是 CRI 兼容运行时的调试命令行工具。
- CRI 从 K8s v1.24 起成为唯一的运行时集成方式（移除了 dockershim）。""",
  """- 生产环境选择 containerd 或 CRI-O。
- 使用 `crictl` 调试容器和镜像。
- 确保运行时版本与 Kubernetes 版本兼容。
- 监控运行时的操作延迟和错误率。""",
  "https://kubernetes.io/docs/concepts/architecture/cri/")

# ═══════════════════════════════════════════════════════════════════
# FTA 故障诊断术语
# ═══════════════════════════════════════════════════════════════════

FTA_TERMS = [
    ("fault-tree-analysis", "故障树分析", "Fault Tree Analysis (FTA)",
     "故障树分析（FTA）是一种自顶向下的演绎式系统安全分析方法。它通过逻辑门将系统级故障（顶事件）分解为底层基本事件的组合，用于识别导致系统故障的根本原因和传播路径。",
     """### 核心思想
从一个已知的系统故障（顶事件）出发，逐层向下分析导致该故障的所有可能原因和路径，直到找到不可再分解的基本事件（根因）。
### 分析流程
1. 定义顶事件 → 2. 构建故障树 → 3. 定性分析（最小割集）→ 4. 定量分析（概率计算）→ 5. 制定改进措施""",
     "FTA 由贝尔实验室的 H.A. Watson 于 1962 年发明，最初用于民兵导弹系统的安全分析。现广泛应用于航空航天、核工业、化工和 IT 运维领域。",
     "用于分析 Kubernetes 集群故障、服务不可用根因、系统性风险识别。",
     "https://en.wikipedia.org/wiki/Fault_tree_analysis"),
    ("top-event", "顶事件", "Top Event",
     "顶事件（Top Event）是故障树最顶层的不期望事件，是整个故障树分析的起点。它代表了需要分析的系统级故障或异常状态。",
     """### 在 K8s 中的示例
- Pod 处于 CrashLoopBackOff 状态
- 集群 API Server 不可用
- Service 无法访问
- 节点 NotReady""",
     "顶事件必须是可观察的、明确的、可验证的。它定义了分析的边界和目标。",
     "在 FTA 诊断中，顶事件通常来自告警、用户反馈或监控系统检测到的异常。",
     ""),
    ("basic-event", "基本事件", "Basic Event",
     "基本事件（Basic Event）是故障树中不可再分解的最底层事件。它代表了导致上层事件发生的根本原因，是故障分析的终点。",
     """### 特征
- 不再向下分解。
- 有已知的发生概率或频率。
- 对应具体的根因（如配置错误、资源不足、网络中断）。""",
     "基本事件是制定修复和预防措施的依据。消除或降低基本事件的发生概率可以直接降低顶事件的发生概率。",
     "在 K8s FTA 中，基本事件对应具体的根因如：CPU Limit 过低、PVC 绑定失败、证书过期等。",
     ""),
    ("or-gate", "或门", "OR Gate",
     "或门（OR Gate）是故障树中的逻辑门，表示任一输入事件发生时输出事件就会发生。它代表了多种独立故障路径的汇聚。",
     """### 逻辑含义
输出 = 输入1 OR 输入2 OR ... OR 输入N
任一输入发生，输出就发生。所有输入都不发生，输出才不发生。""",
     "或门使故障概率增大（P = 1 - ∏(1-Pi)），是系统脆弱性的标志。多个独立故障路径通过或门汇聚意味着系统缺乏冗余。",
     "在 K8s 中，Service 不可用可能是因为所有后端 Pod 不可用 OR 网络不通 OR DNS 解析失败。",
     ""),
    ("and-gate", "与门", "AND Gate",
     "与门（AND Gate）是故障树中的逻辑门，表示所有输入事件同时发生时输出事件才会发生。它代表了冗余系统中的保护机制。",
     """### 逻辑含义
输出 = 输入1 AND 输入2 AND ... AND 输入N
所有输入都发生，输出才发生。任一输入不发生，输出就不发生。""",
     "与门使故障概率减小（P = ∏Pi），是冗余设计的体现。通过引入与门可以增加系统可靠性。",
     "在 K8s 中，etcd 数据丢失 = 主节点磁盘故障 AND 所有备份不可用。",
     ""),
    ("minimal-cut-set", "最小割集", "Minimal Cut Set (MCS)",
     "最小割集（MCS）是使顶事件发生的最小基本事件集合。移除集合中任何一个事件，顶事件就不再由该割集导致。MCS 是 FTA 定性分析的核心结果。",
     """### 分析意义
- 阶数为 1 的 MCS：单点故障（最危险）。
- 阶数为 2 的 MCS：双重故障才会导致系统失效。
- MCS 阶数越低，系统风险越高。""",
     "MCS 分析帮助识别系统的薄弱环节和单点故障。优先处理阶数最低的 MCS 可以最有效地提升系统可靠性。",
     "在 K8s 中，单点故障的 MCS 示例：API Server 证书过期（1阶割集）。",
     ""),
    ("mtbf", "平均问题间隔", "MTBF (Mean Time Between Failures)",
     "MTBF（Mean Time Between Failures，平均故障间隔时间）是衡量系统可靠性的核心指标，表示系统两次故障之间的平均运行时间。MTBF 越长，系统越可靠。",
     """### 计算公式
MTBF = 总运行时间 / 故障次数
MTBF = 1 / λ （λ 为故障率）""",
     "MTBF 用于评估系统组件的可靠性，指导维护计划和备件策略。",
     "在 K8s 中，可统计集群平均无故障运行天数、Pod 平均重启间隔等。",
     ""),
    ("mttr", "平均修复时间", "MTTR (Mean Time To Repair)",
     "MTTR（Mean Time To Repair，平均修复时间）是衡量系统恢复能力的核心指标，表示从故障发生到系统恢复正常的平均时间。MTTR 越短，系统恢复能力越强。",
     """### 计算公式
MTTR = 总修复时间 / 故障次数
MTTR = MTTD + 诊断时间 + 修复时间 + 验证时间""",
     "降低 MTTR 是运维工程的核心目标。通过自动化诊断（AI Agent）、预定义修复方案（Runbook）和自动修复可以显著降低 MTTR。",
     "在 K8s 中，MTTR 包括：发现告警→定位根因→执行修复→验证恢复的总时间。",
     ""),
    ("mttd", "平均检测时间", "MTTD (Mean Time To Detect)",
     "MTTD（Mean Time To Detect，平均检测时间）是从故障发生到被检测到的平均时间。缩短 MTTD 是提升系统可用性的关键。",
     """### 影响因素
- 监控覆盖度：未监控的组件故障无法被检测。
- 告警灵敏度：阈值过高导致延迟检测。
- 检测手段：主动探测 vs 被动告警。""",
     "通过完善监控覆盖、优化告警阈值和引入主动健康检查可以缩短 MTTD。",
     "在 K8s 中，使用 Prometheus 告警、Liveness/Readiness Probe 和 SLO 监控来缩短 MTTD。",
     ""),
    ("availability", "可用性", "Availability",
     "可用性（Availability）是系统正常运行时间占总时间的比例，通常以百分比表示。它是系统可靠性和恢复能力的综合指标。",
     """### 计算公式
A = MTBF / (MTBF + MTTR) × 100%
A = 正常运行时间 / (正常运行时间 + 故障时间) × 100%""",
     "### 可用性等级\n| 等级 | 可用性 | 年停机时间 |\n|------|--------|----------|\n| 99% | 2个9 | 3.65天 |\n| 99.9% | 3个9 | 8.77小时 |\n| 99.99% | 4个9 | 52.6分钟 |\n| 99.999% | 5个9 | 5.26分钟 |",
     "生产系统通常要求至少 3 个 9（99.9%）的可用性。K8s 通过自愈、多副本和高可用架构来保障可用性。",
     ""),
    ("fmea", "故障模式与影响分析", "FMEA (Failure Mode and Effects Analysis)",
     "FMEA（Failure Mode and Effects Analysis，故障模式与影响分析）是一种自底向上的归纳式分析方法。它系统地识别系统中每个组件的潜在故障模式，评估其对系统的影响，并制定预防措施。",
     """### 分析步骤
1. 列出系统所有组件。
2. 识别每个组件的故障模式。
3. 评估每个故障模式的影响（严重度 S、发生频率 O、可检测性 D）。
4. 计算 RPN（风险优先级数）= S × O × D。
5. 按 RPN 排序，优先处理高风险项。""",
     "FMEA 与 FTA 互为补充：FTA 是自顶向下演绎，FMEA 是自底向上归纳。两者结合可以全面覆盖系统风险。",
     "在 K8s 中，FMEA 可用于分析每个组件（API Server、etcd、kubelet 等）的故障模式和影响。",
     ""),
    ("rpn", "风险优先级数", "RPN (Risk Priority Number)",
     "RPN（Risk Priority Number，风险优先级数）是 FMEA 中用于量化风险的指标。它由严重度、发生频率和可检测性三个维度的乘积组成。",
     """### 计算公式
RPN = S × O × D
- S (Severity)：严重度（1-10，10 最严重）
- O (Occurrence)：发生频率（1-10，10 最频繁）
- D (Detection)：可检测性（1-10，10 最难检测）
RPN 范围：1-1000""",
     "RPN 用于对故障模式进行优先级排序。高 RPN 值的故障模式应优先处理。但需注意：即使 RPN 中等，严重度极高的故障也应优先处理。",
     "在 K8s 运维中，用 RPN 评估不同故障场景的风险等级，指导应急预案和资源投入。",
     ""),
    ("common-cause-failure", "共因故障", "Common Cause Failure (CCF)",
     "共因故障（CCF）是由同一个根因导致的多个组件同时故障。共因故障会破坏冗余设计的有效性，是系统可靠性分析中需要特别关注的风险。",
     """### 典型场景
- 同一机架的多个节点因交换机故障同时掉线。
- 同一容器镜像的 Bug 导致所有副本同时崩溃。
- 同一可用区的所有实例因区域级故障同时不可用。""",
     "共因故障是冗余系统失效的主要原因。通过多样化（不同厂商、不同版本、不同区域）可以降低共因故障风险。",
     "在 K8s 中，防止共因故障：多可用区部署、不同节点池使用不同实例类型、避免所有 Pod 使用同一镜像 tag。",
     ""),
    ("voting-gate", "投票门", "Voting Gate (k/n)",
     "投票门（Voting Gate）是故障树中的逻辑门，表示 n 个输入事件中至少 k 个发生时输出事件才发生。它是 AND 门和 OR 门的泛化形式。",
     """### 特殊情况
- k=1：等同于 OR 门（任一输入即触发）。
- k=n：等同于 AND 门（全部输入才触发）。
- k/n：n 中取 k 的表决逻辑。""",
     "投票门常用于分析冗余系统的降级模式（如 3 个节点中 2 个故障时系统不可用 = 2/3 投票门）。",
     "在 K8s 中，etcd 集群在 3 节点中有 2 个故障时不可用（2/3 投票门）。",
     ""),
    ("importance-measure", "重要度", "Importance Measure",
     "重要度（Importance Measure）衡量基本事件对顶事件的影响程度。它是 FTA 定量分析的关键指标，用于确定哪些基本事件最值得改进。",
     """### 常见重要度指标
- **Birnbaum 重要度**：基本事件状态改变对顶事件概率的影响。
- **Fussell-Vesely 重要度**：基本事件参与的割集对顶事件的贡献比例。
- **关键度（Criticality）**：Birnbaum 重要度 × 基本事件概率 / 顶事件概率。""",
     "重要度分析帮助确定资源投入的优先级。重要度最高的基本事件应优先改进，可以最有效地降低顶事件发生概率。",
     "在 K8s FTA 中，重要度分析可以识别最影响系统可用性的根因事件。",
     ""),
    ("failure-rate", "问题率", "Failure Rate (λ)",
     "问题率（Failure Rate，λ）是单位时间内系统或组件发生故障的概率。它是可靠性工程的基础参数。",
     """### 计算公式
λ = 故障次数 / 总运行时间
λ = 1 / MTBF""",
     "问题率通常遵循浴盆曲线（Bathtub Curve）：早期故障期（高λ）→ 稳定期（低λ）→ 耗损期（λ上升）。",
     "在 K8s 中，可统计各组件（API Server、etcd、kubelet）的问题率，识别不稳定组件。",
     ""),
    ("reliability", "可靠度", "Reliability R(t)",
     "可靠度（Reliability，R(t)）是系统在时间 t 内无故障运行的概率。它是时间相关的可靠性指标。",
     """### 计算公式
R(t) = e^(-λt)（指数分布假设）
R(t) = 1 - F(t)（F(t) 为累积故障分布函数）""",
     "可靠度随时间递减。系统的整体可靠度取决于各组件可靠度的组合（串联/并联）。",
     "在 K8s 中，评估集群在特定时间段内的可靠运行概率，指导维护计划。",
     ""),
    ("cut-set-order", "割集阶数", "Cut Set Order",
     "割集阶数（Cut Set Order）是最小割集中包含的基本事件数量。阶数越低，系统越脆弱。",
     """### 风险等级
- **阶数 1**：单点故障（最危险），一个事件就能导致系统故障。
- **阶数 2**：双重故障，两个事件同时发生才导致系统故障。
- **阶数 3+**：需要多个事件同时发生，概率较低。""",
     "消除阶数为 1 的最小割集（单点故障）是提升系统可靠性的首要目标。",
     "在 K8s 中，API Server 单实例部署是 1 阶割集（单点故障），应通过多副本消除。",
     ""),
    ("house-event", "外部事件", "House Event",
     "外部事件（House Event）是故障树中表示正常预期会发生的事件。它不是故障，而是作为条件或触发器存在于故障树中。",
     """### 用途
- 表示系统运行模式的切换。
- 表示计划内的维护操作。
- 作为逻辑门的条件输入。""",
     "House Event 用于简化故障树建模，将正常操作与故障事件区分开来。",
     "在 K8s 中，节点维护窗口、计划性升级等可以作为 House Event 建模。",
     ""),
    ("undeveloped-event", "未展开事件", "Undeveloped Event",
     "未展开事件（Undeveloped Event）是故障树中暂未分解到底的事件。它表示该分支的分析尚未完成，需要在后续分析中继续展开。",
     """### 使用场景
- 分析时间和资源有限时，先标记后展开。
- 某些分支的影响较小，暂不深入。
- 需要更多信息才能继续分析的事件。""",
     "未展开事件应在故障树中明确标记，并在后续迭代中逐步完善。",
     "在 K8s FTA 中，对于不确定的故障路径可先标记为未展开事件。",
     ""),
    ("inhibit-gate", "抑制门", "Inhibit Gate",
     "抑制门（Inhibit Gate）是带条件约束的 AND 门。输出事件仅在输入事件和条件事件同时发生时才发生。",
     """### 逻辑含义
输出 = 输入事件 AND 条件事件
条件事件不是故障，而是使故障生效的外部条件。""",
     "抑制门用于建模条件性故障：只有在特定条件下，故障才会导致上层事件。",
     "在 K8s 中，Pod 调度失败 = 资源不足 AND 没有配置 PriorityClass（条件）。",
     ""),
    ("priority-and-gate", "优先与门", "Priority AND Gate (PAND)",
     "优先与门（PAND）是按时序发生的 AND 门。输出事件仅在输入事件按指定顺序发生时才发生。",
     """### 逻辑含义
输出 = 输入1 先发生 THEN 输入2 发生
事件发生的顺序很重要。""",
     "PAND 用于分析时序敏感的故障场景：某些故障只在特定操作顺序下才会导致问题。",
     "在 K8s 中，数据丢失 = 先删除 PVC 再执行备份（顺序敏感）。",
     ""),
    ("transfer-symbol", "转移符号", "Transfer Symbol",
     "转移符号（Transfer Symbol）是故障树中的跨页连接标记。当故障树过大无法在一页内展示时，使用转移符号将子树连接到其他页面或模块。",
     """### 类型
- **转入（Transfer In）**：三角形 + 标签，引用其他位置的子树。
- **转出（Transfer Out）**：三角形 + 标签，定义可被引用的子树。""",
     "转移符号使大型故障树可以模块化，便于团队协作和分阶段构建。",
     "在 K8s FTA 中，各领域的故障树（网络、存储、调度）可以通过转移符号互联。",
     ""),
]

def generate_fta_file(term):
    """Generate a FTA glossary term file."""
    BASE_FTA.mkdir(parents=True, exist_ok=True)
    filepath = BASE_FTA / f"{term[0]}.md"
    if filepath.exists():
        print(f"  SKIP (exists): {filepath.relative_to(BASE_FTA.parent.parent.parent)}")
        return False
    content = f"""---
title: {term[1]}
description: '{term[3][:80]}...'
category: fta
tags:
- fta
- troubleshooting
- reliability
- {term[0].replace("-", "")}
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- {term[1]} 是什么
- {term[2]} 详解
trigger_keywords:
{chr(10).join(f"- {kw}" for kw in dict.fromkeys([term[1], term[2], "fta"]))}
prerequisites:
- troubleshooting-methodology
created: "2026-06-24"
---

# {term[1]}

> **英文名**: {term[2]}

## 概述

{term[3]}

## 核心概念/原理

{term[4]}

## 关键机制或特性

{term[5]}

## 使用场景与最佳实践

{term[6]}

## 参考链接

- [{term[2]}]({term[7]})

## Related

- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-a-glossary|FTA 术语表]]
"""
    filepath.write_text(content, encoding="utf-8")
    print(f"  CREATED: {filepath.relative_to(BASE_FTA.parent.parent.parent)}")
    return True

# ═══════════════════════════════════════════════════════════════════
# Generator function (same as Round 1)
# ═══════════════════════════════════════════════════════════════════

def generate_k8s_file(term):
    target_dir = term["base"] / term["cat_dir"]
    target_dir.mkdir(parents=True, exist_ok=True)
    filepath = target_dir / f"{term['filename']}.md"
    if filepath.exists():
        print(f"  SKIP (exists): {filepath.relative_to(BASE_K8S.parent.parent)}")
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
{chr(10).join(f"- {kw}" for kw in dict.fromkeys([term['title_zh'], term['title_en'], "dictionary"]))}
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

{term['use_cases_bp']}

## 参考链接

- [{term['title_en']} - Official Documentation]({term['refs']})

## Related

{term['related']}
"""
    filepath.write_text(content, encoding="utf-8")
    print(f"  CREATED: {filepath.relative_to(BASE_K8S.parent.parent)}")
    return True


def main():
    print("=" * 60)
    print("Round 2: Generating remaining glossary term files")
    print("=" * 60)

    # K8s glossary remaining terms
    print(f"\n--- K8s Glossary Terms ({len(TERMS)} terms) ---")
    k8s_created = sum(1 for term in TERMS if generate_k8s_file(term))
    k8s_skipped = len(TERMS) - k8s_created

    # FTA terms
    print(f"\n--- FTA Glossary Terms ({len(FTA_TERMS)} terms) ---")
    fta_created = sum(1 for term in FTA_TERMS if generate_fta_file(term))
    fta_skipped = len(FTA_TERMS) - fta_created

    total = len(TERMS) + len(FTA_TERMS)
    total_created = k8s_created + fta_created
    total_skipped = k8s_skipped + fta_skipped

    print(f"\n{'='*60}")
    print(f"Summary: {total_created} created, {total_skipped} skipped, {total} total")
    print(f"  K8s: {k8s_created} created, {k8s_skipped} skipped")
    print(f"  FTA: {fta_created} created, {fta_skipped} skipped")


if __name__ == "__main__":
    main()
