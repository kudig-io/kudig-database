---
title: production
description: 生产运营标签枢纽 — 涵盖生产就绪、日常巡检、值班手册、事件响应、变更管理、FinOps、集群治理、绿色计算等全部生产运营知识
category: tag-index
tags:
- production
- operations
- incident-response
- finops
- runbook
- change-management
tier: core
difficulty: intermediate-to-advanced
domain: production-operations
k8s_versions: ["1.28", "1.30", "1.32", "1.34"]
created: '2026-07-11'
last_updated: '2026-07-21'
---

# production Tag Hub

> 生产运营页面 — 生产就绪、日常巡检、值班手册、事件响应、变更管理、FinOps 等。

## 核心定义

**生产运营（Production Operations）** 是确保 Kubernetes 集群及其上运行的业务系统持续稳定、安全、高效运行的系统化实践。它涵盖日常巡检、事件响应、变更管理、容量规划、成本治理、安全运营等多个维度。

### 生产运营核心能力

| 能力域 | 描述 | 关键产出 |
|--------|------|----------|
| 生产就绪 | 上线前的全面评估与检查 | 就绪检查清单 |
| 日常巡检 | 集群健康状态的定期检查 | 巡检报告 |
| 事件响应 | 故障发现、定位、修复、复盘 | Runbook + Postmortem |
| 变更管理 | 变更申请、审批、执行、回滚 | 变更窗口 + 冻结策略 |
| 容量规划 | 资源预测、扩缩容决策 | 容量报告 |
| 成本治理 | 资源利用率优化、费用分配 | FinOps 报告 |
| 安全运营 | 漏洞修复、合规审计、应急响应 | 安全报告 |

### 生产运营成熟度

| 级别 | 特征 | 典型表现 |
|------|------|----------|
| L1 教火式 | 被动响应、无流程 | 故障后才知道 |
| L2 可重复 | 有基本 Runbook | 按手册操作 |
| L3 已定义 | 标准化流程 + 自动化 | 告警自动触发 Runbook |
| L4 已管理 | 度量驱动 + 持续优化 | SLO 驱动决策 |
| L5 优化型 | AIOps + 自愈 | 自动检测、自动修复 |

## 生产运维核心 (Production Ops Core)

- [[生产运维/01-production-sre-daily-ops|生产环境日常巡检与值班手册]]
- [[生产运维/03-on-call-playbook|值班手册与告警响应]]
- [[生产运维/04-incident-response-template|事故响应模板与流程]]
- [[生产运维/05-capacity-planning-readiness|容量规划就绪]]
- [[生产运维/06-multi-cluster-operations|多集群运营]]
- [[生产运维/07-change-freeze-policy|变更冻结策略]]
- [[生产运维/08-security-operations-runbook|安全运营 Runbook]]
- [[生产运维/10-observability-operations|可观测性运营]]
- [[生产运维/13-node-and-runtime-ops|节点与运行时运维]]
- [[生产运维/05-production-runbook-generator|生产 Runbook 编写规范]]
- [[生产运维/99-production-readiness-operations-guide|安全生产就绪指南]]

## 成本治理 (Cost Governance)

- [[生产运维/成本治理/01-cost-allocation-chargeback|成本分配与计费回收]]
- [[生产运维/成本治理/02-idle-resource-right-sizing|空闲资源合理配置]]
- [[生产运维/成本治理/03-spot-instance-strategy|Spot 实例策略]]
- [[生产运维/成本治理/13-kubernetes-cost-governance|Kubernetes 成本治理]]
- [[生产运维/成本治理/99-finops-cost-optimization-guide|FinOps 成本优化指南]]

## 事件响应 (Incident Response)

- [[生产运维/事件响应/01-escalation-matrix-severity-levels|升级矩阵与严重性级别]]
- [[生产运维/事件响应/02-war-room-coordination-procedures|作战室协调流程]]
- [[生产运维/事件响应/05-security-incident-response-playbook|安全事件响应 Playbook]]
- [[生产运维/事件响应/23-incident-response-handling|事件响应处理]]
- [[生产运维/事件响应/24-incident-response-runbook-template|事件响应 Runbook 模板]]

