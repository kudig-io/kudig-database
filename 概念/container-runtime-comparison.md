---
title: Container Runtime Comparison
description: Container Runtime Comparison — Kubernetes 生产运维知识库
summary: Container Runtime Comparison — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- containerd
- cri-o
- docker
- runtime
- cri
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Container Runtime Comparison 是什么
- 如何 Container Runtime Comparison
trigger_keywords:
- Container
- Runtime
- Comparison
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Container Runtime Comparison

## Runtime Layering

| Layer | Name | Examples | Responsibility |
|-------|------|---------|----------------|
| High-level | Container Engine | Docker, Podman | Image management, CLI, networking |
| Mid-level | CRI Manager | containerd, CRI-O | Container lifecycle, image [[Distribution|distribution]] |
| Low-level | OCI Runtime | runc, crun, youki | Actual container process creation |

## Production Runtime Comparison

| Dimension | containerd | CRI-O | Docker |
|-----------|-----------|-------|--------|
| Architecture | Monolithic daemon + shim | Monolithic daemon + conmon | dockerd + containerd |
| CRI Compatible | Native (CRI plugin) | Native (designed for CRI) | Requires dockershim (removed) |
| OCI Runtime | runc, crun, kata | runc, crun, kata | runc (default) |
| Image Management | ctr, nerdctl | crictl, podman | docker CLI |
| Memory Usage | Low (~100MB RAM) | Lowest (~50MB RAM) | High (~300MB RAM) |
| Image Pull | Parallel pull | Parallel pull | Parallel pull |
| K8s Integration | Default since v1.24 | Red Hat/OpenShift default | Deprecated in v1.24 |
| Community Support | Broadest | OpenShift ecosystem | Broadest toolchain |
| Security | Rootless support | Rootless support | Rootless support |
| Debug Tools | ctr, crictl, nerdctl | crictl, podman | docker CLI |
| Best For | General K8s clusters | OpenShift / security-first | Development |

## OCI Runtime Options

| Runtime | Language | Characteristics | Use Case |
|---------|----------|----------------|----------|
| runc | Go | OCI reference, most widely used | Default choice |
| crun | C | Lighter, faster startup | Resource-constrained |
| youki | Rust | Memory safe, experimental | Security experiments |
| gVisor (runsc) | Go | Kernel isolation (sandbox) | Multi-tenant, security |
| Kata Containers | Go | Lightweight VM isolation | Strong isolation required |

## K8s CRI Evolution

```
# 🟢 低风险：只读/信息收集，通常无副作用
2014-2020: K8s uses dockershim to talk to Docker Engine
2020: dockershim deprecated (Docker-specific coupling)
2021: dockershim removed from kubelet source
2022+: K8s nodes use containerd or CRI-O directly
        Docker images remain compatible (OCI Image Spec)
```
## Production Recommendations

- **Standard K8s**: containerd (default, proven, well-supported)
- **OpenShift / Security-first**: CRI-O (minimal attack surface)
- **Multi-tenant isolation**: Kata Containers or gVisor as runtime class
- **Development**: Docker Desktop (convenience, tooling)
- **CI/CD build**: Docker BuildKit or Kaniko (image building)

## Related

- [[实体/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[youki]] — youki
- [[cri-o]] — CRI-O
- [[概念/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[概念/docker-architecture.md|Docker Architecture]]
- [[概念/linux-container-foundation.md|Linux Container Foundation]]
- [[containerd|containerd]]
- [[cri-o|CRI-O]]
- OCI Standard


<!-- risk-assessed -->
