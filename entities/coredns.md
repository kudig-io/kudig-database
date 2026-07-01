---
title: CoreDNS (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- coredns
- etcd
- prometheus
- grafana
- crd
- operator
- statefulset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CoreDNS 是什么
- 如何 CoreDNS
trigger_keywords:
- CoreDNS
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
created: "2026-05-23"
---

# CoreDNS

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **插件架构**: 链式插件处理 DNS 请求
- **Kubernetes 集成**: 原生 K8s 服务发现
- **多后端支持**: 文件、etcd、数据库等
- **健康检查**: 上游健康状态监控
- **指标导出**: Prometheus 指标

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，coredns 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[pod-lifecycle]]

## Related

- [[kuadrant]] — Kuadrant
- [[notary-project]] — Notary Project
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 14-coredns-configuration-corefile
- 28-coredns-troubleshooting-optimization
- 11-dns-service-discovery-coredns
- 15-coredns-plugins-reference
- 13-coredns-architecture-principles
- coredns
- RELEASE-NOTES-1.9
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- [[entities/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]] — Cross-reference
- [[entities/networking-terms.md|K8s 网络术语参考]] — Cross-reference
- [[entities/k8s-structured-troubleshooting.md|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[entities/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[concepts/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Cross-reference
- [[concepts/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[concepts/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[concepts/kubernetes-version-evolution.md|Kubernetes 版本演进]] — Cross-reference
- [[concepts/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[skills/dns-fta.md|DNS 异常故障树分析]] — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-21-statefulset-failure.md|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/k8s-network-configuration-guide.md|Kubernetes 网络配置最佳实践]] — Cross-reference
- [[skills/learn-04-service-basics.md|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[skills/learn-02-first-ticket-guide.md|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/learn-oncall-quick-qa.md|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[skills/learn-12-common-problems.md|第十课：常见问题排查]] — Cross-reference
- [[skills/learn-analogy-dictionary.md|K8S 概念类比词典]] — Cross-reference
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/kubernetes-changelog.md|Kubernetes 变更日志索引]] — Cross-reference
- [[entities/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/dns-index.md|DNS 知识图谱索引]]
