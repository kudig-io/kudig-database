---
title: Consul
description: Consul 是 HashiCorp 开源的服务网格和服务发现解决方案。它提供服务发现、健康检查、KV 存储和服务网格（通过 Envoy
  sidecar）等功能...
summary: Consul 是 HashiCorp 开源的服务网格和服务发现解决方案。它提供服务发现、健康检查、KV 存储和服务网格（通过 Envoy sidecar）等功能...
category: dictionary
tags:
- k8s
- glossary
- consul
- service-mesh
- service-discovery
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Consul 是什么
- Consul 详解
trigger_keywords:
- Consul
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Consul

> **英文名**: Consul

## 概述

Consul 是 HashiCorp 开源的服务网格和服务发现解决方案。它提供服务发现、健康检查、KV 存储和服务网格（通过 Envoy sidecar）等功能，支持多数据中心和多云部署。

## 核心概念/原理

### 核心功能

| 功能 | 说明 |
|------|------|
| Service Discovery | 服务注册和 DNS/HTTP 发现 |
| Health Checking | 多维度健康检查 |
| KV Store | 分布式键值存储 |
| Service Mesh | 基于 Envoy 的 L7 流量管理 |
| Multi-DC | 多数据中心联邦 |

### 与 K8s Service 对比

Consul 可补充 K8s 的服务发现：跨集群、非 K8s 服务、多数据中心场景。

## 关键机制或特性

- **Consul Connect**：基于 Envoy 的 mTLS 服务网格。
- **Catalog Sync**：K8s Service 与 Consul Catalog 双向同步。
- **Intentions**：声明式的服务间访问控制策略。
- **Mesh Gateway**：跨数据中心的服务网格通信。
- 支持 Terraform 管理 Consul 配置。

## 使用场景与最佳实践

- 混合云/多云场景使用 Consul 统一服务发现。
- 非 K8s 服务（VM、裸金属）需要纳入服务网格时使用 Consul。
- 使用 Consul KV 存储应用配置。
- 配合 Vault 实现服务间证书管理。
- 使用 `consul-k8s` CLI 安装到 Kubernetes。

## 参考链接

