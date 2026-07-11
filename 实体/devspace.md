---
title: DevSpace (entities)
description: '## 概述'
summary: 'DevSpace 是一款开源的 Kubernetes 开发工具，旨在简化云原生应用的开发工作流。它提供热重载、实时同步、远程调试等功能，让开发者可以直接在 Kubernetes 集群中开发和测试应用，而无需在本地环境复现复杂的微服务架构。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- devspace
- crd
- operator
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- DevSpace 是什么
- 如何 DevSpace
trigger_keywords:
- DevSpace
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# DevSpace

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

DevSpace 是由 Loft（原 DevSpace Cloud）开发的开源 Kubernetes 开发工具，2019 年进入 CNCF Sandbox。它的核心理念是"**直接在 Kubernetes 中开发**"——开发者使用 `devspace dev` 命令将本地代码实时同步到集群中的开发 Pod，获得热重载、远程调试和日志聚合能力，无需在本地复现完整的微服务架构。

DevSpace 解决的核心痛点是"本地开发环境与生产集群不一致"问题。开发者只需本地有源码和 DevSpace CLI，代码变更通过文件同步（tar+rsync）实时推送到集群中的 Pod，容器内的文件变更监控器（如 nodemon）自动重启应用。同时支持端口转发、远程断点调试和终端交互。

## Key Features

- **热重载**：代码更改自动同步到集群中的 Pod 并触发应用重启
- **双向文件同步**：通过 rsync 协议在本地和容器间实时同步文件
- **端口转发**：自动管理本地到 Pod 的端口转发
- **远程调试**：支持 VS Code 和 IntelliJ 的远程断点调试
- **日志流**：实时聚合多 Pod 日志到终端
- **Profile 管理**：为不同环境（dev/staging）创建独立配置 Profile

## Architecture

DevSpace 由 **devspace CLI**（本地运行的命令行工具）、**`devspace.yaml`**（项目配置文件，定义镜像、部署、开发策略）和 **DevSpace Helper Pod**（运行在集群中的辅助 Pod，负责文件同步和终端代理）组成。`devspace dev` 命令根据 `devspace.yaml` 配置：构建镜像 → 部署到集群 → 启动文件同步 → 建立端口转发 → 聚合日志，形成一个完整的开发循环。

## K8s 集成

DevSpace 直接使用 Kubernetes API 和 kubeconfig 操作集群。它创建/更新标准 K8s 资源（Deployment、Service），通过 Kubernetes Port Forward API 建立本地到 Pod 的隧道。文件同步使用自定义的 Sync Client 代理，运行在 Pod 的 init container 或 sidecar 中。

## 生产部署要点

- **镜像策略**：使用 `rebuildStrategy` 减少不必要的构建
- **同步排除**：排除 node_modules、.git 等大目录
- **Profile 管理**：为不同环境创建独立 profile
- **依赖顺序**：明确定义服务启动顺序
- **资源限制**：在集群中为开发 Pod 设置资源限制
- **清理策略**：定期使用 `devspace purge` 清理旧资源

## 生产场景

1. **微服务本地开发**：本地开发服务 A，依赖的服务 B/C 在远程集群中运行
2. **远程调试**：VS Code 连接到集群中的 Pod 设置断点调试
3. **团队开发环境**：统一的开发环境配置，新成员零配置上手
4. **多服务联调**：同时开发多个微服务，日志统一聚合

## 安装

```bash
# 安装 DevSpace CLI
brew install loft-sh/tap/devspace
# 或
curl -L -o devspace "https://github.com/loft-sh/devspace/releases/latest/download/devspace-darwin-arm64" && chmod +x devspace && sudo mv devspace /usr/local/bin

# 初始化项目
cd my-project
devspace init    # 生成 devspace.yaml

# 配置 devspace.yaml
cat > devspace.yaml <<EOF
version: v2beta1
name: myapp
deployments:
  app:
    helm:
      chart:
        name: component-chart
        repo: https://charts.devspace.sh
      values:
        containers:
          - image: myapp:latest
dev:
  app:
    imageSelector: myapp:latest
    sync:
      - path: ./:/app
        excludePaths:
          - node_modules
          - .git
    ports:
      - localPort: 3000
        remotePort: 3000
    logs:
      enabled: true
EOF

# 启动开发模式
devspace dev
```

## 对比

| 特性 | DevSpace | Skaffold | Tilt | Telepresence |
|------|----------|----------|------|--------------|
| 热重载 | ✅ | ✅ | ✅ | ❌ |
| 文件同步 | ✅ rsync | ⚠️ 镜像重建 | ✅ | ❌ |
| 远程调试 | ✅ | ⚠️ | ✅ | ⚠️ |
| 本地代理 | ❌ | ❌ | ❌ | ✅ |

## 参考链接

- [[deployment]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[bpfman]] — bpfman
- [[in-toto]] — in-toto
- [[grpc]] — gRPC
- [[kagent]] — Kagent
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- devspace
- [[实体/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
