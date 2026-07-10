---
title: KubeArmor 运行时安全
description: KubeArmor 是 Accuknox 开源的 CNCF Sandbox 项目，基于 eBPF 和 LSM（Linux Security
  Modules）为 ...
summary: KubeArmor 是 Accuknox 开源的 CNCF Sandbox 项目，基于 eBPF 和 LSM（Linux Security Modules）为
  ...
category: dictionary
tags:
- k8s
- glossary
- security
- runtime
- ebpf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeArmor 运行时安全 是什么
- KubeArmor 详解
trigger_keywords:
- KubeArmor 运行时安全
- KubeArmor
- dictionary
prerequisites:
- kubernetes
---



# KubeArmor 运行时安全（KubeArmor）

## 概述

KubeArmor 是 Accuknox 开源的 CNCF Sandbox 项目，基于 eBPF 和 LSM（Linux Security Modules）为 Kubernetes 提供运行时安全策略，限制容器的文件/网络/进程行为。

## 核心概念/原理

- **eBPF + LSM**：在内核层拦截容器的系统调用
- **运行时策略**：限制容器可访问的文件/网络/进程
- **CNCF Sandbox**：Accuknox 主导
- **可视化**：提供安全事件的可视化和告警

## 关键机制或特性

- KubeArmorPolicy CRD 定义安全策略
- 文件访问控制（读写/执行限制）
- 网络访问控制（出站/入站限制）
- 进程执行控制（允许/拒绝列表）
- AppArmor/SELinux/BPF-LSM 后端
- 安全事件日志和告警
- KubeArmor VM（非 K8s 环境支持）

## 使用场景与最佳实践

- 容器运行时的安全加固
- 最小权限原则的强制执行
- 合规要求下的运行时安全策略
- 零信任架构中的工作负载保护
- 安全审计和合规报告

## 参考链接

- https://kubearmor.io/
- https://github.com/kubearmor/KubeArmor

## Related

- [[系统基础/topic-dictionary/security/falco.md|Falco]]
- [[系统基础/topic-dictionary/security/opa.md|OPA]]
- [[系统基础/topic-dictionary/security/kyverno.md|Kyverno]]
