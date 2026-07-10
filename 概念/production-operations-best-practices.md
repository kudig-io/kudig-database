---
title: Production Operations Best Practices
description: Production Operations Best Practices — Kubernetes 生产运维知识库
summary: Production Operations Best Practices — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- production
- sre
- operations
- capacity-planning
- change-management
- prometheus
- falco
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
- Production Operations Best Practices 是什么
- 如何 Production Operations Best Practices
trigger_keywords:
- Production
- Operations
- Best
- Practices
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[实体/k8s-production-operations.md|Production Operations]]ernetes 生产环境运维最佳实践字典|Operations Best Practices]]

## Production Readiness Checklist

- [ ] HA architecture deployed (minimum 3 control plane nodes)
- [ ] Complete monitoring and alerting system (99.9% coverage)
- [ ] Regular backup and recovery verification (monthly drills)
- [ ] Security compliance baseline check (CIS benchmark passed)
- [ ] Cost governance mechanism established (budget alerts set)
- [ ] Disaster recovery plan complete (RTO < 4 hours, RPO < 15 minutes)

## SLI/SLO Reference Targets

| Availability Metric | Target |
|--------------------|--------|
| API Server availability | 99.95% |
| Node availability | 99.9% |
| Pod scheduling success rate | 99.5% |

| Performance Metric | Target |
|--------------------|--------|
| API Server P99 latency | < 1 second |
| Pod startup time | < 30 seconds |
| Network latency | < 10ms |

| Capacity Metric | Target |
|----------------|--------|
| Resource utilization | 60-80% |
| Cost deviation | < 10% |

## SRE Practices

**SLI/SLO/Error Budget**: Define Service Level Indicators (what to measure), Service Level Objectives (targets), and Error Budgets (allowed failure time). When error budget is exhausted, halt feature deployments and focus on reliability.

**Blameless Post-Mortems**: After every incident, conduct a blameless post-mortem within 48 hours. Document root cause, timeline, and preventive actions. Update runbooks and detection rules.

**Incident Response Flow**:
1. Detection: Alert from monitoring (Prometheus, Falco, Trivy)
2. Triage: Classify severity (Critical/High/Medium/Low), determine blast radius
3. Containment: Isolate affected workloads, scale down compromised deployments
4. Remediation: Deploy fixes, rotate credentials, update detection rules
5. Post-incident: Blameless post-mortem, document, share lessons learned

## Change Management

RFC (Request for Change) process for production changes:
- Document change scope, risk assessment, rollback plan
- Use gray release / canary deployment for gradual rollout
- Monitor metrics during rollout, auto-rollback on failure
- Post-change verification and documentation

## Capacity Planning and Forecasting

- Monitor resource utilization trends (CPU, memory, storage, network)
- Set utilization alert thresholds (warning at 70%, critical at 85%)
- Plan capacity 3-6 months ahead based on growth trajectory
- Maintain 20% headroom for burst capacity

## Related

- radius — radius
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/microservice-resilience-patterns.md|microservice-resilience-patterns]] — Microservice Resilience Patterns
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/gitops-principles.md|GitOps Principles]]
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[概念/microservice-resilience-patterns.md|Microservice Resilience Patterns]]
- [[技能/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[技能/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[技能/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[技能/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[技能/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]]
- [[技能/Agent Orchestration Patterns.md|Agent Orchestration Patterns]]

- 17-production-operations-best-practices

<!-- risk-assessed -->
