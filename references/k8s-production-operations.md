---
title: 生产运维：GitOps、FinOps、灾备恢复与变更管理
description: '# 生产运维'
category: reference
tags:
- k8s
- production-ops
- gitops
- finops
- disaster-recovery
- change-management
- etcd
- flux
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 生产运维：GitOps、FinOps、灾备恢复与变更管理 是什么
- 如何 生产运维：GitOps、FinOps、灾备恢复与变更管理
trigger_keywords:
- 生产运维：GitOps
- FinOps
- 灾备恢复与变更管理
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
---

# 生产运维

## GitOps 工作流

核心原则：
- **声明式**：所有配置以 Git 仓库为唯一真相源
- **版本化**：所有变更通过 Git 提交记录
- **自动化**：Agent 自动同步集群状态与 Git 声明
- **自愈**：手动修改会被自动回滚

主流工具：Argo CD（Pull 模式）、Flux CD（K8s 原生）、Tekton（CI 管道）。

## FinOps 成本治理

- **标签规范化**：所有资源标注 team/env/project 标签
- **资源请求优化**：基于实际使用量调整 requests/limits
- **闲置资源清理**：定期扫描未使用的 PV/PVC/Service
- **成本分配**：按 Namespace/Team 分摊集群成本

## 灾备恢复

- **etcd 快照**：每 30 分钟自动备份
- **Velero 备份**：全集群资源 + PV 数据
- **跨区域恢复**：备用集群配置就绪
- **RTO/RPO 目标**：RTO < 30min，RPO < 5min

---

> 来源：.zread/wiki/drafts/20-sheng-chan-yun-wei-*.md

## Related

- [[synthesis/GitOps x 平台工程.md|GitOps x 平台工程]] — GitOps x 平台工程
- [[synthesis/IaC x 多集群管理.md|IaC x 多集群管理]] — 基础设施即代码 x 多集群管理
- [[flux]] — Flux
- [[etcd]] — etcd
- [[argo]] — Argo Workflows
