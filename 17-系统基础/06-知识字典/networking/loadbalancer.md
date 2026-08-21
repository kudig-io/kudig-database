---
title: 负载均衡器
description: LoadBalancer 是 Service 的一种类型，通过云厂商的负载均衡器将服务暴露到集群外部。它自动创建云平台的 LB 资源并配置外部
  IP。...
summary: LoadBalancer 是 Service 的一种类型，通过云厂商的负载均衡器将服务暴露到集群外部。它自动创建云平台的 LB 资源并配置外部 IP。...
category: dictionary
tags:
- k8s
- glossary
- networking
- service
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 负载均衡器 是什么
- LoadBalancer 详解
trigger_keywords:
- 负载均衡器
- LoadBalancer
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 负载均衡器

> **英文名**: LoadBalancer

## 概述

LoadBalancer 是 Service 的一种类型，通过云厂商的负载均衡器将服务暴露到集群外部。它自动创建云平台的 LB 资源并配置外部 IP。

## 核心概念/原理

### 工作原理

```
创建 LoadBalancer Service → CCM 调用云 API → 创建 LB → 分配外部 IP → 配置转发规则
```

### 注解

不同云厂商通过 Service 注解自定义 LB 行为：
- AWS: `service.beta.kubernetes.io/aws-load-balancer-*`
- GCP: `cloud.google.com/load-balancer-type`
- Azure: `service.beta.kubernetes.io/azure-load-balancer-*`

## 关键机制或特性

- 依赖 Cloud Controller Manager（CCM）和云平台 API。
- 每个 LoadBalancer Service 通常创建一个独立的 LB 实例（成本较高）。
- `loadBalancerClass`（v1.24+）支持指定自定义 LB 实现。

## 使用场景与最佳实践

- 需要外部访问时使用 LoadBalancer。
- 大量服务考虑使用 Ingress/Gateway API 共享一个 LB。
- 监控 LB 的健康状态和成本。
- 使用 `--allocate-node-ports=false`（v1.24+）避免暴露 NodePort。

## 架构深度解析

### LoadBalancer Service 模型

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes 集群                       │
│  ┌──────────────────────────────────────────────────┐   │
│  │  LoadBalancer Controller（云厂商/裸金属实现）       │   │
│  │  - 监听 Service(type=LoadBalancer)                │   │
│  │  - 调用云 API 创建 LB（或 MetalLB/kube-vip 分配）  │   │
│  │  - 回写 status.loadBalancer.ingress               │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  云 LB（ELB/SLB/CLB）或裸金属 LB（MetalLB/kube-vip）│   │
│  │  ┌────────────────────────────────────────────┐  │   │
│  │  │ LB VIP: 1.2.3.4 (公网/内网)                │  │   │
│  │  │ 监听器: TCP/UDP port → NodePort:30080      │  │   │
│  │  └────────────────────────────────────────────┘  │   │
│  └──────────────────────────────────────────────────┘   │
│          │ 流量路径                                     │
│          ▼                                              │
│  客户端 ──▶ LB VIP ──▶ NodePort(每节点) ──▶ Pod           │
│              │              │                           │
│              └── 外部流量策略 ──┘                        │
│  externalTrafficPolicy: Cluster（默认，二次跳转）         │
│  externalTrafficPolicy: Local（保留源 IP，就近）          │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| Service 控制器 | `pkg/controller/service/` | LoadBalancer 生命周期协调 |
| 云提供商接口 | `staging/src/k8s.io/cloud-provider` | 云 LB 创建/更新/删除 |
| 端口分配 | `pkg/registry/core/service/portallocator` | NodePort 分配（30000-32767） |

### 流量路径与关键配置

1. 创建 Service(type=LoadBalancer) → 控制器创建云 LB / MetalLB 分配 VIP
2. 云 LB 监听器后端指向节点 NodePort（或直通 Pod）
3. `externalTrafficPolicy: Cluster`：流量经 NodePort 再 kube-proxy 转发（可能二次跳转，源 IP 丢失）
4. `externalTrafficPolicy: Local`：只转发到本节点 Pod（源 IP 保留，需流量均衡配合）
5. v1.24+ 可 `allocateLoadBalancerNodePorts: false` 禁用 NodePort 暴露

## 生产案例

### 案例 1：externalTrafficPolicy=Local 导致流量倾斜

| 时间 | 事件 |
|------|------|
| 14:00 | 切换 externalTrafficPolicy=Local 后，部分节点 5xx 激增 |
| 14:10 | 分析发现 LB 按节点轮询转发，但 Pod 分布不均 |
| 14:20 | 无 Pod 的节点收到流量被丢弃（Local 模式不转发） |
| 14:40 | 调整 Pod 反亲和/拓扑分布，或回退 Cluster 模式 |

