---
title: Security & Compliance
description: '# Domain 05 — Security & Compliance（安全与合规）'
summary: 本 Domain 整合了原三个分散的安全相关 Domain：
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



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

- [[可观测性/README.md|可观测性]] — 安全审计日志与监控
- [[故障诊断/README.md|故障诊断]] — 安全相关排障
- [[发布变更/README.md|发布变更]] — 安全发布流程

## 目录内容

- [[安全/00-open-source-projects-index.md|00-open-source-projects-index]]
- [[安全/供应链/02-supply-chain-maturity-model.md|02-supply-chain-maturity-model]]
- [[安全/供应链/10-image-security-scanning.md|10-image-security-scanning]]
- [[安全/供应链/07-sigstore-cosign-signing.md|07-sigstore-cosign-signing]]
- [[安全/供应链/01-supply-chain-security-overview.md|01-supply-chain-security-overview]]
- [[安全/供应链/03-sbom-generation-management.md|03-sbom-generation-management]]
- [[安全/供应链/06-github-actions-slsa-build.md|06-github-actions-slsa-build]]
- [[安全/供应链/09-software-bill-of-materials.md|09-software-bill-of-materials]]
- [[安全/供应链/08-fulcio-rekor-transparency.md|08-fulcio-rekor-transparency]]
- [[安全/供应链/10-compliance-automation-audit.md|10-compliance-automation-audit]]
- [[安全/供应链/04-sbom-vulnerability-analysis.md|04-sbom-vulnerability-analysis]]
- [[安全/供应链/13-image-security-scanning.md|13-image-security-scanning]]
- [[安全/供应链/05-slsa-levels-implementation.md|05-slsa-levels-implementation]]
- [[安全/供应链/09-policy-controller-verification.md|09-policy-controller-verification]]
- [[安全/供应链/99-slsa-supply-chain-security-guide.md|99-slsa-supply-chain-security-guide]]
- [[安全/身份与访问/99-vault-k8s-secrets-guide.md|99-vault-k8s-secrets-guide]]
- [[安全/身份与访问/07-rbac-matrix-configuration.md|07-rbac-matrix-configuration]]
- [[安全/身份与访问/11-secret-management-tools.md|11-secret-management-tools]]
- [[安全/身份与访问/01-authentication-authorization-system.md|01-authentication-authorization-system]]
- [[安全/身份与访问/05-vault-enterprise-secrets-management.md|05-vault-enterprise-secrets-management]]
- [[安全/运行时安全/17-gvisor-container-sandbox.md|17-gvisor-container-sandbox]]
- [[安全/运行时安全/99-falco-runtime-security-guide.md|99-falco-runtime-security-guide]]
- [[安全/运行时安全/01-falco-cloud-native-security.md|01-falco-cloud-native-security]]
- [[安全/运行时安全/03-runtime-security-defense.md|03-runtime-security-defense]]
- [[安全/运行时安全/02-sysdig-enterprise-container-security.md|02-sysdig-enterprise-container-security]]
- [[安全/运行时安全/03-aqua-enterprise-container-security.md|03-aqua-enterprise-container-security]]
- [[安全/运行时安全/15-runtime-security-detection.md|15-runtime-security-detection]]
- [[安全/98-merged-indexes/MOC-from-domain-7.md|MOC-from-安全]]
- [[安全/98-merged-indexes/README-from-domain-7.md|README-from-安全]]
- [[安全/98-merged-indexes/00-open-source-projects-index-from-domain-25.md|00-open-source-projects-index-from-安全]]
- [[安全/98-merged-indexes/MOC-from-domain-25.md|MOC-from-安全]]
- [[安全/98-merged-indexes/README-from-domain-25.md|README-from-安全]]
- [[安全/98-merged-indexes/00-open-source-projects-index-from-domain-7.md|00-open-source-projects-index-from-安全]]
- [[安全/98-merged-indexes/00-open-source-projects-index-from-domain-39.md|00-open-source-projects-index-from-安全]]
- [[安全/98-merged-indexes/MOC-from-domain-39.md|MOC-from-安全]]
- [[安全/98-merged-indexes/README-from-domain-39.md|README-from-安全]]
- [[生产运维/事件响应/20-incident-response-process.md|20-incident-response-process]]
- [[安全/网络安全/19-zero-trust-architecture.md|19-zero-trust-architecture]]
- [[安全/网络安全/02-network-security-policies.md|02-network-security-policies]]
- [[安全/网络安全/18-network-defense-depth.md|18-network-defense-depth]]
- [[安全/网络安全/07-zero-trust-security-architecture.md|07-zero-trust-security-architecture]]
- [[安全/网络安全/21-multicluster-security.md|21-multicluster-security]]
- [[安全/合规审计/04-audit-logging-compliance.md|04-audit-logging-compliance]]
- [[安全/合规审计/08-cis-benchmark-compliance-audit.md|08-cis-benchmark-compliance-audit]]
- [[安全/合规审计/11-kubernetes-security-hardening.md|11-kubernetes-security-hardening]]
- [[安全/合规审计/16-compliance-audit-practices.md|16-compliance-audit-practices]]
- [[安全/合规审计/12-compliance-certification.md|12-compliance-certification]]
- [[安全/合规审计/08-security-best-practices.md|08-security-best-practices]]
- [[安全/合规审计/99-cert-manager-tls-guide.md|99-cert-manager-tls-guide]]
- [[安全/合规审计/99-java-security-kubernetes-guide.md|99-java-security-kubernetes-guide]]
- [[安全/合规审计/10-certificate-management.md|10-certificate-management]]
- [[安全/合规审计/17-comprehensive-security-scanning.md|17-comprehensive-security-scanning]]
- [[安全/合规审计/09-security-hardening-production.md|09-security-hardening-production]]
- [[安全/策略治理/09-opa-gatekeeper-policy.md|09-opa-gatekeeper-policy]]
- [[安全/策略治理/99-kyverno-policy-guide.md|99-kyverno-policy-guide]]
- [[安全/策略治理/04-kyverno-enterprise-policy-management.md|04-kyverno-enterprise-policy-management]]
- [[安全/策略治理/14-policy-engines-opa-kyverno.md|14-policy-engines-opa-kyverno]]
- [[安全/策略治理/05-policy-validation-tools.md|05-policy-validation-tools]]
- [[安全/策略治理/06-pod-security-standards.md|06-pod-security-standards]]
- [[安全/策略治理/99-opa-gatekeeper-policy-guide.md|99-opa-gatekeeper-policy-guide]]

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
