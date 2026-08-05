---
title: containerd
description: 容器运行时标签枢纽 — 涵盖 containerd、CRI-O、Docker、CRI 接口、镜像管理、安全沙箱、运行时迁移等全部容器运行时领域知识
category: tag-index
tags:
- containerd
- cri-o
- docker
- container-runtime
- oci
tier: core
difficulty: intermediate-to-advanced
domain: container-runtime
created: '2026-07-11'
last_updated: '2026-07-21'
---

# containerd Tag Hub

> containerd 及容器运行时页面 — 运行时配置、CRI、镜像管理、安全沙箱、运行时迁移等。

## 核心定义

**容器运行时（Container Runtime）** 是 Kubernetes 中负责实际运行容器的底层组件。自 Kubernetes 1.24 移除 Dockershim 后，containerd 和 CRI-O 成为主流选择。它们通过 CRI（Container Runtime Interface）与 kubelet 交互。

### 运行时架构分层

| 层级 | 组件 | 职责 |
|------|------|------|
| CRI 接口 | kubelet ↔ CRI gRPC | Pod/容器生命周期管理 |
| 高级运行时 | containerd / CRI-O | 镜像管理、容器编排 |
| 低级运行时 | runc / crun / kata | 实际创建容器进程 |
| OCI 规范 | runtime-spec / image-spec | 标准化接口 |

### 运行时对比

| 运行时 | 特色 | 适用场景 |
|--------|------|----------|
| containerd | CNCF 毕业、生态完善 | 通用生产环境 |
| CRI-O | 轻量、K8s 专用 | 纯 K8s 环境 |
| Docker | 开发体验好 | 开发/构建（非 K8s 运行时） |
| Kata Containers | VM 级隔离 | 多租户安全 |
| gVisor | 用户态内核 | 不可信工作负载 |


## 容器运行时 (Container Runtime)

- [[14-容器运行时/03-containerd-CRI-O/01-containerd-deep-guide|containerd 深度指南]]
- [[14-容器运行时/00-总览/01-production-readiness-operations-guide|容器运行时生产就绪指南]]
- [[14-容器运行时/06-沙箱运行时/01-gvisor-sandbox-production|gVisor 沙箱生产实践]]
- [[14-容器运行时/06-沙箱运行时/02-firecracker-microvm-guide|Firecracker MicroVM 指南]]

## containerd / CRI-O

- [[14-容器运行时/03-containerd-CRI-O/02-containerd-production-operations|containerd 生产运营]]
- [[14-容器运行时/03-containerd-CRI-O/04-oci-runtimes-comparison|OCI 运行时对比]]
- [[14-容器运行时/03-containerd-CRI-O/05-kata-containers-secure-container|Kata Containers 安全容器]]
- [[14-容器运行时/03-containerd-CRI-O/06-gvisor-sandbox-runtime|gVisor 沙箱运行时]]
- [[14-容器运行时/03-containerd-CRI-O/09-runtime-security-hardening|运行时安全加固]]
- [[14-容器运行时/03-containerd-CRI-O/07-rootless-containers-guide|Rootless 容器指南]]
- [[14-容器运行时/03-containerd-CRI-O/10-containerd-configuration-deep-guide|containerd 配置深度指南]]
- [[14-容器运行时/03-containerd-CRI-O/11-cri-interface-internals|CRI 接口内部机制]]
- [[14-容器运行时/03-containerd-CRI-O/12-container-runtime-lifecycle|容器运行时生命周期]]
- [[14-容器运行时/03-containerd-CRI-O/13-snapshotter-strategies|Snapshotter 策略]]
- [[14-容器运行时/03-containerd-CRI-O/14-nerdctl-production-guide|nerdctl 生产指南]]
- [[14-容器运行时/03-containerd-CRI-O/15-container-shim-v2|Container Shim v2]]

## Docker

- [[14-容器运行时/01-Docker/01-docker-architecture-overview|Docker 架构概述]]
- [[14-容器运行时/01-Docker/03-docker-container-lifecycle|Docker 容器生命周期]]
- [[14-容器运行时/01-Docker/06-docker-compose-orchestration|Docker Compose 编排]]
- [[14-容器运行时/01-Docker/11-docker-automation-devops|Docker 自动化 DevOps]]
- [[14-容器运行时/01-Docker/13-docker-commands-reference|Docker 命令参考]]

## 镜像管理 (Image Management)

