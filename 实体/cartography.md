---
title: Cartography (entities)
description: '## 概述'
summary: 'Cartography 是一个基础设施资产图谱工具，能够自动收集多云环境（AWS、GCP、Azure）、SaaS 服务（GitHub、Okta、GSuite）和安全工具（CrowdStrike、Duo）的资产信息，并将其存储在 Neo4j 图数据库中，构建完整的基础设施关系图谱。'
category: entities
tags:
- k8s
- cncf
- security
- cartography
- containerd
- harbor
- job
- cronjob
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cartography 是什么
- 如何 Cartography
trigger_keywords:
- Cartography
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cartography

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Python

## 概述

Cartography 是一个 CNCF 沙箱项目，由 Lyft 开源，是一个安全资产图谱工具。它通过 Neo4j 图数据库整合来自多个数据源（AWS、GCP、Azure、K8s、GitHub、Okta 等）的基础设施资产信息，构建统一的资产关系图谱。安全团队可以通过 Cypher 查询发现安全风险——如公开暴露的数据库、过于宽松的 IAM 策略、未打补丁的实例等。Cartography 解决了多云多平台环境中资产可见性和安全分析碎片化的问题。

## Key Features（核心能力）

- **多云资产整合**：支持 AWS、GCP、Azure、K8s、GitHub、Okta 等 20+ 数据源
- **Neo4j 图谱**：将资产关系建模为图数据库，支持复杂关系查询
- **自动化同步**：定期从各数据源同步资产和关系数据
- **安全分析查询**：预置安全风险检测 Cypher 查询
- **可扩展架构**：通过 Python 插件机制添加新数据源
- **Jupyter Notebook**：支持在 Notebook 中交互式分析资产图谱

## 架构与工作原理

Cartography 由数据采集和分析两个层组成。采集层通过各数据源的 API（如 AWS SDK、K8s client、GitHub API）拉取资产和关系数据，经过 ETL 转换后写入 Neo4j 图数据库。分析层通过 Cypher 查询语言在图上进行安全分析，如查找「公网暴露的 RDS 实例 → 可访问它的 IAM 角色 → 拥有该角色的用户」等攻击路径。分析作业（Analysis Job）以 JSON/YAML 定义，定期执行并输出风险报告。

## K8s 集成

Cartography 可以作为 CronJob 部署到 Kubernetes，定期同步各云平台和 K8s 集群的资产数据。K8s 数据源 sync 会采集 Cluster、Namespace、Deployment、ServiceAccount、RoleBinding 等资源及其关系。安全团队可以通过图谱查询发现如「具有 cluster-admin 权限的 ServiceAccount → 使用该 SA 的 Pod → Pod 所在节点的安全风险」等攻击路径。

## 生产用例

- **多云安全态势管理**：统一查看多云环境中的资产安全状态
- **攻击路径分析**：发现从外部暴露面到内部敏感资产的攻击路径
- **合规审计**：验证资产配置符合安全策略和合规要求
- **资产盘点**：实时了解全组织的基础设施资产清单

## 安装与快速开始

```bash
pip3 install cartography
# 运行同步
cartography --neo4j-uri bolt://localhost:7687 --target aws
# 或使用 Docker
docker run lyft/cartography --neo4j-uri bolt://neo4j:7687 --target k8s
```

## 对比替代方案

相比传统 CSPM 工具（如 AWS Security Hub），Cartography 支持多云且以图谱方式建模资产关系。相比商业 CSPM（Wiz/Orca），Cartography 是开源的但需要自行维护和配置。

## Related

- [[telepresence]] — Telepresence
- [[08-containerd-multi-tenant]] — [[containerd|containerd]]rd 多租户|containerd 多租户]]租户|多租户]]
- [[harbor]] — Harbor
- [[opentofu]] — OpenTofu
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cartography
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
