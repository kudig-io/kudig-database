---
title: Platform Engineering and Internal Developer Platforms
description: '- [[synthesis/IaC x 多集群管理.md|IaC x 多集群管理]] — synthesis'
category: concepts
tags:
- k8s
- platform-engineering
- idp
- backstage
- developer-experience
- golden-paths
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Platform Engineering and Internal Developer Platforms 是什么
- 如何 Platform Engineering and Internal Developer Platforms
trigger_keywords:
- Platform
- Engineering
- and
- Internal
- Developer
- Platforms
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# [[synthesis/platform-engineering-sre.md|Platform Engineering]] and Internal Developer Platforms

## Platform Engineering Definition

Platform Engineering is the discipline of designing and building toolchains and workflows that enable self-[[Service|service]] capabilities for software engineering organizations. Gartner predicts 80% of large software organizations will establish platform teams by 2026.

## Core Concept: Internal Developer Platform (IDP)

An IDP provides developers with self-service capabilities to:
- Create and deploy services using standardized templates
- Access infrastructure resources without filing tickets
- Monitor and troubleshoot their services
- Follow Golden Paths (opinionated, best-practice workflows)

## Key Components

| Component | Purpose | Leading Tools |
|-----------|---------|---------------|
| Developer Portal | Unified self-service interface | Backstage (CNCF Incubating) |
| Software Catalog | Service inventory and metadata | Backstage Catalog |
| Scaffolder | Template-driven service creation | Backstage Scaffolder |
| TechDocs | Documentation-as-code | Backstage TechDocs |
| Platform-as-Code | Declarative platform definition | Kratix |
| Infrastructure Abstraction | Multi-cloud resource management | Crossplane |

## Golden Paths

Golden Paths are opinionated, best-practice workflows that handle common developer needs:
- "Create a new web service" -> Scaffolder generates K8s manifests, CI/CD pipeline, monitoring config
- "Deploy to production" -> Automated canary release with metric-based promotion
- "Add a database" -> Crossplane provisions DB, injects credentials via Vault

Golden Paths are not mandates -- developers can opt out but lose platform support guarantees.

## Platform Maturity Model

| Level | Capability | Description |
|-------|-----------|-------------|
| 1 | Basic Infrastructure | Scripted provisioning, manual processes |
| 2 | Self-Service Portal | Developer portal with basic service creation |
| 3 | Golden Paths | Standardized templates for common scenarios |
| 4 | Platform-as-Code | Declarative platform definition, GitOps managed |
| 5 | Intelligent Platform | AI-assisted development, predictive scaling |

## Developer Experience Metrics

- **DORA Metrics**: Deployment frequency, lead time, MTTR, change failure rate
- **SPACE Framework**: Satisfaction, Performance, Activity, Communication, Efficiency
- **Platform KPIs**: Time-to-first-deployment, template adoption rate, support ticket volume

## Related

- [[crossplane]] — Crossplane
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[concepts/production-operations-best-practices.md|production-operations-best-practices]] — Production Operations Best Practices
- [[concepts/infrastructure-as-code.md|infrastructure-as-code]] — Infrastructure as Code
- [[backstage]] — Backstage
- [[concepts/gitops-principles.md|GitOps Principles]]
- [[concepts/infrastructure-as-code.md|Infrastructure as Code]]
- [[concepts/production-operations-best-practices.md|Production Operations Best Practices]]
- [[backstage|Backstage]]
- [[crossplane|Crossplane]]
- [[synthesis/GitOps x 平台工程.md|GitOps x 平台工程]] — synthesis
- [[synthesis/IaC x 多集群管理.md|IaC x 多集群管理]] — synthesis
