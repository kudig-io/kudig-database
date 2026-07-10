---
title: Eraser [entities]
description: '## 概述'
summary: 'Eraser 是一个 Kubernetes 原生的镜像清理工具，用于自动从集群节点中删除存在漏洞的和未使用的容器镜像。它通过与漏洞扫描器（如 [[Trivy|Trivy]]）集成，定期扫描节点上的镜像，自动移除包含高危漏洞的镜像，减小节点的攻击面并释放磁盘空间。'
category: entities
tags:
- k8s
- cncf
- image
- eraser
- coredns
- containerd
- opa
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Eraser 是什么
- 如何 Eraser
trigger_keywords:
- Eraser
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Eraser

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

Eraser 是一个 Kubernetes 原生的镜像清理工具，用于自动从集群节点中删除存在漏洞的和未使用的容器镜像。它通过与漏洞扫描器（如 [[Trivy|Trivy]]）集成，定期扫描节点上的镜像，自动移除包含高危漏洞的镜像，减小节点的攻击面并释放磁盘空间。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **排除列表**: 将关键系统镜像（pause、coredns 等）加入排除列表
- **渐进部署**: 先在非生产集群测试清理策略，确认不会误删关键镜像
- **执行时间**: 将清理任务安排在低峰时段执行，减少对节点的影响
- **严重级别**: 根据组织安全策略选择需要清理的漏洞严重级别
- **磁盘监控**: 配合节点磁盘使用率监控，动态调整清理频率

## 架构定位

在 CNCF 生态中，eraser 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[实体/trivy.md|trivy]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[实体/external-secrets.md|secrets]]]] — External Secrets Operator
- [[kube-burner]] — Kube-burner
- [[实体/trivy.md|trivy]] — Trivy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[coredns]] — CoreDNS

- eraser
- [[实体/zot.md|zot]]
- [[实体/kitops.md|KitOps]]
- [[实体/copa.md|Copa (Copacetic)]]
- [[实体/stacker.md|Stacker]]
- [[实体/xregistry.md|xRegistry]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
