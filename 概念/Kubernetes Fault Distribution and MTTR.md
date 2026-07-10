---
title: Kubernetes Fault Distribution and MTTR Benchmarks
description: Kubernetes Fault Distribution and MTTR Benchmarks — Kubernetes 生产运维知识库
summary: Kubernetes Fault Distribution and MTTR Benchmarks — Kubernetes 生产运维知识库
category: synthesis
tags:
- k8s
- reliability
- benchmarks
- mttr
- statistics
- etcd
- scheduler
- prometheus
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Fault Distribution and MTTR Benchmarks 是什么
- 如何 Kubernetes Fault Distribution and MTTR Benchmarks
trigger_keywords:
- Kubernetes
- Fault
- Distribution
- and
- MTTR
- Benchmarks
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
relationships:
- target: '[[概念/etcd Operational Reference.md]]'
  type: uses
- target: '[[技能/Kubernetes FTA Top Events Index.md]]'
  type: uses
- target: '[[系统基础/知识字典/workloads/pods.md]]'
  type: uses
- target: '[[技能/best-practices/scenarios/capacity-planning.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Fault Distribution and MTTR Benchmarks

## Industry Fault Distribution

Based on production data across the industry:

| Category | Share | MTTR | Diagnostic Difficulty | FTA Priority |
|----------|-------|------|----------------------|--------------|
| **App config error** | 35% | 45m | Medium | High - most frequent |
| **Resource exhaustion** | 22% | 30m | Low-Medium | High - common |
| **Network issues** | 18% | 60m | High | High - hard to diagnose |
| **Control plane** | 10% | 90m | Very High | Critical - longest MTTR |
| **Storage** | 8% | 75m | High | Medium - infrequent but slow |
| **Security/auth** | 5% | 40m | Medium | Medium |
| **Other** | 2% | 50m | Variable | Low |

## Key Insights

1. **App config errors dominate at 35%**: FTA should expand configuration-related bottom events. Most YAML errors, resource limit misconfigurations, and missing ConfigMap/Secret references fall here.

2. **Control plane has longest MTTR at 90 minutes**: Though only 10% of incidents, control plane failures (etcd, API Server) require the deepest FTA coverage due to their severity and diagnostic complexity.

3. **Network issues are hardest to diagnose**: 60-minute MTTR despite only 18% frequency indicates significant diagnostic challenge. FTA needs extensive diagnostic branching for DNS, CNI, and policy-related failures.

4. **Resource exhaustion is most automatable**: 30-minute MTTR with clear observable signals (memory > 95%, disk > 90%) makes this the best candidate for [[技能/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]].

## etcd FMEA Risk Priority Numbers

| Failure Mode | Severity (S) | Occurrence (O) | Detection (D) | RPN |
|-------------|:---:|:---:|:---:|:---:|
| Disk space exhausted | 9 | 5 | 3 | **135** |
| Quorum lost | 10 | 3 | 4 | **120** |
| Data corruption | 10 | 2 | 6 | **120** |
| High response latency | 7 | 6 | 2 | **84** |
| Version incompatibility | 8 | 2 | 5 | **80** |
| Certificate expiry | 9 | 4 | 2 | **72** |

> RPN = S x O x D. Values > 100 require focused attention.

## Diagnostic Time Breakdown

| Phase | Typical Time | Optimization |
|-------|-------------|--------------|
| Detection | 1-5 min | Prometheus alerts, symptom vector matching |
| Diagnosis | 5-60 min | FTA-guided path, automated evidence collection |
| Remediation | 2-30 min | Pre-approved runbooks, auto-healing actions |
| Verification | 2-10 min | Automated health checks, SLO validation |

## Top Events by Business Impact

| Top Event | Severity | Frequency | Business Impact |
|-----------|----------|-----------|-----------------|
| TE-1: Cluster unavailable | P0 | Rare | Total service outage |
| TE-2: App unavailable | P0 | Common | Revenue impact |
| TE-3: Pod startup failure | P1 | Common | Deployment blocked |
| TE-4: Network anomaly | P1 | Common | Partial service degradation |
| TE-5: Storage failure | P1 | Uncommon | Data access blocked |
| TE-15: DR failure | P0 | Rare | Business continuity risk |

## [[技能/best-practices/scenarios/capacity-planning.md|Capacity Planning]] Benchmarks

- etcd: 10,000+ writes/second on SSD
- API Server: Handles 1,000+ req/sec per instance
- kube-scheduler: Schedules 100+ [[系统基础/知识字典/workloads/pods.md|pods]]/sec
- Typical cluster: 500-5,000 pods per cluster
- Typical node: 50-110 pods per node (varies by instance type)

## Related

- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[技能/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[概念/etcd Operational Reference.md|etcd Operational Reference]].md|etcd Operational Reference]]
- [[概念/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]
- [[实体/dex.md|Dex (entities)]]


<!-- risk-assessed -->
