---
title: reliability
description: 可靠性工程标签枢纽 — 涵盖 SLO/SLI、混沌工程、灾难恢复、容量规划、事后复盘、性能测试、备份恢复等全部可靠性领域知识
category: tag-index
tags:
- reliability
- slo
- chaos-engineering
- disaster-recovery
- capacity-planning
- resilience
tier: core
difficulty: intermediate-to-advanced
domain: reliability
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-11'
last_updated: '2026-07-21'
---

# reliability Tag Hub

> 可靠性领域页面 — SLO、混沌工程、灾备、容量规划、事后复盘、性能测试等。

## 核心定义

**可靠性工程（Reliability Engineering）** 是确保系统在约定服务水平内持续可用、可恢复的系统化实践。它通过 SLO 驱动决策、混沌工程验证韧性、灾难恢复保障业务连续性。

### 可靠性核心能力

| 能力域 | 描述 | 关键工具/方法 |
|--------|------|---------------|
| SLO 工程 | 定义、度量、决策 | Prometheus + Sloth |
| 混沌工程 | 主动注入故障验证韧性 | Chaos Mesh, LitmusChaos |
| 灾难恢复 | 备份、切换、恢复 | Velero, etcd backup |
| 容量规划 | 资源预测与扩缩容 | 历史数据 + 预测模型 |
| 事后复盘 | 无责复盘与改进 | Blameless Postmortem |
| 性能测试 | 负载、压力、耐久测试 | k6, Locust, wrk2 |
| 备份恢复 | 数据保护与恢复演练 | Velero, VolumeSnapshot |

### 可靠性指标体系

| 指标 | 定义 | 典型目标 |
|------|------|----------|
| 可用性 (Availability) | 系统可用时间比例 | 99.9% (年停机 < 8.76h) |
| RTO | 恢复时间目标 | < 30 分钟 |
| RPO | 恢复点目标 | < 5 分钟 |
| MTTR | 平均修复时间 | < 15 分钟 |
| MTBF | 平均故障间隔 | > 720 小时 |

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
- [[概念/research-2025-2026/10-Reliability-Engineering|可靠性工程深度研究]]

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

## 可靠性工程全景

### 可靠性核心概念

| 概念 | 说明 |
|---|---|
| 可用性 | 系统正常运行时间比例 |
| 持久性 | 数据不丢失的能力 |
| 容错性 | 部分故障不影响整体 |
| 可恢复性 | 故障后快速恢复 |

### 可靠性设计原则

1. **冗余设计**：多副本、多可用区、多区域
2. **故障隔离**：舱壁模式、熔断器、限流
3. **优雅降级**：核心功能优先、非核心可降级
4. **快速恢复**：自动故障转移、备份恢复、混沌演练

## 面试要点

1. **Q：如何设计高可用架构？**
   A：消除单点、多副本、健康检查、自动故障转移、跨 AZ 部署。

2. **Q：RTO/RPO 的含义？**
   A：RTO：恢复时间目标。RPO：恢复点目标(数据丢失量)。根据业务重要性设定。

3. **Q：混沌工程的价值？**
   A：主动发现弱点、验证容错能力、提升团队信心、完善应急预案。

## Related Tags

- [[标签/k8s|k8s]]
- [[标签/observability|observability]]
- [[标签/production|production]]
- [[标签/best-practices|best-practices]]
