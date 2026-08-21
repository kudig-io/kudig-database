---
title: 端点
description: Endpoints 是 Service 后端 Pod 的 IP 地址和端口组合。当 Service 使用 selector 时，Kubernetes
  自动创建对...
summary: Endpoints 是 Service 后端 Pod 的 IP 地址和端口组合。当 Service 使用 selector 时，Kubernetes
  自动创建对...
category: dictionary
tags:
- k8s
- glossary
- endpoints
- networking
tier: peripheral
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

Endpoints 是 Service 后端 Pod 的 IP 地址和端口组合。当 Service 使用 selector 时，Kubernetes 自动创建对应的 Endpoints 对象，记录匹配 Pod 的网络信息。

## 核心概念/原理

### 核心概念

- **自动管理**：Service 的 selector 匹配 Pod 后，Endpoints Controller 自动更新 Endpoints。
- **手动 Endpoints**：不使用 selector 的 Service 可以手动指定 Endpoints，指向外部服务。
- **EndpointSlice**：Endpoints 的替代方案，将端点分片存储，适合大规模集群。

### Endpoints vs EndpointSlice

| 特性 | Endpoints | EndpointSlice |
|------|-----------|---------------|
| 容量 | 单对象存储所有端点 | 分片存储，每片最多 100 个 |
| 性能 | 大规模时 API Server 压力大 | 显著减少 API Server 负载 |
| 推荐 | 小规模 | 生产推荐 |

## 关键机制或特性

- EndpointSlice 从 K8s v1.21 起成为默认方案。
- Endpoints 对象仍可使用但不推荐在大规模集群中使用。
- Headless Service 的 DNS 查询直接返回 Endpoints 中的 Pod IP。

## 使用场景与最佳实践

- 大规模集群确保启用 EndpointSlice API。
- 排查 Service 不通时检查 Endpoints 是否包含正确的后端 Pod。
- 使用 `kubectl get endpointslices` 查看分片的端点信息。

## 架构深度解析

### Endpoints 与 EndpointSlice 关系

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes API                       │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Service（selector 匹配）                         │   │
│  │  └─▶ EndpointSlice Controller 自动生成            │   │
│  │      （v1.21+ 默认，替代 Endpoints 对象）          │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  EndpointSlice（按分片，每片默认 ≤100 端点）        │   │
│  │  - addresses: 10.0.1.5 / 10.0.2.7                │   │
│  │  - ports: [{port: 8080, protocol: TCP}]          │   │
│  │  - conditions: ready / serving / terminating     │   │
│  │  - labels: kubernetes.io/service-name=<svc>      │   │
│  └──────────────────────────────────────────────────┘   │
│                                                         │
│  消费者：kube-proxy（转发规则）/ CoreDNS（记录）/        │
│         Ingress 控制器（Upstream）/ 服务网格（端点）      │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| EndpointSlice 控制器 | `pkg/controller/endpointslice/` | Service→EndpointSlice 生成与更新 |
| Endpoints 控制器 | `pkg/controller/endpoint/` | 旧版 Endpoints 对象维护 |
| 拓扑感知 | `pkg/controller/endpointslice/topologycache` | 拓扑感知路由（Topology Aware Routing） |
| 地址处理 | `pkg/controller/endpointslice/utils.go` | 就绪/未就绪/终止状态判定 |

### 状态判定逻辑

1. Pod 通过 Service selector 匹配后进入候选集合
2. 判定条件：`Ready`（就绪探针 + Running）、`Serving`、`Terminating`（删除中）
3. 满足条件的 Pod 写入 `addresses`，未就绪写入 `notReadyAddresses`
4. 每次 Pod 事件（就绪/删除/IP 变化）触发 EndpointSlice 更新
5. 分片超过 100 端点自动拆分为多个 EndpointSlice（同 service-name label）

## 生产案例

### 案例 1：Service 不通但 Pod 正常——Endpoints 为空

| 时间 | 事件 |
|------|------|
| 16:00 | 业务反馈 Service 访问超时，Pod 均 Running |
| 16:05 | `kubectl get endpoints` 显示 <none> |
| 16:10 | 检查 Service selector 与 Pod label 不匹配（部署标签变更） |
| 16:20 | 修正 selector 后 Endpoints 立即填充，服务恢复 |

