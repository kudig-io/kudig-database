---
title: 服务账号
description: ServiceAccount 是 Kubernetes 中为 Pod 提供身份标识的资源。Pod 通过关联的 ServiceAccount
  向 API Serv...
summary: ServiceAccount 是 Kubernetes 中为 Pod 提供身份标识的资源。Pod 通过关联的 ServiceAccount 向 API
  Serv...
category: dictionary
tags:
- k8s
- glossary
- service-account
- rbac
- security
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务账号 是什么
- ServiceAccount 详解
trigger_keywords:
- 服务账号
- ServiceAccount
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 服务账号

> **英文名**: ServiceAccount

## 概述

ServiceAccount 是 Kubernetes 中为 Pod 提供身份标识的资源。Pod 通过关联的 ServiceAccount 向 API Server 认证身份，获取访问集群资源的权限。

## 核心概念/原理

### 核心概念

- **默认 ServiceAccount**：每个命名空间自动创建 `default` ServiceAccount。
- **Token 注入**：kubelet 自动将 ServiceAccount Token 挂载到 Pod 中（Projected Volume）。
- **Token 特性**：
  - 有界的（bound to Pod）。
  - 有过期时间（默认 1 小时，自动轮转）。
  - 观众限制（audience-restricted）。

### RBAC 集成

通过 RoleBinding 或 ClusterRoleBinding 将权限授予 ServiceAccount，实现 Pod 级别的权限控制。

## 关键机制或特性

- Token Request API（v1.20+）提供时间有界、受众受限的 Token。
- `automountServiceAccountToken: false` 可以禁止自动挂载 Token。
- `boundServiceAccountTokenVolume` 特性确保 Token 安全。

## 使用场景与最佳实践

- 为每个应用创建独立的 ServiceAccount，避免使用 default。
- 遵循最小权限原则，只授予必要的 RBAC 权限。
- 对不需要 API 访问的 Pod，禁用自动 Token 挂载。
- 使用 TokenRequest API 为外部服务生成短期 Token。

## 架构深度解析

### ServiceAccount Token 生命周期

