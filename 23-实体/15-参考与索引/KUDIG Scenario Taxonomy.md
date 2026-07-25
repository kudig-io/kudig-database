---
title: KUDIG Scenario Taxonomy
description: KUDIG Scenario Taxonomy — Kubernetes 生产运维知识库
summary: KUDIG Scenario Taxonomy — Kubernetes 生产运维知识库
category: reference
tags:
- k8s
- scenarios
- taxonomy
- classification
- etcd
- argocd
- rag
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Scenario Taxonomy 是什么
- 如何 KUDIG Scenario Taxonomy
trigger_keywords:
- KUDIG
- Scenario
- Taxonomy
prerequisites:
- kubectl-basics
- gitops-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG Scenario Taxonomy

## Purpose

Organizes knowledge by **production scenario** rather than document structure. Enables Agent intent routing and supports on-call engineers who think in terms of "what am I trying to do" rather than "which domain doc covers this."

## Scenario Classification

| ID | Scenario | Entry Point | Covered Domains | Est. Docs |
|----|----------|-------------|-----------------|-----------|
| SC-01 | Cluster Deployment | topic-scenarios/cluster-deployment.md | domain-1, domain-4, 平台工程 | ~20 |
| SC-02 | Application Deployment | topic-scenarios/app-deployment.md | domain-4, 清单模式 | ~30 |
| SC-03 | Troubleshooting | topic-scenarios/troubleshooting.md | domain-12, topic-fta, topic-skills | ~100 |
| SC-04 | Performance Tuning | topic-scenarios/performance-tuning.md | domain-1, domain-13, 生产运维 | ~25 |
| SC-05 | Security Hardening | topic-scenarios/security-hardening.md | domain-7, domain-25, 安全 | ~30 |
| SC-06 | Monitoring & Alerting | topic-scenarios/monitoring-alerting.md | domain-8, domain-20, 可观测性 | ~30 |
| SC-07 | Backup & Restore | topic-scenarios/backup-restore.md | domain-30, domain-3, topic-fta | ~20 |
| SC-08 | Upgrade & Migration | topic-scenarios/upgrade-migration.md | domain-1, topic-migration | ~25 |
| SC-09 | Daily Operations | topic-scenarios/daily-ops.md | domain-9, topic-skills | ~40 |
| SC-10 | AI Infrastructure | topic-scenarios/ai-infra-ops.md | domain-11, 02-ai-agents | ~30 |
| SC-11 | Network Diagnosis | topic-scenarios/network-diagnosis.md | domain-5, 网络 | ~25 |
| SC-12 | Storage Issues | topic-scenarios/storage-issues.md | domain-6, 存储 | ~20 |
| SC-13 | Security Incident Response | topic-scenarios/security-incident.md | domain-7, domain-25, 安全 | ~15 |
| SC-14 | Capacity Planning | topic-scenarios/capacity-planning.md | domain-18, 平台工程 | ~15 |
| SC-15 | GitOps Workflow | topic-scenarios/gitops-workflow.md | domain-23, 发布变更 | ~20 |
| SC-16 | Service Mesh Operations | topic-scenarios/mesh-ops.md | 网络 | ~15 |
| SC-17 | Multi-Cluster Management | topic-scenarios/multi-cluster.md | domain-9, 云厂商 | ~15 |
| SC-18 | Edge Operations | topic-scenarios/edge-ops.md | 专项技术 | ~10 |
| SC-19 | Cost Optimization | topic-scenarios/cost-optimization.md | domain-18, 平台工程 | ~10 |
| SC-20 | Compliance & Audit | topic-scenarios/compliance-audit.md | domain-25, 安全 | ~10 |

## Scenario Page Structure

Each scenario page should contain:

1. **Scenario Overview**: Goal, trigger conditions, impact scope
2. **Quick Decision Tree**: Mermaid decision graph, 3-step diagnosis
3. **Related Document Index**: Links ordered by diagnostic priority
4. **Runbook**: Directly executable commands/steps
5. **Escalation Path**: When to escalate and to whom

## Scenario to Document Mapping

| Scenario | Primary Docs | Secondary Docs |
|----------|-------------|----------------|
| Cluster Deployment | 集群基础/12-cluster-deployment-patterns.md | 平台工程/*, 发布变更/topic-deployment/* |
| Application Deployment | 工作负载/* | 清单模式/* |
| Troubleshooting | 故障诊断/* | 故障诊断/FTA故障树/list/*, 故障诊断/topic-skills/* |
| Performance Tuning | 集群基础/13-performance-tuning-guide.md | 生产运维/* |
| Security Hardening | 安全/* | 安全/*, 安全/* |
| Monitoring & Alerting | 可观测性/* | 可观测性/*, 可观测性/* |
| Backup & Restore | 集群基础/* (etcd) | 可靠性/* |
| Upgrade & Migration | 集群基础/07,18-upgrade* | 发布变更/topic-migration/* |
| Daily Operations | 平台工程/* | 故障诊断/topic-skills/* |
| AI Infrastructure | AI基础设施/* | AI基础设施/02-ai-agents/* |

## Agent Routing

Agents use the scenario taxonomy for intent classification:

```
User input: "My Pod keeps crashing with OOMKilled"
  -> Intent: Troubleshooting (SC-03)
  -> Sub-scenario: Pod CrashLoopBackOff
  -> FTA path: TE-2 -> IE-2.1 -> BE-2.3
  -> Skill: Pod CrashLoopBackOff/OOMKilled diagnostic

User input: "How do I set up ArgoCD for my cluster?"
  -> Intent: GitOps Workflow (SC-15)
  -> Primary docs: 发布变更/*
```

## Related

- [[INDEX]] — Wiki Index
- [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[deployment]] — Deployment
- [[etcd]] — etcd
- [[23-实体/08-交付与制品/argocd.md|argocd]] — ArgoCD
- [[23-实体/15-参考与索引/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[23-实体/15-参考与索引/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]
- [[29-文档/specs/SCENARIO-TAXONOMY.md|KUDIG 场景分类体系]]


<!-- risk-assessed -->
