---
title: Kyverno
description: Kyverno — Kubernetes 生产运维知识库
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
| Generate | Create dependent resources | NetworkPolicy per namespace |
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

- [[domain-05-security-compliance/14-policy-engines-opa-kyverno.md|14-policy-engines-opa-kyverno]]
- [[domain-05-security-compliance/99-kyverno-policy-guide.md|99-kyverno-policy-guide]]
- [[domain-05-security-compliance/04-kyverno-enterprise-policy-management.md|04-kyverno-enterprise-policy-management]]
- [[domain-19-landscape-references/incubating/kyverno/kyverno.md|kyverno]]
- [[synthesis/IaC x 多集群管理|基础设施即代码 x 多集群管理]] — Cross-reference
- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[entities/trivy|Trivy]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
