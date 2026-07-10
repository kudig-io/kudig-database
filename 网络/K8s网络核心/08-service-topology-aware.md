---
title: 72 - 服务拓扑与端点切片
description: '| 注解/配置 | 值 | 说明 |'
summary: '| 注解/配置 | 值 | 说明 |'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- controller-manager
- flannel
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 服务拓扑与端点切片 是什么
- 如何 服务拓扑与端点切片
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- 服务拓扑与端点切片
- networking
prerequisites:
- kubectl-basics
- networking-basics
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
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/topic-fta/list/service-fta.md
  label: '故障树: service'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 72 - 服务拓扑与端点切片

<!-- chunk: EndpointSlice vs Endpoints -->
## EndpointSlice vs Endpoints

| 特性 | Endpoints | EndpointSlice |
|-----|-----------|---------------|
| 扩展性 | 单对象(大Service问题) | 分片(每片100端点) |
| 更新粒度 | 全量更新 | 增量更新 |
| 拓扑信息 | ❌ | ✅ zone/region |
| IPv6支持 | ✅ | ✅ 更完善 |
| 双栈支持 | ❌ | ✅ |
| 版本 | v1 | v1(GA) |
| 推荐使用 | 否 | 是 |

<!-- chunk: EndpointSlice结构 -->
## EndpointSlice结构

```yaml
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: myservice-abc123
  namespace: default
  labels:
    kubernetes.io/service-name: myservice
  ownerReferences:
  - apiVersion: v1
    kind: Service
    name: myservice
addressType: IPv4
ports:
- name: http
  port: 8080
  protocol: TCP
endpoints:
- addresses:
  - "10.1.1.10"
  conditions:
    ready: true
    serving: true
    terminating: false
  hostname: pod-1
  nodeName: node-1
  zone: us-east-1a
  hints:
    forZones:
    - name: us-east-1a
- addresses:
  - "10.1.2.20"
  conditions:
    ready: true
    serving: true
    terminating: false
  hostname: pod-2
  nodeName: node-2
  zone: us-east-1b
```

<!-- chunk: 拓扑感知路由 -->
## 拓扑感知路由

| 注解/配置 | 值 | 说明 |
|----------|---|------|
| `service.kubernetes.io/topology-mode` | Auto | 自动拓扑路由 |
| `service.kubernetes.io/topology-aware-hints` | Auto | 旧版配置(已废弃) |

```yaml
apiVersion: v1
kind: Service
metadata:
  name: myservice
  annotations:
    service.kubernetes.io/topology-mode: Auto
spec:
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
```

<!-- chunk: 拓扑感知路由条件 -->
## 拓扑感知路由条件

| 条件 | 要求 |
|-----|------|
| kube-proxy模式 | iptables或IPVS |
| 端点分布 | 每个zone有足够端点 |
| 流量均衡 | zone间流量差异<20% |
| 最小端点数 | 建议每zone>=3个 |

<!-- chunk: Service内部流量策略 -->
## Service内部流量策略

```yaml
apiVersion: v1
kind: Service
metadata:
  name: myservice
spec:
  selector:
    app: myapp
  ports:
  - port: 80
  # 优先同节点端点
  internalTrafficPolicy: Local  # Cluster(默认) | Local
```

<!-- chunk: Service外部流量策略 -->
## Service外部流量策略

```yaml
apiVersion: v1
kind: Service
metadata:
  name: myservice
spec:
  type: LoadBalancer
  selector:
    app: myapp
  ports:
  - port: 80
  # 保留客户端IP
  externalTrafficPolicy: Local  # Cluster(默认) | Local
```

<!-- chunk: 流量策略对比 -->
## 流量策略对比

| 策略 | 延迟 | 负载均衡 | 客户端IP | 适用场景 |
|-----|------|---------|---------|---------|
| Cluster | 可能跨节点 | 全局均衡 | SNAT | 默认场景 |
| Local | 本地优先 | 节点内均衡 | 保留 | 延迟敏感/日志分析 |

<!-- chunk: Headless Service端点 -->
## Headless Service端点

```yaml
apiVersion: v1
kind: Service
metadata:
  name: stateful-service
spec:
  clusterIP: None  # Headless
  selector:
    app: stateful-app
  ports:
  - port: 5432
---
# DNS返回所有Pod IP
# stateful-service.default.svc.cluster.local -> [PodIP1, PodIP2, ...]
# pod-0.stateful-service.default.svc.cluster.local -> PodIP1
```

<!-- chunk: 端点切片查看 -->
## 端点切片查看

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出EndpointSlice
kubectl get endpointslices -l kubernetes.io/service-name=myservice

# 查看详情
kubectl describe endpointslice myservice-abc123

# 查看拓扑提示
kubectl get endpointslices -l kubernetes.io/service-name=myservice \
  -o jsonpath='{range .items[*]}{range .endpoints[*]}{.addresses[0]} {.zone} {.hints}{"\n"}{end}{end}'

# 检查拓扑感知状态
kubectl get service myservice -o yaml | grep topology
```
<!-- chunk: 端点控制器配置 -->
## 端点控制器配置

| kube-controller-manager参数 | 默认值 | 说明 |
|----------------------------|-------|------|
| `--endpointslice-updates-batch-period` | 500ms | 批量更新周期 |
| `--max-endpoints-per-slice` | 100 | 每个切片最大端点数 |

<!-- chunk: 监控指标 -->
## 监控指标

| 指标 | 类型 | 说明 |
|-----|-----|------|
| `endpoint_slice_controller_changes` | Counter | 端点变更次数 |
| `endpoint_slice_controller_endpoints` | Gauge | 端点总数 |
| `endpoint_slice_controller_syncs` | Counter | 同步次数 |
| `endpoint_slice_controller_services` | Gauge | 服务数量 |

<!-- chunk: 大规模Service优化 -->
## 大规模Service优化

| 优化项 | 说明 |
|-------|------|
| 使用EndpointSlice | 自动分片 |
| 拓扑感知路由 | 减少跨zone流量 |
| 适当副本数 | 避免过多端点 |
| 就绪探针 | 确保健康端点 |

<!-- chunk: ACK服务网络 -->
## ACK服务网络

| 功能 | 说明 |
|-----|------|
| Terway | 高性能CNI |
| IPVS模式 | 大规模Service |
| 拓扑感知 | 跨AZ流量优化 |
| SLB直连 | 绕过kube-proxy |

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.21 | EndpointSlice GA |
| v1.23 | 拓扑感知路由Beta |
| v1.27 | TopologyAwareHints改进 |
| v1.30 | TrafficDistribution字段 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 KUDIG Database — Global MOC
- [[网络/README.md|[[Domain 5: Networking 网络|Domain 5: Networking 网络]]working]] 网络]]
- [[网络/K8s网络核心/00-network-in-nutshell.md|00 network in nutshell]]
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持

## See Also

- 06-service-concepts-types
- 07-service-implementation-details
- 09-kube-proxy-modes-performance
- 10-service-advanced-features

## Related

- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]


<!-- risk-assessed -->
