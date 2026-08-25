---
title: 阿里云 ACK 与专有云（Apsara Stack）
description: 阿里云容器服务知识域入口 — 公有云 ACK 与专有云 Apsara Stack 双轨导航、文档索引与核心差异
summary: 阿里云 ACK（公有云托管）与专有云 Apsara Stack（本地化）双轨知识域入口、文档索引及快速入口。
category: cloud-provider
tags:
- alibaba-cloud
- ack
- apsara-stack
- private-cloud
- public-cloud
tier: core
created: '2026-05-23'
last_updated: '2026-07-23'
---

# 阿里云 ACK 与专有云（Apsara Stack）

> 阿里云容器服务按交付形态分为两条知识轨：**专有云 Apsara Stack**（本地化部署，政企/金融/运营商）与**公有云 ACK**（托管 Kubernetes）。

## 双轨知识导航

阿里云文档按交付形态组织。远程顾问/SRE 场景以专有云运维主干为重点；通用云原生团队以公有云 ACK 为主。

| 知识轨 | 交付形态 | 子目录 | 核心话题 |
|--------|----------|--------|----------|
| **专有云 ACK** | 客户数据中心本地化部署 | [[18-云厂商/01-阿里云/专有云-ACK/index.md\|专有云-ACK/]] | 天基/ASO 运维、飞天底座组件、升级补丁、合规加固（等保/国密）、盘古存储、ESS/SLS/POP、故障手册 |
| **公有云 ACK** | 阿里云托管 Kubernetes | [[18-云厂商/01-阿里云/公有云-ACK/index.md\|公有云-ACK/]] | ECS 计算、SLB/NLB/ALB、VPC 网络、RAM/RRSA 授权、ROS IaC、EBS 存储、生产 Runbook |
| **公共云架构** | 公有云整体架构与方案 | [[18-云厂商/01-阿里云/公共云/index.md\|公共云/]] | 计算、存储、网络、数据库、安全、大数据、中间件全景 |

## 专有云运维主干（根文档）

以下根文档为专有云（Apsara Stack）场景的运维主干，覆盖定位、方法论与组件映射：

| # | 文档 | 说明 |
|---|:---|:---|
| 1 | [[18-云厂商/01-阿里云/专有云-ACK/01-专有云架构概述.md\|01 专有云架构概述]] | 阿里云专有云产品矩阵、部署模式、与公有云差异 |
| 2 | [[18-云厂商/01-阿里云/公有云-ACK/02-ACK集群运维.md\|02 ACK集群运维]] | ACK 专有版/托管版集群管理、日志监控、安全 |
| 3 | [[18-云厂商/01-阿里云/公有云-ACK/03-Terway-CNI网络.md\|03 Terway CNI网络]] | Terway 模式详解、常见问题排查、IP 管理 |
| 4 | [[18-云厂商/01-阿里云/公有云-ACK/04-阿里云存储集成.md\|04 阿里云存储集成]] | 云盘/ESSD、NAS、OSS、CSI 驱动 |
| 5 | [[18-云厂商/01-阿里云/公有云-ACK/05-阿里云SLB与Ingress.md\|05 阿里云SLB与Ingress]] | SLB/ALB/NLB、Ingress Controller、CDN/WAF |
| 6 | [[18-云厂商/01-阿里云/专有云-ACK/06-阿里云专有云远程顾问指南.md\|06 专有云远程顾问指南]] | 远程诊断方法论、受限场景替代方案、升级话术 |
| — | [[18-云厂商/01-阿里云/专有云-ACK/apsara-stack-components.md\|Apsara Stack 组件索引]] | 飞天底座组件（伏羲/洛神/盘古/女娲/天基/ASO）与 K8s 运维映射 |

## 适用场景

本文档适用于：
- 部署在阿里云专有云环境中的 Kubernetes 运维
- 使用阿里云 ACK（容器服务 Kubernetes 版）的团队
- 需要远程支持专有云 K8s 集群的 SRE 顾问

## 核心差异

| 维度 | 公有云 ACK | 专有云 ACK |
|:---|:---|:---|
| 控制台 | 阿里云官网 | ASO/天基/ASCM |
| 网络 | 公网+VPC | 仅内网/VPC |
| 存储 | 云产品（ESSD/NAS/OSS） | 飞天分布式存储（盘古） |
| 监控 | SLS/ARMS（云产品） | 自建 ELK/Prometheus |
| 升级 | 控制台一键 | ASO 编排或人工 |
| API | 公网 Endpoint | 内网 Endpoint |
| 身份认证 | RAM + OIDC | 客户 IAM/AD + RAM、IDaaS 联邦 |
| 密钥管理 | KMS 云密钥 | 客户 HSM/内部 KMS、国密 SM2/SM4 |

## 快速入口

- **节点问题**：[[18-云厂商/01-阿里云/公有云-ACK/07-ack-node-pool-management.md|节点池管理]] → 查看阿里云特定分支
- **网络问题**：[[18-云厂商/01-阿里云/公有云-ACK/03-Terway-CNI网络.md|Terway 网络]] → 查看 CNI 排障
- **存储问题**：[[18-云厂商/01-阿里云/专有云-ACK/008-apsara-pangu-storage-troubleshooting.md|盘古存储排障]] → 专有云存储故障手册
- **Ingress 问题**：[[18-云厂商/01-阿里云/公有云-ACK/05-阿里云SLB与Ingress.md|SLB 与 Ingress]] → 查看 SLB 特定分支
- **底座组件**：[[18-云厂商/01-阿里云/专有云-ACK/apsara-stack-components.md|Apsara Stack 组件索引]] → 工单→底座组件映射
- **故障手册**：[[18-云厂商/01-阿里云/专有云-ACK/01-apsara-stack-troubleshooting-runbook.md|专有云故障手册]] → 可执行 runbook
- **合规加固**：[[18-云厂商/01-阿里云/专有云-ACK/007-apsara-compliance-hardening.md|合规加固]] → 等保四级/国密
- **公共云全景**：[[18-云厂商/01-阿里云/公共云/index.md|公共云架构]] → 架构师面试准备