## 集群治理 (Cluster Governance)

- [[生产运维/集群治理/04-rbac-governance-model|RBAC 治理模型]]
- [[生产运维/集群治理/14-resource-quota-management|资源配额管理]]

## 绿色计算 (Green Computing)

- [[生产运维/绿色计算/15-green-computing-sustainability|绿色计算可持续性]]
- [[生产运维/绿色计算/99-greenops-sustainable-computing-guide|GreenOps 可持续计算指南]]

## 生产就绪指南 (Production Readiness)

- [[AI基础设施/99-production-readiness-operations-guide|AI 基础设施生产就绪指南]]
- [[安全/99-production-readiness-operations-guide|安全生产就绪指南]]
- [[存储/99-production-readiness-operations-guide|存储生产就绪指南]]
- [[发布变更/99-production-readiness-operations-guide|发布变更生产就绪指南]]
- [[可观测性/99-production-readiness-operations-guide|可观测性生产就绪指南]]
- [[可靠性/99-production-readiness-operations-guide|可靠性生产就绪指南]]
- [[集群基础/99-production-readiness-operations-guide|集群基础生产就绪指南]]
- [[平台工程/99-production-readiness-operations-guide|平台工程生产就绪指南]]
- [[容器运行时/99-production-readiness-operations-guide|容器运行时生产就绪指南]]
- [[网络/99-production-readiness-operations-guide|网络生产就绪指南]]
- [[专项技术/99-production-readiness-operations-guide|专项技术生产就绪指南]]
- [[云厂商/99-production-readiness-operations-guide|云厂商生产就绪指南]]

## 集群基础 (Cluster Fundamentals)

- [[集群基础/01-production-architecture-design-principles|生产架构设计原则]]
- [[集群基础/99-kubernetes-production-architecture-blueprint|K8s 生产架构蓝图]]
- [[集群基础/控制平面/24-production-deployment-best-practices|生产环境部署最佳实践]]
- [[集群基础/控制平面/34-certificate-pki-lifecycle-runbook|证书 PKI 生命周期 Runbook]]
- [[集群基础/控制平面/35-cluster-upgrade-runbook|集群升级 Runbook]]
- [[集群基础/性能调优/19-cluster-performance-tuning|集群性能调优]]

## 概念 (Concepts)

- [[概念/k8s-production-best-practices|K8s 生产最佳实践]]
- [[概念/production-operations-best-practices|生产运营最佳实践]]
- [[概念/Production Troubleshooting Playbook|生产故障排查 Playbook]]
- [[概念/command-risk-assessment|命令风险分级与安全生产规范]]

## 案例研究 (Case Studies)

- [[概念/case-studies/README|案例研究索引]]
- [[概念/case-studies/2026-01-15-node-notready-pod-eviction|Node NotReady Pod 驱逐]]
- [[概念/case-studies/2026-02-05-etcd-inconsistency-503|etcd 不一致 503]]
- [[概念/case-studies/2026-03-15-oomkilled-java-restart|OOMKilled Java 重启]]
- [[概念/case-studies/2026-07-08-prometheus-high-cardinality-oom|Prometheus 高基数 OOM]]
- [[概念/case-studies/2026-07-20-velero-backup-failure|Velero 备份失败]]

## 可靠性 (Reliability)

- [[可靠性/性能测试/04-production-load-testing-playbook|生产负载测试 Playbook]]
- [[可靠性/灾难恢复/12-disaster-recovery-bc-runbook-v1|灾备 BC Runbook v1]]
- [[可靠性/灾难恢复/21-disaster-recovery-bc-runbook-v2|灾备 BC Runbook v2]]
- [[可靠性/灾难恢复/17-disaster-recovery-drills|灾备演练]]

## 应用模式 (Application Patterns)

