---
title: Docker Architecture and Container Runtime
description: Docker Architecture and Container Runtime — Kubernetes 生产运维知识库
summary: Docker Architecture and Container Runtime — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- docker
- container
- containerd
- oci
- runtime
- kubelet
- cri-o
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
- Docker Architecture and Container Runtime 是什么
- 如何 Docker Architecture and Container Runtime
trigger_keywords:
- Docker
- Architecture
- and
- Container
- Runtime
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Docker Architecture and Container Runtime

## Runtime Layered Architecture

Docker operates through five distinct layers, each communicating via standardized APIs:

| Layer | Name | Process | Interface |
|-------|------|---------|-----------|
| User Interface | Docker CLI | docker | REST API (Unix Socket) |
| API [[Service|Service]] | Docker Daemon | dockerd | [[gRPC|gRPC]] API |
| Container Manager | containerd | containerd | OCI Runtime Spec |
| Container Shim | containerd-shim | containerd-shim-runc-v2 | OCI Runtime |
| Low-level Runtime | runc | runc | Linux Kernel |

When `docker run nginx:latest` executes:
1. CLI sends request to dockerd via REST API over Unix Socket
2. dockerd delegates to containerd via gRPC
3. containerd spawns containerd-shim-runc-v2 process
4. shim calls runc to create the actual container
5. runc configures namespaces and cgroups, starts the container process
6. runc exits; shim takes over container lifecycle management

## OCI Standard

The OCI standard consists of three specifications:
- **Runtime Spec**: Container configuration, lifecycle, execution environment
- **Image Spec**: Image layers, config blob, manifest format
- **Distribution Spec**: Registry API, authentication, push/pull protocol

OCI ensures interoperability across container runtimes. Docker images (OCI format) run on any OCI-compliant runtime (runc, crun, youki, gVisor, Kata).

## Containerd Architecture

containerd provides a complete container management system with:
- **Content Store**: Content-addressable blob storage
- **Snapshots**: Filesystem snapshot management (overlayfs, btrfs, zfs)
- **Tasks**: Container process lifecycle
- **Namespaces**: Multi-tenant isolation
- **Leases**: Resource lifecycle management
- **Events**: Event streaming for monitoring

For K8s integration, containerd exposes the CRI (Container Runtime Interface) via the `io.containerd.grpc.v1.cri` plugin.

## K8s Runtime Evolution

```
# 🟢 低风险：只读/信息收集，通常无副作用
2014-2020: K8s uses dockershim (built-in Docker shim)
2020: K8s deprecates dockershim
2021: dockershim removed from kubelet
2022+: K8s nodes use containerd or CRI-O directly
       Docker images remain compatible (OCI standard)
```
Production nodes should use [[containerd|containerd]] or [[cri-o|CRI-O]] as the container runtime. Docker remains valuable for development and image building via BuildKit.

## Alternative Container Engines

| Engine | Daemonless | Rootless | K8s CRI | Best For |
|--------|-----------|----------|---------|----------|
| Docker | No | Limited | No (deprecated) | Development, build |
| Podman | Yes | Full | No | Security-sensitive dev |
| nerdctl | No | Yes | Yes | K8s nodes with Docker-like CLI |
| CRI-O | No | Limited | Yes (native) | K8s dedicated runtime |

## Related

- [[概念/container-runtime-comparison.md|container-runtime-comparison]] — Container Runtime Comparison
- [[docker]] — Docker
- [[实体/container-runtime.md|container-runtime]] — Container Runtime
- [[containerd]] — containerd
- [[youki]] — youki
- [[概念/linux-container-foundation.md|Linux Container Foundation]]
- [[概念/container-runtime-comparison.md|Container Runtime Comparison]]
- [[概念/overlayfs-storage.md|OverlayFS Storage]]
- [[containerd|containerd]]
- [[docker|Docker]]
- OCI Standard

- 01-docker-architecture-overview

<!-- risk-assessed -->
