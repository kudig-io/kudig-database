---
title: Gatekeeper
description: Gatekeeper 是 OPA（Open Policy Agent）的 Kubernetes 原生实现，通过 CRD 在集群中执行准入策略和审计。它将
  Reg...
summary: Gatekeeper 是 OPA（Open Policy Agent）的 Kubernetes 原生实现，通过 CRD 在集群中执行准入策略和审计。它将
  Reg...
category: dictionary
tags:
- k8s
- glossary
- gatekeeper
- opa
- policy
- security
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Gatekeeper 是什么
- OPA Gatekeeper 详解
trigger_keywords:
- Gatekeeper
- OPA Gatekeeper
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Gatekeeper

> **英文名**: OPA Gatekeeper

## 概述

Gatekeeper 是 OPA（Open Policy Agent）的 Kubernetes 原生实现，通过 CRD 在集群中执行准入策略和审计。它将 Rego 策略封装为 ConstraintTemplate，让非 OPA 专家也能定义和执行策略。

## 核心概念/原理

### 核心资源

| 资源 | 功能 |
|------|------|
| ConstraintTemplate | 参数化的 Rego 策略模板 |
| Constraint | ConstraintTemplate 的实例化（指定参数和目标） |
| Config | 同步 K8s 资源到 OPA 缓存 |

### 执行模式

- **Deny**：拒绝不符合策略的请求（准入控制）。
- **Warn**：允许但生成警告。
- **Dryrun**：仅审计，不阻止。
- **Audit**：定期扫描已有资源的合规性。

## 关键机制或特性

- **Admission Webhook**：拦截 API 请求进行策略检查。
- **Mutation**：自动修改不合规资源（alpha）。
- **External Data**：引用外部数据源辅助策略决策。
- **Library**：社区贡献的 ConstraintTemplate 库。
- 与 CI/CD 集成进行部署前策略检查（gator CLI）。

## 使用场景与最佳实践

- 使用 Gatekeeper 替代 PSP 实施 Pod 安全策略。
- 定义约束：禁止 latest 标签、要求 resource limits、限制特权容器。
- 启用 Audit 定期扫描集群中的违规资源。
- 使用 gator CLI 在 CI 流水线中测试策略合规性。
- 考虑 Kyverno 作为更简单的替代方案（YAML 策略）。

## 架构深度解析

### Gatekeeper 准入控制架构

```
┌──────────────────────────────────────────────────────────────┐
│  AdmissionReview（创建/更新请求）                              │
│   │  ① API Server 转发（Mutating/Validating webhook）         │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Gatekeeper Controller（3 组件）                          │  │
│  │ ├─ ConstraintTemplate：定义规则模板（Rego）              │  │
│  │ ├─ Constraint：实例化模板 + 参数（k8srequiredlabels）    │  │
│  │ ├─ Config：同步资源数据（cache）                        │  │
│  │ ├─ ValidatingWebhook：校验请求对象                      │  │
│  │ ├─ MutatingWebhook：修改请求对象（assign/mutator）      │  │
│  │ └─ Audit 定时评估：存量资源合规报告                     │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │  ② 决策：AdmissionResponse.allowed + message             │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ API Server：allowed=true 放行 / false 拒绝 + 原因        │  │
│  │ 审计模式：violation 记录到 Constraint status             │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（open-policy-agent/gatekeeper）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| Webhook | `pkg/webhook/` | AdmissionReview 处理 |
| Rego 执行 | `pkg/controller/constraint/` | 模板实例化与评估 |
| 审计 | `pkg/audit/` | 存量资源合规扫描 |
| 同步缓存 | `pkg/controller/config/` | 资源数据同步 |

### 流程步骤

1. 创建 ConstraintTemplate（Rego 规则）与 Constraint（参数化实例）。
2. 请求进入 ValidatingWebhook，Gatekeeper 将对象序列化为 `input`。
3. 加载匹配的 Constraint，执行 Rego 查询并收集违规信息。
4. 返回 allowed 与 message；`enforcementAction: deny` 拒绝，`dryrun` 仅记录。
5. 审计控制器定期评估存量资源，违规写入 Constraint 的 status。

## 生产案例

### 案例 1：Rego 模板语法错误导致准入全部放行

| 时间 | 事件 |
| --- | --- |
| T+0 | 团队新增"强制资源限额"模板，语法校验通过 |
| T+1d | 审计发现违规资源未被拦截，策略形同虚设 |
| T+2d | 定位：Rego 中 `violation[{"msg": ...}]` 规则写法错误（包名不匹配），查询恒为空 |
| T+3d | 修复模板并补充测试用例（opa test），重新部署 |
| T+1w | 新增策略全部要求通过 `opa test` 才能上线 |

- **根因分析**：Rego 的声明式查询"无匹配即无违规"，模板写错时表现为"静默放行"，比误拦截更危险；缺少策略测试是根本原因。
- **修复命令**：
```bash
# 1. 查看模板状态（只读）
kubectl get constrainttemplates -A
kubectl get k8srequiredresources -A -o yaml | grep -A10 status
# 2. 本地测试 Rego（🟢 低风险）
opa test ./policies/ -v
# 3. 重新部署修复后的模板（🟡 中风险）
kubectl apply -f constraint-template.yaml && kubectl apply -f constraint.yaml
```

### 案例 2：Gatekeeper 故障导致全集群写操作失败

| 时间 | 事件 |
| --- | --- |
| T+0 | 网络抖动，Gatekeeper Pod 重启 |
| T+5min | 全部创建/更新请求超时失败（failurePolicy 默认 Fail） |
| T+30min | 定位：ValidatingWebhook 在 Pod 不可达时拒绝请求 |
| T+1h | 恢复：Pod 拉起来后自动恢复；后续将 failurePolicy 改为 Ignore 并配置健康检查 |
| T+2d | 建立 webhook 可用性监控与 fail-open 灰度策略 |

- **根因分析**：准入 Webhook 的 failurePolicy 决定故障行为：Fail（默认）会在 webhook 不可用时拒绝全部请求，形成"策略系统故障=集群瘫痪"。
- **修复命令**：
```bash
# 1. 查看 webhook 配置（只读）
kubectl get validatingwebhookconfiguration gatekeeper-validating-webhook-configuration -o yaml | grep -A5 failurePolicy
# 2. 改为 Ignore 兜底（🟡 中风险：故障期间策略不强制）
kubectl patch validatingwebhookconfiguration gatekeeper-validating-webhook-configuration \
  --type='json' -p='[{"op":"replace","path":"/webhooks/0/failurePolicy","value":"Ignore"}]'
