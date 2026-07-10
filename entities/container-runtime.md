---
title: Container Runtime (entities)
description: Container Runtime — Kubernetes 生产运维知识库
summary: Container Runtime — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- container
- runtime
- containerd
- cri-o
- cri
- kubelet
- docker
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Container Runtime 是什么
- 如何 Container Runtime
trigger_keywords:
- Container
- Runtime
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Container Runtime

## CRI (Container Runtime Interface)

CRI is the [[gRPC|gRPC]] API between [[kubelet|kubelet]] and container runtime:
- **RuntimeService**: Pod/container lifecycle (RunPodSandbox, CreateContainer, StartContainer)
- **ImageService**: Image management (PullImage, ListImages, RemoveImage)

## Runtime Options

| Runtime | Pros | Cons | Best For |
|---------|------|------|----------|
| **containerd** | Lightweight, performant, rich ecosystem, CN graduated | Debugging needs nerdctl | General production |
| **CRI-O** | Kubernetes-native, minimal dependencies | Smaller feature set, less debugging tooling | Kubernetes-only environments |
| **Docker** (via cri-dockerd) | Rich CLI, familiar tooling | Heavy, deprecated, extra layer | Development only |

## RuntimeClass

RuntimeClass enables selecting different runtimes per Pod:
- **runc**: Standard OCI runtime (default)
- **crun**: Faster, lighter OCI runtime
- **gVisor**: Sandbox for multi-tenant isolation
- **Kata Containers**: VM-level isolation for untrusted workloads

## Evolution

Kubernetes removed the built-in Docker shim (dockershim) in v1.24. Docker images still work -- the image format is OCI-compatible -- but Docker as a runtime requires the external cri-dockerd adapter.

## Related

- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[docker]] — Docker
- [[entities/kubelet.md|kubelet]]
- [[pod-lifecycle|Pod Lifecycle]]
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]

- 21-container-runtime-deep-dive
- 15-container-runtime-interfaces
- [[故障诊断/高级排障/02-node-components/03-container-runtime-troubleshooting.md|03-container-runtime-troubleshooting]]

<!-- risk-assessed -->
