---
title: Kubernetes Pod 安全最佳实践
description: '# Kubernetes Pod 安全最佳实践'
summary: '本指南提供生产环境 Kubernetes Pod 安全配置的最佳实践，涵盖从 Pod 安全标准到运行时安全的全方位内容 ^[inferred]。'
category: skills
tags:
- k8s
- security
- pod-security
- pss
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Pod 安全最佳实践 是什么
- 如何 Kubernetes Pod 安全最佳实践
trigger_keywords:
- Kubernetes
- Pod
- 安全最佳实践
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Pod 安全最佳实践

## 概述

本指南提供生产环境 Kubernetes Pod 安全配置的最佳实践，涵盖从 Pod 安全标准到运行时安全的全方位内容 ^[inferred]。

## Pod 安全标准（PSS）

Kubernetes 内置三个 PSS 级别 ^[inferred]：

| 级别 | 描述 | 限制内容 | 适用场景 |
|------|------|---------|---------|
| **Privileged** | 无限制 | 无 | 系统组件、可信工作负载 |
| **Baseline** | 最低限制 | 禁止 hostNetwork/hostPID/hostIPC | 大多数应用 |
| **Restricted** | 严格限制 | 必须非 root、只读根文件系统 | 安全敏感应用 |

### 命名空间级 PSS 配置

```yaml
labels:
  pod-security.kubernetes.io/enforce: restricted
  pod-security.kubernetes.io/audit: restricted
  pod-security.kubernetes.io/warn: restricted
```

生产命名空间推荐使用 `restricted`，开发命名空间可使用 `baseline` ^[inferred]。

## 安全上下文配置

### Pod 级别

- `runAsNonRoot: true` — 禁止以 root 运行 ^[inferred]
- `runAsUser: 1000` — 指定非 root 用户 ID ^[inferred]
- `seccompProfile.type: RuntimeDefault` — 启用默认 seccomp 配置 ^[inferred]

### 容器级别

- `allowPrivilegeEscalation: false` — 禁止特权提升 ^[inferred]
- `readOnlyRootFilesystem: true` — 只读根文件系统 ^[inferred]
- `capabilities.drop: [ALL]` — 丢弃所有 Linux 能力 ^[inferred]
- 按需添加特定能力（如 `NET_BIND_SERVICE`）^[inferred]

### 只读根文件系统的可写卷

启用 `readOnlyRootFilesystem` 后，需要为 /tmp、/var/cache、/var/run 等提供 emptyDir 卷 ^[inferred]。

## RBAC 最小权限

- 为每个应用创建专用 ServiceAccount ^[inferred]
- `automountServiceAccountToken: false` — 默认不挂载 Token ^[inferred]
- Role 权限最小化，仅授予所需的 API 资源和操作 ^[inferred]

## 镜像安全

- 使用可信镜像仓库白名单 ^[inferred]
- 镜像安全扫描（[[Trivy|Trivy]] 等）^[inferred]
- 避免使用 `latest` 标签，使用固定版本 ^[ambiguous]

## 常见陷阱

### 忽略 init 容器安全

只配置主容器安全上下文而忽略 init 容器会导致安全风险。init 容器也应配置 `runAsNonRoot` 和 `allowPrivilegeEscalation: false` ^[inferred]。

### 卷挂载权限不当

只读根文件系统但未提供可写卷会导致应用无法写入临时文件而启动失败 ^[inferred]。

### 能力配置错误

丢弃所有能力但应用需要特定能力会导致功能异常。应根据应用需求添加所需的最小能力集 ^[inferred]。

## 验证方法

- 检查 PSS 配置：`kubectl get namespace -L pod-security.kubernetes.io/enforce`
- 检查特权容器和 root 用户容器
- 检查 ServiceAccount 和 RBAC 配置

## 相关资源

- [[22-概念/k8s-production-best-practices.md|[[Kubernetes 生产环境最佳实践|Kubernetes 生产环境最佳实践]]]]
- [[22-概念/05-安全/security-defense-depth.md|Defense-in-Depth Security]]
- [[22-概念/secrets-management.md|[[Secrets Management|Secrets Management]]]]
- [[26-技能/07-安全/rbac/audit-rbac-configurations.md|Audit RBAC Configurations]]

## 生产案例

### 案例 1: 特权容器逃逸导致节点被控制

| 时间 | 事件 |
|------|------|
| - | 安全团队发现某 Pod 以 privileged 模式运行 |
| - | 容器内可访问宿主机 /proc 和 /sys |
| - | 🟡 移除 privileged: true，设置 securityContext |

**根因**: 开发为调试方便设置了 privileged，未恢复。

### 案例 2: 容器以 root 运行导致安全漏洞

**现象**: 容器内进程以 UID 0 运行，可读取敏感文件。

**诊断**: `kubectl exec pod -- id` 显示 uid=0(root)

**修复**: 🟢 设置 runAsNonRoot: true + runAsUser: 1000

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 发现容器逃逸 | 立即隔离 Pod + 审查节点 |
| P1 | 安全配置缺失 | 补充 securityContext |
| P2 | 安全加固 | 实施 PSA restricted |

## 面试要点

1. **Q: Pod SecurityContext 的关键配置？**
   A: runAsNonRoot、runAsUser、readOnlyRootFilesystem、allowPrivilegeEscalation=false、capabilities(drop ALL)、seccompProfile(RuntimeDefault)。

2. **Q: 如何实施最小权限原则？**
   A: ① 非 root 运行 ② 只读文件系统 ③ 最小 capabilities ④ 禁止特权模式 ⑤ 限制 hostPath/hostNetwork ⑥ 使用 Seccomp/AppArmor 限制系统调用。

3. **Q: PSA 与 OPA Gatekeeper 的选择？**
   A: PSA: 原生、简单、三级标准(privileged/baseline/restricted)；Gatekeeper: 灵活、自定义策略、审计能力强。简单场景用 PSA，复杂策略用 Gatekeeper/Kyverno。

## Related

- [[23-实体/06-安全/trivy.md|trivy]] — Trivy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/05-安全/secrets-management.md|secrets-management]] — Secrets Management
- [[22-概念/10-最佳实践/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security


<!-- risk-assessed -->
