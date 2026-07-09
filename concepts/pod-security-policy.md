---
title: PodSecurityPolicy
summary: PodSecurityPolicy（PSP）曾是 Kubernetes 提供的集群级 Pod 安全策略机制，用于在 Pod 创建时强制执行安全规则。通过
  PSP，集群管理员可以统一限制 Pod 的特权行为，防止用户部署存在安全隐患的工作负载。
category: concepts
tags:
- core-concept
- 安全
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: deprecated
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PodSecurityPolicy

## 历史作用

PodSecurityPolicy（PSP）曾是 Kubernetes 提供的**集群级 Pod 安全策略机制**，用于在 Pod 创建时强制执行安全规则。通过 PSP，集群管理员可以统一限制 Pod 的特权行为，防止用户部署存在安全隐患的工作负载。

PSP 作为准入控制器运行，在 Pod 被持久化到 etcd 之前进行检查。若 Pod 不符合任何已授权的 PSP，则创建请求被拒绝。

## 被 Pod Security Admission 取代

Kubernetes v1.23 引入 [[pod-security-admission]]（PSA）作为 PSP 的官方替代方案，v1.25 彻底移除 PSP。PSA 采用**内置准入插件**形式，无需额外启用，使用更简洁的三种预设安全级别：

| 级别 | 说明 |
|------|------|
| `privileged` | 无限制，等价于 PSP 的完全开放 |
| `baseline` | 最小限制，防止已知的高危配置 |
| `restricted` | 最严格，遵循 Pod 加固最佳实践 |

PSA 通过命名空间标签启用，相比 PSP 的 RBAC 绑定机制更易理解和维护。

## 关键策略字段（历史参考）

以下字段曾在 PSP 中用于限制 Pod 安全属性，当前在 PSA 和替代方案中仍有对应概念：

| 字段 | 含义 | PSA 对应 |
|------|------|----------|
| `privileged` | 禁止特权容器 | `restricted` 级别禁止 |
| `hostNetwork` | 禁止共享主机网络命名空间 | `baseline` 级别禁止 |
| `hostPID` | 禁止共享主机 PID 命名空间 | `baseline` 级别禁止 |
| `hostIPC` | 禁止共享主机 IPC 命名空间 | `baseline` 级别禁止 |
| `readOnlyRootFilesystem` | 要求根文件系统只读 | `restricted` 级别要求 |
| `runAsNonRoot` | 要求以非 root 用户运行 | `restricted` 级别要求 |
| `allowPrivilegeEscalation` | 禁止特权提升 | `restricted` 级别禁止 |
| `allowedCapabilities` | 限制可添加的 Linux Capabilities | `restricted` 级别限制 |

## 迁移路径

从 PSP 迁移到 PSA 的建议步骤：

1. **审计现有 PSP**：梳理集群中所有活跃的 PSP 规则，明确各命名空间的安全需求
2. **评估 PSA 级别**：将 PSP 映射到 `privileged` / `baseline` / `restricted` 三个级别
3. **启用审计模式**：在命名空间标签中先设置 `audit` 模式，观察潜在违规而不阻止
4. **验证兼容性**：检查现有工作负载是否满足目标级别要求，必要时调整 Pod 配置
5. **切换强制执行**：确认无违规后，将模式从 `audit` 改为 `enforce`
6. **清理 PSP**：完全移除 PSP 相关资源和 RBAC 绑定

对于 PSP 无法满足的复杂场景，可考虑使用 OPA/Gatekeeper 或 Kyverno 等第三方策略引擎。

## 远程顾问要点

作为远程顾问，面对仍在使用 PSP 的客户集群时：

- **风险告知**：明确说明 PSP 已在 v1.25+ 集群中不可用，升级前必须完成迁移
- **升级阻断评估**：若客户计划升级 Kubernetes 版本，PSP 迁移是升级路径上的硬性依赖
- **兼容性检查**：使用 `kubectl auth can-i` 和审计日志确认现有工作负载对 PSA 级别的兼容性
- **迁移优先级**：建议优先处理生产命名空间，测试环境可作为 PSA 迁移的试点
- **替代方案建议**：若 PSA 的三级模型过于粗粒度，推荐引入 Kyverno 或 Gatekeeper 实现更细粒度的策略控制

安全合规相关内容参见 [[安全/98-merged-indexes/index.md|security-compliance]]。

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
