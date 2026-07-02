---
title: KUDIG Knowledge Base Architecture
description: KUDIG Knowledge Base Architecture — Kubernetes 生产运维知识库
summary: KUDIG Knowledge Base Architecture — Kubernetes 生产运维知识库
category: concept
tags:
- k8s
- architecture
- knowledge-base
- fta
- agent
- etcd
- scheduler
- prometheus
- docker
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Knowledge Base Architecture 是什么
- 如何 KUDIG Knowledge Base Architecture
trigger_keywords:
- KUDIG
- Knowledge
- Base
- Architecture
prerequisites:
- kubectl-basics
- prometheus-basics
- ebpf-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG Knowledge Base Architecture

## Overview

The KUDIG-DB is a 3,532-file Kubernetes operations knowledge base designed for both human operators and AI Agents. It spans 40+ knowledge domains from architecture fundamentals to cutting-edge topics like AI infrastructure, eBPF, and [[concepts/platform-engineering-sre.md|platform engineering]].

## Knowledge Layers

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────┐
│  Layer 1: Foundation (Core K8s Knowledge)                    │
│  domain-01-cluster-fundamentals  Architecture Fundamentals                         │
│  domain-01-cluster-fundamentals  Design Principles                                 │
│  domain-01-cluster-fundamentals  Control Plane (etcd, API Server, Scheduler, CM)   │
│  domain-02-workloads-applications  Workloads (Pod, Deployment, StatefulSet, Job)     │
│  domain-03-networking-traffic  Networking (CNI, Service, Ingress, Gateway API)  │
│  domain-04-storage-data  Storage (PV, PVC, CSI)                            │
│  domain-05-security-compliance  Security (RBAC, Network Policy, Runtime)          │
│  domain-06-observability  Observability (Prometheus, Logging, Tracing)      │
│  domain-07-platform-engineering  Platform Operations                               │
├─────────────────────────────────────────────────────────────┤
│  Layer 2: Advanced Operations                                │
│  domain-10-troubleshooting-diagnostics Troubleshooting                                   │
│  domain-13-container-runtime Docker                                            │
│  domain-17-system-foundation Linux                                             │
│  domain-03-networking-traffic Network Fundamentals                              │
│  domain-04-storage-data Storage Fundamentals                              │
│  domain-12-cloud-providers Cloud Provider                                    │
│  domain-11-production-operations Production Operations                             │
├─────────────────────────────────────────────────────────────┤
│  Layer 3: Specialized Domains                                │
│  domain-19-landscape-references Papers           domain-09-reliability-engineering Disaster Recovery      │
│  domain-06-observability Monitoring       domain-17-system-foundation Hardware               │
│  domain-06-observability Logging          domain-18-manifests-patterns YAML Manifests         │
│  domain-03-networking-traffic Container Images domain-17-system-foundation K8s Events             │
│  domain-08-release-change-management GitOps/CI-CD     domain-19-landscape-references CNCF Landscape         │
│  domain-08-release-change-management IaC              domain-03-networking-traffic eBPF Technology        │
│  domain-05-security-compliance Cloud-Native Sec domain-07-platform-engineering Platform Engineering   │
│  domain-03-networking-traffic Service Mesh     domain-15-specialized-tech Edge Computing         │
│  domain-12-cloud-providers Multi-Cloud      domain-15-specialized-tech WebAssembly Cloud-Native│
│  domain-16-database-middleware Database/Middleware domain-05-security-compliance Supply Chain Security│
│  domain-08-release-change-management Automated Testing domain-03-networking-traffic API Gateway           │
├─────────────────────────────────────────────────────────────┤
│  Layer 4: Problem-Solving Engine (KUDIG Differentiator)      │
│  topic-fta      Fault Tree Analysis methodology + 16 top events│
│  topic-skills   19 diagnostic skill cards                    │
│  topic-structural-trouble-shooting  Symptom mapping layer    │
│  topic-scenarios 20 production scenarios                     │
│  topic-cheat-sheet Quick reference cards                     │
├─────────────────────────────────────────────────────────────┤
│  Layer 5: Emerging Topics                                    │
│  02-ai-agents    AI Agent fundamentals, frameworks, deployment│
│  topic-ai-coding   OpenRouter, OpenCode AI coding tools       │
│  topic-application-architecture  96 industry architecture patterns│
│  domain-java-kubernetes  Java on K8s                          │
│  domain-03-networking-traffic  Networking + Terway CNI deep dive      │
│  topic-febm        Forensic Evidence-Based Method            │
│  topic-migration   Migration guides                          │
├─────────────────────────────────────────────────────────────┤
│  Layer 6: Metadata & Infrastructure                          │
│  docs/           Spec documents, dictionaries, domain guides │
│  templates/      Document templates (FTA, skill, cheat sheet)│
│  prompts/        Agent prompt templates                      │
│  man/            Man pages for K8s components                │
└─────────────────────────────────────────────────────────────┘
```
## Problem-Solving Architecture

The KUDIG differentiator is its structured problem-solving engine:

```
Symptom Input Layer (Symptom Vector Matcher)
  - 32-dimensional feature vectors from natural language
  - Cosine similarity matching against known patterns
  - Semantic expansion for colloquial expressions
      ↓
FTA Diagnostic Engine
  - Dynamic probability (time/load/trend/season factors)
  - Intelligent pruning (confidence, evidence contradiction)
  - Bayesian inference for uncertainty
  - Temporal constraint validation
      ↓
Decision Output Layer
  - Root cause confirmation with evidence chain
  - Remediation plan with risk assessment
  - Pre-condition checks and rollback mechanisms
      ↓
Learning Feedback Loop
  - Success/failure probability updates
  - New pattern discovery (PROPOSED state management)
  - Continuous FTA evolution
```

## Version Support

- Kubernetes: v1.25 through v1.32
- Covers standard K8s + ACK (Alibaba Cloud Container Service) extensions
- Includes Terway CNI, ASM (Alibaba Service Mesh), ACK-One multi-cluster

## Related

- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[supply-chain-security]] — Software Supply Chain Security
- [[entities/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[entities/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]
- [[skills/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[entities/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]

- [[README]]

<!-- risk-assessed -->