**根因**：Service 的 `selector` 与 Pod 的 `labels` 不一致（大小写、拼写、键值差异），控制器无法匹配任何 Pod。

**修复命令**：
```bash
# 查看 Service selector 与 Pod labels 🟢 只读
kubectl get svc orders -n app -o jsonpath='{.spec.selector}'
kubectl get pods -n app --show-labels
# 修正 selector（或补齐 Pod label）🟡 中风险
kubectl edit svc orders -n app
# 验证 Endpoints 生成 🟢 只读
kubectl get endpoints slices -l kubernetes.io/service-name=orders -n app
```

### 案例 2：Pod 就绪但流量未转发（拓扑感知路由误判）

**现象**：开启拓扑感知路由（topologyAwareHints）后，部分节点流量倾斜或不通。

**诊断**：拓扑感知路由依赖 EndpointSlice 的 topology 信息（zone/region）；节点 zone 标签缺失或不一致导致提示信息（hints）错误，流量集中在少数端点。

**修复**：统一节点 `topology.kubernetes.io/zone` 标签；确认 `service.kubernetes.io/topology-aware-hints: Auto` 注解的语义；必要时关闭拓扑感知恢复全量转发。

## 对比评测

| 维度 | Endpoints（旧） | EndpointSlice（新） |
|------|-----------------|---------------------|
| 规模限制 | 单对象 1000 端点 | 每分片 100（可扩展） |
| 更新粒度 | 全量替换 | 分片独立更新 |
| 状态字段 | Ready 布尔 | Ready/Serving/Terminating |
| 默认启用 | 早期版本 | v1.21+ |
| 适用场景 | 兼容旧工具 | 生产默认 |

**选型建议**：新集群默认 EndpointSlice；旧工具链需兼容 Endpoints 时双写共存过渡。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| Endpoints 为空 | `kubectl get endpoints -n <ns>` | selector 与 label 不匹配 |
| 端点不更新 | `kubectl get endpointslices -l kubernetes.io/service-name=` | 控制器异常或 API 版本问题 |
| 流量不均衡 | 检查 topology hints | zone 标签缺失或拓扑配置错误 |
| 就绪判定异常 | `kubectl describe pod` 查探针 | 就绪探针失败 |

## 生产部署清单

- [ ] Service selector 与 Pod label 规范统一（命名/大小写约定）
- [ ] 大规模集群确认 EndpointSlice 启用并监控分片数量
- [ ] 拓扑感知路由开启前检查节点 zone 标签完备性
- [ ] 监控 EndpointSlice 同步延迟指标
- [ ] 服务治理工具链兼容性验证（对接 EndpointSlice）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Service 端点异常导致流量中断 | 立即核对 selector/label 并检查控制器健康 |
| P1 | 拓扑感知路由开启 | 先验证 zone 标签与提示信息再全量 |
| P2 | 大规模集群性能优化 | 确保 EndpointSlice 分片策略生效 |

## 面试要点

> 以下 Q&A 覆盖 Endpoints 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Service 访问不通时如何快速定位 Endpoints 问题？**
   A：先 `kubectl get endpoints <svc>` 看是否存在且包含目标 IP：为空→selector 与 Pod label 不匹配；有 IP 但流量不通→检查 kube-proxy 规则与网络策略；IP 与 Pod 实际不符→检查控制器同步延迟与多 Service 共享端点冲突。

2. **Q：EndpointSlice 相比 Endpoints 解决了什么问题？**
   A：旧 Endpoints 单对象存储全量端点，更新是全量替换且容量受限（1000）；EndpointSlice 按 100 端点分片，更新只影响所属分片，并引入 Ready/Serving/Terminating 三态条件（滚动升级场景更准确），支撑大规模集群的服务发现。

3. **Q：拓扑感知路由（Topology Aware Routing）如何工作？**
   A：基于 EndpointSlice 中端点的拓扑信息（zone），控制器为每个端点计算"提示"（hints）：优先返回同 zone 端点，跨 zone 流量仅在同 zone 无可用端点时使用；客户端（kube-proxy/Cilium）按 hints 选择端点，减少跨 zone 流量。前提是节点 zone 标签完备且流量分布可预测。

## 参考链接

- [Endpoints - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/)

## Related

[[17-系统基础/06-知识字典/networking/endpointslices.md|EndpointSlices]]


<!-- risk-assessed -->
