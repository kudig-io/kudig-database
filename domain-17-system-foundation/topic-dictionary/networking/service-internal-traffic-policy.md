---
title: Service Internal Traffic Policy
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- daemonset
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Service Internal Traffic Policy 是什么
- 如何 Service Internal Traffic Policy
trigger_keywords:
- Service
- Internal
- Traffic
- Policy
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

# Service Internal Traffic Policy

## 概述

Service Internal Traffic Policy（Service 内部流量策略）用于控制集群内部发起的流量如何被路由到后端端点。将该策略设置为 `Local` 时，kube-proxy 会仅将流量转发到与请求源位于**同一节点**上的端点，避免跨节点网络跳转，从而降低延迟、减少网络带宽成本，并有助于保留客户端源 IP。

## 核心概念/原理

- **internalTrafficPolicy 字段**：
  - `Cluster`（默认）：kube-proxy 在转发内部流量时，会考虑 Service 的所有后端端点，实现全集群范围的负载均衡。
  - `Local`：kube-proxy 仅将流量路由到本节点上的端点。如果本节点没有符合条件的就绪端点，流量将被丢弃（黑洞）。
- **“内部”流量的定义**：指由当前集群内的 Pod 发起的访问 Service 的流量。该策略不影响从集群外部进入的流量（外部流量由 `externalTrafficPolicy` 控制）。

## 关键机制或特性

- **kube-proxy 端点过滤**：kube-proxy 根据 Service 的 `spec.internalTrafficPolicy` 值，在维护本地路由规则时过滤 EndpointSlice。当策略为 `Local` 时，仅保留 `nodeName` 与当前节点匹配的端点。
- **与 externalTrafficPolicy 的关系**：`internalTrafficPolicy` 和 `externalTrafficPolicy` 是相互独立的字段，可以分别设置。例如，将两者都设为 `Local`，可同时优化内部和外部流量的节点本地路由，并保留外部客户端的真实源 IP。
- **与 Topology Aware Routing 的互斥**：同一 Service 上不能同时启用 `internalTrafficPolicy: Local` 和 Topology Aware Routing（`service.[[entities/kubernetes|kubernetes]].io/topology-mode: Auto`），但可以在集群中为不同的 Service 分别使用这两种特性。

## 使用场景

- **同节点 Pod 通信优化**：当调用方与被调用的后端 Pod 经常位于同一节点时，使用 `Local` 策略可避免数据包经过 overlay 网络或跨节点物理网络，降低延迟和包转发开销。
- **源 IP 保留（内部流量）**：某些安全审计或应用日志需要识别集群内部流量的真实源 Pod IP，`Local` 策略可避免 SNAT 导致的源 IP 丢失。
- **降低网络成本**：在按跨节点/跨可用区流量计费的云环境中，限制流量在节点内传输可显著降低网络费用。
- **与 DSR/外部源 IP 保留配合**：在 Windows 等支持 DSR（Direct Server Return）的环境中，将 `internalTrafficPolicy` 与 `externalTrafficPolicy` 均设为 `Local`，配合 DSR 实现完整的源 IP 保留和节点本地转发。

## 最佳实践/注意事项

- **确保每个节点都有足够的后端 Pod**：使用 `Local` 策略时，必须保证每个可能发起调用的节点上都有该 Service 的就绪端点，否则会导致流量黑洞和服务不可用。建议通过 DaemonSet 或合理的 Pod 反亲和性/拓扑分布约束来保障节点覆盖。
- **监控节点本地端点可用性**：当节点上的后端 Pod 全部终止或漂移后，该节点上对此 Service 的内部调用将失败，需配合健康检查和告警及时发现。
- **不能与 Topology Aware Routing 同时使用**：若 Service 已启用拓扑感知路由，则不能再设置 `internalTrafficPolicy: Local`。若两者都需要，应拆分到不同的 Service 中。
- **升级兼容性**：`internalTrafficPolicy` 自 v1.26 起稳定，在旧版本集群中需确认特性门控已启用。
- **负载均衡粒度变粗**：`Local` 策略将负载均衡范围从全集群缩小到节点内，若节点间 Pod 数量不均衡，可能导致局部热点。

## 生产 YAML 示例

