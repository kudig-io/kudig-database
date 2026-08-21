---
title: Clusternet 多集群网络
description: Clusternet 是 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理和连接能力，通过代理模式实现跨集群 API
  访问和资源分发，无...
summary: Clusternet 是 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理和连接能力，通过代理模式实现跨集群 API 访问和资源分发，无...
category: dictionary
tags:
- k8s
- glossary
- networking
- multi-cluster
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Clusternet 多集群网络 是什么
- Clusternet 详解
trigger_keywords:
- Clusternet 多集群网络
- Clusternet
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Clusternet 多集群网络（Clusternet）

## 概述

Clusternet 是 CNCF Sandbox 项目，提供 Kubernetes 多集群的管理和连接能力，通过代理模式实现跨集群 API 访问和资源分发，无需修改底层网络。

## 核心概念/原理

- **API 代理**：通过代理方式访问子集群 API，无需直连
- **应用分发**：支持 ManifestWork 式的应用分发
- **Scheduler 插件**：多集群调度策略
- **CNCF Sandbox**：轻量级多集群管理方案

## 关键机制或特性

- Hub 集群 + Agent 部署模式
- ServiceExport / ServiceImport 多集群服务发现
- 跨集群 Helm Chart 安装
- 多集群调度框架插件
- 支持边缘集群（弱网环境）
- 与 Karmada 互补的多集群方案

## 使用场景与最佳实践

- 多集群 API 统一访问
- 跨集群应用分发和管理
- 边缘集群的集中管理
- 弱网环境下的集群互联
- 多集群 Helm 应用编排

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────────┐
│                Clusternet Hub（中心集群）                  │
│  ┌──────────────────────────────────────────────────┐   │
│  │  clusternet-hub（API 聚合层）                      │   │
│  │  - Aggregated API：跨集群统一访问                 │   │
│  │  - ClusterRegistration / ClusterFeed 协调         │   │
│  │  - Helm Chart 分发与发布                         │   │
│  └──────────────────────────────────────────────────┘   │
│                          │ HTTPS/gRPC                    │
│                          ▼                              │
│  ┌──────────────────────────────────────────────────┐   │
│  │  clusternet-agent（成员集群）                      │   │
│  │  - 反向连接（弱网/公网穿透）                      │   │
│  │  - 本地执行 Feed（Deployment/Helm）               │   │
│  │  - 状态与事件回传                                 │   │
│  └──────────────────────────────────────────────────┘   │
│  （成员集群间可选：Submariner/KubeSlice 网络互通）        │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（clusternet/clusternet）

| 模块 | 路径 | 职责 |
|------|------|------|
| Hub | `pkg/hub/` | 集群注册、资源模板、分发与发布协调 |
| Agent | `pkg/agent/` | 成员集群执行与反向连接 |
| API 聚合 | `pkg/apis/` | ClusterRegistration/FeedInventory 等 CRD |
| Helm 集成 | `pkg/hub/helm/` | Chart 打包与跨集群发布 |

### 应用分发流程

1. 成员集群安装 agent 并注册（ClusterRegistration），Hub 审批通过
2. 管理员在 Hub 创建 `ClusterFeed`（包含资源模板，支持 Helm Chart）
3. Hub 将 Feed 分发到目标集群的 `FeedInventory`
4. Agent 在成员集群执行（Apply 或 Helm 安装），状态回传
5. 管理员通过 Aggregated API 直接操作任意成员集群（KubeConfig 免切换）

## 生产案例

### 案例 1：弱网环境下 Agent 反向连接频繁掉线

| 时间 | 事件 |
|------|------|
| 09:00 | 边缘集群与中心 Hub 网络抖动，Agent 反复重连 |
| 09:10 | 部分 Feed 分发失败，应用发布卡住 |
| 09:20 | 定位为反向连接无心跳保活，NAT 会话超时后连接失效 |
| 09:40 | 配置心跳间隔与重连退避，连接稳定 |

**根因**：反向连接（outbound-only）依赖 NAT 会话保持；未配置应用层心跳导致中间设备（防火墙/NAT）回收会话。

**修复命令**：
```bash
# 查看 Agent 连接状态 🟢 只读
kubectl -n clusternet-system logs deploy/clusternet-agent | grep -iE "connect|heartbeat"
# 调整心跳与重连参数（Agent 启动参数）🟡 中风险
# --heartbeat-frequency=30s --reconnect-backoff-min=5s
# 验证 Hub 侧在线状态 🟢 只读
kubectl -n clusternet-system get clusters.clusternet.io -o wide
```