```
┌──────────────────────────────────────────────────────────────┐
│  Token 签发                                                 │
│  ├─ 传统：SA controller 生成长期 Token（JWT，无过期）         │
│  │    secret/sa-token → 挂载到 Pod（legacy）                  │
│  └─ 现代（v1.20+）：TokenRequest API                         │
│       ├─ 短期 Token：默认 1h，audience 受限                  │
│       └─ boundServiceAccountTokenVolume：绑定 Pod 生命周期    │
│   │                                                           │
│   ▼ 挂载                                                      │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Pod（projected volume）                                  │  │
│  │ ├─ /var/run/secrets/kubernetes.io/serviceaccount/        │  │
│  │ │   ├─ token（JWT）                                       │  │
│  │ │   ├─ ca.crt（集群 CA，用于校验 apiserver）              │  │
│  │ │   └─ namespace（当前命名空间）                          │  │
│  │ └─ Token 轮转：过期前由 kubelet 重新签发                  │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │  ③ 容器内应用 → HTTPS 请求 apiserver                      │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ API Server 认证链                                        │  │
│  │ ├─ TokenReview：校验签名（SA 私钥）与过期时间             │  │
│  │ ├─ 身份：system:serviceaccount:<ns>:<name>               │  │
│  │ └─ 授权：RBAC 按 SA 身份匹配 RoleBinding                  │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| SA 控制器 | `pkg/controller/serviceaccount/` | 传统 Token 创建与轮转 |
| TokenRequest | `pkg/apis/authentication/v1/` | 短期 Token 签发 API |
| 认证器 | `plugin/pkg/auth/authenticator/token/serviceaccount/` | JWT 校验（签名/过期/aud） |
| 卷投影 | `pkg/volume/projected/` | bound token 卷挂载实现 |

### 流程步骤

1. Pod 创建时 kubelet 调用 TokenRequest 获取短期 Token（默认 1h，可配置 `expirationSeconds`）。
2. Token 通过 projected volume 挂载到容器内固定路径，同时挂载 CA 与命名空间文件。
3. 应用读取 Token 后对 apiserver 发起请求，认证器用 SA 签发公钥验证 JWT 签名与过期时间。
4. Token 临近过期时 kubelet 自动轮换并更新卷内容（文件更新，进程需重新读取）。
5. 授权阶段按 `system:serviceaccount:<ns>:<name>` 身份匹配 RBAC 绑定。

## 生产案例

### 案例 1：泄露的长期 Token 导致集群被完全接管

| 时间 | 事件 |
| --- | --- |
| T-60d | 旧 CI 脚本将 Pod 内 Token 复制到制品仓库（legacy 长期 Token） |
| T+0 | 仓库权限配置错误，Token 泄露到公开网络 |
| T+1d | 攻击者用 Token 枚举集群：`kubectl get secrets --all-namespaces` |
| T+2d | 攻击者创建后门 Deployment 并窃取全部 Secret |
| T+1w | 发现入侵：轮换全部 SA 密钥与集群证书，重建受影响命名空间 |

- **根因分析**：legacy Token 永不过期且无法单独吊销；应用把 SA Token 当普通凭证拷贝出 Pod 是常见泄露路径。
- **修复命令**：
```bash
# 1. 禁用 legacy Token 自动创建（v1.24+ 默认关闭）
kubectl patch sa default -p '{"automountServiceAccountToken":false}' -n app
# 2. 轮换 SA 私钥（🔴 高风险：全集群 Pod 需重建）
kubectl delete secret --all -n kube-system --field-selector type=kubernetes.io/service-account-token
kubectl delete pods -A --field-selector status.phase=Running  # 触发全部重建（慎用）
# 3. 审计绑定在 SA 上的宽权限
kubectl get rolebinding,clusterrolebinding -A | grep -iE "default|app-sa"
```

### 案例 2：bound Token 到期导致长任务批量失败

| 时间 | 事件 |
| --- | --- |
| T+0 | 数据平台将默认 SA Token 传入远端 Spark Driver 执行 6h 任务 |
| T+1h | 任务开始报 401，仅使用 1h 内有效的 Token |
| T+3h | 定位：TokenRequest 默认有效期 1h，长任务进程持有旧 Token |
| T+5h | 修复：任务初始化时启用 Token 自动轮换（client-go `TokenSource`）并重跑 |
| T+1d | 全量任务通过，无再发 |

- **根因分析**：bound Token 安全性与时效性并存，但应用若一次性读取且不轮换，超期后所有 API 调用失败；需在 SDK 层启用自动轮换。
- **修复命令**：
```bash
# 1. 确认 Token 过期行为（只读）
kubectl exec -it <pod> -- sh -c 'ls -l /var/run/secrets/kubernetes.io/serviceaccount/token'
# 2. client-go 启用自动轮换（Go 代码片段）
# rest.InClusterConfig() 默认自带 TokenSource 轮换，确认未自定义 kubeconfig
# 3. Python 等非 Go 客户端：每次请求前重新读取 token 文件
```

## 对比评测

| 维度 | Legacy Token | Bound Token（TokenRequest） | KubeConfig（用户） | OIDC |
| --- | --- | --- | --- | --- |
| 有效期 | 永久 | 1h 默认（可调） | 证书 1 年/Token 短 | 短（ID Token） |
| 可吊销性 | 差（轮换全局密钥） | 好（过期即失效） | 好（证书吊销） | 好（签发方控制） |
| 自动化 | 静态挂载 | kubelet 自动轮换 | 手动维护 | 企业 SSO 集成 |
| 适用对象 | 已废弃 | Pod 内应用 | 运维人员 | 终端用户 |

**选型建议**：Pod 内一律用 bound Token（默认开启）；禁止 legacy Token 创建（v1.24+ `--service-account-extend-token-expiration=false`）。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 401 Unauthorized | Token 过期 / 签名密钥轮换 | `kubectl exec` 查看 token 文件 mtime；确认应用重新读取 |
| 403 Forbidden | SA 无对应 RBAC 权限 | `kubectl auth can-i --list --as=system:serviceaccount:ns:sa` |
| 无法访问 apiserver | CA 文件缺失 / 网络策略 | 检查 projected volume 挂载与 Egress 规则 |
| Token 文件不存在 | automountServiceAccountToken=false | 手动指定 `serviceAccountName` + 挂载 |
| 重启后权限变化 | 绑定了已删除的 Role | 检查 RoleBinding 的 roleRef 是否存在 |

## 生产部署清单

- [ ] 关闭 legacy Token 自动创建（v1.24+ 默认行为，升级时确认）
- [ ] 每个应用独立 SA，杜绝共享 default SA
- [ ] 为无 API 需求的 Pod 设置 `automountServiceAccountToken: false`
- [ ] Token 请求设置合理 audience（如 `api://<service>`）与过期时间
- [ ] SA 权限纳入季度审计（RBAC 基线比对）

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 集群存在大量 legacy Token 且 SA 密钥泄露 | 立即轮换 SA 密钥并强制重建 Pod |
| P1 | 应用仍使用一次性读取的短 Token 跑长任务 | 客户端升级支持 Token 自动轮换 |
| P2 | 多应用共用 default SA | 拆分独立 SA 并收敛权限 |

## 面试要点

1. **Q：ServiceAccount 与普通用户认证的区别？**
   A：ServiceAccount 是集群内对象（存于 etcd），面向 Pod 内进程，身份为 `system:serviceaccount:<ns>:<name>`，凭证为 JWT；普通用户（User）由外部身份源（OIDC/x509）提供，集群内无对象。两者都经过 RBAC 授权，但 SA 可被 Pod 自动挂载凭证。
2. **Q：bound ServiceAccount Token 解决了什么问题？**
   A：legacy Token 永不过期、无法单独吊销、泄露面大。Bound Token（v1.20+）通过 TokenRequest 签发短期（默认 1h）audience 受限 JWT，绑定 Pod 生命周期，由 kubelet 自动轮换，显著缩小泄露窗口并支持按凭证粒度回收。
3. **Q：Pod 内应用访问 apiserver 的推荐方式？**
   A：推荐"in-cluster 配置 + 自动轮换"：SDK 读取投影卷中的 token/ca.crt/namespace，并启用 TokenSource 定时轮换（client-go 默认支持）；避免把 Token 复制出 Pod、避免手动拼请求。如需外部访问则用 KubeConfig 与 OIDC。

## 运维要点

- 轮换演练：定期执行 SA 密钥轮换演练，验证全部 Pod 可自动重建恢复。
- 监控：跟踪 Token 401 错误率，识别轮换失效的应用。
- 审计：审计日志过滤 `system:serviceaccount` 主体，异常高频调用触发告警。
- 升级前置：v1.24+ 升级前检查应用是否依赖 legacy Token 格式。
- 排障入口：401/403 先区分认证（Token 有效性）与授权（RBAC 绑定）两个层面。

## 参考链接

- [ServiceAccount - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/service-accounts-admin/)

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
