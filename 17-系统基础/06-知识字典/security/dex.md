---
title: Dex 身份认证
description: Dex 是 CNCF 托管的 OIDC（OpenID Connect）身份认证服务，作为联邦身份提供者（IdP）连接多种后端认证源（LDAP、SAML、GitH...
summary: Dex 是 CNCF 托管的 OIDC（OpenID Connect）身份认证服务，作为联邦身份提供者（IdP）连接多种后端认证源（LDAP、SAML、GitH...
category: dictionary
tags:
- k8s
- glossary
- security
- identity
- oidc
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dex 身份认证 是什么
- Dex 详解
trigger_keywords:
- Dex 身份认证
- Dex
- dictionary
prerequisites:
- kubernetes
---



# Dex 身份认证（Dex）

## 概述

Dex 是 CNCF 托管的 OIDC（OpenID Connect）身份认证服务，作为联邦身份提供者（IdP）连接多种后端认证源（LDAP、SAML、GitHub 等），为 Kubernetes 和其他应用提供统一的身份认证层。

## 核心概念/原理

- **联邦身份**：充当 IdP 聚合层，统一 LDAP、SAML、GitHub、GitLab、Microsoft 等认证源
- **OIDC 标准**：完整实现 OpenID Connect 协议，兼容所有 OIDC 客户端
- **Kubernetes 原生**：广泛用于 K8s API Server 的 OIDC 认证配置
- **轻量部署**：单二进制，可运行在 K8s 内或独立部署

## 关键机制或特性

- 支持多种 Connector（LDAP、SAML 2.0、GitHub、GitLab、Bitbucket、Microsoft 等）
- Token 刷新（refresh token）和离线访问
- 连接器级别的组映射（group mapping）
- 自定义模板的登录页面
- 与 gangway/oauth2-proxy 配合实现 K8s 登录流程

## 使用场景与最佳实践

- Kubernetes 集群的统一身份认证网关
- 多集群场景下的联邦认证
- 企业 LDAP/AD 与 K8s RBAC 的桥接
- 开发环境的 GitHub OAuth 快速接入

## 架构深度解析

### Dex 认证流程（K8s OIDC 场景）

