---
summary: "本 Domain 整合了原三个分散的安全相关 Domain："
title: Security & Compliance
description: '# Domain 05 — Security & Compliance（安全与合规）'
category: domain
tags:
- security
- compliance
- identity
- runtime-security
- supply-chain
- policy
- zero-trust
- production
- opa
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Security & Compliance 是什么
- 如何 Security & Compliance
- Kubernetes 05 security compliance 最佳实践
trigger_keywords:
- Security
- Compliance
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
created: "2026-05-23"
---

# Domain 05 — Security & Compliance（安全与合规）

> **生产环境安全防护与合规治理的统一入口。**

本 Domain 整合了原三个分散的安全相关 Domain：
- `domain-7-security`（24 文件）
- `domain-25-cloud-native-security`（18 文件）
- `domain-39-supply-chain-security`（14 文件）

## 目录结构

| 子目录 | 内容 | 文件数 |
|--------|------|--------|
| `01-identity-access/` | 认证授权、RBAC、Secret 管理、Vault | 5 |
| `02-network-security/` | 网络策略、零信任、多集群安全、网络防御 | 4 |
| `03-runtime-security/` | [[Falco|Falco]]、Sysdig、Aqua、运行时防御、gVisor | 7 |
| `04-policy-governance/` | OPA/Gatekeeper、[[Kyverno|Kyverno]]、Pod 安全标准 | 7 |
| `05-supply-chain/` | SBOM、SLSA、Sigstore、Cosign、供应链安全 | 13 |
| `06-compliance/` | 审计日志、合规认证、安全加固、证书管理 | 10 |
| `07-incident-response/` | 安全事件响应流程 | 1 |
| `98-merged-indexes/` | 原始 Domain 的元数据文件保留 | 9 |

## 安全事件响应快速入口

```
运行时告警 → 03-runtime-security/
策略违规 → 04-policy-governance/
镜像漏洞 → 05-supply-chain/
证书过期 → 06-compliance/10-certificate-management.md
RBAC 问题 → 01-identity-access/07-rbac-matrix-configuration.md
零信任架构 → 02-network-security/19-zero-trust-architecture.md
安全事件 → 07-incident-response/
```

## 与其他 Domain 的关系

- [[domain-06-observability/README.md|domain-06-observability]] — 安全审计日志与监控
- [[domain-10-troubleshooting-diagnostics/README.md|domain-10-troubleshooting-diagnostics]] — 安全相关排障
- [[domain-08-release-change-management/README.md|domain-08-release-change-management]] — 安全发布流程

## 目录内容

