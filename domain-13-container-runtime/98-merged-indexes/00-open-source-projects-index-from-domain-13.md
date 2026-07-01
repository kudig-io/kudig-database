---
title: Domain-13 Docker — 开源项目索引
description: '| **Docker Engine** | 容器运行时与管理 | Docker/Mirantis | v28.0.0 | - | Apache-2.0
  |'
summary: '| **Docker Engine** | 容器运行时与管理 | Docker/Mirantis | v28.0.0 | - | Apache-2.0
  |'
category: docker
tags:
- docker
- container
- image
- containerd
- cri-o
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- 运维工程师
- SRE
estimated_read_time: 5min
intent_queries:
- Domain-13 Docker — 开源项目索引 是什么
- 如何 Domain-13 Docker — 开源项目索引
- Kubernetes 13 docker 最佳实践
trigger_keywords:
- Domain-13
- Docker
- 开源项目索引
- docker
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/docker.md
  label: '速查卡: docker'
---



# Domain-13 Docker — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Docker Engine** | 容器运行时与管理 | Docker/Mirantis | v28.0.0 | - | Apache-2.0 |
| **Docker Desktop** | 开发者桌面工具 | Docker | v4.40.0 | - | 商业 |
| **containerd** | 行业标准容器运行时 | CNCF Graduated | v2.0.4 | 17k+ | Apache-2.0 |
| **CRI-O** | K8s 专用容器运行时 | CNCF Graduated | v1.33.0 | 5k+ | Apache-2.0 |
| **Podman** | 无守护进程容器工具 | Red Hat (CNCF Sandbox申请) | v5.4.0 | 24k+ | Apache-2.0 |
| **Buildah** | OCI 镜像构建 | Red Hat (CNCF Sandbox申请) | v1.39.0 | 7k+ | Apache-2.0 |
| **Skopeo** | 镜像远程操作 | Red Hat (CNCF Sandbox申请) | v1.17.0 | 8k+ | Apache-2.0 |
| **nerdctl** | containerd CLI | containerd | v2.0.4 | 8k+ | Apache-2.0 |
| **Lima** | macOS/Linux VM 容器 | CNCF Incubating | v1.0.0 | 15k+ | Apache-2.0 |
| **Colima** | macOS 容器运行时 (基于 Lima) | 社区 | v0.8.0 | 18k+ | MIT |
| **Finch** | AWS 容器开发工具 (基于 Lima) | AWS | v1.7.0 | 3k+ | Apache-2.0 |
| **bootc** | OCI 镜像 OS 更新 | Red Hat (CNCF Sandbox申请) | v1.1.0 | 2k+ | Apache-2.0 |
| **composefs** | 只读可挂载文件系统 | Red Hat (CNCF Sandbox申请) | - | - | GPL-2.0+ |
| **runc** | OCI 运行时参考实现 | OCI | v1.2.0 | 12k+ | Apache-2.0 |
| **crun** | C 语言 OCI 运行时 (更快) | Red Hat | v1.20.0 | 3k+ | GPL-2.0+ |
| **youki** | Rust 语言 OCI 运行时 | 社区 | v0.5.0 | 5k+ | Apache-2.0 |
| **dive** | 镜像层分析 | 社区 | v0.13.0 | 47k+ | MIT |
| **trivy** | 镜像漏洞扫描 | Aqua | v0.61.0 | 24k+ | Apache-2.0 |

---

## 运行时对比

| 维度 | Docker | containerd | CRI-O | Podman |
|:---|:---|:---|:---|:---|
| 守护进程 | ✅ | ✅ | ✅ | ❌ (rootless) |
| K8s 原生 | 通过 cri-dockerd | 默认 | 默认 | 通过 cri-o-like |
| 镜像构建 | ✅ (BuildKit) | ❌ (需 buildctl) | ❌ | ✅ (Buildah) |
| Rootless | 有限 | 支持 | 支持 | 原生支持 |
| 桌面开发 | Docker Desktop | nerdctl + Lima | 不适用 | Podman Desktop |
| 协议 | 部分 OCI | 完全 OCI | 完全 OCI | 完全 OCI |

---

## CNCF Sandbox 申请动态 (KubeCon NA 2024)

Red Hat 宣布将以下项目贡献给 CNCF Sandbox:
- **Podman Container Tools**: Podman + Buildah + Skopeo
- **Podman Desktop**: 图形化容器开发工具
- **bootc**: 基于 OCI 镜像的操作系统更新
- **composefs**: 灵活的只读文件系统树

---

## 参考链接

- [Docker 文档](https://docs.docker.com/)
- [containerd 文档](https://containerd.io/docs/)
- [CRI-O 文档](https://cri-o.io/)
- [Podman 文档](https://podman.io/docs/)
- [Lima 文档](https://lima-vm.io/)

---

## Obsidian 相关文档

- domain-13-container-runtime MOC
- [[domain-13-container-runtime/README.md|Docker 容器技术深度解析]]
- Docker 架构概述与核心概念
- Docker 镜像管理详解
- Docker 容器生命周期管理
- Docker 网络深度解析
- Docker 存储与数据卷
- Docker Compose 编排
- Docker 安全最佳实践
- Docker 故障排查指南
- Docker 性能监控与调优
- Docker 日志管理与分析
