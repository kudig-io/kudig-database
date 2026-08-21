---
title: Webhook
description: Webhook 是 Kubernetes 中允许外部服务介入 API 请求处理流程的回调机制。通过 Webhook，可以将认证、授权和准入控制逻辑委托给外部服务...
summary: Webhook 是 Kubernetes 中允许外部服务介入 API 请求处理流程的回调机制。通过 Webhook，可以将认证、授权和准入控制逻辑委托给外部服务...
category: dictionary
tags:
- k8s
- glossary
- security
- webhook
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Webhook 是什么
- Webhook 详解
trigger_keywords:
- Webhook
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Webhook

> **英文名**: Webhook

## 概述

Webhook 是 Kubernetes 中允许外部服务介入 API 请求处理流程的回调机制。通过 Webhook，可以将认证、授权和准入控制逻辑委托给外部服务。

## 核心概念/原理

### Webhook 类型

- **MutatingAdmissionWebhook**：在对象持久化前修改对象。
- **ValidatingAdmissionWebhook**：在对象持久化前验证对象。
- **Authentication Webhook**：自定义认证逻辑（Token Review）。
- **Authorization Webhook**：自定义授权逻辑（SubjectAccessReview）。

### 工作原理

```
API Request → API Server → Webhook (HTTPS) → External Service → Response
```

## 关键机制或特性

- Webhook 服务需要通过 TLS 加密通信。
- 支持 `caBundle` 或 `service` 引用配置 Webhook 服务。
- Webhook 的性能直接影响 API Server 的请求延迟。

## 使用场景与最佳实践

- 实现 Webhook 时确保低延迟和高可用。
- 配置合理的超时时间（默认 10 秒）。
- 使用 `namespaceSelector` 限制 Webhook 的作用范围。
- 测试 Webhook 的故障场景（超时/不可达）。

## 架构深度解析

### Webhook 认证与调用链