- [[domain-05-security-compliance/00-open-source-projects-index.md|00-open-source-projects-index]]
- [[domain-05-security-compliance/05-supply-chain/02-supply-chain-maturity-model.md|02-supply-chain-maturity-model]]
- [[domain-05-security-compliance/05-supply-chain/10-image-security-scanning.md|10-image-security-scanning]]
- [[domain-05-security-compliance/05-supply-chain/07-sigstore-cosign-signing.md|07-sigstore-cosign-signing]]
- [[domain-05-security-compliance/05-supply-chain/01-supply-chain-security-overview.md|01-supply-chain-security-overview]]
- [[domain-05-security-compliance/05-supply-chain/03-sbom-generation-management.md|03-sbom-generation-management]]
- [[domain-05-security-compliance/05-supply-chain/06-github-actions-slsa-build.md|06-github-actions-slsa-build]]
- [[domain-05-security-compliance/05-supply-chain/09-software-bill-of-materials.md|09-software-bill-of-materials]]
- [[domain-05-security-compliance/05-supply-chain/08-fulcio-rekor-transparency.md|08-fulcio-rekor-transparency]]
- [[domain-05-security-compliance/05-supply-chain/10-compliance-automation-audit.md|10-compliance-automation-audit]]
- [[domain-05-security-compliance/05-supply-chain/04-sbom-vulnerability-analysis.md|04-sbom-vulnerability-analysis]]
- [[domain-05-security-compliance/05-supply-chain/13-image-security-scanning.md|13-image-security-scanning]]
- [[domain-05-security-compliance/05-supply-chain/05-slsa-levels-implementation.md|05-slsa-levels-implementation]]
- [[domain-05-security-compliance/05-supply-chain/09-policy-controller-verification.md|09-policy-controller-verification]]
- [[domain-05-security-compliance/05-supply-chain/99-slsa-supply-chain-security-guide.md|99-slsa-supply-chain-security-guide]]
- [[domain-05-security-compliance/01-identity-access/99-vault-k8s-secrets-guide.md|99-vault-k8s-secrets-guide]]
- [[domain-05-security-compliance/01-identity-access/07-rbac-matrix-configuration.md|07-rbac-matrix-configuration]]
- [[domain-05-security-compliance/01-identity-access/11-secret-management-tools.md|11-secret-management-tools]]
- [[domain-05-security-compliance/01-identity-access/01-authentication-authorization-system.md|01-authentication-authorization-system]]
- [[domain-05-security-compliance/01-identity-access/05-vault-enterprise-secrets-management.md|05-vault-enterprise-secrets-management]]
- [[domain-05-security-compliance/03-runtime-security/17-gvisor-container-sandbox.md|17-gvisor-container-sandbox]]
- [[domain-05-security-compliance/03-runtime-security/99-falco-runtime-security-guide.md|99-falco-runtime-security-guide]]
- [[domain-05-security-compliance/03-runtime-security/01-falco-cloud-native-security.md|01-falco-cloud-native-security]]
- [[domain-05-security-compliance/03-runtime-security/03-runtime-security-defense.md|03-runtime-security-defense]]
- [[domain-05-security-compliance/03-runtime-security/02-sysdig-enterprise-container-security.md|02-sysdig-enterprise-container-security]]
- [[domain-05-security-compliance/03-runtime-security/03-aqua-enterprise-container-security.md|03-aqua-enterprise-container-security]]
- [[domain-05-security-compliance/03-runtime-security/15-runtime-security-detection.md|15-runtime-security-detection]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-7.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-7.md|README-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/00-open-source-projects-index-from-domain-25.md|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-25.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-25.md|README-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/00-open-source-projects-index-from-domain-7.md|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/00-open-source-projects-index-from-domain-39.md|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-39.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-39.md|README-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/07-incident-response/20-incident-response-process.md|20-incident-response-process]]
- [[domain-05-security-compliance/02-network-security/19-zero-trust-architecture.md|19-zero-trust-architecture]]
- [[domain-05-security-compliance/02-network-security/02-network-security-policies.md|02-network-security-policies]]
- [[domain-05-security-compliance/02-network-security/18-network-defense-depth.md|18-network-defense-depth]]
- [[domain-05-security-compliance/02-network-security/07-zero-trust-security-architecture.md|07-zero-trust-security-architecture]]
- [[domain-05-security-compliance/02-network-security/21-multicluster-security.md|21-multicluster-security]]
- [[domain-05-security-compliance/06-compliance/04-audit-logging-compliance.md|04-audit-logging-compliance]]
- [[domain-05-security-compliance/06-compliance/08-cis-benchmark-compliance-audit.md|08-cis-benchmark-compliance-audit]]
- [[domain-05-security-compliance/06-compliance/11-kubernetes-security-hardening.md|11-kubernetes-security-hardening]]
- [[domain-05-security-compliance/06-compliance/16-compliance-audit-practices.md|16-compliance-audit-practices]]
- [[domain-05-security-compliance/06-compliance/12-compliance-certification.md|12-compliance-certification]]
- [[domain-05-security-compliance/06-compliance/08-security-best-practices.md|08-security-best-practices]]
- [[domain-05-security-compliance/06-compliance/99-cert-manager-tls-guide.md|99-cert-manager-tls-guide]]
- [[domain-05-security-compliance/06-compliance/99-java-security-kubernetes-guide.md|99-java-security-kubernetes-guide]]
- [[domain-05-security-compliance/06-compliance/10-certificate-management.md|10-certificate-management]]
- [[domain-05-security-compliance/06-compliance/17-comprehensive-security-scanning.md|17-comprehensive-security-scanning]]
- [[domain-05-security-compliance/06-compliance/09-security-hardening-production.md|09-security-hardening-production]]
- [[domain-05-security-compliance/04-policy-governance/09-opa-gatekeeper-policy.md|09-opa-gatekeeper-policy]]
- [[domain-05-security-compliance/04-policy-governance/99-kyverno-policy-guide.md|99-kyverno-policy-guide]]
- [[domain-05-security-compliance/04-policy-governance/04-kyverno-enterprise-policy-management.md|04-kyverno-enterprise-policy-management]]
- [[domain-05-security-compliance/04-policy-governance/14-policy-engines-opa-kyverno.md|14-policy-engines-opa-kyverno]]
- [[domain-05-security-compliance/04-policy-governance/05-policy-validation-tools.md|05-policy-validation-tools]]
- [[domain-05-security-compliance/04-policy-governance/06-pod-security-standards.md|06-pod-security-standards]]
- [[domain-05-security-compliance/04-policy-governance/99-opa-gatekeeper-policy-guide.md|99-opa-gatekeeper-policy-guide]]

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]
