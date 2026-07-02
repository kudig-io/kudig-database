---
title: containerd (entities)
description: containerd — Kubernetes 生产运维知识库
summary: containerd — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# containerd

containerd is an industry-standard [[concepts/container-runtime.md|container runtime]] that manages the complete container lifecycle on a host system. It was donated to CNCF by Docker in 2017 and became the default K8s runtime after dockershim removal in v1.24.

## Key Facts

- **Status**: CNCF graduated project
- **Architecture**: Monolithic daemon with plugin system
- **Memory Footprint**: ~100MB RAM
- **Default OCI Runtime**: runc (also supports crun, kata)
- **CRI Plugin**: io.containerd.[[gRPC|grpc]].v1.cri
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

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

- 07-containerd-disaster-recovery
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- [[domain-19-landscape-references/_archived-release-notes/core-deps/containerd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- RELEASE-NOTES-1.6
- [[domain-19-landscape-references/_archived-release-notes/core-deps/containerd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- [[domain-19-landscape-references/_archived-release-notes/core-deps/containerd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- RELEASE-NOTES-1.1
- RELEASE-NOTES-0.0
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- 04-containerd-upgrade-migration
- 02-containerd-wasm-shim
- containerd
- 05-containerd-windows-support
- 02-containerd-v2-features
- 08-containerd-multi-tenant
- 03-containerd-security-hardening
- 06-containerd-observability
- [[entities/k8s-structured-troubleshooting.md|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[entities/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[entities/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[concepts/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[concepts/linux-sysctl-tuning.md|Linux Sysctl Tuning for Kubernetes]] — Cross-reference
- [[concepts/overlayfs-storage.md|OverlayFS Storage]] — Cross-reference
- [[concepts/node-lifecycle-management.md|节点生命周期管理]] — Cross-reference
- [[skills/ts-node-components.md|节点组件故障排查]] — Cross-reference
- [[skills/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]] — Cross-reference
- [[skills/k8s-cluster-configuration-guide.md|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[skills/skill-reference-diagnostic-workflow.md|Diagnostic Workflow]] — Cross-reference
- [[skills/skill-reference-root-cause-catalog.md|Root Cause Catalog]] — Cross-reference
- [[skills/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[entities/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index.md|Node 知识图谱索引]]


<!-- risk-assessed -->
