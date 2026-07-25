---
title: Azure AKS
description: AKS 知识域 — 集群生命周期、Azure CNI 网络、Managed Disk 存储、Workload Identity、故障排查
category: subdomain
tags:
- aks
- azure
- azure-cni
- workload-identity
- managed-disk
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# Azure AKS

> Azure Kubernetes Service — 微软云托管 K8s 服务。

## AKS 核心特性

| 特性 | 说明 |
|------|------|
| Azure CNI | Pod 直接获得 VNet IP |
| Workload Identity | 基于 OIDC 的无密钥认证 |
| KEDA 自动缩放 | 事件驱动 HPA |
| Azure Policy | 内置策略合规 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[18-云厂商/04-Azure-AKS/azure-aks-overview.md\|AKS 概览]] | 架构/定价/区域 | beginner |
| [[18-云厂商/04-Azure-AKS/02-aks-cluster-lifecycle-upgrades.md\|集群升级]] | 生命周期与升级策略 | intermediate |
| [[18-云厂商/04-Azure-AKS/03-aks-networking-azure-cni.md\|Azure CNI]] | 网络模式与配置 | advanced |
| [[18-云厂商/04-Azure-AKS/04-aks-storage-managed-disk.md\|存储集成]] | Managed Disk/File CSI | intermediate |
| [[18-云厂商/04-Azure-AKS/05-aks-identity-workload-identity.md\|Workload Identity]] | 无密钥身份认证 | advanced |
| [[18-云厂商/04-Azure-AKS/06-aks-troubleshooting-playbook.md\|故障排查]] | AKS 常见问题处理 | advanced |
| [[18-云厂商/04-Azure-AKS/99-azure-aks-production-runbook.md\|生产 Runbook]] | 生产运维运行手册 | advanced |

## Related

- [[18-云厂商/03-Google-GKE/index.md|Google GKE]]
- [[18-云厂商/index.md|云厂商总索引]]
