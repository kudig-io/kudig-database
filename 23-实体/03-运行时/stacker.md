---
title: Stacker (entities)
description: '## 概述'
summary: 'Stacker 是一个无需特权即可构建 OCI 容器镜像的工具。它使用声明式的 YAML 文件（stacker.yaml）定义镜像层，通过 overlay 文件系统构建镜像，无需 Docker daemon 或 root 权限。Stacker 支持可复现构建、内容寻址层缓存和多阶段构建，特别适合 CI/CD 流水线中的安全镜像构建。'
category: entities
tags:
- k8s
- cncf
- image
- stacker
- containerd
- docker
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Stacker 是什么
- 如何 Stacker
trigger_keywords:
- Stacker
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Stacker

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

Stacker 是由 Cisco（Project Stacker 团队）和 Canonical 联合开发的开源 OCI 容器镜像构建工具，2022 年进入 CNCF Sandbox。它通过声明式 YAML 文件（`stacker.yaml`）定义镜像层，使用 overlay 文件系统构建 OCI 标准镜像。Stacker 的核心优势是**无需 Docker daemon 和 root 权限**即可构建镜像，特别适合 CI/CD 环境中的安全构建。

Stacker 与 Docker/BuildKit 的关键区别在于**非特权构建（Unprivileged Build）**。传统容器构建需要 root 权限或 Docker daemon，在多租户 CI/CD 环境中存在安全风险。Stacker 使用 user namespace 和 overlayfs（或 fuse-overlayfs）在用户空间构建镜像，无需特殊权限。它还内置了 LXC/LXD 集成，可以在隔离的容器中运行构建步骤。

## Key Features

- **非特权构建**：无需 root 权限或 Docker daemon，通过 user namespace 构建
- **声明式 YAML**：`stacker.yaml` 定义镜像层、来源和构建步骤
- **内容寻址缓存**：基于内容哈希的层缓存，精确判断缓存有效性
- **多阶段构建**：支持从基础层导入文件，多阶段构建减小最终镜像
- **多种层来源**：从 OCI 镜像、tar 文件、目录导入层
- **可复现构建**：固定版本和校验和，确保构建结果可复现

## Architecture

Stacker 由 **stacker CLI**（命令行工具）、**stacker.yaml**（镜像层定义文件）和 **stacker 工作区**（overlayfs 层叠文件系统）组成。构建时，stacker 为每层创建一个 overlayfs 挂载点（或 fuse-overlayfs 在无特权环境），在该环境中运行构建命令，变更的文件捕获为新的 OCI 层。构建使用 LXC（有 root）或直接用户空间操作（无 root）执行构建步骤。

## K8s 集成

Stacker 构建的 OCI 镜像与标准 Kubernetes 完全兼容。也特别适合在 Kubernetes CI/CD Pod（Tekton、Argo Workflows）中以非特权模式运行——无需 Pod 的 `privileged: true` 或 `securityContext.privileged`，提升流水线安全性。

## 生产部署要点

- **非特权环境**：在 CI/CD 中使用非 root 用户运行 stacker 构建
- **层缓存**：利用 stacker 的层缓存加速 CI/CD 流水线中的重复构建
- **多阶段**：使用多阶段构建减小最终镜像体积
- **锁定版本**：在 from 中使用摘要而非标签锁定基础镜像版本
- **签名**：构建后对镜像签名，确保供应链安全

## 生产场景

1. **CI/CD 安全构建**：非特权 Pod 中构建容器镜像，无需 securityContext
2. **不可变 OS 镜像**：为 bootc/Talos 等不可变 OS 构建自定义镜像层
3. **嵌入式/WASM 构建**：跨平台镜像构建（ARM/RISC-V/WASM）
4. **供应链安全**：内容寻址 + 签名，确保镜像供应链可审计

## 安装与配置

### CLI 安装

