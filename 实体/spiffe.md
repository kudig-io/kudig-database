---
title: SPIFFE (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- security
- spiffe
- istio
- crd
- operator
- kubeflow
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIFFE 是什么
- 如何 SPIFFE
trigger_keywords:
- SPIFFE
prerequisites:
- kubectl-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# SPIFFE

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: 规范文档

## 概述

SPIFFE（Secure Production Identity Framework for Everyone）是一个 CNCF 毕业项目，由 Scytale（现 HPE）发起，为云原生环境提供统一的 workload 身份框架。它定义了一套标准来为不同环境中的工作负载（容器、VM、裸机）颁发和验证加密身份。SPIFFE 解决了跨集群、跨云、跨平台工作负载间 mTLS 认证的难题，无需依赖 IP 地址或网络边界。SPIFFE 由两部分组成：SPIFFE 规范（定义身份格式）和 SPIRE（参考实现）。

## Key Features（核心能力）

- **SPIFFE ID**：标准化的工作负载身份标识 URI（如 spiffe://example.org/ns/default/sa/myapp）
- **SVID（SPIFFE Verifiable Identity Document）**：承载身份的凭证，支持 X.509 和 JWT 两种格式
- **Workload API**：为工作负载提供身份获取、信任bundle 更新的 gRPC API
- **联邦信任**：支持跨域信任建立，实现不同信任域间的工作负载互信
- **多节点架构**：支持 Agent-Server 架构，Server 集群提供 HA
- **可扩展的插件体系**：支持多种密钥存储、节点认证、工作负载注册插件

## 架构与工作原理

SPIFFE 的参考实现 SPIRE 采用 Server-Agent 架构：SPIRE Server 负责签发 SVID 和管理信任域，通过可插拔的 KeyManager 存储签名密钥；SPIRE Agent 以 DaemonSet 彐式运行在每个节点，通过 Workload API 为本地进程提供 SVID 和信任 Bundle。Agent 通过节点认证（Node Attestation）证明节点身份，然后为已注册的工作负载签发 SVID。工作负载通过 Unix Domain Socket 访问 Workload API 获取凭证。

## K8s 集成

在 Kubernetes 中，SPIRE Agent 以 DaemonSet 部署到每个节点，通过 K8s ServiceAccount Token 进行节点认证。工作负载通过 Pod 身份（ServiceAccount + Namespace）自动获取对应的 SPIFFE 身份。SPIFFE 可与 Envoy Proxy 集成实现自动 mTLS，也可与 Istio Service Mesh 集成替代自签证书。

## 生产用例

- **Zero Trust 网络**：工作负载间 mTLS 双向认证，无需信任网络边界
- **跨集群服务通信**：不同 K8s 集群中的服务通过 SPIFFE 联邦信任安全通信
- **多云互连**：跨 AWS、GCP、Azure 的服务建立统一身份和信任关系
- **合规要求**：满足零信任架构（ZTA）和金融级安全合规要求

## 安装与快速开始

```bash
helm repo add spiffe https://spiffe.github.io/helm-charts/
helm install spire spiffe/spire -n spire-system --create-namespace
```

## 对比替代方案

相比传统 PKI/CA 系统，SPIFFE 为云原生工作负载设计了自动化身份管理和短期凭证轮转。相比服务网格自带的 mTLS（如 Istio），SPIFFE 提供跨网格、跨平台的统一身份。

## Related

- [[tikv]] — TiKV
- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubeflow]] — Kubeflow
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spiffe
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]
- [[生态参考/领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
