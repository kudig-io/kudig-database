---
title: Keycloak 身份管理
description: Keycloak 是 Red Hat 赞助的开源身份和访问管理（IAM）平台，提供 SSO、OIDC、SAML、LDAP 集成等企业级身份管理能力，是
  Kube...
summary: Keycloak 是 Red Hat 赞助的开源身份和访问管理（IAM）平台，提供 SSO、OIDC、SAML、LDAP 集成等企业级身份管理能力，是
  Kube...
category: dictionary
tags:
- k8s
- glossary
- security
- identity
- sso
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Keycloak 身份管理 是什么
- Keycloak 详解
trigger_keywords:
- Keycloak 身份管理
- Keycloak
- dictionary
prerequisites:
- kubernetes
---



# Keycloak 身份管理（Keycloak）

## 概述

Keycloak 是 Red Hat 赞助的开源身份和访问管理（IAM）平台，提供 SSO、OIDC、SAML、LDAP 集成等企业级身份管理能力，是 Kubernetes 生态中最常用的外部身份提供者之一。

## 核心概念/原理

- **SSO 平台**：统一的单点登录和身份管理
- **多协议**：支持 OIDC、SAML 2.0、LDAP、Kerberos
- **用户管理**：完整的用户/组/角色管理和自助服务
- **Red Hat 支持**：Red Hat SSO 的开源上游

## 关键机制或特性

- Realm（域）隔离的多租户管理
- Identity Broker（联邦身份代理）连接外部 IdP
- 社交登录（Google/GitHub/Facebook 等）
- 用户自助服务（注册/密码重置/账户管理）
- Fine-Grained Admin Permissions
- OTP/MFA 多因素认证
- 与 Dex 互补（Keycloak 作为 Dex 后端）

## 使用场景与最佳实践

- 企业级 SSO 和身份管理平台
- Kubernetes 集群的外部 OIDC 提供者
- 多应用/多服务的统一认证授权
- 用户自助服务和生命周期管理
- 合规要求下的审计和访问控制

## 架构深度解析

### Keycloak 认证授权架构

