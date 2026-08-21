---
title: Topology Aware Routing
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Topology Aware Routing 是什么
- 如何 Topology Aware Routing
trigger_keywords:
- Topology
- Aware
- Routing
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Topology Aware Routing

## 概述

拓扑感知路由（Topology Aware Routing，旧称 Topology Aware Hints）是一种帮助将网络流量保留在其发起可用区（zone）内的机制。通过在 EndpointSlice 中为端点设置 zone 提示，kube-proxy 可优先将流量路由到同一拓扑区域的端点，从而降低网络延迟、提升可靠性并可能减少跨区流量成本。

## 核心概念/原理

- **拓扑提示（Hints）**：EndpointSlice 控制器在计算 [[service|Service]] 的后端端点时，会考虑每个端点所在节点的拓扑信息（region 和 zone），并在 EndpointSlice 的 `hints.forZones` 字段中为端点分配提示。
- **kube-proxy 消费提示**：kube-proxy 在转发流量时，会根据自身所在 zone 过滤带有对应 zone 提示的端点，优先选择同 zone 端点。如果某个端点被分配到其他 zone，也会有少量跨区流量用于均衡负载。
- **按比例分配**：控制器默认根据各 zone 内节点的**可分配 CPU 核心数**比例来分配端点数量。例如，zone A 的可分配 CPU 是 zone B 的两倍，则 zone A 会分配到约两倍的端点提示。

## 关键机制或特性

- **启用方式**：通过在 Service 上添加注解 `service.[[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]].io/topology-mode: Auto` 开启。在 Kubernetes 1.27 之前，使用旧注解 `service.kubernetes.io/topology-aware-hints`。
- **保护机制（Safeguards）**：当以下任一条件不满足时，系统会回退到全集群范围的路由，避免流量不均衡或黑洞：
  1. 端点数量少于集群 zone 数量。
  2. 无法在各区之间实现可接受的均衡分配（预期过载值超过阈值）。
  3. 有节点缺少 `topology.kubernetes.io/zone` 标签或可分配 CPU 信息。
  4. 部分端点缺少 zone hint（可能处于过渡状态）。
  5. kube-proxy 所在 zone 在 hints 中没有任何对应端点（常见于新增 zone 时）。
- **约束与限制**：
  - 不能与 `internalTrafficPolicy: Local` 同时使用。
  - 假设流量在各 zone 之间大致与节点容量成正比；若大部分流量来自单个 zone，可能导致该区端点过载。
  - EndpointSlice 控制器在计算比例时忽略未就绪节点以及带有 `node-role.kubernetes.io/control-plane` / `master` 标签的节点。
  - 未考虑 Pod 的 tolerations，若工作负载仅调度到部分节点，可能导致分配不均。
  - 与 Horizontal Pod Autoscaler 配合时可能存在延迟或不准确，因为流量压力可能集中在特定 zone。

## 使用场景

- **多可用区部署优化**：在跨多个可用区部署的集群中，将 Pod 间通信限制在同 zone，降低跨 AZ 网络延迟和费用。
- **大数据/批处理流量本地化**：需要大量节点间通信的工作负载，通过同 zone 路由减少骨干网带宽占用。
- **提升故障隔离性**：将流量限制在 zone 内，可在单个 zone 问题时减少对其他 zone 的影响面。

## 最佳实践/注意事项

- **确保每个 zone 有足够端点**：建议每个 zone 至少有 3 个端点，否则控制器很可能无法均匀分配 hints 并回退到全集群路由。
- **流量分布需相对均匀**：该特性不适合流量高度集中在某一个或少数 zone 的服务，否则会导致局部端点过载。
  - 若存在此类情况，建议评估使用 `trafficDistribution` 字段作为更灵活的替代方案。
- **避免与 internalTrafficPolicy: Local 混用**：同一 Service 上不能同时开启 topology-mode 和 `internalTrafficPolicy: Local`，但可以在集群中为不同 Service 分别使用。
- **关注控制器和节点标签**：确保所有工作节点都正确设置了 zone 标签，并且控制平面节点若运行工作负载也纳入考量。
- **监控与回退**：应监控 Service 的 EndpointSlice hints 设置情况，及时发现因 safeguards 触发导致的回退行为。

## 生产 YAML 示例

### 启用拓扑感知路由

```yaml
apiVersion: v1
kind: Service
metadata:
  name: api-backend
  namespace: production
  annotations:
    service.kubernetes.io/topology-mode: Auto     # 启用拓扑感知路由
spec:
  selector:
    app: api-backend
  ports:
  - port: 80
    targetPort: 8080
  # 注意：不能同时设置 internalTrafficPolicy: Local
```

### 确保每个可用区有足够端点

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-backend
  namespace: production
