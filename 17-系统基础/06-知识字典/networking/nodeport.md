---
title: 节点端口
description: NodePort 是 Service 的一种类型，在每个节点上暴露一个固定端口（默认 30000-32767），外部流量可以通过 `NodeIP:NodePor...
summary: NodePort 是 Service 的一种类型，在每个节点上暴露一个固定端口（默认 30000-32767），外部流量可以通过 `NodeIP:NodePor...
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
- 节点端口 是什么
- NodePort 详解
trigger_keywords:
- 节点端口
- NodePort
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点端口

> **英文名**: NodePort

## 概述

NodePort 是 Service 的一种类型，在每个节点上暴露一个固定端口（默认 30000-32767），外部流量可以通过 `NodeIP:NodePort` 访问集群内部服务。

## 核心概念/原理

### 核心概念

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
```

## 关键机制或特性

- NodePort 在所有节点上暴露相同端口。
- `externalTrafficPolicy: Local` 保留客户端源 IP。
- NodePort 是 LoadBalancer 的基础（LoadBalancer 类型自动创建 NodePort）。

## 使用场景与最佳实践

- 开发/测试环境快速暴露服务。
- 生产环境优先使用 LoadBalancer 或 Ingress。
- 注意端口冲突和安全风险（暴露节点端口到外部）。

## 架构深度解析

### NodePort 流量路径

```
┌──────────────────────────────────────────────────────────────┐
│  外部客户端（NodeIP:NodePort）                                 │
│   │                                                            │
│   ▼                                                            │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 节点 A（宿主机网络栈）                                    │  │
│  │  kube-proxy 监听 NodePort（--nodeport-addresses 可选）   │  │
│  │  ┌────────────────────────────────────────────────────┐ │  │
│  │  │ iptables KUBE-NODEPORTS 链                        │ │  │
│  │  │  匹配 dport=NodePort → DNAT 到后端 Pod IP          │ │  │
│  │  │  模式：ExternalTrafficPolicy                        │ │  │
│  │  │  ├─ Cluster（默认）：任意节点可达任意后端           │ │  │
│  │  │  │   后端可能在远端节点 → 跨节点转发（SNAT）        │ │  │
│  │  │  └─ Local：仅本节点 Pod 接收 → 保留源 IP            │ │  │
│  │  └────────────────────────────────────────────────────┘ │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │                                                            │
│   ▼                                                            │
│  后端 Pod（10.244.x.x:port）                                   │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| Service 控制器 | `pkg/controller/service/service_controller.go` | 为 NodePort 类型分配端口并写入 Service.Spec.ports[].nodePort |
| kube-proxy iptables | `pkg/proxy/iptables/proxier.go` | 生成 KUBE-NODEPORTS 链及 DNAT 规则 |
| kube-proxy IPVS | `pkg/proxy/ipvs/proxier.go` | 创建 `nodePort` 类型 VirtualServer，绑定 real server |
| apiserver 校验 | `pkg/registry/core/service/strategy.go` | 校验 NodePort 范围（默认 30000-32767）与端口冲突 |

### 流程步骤

1. 创建 Service(type=NodePort) 时，apiserver 从 `--service-node-port-range` 分配端口。
2. kube-proxy watch 到 Service/EndpointSlice 变更，生成 DNAT 规则（iptables）或 VS 条目（IPVS）。
3. 客户端访问 `任意节点IP:NodePort`，流量被 DNAT 到后端 Pod。
4. `externalTrafficPolicy: Local` 时仅本节点 Pod 接收，健康检查端口 `healthCheckNodePort` 暴露节点状态。
5. LoadBalancer 类型在此基础上由云控制器创建云 LB 并关联节点端口。

## 生产案例

### 案例 1：NodePort 端口耗尽导致新服务创建失败

| 时间 | 事件 |
|------|------|
| 14:00 | 应用发布新增 20 个 NodePort 服务 |
| 14:05 | 报错 `failed to allocate node port: port is already allocated` |
| 14:10 | `kubectl get svc -A -o json | jq '[.items[].spec.ports[].nodePort] | unique | length'` 统计已用端口 3000+ |
| 14:20 | 排查发现历史环境遗留大量未清理的 NodePort 服务 |
| 14:30 | 清理废弃服务后恢复；后续通过准入控制限制 NodePort 使用 |

**根因**：NodePort 默认范围 30000-32767 仅 2768 个端口，未回收的废弃服务占用殆尽。
**修复命令**：
```bash
# 统计已分配的 NodePort 数量 🟢 只读
kubectl get svc -A -o json | jq '[.items[].spec.ports[].nodePort] | unique | length'
# 找出无 Endpoints 的孤儿服务 🟢 只读
kubectl get svc -A | awk '$5 ~ /<none>/'
# 修改默认端口范围（kube-apiserver 参数，需重启）🟡 中风险
# --service-node-port-range=20000-40000
```

