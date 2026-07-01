---
title: Infrastructure as Code
description: '- [[concepts/IaC x 多集群管理.md|IaC x 多集群管理]] — synthesis'
category: concepts
tags:
- k8s
- iac
- terraform
- pulumi
- crossplane
- automation
- etcd
- helm
- argocd
- flux
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Infrastructure as Code 是什么
- 如何 Infrastructure as Code
trigger_keywords:
- Infrastructure
- as
- Code
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- iac-basics
- etcd-basics
- policy-basics
created: "2026-05-23"
---

# Infrastructure as Code

## IaC Tool Comparison

| Tool | Language | Paradigm | State Management | Best For |
|------|----------|----------|-----------------|----------|
| Terraform | HCL | Declarative | Local, S3, Terraform Cloud | Multi-cloud infrastructure |
| Pulumi | TypeScript/Python/Go | Imperative/Declarative | [[Service|Service]], local, S3 | Developer-centric IaC |
| Ansible | YAML | Imperative (config mgmt) | None (idempotent) | Configuration management |
| Crossplane | YAML (K8s CRDs) | Declarative | etcd (K8s native) | K8s-native infra orchestration |
| AWS CDK | TypeScript/Python | Imperative | CloudFormation | AWS-only infrastructure |

## Core IaC Patterns

**Modular Design**: Break infrastructure into reusable modules (networking, compute, storage). Each module has inputs, outputs, and internal resource definitions. Enables consistent patterns across environments.

**State Management**: IaC tools track actual vs desired state. Terraform stores state in backends (S3+DynamoDB for locking, Terraform Cloud for team collaboration). Crossplane stores state in K8s etcd, naturally integrating with [[concepts/gitops-principles.md|GitOps]].

**Policy as Code**: Enforce infrastructure standards through automated policy checks:
- **Sentinel**: HashiCorp policy framework (Terraform Enterprise)
- **OPA**: Open Policy Agent with Rego language (cross-platform)
- **Conftest**: Configuration testing tool for CI/CD pipelines

## Terraform + GitOps Integration

Modern IaC pipelines combine Terraform (cloud resources) with GitOps (K8s resources):
1. Terraform provisions cloud infrastructure (VPCs, load balancers, managed K8s clusters)
2. Crossplane or Helm manages in-cluster resources
3. Both pipelines are GitOps-managed with ArgoCD/Flux
4. PR review catches misconfigurations before deployment

## Crossplane: K8s-Native IaC

Crossplane extends K8s API with custom resources for cloud infrastructure. A `Bucket` CRD provisions S3 storage, a `Database` CRD provisions RDS instances. Benefits:
- Unified API: same kubectl workflow for cloud and cluster resources
- GitOps native: state stored in etcd, reconciled by controllers
- Composition: combine multiple resources into higher-level abstractions

## Related

- [[helm]] — Helm
- [[etcd]] — etcd
- [[entities/argocd.md|argocd]] — ArgoCD
- [[concepts/platform-engineering-idp.md|platform-engineering-idp]] — Platform Engineering and Internal Developer Platforms
- [[crossplane]] — Crossplane
- [[concepts/gitops-principles.md|GitOps Principles]]
- [[concepts/platform-engineering-idp.md|Platform Engineering and IDP]]
- [[crossplane|Crossplane]]
- [[concepts/IaC x 多集群管理.md|IaC x 多集群管理]] — synthesis

- 05-crossplane-enterprise-orchestration
- 99-crossplane-platform-guide
- 00-open-source-projects-index
- 11-infrastructure-as-code
- [[domain-08-release-change-management/README.md|Domain 08: 基础设施即代码 (Infrastructure as Code)]]
- 03-pulumi-enterprise-iac
- 02-ansible-enterprise-automation
- 04-azure-resource-manager-enterprise
- 01-terraform-enterprise-iac
- domain-24-infrastructure-as-code MOC