### 案例 2：跨集群发布同名 Helm Release 冲突

**现象**：通过 ClusterFeed 发布 Helm Chart 到多个集群，部分集群安装失败。

**诊断**：Chart 的 release 名称在各集群内独立但模板引用全局命名资源（如 ClusterRole 名称冲突），或 Chart 依赖的 Namespace 未在目标集群存在。

**修复**：Feed 中显式声明 Namespace 与全局资源命名规范（按集群后缀）；发布前在预发集群执行 `helm template --validate` 校验；失败 Feed 的 Status 提供具体错误便于定位。

## 对比评测

| 维度 | Clusternet | Karmada | Rancher |
|------|-----------|---------|---------|
| 架构 | Hub + 反向连接 Agent | 控制面 + 成员集群 | 管理面 + 导入集群 |
| 弱网适配 | ✅ 反向连接 | 常规连接 | 常规 |
| API 聚合 | ✅ Aggregated API | 部分 | ❌ |
| 应用分发 | Feed/Helm | PropagationPolicy | Rancher Apps |
| 适用场景 | 边缘/公网多集群 | 多云大规模分发 | 多集群管理 UI |

**选型建议**：边缘/弱网集群纳管选 Clusternet；云上大规模策略化分发选 Karmada；可视化多集群管理选 Rancher。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 集群离线 | `kubectl get clusters.clusternet.io` | 反向连接中断或心跳超时 |
| Feed 失败 | 查看 FeedInventory Status | 模板错误或成员集群资源冲突 |
| 分发卡住 | Agent 日志查执行状态 | 网络抖动或 agent 版本不一致 |
| 聚合 API 超时 | 直接 kubectl 测试成员集群 | Hub 与成员集群连通性 |

## 生产部署清单

- [ ] Agent 心跳与重连参数调优（弱网场景必配）
- [ ] 全局资源（RBAC/CRD）命名规范防跨集群冲突
- [ ] Feed 发布走 GitOps 流程，保留回滚版本
- [ ] Hub 高可用部署（3 副本）+ 数据备份
- [ ] 定期演练成员集群掉线恢复流程

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Hub 故障或大批成员集群离线 | 立即恢复 Hub 并检查 Agent 连接参数 |
| P1 | Clusternet 大版本升级（Hub+Agent 协同） | 升级顺序：先 Hub 后 Agent，验证兼容性 |
| P2 | 从单 Hub 演进多 Hub 容灾 | 评估 Shadow Hub 与 DNS 切换方案 |

## 面试要点

> 以下 Q&A 覆盖 Clusternet 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Clusternet 的"反向连接"设计解决了什么问题？**
   A：成员集群（尤其边缘/公网）通常没有公网 IP 或不允许入站连接，Hub 无法主动访问；Agent 通过 outbound-only 反向连接注册并保持长连接，Hub 下发指令、成员集群回传状态都复用这条通道，实现无入站端口的集群纳管。

2. **Q：Clusternet 与 Karmada 的应用分发模型有何差异？**
   A：Karmada 采用"控制面调度"模型：中心侧定义 PropagationPolicy 声明式分发，成员集群只接收最终对象；Clusternet 采用"Feed 下发 + Agent 执行"模型：分发单元可以是资源模板或 Helm Chart，由 Agent 在成员集群本地执行（含 Helm 语义），更贴近"远程执行"而非"中央编排"。

3. **Q：Aggregated API 如何实现跨集群统一访问？**
   A：Clusternet Hub 将各成员集群的 API 资源通过 API 聚合层统一暴露：管理员使用单一 KubeConfig 即可访问任意集群的资源（路径带集群标识），免去多 KubeConfig 切换；请求经 Hub 转发到目标集群，权限模型由成员集群自身 RBAC 决定。

## 参考链接

- https://clusternet.io/
- https://github.com/clusternet/clusternet

## Related

- [[17-系统基础/06-知识字典/platform-engineering/karmada.md|Karmada]]
- [[17-系统基础/06-知识字典/networking/submariner.md|Submariner]]
- [[17-系统基础/06-知识字典/platform-engineering/rancher.md|Rancher]]


<!-- risk-assessed -->