```
┌──────────────────────────────────────────────────────────────┐
│  API Server（apiserver）                                      │
│   │  ① 收到请求（kubectl/控制器）                             │
│   │  ② 匹配 Webhook 配置（TokenReview/AdmissionReview）       │
│   ▼  ③ HTTPS POST（带 caBundle 校验的服务证书）               │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Webhook 服务（外部 HTTPS 端点）                          │  │
│  │ ├─ 认证：TokenReview（token 校验）                      │  │
│  │ ├─ 授权：SubjectAccessReview（权限校验）                 │  │
│  │ └─ 准入：AdmissionReview（对象校验/修改）                │  │
│  │  ├─ 超时默认 10s                                         │  │
│  │  └─ failurePolicy：Fail / Ignore                         │  │
│  └─────────────────────────────────────────────────────────┘  │
│   ▲  ④ 响应（允许/拒绝/补丁）                                │
│   │                                                          │
│  ⑤ 通过则持久化 etcd / 返回用户                              │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| TokenReview | staging/src/k8s.io/api/authentication/ | 认证 webhook 协议 |
| SAR | staging/src/k8s.io/api/authorization/ | 授权 webhook 协议 |
| AdmissionReview | staging/src/k8s.io/api/admission/ | 准入 webhook 协议 |
| 配置 | pkg/apis/admissionregistration/ | Webhook 配置类型 |
| 调用器 | staging/src/k8s.io/apiserver/pkg/admission/ | webhook 调用与重试 |

### 流程步骤

1. API Server 根据配置（kubeconfig 或 service 引用）确定 Webhook 端点与证书。
2. 认证/授权阶段：构造 TokenReview/SubjectAccessReview 请求发送给 Webhook。
3. 准入阶段：构造 AdmissionReview（含对象、旧对象、用户信息）发送。
4. Webhook 返回响应：允许/拒绝（附原因），Mutating 返回 JSON Patch。
5. API Server 按 failurePolicy 处理超时/错误：Fail 拒绝请求，Ignore 放行。

## 生产案例

### 案例 1：认证 Webhook 抖动引发全集群认证风暴（2023 年事故）

| 时间 | 事件 |
|---|---|
| T+0 | 认证 Webhook 服务延迟升高，TokenReview P99 超 10s |
| T+10min | API Server 大量超时重试，kubelet/控制器认证失败连锁反应 |
| T+40min | 定位为 Webhook 后端数据库连接池耗尽（慢查询堆积） |
| T+2h | 修复慢查询 + 扩容 + 增加 Webhook 超时上限，恢复稳定 |

- **根因**：Webhook 单点性能瓶颈 + API Server 重试风暴 + 无超时上限。
- **修复命令**（诊断 + 缓解）：
```bash
# 🟢 查看 API Server 的 webhook 错误与延迟
kubectl logs -n kube-system apiserver-<node> --tail=100 | grep -i webhook
# 🟡 调整 webhook 超时与重试策略（配置 API Server 参数）
kubectl -n kube-system edit configmap kube-apiserver-config
```

### 案例 2：caBundle 配置错误导致 Webhook 全部 TLS 失败

- **现象**：Webhook 服务升级后，全部调用返回 `x509: certificate signed by unknown authority`。
- **诊断**：证书轮换后 caBundle 未同步更新；配置中的 service 引用端口错误。
- **修复**：证书轮换流程联动更新 caBundle（cert-manager 自动注入）；配置校验脚本检查 service 引用可达性。

## 对比评测

| 维度 | 认证 Webhook | 授权 Webhook | 准入 Webhook |
|---|---|---|---|
| 协议 | TokenReview | SubjectAccessReview | AdmissionReview |
| 阶段 | 认证后 | 认证后 | 写操作时 |
| 失败影响 | 全部请求无法认证 | 授权判定失败 | 对象创建/更新被拒 |
| 性能敏感 | 极高（每个请求） | 高 | 中（写操作） |
| 典型实现 | Dex/OIDC 集成 | OPA 外部授权 | Gatekeeper/Kyverno |

- **选型建议**：认证 Webhook 必须极致性能与高可用；授权 Webhook 适合复杂策略；准入 Webhook 是策略执行主战场，按 failurePolicy 分级设计。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| TLS 错误 | caBundle/证书不匹配 | `openssl s_client -connect <svc>:<port>` 验证 |
| 全部认证失败 | Webhook 不可达 + Fail | 检查 service/endpoint 与防火墙 |
| 请求超时 | Webhook 延迟/慢查询 | 查看 webhook 延迟指标与后端 DB |
| 部分失败 | 重试/限流策略 | 检查 API Server 重试日志 |
| 准入误拒 | 策略规则错误 | 查看 AdmissionReview 响应原因 |

## 生产部署清单

- [ ] Webhook 服务高可用（≥2 副本）+ 性能容量规划（认证类按全请求 QPS）
- [ ] 证书自动轮换 + caBundle 联动更新（cert-manager），配置校验脚本
- [ ] 超时/重试策略明确（认证 10s、准入 10s），failurePolicy 分级
- [ ] 故障演练：模拟 Webhook 不可达/延迟，验证降级路径
- [ ] 监控 Webhook 延迟/错误率/调用量，与 API Server 指标联动告警

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | Webhook 故障引发认证/写操作全拒 | 立即降级（Ignore 或临时绕过），恢复后定位 |
| P1 | Webhook 后端协议/API 变更 | 双版本灰度（新旧端点并存），验证后切换 |
| P2 | Webhook 配置结构调整 | 变更评审 + 测试集群验证配置生效范围 |

## 面试要点

> 以下 Q&A 覆盖 Webhook 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Kubernetes 的三种 Webhook（认证/授权/准入）在请求链路中的位置？**
   A：认证阶段（TokenReview）：校验 token/证书身份，如对接 OIDC 验证；授权阶段（SubjectAccessReview）：判定主体是否有权执行操作，如外部 RBAC 服务；准入阶段（AdmissionReview）：对象写入前的校验/修改，如策略引擎。三者串行：认证 → 授权 → 准入，故障影响依次从"全部请求"到"写操作"，性能敏感度依次降低。

2. **Q：Webhook 的超时与 failurePolicy 如何影响集群可用性？**
   A：Webhook 默认超时 10s，超时后按 failurePolicy 处理：Fail 拒绝请求（安全优先，但 Webhook 故障会瘫痪集群写操作/认证）；Ignore 放行（可用性优先，但策略可能被绕过）。认证类 Webhook 故障用 Fail 会导致全集群不可用，必须高可用 + 降级预案；准入类非关键策略建议 Ignore + 监控。

3. **Q：如何设计高可用的认证 Webhook？**
   A：① 多副本 + 水平扩展（认证类按全请求 QPS 规划容量）；② 后端存储高性能（避免慢查询）；③ 超时上限与快速失败（避免请求堆积）；④ 故障转移（多端点配置）；⑤ 缓存 token 校验结果（TTL 内复用）；⑥ 容量压测与熔断机制；⑦ 监控延迟/错误率并联动 API Server 指标告警。

## 运维要点

- 容量：认证 Webhook 按集群全请求 QPS × 2 倍余量规划副本与后端。
- 证书：自动轮换 + caBundle 联动更新，轮换演练纳入 SOP。
- 降级预案：failurePolicy 一键切换脚本 + 演练记录。
- 监控：Webhook 延迟分位数、错误率、调用量、后端依赖健康。
- 审计：认证/授权决策记录归档，对接 SIEM。

## 参考链接

- [Webhook - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/webhook/)

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