```
┌──────────────────────────────────────────────────────────────┐
│  用户（kubectl / 浏览器）                                      │
│   │  ① 访问 K8s API Server → 302 跳转 Dex 登录               │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Dex（OIDC IdP，Deployment）                             │  │
│  │ ├─ Connector 层：LDAP / SAML / GitHub / GitLab 等       │  │
│  │ ├─ OIDC 端点：/authorize /token /userinfo /keys         │  │
│  │ └─ 存储：SQLite（默认）/PostgreSQL/etcd（v2.30+）       │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │  ② 认证成功后返回授权码 → 换取 ID Token（JWT）           │
│   ▼                                                          │
│  kube-apiserver（--oidc-issuer-url / --oidc-client-id）      │
│  ├─ 校验 ID Token 签名（JWKS，/keys 端点）                   │
│  ├─ 映射 claims：groups → RBAC ClusterRoleBinding           │
│  └─ 建立用户会话（kubeconfig 携带 token）                    │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（dexidp/dex）

| 模块 | 路径 | 职责 |
|------|------|------|
| 服务器核心 | `server/` | OIDC 端点实现（authorize/token/userinfo/keys） |
| Connector | `connector/` | LDAP、SAML、GitHub、GitLab 等认证源适配器 |
| 存储 | `storage/` | SQLite/PostgreSQL/etcd 抽象与实现 |
| API | `api/v2/` | gRPC 管理 API（连接器/客户端/密码管理） |

### 流程步骤

1. 用户访问 K8s API（kubectl 或 Web），API Server 未认证跳转 Dex 授权端点。
2. Dex 根据客户端配置选择 Connector，将用户重定向到企业 IdP（LDAP/AD 等）。
3. 用户完成认证后回跳 Dex，Dex 签发授权码；客户端用授权码换取 ID Token。
4. API Server 通过 JWKS 校验 Token 签名、issuer、audience，提取 claims（sub、groups、email）。
5. 用户身份映射到 K8s RBAC（`groups` claim → RoleBinding/ClusterRoleBinding），完成鉴权。

## 生产案例

### 案例 1：Dex 存储锁死导致登录全部失败

| 时间 | 事件 |
|------|------|
| 10:00 | 集群用户登录开始报 500 Internal Server Error |
| 10:05 | Dex Pod 日志出现 `database is locked`（SQLite） |
| 10:10 | 检查发现 SQLite 单文件在 NFS 上，并发写入触发锁 |
| 10:20 | 迁移存储到 PostgreSQL 或本地磁盘 + 副本 |
| 10:40 | 登录恢复 |

**根因**：SQLite 默认存储不适合多副本/共享存储场景。
**修复命令**：
```bash
# 查看 Dex 日志定位存储错误 🟢 只读
kubectl logs -n dex deploy/dex | grep -i "database\|lock"
# 迁移到 PostgreSQL（修改 config 并重启）🟡 中风险
kubectl edit cm dex-config -n dex
kubectl rollout restart deploy dex -n dex
```

### 案例 2：groups claim 未映射导致用户无权限

**现象**：用户认证成功但 kubectl 报 `User ... cannot list pods`。
**诊断**：RBAC 绑定基于 `groups` claim，但 Dex 配置未声明 `group` 字段（`claimGroups`），Token 中无 groups。
**修复**：在 Dex 配置 Connector 中启用组同步（LDAP group 查询），并确认 API Server `--oidc-groups-claim=groups`；用 `kubectl get tokenreview` 或 `jq` 解码 ID Token 验证 claims。

## 对比评测

| 维度 | Dex | Keycloak | oauth2-proxy |
|------|-----|----------|--------------|
| 定位 | 联邦认证网关（轻量） | 完整 IdP（重量） | 反向代理认证 |
| 部署成本 | 低（单二进制） | 高（Java 栈） | 低 |
| Connector | LDAP/SAML/GitHub 等 | 极全 | 依赖上游 IdP |
| K8s 集成 | 原生（OIDC 文档） | 通用 OIDC | 常用 Ingress 前置 |
| 适用场景 | K8s 认证统一入口 | 企业级身份管理 | Web 应用保护 |

**选型建议**：K8s 集群认证优先 Dex（轻量、标准）；需要完整身份管理（用户自助、MFA）选 Keycloak；仅保护 Web 应用用 oauth2-proxy。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 登录 500 | `kubectl logs -n dex deploy/dex` | 存储故障、Connector 配置错误 |
| Token 无效 | 解码 ID Token；检查 JWKS | issuer/client-id 不匹配、签名密钥轮换 |
| 无权限 | `kubectl get clusterrolebinding`；检查 groups claim | RBAC 绑定缺失、组未同步 |
| 跳转死循环 | 检查 redirect URI 白名单 | 回调地址未配置或 HTTPS 强制 |

## 生产部署清单

- [ ] 存储用 PostgreSQL（多副本）或本地 SQLite + 单副本
- [ ] 证书（TLS）与签名密钥轮换流程已建立
- [ ] Connector 组同步已配置并验证 claims
- [ ] API Server OIDC 参数（issuer/client-id/groups-claim）核对
- [ ] 监控接入（Dex metrics：`dex_http_requests_total`、认证失败率）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 已知 CVE（如 token 校验漏洞） | 立即升级，先验证存储兼容性 |
| P1 | SQLite 存储瓶颈 | 迁移 PostgreSQL（需停机窗口） |
| P1 | 新增认证源（SAML/OIDC Connector） | 升级并灰度验证新连接器 |
| P2 | 稳定运行 | 跟随 CNCF 版本节奏年度升级 |

## 面试要点

> 以下 Q&A 覆盖 Dex 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Dex 在 Kubernetes 认证体系中扮演什么角色？**
   A：Dex 是 OIDC 联邦身份提供者（IdP）：它自己不存储用户密码，而是把认证委托给后端 Connector（LDAP/AD/GitHub/SAML），把多种异构认证源统一成标准的 OIDC 协议出口。K8s API Server 配置 `--oidc-issuer-url` 指向 Dex，用户在浏览器/kubectl 完成登录后获得 ID Token，API Server 通过 JWKS 校验签名，把 `groups` claim 映射到 RBAC，实现"企业账号直接访问 K8s"。

2. **Q：ID Token 的 claims 如何映射到 Kubernetes RBAC？**
   A：API Server 通过 `--oidc-groups-claim=groups` 指定从 ID Token 的 `groups` claim 提取用户组，`--oidc-username-claim` 指定用户名（默认 sub）。RBAC 侧创建 ClusterRoleBinding 绑定这些组名即可授权。常见坑：Dex Connector 未启用组同步（Token 无 groups）；组名与 RBAC 绑定不匹配；Token 过期（默认 24h）需刷新。

3. **Q：Dex 生产部署的关键注意事项有哪些？**
   A：① 存储：多副本必须用 PostgreSQL（SQLite 会锁死），etcd 存储是新版选项；② 证书与签名密钥：Dex 的 JWKS 签名密钥轮换要平滑（旧密钥保留过渡期）；③ 高可用：无状态多副本 + 共享存储或各自本地库（注意刷新令牌丢失）；④ 安全：强制 HTTPS、配置静态客户端密钥、限制 redirect URI 白名单；⑤ 监控：认证失败率、Token 签发 QPS、Connector 延迟。

## 运维要点

- 部署形态：单二进制容器，多副本 + PostgreSQL（或 etcd v2.30+）；前端可挂 Ingress 暴露 OIDC 端点。
- 配置管理：Connector 与静态客户端配置走 GitOps，变更后滚动重启并验证登录。
- 排障入口：先看 Dex 日志（认证链路），再验证 IdP 连通性（LDAP/AD 探活），最后解码 ID Token 检查 claims。
- 版本升级：先升级测试集群，验证 Connector 兼容性；签名密钥轮换保留旧密钥过渡期。
- 安全基线：强制 HTTPS、限制 redirect URI、配置静态客户端密钥、审计日志归档。
- 关键指标：认证成功率（< 99% 告警）、Token 签发延迟（P99 < 500ms）、JWKS 拉取频率。
- 容量规划：每 10K DAU 预留 2 vCPU/4Gi 内存；Token 缓存设置 TTL 降低 JWKS 拉取压力。

## 参考链接

- https://dexidp.io/
- https://github.com/dexidp/dex

## Related

- [[17-系统基础/06-知识字典/security/oauth2-proxy.md|oauth2-proxy]]
- [[17-系统基础/06-知识字典/security/opa.md|OPA]]
- [[17-系统基础/06-知识字典/security/vault.md|Vault]]
