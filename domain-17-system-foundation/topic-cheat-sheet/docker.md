---
title: Docker
description: '- [[domain-13-container-runtime/README.md|Docker 容器技术深度解析]]'
summary: '- [[domain-13-container-runtime/README.md|Docker 容器技术深度解析]]'
category: entities
tags:
- k8s
- docker
- container
- image
- build
- containerd
- cri-o
- rag
- etcd
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Docker 是什么
- 如何 Docker
trigger_keywords:
- Docker
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Docker

Docker is the platform that popularized containerization. Since K8s v1.24 removed dockershim, Docker is no longer used as a K8s node runtime, but remains essential for development and image building.

## Key Facts

- **Latest Version**: Docker 26.0+
- **Runtime**: dockerd -> containerd -> runc
- **Image Format**: OCI Image Spec (compatible with all OCI runtimes)
- **Build Engine**: BuildKit (multi-stage, cache-efficient)
- **K8s Status**: Deprecated as runtime (v1.20), removed (v1.24)

## Components

| Component | Role |
|-----------|------|
| Docker CLI | User interface (docker command) |
| dockerd | API [[Service|service]], manages images/networks/volumes |
| containerd | Container lifecycle management |
| containerd-shim | Keeps container running when dockerd restarts |
| runc | OCI runtime, creates actual container process |

## Current Best Practice

Use Docker for development and image building. Use containerd or CRI-O for K8s production nodes. Docker-built images run on any OCI-compliant runtime.

## Related

- [[entities/container-runtime.md|container-runtime]] — Container Runtime
- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[concepts/docker-architecture.md|docker-architecture]] — Docker Architecture and Container Runtime
- [[concepts/container-runtime-comparison.md|container-runtime-comparison]] — Container Runtime Comparison
- [[concepts/docker-architecture.md|Docker Architecture]]
- [[concepts/container-runtime-comparison.md|Container Runtime Comparison]]
- [[containerd|containerd]]

- 00-open-source-projects-index
- 02-docker-registry-enterprise-distribution
- 05-docker-storage-volumes
- 11-docker-automation-devops
- [[domain-13-container-runtime/README.md|Docker 容器技术深度解析]]
- 04-docker-networking-deep-dive
- 03-docker-container-lifecycle
- 09-docker-performance-monitoring
- 99-docker-commands-reference
- 07-docker-security-best-practices
- 01-docker-architecture-overview
- domain-13-container-runtime MOC
- 02-docker-images-management
- 12-java-containerization-guide
- 06-docker-compose-orchestration
- 08-docker-troubleshooting-guide
- 10-docker-logging-management
- [[entities/k8s-design-principles-deep-dive.md|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[entities/kudig-contribution-guide.md|贡献指南、项目概览与版本发布说明]] — Cross-reference
- [[entities/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[entities/k8s-cluster-create.md|Kubernetes 集群创建操作指南]] — Cross-reference
- [[entities/k8s-supply-chain-yaml-cheatsheet.md|供应链安全、YAML 配置清单与速查表]] — Cross-reference
- [[entities/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- domain-13-container-runtime MOC — Cross-reference
- [[concepts/cli-tools-evolution.md|CLI 工具演进]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace.md|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/overlayfs-storage.md|OverlayFS Storage]] — Cross-reference
- [[concepts/linux-container-foundation.md|Linux Container Foundation]] — Cross-reference
- [[skills/learn-13-daemonset-basics.md|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[skills/learn-01-what-is-kubernetes.md|第一课：Kubernetes 入门]] — Cross-reference
- [[skills/learn-06-configmap-secret.md|第六课：ConfigMap 和 Secret - 配置管理]] — Cross-reference
- [[skills/learn-02-pod-basics.md|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- Domain-3: Kubernetes控制平面 — Cross-reference
- [[entities/kubernetes-changelog.md|Kubernetes 变更日志索引]] — Cross-reference
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference


<!-- risk-assessed -->
