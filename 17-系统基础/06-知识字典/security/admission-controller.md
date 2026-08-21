---
title: 准入控制器
description: Admission Controller 是 Kubernetes API Server 中的插件机制，在对象持久化之前拦截和处理 API
  请求。它可以验证和修...
summary: Admission Controller 是 Kubernetes API Server 中的插件机制，在对象持久化之前拦截和处理 API 请求。它可以验证和修...
category: dictionary
tags:
- k8s
- glossary
- security
- admission
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 准入控制器 是什么
- Admission Controller 详解
trigger_keywords:
- 准入控制器
- Admission Controller
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 准入控制器

> **英文名**: Admission Controller

## 概述

Admission Controller 是 Kubernetes API Server 中的插件机制，在对象持久化之前拦截和处理 API 请求。它可以验证和修改请求中的对象，是实施集群策略和安全控制的关键组件。

## 核心概念/原理

### 类型

- **Validating（验证型）**：只验证请求是否合规，不修改对象。如 `ValidatingAdmissionWebhook`。
- **Mutating（变更型）**：可以修改请求中的对象。如 `MutatingAdmissionWebhook`。

### 内置准入控制器

- **LimitRanger**：检查资源是否超出 LimitRange。
- **ResourceQuota**：检查资源是否超出 ResourceQuota。
- **PodSecurity**：强制执行 Pod 安全标准（替代 PSP）。
- **NodeRestriction**：限制 kubelet 可以修改的 API 对象。
- **AlwaysPullImages**：强制每次都拉取镜像。

## 关键机制或特性

- 准入控制链：Mutating → Object Validation → Validating。
- Mutating 控制器可以修改对象，可能需要多次执行（收敛）。
- Webhook 超时或失败时，`failurePolicy` 决定是拒绝（Fail）还是允许（Ignore）。

## 使用场景与最佳实践

- 使用 `ValidatingAdmissionWebhook` 实施自定义策略（如镜像白名单）。
- 使用 OPA Gatekeeper 或 Kyverno 实现声明式策略管理。
- 配置 Webhook 的 `failurePolicy: Ignore` 避免 Webhook 故障导致集群不可用。
- 为 Webhook 配置 `namespaceSelector` 排除系统命名空间。

## 架构深度解析

### 准入控制链执行流程

```
┌──────────────────────────────────────────────────────────────┐
│  kubectl / 控制器 / kubelet 发起请求                          │
│   │                                                          │
│   ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ API Server 认证 → 授权（RBAC）                           │  │
│  │   │                                                      │  │
│  │   ▼  Admission Chain（顺序执行）                          │  │
│  │  ┌─────────────────────────────────────────────────┐    │  │
│  │  │ ① MutatingAdmissionWebhook（可修改对象）        │    │  │
│  │  │   ├─ 内置：NamespaceLifecycle/LimitRanger/      │    │  │
│  │  │   │         PodSecurity/DefaultStorageClass 等  │    │  │
│  │  │   └─ 自定义：webhook（可多次执行直至收敛）      │    │  │
│  │  ├─────────────────────────────────────────────────┤    │  │
│  │  │ ② Object Schema Validation（验证修改后的对象）  │    │  │
│  │  ├─────────────────────────────────────────────────┤    │  │
│  │  │ ③ ValidatingAdmissionWebhook（只读校验）        │    │  │
│  │  │   ├─ 内置：ResourceQuota/NodeRestriction/       │    │  │
│  │  │   │         PodSecurity/AlwaysPullImages 等     │    │  │
│  │  │   └─ 自定义：OPA/Kyverno/ratify 等              │    │  │
│  │  └─────────────────────────────────────────────────┘    │  │
│  │   │                                                      │  │
│  │   ▼ 允许/拒绝                                            │  │
│  │  etcd 持久化 → 响应返回用户                              │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 路径 | 关键职责 |
|---|---|---|
| 准入链 | plugin/pkg/admission/ | 内置准入控制器实现 |
| Webhook | staging/src/k8s.io/api/admission/ | AdmissionReview API |
| 链编排 | pkg/registry/ | 准入链顺序调度 |
| 配置 | pkg/apis/admissionregistration/ | Webhook 配置类型 |
| 调用 | staging/src/k8s.io/apiserver/pkg/admission/ | 插件框架与调用 |

### 流程步骤

1. 请求通过认证与 RBAC 授权后进入准入链。
2. Mutating 阶段：内置与自定义 Webhook 依次执行，可修改对象（如注入 sidecar、默认值）。
3. Schema 校验：按修改后的对象做 OpenAPI 结构校验。
4. Validating 阶段：内置与自定义 Webhook 只读校验（配额、策略、签名验证）。
5. 任一阶段拒绝则整体拒绝；全部通过后对象持久化到 etcd。

## 生产案例

### 案例 1：Webhook 故障引发集群写操作全拒（2023 年典型事故）

| 时间 | 事件 |
|---|---|
| T+0 | 平台升级 OPA Gatekeeper，新版本启动失败 |
| T+5min | ValidatingWebhookConfiguration 指向不可达服务，全部写请求被拒（failurePolicy=Fail） |
| T+40min | 定位为 webhook 服务 404（命名空间变更导致 endpoint 失效） |
| T+1h | 修复 endpoint 并恢复；补加 webhook 健康探针与故障演练 |

- **根因**：failurePolicy=Fail + webhook 服务不可达，准入链全线拒绝；无演练与监控。
- **修复命令**（诊断 + 恢复）：
```bash
# 🟢 检查 webhook 配置与服务可达性
kubectl get validatingwebhookconfiguration -A -o wide
kubectl get endpoints -n gatekeeper
# 🔴 紧急恢复：将 failurePolicy 临时改为 Ignore
kubectl patch validatingwebhookconfiguration gatekeeper --type merge \
  -p '{"webhooks":[{"name":"check-ignore.label.sh","failurePolicy":"Ignore"}]}'