```bash
# 下载 stacker 二进制
wget https://github.com/project-stacker/stacker/releases/latest/download/stacker-linux-amd64
chmod +x stacker-linux-amd64 && sudo mv stacker-linux-amd64 /usr/local/bin/stacker

# 验证安装
stacker --version
```

### stacker.yaml 配置

```yaml
# stacker.yaml - 多阶段构建
build-env:
  build-only: true
  from:
    type: docker
    url: docker://golang:1.21-alpine
  run: |
    go build -o /myapp /src/main.go
  import:
    - path: ./src
    - dest: /src

myapp:
  from:
    type: docker
    url: docker://alpine:3.18
  import:
    - path: ./stacker:myapp
    - dest: /usr/bin/myapp
  run: |
    chmod +x /usr/bin/myapp
  entrypoint: ["/usr/bin/myapp"]
  labels:
    org.opencontainers.image.source: "https://github.com/example/myapp"
```

### 构建与发布

```bash
# 构建镜像（非特权模式）
stacker build

# 查看构建结果
stacker inspect myapp

# 推送到 OCI Registry
stacker publish --url docker://registry.example.com --tag latest

# 清理构建缓存
stacker clean
```

## 运维操作

```bash
# 🟢 查看构建历史
stacker inspect <layer-name>

# 🟡 重新构建（无缓存）
stacker build --no-cache

# 🟡 发布到指定 Registry
stacker publish --url docker://harbor.example.com/project --tag v1.0.0

# 🔴 清理所有构建产物
stacker clean --all
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 构建失败: import not found | 导入路径错误 | 检查 stacker.yaml import 配置 | 修正 path 和 dest |
| 发布失败: unauthorized | Registry 认证失败 | `stacker login registry.example.com` | 重新登录 |
| 构建慢 | 缓存未命中 | `stacker build --no-cache` 对比 | 优化 layer 顺序 |
| Overlay 挂载失败 | 内核不支持 | `lsmod \| grep overlay` | 加载 overlay 内核模块 |

**排查流程：**
```
构建失败
├── 检查 stacker.yaml 语法 → stacker build --dry-run
├── 检查导入文件 → ls ./src
├── 检查基础镜像 → stacker inspect build-env
├── 检查磁盘空间 → df -h
└── 检查内核支持 → lsmod | grep overlay
```

## 生产案例

### 案例一：CI/CD 非特权构建

- **场景**: K8s CI 流水线中构建镜像，不能使用特权容器
- **排查**: Docker-in-Docker 需要特权，存在安全风险
- **方案**: 使用 stacker 非特权构建，在普通 Pod 中完成镜像构建
- **效果**: 无需特权容器，构建安全性提升，符合零信任架构

### 案例二：可重现构建

- **场景**: 合规要求镜像构建可重现、可审计
- **排查**: stacker.yaml 声明式配置 + 内容寻址，确保构建可重现
- **方案**: stacker.yaml 纳入 Git 管理，每次构建生成可审计的 layer 链
- **效果**: 满足供应链安全要求，任何构建可完整重现

## 对比

| 特性 | Stacker | Dockerfile/BuildKit | Kaniko | ko | 适用场景 |
|------|---------|---------------------|--------|-----|----------|
| 非 root 构建 | ✅ | ❌ 需 daemon | ✅ | ✅ | Stacker/Kaniko |
| 声明式 YAML | ✅ stacker.yaml | ✅ Dockerfile | ❌ | ❌ | - |
| Overlay 构建 | ✅ | ✅ | ✅ | ❌ | - |
| K8s 友好 | ✅ 非特权 | ⚠️ | ✅ | ✅ | - |
| 多阶段构建 | ✅ | ✅ | ✅ | ⚠️ | - |

## 参考链接

- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[ratify]] — Ratify
- [[container2wasm]] — container2wasm
- [[23-实体/03-运行时/06-containerd-observability.md|observability]]]] — [[containerd|containerd]]rd 可观测性|containerd 可观测性]]
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- stacker
- [[23-实体/15-参考与索引/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
