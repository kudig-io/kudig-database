---
title: Spiderpool IP 池管理
description: Spiderpool 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供 Underlay 网络的
  IP 地址管理（I...
summary: Spiderpool 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供 Underlay 网络的 IP
  地址管理（I...
category: dictionary
tags:
- k8s
- glossary
- networking
- ipam
- cni
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Spiderpool IP 池管理 是什么
- Spiderpool 详解
trigger_keywords:
- Spiderpool IP 池管理
- Spiderpool
- dictionary
prerequisites:
- kubernetes
---



# Spiderpool IP 池管理（Spiderpool）

## 概述

Spiderpool 是 DaoCloud 开源的 CNCF Sandbox 项目，为 Kubernetes 提供 Underlay 网络的 IP 地址管理（IPAM），解决容器使用固定 IP 和 Underlay 网络的挑战。

## 核心概念/原理

- **Underlay IPAM**：为 Pod 分配 Underlay 网络的固定 IP
- **多 CNI 兼容**：支持 Macvlan、IPVLAN、SR-IOV、IB SR-IOV
- **CNCF Sandbox**：DaoCloud 主导
- **固定 IP**：支持 Pod 固定 IP 和 IP 池管理

## 关键机制或特性

- SpiderIPPool / SpiderSubnet / SpiderEndpoint CRD
- 固定 IP（Pod Annotation 指定 IP）
- IP 池管理和自动回收
- 多网卡 IPAM（Multus 集成）
- IP 冲突检测和自动修复
- Webhook 验证 IP 合法性
- IPv4/IPv6 双栈支持

## 使用场景与最佳实践

- 需要 Pod 固定 IP 的场景（金融/电信）
- Underlay 网络的 K8s 部署
- 多网卡 Pod 的 IP 管理
- SR-IOV 高性能网络的 IP 分配
- 传统网络环境的 K8s 集成

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes 集群                       │
│  ┌──────────────────────────────────────────────────┐   │
│  │  spiderpool-controller（控制面）                   │   │
│  │  - SpiderIPPool / SpiderSubnet CRD 管理           │   │
│  │  - IP 分配与回收（Deallocation）                   │   │
│  │  - Webhook 校验 IP 合法性                          │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  spiderpool-agent（每节点 DaemonSet）              │   │
│  │  - 本地 IP 池缓存与分配（低延迟）                  │   │
│  │  - 路由配置 / 网关配置下发                        │   │
│  │  - 与 Multus / SR-IOV / Macvlan 集成              │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  网络附件（NetworkAttachmentDefinition）           │   │
│  │  - IP 池绑定 / 固定 IP 声明 / 自动分配             │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（spidernet-io/spiderpool）

| 模块 | 路径 | 职责 |
|------|------|------|
| Controller | `pkg/controllers/` | SpiderIPPool/SpiderSubnet 协调与分配 |
| Agent | `pkg/agent/` | 节点侧 IP 分配缓存、路由/网关配置 |
| Multus 集成 | `pkg/multus/` | 为多网卡 Pod 生成网络配置 |
| API/Webhook | `pkg/apis/` | CRD 定义与准入校验 |
| IPAM 核心 | `pkg/ipam/` | 分配/回收/固定 IP 的核心逻辑 |

### IP 分配流程

1. 管理员创建 `SpiderSubnet` 声明网段，子池 `SpiderIPPool` 定义分配策略（自动/固定/保留）
2. Pod 创建时 Multus 调用 spiderpool CNI（作为 meta CNI 的 IPAM）
3. Agent 优先从本地缓存分配 IP（避免每次查询 API Server）
4. 控制器记录分配（IP 与 Pod 绑定），Pod 删除时回收
5. 支持 IPv4/IPv6 双栈与 `SpiderEndpoint`（跨 Pod 迁移保留 IP）

## 生产案例

### 案例 1：固定 IP 状态化应用重建后 IP 漂移

| 时间 | 事件 |
|------|------|
| 10:00 | 数据库 Pod 重建后 IP 变化，白名单访问全部失效 |
| 10:10 | 检查发现固定 IP 通过 annotation 声明，Pod 重建时未生效 |
| 10:20 | 定位为固定 IP 绑定的是 Pod 名，StatefulSet 重建序号变化后失配 |
| 10:40 | 改用 `SpiderEndpoint` 按应用组绑定，IP 稳定保留 |

**根因**：固定 IP 的绑定维度选择错误（按 Pod 名而非应用标识）；StatefulSet 重建后 Pod 名变化导致 IP 重新分配。

