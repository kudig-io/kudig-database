---
title: Communication between Nodes and the Control Plane（节点与控制平面之间的通信）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- agent
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Communication between Nodes and the Control Plane（节点与控制平面之间的通信） 是什么
- 如何 Communication between Nodes and the Control Plane（节点与控制平面之间的通信）
trigger_keywords:
- Communication
- between
- Nodes
- and
- the
- Control
- Plane
- 节点与控制平面之间的通信
title_en: Nodes
---


# Communication between Nodes and the Control Plane（节点与控制平面之间的通信）

## 概述

本文档梳理了 Kubernetes 集群中 API 服务器与节点之间的所有通信路径，目的是帮助用户根据安全需求自定义网络配置，使集群能够在不受信任的网络（或公有云的公网 IP）上运行。

## 核心概念/原理

Kubernetes 采用“轮毂-辐条（Hub-and-Spoke）”API 模式：
- **所有来自节点的 API 调用都终止于 API 服务器**，其他控制平面组件不对外暴露远程服务。
- API 服务器配置为在安全的 HTTPS 端口（通常是 443）上监听远程连接，并启用一种或多种客户端身份验证方式。

### 节点到控制平面（Node to Control Plane）
- 节点需持有集群的公共根证书和有效的客户端凭证（通常以客户端证书形式提供给 kubelet）。
- Pod 可通过 ServiceAccount 自动注入根证书和有效的 Bearer Token，安全地访问 API 服务器。
- `kubernetes` 服务（default 命名空间）配置了一个虚拟 IP，由 kube-proxy 重定向到 API 服务器的 HTTPS 端点。

### 控制平面到节点（Control Plane to Node）
- **API 服务器 → kubelet**：用于获取 Pod 日志、执行 `kubectl attach`、提供端口转发等功能。默认情况下 API 服务器**不验证** kubelet 的服务证书，存在中间人攻击风险。
- **API 服务器 → 节点/Pod/服务**：默认使用纯 HTTP 连接，未加密也未认证。即使手动添加 `https:` 前缀，也不会验证服务端证书或提供客户端凭证。

## 关键机制或特性

- **SSH 隧道**：API 服务器可以建立到每个节点的 SSH 隧道，将所有发往 kubelet、节点、Pod 或服务的流量通过隧道转发。该机制已废弃，不推荐使用。
- **Konnectivity 服务**：v1.18 起引入的替代方案，提供控制平面到集群通信的 TCP 级代理。包含两部分：
  - **Konnectivity Server**：运行在控制平面网络
  - **Konnectivity Agent**：运行在节点网络，主动连接到 Server 并维持连接
  启用后，所有控制平面到节点的流量均通过此通道。

## 使用场景

- 在公有云或不可信网络上部署 Kubernetes，需要加固节点与控制平面之间的通信
- 需要安全地获取 Pod 日志、进入容器或进行端口转发
- 高安全要求环境下，通过 Konnectivity 服务替代 SSH 隧道

## 最佳实践/注意事项

- 为 kubelet 启用 TLS 引导（TLS Bootstrapping），自动分发客户端证书
- 为 API 服务器配置 `--kubelet-certificate-authority`，验证 kubelet 的服务证书
- 启用 kubelet 的身份验证和授权，保护 kubelet API 不被滥用
- 优先使用 Konnectivity 服务，避免使用已废弃的 SSH 隧道
- 避免在不受信网络上直接暴露 API 服务器到节点/Pod/服务的 HTTP 连接

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/control-plane-node-communication/
