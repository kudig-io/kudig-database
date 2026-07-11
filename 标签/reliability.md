---
title: reliability
description: All pages tagged with reliability
category: tag-index
tags:
- reliability
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
---

# reliability Tag Hub

> 可靠性领域页面 — SLO、混沌工程、灾备、容量规划、事后复盘、性能测试等。

## SRE 实践 (SRE Practices)

- [[可靠性/SRE实践/01-availability-calculation-model|可用性计算模型]]
- [[可靠性/SRE实践/02-release-gate-slo-based|基于 SLO 的发布门禁]]
- [[可靠性/SRE实践/03-incident-command-system|事件指挥系统]]
- [[可靠性/SRE实践/04-toil-reduction-automation|减少琐事自动化]]
- [[可靠性/SRE实践/05-error-budget-automation|错误预算自动化]]
- [[可靠性/SRE实践/06-slo-dashboard-design|SLO 仪表盘设计]]
- [[可靠性/SRE实践/07-incident-command-field-guide|事件指挥现场指南]]

## 混沌工程 (Chaos Engineering)

- [[可靠性/混沌工程/01-chaos-engineering-overview|混沌工程概览]]
- [[可靠性/混沌工程/02-chaos-mesh-deployment|Chaos Mesh 部署]]
- [[可靠性/混沌工程/03-chaos-experiment-design|混沌实验设计]]
- [[可靠性/混沌工程/04-litmus-practices|Litmus 实践]]
- [[可靠性/混沌工程/05-chaos-experiment-automation|混沌实验自动化]]
- [[可靠性/混沌工程/06-game-day-runbook-template|Game Day Runbook 模板]]
- [[可靠性/混沌工程/07-blast-radius-control|爆炸半径控制]]

## 容量规划 (Capacity Planning)

- [[可靠性/容量规划/01-capacity-planning-framework|容量规划框架]]
- [[可靠性/容量规划/02-hpa-vpa-cluster-autoscaler-karpenter|HPA/VPA/Cluster Autoscaler/Karpenter]]
- [[可靠性/容量规划/03-resource-quota-limitrange|ResourceQuota/LimitRange]]
- [[可靠性/容量规划/06-autoscaling-best-practices|自动伸缩最佳实践]]
- [[可靠性/容量规划/07-resource-right-sizing-guide|资源合理配置指南]]
- [[可靠性/容量规划/24-capacity-planning-forecasting|容量规划预测]]
- [[可靠性/容量规划/25-ai-driven-capacity-planning-cost-optimization-2025|AI 驱动容量规划与成本优化]]

## 灾难恢复 (Disaster Recovery)

- [[可靠性/灾难恢复/01-multi-region-dr-architecture|多区域灾备架构]]
- [[可靠性/灾难恢复/02-dr-automation-playbook|灾备自动化 Playbook]]
- [[可靠性/灾难恢复/07-kubernetes-backup-restore-deep-dive|Kubernetes 备份恢复深度指南]]
- [[可靠性/灾难恢复/08-chaos-engineering-platforms|混沌工程平台]]
- [[可靠性/灾难恢复/10-dr-scenarios-catalog|灾备场景目录]]
- [[可靠性/灾难恢复/11-az-failure-playbook|AZ 故障 Playbook]]
- [[可靠性/灾难恢复/13-etcd-corruption-recovery-playbook|etcd 损坏恢复 Playbook]]
- [[可靠性/灾难恢复/14-node-failure-bulk-recovery-playbook|节点批量故障恢复 Playbook]]
- [[可靠性/灾难恢复/15-cluster-upgrade-failure-rollback-playbook|集群升级失败回滚 Playbook]]
- [[可靠性/灾难恢复/16-control-plane-loss-recovery-playbook|控制平面丢失恢复 Playbook]]
- [[可靠性/灾难恢复/17-disaster-recovery-drills|灾备演练]]
- [[可靠性/灾难恢复/18-cross-region-disaster-recovery|跨区域灾备]]
- [[可靠性/灾难恢复/99-velero-backup-recovery-guide|Velero 备份恢复指南]]

## 备份恢复 (Backup & Restore)

- [[可靠性/备份恢复/16-enterprise-backup-strategy|企业级备份策略]]

## 事后复盘 (Postmortem)

- [[可靠性/事后复盘/01-blameless-postmortem-template|无指责复盘模板]]
- [[可靠性/事后复盘/02-postmortem-culture-guide|复盘文化指南]]

## 性能测试 (Performance Testing)

- [[可靠性/性能测试/01-load-testing-methodology|负载测试方法论]]
- [[可靠性/性能测试/02-chaos-load-integration|混沌负载集成]]
- [[可靠性/性能测试/02-k6-load-testing-k8s|K6 K8s 负载测试]]
- [[可靠性/性能测试/03-locust-distributed-testing|Locust 分布式测试]]
- [[可靠性/性能测试/04-production-load-testing-playbook|生产负载测试 Playbook]]

## 概念 (Concepts)

- [[概念/slo-error-budget-framework|SLO 错误预算框架]]
- [[概念/chaos-engineering-observability|混沌工程可观测性]]
- [[概念/chaos-engineering-platforms|混沌工程平台]]
- [[概念/incident-management-patterns|事件管理模式]]
- [[概念/K8s 故障分布与 MTTR 基准|K8s 故障分布与 MTTR 基准]]
- [[概念/velero-disaster-recovery|Velero 灾难恢复]]
- [[概念/Research: Kubernetes Reliability Engineering 2025-2026|可靠性工程深度研究]]

## 清单模式 (Manifest Patterns)

- [[清单模式/07-resilience-patterns/01-pdb-patterns|PDB 模式]]
- [[清单模式/07-resilience-patterns/02-hpa-advanced-patterns|HPA 高级模式]]
- [[清单模式/07-resilience-patterns/03-vpa-patterns|VPA 模式]]
- [[清单模式/07-resilience-patterns/04-karpenter-nodepool-patterns|Karpenter NodePool 模式]]
- [[清单模式/07-resilience-patterns/05-health-probe-patterns|健康探针模式]]
- [[清单模式/07-resilience-patterns/06-graceful-shutdown|优雅关闭]]

## FTA 故障树 (Fault Tree Analysis)

- [[故障诊断/FTA故障树/glossary/reliability|可靠性术语]]
- [[故障诊断/FTA故障树/glossary/fault-tree-analysis|故障树分析]]
- [[故障诊断/FTA故障树/glossary/availability|可用性]]
- [[故障诊断/FTA故障树/glossary/mttr|MTTR]]
- [[故障诊断/FTA故障树/glossary/mtbf|MTBF]]

## 可观测性 SLO/SLI

- [[可观测性/SLO-SLI/01-slo-engineering-practice|SLO 工程实践]]
- [[可观测性/SLO-SLI/05-slo-implementation-guide|SLO 实现指南]]
- [[可观测性/SLO-SLI/06-error-budget-management|错误预算管理]]
- [[可观测性/SLO-SLI/07-burn-rate-alerting|燃尽率告警]]

## 生产运维 (Production Operations)

- [[生产运维/06-sla-slo-definition-templates|SLA/SLO 定义模板]]
- [[可靠性/04-error-budget-policy-template|错误预算策略模板]]
- [[可靠性/05-reliability-maturity-model|可靠性成熟度模型]]

## 研究 (Research)

- [[研究/chaos-engineering-practice|混沌工程实践]]

## Related Tags

- [[标签/k8s|k8s]]
- [[标签/observability|observability]]
- [[标签/production|production]]
- [[标签/best-practices|best-practices]]
