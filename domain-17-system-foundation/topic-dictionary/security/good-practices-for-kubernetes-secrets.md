---
title: Kubernetes Secrets 最佳实践
description: '# Kubernetes Secrets 最佳实践'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- rbac
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Secrets 最佳实践 是什么
- 如何 Kubernetes Secrets 最佳实践
trigger_keywords:
- Kubernetes
- Secrets
- 最佳实践
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
created: "2026-05-23"
---

# [[Kubernetes|Kubernetes]] [[Secrets|Secrets]] 最佳实践

## 概述

在 Kubernetes 中，Secret 是用于存储敏感信息（如密码、OAuth 令牌、SSH 密钥）的对象。Secret 提供了对敏感信息使用方式的更多控制，降低了意外暴露的风险。Secret 值以 base64 编码存储，默认情况下以未加密形式保存在 [[domain-17-system-foundation/topic-dictionary/fundamentals/etcd.md|etcd]] 中，但可以配置为静态加密。以下最佳实践面向集群管理员和应用开发者，旨在提高 Secret 对象的安全性并改善管理效率。

## 核心概念/原理

- **Secret 与 ConfigMap 的区分**：Secret 用于机密数据，ConfigMap 用于非机密数据。
- **静态加密**：默认情况下 Secret 以未加密形式存储在 etcd 中，必须显式启用静态加密。
- **访问控制**：通过 RBAC 限制对 Secret 的访问，避免不必要的 `list`、`watch` 或 `get` 权限。
- **etcd 管理**：etcd 是 Kubernetes 的数据存储后端，其安全直接影响 Secret 的保密性。

## 关键机制或特性

### 集群管理员最佳实践

- **配置静态加密**：
  - 必须配置 Secret 数据在 etcd 中的静态加密（Encrypt Secret Data at Rest）。
- **最小权限访问 Secret**：
  - 仅向最特权、系统级组件授予 `watch` 或 `list` 权限。
  - 对人类用户限制 `get`、`watch`、`list` 访问。
  - 仅允许集群管理员访问 etcd（包括只读访问）。
  - **注意**：授予 `list` 访问权限等同于允许主体获取所有 Secret 的内容。
  - 使用独立的命名空间隔离对挂载 Secret 的访问。
- **改进 etcd 管理策略**：
  - 考虑在 etcd 持久存储不再使用时进行安全擦除（wiping/shredding）。
  - 如果有多个 etcd 实例，配置加密的 SSL/TLS 通信以保护传输中的 Secret 数据。
- **访问外部 Secret 存储**：
  - 可使用第三方 Secrets Store CSI Driver 将机密数据保留在集群外部，并仅授权特定 Pod 以卷的形式访问这些数据。

### 开发者最佳实践

- **限制 Secret 仅对特定容器可见**：
  - 如果 Pod 中有多个容器，但只有部分需要访问 Secret，请仅将卷挂载或环境变量配置添加到需要的容器。
- **保护读取后的 Secret 数据**：
  - 应用在从环境变量或卷读取机密信息后，仍需保护其值。避免在日志中以明文形式记录 Secret，或将其传输给不受信任的第三方。
- **避免共享 Secret 清单文件**：
  - 如果将 Secret 数据以 base64 编码写入清单文件并共享或提交到代码仓库，任何能读取该文件的人都可以看到 Secret。
  - **注意**：base64 编码**不是**加密方法，其保密性与明文无异。

### Swap 内存最佳实践

- 有关 Linux 节点上 swap 内存设置的最佳实践，请参阅 swap 内存管理文档。

## 使用场景

- 安全地存储和管理数据库密码、API 密钥、TLS 证书等敏感凭证。
- 通过 RBAC 和命名空间隔离，控制不同团队或应用对 Secret 的访问。
- 满足合规性要求，对 etcd 中的 Secret 启用静态加密，并集成外部密钥管理系统（KMS）。

## 最佳实践/注意事项

- **始终启用 etcd 静态加密**：这是保护 Secret 免受 etcd 备份或磁盘泄露的基础。
- **优先使用卷挂载而非环境变量**：环境变量更容易通过崩溃转储、日志或进程列表泄露；卷挂载可利用 Linux 文件权限机制进行更细粒度的保护。
- **避免在源代码仓库中提交 Secret 清单**：使用外部 Secret 管理工具或 Kubernetes 原生方式（如 `kubectl create secret`）创建 Secret。
- **定期审查 RBAC 规则**，确保没有主体拥有超出必要的 Secret 访问权限。
- **考虑使用 Secrets Store CSI Driver** 将机密数据保留在外部 KMS 或 Vault 中，减少集群内的持久化敏感数据。

## 参考链接

- https://kubernetes.io/docs/concepts/security/secrets-good-practices/

## Related
- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]