```
┌──────────────────────────────────────────────────────────────┐
│  用户/客户端                                                   │
│   │  OIDC / SAML 2.0 / OAuth2                                 │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Keycloak（集群化部署）                                   │  │
│  │ ├─ Realm（领域）：租户/环境隔离                          │  │
│  │ │  └─ Client（客户端）：应用注册与回调配置               │  │
│  │ ├─ 认证流：密码/MFA/社交登录/OTP/条件认证                │  │
│  │ ├─ 授权：RBAC + 细粒度权限（资源服务器）                │  │
│  │ └─ 协议端点：/realms/{r}/protocol/openid-connect         │  │
│  └──────────────────────────┬──────────────────────────────┘  │
│                             │ 用户/会话数据                  │
│                             ▼                                │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 数据存储                                                │  │
│  │ ├─ 数据库：PostgreSQL（用户/领域/客户端）                │  │
│  │ └─ 缓存：Infinispan（分布式会话/令牌缓存）               │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（keycloak/keycloak）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| 服务端 | services/ | 认证/授权核心服务 |
| 协议实现 | services/src/main/java/org/keycloak/protocol/ | OIDC/SAML 端点 |
| 领域管理 | server-spi/ | Realm/Client 管理 SPI |
| 存储抽象 | storage/ | 数据库与缓存层 |
| 主题系统 | themes/ | 登录页/控制台主题 |

### 流程步骤

1. 用户在 Realm 内创建 Client，配置回调地址与协议（OIDC/SAML）。
2. 应用引导用户到 Keycloak 登录端点，完成认证（含 MFA 等条件流程）。
3. Keycloak 颁发 ID Token（身份）与 Access Token（授权），可选 Refresh Token。
4. 应用/API 按 JWKS 校验 Token 签名，或调用 Token Exchange/Introspection 做授权。
5. 会话与令牌缓存由 Infinispan 分布式缓存承载，支持水平扩容。

## 生产案例

### 案例 1：会话缓存风暴导致登录全量失败（2023 年大促事件）

| 时间 | 事件 |
|---|---|
| T+0 | 促销活动开始，登录流量激增 10 倍 |
| T+10min | Keycloak 集群 CPU 100%，登录 P99 超 30s，大量 503 |
| T+30min | Infinispan 缓存节点间网络抖动引发分布式缓存重平衡风暴 |
| T+2h | 扩容 + 调整会话缓存策略（减少复制、提升本地命中），恢复稳定 |

- **根因**：会话缓存复制模式开销过大 + 缓存节点资源不足；未做登录峰值压测。
- **修复命令**（诊断 + 扩容）：
```bash
# 🟢 查看 Keycloak 集群健康与缓存指标
kubectl -n keycloak get pods -o wide && kubectl -n keycloak top pods
# 🟡 调整 Infinispan 缓存配置（异步复制/本地缓存）后滚动重启
kubectl -n keycloak edit configmap keycloak-cache-config
```

### 案例 2：Realm 配置误删导致全应用 SSO 中断

- **现象**：运维误删测试 Realm 后，生产应用登录跳转 404。
- **诊断**：Realm 配置无版本管理与备份；误操作后无快速恢复路径。
- **修复**：Realm 导入导出纳入 GitOps（keycloak-config-cli 声明式管理）；配置每日备份 + 恢复演练，恢复时间从小时级降至分钟级。

## 对比评测

| 维度 | Keycloak | Dex | Authentik |
|---|---|---|---|
| 定位 | 全功能 IdP（含管理台） | 轻量联邦 IdP | 全功能 IdP |
| 用户管理 | 内置（含自助服务） | 委托后端（LDAP 等） | 内置 |
| 授权能力 | RBAC+细粒度 | 有限 | RBAC+策略 |
| 部署复杂度 | 高（需 DB+缓存） | 低（单二进制） | 中 |
| 适用场景 | 企业级 SSO/治理 | K8s OIDC 轻量接入 | 类似 Keycloak 替代 |

- **选型建议**：企业全功能 IdP 选 Keycloak；K8s 集群轻量 OIDC 选 Dex（Keycloak 可作其后端）；Keycloak 太重时可评估 Authentik。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 登录 503 | 缓存/DB 压力过大 | `kubectl top pods`、检查 Infinispan 日志 |
| 跳转 404 | Realm/Client 配置错误 | 核对 clientId/redirect URI 配置 |
| Token 校验失败 | 时钟偏移/密钥轮换 | 校验 ntp、JWKS 缓存刷新 |
| 会话丢失 | 缓存节点重启 | 检查缓存复制模式与持久化 |
| 登录极慢 | DB 连接池耗尽 | 检查 PostgreSQL 连接数与慢查询 |

## 生产部署清单

- [ ] 集群化部署（≥3 副本）+ PostgreSQL 高可用，跨可用区分布
- [ ] 配置声明式管理（keycloak-config-cli/GitOps），禁用控制台手工改配置
- [ ] 会话缓存参数按流量压测调优（复制模式/本地命中）
- [ ] 定期备份与恢复演练（数据库 + Realm 导出），RTO < 30min
- [ ] 监控登录 QPS、延迟、缓存命中率、DB 连接池并告警

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | 登录服务不可用/会话风暴 | 立即扩容 + 调整缓存参数止血，复盘流量模型 |
| P1 | Keycloak 大版本升级（含 DB 迁移） | 备份 + 测试环境迁移演练，低峰窗口升级并保留回滚 |
| P2 | 小版本/主题升级 | 灰度实例验证后滚动，观察登录指标 |

## 面试要点

> 以下 Q&A 覆盖 Keycloak 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Keycloak 的 Realm 与 Client 模型解决什么问题？**
   A：Realm 是隔离的身份空间（租户/环境），各自独立管理用户、客户端、密钥与策略，实现多租户隔离；Client 是接入应用的登记（回调 URI、协议、令牌配置）。这种模型让一套 Keycloak 承载多环境/多租户，但配置面也随之变大，必须声明式管理防漂移。

2. **Q：Keycloak 集群化部署的关键依赖与扩展性？**
   A：依赖分布式缓存（Infinispan）承载会话/令牌/授权缓存，PostgreSQL 存持久数据。水平扩展时缓存节点网络质量是关键（抖动会引发重平衡风暴）；读多写少场景可扩展节点数，写密集需优化 DB。上线前必须做登录峰值压测验证缓存与 DB 容量。

3. **Q：Keycloak 与 Dex 在 K8s 认证场景如何配合？**
   A：两种模式：① 直接以 Keycloak 作为 API Server 的 OIDC 提供者（功能全，含用户管理）；② Dex 作轻量 IdP，Keycloak 作为 Dex 的 OIDC Connector 后端（复用企业账号体系）。实践中后者适合"已有企业 IdP 仅需 K8s 桥接"，前者适合"平台自身需要用户/授权管理"。

## 运维要点

- 容量规划：按并发登录峰值 × 令牌刷新频率规划副本与 DB；压测数据留档。
- 缓存调优：会话缓存异步复制、热点 client 本地缓存；监控重平衡事件。
- 配置管理：全部 Realm/Client 声明式 GitOps，禁止手工变更，变更走审批。
- 备份恢复：DB + Realm 导出每日备份，季度恢复演练，RTO < 30min。
- 告警：登录失败率、延迟、缓存命中、DB 连接池、证书/密钥到期。

## 参考链接

- https://www.keycloak.org/
- https://github.com/keycloak/keycloak

## Related

- [[17-系统基础/06-知识字典/security/dex.md|Dex]]
- [[17-系统基础/06-知识字典/security/oauth2-proxy.md|oauth2-proxy]]
- [[17-系统基础/06-知识字典/security/rbac.md|RBAC]]
