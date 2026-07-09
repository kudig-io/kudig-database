---
title: 跨云迁移手册
description: '策略 1: 重新部署 (Rehost)'
summary: '策略 1: 重新部署 (Rehost)'
category: synthesis
tags:
- multi-cloud
- migration
- cloud-providers
- kubernetes
- strategy
- helm
- rbac
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 跨云迁移手册 是什么
- 如何 跨云迁移手册
trigger_keywords:
- 跨云迁移手册
prerequisites:
- kubectl-basics
- helm-basics
- backup-basics
relationships:
- target: '[[entities/helm.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 跨云迁移手册

## 迁移策略

```
策略 1: 重新部署 (Rehost)
  → 直接在新云部署相同配置
  → 最简单，但无法利用云原生特性

策略 2: 重新平台化 (Replatform)
  → 使用新云的托管服务
  → 如: EBS → GCP Persistent Disk

策略 3: 重构 (Refactor)
  → 利用新云特有服务
  → 如: 使用 GCP Cloud Spanner
```

## 迁移检查清单

```
□ 应用配置抽取 (ConfigMap/Secret)
□ 数据迁移计划 (Velero / 数据库迁移)
□ 网络规划 (VPC / CIDR / 防火墙)
□ 身份认证迁移 (IAM / RBAC)
□ DNS 切换计划
□ 回滚方案
□ 监控和告警重建
□ 成本对比验证
```

## 工具链

| 任务 | 工具 |
|------|------|
| 配置转换 | kustomize, [[entities/helm.md|helm]] |
| 数据迁移 | Velero, 数据库原生工具 |
| 网络测试 | iperf, curl |
| 验证 | k6, 自动化测试 |

## 相关 Domain

- 云厂商/01-aws-eks/01-eks-migration-guide
- 云厂商/02-google-cloud-gke/01-gke-migration-guide
## Related

- [[故障诊断/topic-structural-trouble-shooting/08-cluster-operations/03-helm-troubleshooting.md|Helm 部署故障排查指南 [topic-structural-trouble-shooting]]]
- [[skills/helm-fta.md|Helm 发布异常故障树分析 (skills)]]


<!-- risk-assessed -->
