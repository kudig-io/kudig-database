---
title: Pod 安全策略
description: Pod Security Policy（PSP）是 Kubernetes 早期用于控制 Pod 安全配置的集群级资源。**PSP 在 K8s
  v1.21 中被弃...
summary: Pod Security Policy（PSP）是 Kubernetes 早期用于控制 Pod 安全配置的集群级资源。**PSP 在 K8s v1.21
  中被弃...
category: dictionary
tags:
- k8s
- glossary
- security
- psp
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod 安全策略 是什么
- Pod Security Policy (PSP) 详解
trigger_keywords:
- Pod 安全策略
- Pod Security Policy (PSP)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod 安全策略

> **英文名**: Pod Security Policy (PSP)

## 概述

Pod Security Policy（PSP）是 Kubernetes 早期用于控制 Pod 安全配置的集群级资源。**PSP 在 K8s v1.21 中被弃用，v1.25 中被移除**，已被 Pod Security Standards（PSS）+ Pod Security Admission 替代。

## 核心概念/原理

### PSP 的历史

- **K8s v1.3-v1.20**：PSP 是控制 Pod 安全的主要机制。
- **K8s v1.21**：PSP 被标记为弃用（deprecated）。
- **K8s v1.25**：PSP 被完全移除。

### PSP 的功能（已弃用）

PSP 可以控制：
- 特权容器（privileged）
- 宿主机命名空间（hostNetwork, hostPID, hostIPC）
- 宿主机端口范围
- 卷类型
- 文件系统组
- 用户/组范围
- 允许的能力（capabilities）
- SELinux 上下文

## 关键机制或特性

- PSP 是集群级资源，通过 RBAC 控制谁可以使用哪些 PSP。
- PSP 的复杂性导致难以正确配置，是弃用的主要原因之一。
- 替代方案 PSS（Pod Security Standards）通过命名空间标签实施，更简洁。

## 使用场景与最佳实践

- 如果集群仍在使用 PSP（K8s < v1.25），应计划迁移到 PSS。
- 迁移步骤：1) 审计现有 PSP 规则 → 2) 映射到 PSS 级别 → 3) 在命名空间上应用 PSS 标签 → 4) 验证 → 5) 删除 PSP。
- 新集群直接使用 Pod Security Admission。

## 架构深度解析

### PSP 校验字段全景与决策流

