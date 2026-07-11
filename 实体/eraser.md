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
last_updated: 2026-07
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

Eraser 是一个 CNCF 沙箱项目，由 Microsoft 开源，是 Kubernetes 集群的自动化镜像清理工具。它定期扫描节点上的容器镜像，删除不安全或不再使用的镜像，减少节点存储消耗和攻击面。Eraser 特别关注安全管理——可以自动删除包含已知漏洞（CVE）的镜像，防止不安全镜像在节点上被使用。与 K8s 原生的镜像垃圾回收（基于磁盘阈值）不同，Eraser 提供基于策略的主动镜像清理。

## Key Features（核心能力）

- **漏洞扫描清理**：集成 Trivy 自动扫描并删除包含 CVE 的镜像
- **未使用镜像清理**：删除节点上没有运行容器的镜像
- **镜像排除列表**：通过配置保护关键镜像不被清理
- **定时清理**：通过 CronJob 或 EraserSchedule CRD 定期执行
- **节点资源释放**：可视化报告清理后的存储空间回收
- **ImageList 管理**：通过 CRD 声明式管理需要清理的镜像列表

## 架构与工作原理

Eraser 由 Manager、Collector 和 Remover 三个组件构成。Manager 作为 Controller 管理清理任务的生命周期；Collector 以 DaemonSet 方式运行在每个节点上，扫描本地镜像列表和漏洞信息；Remover 执行实际的镜像删除操作（通过 containerd/CRI API）。通过 ImageList CRD 声明需要删除的镜像，Manager 协调各节点上的 Collector 和 Remover 执行清理。

## K8s 集成

Eraser 通过 CRD 与 Kubernetes 集成。ImageList CRD 定义需要从节点清理的镜像列表（通过镜像名或正则匹配）。Eraser ConfigMap 配置全局策略（排除列表、扫描器配置）。Manager 通过 Deployment 部署，Collector/Remover 通过 DaemonSet 运行在每个节点。通过 containerd 的 Image Service API 或 nerdctl 执行镜像删除。

## 生产用例

- **节点存储管理**：定期清理未使用镜像释放节点磁盘
- **安全漏洞修复**：自动删除包含已知 CVE 的镜像
- **合规要求**：确保节点上不残留过期或不安全的镜像
- **大规模集群维护**：数千节点集群的镜像清理自动化

## 安装与快速开始

```bash
helm repo add eraser https://eraser-dev.github.io/eraser/charts
helm install eraser eraser/eraser -n eraser-system --create-namespace
```

## 对比替代方案

相比 K8s 原生镜像 GC（基于磁盘阈值被动清理），Eraser 提供基于策略的主动清理和安全扫描。相比手动清理脚本，Eraser 提供声明式管理和高可靠性。

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