```

### 案例 2：Mutating Webhook 死循环耗尽 API Server 资源

- **现象**：API Server 负载飙升，Pod 创建延迟达分钟级。
- **诊断**：某 MutatingWebhook 修改对象后未收敛（每次加一个标签），准入链反复调用直至超时；日志显示 AdmissionReview 风暴。
- **修复**：Webhook 收敛性验证（同一对象重复执行结果稳定）；设置 `matchPolicy`、超时与 `reinvocationPolicy`；将触发字段收敛为确定性规则。

## 对比评测

| 维度 | 内置准入控制器 | 自定义 Webhook | OPA Gatekeeper / Kyverno |
|---|---|---|---|
| 能力 | 内核级校验 | 任意逻辑 | 策略引擎（Rego/YAML） |
| 运维成本 | 零（内置） | 自运维 | 框架自运维 |
| 扩展性 | 固定 | 强 | 强（声明式） |
| 风险 | 低 | 高（可用性依赖） | 中（框架保障） |
| 适用 | 基础校验 | 定制需求 | 策略即代码 |

- **选型建议**：基础能力用内置控制器；声明式策略用 Kyverno/Gatekeeper；特殊定制（调用外部系统）才自研 Webhook，并严格遵循故障演练与降级设计。

## 故障排查速查

| 症状 | 可能原因 | 排查命令 |
|---|---|---|
| 全部写操作被拒 | Webhook 不可达 + failurePolicy=Fail | `kubectl get validatingwebhookconfiguration -o yaml` |
| 特定资源被拒 | 策略规则过严 | 查看策略日志与拒绝原因 |
| 请求变慢 | Webhook 延迟/重试 | 查看 webhook 延迟指标与调用次数 |
| 修改未生效 | Mutating 顺序/收敛问题 | 检查 webhook 顺序与 reinvocationPolicy |
| 排除失效 | namespaceSelector 错误 | 核对 selector 与命名空间标签 |

## 生产部署清单

- [ ] Webhook 高可用 + 健康探针 + failurePolicy 明确（关键路径 Fail，可降级路径 Ignore）
- [ ] 网络策略限制 Webhook 访问范围，TLS 证书自动轮换
- [ ] 故障演练：模拟 Webhook 不可达，验证降级路径与告警
- [ ] namespaceSelector 排除系统命名空间，避免自锁
- [ ] 监控准入延迟、Webhook 错误率、拒绝原因分布并告警

## 升级决策点

| 级别 | 条件 | 动作 |
|---|---|---|
| P0 | Webhook 故障导致集群写操作全拒 | 立即临时改为 Ignore 或摘除，恢复后定位根因 |
| P1 | Webhook 规则大版本变更 | 先灰度命名空间验证误拒率，再全量 |
| P2 | 准入链新增/移除控制器 | 变更评审 + 测试集群验证顺序与交互 |

## 面试要点

> 以下 Q&A 覆盖准入控制器面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Mutating 与 Validating Admission Webhook 的执行顺序与区别？**
   A：顺序固定为 Mutating → Schema 校验 → Validating：Mutating 先执行（可修改对象，如注入 sidecar、默认值），可能有多个且需要收敛（reinvocationPolicy 控制重复调用）；Validating 后执行（只读校验，如配额、策略）。顺序设计保证"先改后验"，Validating 看到的是 Mutating 修改后的最终对象。

2. **Q：failurePolicy: Fail 与 Ignore 如何取舍？**
   A：Fail：Webhook 不可用时拒绝请求，保证安全策略不被绕过，但 Webhook 故障会引发全集群写操作瘫痪；Ignore：故障时放行，保障可用性但存在策略绕过窗口。建议：安全关键策略用 Fail + 高可用与演练；非关键注入用 Ignore；生产可配置 `matchPolicy` 与超时（如 10s）限制影响面。

3. **Q：如何设计一个生产级的自定义准入 Webhook？**
   A：① 高可用（≥2 副本）+ 健康探针 + 证书自动轮换；② 明确的 failurePolicy 与超时；③ namespaceSelector 排除系统命名空间；④ 收敛性设计（Mutating 幂等）；⑤ 故障演练（模拟不可达验证降级）；⑥ 全链路监控（延迟/错误/拒绝原因）；⑦ 灰度发布（先小范围 selector）；⑧ 严格的 RBAC 与网络策略。

## 运维要点

- 可用性：Webhook 多副本跨可用区、健康探针、证书自动轮换。
- 故障预案：一键降级（failurePolicy 改 Ignore / 摘除 webhook）脚本与演练。
- 变更管理：准入链变更走评审，先测试集群验证顺序与交互。
- 监控：准入延迟分位数、Webhook 错误率、拒绝原因 TopN、调用次数。
- 审计：拒绝事件与策略命中记录对接 SIEM。

## 参考链接

- [Admission Controller - Official Documentation](https://kubernetes.io/docs/reference/access-authn-authz/admission-controllers/)

## Related

[[17-系统基础/06-知识字典/platform-engineering/admission-webhook-good-practices.md|Admission Webhook 最佳实践]]


<!-- risk-assessed -->