- [[14-容器运行时/02-镜像管理/01-harbor-enterprise-image-registry|Harbor 企业级镜像仓库]]
- [[14-容器运行时/02-镜像管理/02-docker-registry-enterprise-distribution|Docker Registry 企业级 Distribution]]
- [[14-容器运行时/02-镜像管理/03-jfrog-artifactory-enterprise|JFrog Artifactory 企业级]]
- [[14-容器运行时/02-镜像管理/04-harbor-enterprise-security-scanning|Harbor 企业级安全扫描]]
- [[14-容器运行时/02-镜像管理/05-quay-enterprise-registry|Quay 企业级仓库]]
- [[14-容器运行时/02-镜像管理/07-amazon-ecr-enterprise|Amazon ECR 企业级]]
- [[14-容器运行时/02-镜像管理/08-harbor-enterprise-guide|Harbor 企业级指南]]

## 镜像构建 (Image Build)

- [[14-容器运行时/04-镜像构建/03-kaniko-ko-build-guide|Kaniko/ko 构建指南]]
- [[14-容器运行时/04-镜像构建/04-multi-arch-build-guide|多架构构建指南]]
- [[14-容器运行时/04-镜像构建/05-distroless-minimal-images|Distroless 最小镜像]]
- [[14-容器运行时/04-镜像构建/06-image-layer-optimization|镜像层优化]]

## 运行时迁移 (Runtime Migration)

- [[14-容器运行时/05-运行时迁移/01-docker-to-containerd-migration|Docker 到 containerd 迁移]]
- [[14-容器运行时/05-运行时迁移/02-containerd-to-cri-o-migration|containerd 到 CRI-O 迁移]]
- [[14-容器运行时/05-运行时迁移/03-runtime-class-configuration|RuntimeClass 配置]]

## 集群基础 (Cluster Fundamentals)

- [[01-集群基础/03-控制平面/21-container-runtime-deep-dive|容器运行时深度指南]]
- [[01-集群基础/03-控制平面/22-container-storage-deep-dive|容器存储深度指南]]
- [[01-集群基础/03-控制平面/30-in-place-pod-resize|原地 Pod 调整]]
- [[01-集群基础/03-控制平面/32-dynamic-resource-allocation|动态资源分配]]
- [[01-集群基础/01-架构总览/08-windows-containers-support|Windows 容器支持]]

## 概念 (Concepts)

- [[22-概念/15-运行时与系统/container-runtime-comparison|容器运行时对比]]
- [[22-概念/12-研究/container-runtime-evolution|容器运行时演进]]
- [[22-概念/15-运行时与系统/containerd-pod-lifecycle|containerd Pod 生命周期]]
- [[22-概念/15-运行时与系统/kubernetes-containerd-integration|Kubernetes containerd 集成]]
- [[22-概念/15-运行时与系统/docker-architecture|Docker 架构]]
- [[22-概念/15-运行时与系统/linux-container-foundation|Linux 容器基础]]
- [[22-概念/15-运行时与系统/etcd-containerd-storage|etcd containerd 存储]]
- [[22-概念/15-运行时与系统/overlayfs-storage|OverlayFS 存储]]
- [[22-概念/13-research-2025-2026/04-Container-Runtime|容器运行时研究]]

## 工作负载 (Workloads)

- [[02-工作负载/01-核心工作负载/15-container-runtime-interfaces|容器运行时接口]]
- [[02-工作负载/01-核心工作负载/16-runtime-class-configuration|RuntimeClass 配置]]
- [[02-工作负载/01-核心工作负载/17-container-images-registry|容器镜像仓库]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/04-高级排障/structural-02-node-components/01-kubelet-troubleshooting|kubelet 排障]]
- [[19-故障诊断/04-高级排障/structural-02-node-components/03-container-runtime-troubleshooting|容器运行时排障]]
- [[19-故障诊断/04-高级排障/structural-02-node-components/05-image-registry-troubleshooting|镜像仓库排障]]
- [[19-故障诊断/04-高级排障/01-node-component-troubleshooting|节点组件排障]]
- [[19-故障诊断/03-基础设施排障/03-image-registry-troubleshooting|镜像仓库基础设施排障]]
- [[19-故障诊断/08-技能体系/11-image-pull-failure|镜像拉取失败]]

## 安全 (Security)

