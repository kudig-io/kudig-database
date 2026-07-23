---
title: 阿里云
description: 阿里云目录索引 — 公有云 ACK 与专有云 Apsara Stack 双轨导航
summary: 阿里云目录索引，覆盖专有云运维主干、公有云 ACK 与专有云 Apsara Stack 两条知识轨。
category: index
tags:
- index
- alibaba-cloud
- ack
- apsara-stack
tier: supporting
created: '2026-07-02'
last_updated: '2026-07-23'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 阿里云

> 阿里云容器服务 ACK 的两条知识轨：**专有云（Apsara Stack）运维主干** 与 **公有云 ACK 托管服务**。

阿里云文档按交付形态分为两轨。远程顾问/SRE 场景以专有云运维主干为重点；通用云原生团队以公有云 ACK 为主。

## 知识轨速览

| 知识轨 | 交付形态 | 入口 | 核心受众 |
|--------|----------|------|----------|
| **专有云 Apsara Stack** | 客户数据中心本地化部署 | [[云厂商/阿里云/专有云-Apsara/index.md\|专有云-Apsara/]] | 政企/金融/运营商 SRE、远程顾问 |
| **公有云 ACK** | 阿里云托管 Kubernetes | [[云厂商/阿里云/公有云-ACK/index.md\|公有云-ACK/]] | 通用云原生团队、平台工程师 |

## 专有云运维主干（根文档）

以下根文档是专有云（Apsara Stack）场景的运维主干，定位、方法论与组件映射，适用于远程顾问与驻场 SRE：

| # | 文档 | 内容 |
|---|------|------|
| 01 | [[云厂商/阿里云/01-专有云架构概述.md\|专有云架构概述]] | 专有云产品矩阵、部署模式、与公有云差异 |
| 02 | [[云厂商/阿里云/02-ACK集群运维.md\|ACK集群运维]] | ACK 专有版/托管版集群管理、日志监控、安全 |
| 03 | [[云厂商/阿里云/03-Terway-CNI网络.md\|Terway CNI网络]] | Terway 模式详解、常见问题排查、IP 管理 |
| 04 | [[云厂商/阿里云/04-阿里云存储集成.md\|阿里云存储集成]] | 云盘/ESSD、NAS、OSS、CSI 驱动 |
| 05 | [[云厂商/阿里云/05-阿里云SLB与Ingress.md\|阿里云SLB与Ingress]] | SLB/ALB/NLB、Ingress Controller、CDN/WAF |
| 06 | [[云厂商/阿里云/06-阿里云专有云远程顾问指南.md\|专有云远程顾问指南]] | 远程诊断方法论、受限场景替代方案、升级话术 |
| 09 | [[云厂商/阿里云/09-ack-node-pool-management.md\|ACK节点池管理]] | 节点池生命周期、扩缩容、弹性调度 |
| — | [[云厂商/阿里云/apsara-stack-components.md\|Apsara Stack 组件索引]] | 飞天底座组件（伏羲/洛神/盘古/女娲/天基/ASO）与 K8s 运维映射 |

## 子目录

- [[云厂商/阿里云/专有云-Apsara/index.md|专有云-Apsara/]] — 专有云（Apsara Stack）深度运维：天基/ASO 流程、升级补丁、合规加固（等保/国密）、盘古存储排障、ESS/SLS/POP、专属故障手册
- [[云厂商/阿里云/公有云-ACK/index.md|公有云-ACK/]] — 公有云 ACK：ECS 计算、SLB/NLB/ALB、VPC 网络、RAM 授权、ROS IaC、EBS 存储、生产 Runbook

## 概览

- [[云厂商/阿里云/README.md|Readme]]

<!-- risk-assessed -->
