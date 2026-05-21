---
title: containerd
description: containerd — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- container
- runtime
- containerd
- cni
- csi
- kubelet
- docker
- wasm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 是什么
- 如何 containerd
trigger_keywords:
- containerd
prerequisites:
- kubectl-basics
---

# containerd

containerd is an industry-standard container runtime that manages the complete container lifecycle on a host system. It was donated to CNCF by Docker in 2017 and became the default K8s runtime after dockershim removal in v1.24.

## Key Facts

- **Status**: CNCF graduated project
- **Architecture**: Monolithic daemon with plugin system
- **Memory Footprint**: ~100MB RAM
- **Default OCI Runtime**: runc (also supports crun, kata)
- **CRI Plugin**: io.containerd.grpc.v1.cri
- **Configuration**: /etc/containerd/config.toml

## Core Components

| Component | Function |
|-----------|----------|
| Content Store | Content-addressable blob storage for images |
| Snapshots | Filesystem snapshot management (overlayfs, btrfs, zfs) |
| Tasks | Container process lifecycle management |
| Namespaces | Multi-tenant isolation within containerd |
| Events | Event streaming for monitoring and integration |

## K8s Integration

containerd exposes the Container Runtime Interface (CRI) via gRPC. kubelet communicates directly with containerd without intermediate shim layers. Key configuration: sandbox_image (pause container), SystemdCgroup (use systemd for cgroups), registry mirrors.

## Debugging

```bash
# Check containerd status
systemctl status containerd

# List containers
ctr -n k8s.io containers list
crictl ps

# View containerd logs
journalctl -u containerd -f
```

## Related

- [[concepts/linux-container-foundation.md|linux-container-foundation]] — Linux Container Foundation
- [[concepts/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[concepts/container-runtime-comparison.md|container-runtime-comparison]] — Container Runtime Comparison
- [[docker]] — Docker
- [[entities/container-runtime.md|container-runtime]] — Container Runtime
- [[concepts/docker-architecture.md|Docker Architecture]]
- [[concepts/container-runtime-comparison.md|Container Runtime Comparison]]
- [[concepts/linux-container-foundation.md|Linux Container Foundation]]

- [[domain-19-landscape-references/graduated/containerd/07-containerd-disaster-recovery.md|07-containerd-disaster-recovery]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-0.0.md|RELEASE-NOTES-0.0]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/core-deps/containerd/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/graduated/containerd/04-containerd-upgrade-migration.md|04-containerd-upgrade-migration]]
- [[domain-15-specialized-tech/02-containerd-wasm-shim.md|02-containerd-wasm-shim]]
- [[domain-19-landscape-references/graduated/containerd/containerd.md|containerd]]
- [[domain-19-landscape-references/graduated/containerd/05-containerd-windows-support.md|05-containerd-windows-support]]
- [[domain-19-landscape-references/graduated/containerd/02-containerd-v2-features.md|02-containerd-v2-features]]
- [[domain-19-landscape-references/graduated/containerd/08-containerd-multi-tenant.md|08-containerd-multi-tenant]]
- [[domain-19-landscape-references/graduated/containerd/03-containerd-security-hardening.md|03-containerd-security-hardening]]
- [[domain-19-landscape-references/graduated/containerd/06-containerd-observability.md|06-containerd-observability]]
- [[references/k8s-structured-troubleshooting|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[references/k8s-control-plane-deep-dive|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[references/release-notes-core-deps|发布说明索引 — 核心依赖]] — Cross-reference
- [[concepts/core-dependency-version-matrix|核心依赖版本矩阵]] — Cross-reference
- [[concepts/linux-sysctl-tuning|Linux Sysctl Tuning for Kubernetes]] — Cross-reference
- [[concepts/overlayfs-storage|OverlayFS Storage]] — Cross-reference
- [[concepts/node-lifecycle-management|节点生命周期管理]] — Cross-reference
- [[skills/ts-node-components|节点组件故障排查]] — Cross-reference
- [[skills/kubeadm-cluster-deletion|kubeadm 集群删除操作]] — Cross-reference
- [[skills/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[skills/skill-reference-diagnostic-workflow|Diagnostic Workflow]] — Cross-reference
- [[skills/skill-reference-root-cause-catalog|Root Cause Catalog]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[entities/core-deps-changelog|核心依赖变更日志索引]] — Cross-reference
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
