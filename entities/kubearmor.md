---
title: KubeArmor
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- kubearmor
- prometheus
- grafana
- cilium
- crd
- operator
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeArmor 是什么
- 如何 KubeArmor
trigger_keywords:
- KubeArmor
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

# KubeArmor

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

KubeArmor 是一个云原生运行时安全引擎，利用 Linux 安全模块 (LSM - AppArmor, BPF-LSM, SELinux) 在系统级别执行安全策略。它保护 Kubernetes Pod、容器和节点免受已知和未知的威胁，包括进程执行、文件访问和网络操作的细粒度控制。

## 核心能力

- **LSM 强制执行**: 基于 AppArmor/BPF-LSM/SELinux 内核级安全
- **进程控制**: 限制容器内可执行的进程
- **文件保护**: 控制文件/目录的读写访问
- **网络控制**: 限制容器的网络行为
- **系统调用过滤**: 细粒度的 syscall 控制
- **可观测性**: 实时安全遥测数据

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **审计优先**: 先以 audit 模式运行，了解应用行为
- **推荐策略**: 使用 `karmor recommend` 生成基线策略
- **最小权限**: 使用 Allow 模式实现白名单
- **渐进收紧**: 从宽松策略逐步收紧
- **监控告警**: 配置安全事件告警
- **容器加固**: 配合 seccomp 和 capabilities 使用

## 架构定位

在 CNCF 生态中，kubearmor 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[concepts/secrets-management.md|secrets-management]]

## Related

- [[ovn-kubernetes]] — OVN-Kubernetes
- [[vitess]] — Vitess
- [[argo]] — Argo Workflows
- [[keycloak]] — Keycloak
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/kubearmor/kubearmor.md|kubearmor]]
- [[entities/tokenetes.md|Tokenetes]]
- [[entities/containerssh.md|ContainerSSH]]
- [[entities/parsec.md|Parsec]]
- [[entities/athenz.md|Athenz]]
- [[entities/keylime.md|Keylime]]
- [[entities/cartography.md|Cartography]]
- [[entities/bank-vaults.md|Bank-Vaults]]
- [[entities/hexa.md|Hexa]]
- [[entities/paralus.md|Paralus]]
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
