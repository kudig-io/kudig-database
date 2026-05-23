---
title: Falco (entities)
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
created: "2026-05-23"
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

Falco deploys as a [[DaemonSet|DaemonSet]] with one pod per node, monitoring all container syscalls. Recommended configuration uses eBPF driver (safer than kernel module).

## Related

- [[kuasar]] — Kuasar
- [[deployment]] — Deployment
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — [[Cloud Native Defense in Depth|Cloud Native Defense in Depth]]
- networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[cilium]] — Cilium
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[concepts/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[entities/tetragon.md|Tetragon]]

- 99-falco-runtime-security-guide
- 01-falco-cloud-native-security
- falco
- RELEASE-NOTES-0.43
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.26
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.36
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.32
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.33
- RELEASE-NOTES-0.42
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.27
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.37
- RELEASE-NOTES-0.18
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.28
- RELEASE-NOTES-0.38
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.29
- RELEASE-NOTES-0.39
- RELEASE-NOTES-0.7
- RELEASE-NOTES-0.19
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.30
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.41
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.34
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.40
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.35
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.15
- RELEASE-NOTES-0.31
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[references/release-notes-reading-guide|发布说明阅读指南]] — Cross-reference
- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — Cross-reference
- [[synthesis/eBPF x 运行时安全|eBPF x 运行时安全]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[entities/trivy|Trivy]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
