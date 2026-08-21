---
title: Pod 安全策略
description: '# Pod 安全策略'
summary: '# Pod 安全策略'
category: dictionary
tags:
- k8s
- glossary
- terminology
- opa
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod 安全策略 是什么
- 如何 Pod 安全策略
trigger_keywords:
- Pod
- 安全策略
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod 安全策略

## 概述

PodSecurityPolicy（Pod 安全策略，简称 PSP）是一种已移除的 [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] 安全控制机制。它在 Kubernetes v1.21 中被弃用，并在 **v1.25 中彻底移除**。官方文档不再推荐使用该功能，而是提供了内置和第三方的替代方案来实现相同的 Pod 安全限制。

## 核心概念/原理

PodSecurityPolicy 原本是一种集群级资源，用于在 Pod 创建时强制执行安全策略，控制 Pod 的安全上下文字段（如是否允许特权容器、可添加的 capabilities、是否允许 hostPath 等）。由于其设计复杂、权限模型难以管理，社区决定弃用并移除该 API，转而采用更现代化、更灵活的准入控制机制。

## 关键机制或特性

### 替代方案

PodSecurityPolicy 移除后，可通过以下方式实现类似的 Pod 安全限制：

- **Pod Securityod Security Admission]]（推荐）**：Kubernetes 内置的准入控制器，自 v1.25 起稳定。通过命名空间标签强制执行 Pod 安全标准（Pod Security Standards）的三个级别：`privileged`、`baseline`、`restricted`。
- **第三方准入插件**：如 [[kyverno|Kyverno]]、OPA Gatekeeper、Kubewarden 等，可提供更细粒度、更灵活的策略定义和执行能力。

### 迁移支持

Kubernetes 官方提供了从 PodSecurityPolicy 迁移到内置 Pod Security Admission 控制器的详细指南，帮助现有集群平滑过渡。

## 使用场景

- 正在运行 Kubernetes v1.24 及更早版本并使用 PSP 的集群，需要规划迁移路径。
- 需要为 Pod 创建强制执行安全上下文的集群，应直接采用 Pod Security Admission 或第三方策略引擎。

## 最佳实践/注意事项

- **不要再在新集群中使用 PodSecurityPolicy**；该 API 已在 v1.25 中移除。
- 对于仍在使用 PSP 的集群，应尽快参考官方迁移指南完成升级：
  - *Migrate from PodSecurityPolicy to the Built-In PodSecurity Admission Controller*
- 在迁移过程中，可结合 Pod Security Admission 和第三方 Webhook 准入控制器，逐步将旧策略映射为新的策略规则。
- 评估现有工作负载的实际权限需求，借机清理过度宽松的 PSP 规则，应用最小权限原则。

## 架构深度解析

### PSP 准入评估链路（v1.21-，已弃用）

