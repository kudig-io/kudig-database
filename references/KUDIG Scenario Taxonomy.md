---
title: KUDIG Scenario Taxonomy
description: KUDIG Scenario Taxonomy — Kubernetes 生产运维知识库
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
created: "2026-05-23"
---

# KUDIG Scenario Taxonomy

## Purpose

Organizes knowledge by **production scenario** rather than document structure. Enables Agent intent routing and supports on-call engineers who think in terms of "what am I trying to do" rather than "which domain doc covers this."

## Scenario Classification

| ID | Scenario | Entry Point | Covered Domains | Est. Docs |
|----|----------|-------------|-----------------|-----------|
| SC-01 | Cluster Deployment | topic-scenarios/cluster-deployment.md | domain-1, domain-4, domain-07-platform-engineering | ~20 |
| SC-02 | Application Deployment | topic-scenarios/app-deployment.md | domain-4, domain-18-manifests-patterns | ~30 |
| SC-03 | Troubleshooting | topic-scenarios/troubleshooting.md | domain-12, topic-fta, topic-skills | ~100 |
| SC-04 | Performance Tuning | topic-scenarios/performance-tuning.md | domain-1, domain-13, domain-11-production-operations | ~25 |
| SC-05 | Security Hardening | topic-scenarios/security-hardening.md | domain-7, domain-25, domain-05-security-compliance | ~30 |
| SC-06 | Monitoring & Alerting | topic-scenarios/monitoring-alerting.md | domain-8, domain-20, domain-06-observability | ~30 |
| SC-07 | Backup & Restore | topic-scenarios/backup-restore.md | domain-30, domain-3, topic-fta | ~20 |
| SC-08 | Upgrade & Migration | topic-scenarios/upgrade-migration.md | domain-1, topic-migration | ~25 |
| SC-09 | Daily Operations | topic-scenarios/daily-ops.md | domain-9, topic-skills | ~40 |
| SC-10 | AI Infrastructure | topic-scenarios/ai-infra-ops.md | domain-11, topic-ai-agent | ~30 |
| SC-11 | Network Diagnosis | topic-scenarios/network-diagnosis.md | domain-5, domain-03-networking-traffic | ~25 |
| SC-12 | Storage Issues | topic-scenarios/storage-issues.md | domain-6, domain-04-storage-data | ~20 |
| SC-13 | Security Incident Response | topic-scenarios/security-incident.md | domain-7, domain-25, domain-05-security-compliance | ~15 |
| SC-14 | Capacity Planning | topic-scenarios/capacity-planning.md | domain-18, domain-07-platform-engineering | ~15 |
| SC-15 | GitOps Workflow | topic-scenarios/gitops-workflow.md | domain-23, domain-08-release-change-management | ~20 |
| SC-16 | Service Mesh Operations | topic-scenarios/mesh-ops.md | domain-03-networking-traffic | ~15 |
| SC-17 | Multi-Cluster Management | topic-scenarios/multi-cluster.md | domain-9, domain-12-cloud-providers | ~15 |
| SC-18 | Edge Operations | topic-scenarios/edge-ops.md | domain-15-specialized-tech | ~10 |
| SC-19 | Cost Optimization | topic-scenarios/cost-optimization.md | domain-18, domain-07-platform-engineering | ~10 |
| SC-20 | Compliance & Audit | topic-scenarios/compliance-audit.md | domain-25, domain-05-security-compliance | ~10 |

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
| Cluster Deployment | domain-01-cluster-fundamentals/12-cluster-deployment-patterns.md | domain-07-platform-engineering/*, domain-08-release-change-management/topic-deployment/* |
| Application Deployment | domain-02-workloads-applications/* | domain-18-manifests-patterns/* |
| Troubleshooting | domain-10-troubleshooting-diagnostics/* | domain-10-troubleshooting-diagnostics/topic-fta/list/*, domain-10-troubleshooting-diagnostics/topic-skills/* |
| Performance Tuning | domain-01-cluster-fundamentals/13-performance-tuning-guide.md | domain-11-production-operations/* |
| Security Hardening | domain-05-security-compliance/* | domain-05-security-compliance/*, domain-05-security-compliance/* |
| Monitoring & Alerting | domain-06-observability/* | domain-06-observability/*, domain-06-observability/* |
| Backup & Restore | domain-01-cluster-fundamentals/* (etcd) | domain-09-reliability-engineering/* |
| Upgrade & Migration | domain-01-cluster-fundamentals/07,18-upgrade* | domain-08-release-change-management/topic-migration/* |
| Daily Operations | domain-07-platform-engineering/* | domain-10-troubleshooting-diagnostics/topic-skills/* |
| AI Infrastructure | domain-14-ai-ml-infra/* | domain-14-ai-ml-infra/topic-ai-agent/* |

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
  -> Primary docs: domain-08-release-change-management/*
```

## Related

- [[INDEX]] — Wiki Index
- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[deployment]] — Deployment
- [[etcd]] — etcd
- [[entities/argocd.md|argocd]] — ArgoCD
- [[references/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[references/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]
- [[docs/SCENARIO-TAXONOMY.md|KUDIG 场景分类体系]]