spec:
  replicas: 9                      # 3 个 zone × 每 zone 至少 3 个
  selector:
    matchLabels:
      app: api-backend
  template:
    metadata:
      labels:
        app: api-backend
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: api-backend
      containers:
      - name: api
        image: registry.example.com/apps/api:v3.0
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| EndpointSlice 无 hints | Safeguard 触发回退 | `kubectl get endpointslice -l kubernetes.io/service-name=<svc> -o yaml` 检查 hints 字段 |
| 流量仍跨可用区 | 端点数 < zone 数或节点缺少 zone 标签 | `kubectl get nodes --show-labels | grep zone` |
| 局部端点过载 | 流量集中在某个 zone | 评估使用 `trafficDistribution` 替代 |
| 与 internalTrafficPolicy 冲突 | 同一 Service 同时配置了两者 | 移除其中一个配置 |

## 生产检查清单

- [ ] 所有工作节点设置 `topology.kubernetes.io/zone` 标签
- [ ] 每个 zone 至少 3 个端点（否则可能回退到全集群路由）
- [ ] 不与 `internalTrafficPolicy: Local` 同时使用
- [ ] 使用 topologySpreadConstraints 保证 Pod 跨 zone 均匀分布
- [ ] 监控 EndpointSlice hints 设置情况

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点 zone 标签
kubectl get nodes -L topology.kubernetes.io/zone

# 检查 EndpointSlice 的 hints
kubectl get endpointslice -l kubernetes.io/service-name=<svc> -o json | jq '.items[].endpoints[] | {address: .addresses[0], zone: .zone, hints: .hints}'

# 查看 Service 注解
kubectl get svc <name> -o jsonpath='{.metadata.annotations}'
```
## 交叉引用

- [[17-系统基础/06-知识字典/networking/service-internal-traffic-policy.md|Service Internal Traffic Policy]]](service-internal-traffic-policy.md) — 节点本地路由（互斥特性）
- [[17-系统基础/06-知识字典/networking/endpointslices.md|EndpointSlices]]](endpointslices.md) — hints 字段和 zone 信息
- [Service](service.md) — trafficDistribution 字段
- [Cluster Networking](cluster-networking.md) — 跨可用区流量优化

## 架构深度解析

### 拓扑感知路由工作机制

```
┌──────────────────────────────────────────────────────────────┐
│  Service（topologyAwareHints: Auto）                          │
│       │                                                       │
│       ▼                                                       │
│  EndpointSlice Controller                                     │
│  ├─ 为每个 Endpoint 写入 zone 字段（来自 Node 的 zone 标签）  │
│  ├─ 计算各 zone 的 Endpoint 分布                             │
│  └─ 生成 hints：                                             │
│      forZones:                                                │
│      - zone: "zone-a"  → 只提供 zone-a 的 Endpoint          │
│        （该 zone 有 ≥ 阈值数量的 Endpoint 时才写 hints）      │
│       │                                                       │
│       ▼                                                       │
│  kube-proxy（数据面）                                         │
│  ├─ 读取 hints，本地化过滤 Endpoint 集合                      │
│  └─ 生成 NAT 规则时只包含本 zone 后端（跨 zone 仅兜底）       │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| EndpointSlice 控制器 | `pkg/controller/endpointslice/` | 写入 zone 字段并生成 `hints`（topology 包） |
| topology 计算 | `pkg/controller/endpointslice/topology.go` | 按 zone 分布与阈值（1/3 覆盖）决定 hints 生成 |
| kube-proxy | `pkg/proxy/topology.go` | 消费 hints，过滤本 zone Endpoint 生成规则 |
| API 校验 | `pkg/apis/core/validation` | 校验 topologyAwareHints 取值（Auto/Disabled） |

### 流程步骤

1. 为 Service 设置 `topologyAwareHints: Auto`（v1.27+ 默认启用，v1.31 起 GA）。
2. EndpointSlice 控制器为每个端点标注 zone（节点标签 `topology.kubernetes.io/zone`）。
3. 当某 zone 的 Endpoint 数量 ≥ 总端点数的 1/3 时，控制器生成 hints 将流量限制在该 zone。
4. kube-proxy 读取 hints 只生成本 zone 的 DNAT 规则；跨 zone 流量仅在后端不足时兜底。
5. 任一 zone 端点数跌破阈值，hints 自动移除，流量回退全量（无感知故障转移）。

## 生产案例

### 案例 1：启用拓扑感知后单可用区故障导致流量倾斜

| 时间 | 事件 |
|------|------|
| 10:00 | zone-b 网络故障，zone-b 节点全部 NotReady |
| 10:05 | 该 zone 的 Pod 被驱逐，Service 端点数骤降 |
| 10:10 | 观察 zone-a 流量翻倍（hints 未及时移除，仍限制在 zone-a） |
| 10:15 | 控制器移除 hints，流量恢复全量分发 |
| 10:30 | 确认期间 zone-a 容量充足，无服务降级 |
| 11:00 | 复盘：为关键服务配置了 PDB 与跨 zone 冗余，故障转移平滑 |

**根因**：单 zone 故障期间 hints 短暂将流量锁定在本 zone，依赖控制器收敛速度（约分钟级）。
**修复命令**：
```bash
# 查看 Service 的拓扑配置 🟢 只读
kubectl get svc <name> -o yaml | grep -A3 topologyAwareHints
# 查看 EndpointSlice 的 hints 🟢 只读
kubectl get endpointslices -l kubernetes.io/service-name=<svc> -o yaml | grep -B2 -A4 forZones
# 紧急回退为全量路由 🟡 中风险
kubectl patch svc <name> -p '{"spec":{"topologyAwareHints":"Disabled"}}'
```

