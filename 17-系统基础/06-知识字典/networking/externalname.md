---
title: 外部名称
description: ExternalName 是 Service 的一种特殊类型，它将集群内部的 DNS 名称映射到外部的 DNS 名称（CNAME 记录），而不是将流量转发到
  P...
summary: ExternalName 是 Service 的一种特殊类型，它将集群内部的 DNS 名称映射到外部的 DNS 名称（CNAME 记录），而不是将流量转发到
  P...
category: dictionary
tags:
- k8s
- glossary
- networking
- service
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 外部名称 是什么
- ExternalName 详解
trigger_keywords:
- 外部名称
- ExternalName
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 外部名称

> **英文名**: ExternalName

## 概述

ExternalName 是 Service 的一种特殊类型，它将集群内部的 DNS 名称映射到外部的 DNS 名称（CNAME 记录），而不是将流量转发到 Pod。

## 核心概念/原理

### 核心概念

```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-db
spec:
  type: ExternalName
  externalName: db.example.com
```

查询 `external-db.default.svc.cluster.local` 会返回 `db.example.com` 的 CNAME 记录。

### 使用场景

- 将外部数据库映射为集群内部名称。
- 引用其他集群中的服务。
- 渐进式迁移：从外部服务迁移到集群内部服务时，只需修改 Service 类型。

## 关键机制或特性

- ExternalName 不创建 Endpoints，不进行流量转发。
- CoreDNS 直接返回 CNAME 记录。
- 不支持端口映射，客户端使用 `externalName` 的默认端口。

## 使用场景与最佳实践

- 使用 ExternalName 统一管理外部服务的访问方式。
- 迁移外部服务到集群内时只需修改 Service 类型。

## 架构深度解析

### ExternalName 解析机制