- [[应用模式/99-production-readiness-operations-guide|应用模式生产就绪指南]]
- [[应用模式/生产模式/application-security-hardening|应用安全加固]]
- [[应用模式/生产模式/cost-optimization-finops|成本优化 FinOps]]
- [[应用模式/生产模式/stateful-app-patterns|有状态应用模式]]
- [[应用模式/生产模式/progressive-delivery-patterns|渐进式交付模式]]

## 平台工程 (Platform Engineering)

- [[平台工程/99-production-readiness-review-template|生产就绪评估模板]]
- [[平台工程/运维/15-production-troubleshooting|生产环境故障排查]]
- [[平台工程/12-automated-operations-toolchain|自动化运维工具链]]

## 部署方案 (Deployment)

- [[发布变更/部署方案/04-production-environment-deployment|生产环境部署]]
- [[发布变更/部署方案/02-single-node-deployment|单节点部署]]
- [[发布变更/GitOps/08-fleet-gitops-operations-guide|Fleet GitOps 运营]]

## 云厂商 Runbook (Cloud Provider Runbooks)

- [[云厂商/AWS-EKS/99-aws-eks-production-runbook|AWS EKS 生产 Runbook]]
- [[云厂商/Azure-AKS/99-azure-aks-production-runbook|Azure AKS 生产 Runbook]]
- [[云厂商/Google-GKE/99-gke-production-runbook|GKE 生产 Runbook]]
- [[云厂商/阿里云/公有云-ACK/99-alicloud-ack-production-runbook|阿里云 ACK 生产 Runbook]]
- [[云厂商/华为云CCE/99-huawei-cce-production-runbook|华为云 CCE 生产 Runbook]]
- [[云厂商/腾讯云TKE/99-tencent-tke-production-runbook|腾讯云 TKE 生产 Runbook]]

## 专项技术 (Specialized Technologies)

- [[专项技术/03-edge-computing-production-deployment|边缘计算生产部署]]
- [[专项技术/边缘计算/14-edge-production-runbook|边缘生产 Runbook]]
- [[专项技术/WebAssembly/11-wasm-production-deployment|Wasm 生产部署]]

## 数据库中间件 (Database Middleware)

- [[数据库中间件/数据库/15-redis-kubernetes-production-guide|Redis K8s 生产指南]]
- [[数据库中间件/数据库/16-postgresql-kubernetes-production-guide|PostgreSQL K8s 生产指南]]
- [[数据库中间件/数据库/17-mysql-kubernetes-production-guide|MySQL K8s 生产指南]]
- [[数据库中间件/消息队列/06-kafka-kubernetes-production-guide|Kafka K8s 生产指南]]

## 系统基础 (System Foundation)

- [[系统基础/Linux/13-k8s-node-os-image-hardening-baseline|K8s 节点 OS 镜像加固基线]]

## 生产环境全景

### 生产就绪检查清单

| 类别 | 检查项 |
|---|---|
| 高可用 | 多副本、PDB、跨 AZ |
| 可观测 | 监控、日志、追踪、告警 |
| 安全 | RBAC、PSS、网络策略、Secret 加密 |
| 备份 | etcd 备份、PV 快照、恢复演练 |
| 性能 | 资源限制、HPA、缓存 |

### 生产环境关键指标

| 指标 | 目标 |
|---|---|
| 可用性 | 99.9%+ |
| P99 延迟 | <500ms |
| 错误率 | <0.1% |
| 恢复时间 | <5min |

## 面试要点

1. **Q：生产环境的核心要求？**
   A：高可用、可观测、安全、可恢复、性能达标、成本可控。

2. **Q：生产事故应急响应流程？**
   A：发现→评估→止血→修复→验证→复盘。关键是快速止血。

3. **Q：如何保证生产变更安全？**
   A：变更审批、灰度发布、监控验证、回滚预案、变更窗口。

## Related Tags

- [[标签/k8s|k8s]]
- [[标签/reliability|reliability]]
- [[标签/best-practices|best-practices]]
- [[标签/observability|observability]]
- [[标签/security|security]]
