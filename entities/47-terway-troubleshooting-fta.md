---
title: Terway 故障排查
description: '# Terway 故障排查'
summary: '# Terway 故障排查'
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
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 故障排查 是什么
- 如何 Terway 故障排查
- Terway 故障排查 故障排查
- Terway 故障排查 排障步骤
trigger_keywords:
- Terway
- 故障排查
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
fta_id: FTA-47_TERWAY_TROUBLESHOOTING-001
component: 47 Terway Troubleshooting
severity: high
---



# Terway 故障排查

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 07 - Terway 故障树速查 (FTA Troubleshooting Quick Reference)

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

- [[skills/ts-networking.md|ts-networking]] — 网络故障排查
- [[k8gb]] — K8GB
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 47-terway-troubleshooting-fta