---
title: 加固指南 - 调度器配置
description: '# 加固指南 - 调度器配置'
summary: '# 加固指南 - 调度器配置'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- scheduler
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 加固指南 - 调度器配置 是什么
- 如何 加固指南 - 调度器配置
trigger_keywords:
- 加固指南
- 调度器配置
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 加固指南 - 调度器配置

## 概述

[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 调度器（kube-scheduler）是控制平面的关键组件之一。配置错误的调度器可能产生安全影响，例如被用于针对特定节点并驱逐共享该节点及其资源的工作负载或应用，从而协助攻击者实施 **Yo-Yo 攻击**（针对脆弱自动扩缩容器的攻击）。本文档介绍如何提高调度器的安全态势。

## 核心概念/原理

调度器负责将 Pod 分配到合适的节点上运行。如果攻击者能够影响调度决策，他们可能将恶意 Pod 调度到敏感节点，或通过不断调度/驱逐工作负载来消耗集群资源。因此，必须确保调度器的认证、授权和网络配置是安全的，并谨慎审查自定义调度器插件。

## 关键机制或特性

### kube-scheduler 认证配置

设置认证配置时，应确保 kube-scheduler 的认证与 kube-apiserver 保持一致。任何缺少认证标头的请求都应通过 kube-apiserver 进行认证，以保证集群中所有认证的一致性。

- **`authentication-kubeconfig`**：提供正确的 kubeconfig 文件，使调度器能够从 API Server 检索认证配置选项。该文件应使用严格的文件权限进行保护。
- **`authentication-tolerate-lookup-failure`**：设置为 `false`，确保调度器**始终**从 API server 查找认证配置。
- **`authentication-skip-lookup`**：设置为 `false`，确保调度器**始终**从 API server 查找认证配置。
- **`authorization-always-allow-paths`**：这些路径应返回适合匿名授权的数据。默认值为 `/healthz,/readyz,/livez`。
- **`profiling`**：设置为 `false` 以禁用性能分析端点。这些端点提供调试信息，但在生产集群中不应启用，因为它们可能导致拒绝服务或信息泄露。该参数已弃用，现在可通过 KubeScheduler 的 `DebuggingConfiguration` 配置，将 `enableProfiling` 设为 `false`。
- **`requestheader-client-ca-file`**：避免传递此参数。

### 调度器网络命令行选项

- **`bind-address`**：在大多数情况下，kube-scheduler 不需要外部可访问。将绑定地址设置为 `localhost` 是安全的做法。
- **`permit-address-sharing`**：设置为 `false`，以禁止通过 `SO_REUSEADDR` 共享连接。`SO_REUSEADDR` 可能导致重用处于 `TIME_WAIT` 状态的已终止连接。
- **`permit-port-sharing`**：默认值为 `false`。除非您完全理解其安全影响，否则请保持默认值。

### 调度器 TLS 命令行选项

- **`tls-cipher-suites`**：始终提供一组首选的密码套件列表，确保加密永远不会使用不安全的密码套件。

### 自定义调度器的调度配置

当使用基于 Kubernetes 调度代码的自定义调度器时，集群管理员需要谨慎对待使用 `queueSort`、`prefilter`、`filter` 或 `permit` 扩展点的插件。这些扩展点控制调度过程的不同阶段，错误的配置可能影响集群中 kube-scheduler 的行为。

#### 关键注意事项

- **`queueSort` 扩展点**：同一时间只能启用一个使用 `queueSort` 的插件，任何使用该扩展点的插件都应仔细审查。
- **`prefilter` / `filter` 扩展点**：实现这些扩展点的插件可能将所有节点标记为不可调度，从而完全停止新 Pod 的调度。
- **`permit` 扩展点**：实现该扩展点的插件可以阻止或延迟 Pod 的绑定，应经过集群管理员的彻底审查。

#### 禁用高风险扩展点示例

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
  - schedulerName: my-scheduler
    plugins:
      queueSort:
        disabled:
        - name: "*"
      filter:
        disabled:
        - name: "*"
      permit:
        disabled:
        - name: "*"
```

### 禁止用户为节点打标签

集群管理员应确保集群用户无法为节点添加标签。恶意行为者可能利用 `nodeSelector` 将工作负载调度到不应该存在的节点上。

## 使用场景

- 加固生产集群中的 kube-scheduler 配置。
- 部署自定义调度器并审查其插件安全性。
- 防止用户通过节点标签操控 Pod 调度。

## 最佳实践/注意事项

- 确保调度器认证与 API server 保持一致，认证查找失败时不应被容忍。
- 将调度器的 `bind-address` 设置为 `localhost`，减少网络暴露面。
- 在生产环境中**禁用 profiling**（`enableProfiling: false`）。
- 始终配置安全的 TLS 密码套件列表。
- 对于非默认的调度器插件，特别是在 `queueSort`、`filter` 和 `permit` 扩展点上的插件，进行严格的代码审查和安全评估。
- **禁止普通用户为节点添加标签**，防止恶意节点亲和性/选择器攻击。

## 参考链接

- https://kubernetes.io/docs/concepts/security/hardening-guide/scheduler/

## Related

- [[系统基础/topic-dictionary/security/admission-controller.md|准入控制器]]
- [[系统基础/topic-dictionary/security/application-security-checklist.md|应用安全清单]]
- [[系统基础/topic-dictionary/security/athenz.md|Athenz 身份认证与授权]]


<!-- risk-assessed -->
