---
title: Kubescape 安全扫描
description: Kubescape 是 ARMO 开源的 CNCF Sandbox 项目，提供 Kubernetes 集群的全方位安全扫描，包括配置审计、漏洞检测、RBAC
  分...
summary: Kubescape 是 ARMO 开源的 CNCF Sandbox 项目，提供 Kubernetes 集群的全方位安全扫描，包括配置审计、漏洞检测、RBAC
  分...
category: dictionary
tags:
- k8s
- glossary
- security
- scanning
- compliance
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubescape 安全扫描 是什么
- Kubescape 详解
trigger_keywords:
- Kubescape 安全扫描
- Kubescape
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubescape 安全扫描（Kubescape）

## 概述

Kubescape 是 ARMO 开源的 CNCF Sandbox 项目，提供 Kubernetes 集群的全方位安全扫描，包括配置审计、漏洞检测、RBAC 分析和合规检查，是集群安全评估的瑞士军刀。

## 核心概念/原理

- **全方位扫描**：配置/漏洞/RBAC/镜像/网络策略一键扫描
- **合规框架**：内置 NSA/CISA/MITRE/CIS 等合规基准
- **CNCF Sandbox**：ARMO 主导
- **左移安全**：支持 CI/CD 和 IDE 集成

## 关键机制或特性

- `kubescape scan` 一键安全扫描
- 支持多种框架（NSA/CISA/CIS/MITRE/SOC2）
- RBAC 可视化分析
- 镜像漏洞扫描（集成 Grype/Trivy）
- NetworkPolicy 生成建议
- 修复建议自动生成
- Helm Chart 安全扫描

## 使用场景与最佳实践

- K8s 集群安全基线评估
- 合规审计（NSA/CIS/SOC2）
- CI/CD Pipeline 的安全门控
- RBAC 权限审计和优化
- 新集群上线前的安全检查

## 参考链接

- https://kubescape.io/
- https://github.com/kubescape/kubescape

## Related

- [[系统基础/topic-dictionary/security/trivy.md|Trivy]]
- [[系统基础/topic-dictionary/security/opa.md|OPA]]
- [[系统基础/topic-dictionary/security/kyverno.md|Kyverno]]


<!-- risk-assessed -->
