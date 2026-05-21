---
title: Software Supply Chain Security
description: '- [[synthesis/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — synthesis'
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

# Software Supply Chain Security

## Threat Chain

Supply chain attacks can occur at any stage:
1. **Development**: Developer machine compromised, dependency confusion attack
2. **Build**: CI/CD pipeline hijacked, build tools tampered with, backdoor inserted during compilation
3. **Distribution**: Registry attacked, image replaced, tag mutated (same tag points to different content)
4. **Deployment**: Unsigned image deployed to production without verification

## SBOM (Software Bill of Materials)

SBOM is a formal inventory of all components in a software artifact:

| Tool | Format | Use Case |
|------|--------|----------|
| Syft | CycloneDX, SPDX | Container image SBOM generation |
| Trivy | SPDX-JSON | Image scan + SBOM combined |
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
- [[synthesis/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — synthesis

- [[domain-19-landscape-references/20-kubernetes-supply-chain-security-sbom-slsa-sigstore.md|20-kubernetes-supply-chain-security-sbom-slsa-sigstore]]
- [[domain-05-security-compliance/00-open-source-projects-index.md|00-open-source-projects-index]]
- [[domain-05-security-compliance/02-supply-chain-maturity-model.md|02-supply-chain-maturity-model]]
- [[domain-05-security-compliance/07-sigstore-cosign-signing.md|07-sigstore-cosign-signing]]
- [[domain-05-security-compliance/01-supply-chain-security-overview.md|01-supply-chain-security-overview]]
- [[domain-05-security-compliance/03-sbom-generation-management.md|03-sbom-generation-management]]
- [[domain-05-security-compliance/06-github-actions-slsa-build.md|06-github-actions-slsa-build]]
- [[domain-05-security-compliance/08-fulcio-rekor-transparency.md|08-fulcio-rekor-transparency]]
- [[domain-05-security-compliance/10-compliance-automation-audit.md|10-compliance-automation-audit]]
- [[domain-05-security-compliance/README.md|Domain 39: 供应链安全 (Supply Chain Security)]]
- [[domain-05-security-compliance/04-sbom-vulnerability-analysis.md|04-sbom-vulnerability-analysis]]
- [[domain-05-security-compliance/05-slsa-levels-implementation.md|05-slsa-levels-implementation]]
- [[domain-05-security-compliance/09-policy-controller-verification.md|09-policy-controller-verification]]
- [[domain-05-security-compliance/MOC.md|domain-05-security-compliance MOC]]
- [[domain-05-security-compliance/99-slsa-supply-chain-security-guide.md|99-slsa-supply-chain-security-guide]]
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