```
┌──────────────────────────────────────────────────────────────┐
│  Pod 内应用访问 my-ext.default.svc.cluster.local             │
│       │                                                       │
│       ▼                                                       │
│  CoreDNS kubernetes 插件                                      │
│  ├─ 命中 ExternalName Service（type: ExternalName）           │
│  ├─ 返回 CNAME：my-ext.default.svc.cluster.local →           │
│  │    external.example.com                                   │
│  └─ 不创建 Endpoints/EndpointSlice（无 IP 可返回）            │
│       │                                                       │
│       ▼                                                       │
│  glibc 继续解析 external.example.com（经 forward 插件）       │
│       │                                                       │
│       ▼                                                       │
│  外部 DNS → 外部服务 IP（如 203.0.113.10）                    │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| Service 策略校验 | `pkg/registry/core/service/strategy.go` | 校验 ExternalName 必须为合法 DNS 名，且不能与 ClusterIP 等字段混用 |
| EndpointSlice 控制器 | `pkg/controller/endpointslice/` | 对 ExternalName 类型跳过端点生成（无 selector） |
| CoreDNS kubernetes 插件 | coredns `plugin/kubernetes/` | 识别 `externalName` 字段并返回 CNAME 记录 |
| kube-proxy | `pkg/proxy/` | 跳过 ExternalName 服务（无需生成 NAT 规则） |

### 流程步骤

1. 创建 Service(type=ExternalName) 时，apiserver 校验 `spec.externalName` 为合法 FQDN。
2. 控制器不创建 Endpoints/EndpointSlice；kube-proxy 也不生成任何转发规则。
3. Pod 内解析服务名时，CoreDNS 返回 CNAME 指向外部域名。
4. 客户端库（glibc）继续解析 CNAME 链，最终连接外部真实 IP。
5. 外部域名变更只需修改 Service 的 externalName 字段，应用无感知。

## 生产案例

### 案例 1：数据库迁移 DNS 切换事故

| 时间 | 事件 |
|------|------|
| 22:00 | 计划将自建 MySQL 迁移到云 RDS，用 ExternalName 指向旧地址 |
| 22:30 | 修改 externalName 指向 RDS 域名 |
| 22:31 | 大量应用报连接超时，MySQL 连接池迅速打满 |
| 22:40 | 回滚 externalName 为旧地址，恢复服务 |
| 23:00 | 复盘发现 RDS 域名解析出的 IP 被安全组拦截（仅白名单了旧库 IP） |

**根因**：ExternalName 只改 DNS，未同步网络安全策略；且客户端 DNS 缓存导致新旧地址混用，连接池出现部分失败。
**修复命令**：
```bash
# 查看当前 ExternalName 指向 🟢 只读
kubectl get svc mysql-ext -o yaml | grep externalName
# 从 Pod 内验证 CNAME 链 🟢 只读
kubectl exec <pod> -- dig mysql-ext.default.svc.cluster.local CNAME +short
# 切换外部域名（先放通安全组，再改 externalName）🟡 中风险
kubectl patch svc mysql-ext -p '{"spec":{"externalName":"rds-new.example.com"}}'
```

### 案例 2：外部 API 网关故障转移

**现象**：第三方支付 API 迁移至新域名后，集群内服务仍访问旧地址（旧域名已下线）。
**诊断**：`dig <svc> CNAME` 显示指向旧域名；外部 DNS 已无记录，解析 NXDOMAIN。
**修复**：更新 ExternalName 指向新域名，并在切换前用 `kubectl exec` + `dig` 验证新域名的连通性；保留旧域名 DNS 记录 2 个 TTL 周期作为回退窗口。

## 对比评测

| 维度 | ExternalName | 普通 Service(selector) | 手动 Endpoints |
|------|-------------|----------------------|---------------|
| 流量路径 | 仅 DNS CNAME，无转发 | kube-proxy NAT 转发 | kube-proxy NAT 转发到固定 IP |
| 后端灵活性 | 只能域名（外部） | 集群内 Pod | 任意 IP（含外部） |
| 端口支持 | 无（沿用默认端口） | 完整映射 | 完整映射 |
| 适用场景 | 平滑迁移、统一访问入口 | 内部服务 | 外部 IP 直连 |

**选型建议**：仅 DNS 别名场景用 ExternalName；外部服务需要端口映射/负载均衡时用"无 selector Service + 手动 Endpoints"。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 解析到旧地址 | `dig <svc> CNAME +short`；检查客户端缓存 | 外部 DNS TTL、glibc/应用缓存 |
| NXDOMAIN | `dig <svc> CNAME`；`kubectl get svc` | externalName 域名拼写错误、外部 DNS 失效 |
| 连接超时 | `kubectl exec <pod> -- curl -v <externalName>` | 安全组/防火墙未放行、外部服务不可达 |
| 混合新旧地址 | 抓包确认源端口与目标 IP | 连接池复用 + 缓存未过期 |

## 生产部署清单

- [ ] 外部域名解析已在集群外验证（dig 通过）
- [ ] 安全组/白名单已放行新目标 IP（先放行后切换）
- [ ] 应用连接池与 DNS 缓存 TTL 已评估（避免切换期混用）
- [ ] 切换回滚方案已准备（保留旧域名记录 ≥ 2 TTL）
- [ ] 监控服务连通性（ExternalName 无健康检查，需自定义探活）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 外部服务迁移需立即切换 | 先放通目标网络，再改 externalName，观察连接池 |
| P1 | 需要端口映射能力 | 迁移到"无 selector Service + Endpoints"方案 |
| P1 | 外部域名频繁变更 | 使用外部 DNS 托管（ExternalDNS）统一管理 |
| P2 | 单一域名别名稳定运行 | 保持现状，纳入变更管理 |

## 面试要点

> 以下 Q&A 覆盖 ExternalName 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：ExternalName 类型 Service 与普通 Service 的本质区别是什么？**
   A：普通 Service 通过 selector 匹配 Pod 并生成 Endpoints，kube-proxy 做 DNAT 转发；ExternalName 不创建 Endpoints、不生成 NAT 规则，CoreDNS 直接返回 CNAME 记录指向外部域名，流量完全由客户端自己解析和连接。它本质是"DNS 层别名"，不涉及任何集群内流量转发，因此也没有负载均衡和健康检查能力。

2. **Q：ExternalName 的端口如何处理？**
   A：ExternalName 不支持端口映射，`spec.ports` 中的端口仅作为元数据（用于 SRV 记录），客户端必须使用外部服务自身的默认端口连接。需要端口转换时必须改用"无 selector Service + 手动 Endpoints"方式，由 kube-proxy 完成 DNAT 到外部 IP:端口的映射。

3. **Q：使用 ExternalName 做外部服务迁移有哪些风险？**
   A：① DNS 缓存风险：客户端与中间 DNS 的 TTL 缓存导致新旧地址混用，需提前调低外部域名 TTL 并等待生效；② 网络策略风险：目标 IP 变化后安全组/白名单可能未放行；③ 无健康检查：ExternalName 不感知外部服务故障，需自定义探活；④ 回滚困难：域名切换无版本回退，需保留旧记录。正确做法是先放通网络、调低 TTL、灰度验证后再切换。

## 参考链接

- [ExternalName - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/service/#externalname)

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|Clusterip]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|Nodeport]]
- [[17-系统基础/06-知识字典/networking/loadbalancer.md|Loadbalancer]]


<!-- risk-assessed -->
