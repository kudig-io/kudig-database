---
title: Software Supply Chain Security
description: '- [[concepts/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — synthesis'
summary: '- [[concepts/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — synthesis'
category: concepts
tags:
- k8s
- security
- supply-chain
- sbom
- sigstore
- slsa
- cosign
- opa
- agent
- argocd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Software Supply Chain Security 是什么
- 如何 Software Supply Chain Security
trigger_keywords:
- Software
- Supply
- Chain
- Security
prerequisites:
- kubectl-basics
- gitops-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Software Supply Chain Security

## Threat Chain

Supply chain attacks can occur at any stage:
1. **Development**: Developer machine compromised, dependency confusion attack
2. **Build**: CI/CD pipeline hijacked, build tools tampered with, backdoor inserted during compilation
3. **[[Distribution|Distribution]]**: Registry attacked, image replaced, tag mutated (same tag points to different content)
4. **Deployment**: Unsigned image deployed to production without verification

## SBOM (Software Bill of Materials)

SBOM is a formal inventory of all components in a software artifact:

| Tool | Format | Use Case |
|------|--------|----------|
| Syft | CycloneDX, SPDX | Container image SBOM generation |
| [[Trivy|Trivy]] | SPDX-JSON | Image scan + SBOM combined |
| cyclonedx-maven-plugin | CycloneDX | Java application SBOM |

SBOM enables offline vulnerability scanning and dependency tracking without needing the original image.

## Image Signing with Sigstore/Cosign

Cosign signs container images using ephemeral keys bound to OIDC identities (Sigstore keyless signing):
1. Build triggers OIDC authentication (GitHub Actions token, GCP service account)
2. Cosign generates ephemeral key pair, signs the image
3. Signature and certificate uploaded to Sigstore transparency log (Fulcio + Rekor)
4. Deployment admission verifies signature before allowing image to run

## SLSA Framework

| SLSA Level | Requirement | Implementation |
|------------|------------|----------------|
| Level 1 | Documented build process | Tekton Chains / GitHub Actions |
| Level 2 | Hosted build platform | GitHub Actions / Tekton |
| Level 3 | Hardened build platform | Tekton Chains + Cosign + SBOM |
| Level 4 | Two-party review + reproducible | Full chain signing + Hermetic Build |

## Admission Verification

Kyverno or OPA Gatekeeper policies verify image signatures before deployment:
- Block unsigned images
- Block images from untrusted registries
- Require images with no critical vulnerabilities
- Verify SBOM is attached

## Related

- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]] — CI/CD Pipeline Patterns
- [[opa]] — OPA (Open Policy Agent)
- [[kyverno]] — Kyverno
- [[entities/trivy.md|trivy]] — Trivy
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[concepts/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[entities/trivy.md|Trivy]]
- Cosign/Sigstore
- [[kyverno|Kyverno]]
- [[concepts/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — synthesis

- 20-kubernetes-supply-chain-security-sbom-slsa-sigstore
- [[domain-05-security-compliance/00-open-source-projects-index.md|00-open-source-projects-index]]
- 02-supply-chain-maturity-model
- 07-sigstore-cosign-signing
- 01-supply-chain-security-overview
- 03-sbom-generation-management
- 06-github-actions-slsa-build
- 08-fulcio-rekor-transparency
- 10-compliance-automation-audit
- [[domain-05-security-compliance/README.md|Domain 05: 供应链安全 (Supply Chain Security)]]
- 04-sbom-vulnerability-analysis
- 05-slsa-levels-implementation
- 09-policy-controller-verification
- domain-05-security-compliance MOC
- 99-slsa-supply-chain-security-guide
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[entities/argocd.md|ArgoCD]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
