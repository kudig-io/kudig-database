---
title: Docker
description: '- [[domain-13-container-runtime/README|Docker 容器技术深度解析]]'
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
created: "2026-05-23"
---

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

- [[entities/container-runtime|container-runtime]] — Container Runtime
- [[containerd]] — containerd
- [[cri-o]] — CRI-O
- [[concepts/docker-architecture|docker-architecture]] — Docker Architecture and Container Runtime
- [[concepts/container-runtime-comparison|container-runtime-comparison]] — Container Runtime Comparison
- [[concepts/docker-architecture|Docker Architecture]]
- [[concepts/container-runtime-comparison|Container Runtime Comparison]]
- [[containerd|containerd]]

- 00-open-source-projects-index
- 02-docker-registry-enterprise-distribution
- 05-docker-storage-volumes
- 11-docker-automation-devops
- [[domain-13-container-runtime/README|Docker 容器技术深度解析]]
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
- [[references/k8s-design-principles-deep-dive|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[references/kudig-contribution-guide|贡献指南、项目概览与版本发布说明]] — Cross-reference
- [[references/k8s-knowledge-map|Kubernetes Knowledge Map]] — Cross-reference
- [[references/k8s-cluster-create|Kubernetes 集群创建操作指南]] — Cross-reference
- [[references/k8s-supply-chain-yaml-cheatsheet|供应链安全、YAML 配置清单与速查表]] — Cross-reference
- [[references/k8s-difficulty-index|Kubernetes Difficulty Index]] — Cross-reference
- domain-13-container-runtime MOC — Cross-reference
- [[concepts/cli-tools-evolution|CLI 工具演进]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/overlayfs-storage|OverlayFS Storage]] — Cross-reference
- [[concepts/linux-container-foundation|Linux Container Foundation]] — Cross-reference
- [[skills/learn-13-daemonset-basics|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[skills/learn-01-what-is-kubernetes|第一课：Kubernetes 入门]] — Cross-reference
- [[skills/learn-06-configmap-secret|第六课：ConfigMap 和 Secret - 配置管理]] — Cross-reference
- [[skills/learn-02-pod-basics|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- Domain-3: Kubernetes控制平面 — Cross-reference
- [[entities/kubernetes-changelog|Kubernetes 变更日志索引]] — Cross-reference
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