### 节点本地流量策略

```yaml
apiVersion: v1
kind: Service
metadata:
  name: metrics-collector
  namespace: monitoring
spec:
  selector:
    app: metrics-collector
  ports:
  - port: 9090
    targetPort: 9090
    protocol: TCP
  internalTrafficPolicy: Local     # 仅路由到同节点的端点
  externalTrafficPolicy: Cluster   # 外部流量仍使用全集群路由
---
# 使用 DaemonSet 确保每个节点都有端点（避免流量黑洞）
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: metrics-collector
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: metrics-collector
  template:
    metadata:
      labels:
        app: metrics-collector
    spec:
      containers:
      - name: collector
        image: registry.example.com/monitoring/collector:v2.0
        ports:
        - containerPort: 9090
        resources:
          requests:
            cpu: "100m"
            memory: "128Mi"
```

### 内外兼顾：双 Local 策略

```yaml
# 同时优化内部和外部流量的节点本地路由
apiVersion: v1
kind: Service
metadata:
  name: node-local-cache
  namespace: production
spec:
  type: NodePort
  selector:
    app: cache
  ports:
  - port: 6379
    targetPort: 6379
    nodePort: 30379
  internalTrafficPolicy: Local     # 内部流量走本节点
  externalTrafficPolicy: Local     # 外部流量也走本节点（保留源 IP）
```

## 流量策略对比矩阵

| 维度 | `Cluster`（默认） | `Local` |
|------|-------------------|---------|
| 路由范围 | 全集群所有端点 | 仅同节点端点 |
| 负载均衡 | 全局均衡 | 节点内均衡 |
| 网络延迟 | 可能跨节点/跨 AZ | 最低（节点内） |
| 源 IP 保留 | 可能被 SNAT | 保留 |
| 黑洞风险 | 无 | 有（节点无端点时） |
| 跨 AZ 流量成本 | 高 | 无 |
| 适用工作负载 | Deployment（不保证节点覆盖） | DaemonSet 或高覆盖 Deployment |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 部分节点上的 Pod 无法访问 Service | 目标 Service 使用 `Local` 策略但该节点无就绪端点 | `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` 检查各节点端点分布 |
| 流量分布不均 | 各节点上的后端 Pod 数量不一致 | 使用 DaemonSet 或 topologySpreadConstraints 均匀分布 |
| 与 Topology Aware Routing 冲突 | 同一 Service 同时启用了两种特性 | 二选一：移除 `topology-mode` 注解或改回 `internalTrafficPolicy: Cluster` |
| 升级到 v1.26 前特性不生效 | 旧版本需启用特性门控 | 确认 kube-proxy 版本 ≥ v1.26 或特性门控已启用 |

## 生产检查清单

- [ ] 使用 `Local` 策略的 Service 确保每个节点都有就绪端点（推荐 DaemonSet）
- [ ] 或使用 `topologySpreadConstraints` 保证 Deployment Pod 均匀分布
- [ ] 确认未与 `service.kubernetes.io/topology-mode: Auto` 同时使用
- [ ] 监控各节点的端点可用性，设置告警
- [ ] 评估 `Local` 策略下节点间负载不均的影响

## 命令快速参考

```bash
# 查看 Service 的流量策略
kubectl get svc <name> -o jsonpath='{.spec.internalTrafficPolicy}'

# 查看 EndpointSlice 中各端点的节点分布
kubectl get endpointslices -l kubernetes.io/service-name=<svc> -o yaml | grep -A2 nodeName

# 检查节点上的 kube-proxy 规则（iptables 模式）
iptables-save | grep <service-clusterip>

# 验证同节点 Pod 是否能访问 Service
kubectl exec <pod-on-same-node> -- curl -s http://<service>:<port>/healthz
```

## 交叉引用

- [Service](service.md) — Service 类型和流量策略全局概览
- [EndpointSlices](endpointslices.md) — kube-proxy 如何基于 EndpointSlice 过滤端点
- [Topology Aware Routing](topology-aware-routing.md) — 基于可用区的流量优化（互斥特性）
- [DaemonSet](../workloads/daemonset.md) — 确保节点覆盖的推荐部署方式

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/service-traffic-policy/