### 案例 2：跨可用区延迟优化收益验证

**现象**：多可用区集群服务间调用延迟高（跨 zone 一跳 ~5ms），启用拓扑感知后延迟下降不明显。
**诊断**：`kubectl get endpointslices -o yaml` 发现各 zone 端点数量不均（zone-a 占 80%），hints 未生效；检查发现 Service 未设置 `topologyAwareHints: Auto`。
**修复**：为服务开启 Auto 并均衡副本分布（topologySpreadConstraints），延迟降低 40%；同时确认 `serviceInternalTrafficPolicy` 未与拓扑感知互斥配置。

## 对比评测

| 维度 | 拓扑感知路由 | Service Internal Traffic Policy | 手动亲和调度 |
|------|-------------|--------------------------------|--------------|
| 控制粒度 | Endpoint 层面（zone 过滤） | 节点层面（仅本节点后端） | 调度层面（副本分布） |
| 自动回退 | 有（阈值机制） | 有（本节点无后端时全量） | 无 |
| 配置成本 | 一行字段 | 一行字段 | 需要 topologySpreadConstraints |
| 适用场景 | 跨可用区延迟优化 | 同节点数据本地性 | 容量/故障域规划 |

**选型建议**：多可用区集群默认开启 topologyAwareHints；数据本地性场景叠加 internalTrafficPolicy；两者与拓扑约束互补使用。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| hints 不生效 | `kubectl get endpointslices -o yaml \| grep forZones` | 端点分布不足阈值、字段未启用 |
| 单 zone 流量暴增 | 检查节点/zone 标签一致性 | 节点 zone 标签缺失或漂移 |
| 回退不及时 | 观察控制器日志 | 大规模变更时收敛延迟 |
| 与内部流量策略冲突 | 检查两个字段配置 | 互斥使用导致路由异常 |
| 跨 zone 延迟未降 | `kubectl get endpoints -o wide` 看分布 | 副本分布不均（需拓扑约束） |

## 生产部署清单

- [ ] 所有节点已打 zone/region 标签（`topology.kubernetes.io/zone`）
- [ ] 服务副本分布均衡（topologySpreadConstraints 或反亲和）
- [ ] 各 zone 容量冗余 ≥ 2x（hints 生效期单 zone 承受全量流量）
- [ ] 关键服务已配置 PDB 与跨 zone 故障演练
- [ ] 监控 hints 状态与流量分布（按 zone 维度）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 单 zone 故障时服务容量不足 | 先扩容冗余，再启用 hints |
| P1 | 跨 zone 延迟敏感 | 启用 Auto，验证端点分布阈值 |
| P1 | 与 internalTrafficPolicy 同时需求 | 确认组合语义，小流量验证 |
| P2 | 单 zone 集群 | 无需启用，保持 Disabled |

## 面试要点

> 以下 Q&A 覆盖拓扑感知路由面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：拓扑感知路由（Topology Aware Routing）的工作原理是什么？**
   A：EndpointSlice 控制器根据节点 zone 标签为每个端点标注 zone，并计算各 zone 端点占比；当某 zone 端点数 ≥ 总数的 1/3 时生成 hints（forZones），kube-proxy 据此只生成该 zone 的 DNAT 规则，将流量限制在本 zone 减少跨可用区延迟与带宽成本；当端点分布跌破阈值时自动移除 hints 回退全量路由。本质是"数据面本地化过滤 + 控制面阈值保护"。

2. **Q：拓扑感知路由与 Service Internal Traffic Policy 有什么区别？**
   A：拓扑感知在 zone 粒度过滤 Endpoint（hints 机制，跨 zone 仍可兜底）；Internal Traffic Policy 在节点粒度限制（`Cluster` 全节点或 `Local` 仅本节点后端），两者可以组合但互斥特性需注意：Local 模式会跳过拓扑过滤逻辑。拓扑感知适合跨可用区优化，Local 适合数据本地性（如读本地缓存副本）。

3. **Q：启用拓扑感知路由有哪些风险？如何规避？**
   A：风险：① 单 zone 故障期间流量被锁定在本 zone（控制器分钟级收敛，期间需本 zone 容量足够）；② 端点分布不均导致 hints 不生效或频繁抖动；③ zone 标签缺失导致计算错误。规避：① 各 zone 冗余 ≥ 2x 并演练故障转移；② 用 topologySpreadConstraints 保证分布；③ 定期巡检节点标签一致性；④ 关键服务设置 `topologyAwareHints: Disabled` 作为逃生通道。

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/topology-aware-routing/

## Related

- [[17-系统基础/06-知识字典/networking/aeraki-mesh.md|Aeraki Mesh 七层网格]]
- [[17-系统基础/06-知识字典/networking/akri.md|Akri 边缘设备发现]]
- [[17-系统基础/06-知识字典/networking/antrea.md|Antrea 网络方案]]


<!-- risk-assessed -->
