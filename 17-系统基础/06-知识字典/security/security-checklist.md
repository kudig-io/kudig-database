---
title: 安全清单
description: '# 安全清单'
summary: '# 安全清单'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- kubelet
- controller-manager
- rbac
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
- 安全清单 是什么
- 如何 安全清单
trigger_keywords:
- 安全清单
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安全清单

## 概述

本清单旨在提供一个基础的安全指导列表，并附带指向各主题更全面文档的链接。它并不声称是详尽的，而是会随着时间不断发展。清单中的项目顺序不反映优先级，某些项目可能在各小节下的段落中有更详细的说明。请记住，清单本身不足以单独实现良好的安全态势；安全是一条永无止境的旅程，需要持续关注和改进。

## 核心概念/原理

安全清单覆盖了 [[kubernetes|Kubernetes]] 集群安全的多个关键领域：认证与授权、网络安全、Pod 安全、日志与审计、Pod 放置、[[secrets|Secrets]]、镜像和准入控制器。它为集群管理员提供了一套可操作的基线检查项。

## 关键机制或特性

### 认证与授权

- `system:masters` 组在引导完成后不用于用户或组件认证。
- kube-controller-manager 启用 `--use-service-account-credentials`。
- 根证书受到保护（离线 CA 或具有有效访问控制的托管在线 CA）。
- 中间证书和叶子证书的有效期不超过 3 年。
- 建立定期访问审查流程，审查间隔不超过 24 个月。
- 遵循 RBAC 最佳实践指南。

**说明**：`system:masters` 应仅作为“应急破门”机制，而不是日常管理员账号使用。

### 网络安全

- 使用的 CNI 插件支持网络策略（[[17-系统基础/06-知识字典/networking/network-policies.md|Network Policies]]）。
- 为集群中的所有工作负载应用入站和出站网络策略。
- 在每个命名空间中设置默认网络策略，选择所有 Pod 并拒绝所有流量。
- 在适当时使用服务网格加密集群内的所有通信。
- [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]]、kubelet API 和 etcd 不公开暴露在互联网上。
- 过滤工作负载对云元数据 API（`169.254.169.254`）的访问。
- 限制 LoadBalancer 和 ExternalIPs 的使用（缓解 CVE-2020-8554）。

### Pod 安全

- 仅在必要时授予工作负载的 `create`、`update`、`patch`、`delete` RBAC 权限。
- 为所有命名空间应用并强制执行适当的 Pod 安全标准策略。
- 为工作负载设置内存限制，且限制值等于或小于请求值。
- 可在敏感工作负载上设置 CPU 限制。
- 在支持的节点上启用 Seccomp，并配置适当的系统调用配置文件。
- 在支持的节点上启用 AppArmor 或 SELinux，并配置适当的配置文件。

**说明**：RBAC 授权至关重要，但其粒度不足以对 Pod 资源本身进行细粒度授权。Pod 安全标准通过三个策略（privileged、baseline、restricted）限制 PodSpec 中安全相关字段的设置，可通过内置的 Pod Security Admission 或第三方 webhook 强制执行。

### 日志与审计

- 如果启用了审计日志，应保护其免受一般访问。

### Pod 放置

- 根据应用的敏感级别进行 Pod 放置。
- 敏感应用应在隔离的节点上运行，或使用特定的沙箱运行时运行。

**说明**：可使用 Node Selectors、PodTolerationRestriction 和 RuntimeClass 等功能强制执行节点隔离策略，防止不同敏感级别的应用意外部署到同一节点。

### Secrets

- 不使用 ConfigMap 存储机密数据。
- 为 Secret API 配置静态加密。
- 如适用，部署并使用将 Secret 存储在第三方存储中的注入机制（如 Secrets Store CSI Driver）。
- 不需要服务账号令牌的 Pod 不应挂载它们。
- 使用 Bound Service Account Token Volume 替代不过期的令牌。

### 镜像

- 最小化容器镜像中的不必要内容。
- 容器镜像配置为以非特权用户运行。
- 通过 sha256 摘要（而非标签）引用容器镜像，或在部署时通过准入控制验证镜像的数字签名。
- 定期扫描容器镜像，修补已知漏洞软件。

**说明**：生产镜像不应包含 shell 或调试工具，可使用临时调试容器（ephemeral debug container）进行故障排查。

### 准入控制器

- 启用适当的准入控制器选择。
- 通过 Pod Security Admission 和/或 webhook 准入控制器强制执行 Pod 安全策略。
- 安全地配置准入链插件和 webhook。

**推荐启用的默认插件**：`CertificateApproval`、`CertificateSigning`、`CertificateSubjectRestriction`、`LimitRanger`、`MutatingAdmissionWebhook`、`PodSecurity`、`ResourceQuota`、`ValidatingAdmissionWebhook`。

**建议额外启用的插件**：`DenyServiceExternalIPs`、`NodeRestriction`。

**可考虑启用的插件**：`AlwaysPullImages`、`ImagePolicyWebhook`。

## 使用场景

- 集群管理员在部署或维护集群时进行安全基线检查。
- 安全审计团队评估集群的安全配置。
- 作为安全加固项目的起点，逐步实施更全面的安全措施。

## 最佳实践/注意事项

- 清单是起点而非终点，需要持续改进和关注。
- 某些建议可能对特定安全需求过于严格或过于宽松，需根据实际情况评估。
- Kubernetes 安全不是“一刀切”，每个类别的清单项都应根据其优点进行评估。
- 在应用 Pod 安全标准时，可先在 `warn` 和 `audit` 模式下运行，再切换到 `enforce` 模式。
- 定期审查和更新安全配置，以应对新出现的威胁和漏洞。

## 参考链接

- https://kubernetes.io/docs/concepts/security/security-checklist/

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
