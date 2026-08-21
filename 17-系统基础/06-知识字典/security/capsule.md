---
title: Capsule 多租户管理
description: Capsule 是 CNCF Sandbox 项目，为 Kubernetes 提供轻量级多租户管理，通过 Tenant CRD 实现命名空间级别的资源隔离和策略...
summary: Capsule 是 CNCF Sandbox 项目，为 Kubernetes 提供轻量级多租户管理，通过 Tenant CRD 实现命名空间级别的资源隔离和策略...
category: dictionary
tags:
- k8s
- glossary
- security
- multi-tenancy
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
- Capsule 多租户管理 是什么
- Capsule 详解
trigger_keywords:
- Capsule 多租户管理
- Capsule
- dictionary
prerequisites:
- kubernetes
---



# Capsule 多租户管理（Capsule）

## 概述

Capsule 是 CNCF Sandbox 项目，为 Kubernetes 提供轻量级多租户管理，通过 Tenant CRD 实现命名空间级别的资源隔离和策略管理，无需引入额外的控制面组件。

## 核心概念/原理

- **轻量多租户**：通过 CRD 和 Admission Webhook 实现，无需额外控制面
- **命名空间隔离**：每个租户拥有独立的命名空间集合
- **策略继承**：租户级策略自动应用到其所有命名空间
- **CNCF Sandbox**：Clastix 主导开发

## 关键机制或特性

- Tenant CRD 定义租户及其命名空间
- NetworkPolicy 自动注入（租户间隔离）
- ResourceQuota / LimitRange 按租户管理
- 存储类限制（每租户可用 StorageClass）
- Ingress 类限制（每租户可用 IngressClass）
- 节点选择器限制（NodeSelector 按租户隔离）

## 使用场景与最佳实践

- 企业内部的 K8s 多租户管理
- 开发团队的资源隔离
- SaaS 平台的租户管理
- 共享集群的安全隔离
- 替代 vCluster / OCM 的轻量方案

## 架构深度解析

### Capsule 多租户模型

