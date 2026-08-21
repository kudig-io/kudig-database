---
title: 端点
description: Endpoints 是 Kubernetes 中 Service 后端 Pod 的 IP 地址和端口的集合。当 Service 没有指定
  selector 时，...
summary: Endpoints 是 Kubernetes 中 Service 后端 Pod 的 IP 地址和端口的集合。当 Service 没有指定 selector
  时，...
category: dictionary
tags:
- k8s
- glossary
- endpoint
- service
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 端点 是什么
- Endpoints 详解
trigger_keywords:
- 端点
- Endpoints
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 端点

> **英文名**: Endpoints

## 概述

Endpoints 是 Kubernetes 中 Service 后端 Pod 的 IP 地址和端口的集合。当 Service 没有指定 selector 时，需要手动创建 Endpoints 资源。EndpointSlice 是 Endpoints 的现代替代方案，适用于大规模集群。

## 核心概念/原理

### Endpoints vs EndpointSlice

| 特性 | Endpoints | EndpointSlice |
|------|-----------|---------------|
| API | v1 | discovery.k8s.io/v1 |
| 扩展性 | 单个对象包含所有后端 | 分片存储，每片 100 个 |
| 适用场景 | 小规模集群 | 大规模集群（推荐） |

### 工作原理

当 Service 定义了 selector，kube-controller-manager 自动创建对应的 Endpoints/EndpointSlice 对象。

## 关键机制或特性

- 每个 Endpoint 包含 IP、端口和就绪状态。
- EndpointSlice 按拓扑分区，支持 `topology.kubernetes.io/zone` 标签。
- 外部服务可通过手动 Endpoints + ExternalName Service 接入。

## 使用场景与最佳实践

- 大规模集群优先使用 EndpointSlice API。
- 排查 Service 不通时，检查 Endpoints 是否包含预期的后端 Pod。
- 使用 `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` 查看。
- Headless Service 的 Endpoints 直接返回 Pod IP。

## 架构深度解析

### Endpoint 数据模型