### 案例 2：NodePort 服务延迟高（跨节点转发）

**现象**：通过 NodePort 访问服务 P99 延迟 800ms，内部 ClusterIP 访问仅 20ms。
**诊断**：`externalTrafficPolicy` 为默认 Cluster，流量可能先到无后端 Pod 的节点再二次转发；`kubectl get svc -o yaml` 确认策略；抓包确认两次 NAT。
**修复**：将 `externalTrafficPolicy` 改为 `Local` 保留源 IP 并减少一跳（注意后端分布不均问题）；或前置负载均衡器只将流量导向有 Pod 的节点。

## 对比评测

| 维度 | NodePort | LoadBalancer | Ingress |
|------|----------|--------------|---------|
| 暴露层级 | 节点 IP:端口 | 云 LB VIP | 7 层域名/路径 |
| 端口资源 | 30000-32767 有限 | 无限制（每服务一 LB） | 无限制（共享 80/443） |
| 成本 | 低 | 高（每服务一个 LB） | 中（一个 LB + Ingress Controller） |
| 适用场景 | 测试/临时暴露 | 需要云 LB 能力 | 生产 HTTP(S) 服务 |

**选型建议**：生产 HTTP 服务用 Ingress；非 HTTP 协议用 LoadBalancer；NodePort 仅限测试与裸金属场景。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| NodePort 无法访问 | `kubectl get svc`；`ss -lntp \| grep <port>` | kube-proxy 未同步、防火墙未放行 |
| 端口分配失败 | `kubectl get svc -A` 统计 | 端口耗尽、范围冲突 |
| 延迟高 | 检查 ExternalTrafficPolicy | 跨节点二次转发 |
| 源 IP 丢失 | 查看 `externalTrafficPolicy` | Cluster 模式 SNAT |
| 部分节点不通 | `kubectl get nodes`；`kubectl get endpoints <svc>` | 节点 NotReady、kube-proxy 异常 |

## 生产部署清单

- [ ] NodePort 范围已调整并记录（默认 30000-32767 是否够用）
- [ ] 安全组/防火墙仅放行必要节点的 NodePort 端口
- [ ] 生产服务已用 LoadBalancer/Ingress 替代 NodePort
- [ ] 使用准入策略限制 NodePort 滥用（如 ValidatingAdmissionPolicy）
- [ ] 确认 externalTrafficPolicy 选择符合延迟/源 IP 需求

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 端口分配失败影响发布 | 清理废弃服务或调整 service-node-port-range |
| P1 | 安全要求不暴露节点端口 | 迁移到 LoadBalancer/Ingress，关闭 NodePort 支持 |
| P1 | 需要保留客户端源 IP | 设置 externalTrafficPolicy: Local |
| P2 | NodePort 仅测试使用 | 维持现状，纳入端口管理规范 |

## 面试要点

> 以下 Q&A 覆盖 NodePort 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：NodePort、ClusterIP、LoadBalancer 三者关系是什么？**
   A：ClusterIP 是集群内部虚拟 IP（仅集群内可达）；NodePort 在 ClusterIP 基础上将服务暴露到所有节点的指定端口（30000-32767），外部流量经 DNAT 到达后端；LoadBalancer 在 NodePort 基础上由云控制器创建外部负载均衡器，将流量导入节点端口。三者是叠加关系：LoadBalancer 隐含 NodePort，NodePort 隐含 ClusterIP。

2. **Q：externalTrafficPolicy: Cluster 与 Local 有什么区别？**
   A：Cluster（默认）模式下流量到达任意节点都会被转发到任意后端 Pod，可能发生跨节点二次转发（额外一跳 + SNAT，丢失源 IP）；Local 模式下流量只在本节点 Pod 之间转发（无跨节点、保留源 IP），但要求所有节点都有后端 Pod 才能均衡（否则健康检查节点会剔除无后端的节点，用 healthCheckNodePort 探测）。选择 Local 时需关注 Pod 分布与容量。

3. **Q：NodePort 端口耗尽如何排查和预防？**
   A：排查用 `kubectl get svc -A -o json | jq '[.items[].spec.ports[].nodePort] | unique | length'` 统计占用，找出废弃服务清理；预防手段：① 调大 `--service-node-port-range`；② 用 ValidatingAdmissionPolicy 限制 NodePort 使用场景；③ 生产服务统一走 Ingress/LoadBalancer；④ 对 Service 加标签定期巡检。

## 参考链接

- [NodePort - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/service/#type-nodeport)

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|Clusterip]]
- [[17-系统基础/06-知识字典/networking/loadbalancer.md|Loadbalancer]]
- [[17-系统基础/06-知识字典/networking/headless-service.md|Headless Service]]


<!-- risk-assessed -->