```
┌──────────────────────────────────────────────────────────────┐
│  平台管理员（Cluster Admin）                                   │
│   │  创建 Tenant（租户资源）                                   │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Capsule Operator（Deployment）                          │  │
│  │ ├─ 管理 Tenant CRD：Owner（租户管理员）                 │  │
│  │ ├─ 资源配额：ResourceQuota 模板自动下发                 │  │
│  │ ├─ 网络策略：NetworkPolicy 模板（可选）                 │  │
│  │ ├─ 限制范围：LimitRange 模板                           │  │
│  │ └─ 策略引擎：与 OPA/Kyverno 集成                       │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │  ① 租户管理员创建 Namespace（带 capsule.clastix.io/tenant 标签）│
│   ▼                                                          │
│  Capsule Proxy（可选，K8s API 网关）                          │
│  ├─ 租户管理员经 proxy 访问受限 API 视图                     │
│  └─ 隔离命名空间列表与资源查看权限                           │
│   │                                                          │
│   ▼                                                          │
│  租户命名空间集合（共享配额池，物理隔离于其他租户）            │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（clastix/capsule）

| 模块 | 路径 | 职责 |
|------|------|------|
| Tenant 控制器 | `controllers/tenant/` | 租户创建/更新时下发配额、策略模板 |
| 配额协调 | `controllers/resourcequota/` | 将租户配额聚合到命名空间（资源池） |
| Webhook | `webhooks/` | 校验命名空间归属、租户资源操作权限 |
| Proxy | `proxy/` | 租户 API 视图代理（namespace 过滤） |

### 流程步骤

1. 平台管理员创建 Tenant，指定 Owner（租户管理员）、配额模板、网络策略模板。
2. 租户管理员创建命名空间并打 `capsule.clastix.io/tenant=<name>` 标签。
3. Capsule Webhook 校验命名空间归属；配额控制器将租户级配额聚合下发到各命名空间。
4. 租户管理员通过 Capsule Proxy（或原生 API）管理自己的命名空间，权限受限。
5. 平台侧持续监控租户配额使用率与策略合规性。

## 生产案例

### 案例 1：租户配额超额导致命名空间创建失败

| 时间 | 事件 |
|------|------|
| 10:00 | 租户 A 新增命名空间失败，报 quota 不足 |
| 10:05 | `kubectl get resourcequota -A` 显示租户池使用率 100% |
| 10:10 | 定位到租户内某团队滥用（一次性创建 100 个命名空间） |
| 10:20 | 调整租户配额并追加治理规则（命名空间数量限制） |
| 10:40 | 恢复创建 |

**根因**：租户配额池共享，内部团队未按子配额约束。
**修复命令**：
```bash
# 查看租户与配额状态 🟢 只读
kubectl get tenant
kubectl describe tenant <name>
# 查看命名空间级配额使用 🟢 只读
kubectl get resourcequota -n <ns> -o yaml
# 调整租户配额 🟡 中风险
kubectl edit tenant <name>
```

### 案例 2：租户管理员越权访问其他租户资源

**现象**：租户 A 管理员能列出集群中所有命名空间（含租户 B）。
**诊断**：直接使用原生 API 时，ClusterRole 绑定过宽（`cluster-admin` 或 `list namespaces` 权限）。
**修复**：改用 Capsule Proxy 作为租户入口（自动过滤命名空间）；或收紧 RBAC 为命名空间级 Role；审计 `kubectl auth can-i --list` 验证权限边界。

## 对比评测

| 维度 | Capsule | vCluster | OCM（ManagedCluster） |
|------|---------|----------|----------------------|
| 隔离级别 | 命名空间级（配额/策略） | 虚拟集群（完整 API） | 物理集群 |
| 资源开销 | 极低（Operator） | 中（API Server 模拟） | 高 |
| 租户自治 | 中（受限 API） | 高（完整 API） | 高 |
| 运维复杂度 | 低 | 中 | 高 |
| 适用场景 | 共享集群租户隔离 | 强隔离轻量需求 | 多集群管理 |

**选型建议**：共享集群多团队隔离首选 Capsule；需要完整 API 自治选 vCluster；跨集群租户管理用 OCM。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 命名空间创建失败 | `kubectl get tenant`；Webhook 日志 | 配额不足、归属校验失败 |
| 配额不生效 | `kubectl get resourcequota -A` | 模板未下发、控制器异常 |
| 租户越权 | `kubectl auth can-i --list -n <ns>` | RBAC 绑定过宽、未用 Proxy |
| Proxy 404 | 检查 Proxy 路由与证书 | Capsule Proxy 未配置、RBAC 缺失 |

## 生产部署清单

- [ ] Tenant 命名规范与 Owner 职责明确（含审批流程）
- [ ] 配额模板覆盖 CPU/内存/PVC/命名空间数量
- [ ] 网络策略模板（租户默认隔离）验证
- [ ] Capsule Proxy 部署并验证租户入口权限
- [ ] 监控接入（租户配额使用率、Webhook 拒绝率）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Webhook 故障导致命名空间操作失败 | 立即回滚或修复 Webhook 配置 |
| P1 | 需要更细粒度租户策略（OPA 集成） | 升级并配置策略引擎 |
| P1 | 租户数快速增长 | 评估 Proxy 扩容与配额池拆分 |
| P2 | 稳定运行 | 跟随社区版本年度升级 |

## 面试要点

> 以下 Q&A 覆盖 Capsule 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Capsule 的多租户模型与原生 K8s RBAC 有何区别？**
   A：原生 RBAC 是"权限"维度：通过 Role/ClusterRole 控制谁能做什么，但缺少"资源配额池"（多个命名空间共享配额）与"租户自治"（租户管理员管理自己的资源）。Capsule 引入 Tenant 抽象：一个租户包含多个命名空间，配额聚合下发（ResourceQuota 池）、网络策略模板化、租户管理员经 Proxy 获得受限 API 视图，是"配额 + 策略 + 自治"三层能力的组合。

2. **Q：Capsule 如何实现租户配额共享？**
   A：租户在 Tenant 定义 `spec.resourceQuota`（如 CPU 总量），Capsule 控制器在租户命名空间创建聚合型 ResourceQuota，各命名空间再挂 `spec.scopeSelector` 关联；Pod 创建时配额按租户池统一核算（命名空间配额 + 租户池配额双重校验）。调整配额只需改 Tenant，自动下发，无需逐个命名空间操作。

3. **Q：Capsule 与 vCluster 的隔离强度差异？**
   A：Capsule 是"命名空间级软隔离"：共享同一物理集群 API Server，隔离靠配额、RBAC、网络策略与 Proxy 视图过滤，资源开销极低但强隔离（如 etcd 数据）无保障；vCluster 为每个租户运行虚拟 API Server（数据存租户命名空间），提供完整 API 自治与更强隔离，但资源开销与运维复杂度高。选择依据：租户信任度与自治需求。

## 运维要点

- 部署形态：Operator（Deployment）+ Webhook（Mutating/Validating）+ 可选 Proxy。
- 租户治理：Tenant 变更走 GitOps；配额调整需评估现有使用率（避免挤占）。
- 排障入口：先看 Operator 日志（reconcile 错误），再查 Webhook 配置（MutatingWebhookConfiguration）。
- 升级顺序：先升级测试集群验证 Webhook 兼容性；生产升级保留回滚版本。

## 参考链接

- https://capsule.clastix.io/
- https://github.com/clastix/capsule

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|RBAC]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]
- [[17-系统基础/06-知识字典/security/opa.md|OPA]]
