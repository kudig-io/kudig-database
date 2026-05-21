---
title: Trivy
description: Trivy — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- security
- scanning
- trivy
- vulnerability
- sbom
- docker
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Trivy 是什么
- 如何 Trivy
trigger_keywords:
- Trivy
prerequisites:
- kubectl-basics
- iac-basics
- policy-basics
---

# Trivy

Trivy (by Aqua Security) is a comprehensive, open-source security scanner for cloud native artifacts.

## Key Facts

- **License**: Apache-2.0 (free)
- **Scanner Types**: Vulnerability, misconfiguration, secret, SBOM
- **Targets**: Container images, filesystems, K8s clusters, Git repos, IaC files

## Scan Capabilities

| Scan Type | Description | Output |
|-----------|-------------|--------|
| Vulnerability | OS packages and language dependencies | CVE list with severity |
| Misconfiguration | K8s manifests, Dockerfile, Terraform | Policy violations |
| Secret Detection | Hardcoded credentials in code/repos | Exposed secrets |
| SBOM Generation | Software Bill of Materials | CycloneDX/SPDX format |

## CI/CD Integration

```bash
# Scan image for critical/high vulnerabilities
trivy image --severity HIGH,CRITICAL nginx:1.25

# Scan K8s cluster for misconfigurations
trivy k8s --severity HIGH,CRITICAL --report summary cluster

# Generate SBOM
trivy image --format spdx-json --output sbom.json nginx:1.25
```

## Related

- [[falco]] — Falco
- [[kyverno]] — Kyverno
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]] — CI/CD Pipeline Patterns
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[supply-chain-security]] — Software Supply Chain Security
- [[supply-chain-security|Supply Chain Security]]
- [[concepts/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]

- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.26.md|RELEASE-NOTES-0.26]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.22.md|RELEASE-NOTES-0.22]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.23.md|RELEASE-NOTES-0.23]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.69.md|RELEASE-NOTES-0.69]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.0.md|RELEASE-NOTES-0.0]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.1.md|RELEASE-NOTES-0.1]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.5.md|RELEASE-NOTES-0.5]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.10.md|RELEASE-NOTES-0.10]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.24.md|RELEASE-NOTES-0.24]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.25.md|RELEASE-NOTES-0.25]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.21.md|RELEASE-NOTES-0.21]]
- [[domain-19-landscape-references/topic-release-notes/security/trivy/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]