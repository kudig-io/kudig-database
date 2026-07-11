---
title: bootc (entities)
description: '## 概述'
summary: 'bootc 是一个基于 OCI 容器镜像的 Linux 系统启动和升级工具，将容器镜像作为操作系统的部署单元。它允许使用标准的容器构建工具（如 Dockerfile）来定义和构建可启动的 Linux 系统，并通过事务性更新机制实现系统的原子升级和回滚。bootc 将容器工作流的优势（镜像注册中心、版本标签、CI/CD 流水线）引入操作系统管理领域。'
category: entities
tags:
- k8s
- cncf
- runtime
- bootc
- docker
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- bootc 是什么
- 如何 bootc
trigger_keywords:
- bootc
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# bootc

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

bootc（Bootable Container）是由 Red Hat 发起的项目，旨在将"容器即操作系统"的理念落地。它基于 OCI 标准容器镜像作为可引导操作系统的基础镜像，将 Kubernetes 风格的声明式容器工作流延伸到操作系统层面。传统 OS 管理依赖包管理器（rpm/dpkg）逐包安装，而 bootc 则通过拉取/升级整个容器镜像来管理系统——每次升级都是原子性的全量替换，配合 OSTree 或类似事务存储实现无缝回滚。

bootc 项目于 2024 年进入 CNCF Sandbox。它使用 Rust 编写，专注于安全的系统更新机制。用户可以使用标准 Dockerfile 定义系统镜像（从 `bootc/fedora-bootc` 等基础镜像出发），然后通过 `bootc upgrade` 拉取新版本或 `bootc rollback` 回退。

## Key Features

- **容器即系统**：标准 OCI 容器镜像直接作为可启动操作系统，使用 Containerfile/Dockerfile 定义
- **事务性更新**：基于 OSTree 的原子升级和回滚，失败可自动/手动恢复到前一版本
- **声明式配置**：通过 `/usr/lib/bootc/install/` 配置安装参数，零接触安装
- **OCI Registry 集成**：系统镜像通过标准 OCI Registry 分发，支持签名验证和供应链安全
- **多基础镜像**：支持 Fedora、CentOS Stream、RHEL 等基础镜像
- **K8s 节点管理**：可作为 Kubernetes 节点 OS，通过镜像标签统一管理集群节点版本

## Architecture

bootc 架构分为三层：**基础镜像层**（Base Image，如 `quay.io/centos-bootc/centos-bootc`）包含内核和核心系统；**自定义层**（User Layer）由用户的 Dockerfile 定义额外的包和配置；**部署层**（Deployment）通过 `bootc switch/upgrade` 管理。系统使用 OSTree 作为后端存储，每次升级创建新的 deployment，GRUB/systemd-boot 引导时可以选择不同的 deployment。

## K8s 集成

bootc 特别适合作为 Kubernetes 节点操作系统。通过将节点 OS 打包为 OCI 镜像，可以实现：节点池的版本化管理（镜像标签）、CI/CD 驱动的节点滚动升级、与 Cluster API 或 Karpenter 配合实现自动化节点生命周期管理。节点升级时只需更新 `bootc` 镜像引用，新节点自动拉取新版本。

## 生产部署要点

- **基础镜像选择**：使用官方 bootc 基础镜像（Fedora/CentOS bootc）作为起点
- **分层构建**：将通用配置放在基础层，应用特定配置放在上层
- **CI/CD 集成**：将系统镜像构建集成到 CI/CD 流水线，自动测试和发布
- **版本标签**：使用语义版本标签管理系统镜像，保留回滚路径
- **最小化镜像**：只安装必要的包，减小镜像大小和攻击面

## 生产场景

1. **Kubernetes 节点 OS 统一管理**：数百节点使用同一 OCI 镜像，版本一致性有保障
2. **边缘计算零接触部署**：预配置 bootc 镜像刷入设备，通过 Registry 远程升级
3. **安全合规基线**：将安全基线（CIS Benchmark）固化到系统镜像中，确保合规
4. **快速灾难恢复**：系统损坏时直接从 Registry 拉取新镜像恢复，无需手动重建

## 安装

```bash
# 构建自定义 bootc 镜像（Containerfile 示例）
cat > Containerfile <<EOF
FROM quay.io/centos-bootc/centos-bootc:stream9
RUN dnf install -y vim tcpdump && dnf clean all
RUN echo 'nodeserver' > /etc/hostname
EOF
podman build -t my-bootc:v1 .

# 安装到磁盘（裸金属/VM）
bootc install to-disk /dev/sda --img quay.io/myorg/my-bootc:v1

# 运行中系统升级
bootc upgrade              # 拉取新镜像
bootc switch --img quay.io/myorg/my-bootc:v2  # 切换版本
```

## 对比

| 特性 | bootc | Flatcar | Talos Linux | NixOS |
|------|-------|---------|-------------|-------|
| 容器镜像作为 OS | ✅ | ❌ | ❌ | ❌ |
| 回滚机制 | OSTree | A/B 分区 | APID 驱动 | 声明式 |
| Dockerfile 构建 | ✅ | ❌ | ❌ | ❌ |
| K8s 节点优化 | ✅ | ✅ | ✅ | ⚠️ |

## 参考链接

- [[pod-lifecycle]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[实体/cncf-edge-ai.md|cncf-edge-ai]] — CNCF 边缘计算与 AI/ML 项目全景
- [[confidential-containers]] — Confidential Containersrs (CoCo)|Confidential Containers (CoCo)]]
- [[k8sgpt]] — K8sGPT
- [[trickster]] — Trickster
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bootc
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[实体/tetragon.md|Tetragon]] — Cross-reference


<!-- risk-assessed -->
