---
title: Telepresence (entities)
description: '## 概述'
summary: 'Telepresence 是一个 Kubernetes 本地开发工具，它在本地开发环境和远程 Kubernetes 集群之间创建网络隧道。'
category: entities
tags:
- k8s
- cncf
- networking
- telepresence
- containerd
- docker
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Telepresence 是什么
- 如何 Telepresence
trigger_keywords:
- Telepresence
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Telepresence

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Telepresence 是一个 Kubernetes 本地开发工具，由 Ambassador（原 Datawire）开发，2018 年加入 CNCF 沙箱。它在本地开发环境和远程 Kubernetes 集群之间创建透明网络隧道，使开发者可以在本地运行服务的同时，访问集群中的其他服务和资源（ConfigMap、Secret、Service DNS 等）。Telepresence 还支持流量拦截（Intercept），将集群中特定服务的请求重定向到本地进行调试。这极大提升了微服务开发体验——开发者可以在本地用 IDE 调试单个服务，同时与集群中的其他微服务正常通信，无需在集群中构建部署。

## 核心能力

- **流量拦截（Intercept）**: 将 K8s 服务的请求重定向到本地进程，支持基于 Header 的选择性拦截
- **双向网络代理**: 本地访问集群中任意 Service，集群流量也能路由到本地
- **DNS 代理**: 自动解析集群内 Service DNS（如 `my-service.my-namespace.svc.cluster.local`）
- **卷挂载**: 将远程 Pod 的 Volume 挂载到本地文件系统
- **环境变量同步**: 自动同步远程 Pod 的环境变量到本地
- **Docker 模式**: 在本地 Docker 容器中运行服务，确保环境一致性

## 架构

Telepresence 通过流量拦截和网络隧道实现本地-集群通信：

- **Telepresence CLI**: 本地客户端，管理连接和拦截会话
- **Traffic Manager**: 部署在集群中的控制器，协调所有 Traffic Agent
- **Traffic Agent**: 注入到目标 Pod 中的 sidecar，负责流量拦截和路由
- **Intercept（拦截）**: 通过 Traffic Agent 将特定请求（基于 Header 匹配）重定向到本地
- **个人拦截**: 使用唯一 Header 实现多人共享集群但不互相干扰
- **Volume Mount**: 通过 sshfs/FUSE 将 Pod Volume 挂载到本地

数据流（拦截模式）：`客户端 → Service → Traffic Agent (Header 匹配) → 本地进程`

## K8s 集成

Telepresence 的 Traffic Manager 以 Deployment 运行在集群的 `ambassador` 命名空间中。当启用拦截时，Telepresence 通过 Mutating Webhook 向目标 Pod 注入 Traffic Agent sidecar，sidecar 拦截入站流量，根据 Header 规则决定是转发到本地还是原容器。`telepresence connect` 通过 kubeconfig 建立 VPN 隧道，使本地可以访问集群内 Service DNS 和 IP。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Service、ConfigMap、Secret 资源深度集成，本地进程如同运行在集群内。

## 生产场景

1. **微服务本地调试**: 在本地 IDE 中运行并断点调试单个微服务，其余依赖在集群中
2. **生产问题排查**: 拦截生产环境特定请求（基于 Header）到本地进行调试
3. **前端开发联调**: 前端在本地运行，后端 API 直接访问集群中的服务
4. **数据库连接调试**: 本地应用通过 Teleposition 隧道直连集群中的数据库 Service

## 安装

```bash
# 安装 Telepresence CLI
brew install datawire/blackbird/telepresence
# 或
curl -fL https://app.getambassador.io/download/tel2/darwin/amd64/latest/telepresence -o /usr/local/bin/telepresence

# 连接到集群
telepresence connect

# 拦截服务（将集群中 my-service 的流量重定向到本地 8080 端口）
telepresence intercept my-service --port 8080 --env-file ./service.env

# 个人拦截（基于 Header，不影响其他流量）
telepresence intercept my-service --port 8080 --http-match="x-debug-id=my-unique-id"

# 结束拦截
telepresence leave my-service
```

## 对比

| 特性 | Telepresence | DevSpace | Skaffold | Mirrord |
|------|-------------|----------|----------|---------|
| 流量拦截 | ✅ | ❌ | ❌ | ✅ |
| DNS 代理 | ✅ | ❌ | ❌ | ✅ |
| Docker 模式 | ✅ | ✅ | ✅ | ❌ |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，Telepresence 属于 **Networking** 类别，为云原生应用开发提供本地-集群连通能力。

## 参考链接

- [[deployment]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[03-containerd-security-hardening]] — [[containerd|containerd]]rd 安全加固|containerd 安全加固]]
- [[k0s]] — K0s
- [[kubeedge]] — KubeEdge
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- telepresence
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
