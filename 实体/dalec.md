---
title: Dalec
description: '## 概述'
summary: 'Dalec 是一个声明式的 Linux 系统包构建工具，通过简洁的 YAML 规范定义如何构建 RPM、DEB 等 Linux 包，而无需手动编写 spec 文件或 debian/rules。它基于 BuildKit 构建，能够交叉编译多架构包，支持自动依赖管理、补丁应用、签名等功能。Dalec 特别适合需要将软件打包为多个发行版格式的场景。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- dalec
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dalec 是什么
- 如何 Dalec
trigger_keywords:
- Dalec
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Dalec

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Dalec（Declarative Artifact Lifecycle Engine for Containers）是由 Microsoft（Azure Linux/SBL 团队）开发的开源声明式 Linux 包构建工具，2024 年进入 CNCF Sandbox。它通过简洁的 YAML 规范文件定义如何从源码构建 RPM（RHEL/Fedora/CentOS）、DEB（Debian/Ubuntu）等 Linux 发行版包，而无需手动编写复杂的 RPM spec 文件或 `debian/rules` 脚本。

Dalec 基于 **BuildKit** 构建引擎，利用 Dockerfile 的前端扩展机制（`#syntax` 指令）实现声明式构建。它支持交叉编译多架构包（x86_64、aarch64）、自动依赖管理、补丁应用、GPG 签名等企业级打包需求。Dalec 特别适合需要将同一软件同时打包为多个发行版格式的场景（如开源软件的 CI/CD 多发行版发布）。

## Key Features

- **声明式 YAML 规范**：用 `dalec.spec.yml` 描述包元数据、源码、依赖、构建步骤和测试
- **多格式输出**：一次定义生成 RPM 和 DEB 包，无需维护两套构建脚本
- **BuildKit 引擎**：利用 BuildKit 的并行构建、缓存和远程执行能力
- **交叉编译**：支持在 x86 主机上构建 ARM 包，多架构包一次生成
- **自动依赖管理**：声明 build-time 和 runtime 依赖，Dalec 自动安装
- **签名与验证**：支持 GPG 签名和包完整性验证

## Architecture

Dalec 作为 BuildKit 的自定义前端工作。开发者创建 `dalec.spec.yml` 文件，然后通过 `docker build -f dalec.spec.yml .` 命令触发构建。Dalec 前端解析 YAML 规范，将其翻译为一系列 BuildKit 步骤（拉取基础镜像、安装构建依赖、编译源码、生成包文件）。每个目标格式（RPM/DEB）有对应的构建模板，最终输出构建好的包文件。

## K8s 集成

Dalec 本身不直接依赖 Kubernetes，但它非常适合在 Kubernetes CI/CD 管道中运行。通过 Tekton、Argo Workflows 或 GitHub Actions 在 K8s Pod 中执行 Dalec 构建，利用 BuildKit 的分布式缓存能力加速构建。也支持通过 OCI 镜像形式分发构建好的包，与 Kubernetes 节点镜像更新流程（如 bootc）配合。

## 生产部署要点

- **源码固定**：使用 Git commit hash 而非分支名固定源码版本
- **依赖最小化**：只声明实际需要的运行时依赖
- **测试覆盖**：为每个包编写基本测试确保安装和运行正常
- **CI/CD 集成**：将 Dalec 构建集成到流水线，自动构建多发行版包
- **版本策略**：使用 revision 字段区分同一上游版本的不同打包版本

## 生产场景

1. **多发行版软件发布**：开源软件 CI 中一次性生成 RPM 和 DEB 包
2. **内部工具打包**：企业内部工具打包为系统包分发到生产服务器
3. **K8s 节点 OS 包管理**：为 bootc/Talos 等不可变 OS 构建自定义包
4. **安全补丁分发**：快速构建和分发安全补丁包到多发行版环境

## 安装

```bash
# Dalec 作为 BuildKit 前端，无需单独安装
# 创建 dalec.spec.yml
cat > dalec.spec.yml <<EOF
syntax: ghcr.io/azure/dalec/frontend:latest
---
name: myapp
version: 1.0.0
revision: 1
description: My application
license: Apache-2.0
sources:
  -:
    ref: https://github.com/myorg/myapp/archive/v1.0.0.tar.gz
build:
  steps:
    - command: make
      dest_dir: /usr/bin
dependencies:
  runtime:
    - glibc
tests:
  - name: binary-exists
    files:
      - /usr/bin/myapp
EOF

# 使用 Docker BuildKit 构建
DOCKER_BUILDKIT=1 docker build -f dalec.spec.yml --target rpm -t myapp-rpm .
```

## 对比

| 特性 | Dalec | fpm | rpmbuild | dpkg-buildpackage |
|------|-------|-----|----------|-------------------|
| 声明式 YAML | ✅ | ✅ Ruby DSL | ❌ spec 文件 | ❌ debian/rules |
| 多格式输出 | ✅ RPM+DEB | ✅ 多格式 | ❌ RPM only | ❌ DEB only |
| BuildKit 缓存 | ✅ | ❌ | ❌ | ❌ |
| 交叉编译 | ✅ | ⚠️ | ⚠️ | ⚠️ |

## 参考链接

- [[概念/declarative-api.md|declarative-api]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[pipecd]] — PipeCD
- [[hami]] — HAMI
- [[open-policy-containers]] — [[实体/open-policy-containers.md|Open Policy Containers (OPCR)]]
- [[werf]] — werf
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- dalec
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
