---
title: Terway 运维手册
description: '# Terway 运维手册'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- prometheus
- grafana
- cilium
- networkpolicy
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 运维手册 是什么
- 如何 Terway 运维手册
trigger_keywords:
- Terway
- 运维手册
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 运维手册

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 04 - Terway 运维手册 (Operations Manual)

## 技术细节

详见源文档获取完整技术细节。^[inferred]


## 与 K8s 网络模型的关系

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[NetworkPolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[cilium]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|networkpolicy]]

## Related

- [[antrea]] — Antrea
- [[01-terway-product-overview]] — Terway 产品概览
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[02-terway-architecture-deep-dive]]
- [[04-terway-crd-operations]]
- [[03-terway-usage-guide]]
- [[07-terway-performance-tuning]]
- [[06-terway-testing-validation]]
- [[08-terway-troubleshooting-fta]]
- 44-terway-operations-manual

<!-- risk-assessed -->
