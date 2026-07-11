---
title: ContainerSSH (entities)
description: '## 概述'
summary: 'ContainerSSH 是一个 SSH 服务器，它为每个 SSH 连接动态启动一个容器或 Kubernetes Pod，提供隔离的 shell 环境。用户通过 SSH 连接时，ContainerSSH 调用外部认证服务验证用户身份，然后根据配置为该用户启动专属的容器实例。这种架构非常适合提供安全的沙箱环境、蜜罐系统、CI/CD 执行器或多租户开发环境。'
category: entities
tags:
- k8s
- cncf
- security
- containerssh
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
- ContainerSSH 是什么
- 如何 ContainerSSH
trigger_keywords:
- ContainerSSH
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# ContainerSSH

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

ContainerSSH 是一个 CNCF 沙箱项目，由 Red Hat 和 GESIS 开发，是一个 SSH 服务器，为每个 SSH 连接启动独立的容器作为用户的 Shell 环境。它将 SSH 访问的隔离性提升到容器级别——每个用户登录后获得一个隔离的容器，而非共享主机环境。ContainerSSH 特别适合教育平台、CI/CD 系统和需要为用户提供安全隔离 Shell 访问的场景。

## Key Features（核心能力）

- **每连接容器**：每个 SSH 会话获得独立的容器，会话结束容器销毁
- **多后端支持**：支持 Kubernetes、Docker 作为容器后端
- **配置热加载**：支持运行时动态更新配置
- **审计日志**：记录完整的命令执行审计日志
- **认证集成**：支持密码、公钥、OAuth2 等多种认证方式
- **安全审计**：支持将用户的 TTY 输出录制为审计视频

## 架构与工作原理

ContainerSSH 由 SSH Server 和后端执行器两部分组成。SSH Server 接收客户端 SSH 连接，通过配置的认证后端（HTTP Auth、Password File、OAuth2）验证用户身份。认证通过后，通过 Backend Provider（K8s/Docker）创建一个容器作为用户的 Shell 环境，将 SSH 的 stdin/stdout/stderr 流转发到容器。会话结束后容器自动销毁。

## K8s 集成

当配置 Kubernetes 后端时，ContainerSSH 在用户登录时创建一个 K8s Pod，将 SSH 会话的 TTY 连接到 Pod 的容器进程。Pod 配置（镜像、资源限制、挂载）可通过 ConfigMap 动态定义。通过 RBAC ServiceAccount 控制 ContainerSSH 创建 Pod 的权限。会话结束后 Pod 自动删除。支持通过 PVC 为用户持久化主目录。

## 生产用例

- **教育平台**：为学生提供隔离的编程环境
- **安全 Shell 访问**：为外部用户提供审计的 SSH 访问
- **CI/CD 构建节点**：通过 SSH 提供 CI/CD 构建环境
- **调试环境**：为开发者提供临时的容器化 Shell 环境

## 安装与快速开始

```bash
docker run -p 2222:2222 -v $(pwd)/config.yaml:/config.yaml containerssh/containerssh
# K8s 部署
helm repo add containerssh https://containerssh.github.io/helm-charts/
helm install containerssh containerssh/containerssh -n containerssh --create-namespace
```

## 对比替代方案

相比传统 SSH（共享主机），ContainerSSH 提供容器级隔离，安全性更高。相比 Teleport（SSH 代理），ContainerSSH 不只是代理而是创建全新容器环境。

## Related

- [[easegress]] — Easegress
- [[perses]] — Perses
- [[tremor]] — Tremor
- [[drasi]] — Drasi
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- containerssh
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
