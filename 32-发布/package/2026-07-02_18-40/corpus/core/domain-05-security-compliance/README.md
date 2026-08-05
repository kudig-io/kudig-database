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

- [[domain-06-observability/README.md|domain-06-observability]] — 安全审计日志与监控
- [[domain-10-troubleshooting-diagnostics/README.md|domain-10-troubleshooting-diagnostics]] — 安全相关排障
- [[domain-08-release-change-management/README.md|domain-08-release-change-management]] — 安全发布流程

## 目录内容

- [[domain-05-security-compliance/00-open-source-projects-index.md|00-open-source-projects-index]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/01-supply-chain-maturity-model|02-supply-chain-maturity-model]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/11-image-security-scanning|10-image-security-scanning]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/06-sigstore-cosign-signing|07-sigstore-cosign-signing]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-05-security-compliance/01-supply-chain/01-supply-chain-security-overview|01-supply-chain-security-overview]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/02-sbom-generation-management|03-sbom-generation-management]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/05-github-actions-slsa-build|06-github-actions-slsa-build]]
- [[10-software-bill-of-materials|09-software-bill-of-materials]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/07-fulcio-rekor-transparency|08-fulcio-rekor-transparency]]
- [[11-compliance-automation-audit|10-compliance-automation-audit]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/03-sbom-vulnerability-analysis|04-sbom-vulnerability-analysis]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/12-image-security-scanning|13-image-security-scanning]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/04-slsa-levels-implementation|05-slsa-levels-implementation]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/08-policy-controller-verification|09-policy-controller-verification]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/05-supply-chain/13-slsa-supply-chain-security-guide|99-slsa-supply-chain-security-guide]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/01-identity-access/08-vault-k8s-secrets-guide|99-vault-k8s-secrets-guide]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/01-identity-access/06-rbac-matrix-configuration|07-rbac-matrix-configuration]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/01-identity-access/07-secret-management-tools|11-secret-management-tools]]
- [[domain-05-security-compliance/身份与访问/01-authentication-authorization-system.md|01-authentication-authorization-system]]
- [[domain-05-security-compliance/身份与访问/05-vault-enterprise-secrets-management.md|05-vault-enterprise-secrets-management]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/03-runtime-security/06-gvisor-container-sandbox|17-gvisor-container-sandbox]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/03-runtime-security/07-falco-runtime-security-guide|99-falco-runtime-security-guide]]
- [[domain-05-security-compliance/运行时安全/01-falco-cloud-native-security.md|01-falco-cloud-native-security]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/03-runtime-security/04-runtime-security-defense|03-runtime-security-defense]]
- [[domain-05-security-compliance/运行时安全/02-sysdig-enterprise-container-security.md|02-sysdig-enterprise-container-security]]
- [[domain-05-security-compliance/运行时安全/03-aqua-enterprise-container-security.md|03-aqua-enterprise-container-security]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/03-runtime-security/05-runtime-security-detection|15-runtime-security-detection]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-7.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-7.md|README-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/00-open-source-projects-index-from-domain-25.md|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-25.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-25.md|README-from-domain-05-security-compliance]]
- [[02-open-source-projects-index-from-domain-7|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/98-merged-indexes/01-open-source-projects-index-from-domain-39|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-39.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/README-from-domain-39.md|README-from-domain-05-security-compliance]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-05-security-compliance/02-incident-response/01-incident-response-process|20-incident-response-process]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/02-network-security/04-zero-trust-architecture|19-zero-trust-architecture]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/02-network-security/01-network-security-policies|02-network-security-policies]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/02-network-security/03-network-defense-depth|18-network-defense-depth]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/02-network-security/02-zero-trust-security-architecture|07-zero-trust-security-architecture]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/02-network-security/05-multicluster-security|21-multicluster-security]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/03-audit-logging-compliance|04-audit-logging-compliance]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/04-cis-benchmark-compliance-audit|08-cis-benchmark-compliance-audit]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/08-kubernetes-security-hardening|11-kubernetes-security-hardening]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/10-compliance-audit-practices|16-compliance-audit-practices]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/09-compliance-certification|12-compliance-certification]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/05-security-best-practices|08-security-best-practices]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/12-cert-manager-tls-guide|99-cert-manager-tls-guide]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/13-java-security-kubernetes-guide|99-java-security-kubernetes-guide]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/07-certificate-management|10-certificate-management]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/11-comprehensive-security-scanning|17-comprehensive-security-scanning]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/06-compliance/06-security-hardening-production|09-security-hardening-production]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/04-policy-governance/04-opa-gatekeeper-policy|09-opa-gatekeeper-policy]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/04-policy-governance/06-kyverno-policy-guide|99-kyverno-policy-guide]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/04-policy-governance/01-kyverno-enterprise-policy-management|04-kyverno-enterprise-policy-management]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/04-policy-governance/05-policy-engines-opa-kyverno|14-policy-engines-opa-kyverno]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/04-policy-governance/02-policy-validation-tools|05-policy-validation-tools]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/04-policy-governance/03-pod-security-standards|06-pod-security-standards]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-05-security-compliance/04-policy-governance/07-opa-gatekeeper-policy-guide|99-opa-gatekeeper-policy-guide]]

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
