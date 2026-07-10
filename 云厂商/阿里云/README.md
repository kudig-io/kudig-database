---
title: 阿里云专有云与ACK
summary: 阿里云专有云与ACK文档索引及快速入口。
category: cloud-provider
tags:
- cloud-providers
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
---


# 阿里云专有云与ACK

## 文档索引

| # | 文档 | 说明 |
|---|:---|:---|
| 1 | [[云厂商/阿里云/01-专有云架构概述.md|01 专有云架构概述]] | 阿里云专有云产品矩阵、部署模式、与公有云差异 |
| 2 | [[云厂商/阿里云/02-ACK集群运维.md|02 ACK集群运维]] | ACK专有版/托管版集群管理、日志监控、安全 |
| 3 | [[云厂商/阿里云/03-Terway-CNI网络.md|03 Terway CNI网络]] | Terway模式详解、常见问题排查、IP管理 |
| 4 | [[云厂商/阿里云/04-阿里云存储集成.md|04 阿里云存储集成]] | 云盘/ESSD、NAS、OSS、CSI驱动 |
| 5 | [[云厂商/阿里云/05-阿里云SLB与Ingress.md|05 阿里云SLB与Ingress]] | SLB/ALB/NLB、Ingress Controller、CDN/WAF |
| 6 | [[云厂商/阿里云/06-阿里云专有云远程顾问指南.md|06 阿里云专有云远程顾问指南]] | 远程诊断方法论、受限场景替代方案、升级话术 |

## 适用场景

本文档适用于：
- 部署在阿里云专有云环境中的Kubernetes运维
- 使用阿里云ACK（容器服务Kubernetes版）的团队
- 需要远程支持专有云K8s集群的SRE顾问

## 核心差异

| 维度 | 公有云ACK | 专有云ACK |
|:---|:---|:---|
| 控制台 | 阿里云官网 | ASO/天基/ASCM |
| 网络 | 公网+VPC | 仅内网/VPC |
| 存储 | 云产品（ESSD/NAS/OSS） | 飞天分布式存储 |
| 监控 | SLS/ARMS（云产品） | 自建ELK/Prometheus |
| 升级 | 控制台一键 | ASO编排或人工 |
| API | 公网Endpoint | 内网Endpoint |

## 快速入口

- **节点问题**：[[脚本/video-scripts/node-notready.md|node notready]] → 查看阿里云特定分支
- **网络问题**：k8s-dns-failure → 查看阿里云DNS特定分支
- **存储问题**：k8s-pvc-storage → 查看阿里云CSI特定分支
- **Ingress问题**：k8s-ingress-gateway-failure → 查看SLB特定分支
- **自动伸缩**：k8s-autoscaling → 查看ESS特定分支
