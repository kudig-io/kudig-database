---
title: Wiki Tag Taxonomy
description: 生产环境维度整合后的 20 个核心 Domain，按运维职能分层。
category: references
tags:
- taxonomy
- tags
- metadata
- docker
- gateway
- ebpf
- llm
- rag
- agent
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Wiki Tag Taxonomy 是什么
- 如何 Wiki Tag Taxonomy
trigger_keywords:
- Wiki
- Tag
- Taxonomy
prerequisites:
- kubectl-basics
- ebpf-basics
---

# Tag Taxonomy

Canonical tags for the wiki. Use only these tags in frontmatter.
New tags require discussion and addition to this file.

## Core

- `ai` — Artificial Intelligence (general)
- `ml` — Machine Learning
- `dl` — Deep Learning
- `nlp` — Natural Language Processing
- `cv` — Computer Vision
- `rl` — Reinforcement Learning
- `llm` — Large Language Models
- `agents` — AI Agents & Agentic Systems
- `diffusion` — Diffusion Models & Generative AI
- `safety` — AI Safety & Alignment
- `software-engineering` — Software development practices
- `performance` — Benchmarking & optimization
- `dev-tooling` — Developer tools & workflows

## SRE & Reliability

- `sre` — Site Reliability Engineering
- `slo` — Service Level Objectives
- `sli` — Service Level Indicators
- `error-budget` — Error budget management
- `burn-rate` — Burn rate alerting
- `chaos-engineering` — Chaos engineering
- `postmortem` — Blameless postmortem
- `game-day` — GameDay / chaos drill
- `load-testing` — Load & performance testing
- `disaster-recovery` — Disaster recovery & DR playbooks
- `incident-management` — Incident response & command
- `toil` — Toil reduction & automation

## Data & Messaging

- `message-queue` — Message queues (Kafka, Pulsar, NATS)
- `time-series` — Time-series databases
- `tsdb` — TSDB internals (Prometheus, InfluxDB)
- `operator` — Kubernetes operators
- `cdc` — Change Data Capture
- `stream-processing` — Stream processing frameworks
- `data-streaming` — Data streaming pipelines
- `database` — Database systems & management

## Source Types

- `paper` — Academic paper
- `survey` — Survey or review paper
- `benchmark` — Benchmark or evaluation
- `method` — New method or algorithm
- `dataset` — Dataset description
- `specification` — Standard, RFC, or technical specification

## Entity Types

- `person` — Individual (researcher, engineer, etc.)
- `organization` — Company, lab, university
- `project` — Named project or initiative
- `tool` — Software tool or library

---

*Add domain-specific tags below this line after vault setup.*

## Domain Taxonomy (2026-05-21 更新)

生产环境维度整合后的 20 个核心 Domain，按运维职能分层。

### Tier 1 — 核心技术域

- `domain/cluster-fundamentals` — 集群架构、设计原则、控制平面
- `domain/workloads-applications` — 工作负载、应用部署
- `domain/networking-traffic` — 网络、Service Mesh、Gateway、eBPF
- `domain/storage-data` — 存储体系、数据管理
- `domain/security-compliance` — 安全、合规、供应链
- `domain/observability` — 监控、日志、链路追踪、告警

### Tier 2 — 平台与工程域

- `domain/platform-engineering` — IDP、平台运维、DevEx
- `domain/release-change-management` — GitOps、IaC、变更管理
- `domain/reliability-engineering` — SRE、SLO/SLI、混沌工程、灾备、事后复盘

### Tier 3 — 运维场景域

- `domain/troubleshooting-diagnostics` — 排障、FTA、诊断
- `domain/production-operations` — FinOps、事件响应、治理

### Tier 4 — 部署与生态域

- `domain/cloud-providers` — 多云厂商、混合部署
- `domain/container-runtime` — [[entities/docker|docker]]、镜像管理
- `domain/ai-ml-infra` — AI 基础设施
- `domain/specialized-tech` — 边缘计算、WebAssembly、扩展
- `domain/database-middleware` — 数据库、中间件

### Tier 5 — 基础与参考域

- `domain/system-foundation` — Linux、硬件、事件
- `domain/manifests-patterns` — YAML 参考
- `domain/landscape-references` — CNCF、论文
- `domain/application-patterns` — 业务架构参考

---

*原 43 个 Domain 已整合为 20 个。历史映射见 `_reports/domain-migration-EXECUTED-2026-05-21.md`。*
