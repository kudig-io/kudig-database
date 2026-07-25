---
title: CNCF 容器运行时与工具链项目全景
description: '## 概述'
summary: 'CNCF 容器生态覆盖 **容器运行时**、**容器构建**、**开发者工具** 和 **新型运行时（Wasm/Unikernel）** 四大领域。'
category: entities
tags:
- k8s
- cncf
- container-runtime
- build-tools
- developer-tools
- containerd
- cri-o
- docker
- wasm
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
- CNCF 容器运行时与工具链项目全景 是什么
- 如何 CNCF 容器运行时与工具链项目全景
trigger_keywords:
- CNCF
- 容器运行时与工具链项目全景
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNCF 容器运行时与工具链项目全景

> 聚合页面 | 涵盖 26 个 CNCF 容器运行时和工具项目

## 概述

CNCF 容器生态覆盖 **容器运行时**、**容器构建**、**开发者工具** 和 **新型运行时（Wasm/Unikernel）** 四大领域。

---

## 容器运行时（[[22-概念/15-运行时与系统/container-runtime.md|Container Runtime]]）

### [[containerd]] — 毕业项目

containerd 是行业标准容器运行时，K8s 最常用的 CRI 实现。

- **架构**: 插件化设计，v2 版本重构
- **安全加固**: seccomp、AppArmor、rootless
- **迁移升级**: 从 dockershim 迁移
- **多租户**: 资源隔离与配额
- **可观测性**: [[23-实体/03-运行时/06-containerd-observability.md|observability]]|指标与日志]]
- **Windows 支持**: Windows 容器支持
- **灾备**: [[07-containerd-disaster-recovery|状态备份与恢复]]

### [[cri-o]] — 毕业项目

CRI-O 是专为 K8s 设计的轻量级容器运行时。

- 仅实现 CRI 接口，不包含构建功能
- Red Hat OpenShift 默认运行时
- 与 K8s 版本严格对齐

### [[youki]] — 沙箱项目

Youki 是 Rust 编写的 OCI 容器运行时。

### [[kuasar]] — 沙箱项目

Kuasar 是多沙箱容器运行时。

### [[hyperlight]] — 沙箱项目

Hyperlight 提供轻量级无 VM 沙箱。

---

## WebAssembly（Wasm）运行时

### [[wasmedge]] — 沙箱项目

WasmEdge 是轻量级、高性能的 Wasm 运行时。

- 边缘计算和 IoT 场景
- K8s 集成（crun + WasmEdge）

### [[spin]] — 沙箱项目

Spin 是 Fermyon 的 Wasm 应用框架。

### [[spinkube]] — 沙箱项目

SpinKube 在 K8s 上运行 Spin Wasm 应用。

### [[wasmcloud]] — 孵化项目

WasmCloud 是分布式 Wasm 应用平台。

### [[container2wasm]] — 沙箱项目

Container2Wasm 将容器镜像转换为 Wasm 模块。

### [[urunc]] — 沙箱项目

Urunc 是 Unikernel 容器运行时。

---

## 容器构建工具

### [[buildpacks]] — 孵化项目

Cloud Native Buildpacks 自动将源代码转化为 OCI 镜像。

- 无需 Dockerfile
- 自动检测语言和依赖
- 安全加固的构建过程

### [[slimtoolkit]] — 沙箱项目

Slim Toolkit 自动优化和压缩容器镜像。

### [[ko]] — 沙箱项目

ko 专为 Go 应用设计的容器镜像构建工具。

### [[stacker]] — 沙箱项目

Stacker 是 OCI 原生的容器镜像构建工具。

### [[dalec]] — 沙箱项目

Dalec 是跨发行版的包构建框架。

### [[composefs]] — 沙箱项目

ComposeFS 提供只读的可组合文件系统镜像。

### [[bootc]] — 沙箱项目

Bootc 使用 OCI 容器作为操作系统镜像。

---

## 开发者工具

### [[podman-desktop]] — 沙箱项目

Podman Desktop 是桌面容器管理工具。

- 替代 Docker Desktop
- 支持 Podman、Docker 和 Lima 后端

### [[podman-container-tools]] — 沙箱项目

Podman 容器工具链（Buildah、Skopeo 等）。

### [[lima]] — 沙箱项目

Lima 在 macOS/Linux 上运行 Linux 虚拟机（替代 Docker Desktop 的后端）。

### [[kairos]] — 沙箱项目

Kairos 是不可变 Linux OS 框架，使用 OCI 镜像管理 OS。

### [[devspace]] — 沙箱项目

DevSpace 为 K8s 提供开发时的热重载和远程调试。

### [[devfile]] — 沙箱项目

Devfile 定义云原生开发工作区的标准格式。

### [[vscode-kubernetes-tools]] — 沙箱项目

VS Code Kubernetes 扩展工具。

---

## 架构选型建议

| 场景 | 推荐方案 |
|---|---|
| K8s 容器运行时 | containerd（默认）或 CRI-O |
| 无 Dockerfile 构建 | Cloud Native Buildpacks |
| Go 应用构建 | ko |
| 镜像瘦身 | Slim Toolkit |
| Wasm 运行时 | WasmEdge 或 Spin |
| 桌面开发 | Podman Desktop + Lima |

---

## 相关页面

- [[23-实体/15-参考与索引/cncf-networking.md|cncf-networking]] — 网络与服务网格
- [[23-实体/15-参考与索引/cncf-storage.md|cncf-storage]] — 存储与数据库
- [[23-实体/15-参考与索引/cncf-orchestration.md|cncf-orchestration]] — 编排与应用管理
- [[23-实体/02-K8s核心组件/container-runtime.md|container-runtime]] — 容器运行时概念

## Related

- [[23-实体/02-K8s核心组件/container-runtime.md|container-runtime]] — Container Runtime
- [[lima]] — Lima
- [[podman-desktop]] — Podman Desktop
- [[podman-container-tools]] — Podman Desktop
- [[buildpacks]] — Cloud Native Buildpacks


<!-- risk-assessed -->
