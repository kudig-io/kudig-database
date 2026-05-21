---
title: Falco
description: Falco — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- security
- runtime
- falco
- detection
- ebpf
- cilium
- daemonset
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Falco 是什么
- 如何 Falco
trigger_keywords:
- Falco
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

# Falco

Falco is the de facto runtime security threat detection engine for cloud native environments, graduated from CNCF.

## Key Facts

- **Status**: CNCF graduated
- **Engine**: eBPF or kernel module (dual engine)
- **Detection**: Rule-based system call monitoring
- **Output**: JSON events to stdout, files, or notification systems

## Typical Attack Detections

| Detection Rule | What It Catches |
|---------------|----------------|
| Terminal shell in container | Interactive shell or bash execution |
| Read sensitive file | Access to /etc/shadow, SSH keys |
| Container mounted host filesystem | Potential container escape |
| Crypto mining detected | Cryptocurrency mining processes |
| Outbound connection to C2 | Known command-and-control servers |
| Unexpected process execution | Unauthorized binary execution |
| Network activity from unexpected process | Lateral movement indicators |

## Deployment

Falco deploys as a DaemonSet with one pod per node, monitoring all container syscalls. Recommended configuration uses eBPF driver (safer than kernel module).

## Related

- [[kuasar]] — Kuasar
- [[deployment]] — Deployment
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[cilium]] — Cilium
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[concepts/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[entities/tetragon.md|Tetragon]]

- [[domain-05-security-compliance/99-falco-runtime-security-guide.md|99-falco-runtime-security-guide]]
- [[domain-05-security-compliance/01-falco-cloud-native-security.md|01-falco-cloud-native-security]]
- [[domain-19-landscape-references/graduated/falco/falco.md|falco]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.43.md|RELEASE-NOTES-0.43]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.26.md|RELEASE-NOTES-0.26]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.36.md|RELEASE-NOTES-0.36]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.22.md|RELEASE-NOTES-0.22]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.32.md|RELEASE-NOTES-0.32]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.23.md|RELEASE-NOTES-0.23]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.33.md|RELEASE-NOTES-0.33]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.42.md|RELEASE-NOTES-0.42]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.27.md|RELEASE-NOTES-0.27]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.37.md|RELEASE-NOTES-0.37]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.28.md|RELEASE-NOTES-0.28]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.38.md|RELEASE-NOTES-0.38]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.29.md|RELEASE-NOTES-0.29]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.39.md|RELEASE-NOTES-0.39]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.1.md|RELEASE-NOTES-0.1]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.5.md|RELEASE-NOTES-0.5]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.30.md|RELEASE-NOTES-0.30]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.10.md|RELEASE-NOTES-0.10]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.41.md|RELEASE-NOTES-0.41]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.24.md|RELEASE-NOTES-0.24]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.34.md|RELEASE-NOTES-0.34]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.40.md|RELEASE-NOTES-0.40]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.25.md|RELEASE-NOTES-0.25]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.35.md|RELEASE-NOTES-0.35]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.21.md|RELEASE-NOTES-0.21]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]
- [[domain-19-landscape-references/topic-release-notes/security/falco/RELEASE-NOTES-0.31.md|RELEASE-NOTES-0.31]]
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[references/release-notes-reading-guide|发布说明阅读指南]] — Cross-reference
- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — Cross-reference
- [[synthesis/eBPF x 运行时安全|eBPF x 运行时安全]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[entities/trivy|Trivy]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
