---
title: Cartography 资产图谱
description: 'Cartography 是 Lyft 开源的安全资产图谱工具，自动收集和关联云基础设施的资产信息，以图数据库（Neo4j）可视化展示资产关系和安全态势。...'
category: dictionary
tags:
- k8s
- glossary
- security
- asset-management
- graph
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cartography 资产图谱 是什么
- Cartography 详解
trigger_keywords:
- Cartography 资产图谱
- Cartography
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Cartography 资产图谱（Cartography）

## 概述

Cartography 是 Lyft 开源的安全资产图谱工具，自动收集和关联云基础设施的资产信息，以图数据库（Neo4j）可视化展示资产关系和安全态势。

## 核心概念/原理

- **资产图谱**：自动发现和关联云基础设施资产
- **Neo4j 可视化**：图数据库驱动的资产关系视图
- **Lyft 开源**：经过 Lyft 大规模生产验证
- **多云支持**：AWS/GCP/Azure/K8s 资产采集

## 关键机制或特性

- 自动化资产采集（Cron 调度）
- 多云资产关联（EC2→S3→IAM→VPC）
- Kubernetes 资产采集
- 安全分析查询（Cypher 查询语言）
- 自定义分析插件
- 差异检测（变更追踪）
- Grafana Dashboard 集成

## 使用场景与最佳实践

- 云基础设施的资产盘点
- 安全态势的可视化分析
- 资产关系的自动化发现
- 合规审计的资产报告
- 安全团队的攻击面分析

## 参考链接

- https://cartography-cncf.github.io/cartography/
- https://github.com/lyft/cartography

## Related

- [[domain-17-system-foundation/topic-dictionary/security/kubescape.md|Kubescape]]
- [[domain-17-system-foundation/topic-dictionary/security/trivy.md|Trivy]]
- [[domain-17-system-foundation/topic-dictionary/operations/cloud-custodian.md|Cloud Custodian]]
