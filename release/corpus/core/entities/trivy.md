---
title: Trivy
description: Trivy — Kubernetes 生产运维知识库
summary: Trivy — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
| Secret Detection | Hardcoded credentials in code/repos | Exposed [[Secrets|secrets]] |
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
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]] — [[concepts/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[supply-chain-security]] — Software Supply Chain Security
- [[supply-chain-security|Supply Chain Security]]
- [[concepts/ci-cd-pipeline-patterns.md|CI/CD Pipeline Patterns]]
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]

- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.26
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.18
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.69
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.7
- RELEASE-NOTES-0.19
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.15