---
title: EndpointSlices
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- EndpointSlices 是什么
- 如何 EndpointSlices
trigger_keywords:
- EndpointSlices
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# EndpointSlices

## 概述

EndpointSlice 是 [[Kubernetes|Kubernetes]] 自 v1.21 起稳定的 API，用于跟踪 [[Service|Service]] 的后端网络端点（通常是 Pod 的 IP 地址）。它是旧版 Endpoints API 的演进，能够支撑大规模 Service（数千个后端 Pod），并高效地更新后端列表，是 kube-proxy 进行内部流量路由的权威数据来源。

## 核心概念/原理

- **切片（Slice）**：一个 EndpointSlice 对象代表某个 Service 后端端点的一个子集。控制平面按 IP 协议族、端口、Service 名称等维度将端点分组到不同的 Slice 中。
- **自动创建与维护**：对于定义了 selector 的 Service，EndpointSlice 控制器会自动创建和维护对应的 EndpointSlice，持续同步 Pod 的变化。
- **地址类型**：每个 EndpointSlice 只包含一种地址类型：`IPv4` 或 `IPv6`。双栈 Service 至少对应两个 EndpointSlice。
- **条件（Conditions）**：
  - `ready`：端点是否准备好接收流量（`serving && !terminating` 的快捷方式）。
  - `serving`：端点是否正在提供服务。
  - `terminating`：端点是否正在终止（Pod 收到删除时间戳时设置），在滚动更新期间可用于避免流量丢失。
- **拓扑信息**：每个端点可携带 `nodeName` 和 `zone`，用于支持拓扑感知路由等功能。

## 关键机制或特性

- **容量与分配策略**：默认每个 EndpointSlice 最多包含 100 个端点，可通过 `--max-endpoints-per-slice` 配置，最大支持 1000。控制平面优先减少更新次数（降低向所有节点传播的开销），而非追求每个 Slice 完全填满。
- **管理标签（managed-by）**：`endpointslice.kubernetes.io/managed-by` 标签标识 EndpointSlice 的管理者。控制平面管理器的值为 `endpointslice-controller.k8s.io`，自定义控制器或手动管理应使用唯一值。
- **所有权（Ownership）**：EndpointSlice 通常由对应的 Service 通过 ownerReference 拥有，并带有 `kubernetes.io/service-name` 标签，便于查询。
- **端点去重**：由于更新可能异步到达，同一端点可能短暂出现在多个 Slice 中。消费者（如 kube-proxy）必须聚合所有关联的 EndpointSlice 并去重。
- **EndpointSlice Mirroring（已弃用）**：为兼容旧版 Endpoints API，控制平面会将用户创建的 Endpoints 镜像为 EndpointSlice。该功能及 Endpoints API 均已弃用，建议直接创建 EndpointSlice。

## 使用场景

- **大规模 Service 后端管理**：当 Service 背后有数百至数千个 Pod 时，EndpointSlice 将端点拆分为多个对象，避免单个 API 对象过大。
- **kube-proxy 路由依据**：每个节点的 kube-proxy 监听 EndpointSlice 变化，维护本地路由规则。
- **自定义服务发现**：Service Mesh 或自定义控制器可直接消费 EndpointSlice，实现更精细的流量控制。
- **手动管理外部端点**：对于无 selector 的 Service，手动创建 EndpointSlice 可将流量转发到集群外部地址。

## 最佳实践/注意事项

- **优先使用 EndpointSlice API**：新开发或迁移工作应避免使用旧版 Endpoints API，以获得双栈支持、更大规模和更丰富的元数据。
- **手动创建时避免无效 IP**：EndpointSlice 中的地址不能是 loopback（127.0.0.0/8, ::1/128）、link-local（169.254.0.0/16, fe80::/64）或其他 Kubernetes Service 的 ClusterIP。
- **设置 managed-by 标签**：自定义工具或控制器管理 EndpointSlice 时，应设置合适的 `managed-by` 标签值，避免与系统控制器冲突。
- **客户端需聚合去重**：读取 EndpointSlice 的客户端必须遍历 Service 关联的所有 Slice，并合并去重，参考 `kube-proxy` 中的 `EndpointSliceCache` 实现。

## 生产 YAML 示例

### 手动创建 EndpointSlice（外部服务）

