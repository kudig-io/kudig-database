---
title: 服务账号令牌
description: ServiceAccount Token 是 Kubernetes 为 Pod 自动颁发的认证令牌，允许 Pod 向 API Server
  证明身份。从 K8s...
summary: ServiceAccount Token 是 Kubernetes 为 Pod 自动颁发的认证令牌，允许 Pod 向 API Server 证明身份。从
  K8s...
category: dictionary
tags:
- k8s
- glossary
- security
- service-account
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务账号令牌 是什么
- ServiceAccount Token 详解
trigger_keywords:
- 服务账号令牌
- ServiceAccount Token
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 服务账号令牌

> **英文名**: ServiceAccount Token

## 概述

ServiceAccount Token 是 Kubernetes 为 Pod 自动颁发的认证令牌，允许 Pod 向 API Server 证明身份。从 K8s v1.21 起使用 TokenRequest API 颁发有界、过期的 Token。

## 核心概念/原理

### Token 特性

- **有界（Bound）**：Token 绑定到特定的 Pod 和 ServiceAccount。
- **过期（Expiring）**：默认 1 小时过期，kubelet 自动轮转。
- **受众限制（Audience-restricted）**：Token 只能用于特定的 API 受众。

### Token 注入

kubelet 通过 Projected Volume 自动将 Token 注入 Pod：

```yaml
# 自动注入（无需手动配置）
volumes:
- name: kube-api-access
  projected:
    sources:
    - serviceAccountToken:
        expirationSeconds: 3600
        path: token
```

## 关键机制或特性

- 旧版 Secret-based Token（非过期）已弃用。
- TokenRequest API 提供短期、有界的 Token。
- `automountServiceAccountToken: false` 可以禁用自动 Token 注入。

## 使用场景与最佳实践

- 不需要 API 访问的 Pod 禁用自动 Token 挂载。
- 使用 TokenRequest API 为外部服务生成短期 Token。
- 审计 ServiceAccount Token 的使用情况。

## 架构深度解析

### ServiceAccount Token 类型与签发链路