```
┌─────────────────────────────────────────────────────────┐
│  Endpoints 对象（v1 核心 API）                            │
│  ┌──────────────────────────────────────────────────┐   │
│  │  apiVersion: v1                                   │   │
│  │  kind: Endpoints                                  │   │
│  │  metadata.name: my-svc（与 Service 同名）          │   │
│  │  subsets:                                         │   │
│  │  - addresses:                                     │   │
│  │    - ip: 10.0.1.5                                 │   │
│  │      nodeName: node-a                             │   │
│  │      targetRef: {kind: Pod, name: app-0}          │   │
│  │    - ip: 10.0.2.7                                 │   │
│  │    notReadyAddresses: [...]                       │   │
│  │    ports: [{name: http, port: 8080, protocol: TCP}]│   │
│  └──────────────────────────────────────────────────┘   │
│                                                         │
│  生成者：Endpoints Controller（selector 匹配 Pod）       │
│  消费者：kube-proxy / CoreDNS / Ingress / 监控工具       │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| Endpoints 控制器 | `pkg/controller/endpoint/` | selector 匹配与 Endpoints 生成 |
| 端点计算 | `pkg/controller/endpoint/reconciler.go` | 就绪/未就绪集合计算 |
| Service 关联 | `pkg/registry/core/endpoints/` | Endpoints 与 Service 的命名关联 |

### Endpoints 生命周期

1. Service 创建（带 selector）→ 控制器开始 watch 匹配的 Pod
2. Pod Ready 后写入 `subsets[].addresses`；未就绪写入 `notReadyAddresses`
3. Pod 删除/就绪变化 → 增量更新 Endpoints（全量替换语义）
4. 无 selector 的 Service：Endpoints 由用户/外部控制器手工维护
5. 最大 1000 端点（v1.21+ 推荐 EndpointSlice 规避）

## 生产案例

### 案例 1：手工维护 Endpoints 的服务在 Pod 重建后失联

| 时间 | 事件 |
|------|------|
| 11:00 | 外部数据库迁移（Pod 化），Service 无 selector + 手工 Endpoints |
| 11:10 | 数据库 Pod 重启 IP 变化，手工 Endpoints 未更新，业务全部超时 |
| 11:30 | 改为 selector + EndpointSlice 自动管理，问题消除 |

**根因**：`selector` 缺失时 Endpoints 不会自动同步；手工维护在 IP 变化场景下必然滞后。

**修复命令**：
```bash
# 查看 Service 是否无 selector 🟢 只读
kubectl get svc ext-db -o jsonpath='{.spec.selector}'
# 为 Pod 打上匹配标签（YAML）🟡 中风险
kubectl label pod ext-db-0 app=ext-db
# 补全 Service selector 后 Endpoints 自动管理 🟡 中风险
kubectl edit svc ext-db
# spec.selector: { app: ext-db }
```

### 案例 2：多端口 Service 的 Endpoints 端口错乱

**现象**：Service 声明双端口（http:8080, metrics:9090），后端只有单端口容器，metrics 端点解析异常。

**诊断**：Endpoints 的 subsets 按"地址 × 端口"组合：容器未声明 `containerPort` 或端口名不匹配时，控制器生成错误端口集。

**修复**：Pod 模板显式声明 `containerPort` 与端口名（`name: metrics`）；Service 端口与容器端口名一一对应；验证 `kubectl get endpoints ext-svc -o yaml` 的 subsets 结构。

## 对比评测

| 维度 | Endpoints（v1） | EndpointSlice |
|------|-----------------|---------------|
| 容量 | 1000 上限 | 分片 100 |
| 更新 | 全量替换 | 分片级 |
| 状态 | Ready 单态 | Ready/Serving/Terminating |
| 标签 | 无 | service-name 等 |
| 适用场景 | 简单服务/兼容 | 生产默认 |

**选型建议**：新服务一律依赖自动生成（selector）；仅兼容旧系统时使用手工 Endpoints，并配合监控防漂移。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| Endpoints 为空 | `kubectl get endpoints <svc>` | selector 不匹配或无 selector |
| 端口错误 | `kubectl get endpoints -o yaml` | containerPort 未声明 |
| 更新滞后 | 对比 Pod 状态与 Endpoints | 控制器异常或 watch 丢失 |
| 端点漂移 | `kubectl get endpoints -w` | 手工维护未自动化 |

## 生产部署清单

- [ ] 业务服务全部使用 selector 自动管理 Endpoints
- [ ] 手工 Endpoints 场景配置变更告警（IP 漂移检测）
- [ ] Pod 模板统一声明 containerPort 与端口名
- [ ] 监控 Endpoints 数量逼近 1000 上限的服务
- [ ] 工具链升级支持 EndpointSlice

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Endpoints 异常导致服务中断 | 立即核对 selector 并修复端口声明 |
| P1 | 服务规模增长（>1000 端点） | 迁移 EndpointSlice 并验证消费者兼容 |
| P2 | 手工 Endpoints 服务自动化改造 | 逐步替换为 selector 管理 |

## 面试要点

> 以下 Q&A 覆盖 Endpoint 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Endpoints 与 Service 的关系是什么？**
   A：Endpoints 是 Service 的后端 IP:Port 集合，由 Endpoints Controller 依据 Service 的 selector 匹配 Pod 自动生成（同名对象）；无 selector 的 Service 不自动生成，需要手工维护或通过 ExternalName 指向外部。kube-proxy 等消费者基于 Endpoints 建立转发规则。

2. **Q：subsets 中的 addresses 与 notReadyAddresses 有什么区别？**
   A：`addresses` 包含 Ready 且符合调度条件的 Pod（可接收流量）；`notReadyAddresses` 包含未就绪（探针失败/启动中）的 Pod——这类端点仍可用于 headless 服务发现，但 kube-proxy 不会为其建立转发规则（除非配置 publishNotReadyAddresses）。

3. **Q：为什么推荐用 EndpointSlice 替代 Endpoints？**
   A：Endpoints 单对象上限 1000 端点、全量替换更新、只有 Ready 单状态；EndpointSlice 按 100 端点分片、增量更新、提供 Ready/Serving/Terminating 三态（滚动升级时更准确剔除 Terminating 端点），大规模集群与拓扑感知路由（Topology Aware Routing）都依赖 EndpointSlice。

## 参考链接

- [Endpoints - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/service/#endpoints)

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/headless-service.md|Headless Service]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|ClusterIP]]
- [[17-系统基础/06-知识字典/networking/coredns.md|CoreDNS]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]


<!-- risk-assessed -->
