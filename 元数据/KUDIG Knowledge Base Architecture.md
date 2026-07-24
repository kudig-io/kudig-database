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

The KUDIG-DB is a 3,532-file Kubernetes operations knowledge base designed for both human operators and AI Agents. It spans 40+ knowledge domains from architecture fundamentals to cutting-edge topics like AI infrastructure, eBPF, and [[概念/platform-engineering-sre.md|platform engineering]].

## Knowledge Layers

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────┐
│  Layer 1: Foundation (Core K8s Knowledge)                    │
│  集群基础  Architecture Fundamentals                         │
│  集群基础  Design Principles                                 │
│  集群基础  Control Plane (etcd, API Server, Scheduler, CM)   │
│  工作负载  Workloads (Pod, Deployment, StatefulSet, Job)     │
│  网络  Networking (CNI, Service, Ingress, Gateway API)  │
│  存储  Storage (PV, PVC, CSI)                            │
│  安全  Security (RBAC, Network Policy, Runtime)          │
│  可观测性  Observability (Prometheus, Logging, Tracing)      │
│  平台工程  Platform Operations                               │
├─────────────────────────────────────────────────────────────┤
│  Layer 2: Advanced Operations                                │
│  故障诊断 Troubleshooting                                   │
│  容器运行时 Docker                                            │
│  系统基础 Linux                                             │
│  网络 Network Fundamentals                              │
│  存储 Storage Fundamentals                              │
│  云厂商 Cloud Provider                                    │
│  生产运维 Production Operations                             │
├─────────────────────────────────────────────────────────────┤
│  Layer 3: Specialized Domains                                │
│  生态参考 Papers           可靠性 Disaster Recovery      │
│  可观测性 Monitoring       系统基础 Hardware               │
│  可观测性 Logging          清单模式 YAML Manifests         │
│  网络 Container Images 系统基础 K8s Events             │
│  发布变更 GitOps/CI-CD     生态参考 CNCF Landscape         │
│  发布变更 IaC              网络 eBPF Technology        │
│  安全 Cloud-Native Sec 平台工程 Platform Engineering   │
│  网络 Service Mesh     专项技术 Edge Computing         │
│  云厂商 Multi-Cloud      专项技术 WebAssembly Cloud-Native│
│  数据库中间件 Database/Middleware 安全 Supply Chain Security│
│  发布变更 Automated Testing 网络 API Gateway           │
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
│  网络  Networking + Terway CNI deep dive      │
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
- [[实体/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[实体/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]
- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[实体/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]

- [[README]]

<!-- risk-assessed -->