```
┌──────────────────────────────────────────────────────────────┐
│  两类 Token 对比                                              │
│  ├─ Legacy（v1.24 前默认）：                                  │
│  │   ├─ 签发：SA controller → Secret（kubernetes.io/          │
│  │   │   service-account-token）                               │
│  │   ├─ 内容：JWT（无过期、无 audience）                       │
│  │   └─ 吊销：仅能轮换全局签名密钥（影响全部 Pod）             │
│  │                                                            │
│  └─ Bound Token（v1.20+，默认）：                              │
│      ├─ 签发：TokenRequest API（apiserver 直接签发）           │
│      ├─ 内容：JWT（exp/aud/iss + 绑定 Pod 标识）               │
│      ├─ 挂载：projected volume（kubelet 自动轮换）             │
│      └─ 吊销：过期即失效，无需全局轮换                         │
│                                                                 │
│  TokenRequest 链路：                                            │
│  kubelet → POST /api/v1/namespaces/{ns}/serviceaccounts/       │
│  {sa}/token → apiserver 校验 → 签发 JWT → 挂载到 Pod            │
│                                                                 │
│  校验链路：                                                     │
│  Token → apiserver JWT 校验（签名/exp/aud/iss）                 │
│   → 身份 system:serviceaccount:<ns>:<name> → RBAC 授权          │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| TokenRequest | `pkg/registry/core/serviceaccount/` | Token 签发 API |
| 认证器 | `plugin/pkg/auth/authenticator/token/serviceaccount/` | JWT 校验 |
| 卷投影 | `pkg/volume/projected/` | bound token 挂载 |
| SA 控制器 | `pkg/controller/serviceaccount/` | legacy Token 管理（v1.24-） |

### 流程步骤

1. kubelet 在 Pod 创建时调用 TokenRequest 申请短期 Token（默认 1h）。
2. apiserver 校验请求者权限后用 SA 签名密钥签发 JWT（含 exp/aud/iss）。
3. Token 经 projected volume 挂载（token + ca.crt + namespace）。
4. 应用请求 apiserver 时携带 Token，认证器校验签名与时效。
5. Token 过期前 kubelet 自动签发新 Token 并更新卷内容。

## 生产案例

### 案例 1：Token 过期导致 DaemonSet 全部失联

| 时间 | 事件 |
| --- | --- |
| T+0 | 节点升级触发全部 Pod 重建 |
| T+30min | 日志采集 DaemonSet 全部报 401，日志中断 |
| T+2h | 定位：日志 agent 缓存了旧 Token，未随卷更新重读 |
| T+4h | 修复：agent 升级为每次请求读取 token 文件（或 SDK TokenSource） |
| T+1d | 恢复，日志链路重建 |

- **根因分析**：bound Token 默认 1h 过期，应用若缓存 Token 而不重读文件，会在轮换后持续 401；DaemonSet 类常驻进程尤其容易踩坑。
- **修复命令**：
```bash
# 1. 确认 Token 轮换情况（只读）
kubectl exec -it <pod> -- ls -l /var/run/secrets/kubernetes.io/serviceaccount/token
# 2. 验证应用是否重读（观察 401 日志）
kubectl logs <pod> --since=10m | grep -i 401
# 3. 修复：SDK 开启自动轮换（Go 示例）
# rest.InClusterConfig() 默认已启用 TokenSource 轮换
```

### 案例 2：跨集群复用 Token 引发安全事故

| 时间 | 事件 |
| --- | --- |
| T+0 | 运维将 A 集群 SA Token 复制用于 B 集群的 CI 认证 |
| T+1w | B 集群审计发现该 Token 有 B 集群 RBAC 权限（两集群共用 CA 签名密钥） |
| T+2d | 定位：Token 无 audience 限制（legacy），跨集群同样有效 |
| T+1w | 轮换：两集群分别启用独立 CA/签名密钥 + audience 限定 |

- **根因分析**：legacy Token 无 audience/绑定，跨集群共享签名密钥时 Token 可通用；bound Token 的 aud/iss 绑定能阻断此类复用。
- **修复命令**：
```bash
# 1. 审计 legacy Token（只读）
kubectl get secrets -A --field-selector type=kubernetes.io/service-account-token | head -20
# 2. 升级 bound Token（v1.24+ 默认，无需操作；确认未开 legacy 扩展）
# 3. 为关键服务签发带 audience 的 Token
kubectl create token my-sa -n app --audience=api://my-service --duration=24h
```

## 对比评测

| 维度 | Legacy Token | Bound Token | 外部 Token（OIDC） | 静态 KubeConfig |
| --- | --- | --- | --- | --- |
| 过期 | 永久 | 1h 默认 | 短 | 证书长期 |
| 吊销粒度 | 全局 | 单 Token | 签发方 | 证书 |
| 自动轮换 | 无 | kubelet | 无 | 无 |
| 安全等级 | 低 | 高 | 高 | 中 |
| 适用 | 已废弃 | Pod 内应用 | 用户/CI | 管理员 |

**选型建议**：Pod 内一律 bound Token；用户/CI 用 OIDC 短时凭证；禁止 legacy Token 创建（v1.24+ 默认关闭）。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 401 Unauthorized | Token 过期/未重读 | 检查应用是否缓存 Token；等待 kubelet 轮换 |
| Token 文件不更新 | kubelet 轮换周期/挂载异常 | `kubectl describe pod` 看 volume 事件 |
| 跨集群 Token 有效 | 共享签名密钥/无 audience | 隔离密钥 + audience 限制 |
| 创建 Token 失败 | 权限不足/audience 非法 | `kubectl auth can-i create serviceaccounts/token` |
| 应用启动即 401 | Token 请求时 SA 未就绪 | 检查 SA 存在与 RBAC |

## 生产部署清单

- [ ] v1.24+ 确认 legacy Token 自动创建已关闭
- [ ] 全部应用使用 SDK 自动轮换（TokenSource）或重读文件
- [ ] 关键服务 Token 设置 audience 与合理 duration
- [ ] 禁止跨集群复用 Token（独立签名密钥）
- [ ] 401 错误率监控与轮换失效告警

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 跨集群共享签名密钥且存在 legacy Token | 隔离密钥并批量轮换 |
| P1 | 应用缓存 Token 不重读（长任务/常驻进程） | 升级 SDK 或改重读模式 |
| P2 | 未使用 audience 限制 | 为关键服务补充 audience |

## 面试要点

1. **Q：Legacy Token 与 Bound Token 的核心区别？**
   A：Legacy 由 SA controller 签发、存于 Secret、永不过期、无 audience、吊销只能全局轮换密钥；Bound 由 TokenRequest API 签发、直接挂载投影卷、默认 1h 过期且自动轮换、绑定 Pod 身份与 audience、过期即失效。安全上 Bound 大幅缩小泄露窗口并支持按凭证回收。
2. **Q：Bound Token 自动轮换的机制是什么？**
   A：kubelet 在 Pod 生命周期内周期性检查 Token 剩余有效期，临近过期（默认 80% 时间）时调用 TokenRequest 签发新 Token 并更新投影卷文件；应用需重新读取文件（SDK TokenSource 或每次请求读取）才能感知新 Token。过期后旧 Token 立即失效。
3. **Q：为什么禁止把 SA Token 用于跨集群认证？**
   A：Token 的可信边界是"签发它的集群"：若集群间共享签名密钥，A 集群的 Token 在 B 集群同样通过验证（无 audience 限定时）。正确做法：每集群独立密钥；跨集群使用各自签发的凭证或统一 OIDC。

## 运维要点

- 监控：401 错误率与 Token 轮换成功率（kubelet 日志）联动告警。
- 审计：`create serviceaccounts/token` 操作审计，异常签发告警。
- 升级前置：v1.24+ 升级前确认应用不依赖 legacy Token。
- 演练：SA 密钥轮换演练（重建全部 Pod）季度执行。
- 排障入口：401 先确认 Token 文件 mtime（是否轮换），再确认应用读取方式。

## 参考链接

- [ServiceAccount Token - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/service-accounts-admin/#tokenrequest-api)

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
