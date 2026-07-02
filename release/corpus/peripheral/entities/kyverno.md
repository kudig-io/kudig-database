---
title: Kyverno [entities]
description: Kyverno — Kubernetes 生产运维知识库
summary: Kyverno — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- security
- policy
- kyverno
- admission
- governance
- opa
- networkpolicy
- operator
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kyverno 是什么
- 如何 Kyverno
trigger_keywords:
- Kyverno
prerequisites:
- kubectl-basics
- gitops-basics
- policy-basics
---



# Kyverno

Kyverno is a Kubernetes-native policy engine that uses YAML syntax for policy definitions, making it accessible to K8s operators without learning Rego.

## Key Facts

- **Status**: CNCF incubating (2023)
- **Language**: YAML (K8s-native, not Rego)
- **Mechanism**: Admission Webhooks (ValidatingAdmissionPolicy)
- **Modes**: Validate, Mutate, Generate, Cleanup, ImageVerify

## Policy Modes

| Mode | Description | Example |
|------|-------------|---------|
| Validate | Reject non-compliant resources | Require resource limits, non-root user |
| Mutate | Auto-fix non-compliant resources | Add default labels, set security context |
| Generate | Create dependent resources | [[NetworkPolicy|NetworkPolicy]] per namespace |
| Cleanup | Delete stale resources | Remove expired test environments |
| ImageVerify | Verify image signatures | Block unsigned images from deployment |

## Compliance Use Cases

Kyverno automates compliance checks for SOC 2, ISO 27001, PCI-DSS, and NIST CSF by defining policies that enforce security baselines (non-root containers, resource limits, image registry allowlists, NetworkPolicy requirements).

## Related

- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[supply-chain-security]] — Software Supply Chain Security
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[supply-chain-security|Supply Chain Security]]

- 14-policy-engines-opa-kyverno
- 99-kyverno-policy-guide
- 04-kyverno-enterprise-policy-management
- kyverno
- [[concepts/IaC x 多集群管理.md|基础设施即代码 x 多集群管理]] — Cross-reference
- [[concepts/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — Cross-reference
- [[concepts/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[entities/argocd.md|ArgoCD]] — Cross-reference
- [[entities/trivy.md|Trivy]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
