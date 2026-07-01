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
created: "2026-05-23"
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
- `domain/container-runtime` — [[entities/docker.md|docker]]、镜像管理
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


## KUDIG Domain Tags (2026-05-23)

- `cluster` — Kubernetes cluster fundamentals
- `workloads` — Pods, deployments, statefulsets, jobs
- `networking` — CNI, service mesh, ingress, DNS
- `storage` — PVC, CSI, volumes, object storage
- `security` — RBAC, secrets, TLS, network policies
- `observability` — Monitoring, logging, tracing, alerting
- `reliability` — SRE, chaos engineering, disaster recovery
- `troubleshooting` — Diagnostics, FTA, remediation
- `platform` — Platform engineering, IDP, DevEx
- `release` — GitOps, CI/CD, deployment strategies
- `cloud-providers` — Cloud vendors, multi-cloud, hybrid
- `synthesis` — Cross-domain analysis, case studies
- `cross-domain` — Multi-domain topics
- `remote-consultant` — Remote advisor mode
- `career` — Learning paths, career development

## Content Tags (2026-05-24 补全)

- `index` — 索引页、合并索引、导航入口
- `core-concept` — 核心概念定义与解释
- `incident` — 事件响应、故障处理、On-Call
- `production` — 生产环境运维、FinOps、治理
- `dialogue` — 对话式教学脚本、场景演练
- `navigation` — MOC、README、导航页面
- `k8s` — Kubernetes 通用标签（与 cluster 互换使用）
- `reference` — 参考文档、速查表、配置模板
- `remediation` — 修复方案、修复手册
- `playbook` — 操作手册、标准操作流程
- `uncategorized` — 待分类页面
- `skill` — Agent Skill 定义、能力模块
- `reports` — 质量报告、审计报告、修复报告
- `remote-advisor` — 远程顾问模式内容
- `quality` — 质量保证、测试、验证
- `pod` — Pod 相关（调度、生命周期、调试）
- `training` — 培训材料、课程、讲师指南
- `best-practices` — 最佳实践、设计模式
- `cncf` — CNCF 项目、毕业/孵化/沙箱状态
- `gitops` — GitOps 工作流、ArgoCD、Flux
- `monitoring` — 监控体系（Prometheus、Grafana）
- `logging` — 日志收集与分析（Fluentd、Loki）
- `service-mesh` — Istio、Linkerd、Envoy
- `ingress` — Ingress Controller、Gateway API
- `rbac` — 角色访问控制
- `helm` — Helm Chart 管理
- `etcd` — etcd 运维与调优
- `containerd` — containerd 运行时
- `argocd` — ArgoCD 持续部署
- `prometheus` — Prometheus 监控
- `deployment` — Deployment 策略与管理
