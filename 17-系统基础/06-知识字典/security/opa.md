---
title: Open Policy Agent
description: Open Policy Agent（OPA）是 CNCF 毕业项目，提供通用的策略引擎，可在 Kubernetes 准入控制、API 网关、SSH、Terraf...
summary: Open Policy Agent（OPA）是 CNCF 毕业项目，提供通用的策略引擎，可在 Kubernetes 准入控制、API 网关、SSH、Terraf...
category: dictionary
tags:
- k8s
- glossary
- opa
- policy
- security
- cncf
tier: core
created: 2026-05
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Open Policy Agent 是什么
- OPA (Open Policy Agent) 详解
trigger_keywords:
- Open Policy Agent
- OPA (Open Policy Agent)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Open Policy Agent

> **英文名**: OPA (Open Policy Agent)

## 概述

Open Policy Agent（OPA）是 CNCF 毕业项目，提供通用的策略引擎，可在 Kubernetes 准入控制、API 网关、SSH、Terraform 等场景中执行统一的策略决策。

## 核心概念/原理

### 核心概念

- **Rego**：OPA 的策略编写语言，声明式、逻辑编程风格。
- **Policy**：定义允许/拒绝条件的规则集合。
- **Input**：请求上下文（JSON 格式）。
- **Decision**：OPA 返回的 allow/deny 结果。

```rego
package kubernetes.admission

deny[msg] {
  input.request.kind.kind == "Pod"
  not input.request.object.spec.containers[_].securityContext.runAsNonRoot
  msg := "Pod must set runAsNonRoot=true"
}
```

## 关键机制或特性

- **Gatekeeper**：OPA 的 Kubernetes 原生实现，通过 CRD 管理策略。
- **ConstraintTemplate**：参数化的策略模板。
- **Audit**：定期审计已有资源是否违反策略。
- **Mutation**：自动修正不符合策略的资源。
- **外部数据**：引用 ConfigMap 等外部数据辅助决策。

## 使用场景与最佳实践

- 使用 OPA Gatekeeper 替代 PSP 实现 Pod 安全策略。
- 定义约束：禁止 latest 标签镜像、要求 resource limits 等。
- 使用 ConstraintTemplate 构建团队可复用的策略库。
- 配合 CI/CD 在部署前进行策略检查（dry-run）。
- 启用 Audit 功能定期扫描集群中的违规资源。

## 架构深度解析

### OPA 策略引擎架构