- [Consul Official](https://www.consul.io/)

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              HashiCorp Consul                       │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Server      │  │ Client Agent │  │ Connect   │  │
│  │ (Raft 集群) │  │ (每节点)     │  │ (Service  │  │
│  │             │  │              │  │  Mesh)    │  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │     Service Catalog + KV Store + DNS        │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（hashicorp/consul）

| 模块 | 路径 | 职责 |
|------|------|------|
| Agent | `agent/` | Client/Server Agent 主循环 |
| Catalog | `agent/consul/` | 服务注册与发现 |
| Raft | `agent/consul/fsm/` | 一致性协议与状态机 |
| Connect | `connect/` | Service Mesh（mTLS/意图） |
| DNS | `agent/dns/` | DNS 接口（service.consul） |
| K8s | `connect/kube/` | Kubernetes 集成 |

### 服务发现流程

1. 服务启动 → 向本地 Client Agent 注册
2. Client Agent 同步到 Server 集群（Raft 共识）
3. 服务发现：DNS（`svc.service.consul`）或 HTTP API
4. 健康检查失败 → 自动从 Catalog 摘除
5. Connect：服务间 mTLS + 意图（Intention）授权

## 生产案例

### 案例 1：Raft 集群失去 Leader

| 时间 | 事件 |
|------|------|
| 05:00 | Consul Server 集群 3 节点中 2 节点宕机 |
| 05:01 | 服务发现失败，DNS 查询超时 |
| 05:10 | 确认：Raft 无法选举 Leader（需要多数派） |
| 05:20 | 修复：恢复宕机节点，或使用 `consul operator raft remove-peer` 移除故障节点 |

**修复命令**：
```bash
# 检查 Raft 状态 🟢 只读
consul operator raft list-peers
# 查看服务健康 🟢 只读
consul catalog services -tags
# 移除故障节点 🔴 高风险
consul operator raft remove-peer -address="10.0.0.5:8300"
```

### 案例 2：Connect mTLS 证书轮转失败

**现象**：服务间通信失败，日志显示 `certificate has expired`。

**诊断**：Consul Connect CA 的中间证书过期，未自动轮转。

**修复**：手动触发 CA 轮转，或切换到 Vault 作为 Connect CA。

## 对比评测

| 维度 | Consul | Istio | 自建注册中心 |
|------|--------|-------|-------------|
| 服务发现 | 原生（强一致） | 依赖 K8s | 自研 |
| 服务网格 | 原生（Connect） | 完整 | 无 |
| 多数据中心 | 原生（WAN 池） | 需多集群 | 自研 |
| 配置中心 | 原生（KV） | 无 | 自研 |
| 非 K8s 支持 | VM/裸机原生 | 弱 | 强 |

**选型建议**：VM + K8s 混合架构且需要发现/配置/网格一体化选 Consul；纯 K8s HTTP 生态选 Istio；轻量场景用 K8s 原生 Service。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 服务注册失败 | `consul members`；`consul operator raft list-peers` | Server 不可用、ACL 令牌错误 |
| 发现返回陈旧 | `consul catalog services`；检查 TTL | 健康检查间隔长、Agent 失联 |
| 网格流量被拒 | `consul intention list` | Intention 缺失或 deny |
| KV 不一致 | `consul kv get -recurse` | 跨 DC 同步延迟 |

## 生产部署清单

- [ ] Consul Server ≥3 且跨可用区部署（Raft 多数派）
- [ ] ACL 开启且令牌管理（bootstrap + 轮换）
- [ ] 健康检查覆盖关键服务（HTTP/TCP/gRPC 探针）
- [ ] 数据备份与恢复演练（`consul snapshot`）
- [ ] 监控接入（`consul_*` metrics：leader 状态、健康检查数量）

## 常见误区与设计要点

- **误区 1**：Agent 与 Server 混部署——Agent 应部署在每节点（含业务），Server 独立 3-5 副本。
- **误区 2**：忽略 ACL——未开启 ACL 的 Consul 任何网络可达者都可读写。
- **设计要点**：WAN 池跨数据中心互联（gossip 端口 8302）；服务发现走 DNS 接口（8600）减少代码改动；关键 KV 用 `consul-template` 自动渲染配置。

## 性能参考

- 发现 QPS：单 Server 数千 TPS 写入（受 Raft 提交限制），读可水平扩展。
- 延迟：本地读 < 1ms，强一致读 +2ms 左右。
- 规模：社区验证单 DC 数千节点；更大规模启用 `-disable-coordinates` 等调优。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Raft 集群不可用 | 恢复多数派节点，从快照恢复 |
| P1 | 服务发现延迟 > 5s | 检查 Server 负载，扩容集群 |
| P2 | 单节点健康检查异常 | 检查该节点 Agent 状态 |

## 面试要点

1. **Q：Consul 与 Kubernetes 内置服务发现的区别？**
   A：K8s 服务发现基于 DNS + Service/Endpoint，仅限集群内部；Consul 支持跨集群/跨数据中心服务发现，支持 VM 和容器混合环境。Consul 提供 KV Store、多数据中心、Connect Service Mesh 等额外功能。适合混合云/多云环境；纯 K8s 环境可使用内置 DNS。

2. **Q：Consul Connect 如何实现 Service Mesh？**
   A：Connect 通过 Sidecar Proxy（默认 Envoy）实现：① 服务注册时自动注入 Proxy；② Proxy 间建立 mTLS（基于 Consul CA）；③ Intention（意图）控制服务间访问权限；④ 支持 L7 流量管理（路由/重试/超时）。与 Istio 相比，Consul Connect 更轻量，与 Consul 服务发现深度集成。

3. **Q：Consul 的 Raft 一致性协议如何工作？**
   A：Consul Server 集群使用 Raft 保证一致性：① 选举 Leader（多数派投票）；② 写请求转发到 Leader；③ Leader 复制日志到 Follower；④ 多数派确认后提交；⑤ 读请求可由任意节点处理（可配置一致性级别）。建议 3 或 5 个 Server 节点。

## Related

- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/coredns.md|CoreDNS]]
- [[17-系统基础/06-知识字典/security/vault.md|Vault]]


<!-- risk-assessed -->
