---
title: FinOps & Cost Governance
description: 成本治理知识域 — 成本分摊/Chargeback、资源 Right-Sizing、Spot 实例策略、K8s 成本治理
category: subdomain
tags:
- finops
- cost-optimization
- right-sizing
- spot-instance
- chargeback
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 成本治理 FinOps

> Kubernetes 成本可视化、优化与治理的全链路实践。

## FinOps 成熟度模型

| 阶段 | 活动 | 目标 |
|------|------|------|
| Inform | 成本可视化/分摊 | 知道钱花在哪 |
| Optimize | Right-Sizing/Spot | 减少浪费 |
| Operate | 自动化治理/预算 | 持续优化 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[13-生产运维/01-成本治理/01-cost-allocation-chargeback.md\|成本分摊]] | 按团队/项目 Chargeback | intermediate |
| [[13-生产运维/01-成本治理/02-idle-resource-right-sizing.md\|Right-Sizing]] | 闲置资源识别与调整 | intermediate |
| [[13-生产运维/01-成本治理/03-spot-instance-strategy.md\|Spot 实例]] | 抢占式实例策略 | advanced |
| [[13-生产运维/01-成本治理/05-kubernetes-cost-governance.md\|K8s 成本治理]] | 集群级成本管控 | advanced |
| [[13-生产运维/01-成本治理/06-finops-cost-governance-runbook.md\|FinOps Runbook]] | 成本治理操作手册 | advanced |
| [[13-生产运维/01-成本治理/07-finops-cost-optimization-guide.md\|FinOps 指南]] | 完整成本优化指南 | advanced |

## 成本优化工具

| 工具 | 能力 |
|------|------|
| Kubecost | K8s 成本可视化与分摊 |
| OpenCost | 开源成本监控 |
| Goldilocks | VPA 推荐 Right-Sizing |
| StormForge | AI 驱动资源优化 |

## Related

- [[13-生产运维/02-集群治理/index.md|集群治理]]
- [[13-生产运维/04-绿色计算/index.md|绿色计算]]
