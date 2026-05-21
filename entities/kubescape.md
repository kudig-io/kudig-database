---
title: Kubescape
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- kubescape
- prometheus
- grafana
- cilium
- helm
- rbac
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubescape 是什么
- 如何 Kubescape
trigger_keywords:
- Kubescape
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

# Kubescape

> **CNCF 状态**: Incubating | **类别**: Observability | **主要语言**: Go

## 概述

Kubescape 是第一个用于测试 Kubernetes 是否按照 NSA-CISA 和 MITRE ATT&CK 框架安全部署的开源工具。它提供全面的安全平台，包括风险分析、安全合规、镜像漏洞扫描和运行时安全监控。

## 核心能力

- **安全合规扫描**: 支持 NSA-CISA、MITRE ATT&CK、CIS Benchmark
- **镜像漏洞扫描**: 集成 Grype 检测 CVE 漏洞
- **配置扫描**: YAML/Helm/Kustomize 静态分析
- **RBAC 可视化**: 权限分析和最小权限建议
- **运行时监控**: eBPF 实时检测异常行为
- **CI/CD 集成**: GitHub Actions、GitLab CI、Jenkins 插件

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **CI/CD 集成**: 在 PR 阶段自动扫描，阻止不安全配置合并
- **持续监控**: 部署 Operator 定期扫描，跟踪安全态势变化
- **例外管理**: 使用注解标记已接受的风险 `kubescape.io/ignore`
- **渐进式修复**: 按严重级别优先修复 Critical/High 问题
- **SBOM 生成**: 配合漏洞扫描建立软件物料清单

## 架构定位

在 CNCF 生态中，kubescape 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[concepts/storage-model.md|storage-model]]

## Related

- [[clusternet]] — Clusternet
- [[kubeslice]] — KubeSlice
- [[hyperlight]] — Hyperlight
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[helm]] — Helm

- [[domain-19-landscape-references/incubating/kubescape/kubescape.md|kubescape]]
- [[skills/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