# 3. 监控 webhook 健康（Prometheus）
# gatekeeper_webhook_request_duration_seconds / gatekeeper_constraint_violations
```

## 对比评测

| 维度 | Gatekeeper | Kyverno | PSA（内置） |
| --- | --- | --- | --- |
| 策略语言 | Rego（学习曲线高） | YAML（低门槛） | 标签（零门槛） |
| Mutation | 支持（mutators） | 支持（内置） | 不支持 |
| 背景检查 | 支持（data.inventory） | 部分 | 不支持 |
| 生态 | CNCF、OPA 体系 | 活跃 | 官方 |
| 适用场景 | 复杂组织策略 | 快速落地 | 基线 |

**选型建议**：已有 OPA 体系/需要复杂 Rego 逻辑选 Gatekeeper；快速落地与 Mutation 场景选 Kyverno；基线一律 PSA。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 策略不拦截 | Rego 无匹配/模板未同步 | `kubectl get constraint -o yaml` 看 status；`opa eval` 本地验证 |
| 全部请求失败 | webhook 不可达 + failurePolicy=Fail | 检查 gatekeeper-controller-manager Pod；临时改 Ignore |
| 违规不显示 | audit 周期未到（默认 60s+） | 等待或 `kubectl get k8srequiredlabels -o yaml` 查 status |
| 误拦截 | 模板参数过宽 | `kubectl describe constraint` 看 message 与匹配范围 |
| 性能下降 | 大量 Rego 查询 | 配置 Config 资源同步、限制同步对象 |

## 生产部署清单

- [ ] 全部策略配套 `opa test` 单测，CI 强制
- [ ] 新策略先 `dryrun` 观察 1-2 周再 `deny`
- [ ] failurePolicy 与健康检查预案文档化
- [ ] webhook 可用性与违规指标接入监控告警
- [ ] 策略即代码：模板入库、评审、版本化

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | webhook 故障导致集群写失败 | 立即修复控制器，评估 failurePolicy 切换 |
| P1 | 关键策略无单测（静默放行风险） | 补齐 opa test 并纳入 CI |
| P2 | 策略全部 deny 无灰度 | 建立 dryrun → deny 的发布流程 |

## 面试要点

1. **Q：Gatekeeper 如何工作？ConstraintTemplate 与 Constraint 的关系？**
   A：ConstraintTemplate 定义 Rego 规则模板（类），Constraint 是模板的实例化（含参数，如强制哪些标签）。请求进来时，Gatekeeper 把对象作为 `input` 执行匹配 Constraint 的 Rego 查询；有违规则按 enforcementAction 拒绝或记录。Template 可复用、Constraint 可多实例，实现"一次定义、多场景参数化"。
2. **Q：Rego 策略为什么容易出现"静默放行"？**
   A：Rego 是声明式查询：规则写成"找出违规对象"，若逻辑错误（包名不符、变量未绑定、条件恒假）则查询结果为空 = 无违规 = 放行，且不报错。对策：`opa test` 单测覆盖"应拦截/应放行"两种样例；生产先 dryrun 观察再 deny。
3. **Q：准入 Webhook 故障时集群会怎样？如何防护？**
   A：默认 failurePolicy=Fail，webhook 不可达时所有写操作被拒（安全优先但可用性受损）；Ignore 则故障时策略失效。防护：多副本 + PDB、健康检查、监控 webhook 延迟与错误率、关键策略双引擎冗余（如 Gatekeeper + Kyverno 分层）。

## 运维要点

- 策略评审：Rego 模板变更走代码评审 + opa test 门禁。
- 灰度节奏：dryrun → warn → deny 三阶段发布。
- 监控：`gatekeeper_constraint_violations` 与 webhook 延迟告警。
- 升级：版本升级关注 CRD schema 变更，先升级 CRD 再升级控制器。
- 排障入口：先确认 Constraint status（同步/违规），再本地 `opa eval` 复现。

## 参考链接

- [Gatekeeper Official](https://open-policy-agent.github.io/gatekeeper/)

## Related

- [[17-系统基础/06-知识字典/security/opa.md|OPA]]
- [[17-系统基础/06-知识字典/security/kyverno.md|Kyverno]]
- [[17-系统基础/06-知识字典/security/admission-controller.md|Admission Controller]]
- [[17-系统基础/06-知识字典/security/pod-security-policy.md|Pod Security Policy]]
- [[17-系统基础/06-知识字典/security/webhook.md|Webhook]]


<!-- risk-assessed -->