```
┌──────────────────────────────────────────────────────────────┐
│  Pod 请求 → PSP 准入（admission controller）                   │
│   │                                                           │
│   ├─① 策略选择：RBAC use 权限 + 优先级（评分）                 │
│   │                                                           │
│   ├─② 逐字段校验（任一违反即拒绝）：                           │
│   │   ├─ privileged / allowPrivilegeEscalation                │
│   │   ├─ hostNetwork / hostPID / hostIPC / hostPorts          │
│   │   ├─ volumes（白名单：configMap/secret/pvc/...）           │
│   │   ├─ allowedCapabilities / requiredDropCapabilities       │
│   │   ├─ runAsUser / runAsGroup / supplementalGroups          │
│   │   ├─ fsGroup / seLinuxOptions / seccompProfile            │
│   │   └─ allowedProcMountTypes / allowedHostPaths              │
│   │                                                           │
│   └─③ 通过 → 写入 Pod 对象 → 后续准入（LimitRange/Quota）      │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| 准入主流程 | `plugin/pkg/admission/security/podsecuritypolicy/admission.go` | PSP 选择与校验 |
| 策略策略 | `pkg/security/podsecuritypolicy/` | 各字段校验器实现 |
| 默认值 | `pkg/security/podsecuritypolicy/util/` | 校验失败时补充默认字段 |
| API 定义 | `staging/src/k8s.io/api/policy/v1beta1/` | PSP 资源 schema（已移除） |

### 流程步骤

1. Pod 创建时 PSP 准入控制器基于请求者身份选择可用的 PSP 集合。
2. 通过"互斥字段满足度"评分机制在候选 PSP 中选出最优策略。
3. 对 securityContext 与卷配置执行 20+ 字段校验，任一违反即拒绝。
4. 校验通过后可对缺失字段注入默认值（如默认 fsGroup、SELinux 类型）。
5. v1.21 起弃用、v1.25 移除 API，存量集群必须提前迁移。

## 生产案例

### 案例 1：PSP SELinux 误配导致核心业务批量 CrashLoopBackOff

| 时间 | 事件 |
| --- | --- |
| T+0 | 安全团队收紧 PSP：强制 `seLinuxOptions.type=container_t` |
| T+15min | 订单服务 80% Pod 进入 CrashLoopBackOff |
| T+45min | 定位：应用依赖宿主机目录（hostPath），SELinux 类型不匹配被拒绝读写 |
| T+2h | 为受影响工作负载单独签发宽松 PSP（保留 hostPath 白名单）并回滚默认策略 |
| T+4h | 完成策略分级：默认 restricted、白名单工作负载 baseline |

- **根因分析**：PSP 全局收紧未做灰度，且未考虑 hostPath 工作负载的 SELinux 上下文差异；单一策略试图覆盖全部工作负载。
- **修复命令**：
```bash
# 1. 查看 PSP 与 SA 绑定关系（只读）
kubectl get psp -o yaml | grep -E "name:|seLinux"
# 2. 为工作负载单独绑定宽松策略（🟡 中风险）
kubectl create clusterrole psp:baseline --verb=use --resource=podsecuritypolicies --resource-name=psp-baseline
kubectl create clusterrolebinding psp:baseline:sa --clusterrole=psp:baseline --serviceaccount=prod:order-sa
# 3. 验证恢复
kubectl rollout status deployment/order -n prod  # 🟢 只读
```

### 案例 2：PSP 阻断 CI 流水线导致发布停滞

| 时间 | 事件 |
| --- | --- |
| T+0 | CI 流水线升级 kubectl 版本后，测试环境 Pod 全部创建失败 |
| T+1h | 错误信息：`pods "xxx" is forbidden: unable to validate against any pod security policy` |
| T+3h | 定位：新版本 kubectl 发送的请求中携带更多 securityContext 字段，PSP 白名单不匹配 |
| T+6h | 运维将 CI SA 绑定到宽泛 PSP（privileged），流水线恢复 |
| T+1w | 迁移至 Pod Security Admission `baseline` 并逐步收紧 |

- **根因分析**：PSP 匹配是"整体最优匹配"，新增字段（如 seccompProfile）可能使既有策略失效；CI 这类高危主体被授予 privileged 是错误修复。
- **修复命令**：
```bash
# 1. 查看拒绝原因（只读）
kubectl describe pod <failed-pod> -n ci | grep -A3 Events
# 2. 用 PSA 替代：给 CI 命名空间设 baseline（🟡 中风险）
kubectl label ns ci pod-security.kubernetes.io/enforce=baseline --overwrite
# 3. 为 CI 补充分级策略（如允许特定 capabilities）
kubectl get podsecuritypolicy psp-baseline -o yaml > psp.yaml && kubectl apply -f psp.yaml
```

## 对比评测

| 维度 | PSP（字段白名单） | PSS（三级标准） | Kyverno（策略即代码） |
| --- | --- | --- | --- |
| 配置复杂度 | 高（20+ 字段 + RBAC） | 低（3 级标签） | 中（YAML 策略） |
| 可预测性 | 评分机制难预测 | 确定性匹配 | 规则直白 |
| 维护成本 | 高（已废弃） | 低（内置） | 中（第三方升级） |
| 生态 | 移除 | 官方标准 | 活跃社区 |

**选型建议**：历史 PSP 一律迁移；默认 PSS `baseline`，敏感命名空间 `restricted`，特殊工作负载（特权、hostPath）用 Kyverno 白名单例外。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| unable to validate against any policy | 无可用 PSP 或全部不匹配 | 检查 SA 绑定的 PSP 与 `use` 权限 |
| Pod 以错误 UID 运行 | runAsUser 策略未命中 | 查看 PSP `runAsUser.rule` 与 Pod spec |
| hostPath 挂载失败 | allowedHostPaths 白名单 | 补充 pathPrefix 或改用 PVC |
| 升级 v1.25 失败 | PSP API 残留 | 升级前 `kubectl delete psp --all` |

## 生产部署清单

- [ ] 盘点全部工作负载实际所需权限（privileged/hostPath/capabilities）
- [ ] 按 workload 分级设计 2-3 档 PSP（restricted/baseline/special）
- [ ] 迁移到 PSS 标签并灰度 `audit` → `warn` → `enforce`
- [ ] 删除 PSP 对象与 RBAC 绑定，纳入升级前检查
- [ ] 为特权工作负载建立豁免台账并季度复核

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | v1.25+ 集群仍有 PSP 对象 | 立即清理并切换 PSA，否则 apiserver 无法启动 |
| P1 | CI/特权 SA 绑定 privileged PSP | 迁移到分级策略并审计必要性 |
| P2 | PSP 已清但特权工作负载无台账 | 建立豁免清单与复核周期 |

## 面试要点

1. **Q：PSP 的"互斥字段"评分机制是什么？**
   A：当多个 PSP 都可用时，控制器对候选策略中"互斥字段"（如 privileged、hostNetwork、runAsUser 等）统计满足数，满足最多者为最优；若两个 PSP 都不完全满足则取满足数多者，仍可能因字段冲突拒绝。该机制导致策略行为难以预测，是弃用原因之一。
2. **Q：为什么 PSP 在 v1.25 被移除而不是修复？**
   A：PSP 将策略选择与 RBAC 绑定耦合，20+ 字段的评分模型复杂且易错，且与容器安全最佳实践（分层、可预测）冲突。社区最终选择内置 PSS 三级标准（标签驱动、确定性）并支持第三方引擎扩展，从根本简化模型。
3. **Q：迁移 PSP 时如何避免业务中断？**
   A：三步：先 audit 模式观察违规工作负载（只记录不拦截）→ 为违规项修复或建立豁免 → warn 阶段收集反馈 → enforce 强制执行。同时将特权类工作负载单独纳入白名单策略，避免"一刀切"。

## 运维要点

- 迁移节奏：PSP → PSA 迁移建议按命名空间灰度，每批 1-2 周观察期。
- 台账管理：特权工作负载豁免清单必须与安全团队双签。
- 巡检：`kubectl get psp` 返回 404 即为已移除版本，防止旧脚本报错。
- 文档：保留 PSP 时代策略映射表，支撑合规审计。
- 排障入口：拒绝事件出现在 Pod 的 Events 中，直接读取 `Forbidden` 详情即可定位违反字段。

## 参考链接

- [Pod Security Policy (PSP) - Official Documentation](https://kubernetes.io/docs/concepts/security/pod-security-standards/)

## Related

[[17-系统基础/06-知识字典/security/pod-security-standards.md|Pod Security Standards]]


<!-- risk-assessed -->