```
┌──────────────────────────────────────────────────────────────┐
│  Pod 创建请求（kubectl / 控制器）                              │
│   │  ① 认证 → 授权（RBAC）                                     │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ API Server 准入链（admission chain）                     │  │
│  │ └─ PodSecurityPolicy（psp admission）                    │  │
│  │    ├─ 1. 根据请求者 Username 选择可用 PSP 列表            │  │
│  │    │    （通过 RBAC：use verb on PSP）                    │  │
│  │    ├─ 2. 按 PSP 优先级排序（字段互斥打分）                │  │
│  │    ├─ 3. 对 Pod 逐项校验：privileged / hostNetwork /     │  │
│  │    │    volumes / capabilities / SELinux / runAsUser /   │  │
│  │    │    fsGroup / seccomp 等 20+ 字段                    │  │
│  │    └─ 4. 无可用 PSP → 拒绝创建（Forbidden）              │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │  ② 校验通过 → 进入后续准入（LimitRange/ResourceQuota）    │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ etcd 持久化 → Scheduler → Kubelet 创建容器               │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| PSP 准入 | `plugin/pkg/admission/security/podsecuritypolicy/` | PSP 选择与校验主逻辑 |
| 策略选择 | `plugin/pkg/admission/security/podsecuritypolicy/admission.go` | 按 RBAC 与优先级选择 PSP |
| 字段校验 | `pkg/security/podsecuritypolicy/` | privileged/capabilities/volumes 等校验 |
| 迁移支持 | `pkg/apis/policy/validation/` | v1beta1 PSP API 校验（移除前版本） |

### 流程步骤

1. Pod 提交后 PSP 准入控制器根据请求者身份（Username）查询其有权 `use` 的 PSP 列表。
2. 对候选 PSP 按"互斥字段可满足性"计算优先级，选择最佳匹配策略。
3. 对 Pod 的 securityContext 逐字段校验，任一不符即拒绝并返回原因。
4. PSP 不修改 Pod 对象，仅做强制校验（无默认值注入能力）。
5. v1.25 起 PSP API 被移除，集群升级前必须完成迁移（否则升级失败）。

## 生产案例

### 案例 1：宽松 PSP 成为供应链攻击的放大面

| 时间 | 事件 |
| --- | --- |
| T-30d | 运维为"图省事"创建了允许 `privileged: true` 的宽泛 PSP 并绑定全部 SA |
| T+0 | 攻击者利用镜像仓库漏洞植入恶意镜像，Pod 以特权模式运行 |
| T+20min | 恶意容器挂载宿主机 `/` 目录并写入 SSH 公钥 |
| T+2h | 检测到异常进程（crypto miner），确认 3 台节点被横向渗透 |
| T+8h | 重置集群凭证、隔离节点、回滚镜像，业务中断 6 小时 |

- **根因分析**：PSP 边界设计是"白名单"，宽泛策略（privileged + hostPath 任意挂载）等于没有策略；镜像供应链缺少签名校验。
- **修复命令**：
```bash
# 1. 审计当前 PSP 绑定范围（只读）
kubectl get psp -o json | jq -r '.items[].metadata.name'
# 2. 在迁移前收紧：为高危 PSP 打标签并停止绑定（🟡 中风险）
kubectl delete clusterrolebinding psp:privileged --ignore-not-found
# 3. 迁移到 Pod Security Admission 基线
kubectl label ns prod pod-security.kubernetes.io/enforce=baseline --overwrite
```

### 案例 2：PSP 未迁移导致 v1.25 集群升级失败

| 时间 | 事件 |
| --- | --- |
| T+0 | 集群升级 v1.24 → v1.25，`kube-apiserver` 启动失败 |
| T+1h | 定位：etcd 中残留 PSP 对象且 apiserver 强制移除该 API |
| T+3h | 降级回滚至 v1.24，制定迁移计划 |
| T+2w | 全部 PSP 规则映射为 PSS 标签 + 自定义策略，完成升级 |

- **根因分析**：PSP API 在 v1.25 被彻底移除，存量 PSP 对象必须提前清理；同时所有绑定 PSP 的 Role/ClusterRole 中的 `use` 权限也需删除。
- **修复命令**：
```bash
# 1. 升级前清理 PSP 对象与绑定（🟡 中风险，需在维护窗口执行）
kubectl get psp -o name | xargs kubectl delete
# 2. 删除所有引用 PSP 的 RBAC 规则
kubectl get clusterrolebinding -o json | jq -r '.items[].metadata.name' | grep -i psp | xargs -r kubectl delete clusterrolebinding
# 3. 用 kubeadm 升级控制面组件
kubeadm upgrade apply v1.25.0 --yes  # 🔴 高风险：仅维护窗口执行
```

## 对比评测

| 维度 | PodSecurityPolicy | Pod Security Admission | Gatekeeper/Kyverno |
| --- | --- | --- | --- |
| 状态 | v1.25 移除 | 内置 GA | 第三方（活跃） |
| 策略模型 | 白名单评分 | 3 级（privileged/baseline/restricted） | 任意约束（Rego/CEL） |
| 默认值注入 | 不支持 | 支持（PSA 仅强制） | 支持（mutate） |
| 动态策略更新 | 困难 | 简单（标签变更） | 灵活 |
| 适用定位 | 历史遗留 | 基线安全 | 复杂自定义策略 |

**选型建议**：新集群一律用 Pod Security Admission 做基线；需要组织级自定义策略（如禁止特定镜像仓库）时叠加 Gatekeeper 或 Kyverno。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| Pod 被拒绝但无明确原因 | PSP 白名单无匹配 | `kubectl auth can-i use psp/restricted --as=system:serviceaccount:ns:sa` |
| 升级后 psp 命令报错 | API 已移除 | 改用 `kubectl get podsecuritypolicies`（v1.25 前）或 PSA 标签 |
| 策略改了不生效 | PSP 只对新 Pod 生效 | 重建存量 Pod：`kubectl rollout restart deploy` |
| 特权容器仍能创建 | 请求者绑定宽松 PSP | 审计 ClusterRoleBinding 中 `use` 权限 |

## 生产部署清单

- [ ] 升级前完成 PSP → PSS 映射审计（privileged/baseline/restricted 逐项对照）
- [ ] 删除全部 PSP 对象及 RBAC `use` 绑定
- [ ] 在全部命名空间设置 `pod-security.kubernetes.io/enforce` 标签
- [ ] 使用 `audit` 模式灰度观察，再切换 `enforce`
- [ ] 保留 PSP 文档归档供合规追溯

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 集群仍运行 PSP 且计划升级至 v1.25+ | 升级前强制清理 PSP 并切换 PSA，否则升级失败 |
| P1 | PSP 策略存在宽泛规则（privileged 全开） | 立即按 PSS 基线收紧并灰度 enforce |
| P2 | 已迁移 PSA 但无自定义策略引擎 | 评估 Gatekeeper/Kyverno 补充组织策略 |

## 面试要点

1. **Q：PodSecurityPolicy 为什么被弃用？替代方案是什么？**
   A：PSP 存在三大问题：策略选择依赖 RBAC 绑定导致配置复杂易错、20+ 字段学习成本高、白名单评分机制不可预测。替代方案为内置 Pod Security Admission（PSS 三级标准，命名空间标签驱动）与第三方策略引擎（Gatekeeper/Kyverno）。
2. **Q：如何把 PSP 迁移到 Pod Security Admission？**
   A：四步法：审计现有 PSP 实际生效规则 → 将规则映射到 privileged/baseline/restricted 三级 → 先用 `audit`/`warn` 模式观察违规 Pod → 确认后切换 `enforce` 并清理 PSP 对象与 RBAC 绑定。注意 v1.25 前必须完成迁移。
3. **Q：Pod Security Admission 与 Gatekeeper 的边界是什么？**
   A：PSA 内置、零依赖，覆盖 3 级基线安全，适合统一基线；Gatekeeper 提供 Rego 全自定义策略与 Mutation，适合组织级复杂规则（如镜像仓库白名单、标签强制）。实践中通常 PSA 做基线 + Gatekeeper 做扩展。

## 运维要点

- 升级窗口管理：PSP 清理必须纳入 v1.25+ 升级前置检查项。
- 灰度策略：PSA 先 `warn`/`audit` 观察 1-2 周再 `enforce`，避免误杀存量工作负载。
- 审计留存：迁移前后的策略映射表归档，供合规审计追溯。
- 巡检：定期扫描命名空间是否缺少 PSA 标签（`kubectl get ns -o jsonpath` 校验）。
- 排障入口：Pod 拒绝事件（`kubectl describe pod` 的 Events 段）会直接标明违反的 PSS 级别与字段。

## 参考链接

- https://kubernetes.io/docs/concepts/security/pod-security-policy/

## Related

- [[17-系统基础/06-知识字典/security/admission-controller.md|准入控制器]]
- [[17-系统基础/06-知识字典/security/application-security-checklist.md|应用安全清单]]
- [[17-系统基础/06-知识字典/security/athenz.md|Athenz 身份认证与授权]]


<!-- risk-assessed -->
