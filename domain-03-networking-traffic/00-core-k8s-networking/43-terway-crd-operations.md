---
title: Terway CRD 资源操作
description: '## 概述'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- networkpolicy
- crd
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway CRD 资源操作 是什么
- 如何 Terway CRD 资源操作
trigger_keywords:
- Terway
- CRD
- 资源操作
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
created: "2026-05-23"
---

# Terway CRD 资源操作

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)

## 技术细节

详见源文档获取完整技术细节。^[inferred]


## 与 K8s 网络模型的关系

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[NetworkPolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[cilium]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|networkpolicy]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[40-terway-product-overview]] — Terway 产品概览
- [[44-terway-operations-manual]] — Terway 运维手册
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[41-terway-architecture-deep-dive]]
- [[42-terway-usage-guide]]
- [[46-terway-performance-tuning]]
- [[45-terway-testing-validation]]
- [[47-terway-troubleshooting-fta]]
- 43-terway-crd-operations