- [[08-安全/03-运行时安全/02-sysdig-enterprise-container-security|Sysdig 容器安全]]
- [[08-安全/03-运行时安全/04-runtime-security-defense|运行时安全防御]]
- [[08-安全/03-运行时安全/06-gvisor-container-sandbox|gVisor 容器沙箱]]
- [[08-安全/05-供应链/13-image-security-scanning|镜像安全扫描]]

## 平台工程 (Platform Engineering)

- [[10-平台工程/06-代码分析/cluster-create/18-cri-runtime|CRI 运行时管理]]
- [[10-平台工程/02-运维/06-automation-toolchain|自动化工具链]]
- [[10-平台工程/02-运维/08-automated-operations-toolchain|自动化运维工具链]]

## 生产运维 (Production Operations)

- [[13-生产运维/07-运维手册/10-node-and-runtime-ops|节点与运行时运维]]

## WebAssembly / 边缘计算

- [[16-专项技术/02-WebAssembly/01-wasm-fundamentals-cloud-native|Wasm 云原生基础]]
- [[16-专项技术/02-WebAssembly/02-containerd-wasm-shim|containerd Wasm Shim]]
- [[16-专项技术/02-WebAssembly/03-spinkube-framework|SpinKube 框架]]
- [[16-专项技术/02-WebAssembly/05-wasmedge-runtime|WasmEdge 运行时]]
- [[16-专项技术/01-边缘计算/01-edge-computing-architecture|边缘计算架构]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/fundamentals/container-runtime|Container Runtime]]
- [[17-系统基础/06-知识字典/fundamentals/containerd|containerd]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-components|Kubernetes 组件]]
- [[17-系统基础/06-知识字典/workloads/container-runtime-interface-cri|CRI]]
- [[17-系统基础/06-知识字典/workloads/runtime-class|RuntimeClass]]
- [[17-系统基础/06-知识字典/workloads/user-namespaces|User Namespaces]]
- [[17-系统基础/06-知识字典/platform-engineering/network-plugins|网络插件]]
- [[17-系统基础/01-Linux/09-linux-container-fundamentals|Linux 容器基础]]
- [[17-系统基础/01-Linux/06-linux-storage-management|Linux 存储管理]]
- [[17-系统基础/05-速查卡/docker|Docker 速查卡]]

## 发布说明 (Release Notes)

- [[23-实体/15-参考与索引/release-notes-core-deps|Core Deps Release Notes]]
- [[23-实体/15-参考与索引/core-deps-changelog|Core Deps Changelog]]

## 实体 (Entities)

- [[23-实体/03-运行时/containerd|containerd]]
- [[23-实体/03-运行时/cri-o|CRI-O]]
- [[23-实体/03-运行时/docker|Docker]]
- [[23-实体/08-交付与制品/harbor|Harbor]]
- [[23-实体/02-K8s核心组件/container-runtime|Container Runtime]]
- [[23-实体/08-交付与制品/distribution|Distribution]]
- [[23-实体/08-交付与制品/dragonfly|Dragonfly]]
- [[23-实体/15-参考与索引/cncf-runtime|CNCF Runtime]]
- [[23-实体/15-参考与索引/k8s-container-linux-fundamentals|K8s Container Linux Fundamentals]]

## 综合 (Synthesis)

- [[24-综合/04-安全与合规/container-runtime-image-security|容器运行时与镜像安全]]

## containerd 技术全景

### containerd 架构

| 组件 | 功能 |
|---|---|
| containerd | 容器运行时守护进程 |
| runc | OCI 运行时 |
| ctr | 调试 CLI |
| crictl | CRI 调试工具 |

### 常用命令

```bash
# 🟢 查看容器
crictl ps
# 🟢 查看镜像
crictl images
# 🟡 拉取镜像
crictl pull <image>
# 🟢 查看日志
crictl logs <container-id>
```

## 面试要点

1. **Q：containerd vs Docker 的区别？**
   A：Docker：完整平台(构建/编排)。containerd：轻量运行时。K8s 1.24+ 移除 dockershim。

2. **Q：CRI 的工作原理？**
   A：kubelet 通过 CRI gRPC 调用 containerd。RuntimeService(容器) + ImageService(镜像)。

3. **Q：containerd 故障排查？**
   A：systemctl status containerd→journalctl→crictl 检查→配置文件→重启。

## Related Tags

- [[27-标签/k8s|k8s]]
- [[27-标签/security|security]]
- [[27-标签/production|production]]