```
┌──────────────────────────────────────────────────────────────┐
│  策略（Rego 文件 / Bundle）                                    │
│  ├─ 模块：package + rules（default allow = false）            │
│  ├─ Bundle：打包分发（OPA Server 拉取 / 本地挂载）             │
│  └─ Data：外部数据（集群资源、用户信息）                       │
│                                                                 │
│  集成模式：                                                     │
│  ├─ 1. 嵌入式库：应用内嵌 OPA（Go/Java SDK）                  │
│  ├─ 2. Sidecar：容器旁挂 OPA，本地决策（低延迟）              │
│  ├─ 3. Server：独立 OPA 服务，多客户端查询                    │
│  └─ 4. Gatekeeper：K8s 准入控制器封装（Webhook 集成）          │
│                                                                 │
│  决策流程：                                                     │
│  input（请求） + data（外部数据） + policy（Rego）              │
│   → eval（部分求值/全量求值）                                   │
│   → decision（allow/deny + 详细原因）                           │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（open-policy-agent/opa）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| 编译 | `ast/` | Rego 解析、编译与优化 |
| 查询 | `rego/` | 查询执行引擎（eval） |
| 服务 | `server/` | HTTP API（v1/data、v1/policies） |
| Bundle | `download/` | bundle 拉取与热更新 |

### 流程步骤

1. 编写 Rego 策略（`package` + 规则），编译为 AST。
2. 请求到达：input（请求对象）+ data（外部数据）合并。
3. 查询执行：按规则求值，`allow` 规则结果为 true/false/undefined。
4. 返回决策 JSON：允许/拒绝 + 消息与原因。
5. Bundle 更新触发热加载，策略变更即时生效。

## 生产案例

### 案例 1：默认拒绝策略缺失导致越权访问

| 时间 | 事件 |
| --- | --- |
| T+0 | 团队迁移 RBAC 到 OPA 前先做了 demo 策略 |
| T+1w | 生产接入后发现普通用户可访问管理接口 |
| T+2d | 定位：Rego 中未声明 `default allow = false`，未匹配规则返回 undefined 被当允许 |
| T+3d | 修复：显式默认拒绝 + 补充测试用例 |
| T+1w | 全量策略评审，杜绝"未定义即放行" |

- **根因分析**：OPA 决策中 undefined ≠ false，若应用把 undefined 当 allow 处理即形成越权；必须显式 `default allow = false` 并在应用层拒绝非 true 结果。
- **修复命令**：
```bash
# 1. 本地复现（🟢 低风险）
opa eval -d policy.rego -i input.json 'data.example.allow'
# 2. 修复策略：显式默认拒绝
# default allow = false
# allow { input.user in data.allowed_users ... }
# 3. 回归验证
opa test . -v  # 用例：未授权用户 → allow=false
```

### 案例 2：Bundle 更新失败导致策略不一致

| 时间 | 事件 |
| --- | --- |
| T+0 | 安全团队更新 Bundle（收紧策略）并推送 |
| T+30min | 部分集群仍执行旧策略，审计发现违规未被拦截 |
| T+2h | 定位：Bundle 服务认证失败（Token 过期），OPA 静默保持旧 Bundle |
| T+4h | 更新 Token、验证各集群 Bundle 版本一致 |
| T+1d | 建立 Bundle 版本监控与签名校验 |

- **根因分析**：OPA 拉取 Bundle 失败时会保留旧版本继续服务（可用性优先），若没有版本监控则策略漂移不可见。
- **修复命令**：
```bash
# 1. 查看当前 bundle 版本（只读）
curl -s localhost:8181/v1/data/system | jq '.result.bundle'
# 2. 手动触发拉取（🟡 中风险）
curl -X POST localhost:8181/v1/bundles -d '{"name":"authz"}' 
# 3. 启用 bundle 签名与失败告警
# opa run --bundle bundle.tar.gz --signing-key ...
```

## 对比评测

| 维度 | OPA（通用） | Gatekeeper | Kyverno | OpenFGA |
| --- | --- | --- | --- | --- |
| 定位 | 通用策略引擎 | K8s 准入 | K8s 原生策略 | 细粒度授权（Zanzibar） |
| 语言 | Rego | Rego | YAML | DSL |
| 集成面 | 应用/API/CI 全场景 | 仅 K8s | 仅 K8s | 应用层 |
| 复杂度 | 高 | 高 | 低 | 中 |

**选型建议**：跨系统统一策略用 OPA；仅 K8s 准入用 Gatekeeper/Kyverno；应用内细粒度权限（如文档级）用 OpenFGA。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| 决策与预期不符 | undefined 被当允许 | 检查 `default allow`；应用层拒绝非 true |
| 策略不生效 | Bundle 未更新/加载失败 | `curl /v1/policies`；检查 bundle 版本 |
| 查询超时 | 规则复杂度/数据量大 | 用部分求值、缓存 data、限制循环 |
| 数据不一致 | 外部 data 同步失败 | 检查 data 源连通性与刷新周期 |
| 误拦截 | 规则条件过宽 | 用 trace 逐步定位：`opa eval --explain=full` |

## 生产部署清单

- [ ] 全部策略显式 `default allow = false`（或等价默认拒绝）
- [ ] `opa test` 覆盖允许/拒绝双路径，CI 强制
- [ ] Bundle 分发启用签名校验与版本监控
- [ ] 决策日志（decision log）接入审计
- [ ] 性能基准：P95 决策延迟纳入监控

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 应用将 undefined 当 allow | 立即修复默认拒绝并全量审计 |
| P1 | Bundle 无签名/版本监控 | 启用签名与监控，防策略漂移 |
| P2 | 决策无日志 | 开启 decision log 对接审计 |

## 面试要点

1. **Q：OPA 中 undefined 与 false 的区别？为什么重要？**
   A：Rego 查询结果为 undefined（规则未匹配）与 false 语义不同。安全关键点：`allow` 未定义时，若应用把"非 true"都当允许就形成越权。正确做法是显式 `default allow = false` 并在应用侧只接受 `result == true`。
2. **Q：OPA 的几种集成模式如何选？**
   A：嵌入式（库）适合单一应用内策略，延迟最低；Sidecar 适合容器应用，决策在本地；Server 模式适合多客户端共享策略与集中审计；Gatekeeper 是 K8s 准入的封装。选择依据：延迟要求、策略共享度、运维复杂度。
3. **Q：如何保证 OPA 策略的热更新与一致性？**
   A：Bundle 机制：策略打包 + 版本化 + 签名，OPA 定时拉取热加载；失败时保留旧版本（可用性优先），因此必须监控 Bundle 版本一致性并签名防篡改。多集群部署时以"版本号 + 校验和"作为一致性基线。

## 运维要点

- 决策日志：开启 `decision_logs`，全量记录 input/decision 供审计与回溯。
- 性能：复杂策略用 `opa eval --partial` 部分求值；数据量大时做 data 缓存。
- 版本治理：策略仓库 + 版本化 bundle，发布走审批。
- 故障演练：Bundle 服务中断演练，验证集群行为符合预期（fail-closed 还是 fail-open）。
- 排障入口：`--explain=full` 逐步追踪规则求值路径。

## 参考链接

- [OPA Official](https://www.openpolicyagent.org/)

## Related

- [[17-系统基础/06-知识字典/security/kyverno.md|Kyverno]]
- [[17-系统基础/06-知识字典/security/admission-controller.md|Admission Controller]]
- [[17-系统基础/06-知识字典/security/pod-security-policy.md|Pod Security Policy]]
- [[17-系统基础/06-知识字典/security/rbac.md|RBAC]]
- [[17-系统基础/06-知识字典/security/webhook.md|Webhook]]


<!-- risk-assessed -->
