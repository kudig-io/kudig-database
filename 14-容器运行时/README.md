---
title: Container Runtime
description: 容器运行时知识域 — Docker、containerd/CRI-O、镜像管理、镜像构建、运行时迁移、安全沙箱
summary: 容器运行时知识域入口，涵盖 Docker 架构与运维、containerd/CRI-O 对比、镜像仓库(Harbor)、多阶段构建、Dockershim 迁移、安全沙箱对比
category: domain
tags:
- docker
- containerd
- container
- image
- registry
- harbor
- supply-chain
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
---
# 容器运行时 Container Runtime

> Docker、containerd/CRI-O、镜像管理、镜像构建与运行时迁移。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[14-容器运行时/01-Docker/index.md\|Docker/]] | Docker 架构与运维 | Docker Engine/Compose/网络/存储/安全 |
| [[14-容器运行时/03-containerd-CRI-O/index.md\|containerd-CRI-O/]] | 轻量运行时 | containerd vs CRI-O、CRI 接口、RuntimeClass |
| [[14-容器运行时/02-镜像管理/index.md\|镜像管理/]] | 镜像仓库 | Harbor/Registry/镜像策略/供应链安全 |
| [[14-容器运行时/04-镜像构建/index.md\|镜像构建/]] | 构建优化 | 多阶段构建/BuildKit/Kaniko/distroless |
| [[14-容器运行时/05-运行时迁移/index.md\|运行时迁移/]] | 迁移实践 | Dockershim 移除/Docker→containerd 迁移 |

## 跨域导航

- [[15-AI基础设施/README.md|AI基础设施]]
- [[16-专项技术/README.md|专项技术]]
- [[18-云厂商/README.md|云厂商]]
- [[11-发布变更/README.md|发布变更]]
- [[09-可观测性/README.md|可观测性]]
- [[12-可靠性/README.md|可靠性]]
- [[06-存储/README.md|存储]]
- [[08-安全/README.md|安全]]
- [[02-工作负载/README.md|工作负载]]
- [[10-平台工程/README.md|平台工程]]
- [[04-应用模式/README.md|应用模式]]
- [[19-故障诊断/README.md|故障诊断]]
- [[07-数据库中间件/README.md|数据库中间件]]
- [[03-清单模式/README.md|清单模式]]
- [[13-生产运维/README.md|生产运维]]
- [[21-生态参考/README.md|生态参考]]
- [[17-系统基础/README.md|系统基础]]
- [[05-网络/README.md|网络]]
- [[01-集群基础/README.md|集群基础]]

## Related

- [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production|Python 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production|Go 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production|Rust 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management|GPU 工作负载管理]]
- [[09-可观测性/00-总览/13-ebpf-observability-deep-dive|eBPF 可观测性深度实践]]
- [[12-可靠性/06-SRE实践/13-ai-workload-reliability|AI 工作负载可靠性]]
- [[19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[27-标签/06-AI与专项/research|research]]