**修复命令**：
```bash
# 查看 IP 池与分配记录 🟢 只读
kubectl get spiderippool -o wide
kubectl get spiderendpoints -o wide
# 为应用组声明固定 IP（YAML）🟡 中风险
# apiVersion: spiderpool.spidernet.io/v2beta1
# kind: SpiderIPPool
# spec:
#   ips: ["10.20.0.10-10.20.0.20"]
#   application: { appKind: StatefulSet, appName: db }
kubectl apply -f fixed-ippool.yaml
```

### 案例 2：多网卡 Pod 主网卡 IP 被误回收

**现象**：SR-IOV 多网卡 Pod 运行中主网卡 IP 丢失，网络中断。

**诊断**：多网卡场景下主网卡与辅助网卡使用同一 IP 池；辅助网卡删除时误回收了主网卡 IP（同一 Pod 多网卡共享分配的边界问题）。

**修复**：为不同网卡分配独立 IP 池（主/辅分离）；升级 spiderpool 至修复版本；配置 `SpiderIPPool` 的 `multiNetworkIndex` 与网卡绑定关系。

## 对比评测

| 维度 | Spiderpool | Whereabouts | 静态 IPAM（Multus 内置） |
|------|-----------|-------------|--------------------------|
| 固定 IP | ✅ 应用级绑定 | 部分 | 手动声明 |
| 双栈 | ✅ | ✅ | 有限 |
| 回收机制 | 控制器协调 | IP 池 GC | 无 |
| 多网卡 | ✅ 深度集成 | 基本 | 每网卡独立 |
| 运维复杂度 | 中（CRD 丰富） | 低 | 低 |

**选型建议**：Underlay/多网卡/固定 IP 复杂需求选 Spiderpool；简单场景 Whereabouts 足够；静态分配仅适合测试环境。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| IP 分配失败 | `kubectl logs ds/spiderpool-agent` | 池耗尽或 Webhook 拒绝 |
| 固定 IP 漂移 | `kubectl get spiderendpoints` | 绑定维度错误 |
| 多网卡冲突 | 检查各网卡 IP 池归属 | 池混用导致误回收 |
| 双栈异常 | `kubectl get spiderippool` 查 v6 池 | v6 池未创建或路由缺失 |

## 生产部署清单

- [ ] 关键应用固定 IP 用应用级绑定（SpiderEndpoint）
- [ ] 多网卡场景主/辅网卡 IP 池分离
- [ ] IP 池水位监控与告警（预留 20% 余量）
- [ ] 升级前备份 SpiderSubnet/SpiderIPPool CRD 清单
- [ ] 定期审计分配记录与 Pod 实际 IP 一致性

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 批量 IP 分配失败或误回收 | 立即检查池状态并回滚相关变更 |
| P1 | CNI（Multus/SR-IOV）版本升级 | 验证 spiderpool 兼容矩阵再全量 |
| P2 | 需要跨集群 IP 池共享 | 评估多集群 IPAM 方案 |

## 面试要点

> 以下 Q&A 覆盖 Spiderpool 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Spiderpool 与 Whereabouts 的核心差异？**
   A：Whereabouts 是轻量 IPAM（基于 IPPool 注解），固定 IP 能力有限；Spiderpool 提供完整的 CRD 体系（SpiderSubnet/SpiderIPPool/SpiderEndpoint），支持应用级固定 IP、双栈、多网卡归属、IP 回收审计，以及与 SR-IOV/Macvlan 深度集成，适合生产级 Underlay 网络。

2. **Q：固定 IP 如何做到 Pod 重建后保持不变？**
   A：通过 SpiderEndpoint 将 IP 绑定到应用标识（Deployment/StatefulSet 名称），Pod 重建时 spiderpool-controller 依据应用标识复用原 IP；同时支持跨节点迁移保留（IP 跟随应用而非节点），配合 StatefulSet 的稳定网络身份实现完整固定 IP 语义。

3. **Q：spiderpool 在多网卡（Multus）场景中扮演什么角色？**
   A：Multus 负责为 Pod 挂载多张网卡，spiderpool 作为其 IPAM：为每张网卡按 NetworkAttachmentDefinition 声明的池分配 IP，并管理主/辅网卡的路由与网关策略（默认路由只走主网卡），解决"多网卡 + IP 管理 + 路由策略"的组合问题。

## 参考链接

- https://spiderpool.dev/
- https://github.com/spidernet-io/spiderpool

## Related

- [[17-系统基础/06-知识字典/networking/cni.md|CNI]]
- [[17-系统基础/06-知识字典/networking/metallb.md|MetalLB]]
- [[17-系统基础/06-知识字典/networking/antrea.md|Antrea]]
