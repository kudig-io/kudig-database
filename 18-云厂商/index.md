---
title: 云厂商（Cloud Providers）
description: 云厂商知识域索引 — 阿里云/AWS/GCP/Azure/腾讯云/华为云 K8s 服务、多云混合、其他云
summary: 云厂商知识域目录索引，覆盖各云托管 K8s 服务、专有云（Apsara Stack）、多云混合部署、其他云与选型决策矩阵。
category: index
tags:
- index
- cloud
- multicloud
tier: supporting
created: '2026-07-02'
last_updated: '2026-07-23'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 云厂商（Cloud Providers）

> 阿里云、AWS-EKS、Google-GKE、Azure-AKS、腾讯云 TKE、华为云 CCE、多云混合与其他云。

## 概览

- [[18-云厂商/README.md|Readme]]

## 子目录

| 子目录 | 云厂商 | 核心话题 |
|--------|--------|----------|
| [[18-云厂商/01-阿里云/index.md\|阿里云/]] | 阿里云 | ACK 公有云托管、专有云 Apsara Stack（天基/盘古/合规加固） |
| [[18-云厂商/02-AWS-EKS/index.md\|AWS-EKS/]] | AWS | EKS 架构、Fargate、IRSA、Add-ons |
| [[18-云厂商/03-Google-GKE/index.md\|Google-GKE/]] | Google Cloud | GKE Autopilot/Standard、Workload Identity |
| [[18-云厂商/04-Azure-AKS/index.md\|Azure-AKS/]] | Azure | AKS 架构、AAD 集成、Virtual Nodes |
| [[18-云厂商/05-腾讯云TKE/index.md\|腾讯云TKE/]] | 腾讯云 | TKE 集群、超级节点、弹性容器 |
| [[18-云厂商/06-华为云CCE/index.md\|华为云CCE/]] | 华为云 | CCE Turbo、Volcano 调度、鲲鹏 |
| [[18-云厂商/07-多云混合/index.md\|多云混合/]] | 多云架构 | 多云管理、混合云连接、一致性、灾备 |
| [[18-云厂商/08-其他云/index.md\|其他云/]] | 其他 | 天翼云/移动云/IBM/Oracle/UCloud/火山引擎 |
- [[18-云厂商/00-总览/index.md|00-总览]]

## 文档

- [[18-云厂商/00-总览/99-production-readiness-operations-guide.md|Production Readiness Operations Guide]]

<!-- risk-assessed -->