```yaml
# 无 selector Service — 将流量转发到集群外部数据库
apiVersion: v1
kind: Service
metadata:
  name: external-database
  namespace: production
spec:
  ports:
  - port: 5432
    targetPort: 5432
    protocol: TCP
  # 无 selector，需手动管理 EndpointSlice
---
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: external-database-1
  namespace: production
  labels:
    kubernetes.io/service-name: external-database
    endpointslice.kubernetes.io/managed-by: manual-controller
  ownerReferences:
  - apiVersion: v1
    kind: Service
    name: external-database
    uid: "<service-uid>"          # kubectl get svc external-database -o jsonpath='{.metadata.uid}'
addressType: IPv4
ports:
- name: postgres
  port: 5432
  protocol: TCP
endpoints:
- addresses:
  - "10.200.1.100"               # 主库
  conditions:
    ready: true
    serving: true
    terminating: false
  nodeName: ""                    # 外部地址无 nodeName
- addresses:
  - "10.200.1.101"               # 从库
  conditions:
    ready: true
    serving: true
    terminating: false
```

### 查看自动生成的 EndpointSlice

```yaml
# kubectl get endpointslices -l kubernetes.io/service-name=my-svc -o yaml
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: my-svc-abc12
  labels:
    kubernetes.io/service-name: my-svc
    endpointslice.kubernetes.io/managed-by: endpointslice-controller.k8s.io
addressType: IPv4
ports:
- name: http
  port: 8080
  protocol: TCP
endpoints:
- addresses: ["10.244.1.5"]
  conditions:
    ready: true
    serving: true
    terminating: false
  nodeName: worker-1
  zone: us-east-1a
- addresses: ["10.244.2.8"]
  conditions:
    ready: true
    serving: true
    terminating: false
  nodeName: worker-2
  zone: us-east-1b
```

## EndpointSlice 条件字段说明

| 字段 | 含义 | 典型场景 |
|------|------|----------|
| `ready` | 端点可接收流量 | Pod 的 readinessProbe 通过 |
| `serving` | 端点正在提供服务 | 即使在终止中仍可能为 true |
| `terminating` | 端点正在终止 | Pod 收到删除时间戳 |
| `ready=false, serving=true, terminating=true` | Pod 正在终止但仍在服务 | 滚动更新期间排空连接 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Service 无后端 | EndpointSlice 为空或不存在 | `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` |
| 后端 Pod 存在但不在 EndpointSlice 中 | Pod 的 readinessProbe 失败或标签不匹配 | `kubectl get [[Pods|pods]] -l <selector> -o wide`；检查 Pod Ready 状态 |
| 手动 EndpointSlice 被覆盖 | `managed-by` 标签与系统控制器冲突 | 使用唯一的 `managed-by` 值；确认 Service 无 selector |
| 端点出现重复 | 异步更新导致同一 Pod 出现在多个 Slice 中 | 正常现象，消费端需聚合去重 |
| 外部端点创建失败 | 地址使用了 loopback 或 link-local IP | 使用有效的可路由 IP 地址 |

## 生产检查清单

- [ ] 新开发优先使用 EndpointSlice API（而非旧版 Endpoints）
- [ ] 手动管理 EndpointSlice 时设置唯一的 `managed-by` 标签
- [ ] 手动 EndpointSlice 不使用 loopback、link-local 或 ClusterIP 作为地址
- [ ] 消费 EndpointSlice 的客户端实现聚合去重逻辑
- [ ] 监控 EndpointSlice 数量和端点总数

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出 Service 关联的 EndpointSlice
kubectl get endpointslices -l kubernetes.io/service-name=<svc> -n <ns>

# 查看 EndpointSlice 详情
kubectl describe endpointslice <name> -n <ns>

# 统计端点总数
kubectl get endpointslices -l kubernetes.io/service-name=<svc> -o json | jq '[.items[].endpoints[]] | length'

# 查看端点的节点和可用区分布
kubectl get endpointslices -l kubernetes.io/service-name=<svc> -o json | jq '.items[].endpoints[] | {address: .addresses[0], node: .nodeName, zone: .zone}'

# 检查旧版 Endpoints（已弃用）
kubectl get endpoints <svc> -n <ns>
```
## 交叉引用

- [Service](service.md) — Service 如何通过 selector 自动生成 EndpointSlice
- [Service Internal Traffic Policy](service-internal-traffic-policy.md) — kube-proxy 如何基于 EndpointSlice 的 nodeName 过滤端点
- [Topology Aware Routing](topology-aware-routing.md) — EndpointSlice 中 zone hints 的生成和消费
- [DNS for Services and Pods](dns-for-services-and-pods.md) — Headless Service 的 EndpointSlice 与 DNS 记录

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/

## Related

- [[domain-19-landscape-references/领域索引/dns-index.md|DNS 知识图谱索引]]


<!-- risk-assessed -->