**根因**：Local 模式只转发到"有 Pod 的节点"，流量分发与 Pod 分布强耦合；节点数远多于 Pod 副本时丢弃率高。

**修复命令**：
```bash
# 查看 Service 的 externalTrafficPolicy 🟢 只读
kubectl get svc web -o jsonpath='{.spec.externalTrafficPolicy}'
# 回退 Cluster 模式（接受二次跳转与源 IP 丢失）🟡 中风险
kubectl patch svc web -p '{"spec":{"externalTrafficPolicy":"Cluster"}}'
# 或保持 Local 模式并调整 Pod 拓扑 🟡 中风险
# 使用 topologySpreadConstraints 均匀分布 Pod
```

### 案例 2：LoadBalancer 控制器故障导致 Service Pending

**现象**：新建 LoadBalancer Service 一直 `<pending>`，无外部 IP。

**诊断**：云厂商 LB 控制器（cloud-controller-manager）异常；或裸金属场景未安装 MetalLB/kube-vip 控制器；或配额不足（云 API 报错）。

**修复**：检查控制器日志与配额；裸金属场景安装 LB 控制器后重新触发协调（删除重建 Service）。

## 对比评测

| 维度 | 云厂商 LB | MetalLB（裸金属） | kube-vip（裸金属） |
|------|----------|------------------|-------------------|
| 数据面 | 云负载均衡 | FRR/BGP 或 ARP | IPVS/ARP/BGP |
| 源 IP 保持 | ✅ 支持 | 依赖模式 | 依赖模式 |
| 成本 | 按量计费 | 无额外成本 | 无额外成本 |
| 适用场景 | 云上集群 | 裸金属/边缘 | 自建 HA+LB |

**选型建议**：云上直接使用云 LB；裸金属首选 MetalLB；需要控制面 VIP 一体方案选 kube-vip。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| Pending | `kubectl describe svc` 看事件 | LB 控制器故障或配额不足 |
| 访问超时 | 检查 LB 后端节点健康 | NodePort 被防火墙阻断 |
| 源 IP 丢失 | 查 externalTrafficPolicy | Cluster 模式二次跳转 |
| 流量倾斜 | 对比各节点 Pod 数 | Local 模式分布不均 |

## 生产部署清单

- [ ] externalTrafficPolicy 与 Pod 拓扑分布配套设计
- [ ] 监控 LB 数量与成本（云场景）
- [ ] 批量服务收敛到 Ingress/Gateway 共享 LB
- [ ] 裸金属 LB 控制器健康与 VIP 池水位监控
- [ ] v1.24+ 设置 allocateLoadBalancerNodePorts=false（如不需要 NodePort）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | LB 全部 Pending 或流量中断 | 立即检查 LB 控制器与云配额 |
| P1 | 云 LB 规格/类型变更 | 评估迁移窗口与连接保持影响 |
| P2 | 大量 Service 使用独立 LB | 收敛到 Ingress/Gateway 共享入口 |

## 面试要点

> 以下 Q&A 覆盖 LoadBalancer Service 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：LoadBalancer Service 的实现原理是什么？**
   A：它是 ClusterIP/NodePort 的扩展：控制器（云厂商 LB 或 MetalLB/kube-vip）为 Service 分配外部 IP 并创建负载均衡器，后端指向节点 NodePort（或直通 Pod）；外部 IP 回写到 `status.loadBalancer.ingress`，客户端经 LB → NodePort → kube-proxy → Pod 访问。

2. **Q：externalTrafficPolicy=Local 与 Cluster 的区别及取舍？**
   A：Cluster 模式任意节点均可转发（负载均衡由内核完成），代价是二次跳转与源 IP 被 SNAT 覆盖；Local 模式只在本节点有 Pod 时转发，保留源 IP（对审计/限流重要），但流量分发依赖 Pod 分布，节点与副本不匹配时丢包。取舍：源 IP 需求 > 均衡性选 Local，否则 Cluster。

3. **Q：裸金属集群如何实现 LoadBalancer 类型 Service？**
   A：安装 LB 控制器如 MetalLB（二层 ARP 或 BGP 宣告 VIP）或 kube-vip（ARP/BGP + IPVS）：控制器监听 LoadBalancer Service，分配 VIP 并注入转发规则，使裸金属集群获得与云上一致的 LoadBalancer 语义；VIP 池需独立规划且与节点网段兼容。

## 参考链接

- [LoadBalancer - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/service/#loadbalancer)

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|Clusterip]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|Nodeport]]
- [[17-系统基础/06-知识字典/networking/headless-service.md|Headless Service]]


<!-- risk-assessed -->
